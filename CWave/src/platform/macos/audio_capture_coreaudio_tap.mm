// ============================================================
// platform/macos/audio_capture_coreaudio_tap.mm
//
// Native macOS 14.2+ system-audio loopback using CATapDescription.
//
// Architecture:
//   1. CATapDescription describes a stereo mixdown of ALL processes.
//   2. AudioHardwareCreateProcessTap creates a tap object.
//   3. AudioHardwareCreateAggregateDevice wraps the tap in an
//      aggregate device (tap-only, no sub-devices needed).
//   4. AudioDeviceCreateIOProcID + AudioDeviceStart on the aggregate
//      device. The tap's audio arrives in inInputData of the callback.
//   5. The callback averages channels to mono, writes to ring buffer.
//
// No virtual audio driver required. Requires macOS 14.2+.
// ============================================================

#include "../../audio_capture.h"

#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/CATapDescription.h>
#import <CoreAudio/AudioHardwareTapping.h>
#import <Foundation/Foundation.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ============================================================
// AudioCaptureCoreAudioTap
// ============================================================
class AudioCaptureCoreAudioTap final : public AudioCaptureBase
{
public:
    AudioCaptureCoreAudioTap();
    ~AudioCaptureCoreAudioTap() override;

    bool     Init()     override;
    void     Shutdown() override;

    uint32_t GetSampleRate() const override { return m_sampleRate; }
    uint16_t GetChannels()   const override { return m_channels;   }
    int      GetSamples(float* buf, int count) override;

private:
    static OSStatus IOProcCallback(AudioDeviceID                device,
                                   const AudioTimeStamp*        now,
                                   const AudioBufferList*       inInputData,
                                   const AudioTimeStamp*        inInputTime,
                                   AudioBufferList*             outOutputData,
                                   const AudioTimeStamp*        outOutputTime,
                                   void*                        clientData);

    AudioObjectID       m_tapID       = kAudioObjectUnknown;
    AudioObjectID       m_aggDevID    = kAudioObjectUnknown;
    AudioDeviceIOProcID m_ioProcID    = nullptr;
    uint32_t            m_sampleRate  = 0;
    uint16_t            m_channels    = 2;
    bool                m_nonInterleaved = false;

    __strong CATapDescription* m_tapDesc = nil;

    static constexpr int RING_SIZE = 1 << 17;
    float            m_ringBuffer[RING_SIZE]{};
    std::atomic<int> m_writePos{0};
    std::atomic<int> m_readPos{0};
};

// ============================================================
AudioCaptureCoreAudioTap::AudioCaptureCoreAudioTap()  = default;
AudioCaptureCoreAudioTap::~AudioCaptureCoreAudioTap() { Shutdown(); }

