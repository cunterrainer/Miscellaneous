// ============================================================
// main.cpp - Audio Visualizer entry point & main loop
//
// Orchestration:
//   1. Init audio capture (WASAPI loopback on default output)
//   2. Init terminal visualizer
//   3. Accumulate samples into an overlap-add FFT window
//   4. Apply Hann window → run FFT → compute magnitudes
//   5. Map magnitudes → log-scale bands → render frame
//   6. Repeat at ~60 fps until Ctrl+C
//
// Thread model:
//   Main thread   : FFT + render (~60 fps)
//   Capture thread: WASAPI polling + ring-buffer fill (in AudioCapture)
// ============================================================

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>

#include "audio_capture.h"
#include "fft.h"
#include "visualizer.h"
#include "config.h"

static constexpr int FRAME_MS = 1000 / TARGET_FPS;

// ---- FFT overlap ----
// Using 50% overlap (hop size = FFT_SIZE/2) gives smoother
// temporal response without doubling CPU work significantly.
static constexpr int HOP_SIZE    = FFT_SIZE / 2;

// ---- Global shutdown flag ----
// Set by the Ctrl+C handler; checked in the main loop.
static std::atomic<bool> g_running{true};

// ============================================================
// CtrlHandler - handles Ctrl+C / close events gracefully
// ============================================================
static BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        g_running = false;
        return TRUE;   // suppress default termination
    }
    return FALSE;
}

// ============================================================
// main
// ============================================================
int main()
{
    // Set process priority slightly above normal for smoother rendering
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

    // Register Ctrl+C handler before anything else
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    printf("Audio Visualizer - initialising...\n");

    // ----------------------------------------------------------
    // 1. Audio capture
    // ----------------------------------------------------------
    AudioCapture audio;
    if (!audio.Init()) {
        fprintf(stderr,
            "\nCould not start audio capture.\n"
            "Ensure a default audio output device is connected and working.\n"
            "Tip: open the Windows Sound settings and check playback devices.\n");
        return 1;
    }

    DWORD sampleRate = audio.GetSampleRate();
    printf("Sample rate : %u Hz\n", sampleRate);
    printf("Press Ctrl+C to exit.\n\n");

    // Brief pause so the user can read the init messages before
    // the visualizer clears the screen.
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // ----------------------------------------------------------
    // 2. Terminal visualizer
    // ----------------------------------------------------------
    Visualizer vis;
    if (!vis.Init()) {
        fprintf(stderr, "Failed to initialise the terminal visualizer.\n");
        audio.Shutdown();
        return 1;
    }

    // ----------------------------------------------------------
    // 3. Allocate FFT working buffers
    // ----------------------------------------------------------

    // Sample accumulation buffer - we fill this until we have
    // FFT_SIZE samples, then process and slide by HOP_SIZE.
    float accumBuf[FFT_SIZE] = {};
    int   accumCount = 0;

    // FFT in/out buffers
    float fftReal[FFT_SIZE] = {};
    float fftImag[FFT_SIZE] = {};

    // Magnitude spectrum - only the first FFT_SIZE/2 bins are unique
    // (the rest are the complex conjugate mirror).
    float magnitudes[FFT_SIZE / 2] = {};

    // Temporary pull buffer for draining the ring buffer each frame
    static constexpr int PULL_SIZE = 8192;
    float pullBuf[PULL_SIZE];

    // ----------------------------------------------------------
    // 4. Main render loop
    // ----------------------------------------------------------
    while (g_running) {
        auto frameStart = std::chrono::steady_clock::now();

        // ---- Pull fresh samples from the capture ring buffer ----
        int got = audio.GetSamples(pullBuf, PULL_SIZE);

        // ---- Append pulled samples to accumulation buffer ----
        // If the accumulation buffer is already full, we skip old
        // samples to avoid building up a growing backlog that would
        // add latency to the display.
        bool fftRanThisFrame = false;

        int idx = 0;
        while (idx < got) {
            int space = FFT_SIZE - accumCount;
            int take  = (got - idx < space) ? (got - idx) : space;
            memcpy(accumBuf + accumCount, pullBuf + idx, take * sizeof(float));
            accumCount += take;
            idx        += take;

            // When we have a full window, run the FFT
            if (accumCount >= FFT_SIZE) {

                // -- Copy into FFT real buffer; zero imaginary part --
                memcpy(fftReal, accumBuf, FFT_SIZE * sizeof(float));
                memset(fftImag, 0,        FFT_SIZE * sizeof(float));

                // -- Apply Hann window to reduce spectral leakage --
                // Without windowing, abrupt edges at the buffer boundaries
                // appear as high-frequency artifacts in the spectrum.
                FFT::applyHannWindow(fftReal, FFT_SIZE);

                // -- Compute the FFT (in-place, Cooley-Tukey DIT) --
                FFT::compute(fftReal, fftImag, FFT_SIZE);

                // -- Compute magnitudes for the positive-frequency bins --
                // Bin k corresponds to frequency f = k * sampleRate / FFT_SIZE.
                // We use only bins 1 .. FFT_SIZE/2-1 (skip DC at bin 0).
                for (int k = 0; k < FFT_SIZE / 2; k++) {
                    // Normalize by FFT_SIZE so magnitude is independent of window size
                    magnitudes[k] = FFT::magnitude(fftReal[k], fftImag[k])
                                    / static_cast<float>(FFT_SIZE);
                }

                // -- Slide window by HOP_SIZE (50% overlap) --
                // Retaining the second half of the buffer as the start
                // of the next window gives better temporal continuity.
                memmove(accumBuf, accumBuf + HOP_SIZE, HOP_SIZE * sizeof(float));
                accumCount = HOP_SIZE;

                fftRanThisFrame = true;
                break;   // process at most one FFT window per frame iteration
            }
        }

        // ---- If no new audio arrived (silence or stopped playback), zero the
        //      magnitudes so Render sees silence and decays the bars to zero.
        //      Without this, Render is never called when WASAPI produces no
        //      packets, and the bars freeze at their last values indefinitely.
        if (!fftRanThisFrame)
            memset(magnitudes, 0, sizeof(magnitudes));

        // Always render every frame so smoothing/decay runs continuously.
        vis.Render(magnitudes, FFT_SIZE / 2, static_cast<float>(sampleRate));

        // ---- Frame rate cap ----
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - frameStart).count();
        if (elapsed < FRAME_MS)
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_MS - elapsed));
    }

    // ----------------------------------------------------------
    // 5. Clean shutdown
    // ----------------------------------------------------------
    vis.Shutdown();
    audio.Shutdown();

    printf("Audio Visualizer stopped.\n");
    return 0;
}
