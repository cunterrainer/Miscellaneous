// ============================================================
// platform/macos/audio_capture_coreaudio.cpp
//
// CoreAudio AudioQueue input capture implementation.
//
// How it works:
//   - AudioQueueNewInput creates an input queue targeting the
//     default input device.
//   - CoreAudio calls our Callback on its own real-time thread
//     each time a buffer of samples is ready.
//   - The callback averages channels to mono and writes samples
//     into a lock-free ring buffer (same scheme as WASAPI impl).
//   - GetSamples() drains the ring buffer from the main thread.
//
// For system-audio loopback:
//   Install BlackHole (https://github.com/ExistingSound/BlackHole)
//   and set it as the macOS default input device. Audio routed
//   through BlackHole will then be captured here.
// ============================================================

#include "audio_capture_coreaudio.h"
#include "../../audio_capture.h"

#include <CoreAudio/CoreAudio.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ============================================================
AudioCaptureCoreAudio::AudioCaptureCoreAudio()  = default;
AudioCaptureCoreAudio::~AudioCaptureCoreAudio() { Shutdown(); }

// ============================================================
// Init
// ============================================================
bool AudioCaptureCoreAudio::Init()
{
    // ----------------------------------------------------------
    // 1. Get the default input device ID.
    // ----------------------------------------------------------
    AudioDeviceID deviceID = kAudioObjectUnknown;
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 dataSize = sizeof(deviceID);
    OSStatus err = AudioObjectGetPropertyData(
        kAudioObjectSystemObject, &addr, 0, nullptr, &dataSize, &deviceID);

    if (err != noErr || deviceID == kAudioObjectUnknown) {
        fprintf(stderr,
            "[AudioCapture] Could not find a default input device (err=%d).\n"
            "  Make sure a microphone or input device is connected.\n"
            "  For loopback capture, install BlackHole and set it as default input.\n",
            (int)err);
        return false;
    }

    // ----------------------------------------------------------
    // 2. Use a fixed 44100 Hz sample rate.
    //    Apple Silicon built-in microphones report a native rate of
    //    16000 Hz, which would cap the FFT display at 8 kHz (Nyquist).
    //    Requesting 44100 Hz explicitly lets CoreAudio's AudioQueue
    //    perform sample-rate conversion transparently, keeping Nyquist
    //    at 22050 Hz so the full 20 kHz range is visible.
    // ----------------------------------------------------------
    m_sampleRate = 44100;

    // ----------------------------------------------------------
    // 3. Query the device's input channel count.
    // ----------------------------------------------------------
    AudioObjectPropertyAddress chanAddr = {
        kAudioDevicePropertyStreamConfiguration,
        kAudioDevicePropertyScopeInput,
        kAudioObjectPropertyElementMain
    };
    dataSize = 0;
    AudioObjectGetPropertyDataSize(deviceID, &chanAddr, 0, nullptr, &dataSize);

    uint16_t channels = 2;  // fallback
    if (dataSize > 0) {
        AudioBufferList* bufList = reinterpret_cast<AudioBufferList*>(malloc(dataSize));
        if (bufList) {
            if (AudioObjectGetPropertyData(deviceID, &chanAddr, 0, nullptr,
                                           &dataSize, bufList) == noErr) {
                UInt32 total = 0;
                for (UInt32 i = 0; i < bufList->mNumberBuffers; i++)
                    total += bufList->mBuffers[i].mNumberChannels;
                if (total > 0)
                    channels = static_cast<uint16_t>(total);
            }
            free(bufList);
        }
    }
    m_channels = channels;

    fprintf(stdout,
        "[AudioCapture] Format: %u Hz, %u ch, 32-bit IEEE float32\n",
        m_sampleRate, m_channels);

    // ----------------------------------------------------------
    // 4. Build AudioStreamBasicDescription for interleaved float32.
    //    We request this format; CoreAudio will convert if needed.
    // ----------------------------------------------------------
    AudioStreamBasicDescription fmt{};
    fmt.mSampleRate       = static_cast<Float64>(m_sampleRate);
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    fmt.mChannelsPerFrame = m_channels;
    fmt.mBytesPerFrame    = sizeof(float) * m_channels;
    fmt.mFramesPerPacket  = 1;
    fmt.mBytesPerPacket   = fmt.mBytesPerFrame;
    fmt.mBitsPerChannel   = 32;

    // ----------------------------------------------------------
    // 5. Create the input AudioQueue.
    // ----------------------------------------------------------
    err = AudioQueueNewInput(
        &fmt,
        Callback,
        this,
        nullptr,              // run loop (nullptr = CoreAudio manages its own)
        kCFRunLoopCommonModes,
        0,
        &m_queue);

    if (err != noErr) {
        fprintf(stderr, "[AudioCapture] AudioQueueNewInput failed (err=%d)\n", (int)err);
        return false;
    }

    // ----------------------------------------------------------
    // 6. Allocate and enqueue input buffers.
    // ----------------------------------------------------------
    const UInt32 bufferBytes = BUFFER_FRAMES * fmt.mBytesPerFrame;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueBufferRef buf = nullptr;
        err = AudioQueueAllocateBuffer(m_queue, bufferBytes, &buf);
        if (err != noErr) {
            fprintf(stderr, "[AudioCapture] AudioQueueAllocateBuffer failed (err=%d)\n", (int)err);
            AudioQueueDispose(m_queue, true);
            m_queue = nullptr;
            return false;
        }
        AudioQueueEnqueueBuffer(m_queue, buf, 0, nullptr);
    }

    // ----------------------------------------------------------
    // 7. Start the queue.
    // ----------------------------------------------------------
    err = AudioQueueStart(m_queue, nullptr);
    if (err != noErr) {
        fprintf(stderr, "[AudioCapture] AudioQueueStart failed (err=%d)\n", (int)err);
        AudioQueueDispose(m_queue, true);
        m_queue = nullptr;
        return false;
    }

    return true;
}

// ============================================================
// Shutdown
// ============================================================
void AudioCaptureCoreAudio::Shutdown()
{
    if (m_queue) {
        AudioQueueStop(m_queue, true);    // true = synchronous stop
        AudioQueueDispose(m_queue, true); // true = synchronous dispose
        m_queue = nullptr;
    }
}

// ============================================================
// Callback - called on CoreAudio's real-time thread
// ============================================================
void AudioCaptureCoreAudio::Callback(
    void*                               userData,
    AudioQueueRef                       queue,
    AudioQueueBufferRef                 buffer,
    const AudioTimeStamp*               /*startTime*/,
    UInt32                              /*numPackets*/,
    const AudioStreamPacketDescription* /*packetDesc*/)
{
    auto* self = static_cast<AudioCaptureCoreAudio*>(userData);

    const float*  samples   = static_cast<const float*>(buffer->mAudioData);
    const UInt32  numFrames = buffer->mAudioDataByteSize
                              / (sizeof(float) * self->m_channels);

    // Average channels to mono and write into ring buffer (single producer).
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

    // Re-enqueue the buffer so CoreAudio can fill it again.
    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

// ============================================================
// GetSamples - called from main thread to pull samples
// ============================================================
int AudioCaptureCoreAudio::GetSamples(float* buf, int count)
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
// Factory - returns the macOS CoreAudio implementation
// ============================================================
AudioCaptureBase* AudioCapture::Create()
{
    return new AudioCaptureCoreAudio();
}
