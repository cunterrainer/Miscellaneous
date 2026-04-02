// ============================================================
// main.cpp - Audio Visualizer entry point & main loop
//
// Orchestration:
//   1. Parse command-line arguments (override config.h defaults)
//   2. Init audio capture (platform backend on default device)
//   3. Init terminal visualizer
//   4. Accumulate samples into an overlap-add FFT window
//   5. Apply Hann window -> run FFT -> compute magnitudes
//   6. Map magnitudes -> log-scale bands -> render frame
//   7. Repeat at ~60 fps until Ctrl+C
//
// Thread model:
//   Main thread   : FFT + render (~60 fps)
//   Capture thread: audio polling + ring-buffer fill (in AudioCaptureBase)
// ============================================================

#include "platform/platform.h"

#ifdef PLATFORM_WINDOWS
  #include <windows.h>
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
  #include <csignal>
  #include <sys/resource.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <string>

#include "audio_capture.h"
#include "fft.h"
#include "visualizer.h"
#include "config.h"

// ---- Global shutdown flag ----
// Set by the signal/Ctrl+C handler; checked in the main loop.
static std::atomic<bool> g_running{true};

// ============================================================
// Platform signal handlers
// ============================================================
#ifdef PLATFORM_WINDOWS
static BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        g_running = false;
        return TRUE;   // suppress default termination
    }
    return FALSE;
}
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
static void SigHandler(int /*sig*/)
{
    g_running = false;
}
#endif

// ============================================================
// Help / usage
// ============================================================
static void printHelp(const char* progName)
{
    Config def;   // default-constructed → picks up constexpr values from config.h
    printf(
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Real-time terminal audio visualizer.\n"
        "\n"
        "FFT:\n"
        "  -f, --fft-size <N>       FFT frame size (power of 2)       [%d]\n"
        "\n"
        "Display:\n"
        "  -b, --max-bands <N>      Max frequency bands (columns)     [%d]\n"
        "  -l, --min-freq <Hz>      Lowest displayed frequency        [%.1f]\n"
        "  -u, --max-freq <Hz>      Highest displayed frequency       [%.1f]\n"
        "\n"
        "Frame rate:\n"
        "  -r, --fps <N>            Target frames per second          [%d]\n"
        "\n"
        "Amplitude scaling:\n"
        "  -m, --magnitude <F>      FFT magnitude multiplier          [%.1f]\n"
        "  -e, --emphasis <F>       High-frequency emphasis strength  [%.1f]\n"
        "  -p, --pivot <Hz>         Emphasis pivot frequency          [%.1f]\n"
        "\n"
        "Bar smoothing:\n"
        "  -a, --rise <F>           Rise speed (0.0-1.0)              [%.2f]\n"
        "  -d, --decay <F>          Fall decay (0.0-1.0)              [%.2f]\n"
        "  -t, --silence <F>        Silence snap threshold            [%.3f]\n"
        "\n"
        "Peak-hold indicators:\n"
        "  -k, --peak-hold <N>      Frames to hold peak dot           [%d]\n"
        "  -q, --peak-decay <F>     Peak decay rate (0.0-1.0)         [%.2f]\n"
        "\n"
        "Misc:\n"
        "  -h, --help               Show this help message and exit\n",
        progName,
        def.fftSize,
        def.maxBands,
        def.minFreqHz, def.maxFreqHz,
        def.targetFps,
        def.magnitudeScale,
        def.emphasisStrength,
        def.emphasisPivotHz,
        def.riseAlpha,
        def.fallDecay,
        def.silenceThreshold,
        def.peakHoldFrames,
        def.peakDecayRate
    );
}

// ============================================================
// Argument parsing helpers
// ============================================================
static bool matchFlag(const char* arg, const char* shortF, const char* longF)
{
    return strcmp(arg, shortF) == 0 || strcmp(arg, longF) == 0;
}

static bool isPowerOf2(int n) { return n > 0 && (n & (n - 1)) == 0; }

