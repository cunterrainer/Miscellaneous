// ============================================================
// visualizer.cpp - Terminal bar-chart renderer
//
// Rendering pipeline each frame:
//   1. Query terminal dimensions (handles window resize).
//   2. Map FFT bins → display bands on a log-frequency scale.
//   3. Smooth bar heights with an exponential moving average;
//      track per-band peaks with a hold-and-decay mechanism.
//   4. Build the entire frame as a std::string (no partial writes).
//   5. Write the string in one fwrite() call to minimize flicker.
//
// ANSI escape codes used:
//   \033[H          - move cursor to top-left (home)
//   \033[2J         - clear screen (used once at startup only)
//   \033[?25l/h     - hide / show cursor
//   \033[0m         - reset colors
//   \033[1;37m      - bold white
//   \033[31m…\033[34m - colored bars
// ============================================================

#include "visualizer.h"
#include "config.h"
#include "platform/platform.h"

#ifdef PLATFORM_WINDOWS
  #include <windows.h>
  #include <io.h>     // _setmode, _fileno
  #include <fcntl.h>  // _O_BINARY
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>

// All tuning constants live in config.h

// ---- UTF-8 block characters ----------------------------------
// Written as byte sequences so the compiler doesn't mangle them.
// U+2588 FULL BLOCK      = 0xE2 0x96 0x88
// U+2584 LOWER HALF BLOCK= 0xE2 0x96 0x84
// U+2580 UPPER HALF BLOCK= 0xE2 0x96 0x80
// U+2015 HORIZONTAL BAR  = 0xE2 0x80 0x95  (peak indicator)
static const char BLOCK_FULL[]  = "\xe2\x96\x88";   // █
static const char BLOCK_LOWER[] = "\xe2\x96\x84";   // ▄  (sub-cell smoothing)
static const char PEAK_CHAR[]   = "\xe2\x80\x95";   // ― (peak-hold dot)

// ============================================================
Visualizer::Visualizer()  = default;
Visualizer::~Visualizer() { Shutdown(); }

void Visualizer::SetConfig(const Config& cfg) { m_cfg = cfg; }

// ============================================================
// Init
// ============================================================
bool Visualizer::Init()
{
#ifdef PLATFORM_WINDOWS
    // --- Enable ANSI escape code processing (Windows 10+) ---
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;

    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode
                   | ENABLE_VIRTUAL_TERMINAL_PROCESSING
                   | DISABLE_NEWLINE_AUTO_RETURN);

    // Set console code page to UTF-8 so block chars render correctly
    SetConsoleOutputCP(CP_UTF8);

    // Switch stdout to binary mode to prevent Windows CRLF translation
    // (we emit \r\n ourselves when needed)
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    // macOS: ANSI codes and UTF-8 are supported natively; no setup needed.

    // Query initial size
    UpdateDimensions();

    // Clear screen once; subsequent frames only move the cursor to home
    // to avoid the full-screen flash of a repeated clear.
    fwrite("\033[2J\033[H\033[?25l", 1, 15, stdout);
    fflush(stdout);

    // Vectors are zeroed by UpdateDimensions() called above.

    m_initialized = true;
    return true;
}

