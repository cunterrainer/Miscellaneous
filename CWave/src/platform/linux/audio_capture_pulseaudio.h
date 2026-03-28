#ifndef CWAVE_AUDIO_CAPTURE_PULSEAUDIO_H
#define CWAVE_AUDIO_CAPTURE_PULSEAUDIO_H

// ============================================================
// platform/linux/audio_capture_pulseaudio.h
//
// PulseAudio monitor audio capture for Linux.
// Taps the default sink's monitor source to capture exactly
// what the user hears (system audio loopback).
//
// Works on both PulseAudio and PipeWire (via pipewire-pulse).
// Required pkg-config: libpulse
// ============================================================

#include "../../audio_capture.h"

#include <pulse/pulseaudio.h>

#include <atomic>
#include <string>

class AudioCapturePulseAudio final : public AudioCaptureBase
{
public:
    AudioCapturePulseAudio();
    ~AudioCapturePulseAudio() override;

    bool     Init()     override;
    void     Shutdown() override;

    uint32_t GetSampleRate() const override { return m_sampleRate; }
    uint16_t GetChannels()   const override { return m_channels;   }

    // Copy up to 'count' mono float32 samples into 'buf'.
    // Samples are averaged across all channels → mono.
    // Returns the actual number of samples copied (may be less).
    int GetSamples(float* buf, int count) override;

private:
    // ---- PulseAudio async callbacks ----
    static void ContextStateCallback(pa_context* ctx, void* userdata);
    static void ServerInfoCallback(pa_context* ctx,
                                   const pa_server_info* info,
                                   void* userdata);
    static void SinkInfoCallback(pa_context* ctx,
                                 const pa_sink_info* info,
                                 int eol, void* userdata);
    static void StreamStateCallback(pa_stream* stream, void* userdata);
    static void StreamReadCallback(pa_stream* stream,
                                   size_t nbytes, void* userdata);

    // ---- PulseAudio objects ----
    pa_threaded_mainloop* m_mainloop = nullptr;
    pa_context*           m_context  = nullptr;
    pa_stream*            m_stream   = nullptr;

    // ---- Discovered at init time ----
    std::string m_monitorSourceName;
    std::string m_defaultSinkName;

    // ---- Format info ----
    uint32_t m_sampleRate = 0;
    uint16_t m_channels   = 0;

    // ---- Lock-free single-producer / single-consumer ring buffer ----
    // Sized to hold ~1.3 seconds at 48 kHz stereo (mono after mixing).
    static constexpr int RING_SIZE = 1 << 17;  // 131072 samples
    float              m_ringBuffer[RING_SIZE] = {};
    std::atomic<int>   m_writePos{0};
    std::atomic<int>   m_readPos{0};
};

#endif // CWAVE_AUDIO_CAPTURE_PULSEAUDIO_H
