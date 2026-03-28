#ifndef CWAVE_AUDIO_CAPTURE_H
#define CWAVE_AUDIO_CAPTURE_H

// ============================================================
// audio_capture.h - Platform-agnostic audio capture interface
//
// Defines AudioCaptureBase, the abstract interface that all
// platform backends implement.
//
// Platform implementations live in:
//   platform/windows/audio_capture_wasapi.*       (WASAPI loopback)
//   platform/macos/audio_capture_coreaudio.*      (CoreAudio input)
//   platform/linux/audio_capture_pulseaudio.*     (PulseAudio monitor)
//
// Usage:
//   std::unique_ptr<AudioCaptureBase> audio(AudioCapture::Create());
//   audio->Init();
//   audio->GetSampleRate();
//   audio->GetSamples(buf, count);
//   audio->Shutdown();
// ============================================================

#include <cstdint>

// ============================================================
// AudioCaptureBase - interface all platform implementations satisfy
// ============================================================
class AudioCaptureBase
{
public:
    virtual ~AudioCaptureBase() = default;

    // Initialize the audio backend and start capture.
    // Returns false and prints an error message on failure.
    virtual bool Init() = 0;

    // Stop capture and release all audio resources.
    virtual void Shutdown() = 0;

    // Sample rate of the captured stream (e.g. 44100, 48000).
    virtual uint32_t GetSampleRate() const = 0;

    // Number of channels in the captured stream (usually 2).
    virtual uint16_t GetChannels() const = 0;

    // Copy up to 'count' mono float32 samples into 'buf'.
    // Samples are averaged across all channels → mono.
    // Returns the actual number of samples copied (may be less
    // than 'count' if the ring buffer doesn't have enough data).
    virtual int GetSamples(float* buf, int count) = 0;
};

// ============================================================
// Factory - implemented in the platform-specific .cpp file.
// Returns a heap-allocated concrete implementation.
// The caller owns the returned pointer.
// ============================================================
namespace AudioCapture
{
    AudioCaptureBase* Create();
}

#endif // CWAVE_AUDIO_CAPTURE_H
