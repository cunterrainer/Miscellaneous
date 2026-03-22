#ifndef CWAVE_AUDIO_CAPTURE_WASAPI_H
#define CWAVE_AUDIO_CAPTURE_WASAPI_H

// ============================================================
// platform/windows/audio_capture_wasapi.h
//
// WASAPI loopback audio capture for Windows.
// Taps the default render (output) endpoint in loopback mode
// so we capture exactly what the user hears.
//
// Required link libraries: ole32, uuid
// ============================================================

#include "../../audio_capture.h"

// Windows headers (WIN32_LEAN_AND_MEAN / NOMINMAX set via CMake)
#include <windows.h>
#include <mmdeviceapi.h>   // IMMDeviceEnumerator, IMMDevice
#include <audioclient.h>   // IAudioClient, IAudioCaptureClient

#include <atomic>
#include <thread>

class AudioCaptureWASAPI final : public AudioCaptureBase
{
public:
    AudioCaptureWASAPI();
    ~AudioCaptureWASAPI() override;

    bool     Init()     override;
    void     Shutdown() override;

    uint32_t GetSampleRate() const override { return m_sampleRate; }
    uint16_t GetChannels()   const override { return m_channels;   }

    // Copy up to 'count' mono float32 samples into 'buf'.
    // Samples are averaged across all channels → mono.
    // Returns the actual number of samples copied (may be less).
    int GetSamples(float* buf, int count) override;

private:
    // Background thread: drains the WASAPI packet queue and
    // writes mono samples into the lock-free ring buffer.
    void CaptureThread();

    // ---- COM / WASAPI objects ----
    IMMDeviceEnumerator*  m_pEnumerator    = nullptr;
    IMMDevice*            m_pDevice        = nullptr;
    IAudioClient*         m_pAudioClient   = nullptr;
    IAudioCaptureClient*  m_pCaptureClient = nullptr;
    WAVEFORMATEX*         m_pWaveFormat    = nullptr;

    // ---- Format info ----
    uint32_t m_sampleRate = 0;
    uint16_t m_channels   = 0;
    bool     m_isFloat    = false;   // true = IEEE float32, false = PCM int16

    // ---- Lock-free single-producer / single-consumer ring buffer ----
    // Sized to hold ~1.3 seconds at 48 kHz stereo (mono after mixing).
    static constexpr int RING_SIZE = 1 << 17;  // 131072 samples
    float              m_ringBuffer[RING_SIZE] = {};
    std::atomic<int>   m_writePos{0};
    std::atomic<int>   m_readPos{0};

    // ---- Background capture thread ----
    std::thread       m_captureThread;
    std::atomic<bool> m_running{false};
};

#endif // CWAVE_AUDIO_CAPTURE_WASAPI_H
