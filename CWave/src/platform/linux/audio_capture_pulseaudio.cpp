// ============================================================
// platform/linux/audio_capture_pulseaudio.cpp
//
// PulseAudio monitor capture for Linux.
//
// How it works:
//   PulseAudio exposes a "monitor source" for every output sink.
//   Recording from this monitor gives us the exact audio mix
//   being sent to the speakers — the Linux equivalent of
//   WASAPI loopback on Windows or CoreAudio tap on macOS.
//
// Thread model:
//   pa_threaded_mainloop runs its own event-processing thread.
//   The StreamReadCallback fires on that thread whenever audio
//   data is available, converts to mono, and writes to the
//   lock-free ring buffer.  The main thread reads samples via
//   GetSamples() — identical to the other platform backends.
//
// PipeWire compatibility:
//   PipeWire's pipewire-pulse service implements the full
//   libpulse API.  This code works unchanged on PipeWire.
// ============================================================

#include "audio_capture_pulseaudio.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

// ============================================================
AudioCapturePulseAudio::AudioCapturePulseAudio()  = default;
AudioCapturePulseAudio::~AudioCapturePulseAudio() { Shutdown(); }

// ============================================================
// Static callbacks
// ============================================================

void AudioCapturePulseAudio::ContextStateCallback(pa_context* ctx, void* userdata)
{
    auto* self = static_cast<AudioCapturePulseAudio*>(userdata);
    pa_context_state_t state = pa_context_get_state(ctx);

    switch (state) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            pa_threaded_mainloop_signal(self->m_mainloop, 0);
            break;
        default:
            break;
    }
}

void AudioCapturePulseAudio::ServerInfoCallback(pa_context* /*ctx*/,
                                                const pa_server_info* info,
                                                void* userdata)
{
    auto* self = static_cast<AudioCapturePulseAudio*>(userdata);

    if (info && info->default_sink_name)
        self->m_defaultSinkName = info->default_sink_name;

    pa_threaded_mainloop_signal(self->m_mainloop, 0);
}

void AudioCapturePulseAudio::SinkInfoCallback(pa_context* /*ctx*/,
                                              const pa_sink_info* info,
                                              int eol, void* userdata)
{
    auto* self = static_cast<AudioCapturePulseAudio*>(userdata);

    if (eol > 0) {
        pa_threaded_mainloop_signal(self->m_mainloop, 0);
        return;
    }

    if (info) {
        if (info->monitor_source_name)
            self->m_monitorSourceName = info->monitor_source_name;
        self->m_sampleRate = info->sample_spec.rate;
    }

    pa_threaded_mainloop_signal(self->m_mainloop, 0);
}

void AudioCapturePulseAudio::StreamStateCallback(pa_stream* stream, void* userdata)
{
    auto* self = static_cast<AudioCapturePulseAudio*>(userdata);
    pa_stream_state_t state = pa_stream_get_state(stream);

    switch (state) {
        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(self->m_mainloop, 0);
            break;
        default:
            break;
    }
}

void AudioCapturePulseAudio::StreamReadCallback(pa_stream* stream,
                                                size_t /*nbytes*/,
                                                void* userdata)
{
    auto* self = static_cast<AudioCapturePulseAudio*>(userdata);

    const void* data = nullptr;
    size_t length = 0;

    // Drain all available data
    while (pa_stream_peek(stream, &data, &length) >= 0 && length > 0) {

        if (data != nullptr) {
            // Normal audio data — convert multi-channel to mono
            const auto* samples = static_cast<const float*>(data);
            size_t numFrames = length / (sizeof(float) * self->m_channels);

            int wp = self->m_writePos.load(std::memory_order_relaxed);

            for (size_t frame = 0; frame < numFrames; frame++) {
                float mono = 0.0f;
                for (uint16_t ch = 0; ch < self->m_channels; ch++)
                    mono += samples[frame * self->m_channels + ch];
                mono /= static_cast<float>(self->m_channels);

                self->m_ringBuffer[wp] = mono;
                wp = (wp + 1) & (RING_SIZE - 1);
            }

            self->m_writePos.store(wp, std::memory_order_release);
        }
        // else: data == nullptr with length > 0 is a "hole" (silence gap).
        // We skip it — the main loop handles silence by zeroing magnitudes.

        pa_stream_drop(stream);
    }
}