// ============================================================
// Shutdown
// ============================================================
void Visualizer::Shutdown()
{
    if (!m_initialized) return;

    // Restore cursor and move to bottom of display
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[?25h\033[%d;1H\r\n", m_height);
    fwrite(buf, 1, strlen(buf), stdout);
    fflush(stdout);

    m_initialized = false;
}

// ============================================================
// OuterRadius - aspect-corrected radius for circular mode.
// Terminal cells are ~2× taller than wide, so we halve the
// horizontal extent to keep the circle visually round.
// ============================================================
float Visualizer::OuterRadius() const
{
    float halfW = static_cast<float>(m_width)  * 0.5f * 0.5f;  // aspect correction
    float halfH = static_cast<float>(m_height) * 0.5f;
    return std::min(halfW, halfH) - 1.0f;
}

// ============================================================
// UpdateDimensions - query the live console window size.
// Returns true when the terminal was resized since the last call.
// On resize: clears the screen and resizes per-band state vectors
// so stale content from the old layout doesn't linger.
// ============================================================
bool Visualizer::UpdateDimensions()
{
    int newWidth  = m_width;
    int newHeight = m_height;

#ifdef PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        newWidth  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        newWidth  = ws.ws_col;
        newHeight = ws.ws_row;
    }
#endif

    // Clamp width to [4, MAX_BANDS] and height to a sane minimum.
    newWidth  = std::max(4,  std::min(newWidth,  m_cfg.maxBands));
    newHeight = std::max(4,  newHeight);

    if (newWidth == m_prevWidth && newHeight == m_prevHeight)
        return false;   // no change

    m_width  = newWidth;
    m_height = newHeight;
    m_prevWidth  = newWidth;
    m_prevHeight = newHeight;

    int numBands;
    if (m_cfg.circular) {
        float outerR = OuterRadius();
        numBands = std::max(8, static_cast<int>(2.0f * 3.14159265f * outerR));
    } else {
        numBands = std::max(1, m_width - 2);
    }

    // Resize per-band vectors and zero them so bars don't jump on resize.
    m_smoothed .assign(numBands, 0.0f);
    m_peaks    .assign(numBands, 0.0f);
    m_peakTimer.assign(numBands, 0);

    // Full clear so characters from the previous layout don't linger.
    fwrite("\033[2J\033[H", 1, 7, stdout);
    fflush(stdout);

    return true;
}

// ============================================================
// BandColor - ANSI color escape sequence for a frequency band
//
// Frequency spectrum colour mapping:
//   0  –  20% → red    (bass: 20 – ~300 Hz)
//   20 –  45% → yellow (low-mid: ~300 – 1 kHz)
//   45 –  70% → green  (mid: ~1 – 4 kHz)
//   70 –  88% → cyan   (upper-mid / presence: ~4 – 10 kHz)
//   88 – 100% → blue   (high / air: ~10 – 20 kHz)
// ============================================================
const char* Visualizer::BandColor(int band, int numBands)
{
    float r = static_cast<float>(band) / static_cast<float>(numBands);
    if (r < 0.20f) return "\033[91m";   // bright red   - bass
    if (r < 0.45f) return "\033[93m";   // bright yellow - low-mid
    if (r < 0.70f) return "\033[92m";   // bright green  - mid
    if (r < 0.88f) return "\033[96m";   // bright cyan   - upper-mid
    return               "\033[94m";   // bright blue   - high
}

// ============================================================
// ComputeBands - shared FFT bin → display band mapping
//
// Fills rawBands[0..numBands-1] with values in [0,1].
// Applies log-frequency mapping, max-bin selection, and
// high-frequency pre-emphasis.
// ============================================================
void Visualizer::ComputeBands(const float* magnitudes, int binCount, float sampleRate,
                               std::vector<float>& rawBands)
{
    int numBands = static_cast<int>(rawBands.size());

    float minFreq  = m_cfg.minFreqHz;
    float maxFreq  = std::min(m_cfg.maxFreqHz, sampleRate * 0.5f);
    float logMin   = log10f(minFreq);
    float logMax   = log10f(maxFreq);
    float freqToBin = static_cast<float>(binCount) / (sampleRate * 0.5f);

    for (int b = 0; b < numBands; b++) {
        float fLow  = powf(10.0f, logMin + (logMax - logMin) *
                               static_cast<float>(b)   / numBands);
        float fHigh = powf(10.0f, logMin + (logMax - logMin) *
                               static_cast<float>(b+1) / numBands);

        int kLow  = std::max(1,           static_cast<int>(fLow  * freqToBin));
        int kHigh = std::min(binCount - 1, static_cast<int>(fHigh * freqToBin) + 1);

        float peak = 0.0f;
        for (int k = kLow; k <= kHigh; k++) {
            if (magnitudes[k] > peak) peak = magnitudes[k];
        }

        float fCenter  = sqrtf(fLow * fHigh);
        float emphasis = 1.0f + m_cfg.emphasisStrength *
                         log10f(std::max(fCenter, m_cfg.minFreqHz) / m_cfg.emphasisPivotHz);
        emphasis = std::max(0.5f, emphasis);

        rawBands[b] = std::min(1.0f, peak * m_cfg.magnitudeScale * emphasis);
    }
}

// ============================================================
// Render - dispatch to linear or circular renderer
// ============================================================
void Visualizer::Render(const float* magnitudes, int binCount, float sampleRate)
{
    if (!m_initialized) return;
    UpdateDimensions();

    if (m_cfg.circular)
        RenderCircular(magnitudes, binCount, sampleRate);
    else
        RenderLinear(magnitudes, binCount, sampleRate);
}

// ============================================================
// RenderLinear - build and flush one linear bar-chart frame
// ============================================================
void Visualizer::RenderLinear(const float* magnitudes, int binCount, float sampleRate)
{
    int numBands = static_cast<int>(m_smoothed.size());
    if (numBands < 1) return;

    // Rows available for bar chart (header=2, footer=1, border=1)
    int barRows = m_height - 4;
    if (barRows < 2) barRows = 2;

    float minFreq = m_cfg.minFreqHz;
    float maxFreq = std::min(m_cfg.maxFreqHz, sampleRate * 0.5f);
    float logMin  = log10f(minFreq);
    float logMax  = log10f(maxFreq);

    // ----------------------------------------------------------
    // Step 1: Map FFT bins → display bands
    // ----------------------------------------------------------
    std::vector<float> rawBands(numBands, 0.0f);
    ComputeBands(magnitudes, binCount, sampleRate, rawBands);

    // ----------------------------------------------------------
    // Step 2: Smooth bar heights and track peaks
    // ----------------------------------------------------------
    for (int b = 0; b < numBands; b++) {
        float target = rawBands[b];

        if (target > m_smoothed[b]) {
            m_smoothed[b] += m_cfg.riseAlpha * (target - m_smoothed[b]);
        } else {
            m_smoothed[b] *= m_cfg.fallDecay;
        }
        m_smoothed[b] = std::max(0.0f, std::min(1.0f, m_smoothed[b]));

        if (m_smoothed[b] >= m_peaks[b]) {
            m_peaks[b]     = m_smoothed[b];
            m_peakTimer[b] = m_cfg.peakHoldFrames;
        } else {
            if (m_peakTimer[b] > 0) {
                --m_peakTimer[b];
            } else {
                m_peaks[b] *= m_cfg.peakDecayRate;
            }
        }
        m_peaks[b] = std::max(0.0f, std::min(1.0f, m_peaks[b]));

        if (m_smoothed[b] < m_cfg.silenceThreshold) {
            m_smoothed[b]  = 0.0f;
            m_peaks[b]     = 0.0f;
            m_peakTimer[b] = 0;
        }
    }

    // ----------------------------------------------------------
    // Step 3: Build the frame string
    // ----------------------------------------------------------
    std::string frame;
    frame.reserve(static_cast<size_t>(m_width) * m_height * 20);

    frame += "\033[H";

    // ---- Header bar ----
    frame += "\033[1;97m";
    frame += " \xe2\x96\x88 cwave ";
    frame += "\033[0;90m";
    {
        int hdrLen = 17;
        for (int i = hdrLen; i < m_width - 1; i++) frame += '-';
    }
    frame += "\033[0m\r\n";

    // ---- Separator ----
    frame += "\033[90m";
    for (int i = 0; i < m_width; i++) frame += '-';
    frame += "\033[0m\r\n";

    // ---- Bar chart rows ----
    for (int row = 0; row < barRows; row++) {
        float rowTop    = 1.0f - static_cast<float>(row)     / barRows;
        float rowBottom = 1.0f - static_cast<float>(row + 1) / barRows;
        float rowMid    = (rowTop + rowBottom) * 0.5f;

        for (int b = 0; b < numBands; b++) {
            float val  = m_smoothed[b];
            float peak = m_peaks[b];

            if (val >= rowTop) {
                frame += BandColor(b, numBands);
                frame += BLOCK_FULL;
            } else if (val >= rowMid) {
                frame += BandColor(b, numBands);
                frame += BLOCK_LOWER;
            } else if (val == 0.0f && peak >= rowBottom && peak < rowTop) {
                frame += "\033[97m";
                frame += PEAK_CHAR;
            } else {
                frame += "\033[0m ";
            }
        }
        frame += "\033[0m\r\n";
    }

    // ---- Frequency labels ----
    {
        frame += "\033[0;90m";

        std::string labelRow(m_width, ' ');

        struct Label { float freq; const char* text; };
        static const Label kLabels[] = {
            { 60.f,    "60" },
            { 150.f,   "150" },
            { 300.f,   "300" },
            { 600.f,   "600" },
            { 1000.f,  "1k" },
            { 2000.f,  "2k" },
            { 4000.f,  "4k" },
            { 8000.f,  "8k" },
            { 16000.f, "16k" },
        };

        for (const auto& lbl : kLabels) {
            if (lbl.freq > maxFreq) continue;
            float norm = (log10f(lbl.freq) - logMin) / (logMax - logMin);
            int   col  = static_cast<int>(norm * numBands);
            if (col < 0 || col + static_cast<int>(strlen(lbl.text)) >= m_width) continue;
            for (int c = 0; lbl.text[c] && (col + c) < m_width; c++)
                labelRow[col + c] = lbl.text[c];
        }

        frame += labelRow;
        frame += "\033[0m";
    }

    // ----------------------------------------------------------
    // Step 4: Flush the complete frame in one write
    // ----------------------------------------------------------
    fwrite(frame.c_str(), 1, frame.size(), stdout);
    fflush(stdout);
}

// ============================================================
// RenderCircular - centered circle with radiating frequency pillars
//
// Coordinate system:
//   cx, cy = center of the terminal in character cells
//   dx = (x - cx) * 0.5  (aspect correction: cells are ~2× taller than wide)
//   dy = (y - cy)
//   r  = sqrt(dx² + dy²)
//   θ  = atan2(dx, -dy)  → 0 at 12-o'clock, clockwise positive
//
// Band mapping:
//   band 0 (bass/red) at 12 o'clock, sweeping clockwise.
//
// Inner disk:
//   r < innerR  → filled gray disk  (CenterStyle::Disk)
//               → ring outline only (CenterStyle::Ring)
//               → empty             (CenterStyle::Empty)
// ============================================================
void Visualizer::RenderCircular(const float* magnitudes, int binCount, float sampleRate)
{
    int numBands = static_cast<int>(m_smoothed.size());
    if (numBands < 1) return;

    // ----------------------------------------------------------
    // Step 1: FFT bins → raw bands
    // ----------------------------------------------------------
    std::vector<float> rawBands(numBands, 0.0f);
    ComputeBands(magnitudes, binCount, sampleRate, rawBands);

    // ----------------------------------------------------------
    // Step 2: Smooth and track peaks
    // ----------------------------------------------------------
    for (int b = 0; b < numBands; b++) {
        float target = rawBands[b];

        if (target > m_smoothed[b]) {
            m_smoothed[b] += m_cfg.riseAlpha * (target - m_smoothed[b]);
        } else {
            m_smoothed[b] *= m_cfg.fallDecay;
        }
        m_smoothed[b] = std::max(0.0f, std::min(1.0f, m_smoothed[b]));

        if (m_smoothed[b] >= m_peaks[b]) {
            m_peaks[b]     = m_smoothed[b];
            m_peakTimer[b] = m_cfg.peakHoldFrames;
        } else {
            if (m_peakTimer[b] > 0) {
                --m_peakTimer[b];
            } else {
                m_peaks[b] *= m_cfg.peakDecayRate;
            }
        }
        m_peaks[b] = std::max(0.0f, std::min(1.0f, m_peaks[b]));

        if (m_smoothed[b] < m_cfg.silenceThreshold) {
            m_smoothed[b]  = 0.0f;
            m_peaks[b]     = 0.0f;
            m_peakTimer[b] = 0;
        }
    }

    // ----------------------------------------------------------
    // Step 3: Geometry constants
    // ----------------------------------------------------------
    float cx = static_cast<float>(m_width)  * 0.5f;
    float cy = static_cast<float>(m_height) * 0.5f;
    float outerR  = OuterRadius();
    // innerR: ~1/6 of outerR, minimum 2 character-radius units
    float innerR  = std::max(2.0f, outerR * 0.18f);
    float pillarMax = outerR - innerR - 0.5f;
    if (pillarMax < 1.0f) pillarMax = 1.0f;

    static constexpr float TWO_PI = 6.28318530f;

    // ----------------------------------------------------------
    // Step 4: Build frame row by row
    // ----------------------------------------------------------
    std::string frame;
    frame.reserve(static_cast<size_t>(m_width) * m_height * 16);

    frame += "\033[H";

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            // Aspect-corrected vector from center
            float dx = (static_cast<float>(x) - cx + 0.5f) * 0.5f;
            float dy =  static_cast<float>(y) - cy + 0.5f;
            float r  = sqrtf(dx * dx + dy * dy);

            // Angle: 0 at 12-o'clock, clockwise
            float theta = atan2f(dx, -dy);
            if (theta < 0.0f) theta += TWO_PI;

            int band = static_cast<int>(theta / TWO_PI * static_cast<float>(numBands));
            if (band < 0)         band = 0;
            if (band >= numBands) band = numBands - 1;

            float pillarLen  = m_smoothed[band] * pillarMax;
            float peakR      = innerR + m_peaks[band] * pillarMax;

            if (r < innerR - 0.5f) {
                // Inside the inner circle
                switch (m_cfg.centerStyle) {
                case CenterStyle::Disk:
                    frame += "\033[90m";
                    frame += BLOCK_FULL;
                    break;
                case CenterStyle::Ring:
                case CenterStyle::Empty:
                    frame += "\033[0m ";
                    break;
                }
            } else if (r < innerR + 0.5f) {
                // Ring boundary
                switch (m_cfg.centerStyle) {
                case CenterStyle::Disk:
                    frame += "\033[37m";
                    frame += BLOCK_FULL;
                    break;
                case CenterStyle::Ring:
                    frame += "\033[37m";
                    frame += BLOCK_FULL;
                    break;
                case CenterStyle::Empty:
                    frame += "\033[0m ";
                    break;
                }
            } else if (r < innerR + pillarLen) {
                // Inside the pillar
                frame += BandColor(band, numBands);
                frame += BLOCK_FULL;
            } else if (m_smoothed[band] == 0.0f && m_peaks[band] > m_cfg.silenceThreshold
                       && fabsf(r - peakR) < 0.6f) {
                // Peak-hold indicator dot
                frame += "\033[97m";
                frame += PEAK_CHAR;
            } else {
                frame += "\033[0m ";
            }
        }
        frame += "\033[0m\r\n";
    }

    // ----------------------------------------------------------
    // Step 5: Flush
    // ----------------------------------------------------------
    fwrite(frame.c_str(), 1, frame.size(), stdout);
    fflush(stdout);
}
