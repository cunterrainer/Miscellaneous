// ============================================================
// audio_capture.cpp - WASAPI loopback capture implementation
//
// How WASAPI loopback works:
//   - Normally IAudioClient captures from a microphone (render→ADC).
//   - With AUDCLNT_STREAMFLAGS_LOOPBACK, the same pipeline taps
//     the signal going TO the speakers BEFORE the DAC.
//   - This gives us exactly what the user hears, in the same
//     format the audio engine uses internally (usually IEEE float32,
//     stereo, 44100 Hz or 48000 Hz).
//
// Thread model:
//   - CaptureThread() runs on a dedicated background thread.
//   - It polls WASAPI ~every 5 ms and drains available packets
//     into a lock-free ring buffer.
//   - The main thread calls GetSamples() to pull from the ring
//     buffer without blocking.
// ============================================================

#include "audio_capture.h"

// Needed for WAVEFORMATEXTENSIBLE and its SubFormat GUIDs
#include <mmreg.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

// GUID for IEEE float subformat (KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
// Defined inline so we don't need ksmedia.h
static const GUID kIEEE_FLOAT_GUID =
    { 0x00000003, 0x0000, 0x0010,
      { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

// Convenience macro: log the error and return false from Init()
#define CHECKHR(hr, msg)                                                \
    if (FAILED(hr)) {                                                   \
        fprintf(stderr, "[AudioCapture] %s  (hr=0x%08X)\n", msg, (unsigned)hr); \
        return false;                                                   \
    }

// ============================================================
AudioCapture::AudioCapture()  = default;
AudioCapture::~AudioCapture() { Shutdown(); }

// ============================================================
// Init
// ============================================================
bool AudioCapture::Init()
{
    HRESULT hr;

    // ----------------------------------------------------------
    // 1. Initialize COM on this thread (WASAPI is COM-based).
    //    RPC_E_CHANGED_MODE means COM is already initialised with
    //    a compatible apartment model - treat as success.
    // ----------------------------------------------------------
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        fprintf(stderr, "[AudioCapture] CoInitializeEx failed  (hr=0x%08X)\n", (unsigned)hr);
        return false;
    }

    // ----------------------------------------------------------
    // 2. Create a device enumerator - the entry point for
    //    discovering audio endpoints (speakers, headphones, etc.)
    // ----------------------------------------------------------
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&m_pEnumerator));
    CHECKHR(hr, "CoCreateInstance(MMDeviceEnumerator) failed");

    // ----------------------------------------------------------
    // 3. Get the system's default audio OUTPUT endpoint.
    //    eRender   = output device (speakers / headphones)
    //    eConsole  = the device used for game/media audio
    //    We capture from a render device in loopback mode.
    // ----------------------------------------------------------
    hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
    if (FAILED(hr)) {
        fprintf(stderr,
            "[AudioCapture] GetDefaultAudioEndpoint failed (hr=0x%08X).\n"
            "  Make sure an audio output device is connected and enabled.\n",
            (unsigned)hr);
        return false;
    }

    // ----------------------------------------------------------
    // 4. Activate an IAudioClient on that device.
    //    IAudioClient is the main WASAPI interface - it controls
    //    the audio stream lifecycle.
    // ----------------------------------------------------------
    hr = m_pDevice->Activate(
        __uuidof(IAudioClient), CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&m_pAudioClient));
    CHECKHR(hr, "IMMDevice::Activate(IAudioClient) failed");

    // ----------------------------------------------------------
    // 5. Query the engine's native mix format.
    //    This is the format WASAPI uses internally - usually
    //    IEEE float32, stereo, 44100 or 48000 Hz.  We must use
    //    this exact format in shared-mode loopback.
    // ----------------------------------------------------------
    hr = m_pAudioClient->GetMixFormat(&m_pWaveFormat);
    CHECKHR(hr, "IAudioClient::GetMixFormat failed");

    m_sampleRate = m_pWaveFormat->nSamplesPerSec;
    m_channels   = m_pWaveFormat->nChannels;

    // Determine if samples are IEEE float or integer PCM.
    // The mix format tag is either WAVE_FORMAT_IEEE_FLOAT (0x0003)
    // or WAVE_FORMAT_EXTENSIBLE (0xFFFE), which carries a SubFormat GUID.
    if (m_pWaveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        m_isFloat = true;
    } else if (m_pWaveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        auto* pEx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_pWaveFormat);
        m_isFloat = (memcmp(&pEx->SubFormat, &kIEEE_FLOAT_GUID,
                            sizeof(GUID)) == 0);
    } else {
        // Assume 16-bit PCM integer (older devices / unusual config)
        m_isFloat = false;
    }

    fprintf(stdout,
        "[AudioCapture] Format: %u Hz, %u ch, %u-bit %s\n",
        m_sampleRate, m_channels,
        m_pWaveFormat->wBitsPerSample,
        m_isFloat ? "IEEE float32" : "PCM int16");

    // ----------------------------------------------------------
    // 6. Initialize the audio client in SHARED + LOOPBACK mode.
    //    AUDCLNT_STREAMFLAGS_LOOPBACK  - tap the render pipeline
    //    Buffer duration: 100 ms (= 1 000 000 * 100-ns ticks)
    // ----------------------------------------------------------
    REFERENCE_TIME hnsBufferDuration = 1000000; // 100 ms
    hr = m_pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        hnsBufferDuration,
        0,                   // periodicity - 0 for shared mode
        m_pWaveFormat,
        nullptr);
    CHECKHR(hr, "IAudioClient::Initialize(LOOPBACK) failed");

    // ----------------------------------------------------------
    // 7. Obtain the capture client interface.
    //    IAudioCaptureClient lets us read available audio packets.
    // ----------------------------------------------------------
    hr = m_pAudioClient->GetService(
        __uuidof(IAudioCaptureClient),
        reinterpret_cast<void**>(&m_pCaptureClient));
    CHECKHR(hr, "IAudioClient::GetService(IAudioCaptureClient) failed");

    // ----------------------------------------------------------
    // 8. Start the audio stream, then launch the capture thread.
    // ----------------------------------------------------------
    hr = m_pAudioClient->Start();
    CHECKHR(hr, "IAudioClient::Start failed");

    m_running = true;
    m_captureThread = std::thread(&AudioCapture::CaptureThread, this);

    return true;
}