// ============================================================
// Init
// ============================================================
bool AudioCapturePulseAudio::Init()
{
    // ----------------------------------------------------------
    // Step 1: Create the threaded mainloop
    // ----------------------------------------------------------
    m_mainloop = pa_threaded_mainloop_new();
    if (!m_mainloop) {
        fprintf(stderr, "[AudioCapture] Failed to create PulseAudio mainloop.\n");
        return false;
    }

    // ----------------------------------------------------------
    // Step 2: Create the context (server connection handle)
    // ----------------------------------------------------------
    pa_mainloop_api* api = pa_threaded_mainloop_get_api(m_mainloop);
    m_context = pa_context_new(api, "CWave");
    if (!m_context) {
        fprintf(stderr, "[AudioCapture] Failed to create PulseAudio context.\n");
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
        return false;
    }

    pa_context_set_state_callback(m_context, ContextStateCallback, this);

    // ----------------------------------------------------------
    // Step 3: Start the mainloop thread and connect
    // ----------------------------------------------------------
    if (pa_threaded_mainloop_start(m_mainloop) < 0) {
        fprintf(stderr, "[AudioCapture] Failed to start PulseAudio mainloop.\n");
        pa_context_unref(m_context);
        m_context = nullptr;
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
        return false;
    }

    pa_threaded_mainloop_lock(m_mainloop);

    if (pa_context_connect(m_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        fprintf(stderr, "[AudioCapture] pa_context_connect failed: %s\n",
                pa_strerror(pa_context_errno(m_context)));
        pa_threaded_mainloop_unlock(m_mainloop);
        Shutdown();
        return false;
    }

    // Wait for the context to reach READY state
    for (;;) {
        pa_context_state_t state = pa_context_get_state(m_context);
        if (state == PA_CONTEXT_READY) break;
        if (!PA_CONTEXT_IS_GOOD(state)) {
            fprintf(stderr, "[AudioCapture] PulseAudio connection failed.\n");
            pa_threaded_mainloop_unlock(m_mainloop);
            Shutdown();
            return false;
        }
        pa_threaded_mainloop_wait(m_mainloop);
    }

    // ----------------------------------------------------------
    // Step 4: Query server info to get the default sink name
    // ----------------------------------------------------------
    pa_operation* op = pa_context_get_server_info(m_context,
                                                  ServerInfoCallback, this);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainloop);
    pa_operation_unref(op);

    if (m_defaultSinkName.empty()) {
        fprintf(stderr,
            "[AudioCapture] Could not find a default audio output device.\n"
            "Check with: pactl list sinks short\n");
        pa_threaded_mainloop_unlock(m_mainloop);
        Shutdown();
        return false;
    }

    // ----------------------------------------------------------
    // Step 5: Query sink info for monitor source name & sample rate
    // ----------------------------------------------------------
    op = pa_context_get_sink_info_by_name(m_context,
                                          m_defaultSinkName.c_str(),
                                          SinkInfoCallback, this);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainloop);
    pa_operation_unref(op);

    if (m_monitorSourceName.empty() || m_sampleRate == 0) {
        fprintf(stderr,
            "[AudioCapture] Could not query sink '%s'.\n",
            m_defaultSinkName.c_str());
        pa_threaded_mainloop_unlock(m_mainloop);
        Shutdown();
        return false;
    }

    fprintf(stdout, "[AudioCapture] Sink    : %s\n", m_defaultSinkName.c_str());
    fprintf(stdout, "[AudioCapture] Monitor : %s\n", m_monitorSourceName.c_str());
    fprintf(stdout, "[AudioCapture] Format  : %u Hz, stereo, float32\n", m_sampleRate);

    // ----------------------------------------------------------
    // Step 6: Create and connect the recording stream
    // ----------------------------------------------------------
    m_channels = 2;  // stereo capture, mixed to mono in callback

    pa_sample_spec sampleSpec{};
    sampleSpec.format   = PA_SAMPLE_FLOAT32LE;
    sampleSpec.rate     = m_sampleRate;
    sampleSpec.channels = m_channels;

    m_stream = pa_stream_new(m_context, "CWave Capture", &sampleSpec, nullptr);
    if (!m_stream) {
        fprintf(stderr, "[AudioCapture] Failed to create PulseAudio stream.\n");
        pa_threaded_mainloop_unlock(m_mainloop);
        Shutdown();
        return false;
    }

    pa_stream_set_state_callback(m_stream, StreamStateCallback, this);
    pa_stream_set_read_callback(m_stream, StreamReadCallback, this);

    // Request low-latency buffer (~1024 frames)
    pa_buffer_attr bufAttr{};
    bufAttr.maxlength = static_cast<uint32_t>(-1);
    bufAttr.fragsize  = 1024 * sizeof(float) * m_channels;

    if (pa_stream_connect_record(m_stream,
                                 m_monitorSourceName.c_str(),
                                 &bufAttr,
                                 PA_STREAM_ADJUST_LATENCY) < 0) {
        fprintf(stderr, "[AudioCapture] Failed to connect stream to monitor: %s\n",
                pa_strerror(pa_context_errno(m_context)));
        pa_threaded_mainloop_unlock(m_mainloop);
        Shutdown();
        return false;
    }

    // Wait for the stream to become ready
    for (;;) {
        pa_stream_state_t state = pa_stream_get_state(m_stream);
        if (state == PA_STREAM_READY) break;
        if (!PA_STREAM_IS_GOOD(state)) {
            fprintf(stderr, "[AudioCapture] PulseAudio stream failed.\n");
            pa_threaded_mainloop_unlock(m_mainloop);
            Shutdown();
            return false;
        }
        pa_threaded_mainloop_wait(m_mainloop);
    }

    // ----------------------------------------------------------
    // Step 7: Unlock — audio now flows via StreamReadCallback
    // ----------------------------------------------------------
    pa_threaded_mainloop_unlock(m_mainloop);

    return true;
}

// ============================================================
// Shutdown
// ============================================================
void AudioCapturePulseAudio::Shutdown()
{
    if (!m_mainloop) return;

    pa_threaded_mainloop_lock(m_mainloop);

    if (m_stream) {
        pa_stream_disconnect(m_stream);
        pa_stream_unref(m_stream);
        m_stream = nullptr;
    }

    if (m_context) {
        pa_context_disconnect(m_context);
        pa_context_unref(m_context);
        m_context = nullptr;
    }

    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_stop(m_mainloop);
    pa_threaded_mainloop_free(m_mainloop);
    m_mainloop = nullptr;
}

// ============================================================
// GetSamples - lock-free consumer (main thread)
// ============================================================
int AudioCapturePulseAudio::GetSamples(float* buf, int count)
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
    return new AudioCapturePulseAudio();
}
