#pragma once

// ============================================================
// audio_capture.h - WASAPI loopback audio capture interface
//
// Uses the Windows Audio Session API (WASAPI) in loopback mode
// to capture whatever audio is currently playing through the
// default output device (speakers / headphones).
//
// No external libraries - only Windows SDK headers.
// Required link libraries: ole32, uuid
// ============================================================

// Windows headers (must come before COM headers)
// WIN32_LEAN_AND_MEAN and NOMINMAX are set via CMakeLists.txt
#include <windows.h>
#include <mmdeviceapi.h>    // IMMDeviceEnumerator, IMMDevice
#include <audioclient.h>    // IAudioClient, IAudioCaptureClient

#include <atomic>
#include <thread>

// ============================================================
// AudioCapture
//
// Owns the WASAPI loopback session and fills an internal
// ring buffer with mono float32 samples on a background thread.
// Call GetSamples() from the main thread to consume samples.
// ============================================================
class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    // Initialize COM, find the default render endpoint, set up
    // WASAPI loopback capture, and start the background thread.
    // Returns false and prints an error message on failure.
    bool Init();

    // Stop capture, release all COM resources, un-init COM.
    void Shutdown();

    // Sample rate of the captured stream (e.g. 44100, 48000).
    DWORD GetSampleRate() const { return m_sampleRate; }

    // Number of channels in the captured stream (usually 2).
    WORD  GetChannels()   const { return m_channels;   }

    // Copy up to 'count' mono float32 samples into 'buf'.
    // Samples are averaged across all channels → mono.
    // Returns the actual number of samples copied (may be less
    // than 'count' if the ring buffer doesn't have enough data).
    int GetSamples(float* buf, int count);

private:
    // Background thread: drains the WASAPI packet queue and
    // writes interleaved mono samples into the ring buffer.
    void CaptureThread();

    // ---- COM / WASAPI objects ----
    IMMDeviceEnumerator*  m_pEnumerator    = nullptr;
    IMMDevice*            m_pDevice        = nullptr;
    IAudioClient*         m_pAudioClient   = nullptr;
    IAudioCaptureClient*  m_pCaptureClient = nullptr;
    WAVEFORMATEX*         m_pWaveFormat    = nullptr;

    // ---- Format info ----
    DWORD m_sampleRate = 0;
    WORD  m_channels   = 0;
    bool  m_isFloat    = false;   // true = IEEE float32, false = PCM int16

    // ---- Lock-free single-producer / single-consumer ring buffer ----
    // Sized to hold ~1.3 seconds at 48 kHz stereo (mono after mixing).
    static constexpr int RING_SIZE = 1 << 17;  // 131072 samples
    float              m_ringBuffer[RING_SIZE] = {};
    std::atomic<int>   m_writePos{0};
    std::atomic<int>   m_readPos{0};

    // ---- Background capture thread ----
    std::thread      m_captureThread;
    std::atomic<bool> m_running{false};
};