static bool parseConfig(int argc, char* argv[], Config& cfg)
{
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (matchFlag(arg, "-h", "--help")) {
            printHelp(argv[0]);
            exit(0);
        }

        // Check for known flags first, then consume the value
        bool isKnown =
            matchFlag(arg, "-f", "--fft-size")   || matchFlag(arg, "-b", "--max-bands")  ||
            matchFlag(arg, "-l", "--min-freq")   || matchFlag(arg, "-u", "--max-freq")   ||
            matchFlag(arg, "-r", "--fps")        || matchFlag(arg, "-m", "--magnitude")  ||
            matchFlag(arg, "-e", "--emphasis")   || matchFlag(arg, "-p", "--pivot")       ||
            matchFlag(arg, "-a", "--rise")       || matchFlag(arg, "-d", "--decay")       ||
            matchFlag(arg, "-t", "--silence")    || matchFlag(arg, "-k", "--peak-hold")   ||
            matchFlag(arg, "-q", "--peak-decay");

        if (!isKnown) {
            fprintf(stderr, "Error: unknown option '%s'. Use --help for usage.\n", arg);
            return false;
        }

        if (i + 1 >= argc) {
            fprintf(stderr, "Error: '%s' requires a value.\n", arg);
            return false;
        }
        const char* val = argv[++i];

        if      (matchFlag(arg, "-f", "--fft-size"))   cfg.fftSize          = atoi(val);
        else if (matchFlag(arg, "-b", "--max-bands"))   cfg.maxBands         = atoi(val);
        else if (matchFlag(arg, "-l", "--min-freq"))    cfg.minFreqHz        = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-u", "--max-freq"))    cfg.maxFreqHz        = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-r", "--fps"))         cfg.targetFps        = atoi(val);
        else if (matchFlag(arg, "-m", "--magnitude"))   cfg.magnitudeScale   = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-e", "--emphasis"))     cfg.emphasisStrength = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-p", "--pivot"))        cfg.emphasisPivotHz  = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-a", "--rise"))         cfg.riseAlpha        = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-d", "--decay"))        cfg.fallDecay        = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-t", "--silence"))      cfg.silenceThreshold = static_cast<float>(atof(val));
        else if (matchFlag(arg, "-k", "--peak-hold"))    cfg.peakHoldFrames   = atoi(val);
        else if (matchFlag(arg, "-q", "--peak-decay"))   cfg.peakDecayRate    = static_cast<float>(atof(val));
    }

    // Validate FFT size
    if (!isPowerOf2(cfg.fftSize)) {
        fprintf(stderr, "Error: --fft-size must be a power of 2 (got %d).\n", cfg.fftSize);
        return false;
    }
    if (cfg.targetFps < 1) {
        fprintf(stderr, "Error: --fps must be at least 1.\n");
        return false;
    }

    return true;
}

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[])
{
    // ----------------------------------------------------------
    // 0. Parse command-line arguments
    // ----------------------------------------------------------
    Config cfg;
    if (!parseConfig(argc, argv, cfg))
        return 1;

    const int frameMs = 1000 / cfg.targetFps;

    // Using 50% overlap (hop size = fftSize/2) gives smoother
    // temporal response without doubling CPU work significantly.
    const int hopSize = cfg.fftSize / 2;

    // Set process priority slightly above normal for smoother rendering
#ifdef PLATFORM_WINDOWS
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
#endif
#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
    setpriority(PRIO_PROCESS, 0, -10);
#endif

    // Register signal handler before anything else
#ifdef PLATFORM_WINDOWS
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif
#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
    signal(SIGINT,  SigHandler);
    signal(SIGTERM, SigHandler);
#endif

    printf("Audio Visualizer - initialising...\n");

    // ----------------------------------------------------------
    // 1. Audio capture
    // ----------------------------------------------------------
    std::unique_ptr<AudioCaptureBase> audio(AudioCapture::Create());
    if (!audio->Init()) {
        fprintf(stderr,
            "\nCould not start audio capture.\n"
            "Ensure a default audio input device is connected and working.\n");
        return 1;
    }

    uint32_t sampleRate = audio->GetSampleRate();
    printf("Sample rate : %u Hz\n", sampleRate);
    printf("Press Ctrl+C to exit.\n\n");

    // Brief pause so the user can read the init messages before
    // the visualizer clears the screen.
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // ----------------------------------------------------------
    // 2. Terminal visualizer
    // ----------------------------------------------------------
    Visualizer vis;
    vis.SetConfig(cfg);
    if (!vis.Init()) {
        fprintf(stderr, "Failed to initialise the terminal visualizer.\n");
        audio->Shutdown();
        return 1;
    }

    // ----------------------------------------------------------
    // 3. Allocate FFT working buffers (sized by runtime config)
    // ----------------------------------------------------------

    // Sample accumulation buffer - we fill this until we have
    // fftSize samples, then process and slide by hopSize.
    std::vector<float> accumBuf(cfg.fftSize, 0.0f);
    int accumCount = 0;

    // FFT in/out buffers
    std::vector<float> fftReal(cfg.fftSize, 0.0f);
    std::vector<float> fftImag(cfg.fftSize, 0.0f);

    // Magnitude spectrum - only the first fftSize/2 bins are unique
    // (the rest are the complex conjugate mirror).
    std::vector<float> magnitudes(cfg.fftSize / 2, 0.0f);

    // Temporary pull buffer for draining the ring buffer each frame
    static constexpr int PULL_SIZE = 8192;
    float pullBuf[PULL_SIZE];

    // ----------------------------------------------------------
    // 4. Main render loop
    // ----------------------------------------------------------
    while (g_running) {
        auto frameStart = std::chrono::steady_clock::now();

        // ---- Pull fresh samples from the capture ring buffer ----
        int got = audio->GetSamples(pullBuf, PULL_SIZE);

        // ---- Append pulled samples to accumulation buffer ----
        bool fftRanThisFrame = false;

        int idx = 0;
        while (idx < got) {
            int space = cfg.fftSize - accumCount;
            int take  = (got - idx < space) ? (got - idx) : space;
            memcpy(accumBuf.data() + accumCount, pullBuf + idx, take * sizeof(float));
            accumCount += take;
            idx        += take;

            // When we have a full window, run the FFT
            if (accumCount >= cfg.fftSize) {

                // -- Copy into FFT real buffer; zero imaginary part --
                memcpy(fftReal.data(), accumBuf.data(), cfg.fftSize * sizeof(float));
                memset(fftImag.data(), 0,               cfg.fftSize * sizeof(float));

                // -- Apply Hann window to reduce spectral leakage --
                FFT::applyHannWindow(fftReal.data(), cfg.fftSize);

                // -- Compute the FFT (in-place, Cooley-Tukey DIT) --
                FFT::compute(fftReal.data(), fftImag.data(), cfg.fftSize);

                // -- Compute magnitudes for the positive-frequency bins --
                for (int k = 0; k < cfg.fftSize / 2; k++) {
                    magnitudes[k] = FFT::magnitude(fftReal[k], fftImag[k])
                                    / static_cast<float>(cfg.fftSize);
                }

                // -- Slide window by hopSize (50% overlap) --
                memmove(accumBuf.data(), accumBuf.data() + hopSize, hopSize * sizeof(float));
                accumCount = hopSize;

                fftRanThisFrame = true;
                break;   // process at most one FFT window per frame iteration
            }
        }

        // ---- If no new audio arrived, zero the magnitudes so bars decay.
        if (!fftRanThisFrame)
            memset(magnitudes.data(), 0, magnitudes.size() * sizeof(float));

        // Always render every frame so smoothing/decay runs continuously.
        vis.Render(magnitudes.data(), cfg.fftSize / 2, static_cast<float>(sampleRate));

        // ---- Frame rate cap ----
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - frameStart).count();
        if (elapsed < frameMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(frameMs - elapsed));
    }

    // ----------------------------------------------------------
    // 5. Clean shutdown
    // ----------------------------------------------------------
    vis.Shutdown();
    audio->Shutdown();

    printf("Audio Visualizer stopped.\n");
    return 0;
}