// ============================================================
// Init
// ============================================================
bool AudioCaptureCoreAudioTap::Init()
{
    OSStatus err;

    // ----------------------------------------------------------
    // 1. Create the process tap.
    //    Stereo global tap excluding no processes = capture everything.
    //    privateTap = only visible to this process.
    //    CATapUnmuted = user continues to hear audio normally.
    // ----------------------------------------------------------
    m_tapDesc = [[CATapDescription alloc] initStereoGlobalTapButExcludeProcesses:@[]];
    m_tapDesc.muteBehavior = CATapUnmuted;
    m_tapDesc.privateTap = YES;
    m_tapDesc.exclusive = YES;
    m_tapDesc.name = @"CWave System Tap";

    err = AudioHardwareCreateProcessTap(m_tapDesc, &m_tapID);
    if (err != noErr) {
        fprintf(stderr,
            "[AudioCapture] AudioHardwareCreateProcessTap failed (err=%d).\n"
            "  Requires macOS 14.2+. Use -DCWAVE_MACOS_NATIVE_LOOPBACK=OFF for older systems.\n",
            (int)err);
        m_tapDesc = nil;
        return false;
    }

    // ----------------------------------------------------------
    // 2. Query the tap's format (sample rate, channels, interleaving).
    // ----------------------------------------------------------
    {
        AudioObjectPropertyAddress fmtAddr = {
            kAudioTapPropertyFormat,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        AudioStreamBasicDescription tapFmt{};
        UInt32 dataSize = sizeof(tapFmt);
        err = AudioObjectGetPropertyData(m_tapID, &fmtAddr,
                                         0, nullptr, &dataSize, &tapFmt);
        if (err == noErr && tapFmt.mSampleRate > 0) {
            m_sampleRate     = static_cast<uint32_t>(tapFmt.mSampleRate);
            m_channels       = static_cast<uint16_t>(tapFmt.mChannelsPerFrame);
            m_nonInterleaved = (tapFmt.mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;
        } else {
            m_sampleRate     = 48000;
            m_channels       = 2;
            m_nonInterleaved = false;
        }
    }

    fprintf(stdout,
        "[AudioCapture] Format: %u Hz, %u ch, 32-bit float (%s) (system tap)\n",
        m_sampleRate, m_channels,
        m_nonInterleaved ? "non-interleaved" : "interleaved");

    // ----------------------------------------------------------
    // 3. Get the tap's UID string for the aggregate device dictionary.
    // ----------------------------------------------------------
    CFStringRef tapUID = nullptr;
    {
        AudioObjectPropertyAddress uidAddr = {
            kAudioTapPropertyUID,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 dataSize = sizeof(tapUID);
        err = AudioObjectGetPropertyData(m_tapID, &uidAddr,
                                         0, nullptr, &dataSize, &tapUID);
        if (err != noErr || tapUID == nullptr) {
            fprintf(stderr, "[AudioCapture] Could not get tap UID (err=%d).\n", (int)err);
            goto cleanup_tap;
        }
    }

    // ----------------------------------------------------------
    // 4. Create an aggregate device containing only the tap.
    //    No sub-devices needed — the tap provides its own input
    //    stream with the captured system audio.
    // ----------------------------------------------------------
    {
        NSDictionary* aggDesc = @{
            @(kAudioAggregateDeviceNameKey):       @"CWave System Tap",
            @(kAudioAggregateDeviceUIDKey):        @"com.cwave.systap.agg",
            @(kAudioAggregateDeviceIsPrivateKey):  @YES,
            @(kAudioAggregateDeviceTapListKey):    @[
                @{
                    @(kAudioSubTapUIDKey): (__bridge NSString*)tapUID,
                    @(kAudioSubTapDriftCompensationKey): @YES,
                },
            ],
        };

        err = AudioHardwareCreateAggregateDevice(
            (__bridge CFDictionaryRef)aggDesc, &m_aggDevID);

        CFRelease(tapUID);
        tapUID = nullptr;

        if (err != noErr || m_aggDevID == kAudioObjectUnknown) {
            fprintf(stderr,
                "[AudioCapture] AudioHardwareCreateAggregateDevice failed (err=%d).\n", (int)err);
            goto cleanup_tap;
        }
    }

    // ----------------------------------------------------------
    // 5. Register IOProc on the aggregate device and start.
    // ----------------------------------------------------------
    err = AudioDeviceCreateIOProcID(static_cast<AudioDeviceID>(m_aggDevID),
                                    IOProcCallback, this, &m_ioProcID);
    if (err != noErr) {
        fprintf(stderr,
            "[AudioCapture] AudioDeviceCreateIOProcID failed (err=%d).\n", (int)err);
        goto cleanup_agg;
    }

    err = AudioDeviceStart(static_cast<AudioDeviceID>(m_aggDevID), m_ioProcID);
    if (err != noErr) {
        fprintf(stderr,
            "[AudioCapture] AudioDeviceStart failed (err=%d).\n", (int)err);
        AudioDeviceDestroyIOProcID(static_cast<AudioDeviceID>(m_aggDevID), m_ioProcID);
        m_ioProcID = nullptr;
        goto cleanup_agg;
    }

    return true;

cleanup_agg:
    AudioHardwareDestroyAggregateDevice(m_aggDevID);
    m_aggDevID = kAudioObjectUnknown;
cleanup_tap:
    if (tapUID) { CFRelease(tapUID); tapUID = nullptr; }
    AudioHardwareDestroyProcessTap(m_tapID);
    m_tapID   = kAudioObjectUnknown;
    m_tapDesc = nil;
    return false;
}

// ============================================================
// Shutdown
// ============================================================
void AudioCaptureCoreAudioTap::Shutdown()
{
    if (m_ioProcID && m_aggDevID != kAudioObjectUnknown) {
        AudioDeviceStop(static_cast<AudioDeviceID>(m_aggDevID), m_ioProcID);
        AudioDeviceDestroyIOProcID(static_cast<AudioDeviceID>(m_aggDevID), m_ioProcID);
        m_ioProcID = nullptr;
    }

    if (m_aggDevID != kAudioObjectUnknown) {
        AudioHardwareDestroyAggregateDevice(m_aggDevID);
        m_aggDevID = kAudioObjectUnknown;
    }

    if (m_tapID != kAudioObjectUnknown) {
        AudioHardwareDestroyProcessTap(m_tapID);
        m_tapID = kAudioObjectUnknown;
    }

    m_tapDesc = nil;
}

// ============================================================
// IOProcCallback - called on CoreAudio's real-time thread.
// ============================================================
OSStatus AudioCaptureCoreAudioTap::IOProcCallback(
    AudioDeviceID          /*device*/,
    const AudioTimeStamp*  /*now*/,
    const AudioBufferList* inInputData,
    const AudioTimeStamp*  /*inInputTime*/,
    AudioBufferList*       /*outOutputData*/,
    const AudioTimeStamp*  /*outOutputTime*/,
    void*                  clientData)
{
    auto* self = static_cast<AudioCaptureCoreAudioTap*>(clientData);

    if (!inInputData || inInputData->mNumberBuffers == 0) return noErr;

    if (self->m_nonInterleaved) {
        const UInt32 numFrames = inInputData->mBuffers[0].mDataByteSize / sizeof(float);
        const UInt32 numCh     = inInputData->mNumberBuffers;

        for (UInt32 frame = 0; frame < numFrames; frame++) {
            float mono = 0.0f;
            for (UInt32 ch = 0; ch < numCh; ch++) {
                const float* data =
                    static_cast<const float*>(inInputData->mBuffers[ch].mData);
                mono += data[frame];
            }
            mono /= static_cast<float>(numCh);

            int wp = self->m_writePos.load(std::memory_order_relaxed);
            self->m_ringBuffer[wp] = mono;
            self->m_writePos.store((wp + 1) & (RING_SIZE - 1),
                                   std::memory_order_release);
        }
    } else {
        const float* samples   = static_cast<const float*>(inInputData->mBuffers[0].mData);
        const UInt32 numFrames = inInputData->mBuffers[0].mDataByteSize
                                 / (sizeof(float) * self->m_channels);

        for (UInt32 frame = 0; frame < numFrames; frame++) {
            float mono = 0.0f;
            for (uint16_t ch = 0; ch < self->m_channels; ch++)
                mono += samples[frame * self->m_channels + ch];
            mono /= static_cast<float>(self->m_channels);

            int wp = self->m_writePos.load(std::memory_order_relaxed);
            self->m_ringBuffer[wp] = mono;
            self->m_writePos.store((wp + 1) & (RING_SIZE - 1),
                                   std::memory_order_release);
        }
    }

    return noErr;
}

// ============================================================
// GetSamples - called from main thread to pull samples
// ============================================================
int AudioCaptureCoreAudioTap::GetSamples(float* buf, int count)
{
    int wp = m_writePos.load(std::memory_order_acquire);
    int rp = m_readPos.load(std::memory_order_relaxed);

    int available = (wp - rp + RING_SIZE) & (RING_SIZE - 1);
    int toRead    = std::min(available, count);

    for (int i = 0; i < toRead; i++)
        buf[i] = m_ringBuffer[(rp + i) & (RING_SIZE - 1)];

    m_readPos.store((rp + toRead) & (RING_SIZE - 1),
                    std::memory_order_release);
    return toRead;
}

// ============================================================
// Factory
// ============================================================
AudioCaptureBase* AudioCapture::Create()
{
    return new AudioCaptureCoreAudioTap();
}