// ============================================================
// Shutdown - stop capture, release all resources
// ============================================================
void AudioCapture::Shutdown()
{
    // Signal the capture thread to exit and wait for it
    m_running = false;
    if (m_captureThread.joinable())
        m_captureThread.join();

    // Stop the WASAPI stream
    if (m_pAudioClient)
        m_pAudioClient->Stop();

    // Release COM interfaces in reverse acquisition order
    if (m_pCaptureClient) { m_pCaptureClient->Release(); m_pCaptureClient = nullptr; }
    if (m_pAudioClient)   { m_pAudioClient->Release();   m_pAudioClient   = nullptr; }
    if (m_pDevice)        { m_pDevice->Release();        m_pDevice        = nullptr; }
    if (m_pEnumerator)    { m_pEnumerator->Release();    m_pEnumerator    = nullptr; }

    // Free the format structure allocated by GetMixFormat
    if (m_pWaveFormat) { CoTaskMemFree(m_pWaveFormat); m_pWaveFormat = nullptr; }

    CoUninitialize();
}

// ============================================================
// CaptureThread - background thread that drains WASAPI packets
// ============================================================
void AudioCapture::CaptureThread()
{
    // Raise this thread's priority slightly for low-latency capture
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    while (m_running) {

        // Query how many frames are available in the next packet
        UINT32 packetSize = 0;
        HRESULT hr = m_pCaptureClient->GetNextPacketSize(&packetSize);
        if (FAILED(hr)) break;

        // Drain all available packets in this polling interval
        while (packetSize > 0) {

            BYTE*  pData      = nullptr;
            UINT32 numFrames  = 0;
            DWORD  flags      = 0;

            // Lock the next packet
            hr = m_pCaptureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            // -----------------------------------------------
            // Convert multi-channel frames → mono float32
            // and write into the ring buffer.
            // -----------------------------------------------
            for (UINT32 frame = 0; frame < numFrames; frame++) {

                float sample = 0.0f;

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    // WASAPI signals silence explicitly; don't read pData
                    sample = 0.0f;

                } else if (m_isFloat) {
                    // IEEE float32: sizeof(float) = 4 bytes per sample/channel
                    const float* fData = reinterpret_cast<const float*>(pData);
                    for (WORD ch = 0; ch < m_channels; ch++)
                        sample += fData[frame * m_channels + ch];
                    sample /= static_cast<float>(m_channels);

                } else {
                    // 16-bit signed PCM fallback
                    const short* sData = reinterpret_cast<const short*>(pData);
                    for (WORD ch = 0; ch < m_channels; ch++)
                        sample += sData[frame * m_channels + ch] / 32768.0f;
                    sample /= static_cast<float>(m_channels);
                }

                // Write sample to ring buffer (single producer)
                int wp = m_writePos.load(std::memory_order_relaxed);
                m_ringBuffer[wp] = sample;
                m_writePos.store((wp + 1) & (RING_SIZE - 1),
                                 std::memory_order_release);
            }

            // Unlock the packet - WASAPI can reuse the memory
            m_pCaptureClient->ReleaseBuffer(numFrames);

            // Check next packet
            hr = m_pCaptureClient->GetNextPacketSize(&packetSize);
            if (FAILED(hr)) { packetSize = 0; }
        }

        // Sleep to yield CPU; 5 ms keeps latency low without spinning
        Sleep(5);
    }
}

// ============================================================
// GetSamples - called from main thread to pull samples
// ============================================================
int AudioCapture::GetSamples(float* buf, int count)
{
    int wp = m_writePos.load(std::memory_order_acquire);
    int rp = m_readPos.load(std::memory_order_relaxed);

    // How many samples are in the ring buffer?
    int available = (wp - rp + RING_SIZE) & (RING_SIZE - 1);
    int toRead    = std::min(available, count);

    // Copy samples, wrapping around the ring buffer boundary
    for (int i = 0; i < toRead; i++)
        buf[i] = m_ringBuffer[(rp + i) & (RING_SIZE - 1)];

    m_readPos.store((rp + toRead) & (RING_SIZE - 1),
                    std::memory_order_release);
    return toRead;
}
