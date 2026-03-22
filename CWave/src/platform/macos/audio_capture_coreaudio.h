#ifndef CWAVE_AUDIO_CAPTURE_COREAUDIO_H
#define CWAVE_AUDIO_CAPTURE_COREAUDIO_H

// ============================================================
// platform/macos/audio_capture_coreaudio.h
//
// CoreAudio (AudioQueue) input capture for macOS.
// Captures from the default input device.
//
// For loopback (capturing system audio output), install the
// free BlackHole virtual audio driver and set it as the default
// input device. The app will then capture system audio exactly
// as WASAPI loopback does on Windows.
//
// Required frameworks: AudioToolbox, CoreFoundation
// ============================================================

#include "../../audio_capture.h"

#include <AudioToolbox/AudioToolbox.h>
#include <atomic>

class AudioCaptureCoreAudio final : public AudioCaptureBase
{
public:
    AudioCaptureCoreAudio();
    ~AudioCaptureCoreAudio() override;

    bool     Init()     override;
    void     Shutdown() override;

    uint32_t GetSampleRate() const override { return m_sampleRate; }
    uint16_t GetChannels()   const override { return m_channels;   }

    // Copy up to 'count' mono float32 samples into 'buf'.
    // Samples are averaged across all channels → mono.
    // Returns the actual number of samples copied (may be less).
    int GetSamples(float* buf, int count) override;

private:
    // AudioQueue input callback — called on CoreAudio's internal thread.
    // Static forwarder casts clientData back to 'this'.
    static void Callback(void*                               userData,
                         AudioQueueRef                       queue,
                         AudioQueueBufferRef                 buffer,
                         const AudioTimeStamp*               startTime,
                         UInt32                              numPackets,
                         const AudioStreamPacketDescription* packetDesc);

    AudioQueueRef m_queue      = nullptr;
    uint32_t      m_sampleRate = 0;
    uint16_t      m_channels   = 0;

    // ---- Lock-free single-producer / single-consumer ring buffer ----
    // Sized to hold ~1.3 seconds at 48 kHz (mono after mixing).
    static constexpr int RING_SIZE = 1 << 17;  // 131072 samples
    float            m_ringBuffer[RING_SIZE] = {};
    std::atomic<int> m_writePos{0};
    std::atomic<int> m_readPos{0};

    static constexpr int NUM_BUFFERS    = 3;
    static constexpr int BUFFER_FRAMES  = 1024;
};

#endif // CWAVE_AUDIO_CAPTURE_COREAUDIO_H
