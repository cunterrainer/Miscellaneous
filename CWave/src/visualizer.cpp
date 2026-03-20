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

#include <windows.h>
#include <io.h>          // _setmode, _fileno
#include <fcntl.h>       // _O_BINARY

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

// ============================================================
// Init
// ============================================================
bool Visualizer::Init()
{
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
// UpdateDimensions - query the live console window size.
// Returns true when the terminal was resized since the last call.
// On resize: clears the screen and resizes per-band state vectors
// so stale content from the old layout doesn't linger.
// ============================================================
bool Visualizer::UpdateDimensions()
{
    int newWidth  = m_width;
    int newHeight = m_height;

    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        newWidth  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }

    // Clamp width to [4, MAX_BANDS] and height to a sane minimum.
    // MAX_BANDS is a safety cap; it's set high enough (300) that it
    // never limits a real terminal - see config.h.
    newWidth  = std::max(4,  std::min(newWidth,  MAX_BANDS));
    newHeight = std::max(4,  newHeight);

    if (newWidth == m_prevWidth && newHeight == m_prevHeight)
        return false;   // no change

    m_width  = newWidth;
    m_height = newHeight;
    m_prevWidth  = newWidth;
    m_prevHeight = newHeight;

    // The number of display bands equals the usable column count.
    int numBands = std::max(1, m_width - 2);

    // Resize per-band vectors and zero them so bars don't jump on resize.
    m_smoothed .assign(numBands, 0.0f);
    m_peaks    .assign(numBands, 0.0f);
    m_peakTimer.assign(numBands, 0);

    // Full clear so characters from the previous (wider/taller) layout
    // don't leave ghost columns or rows on screen.
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
// Render - build and flush one display frame
// ============================================================
void Visualizer::Render(const float* magnitudes, int binCount, float sampleRate)
{
    if (!m_initialized) return;

    // Refresh terminal size; vectors are resized inside if dimensions changed.
    UpdateDimensions();

    // Number of display bands = usable terminal columns.
    // Derived from the vector size (set by UpdateDimensions) so it always
    // matches the allocated per-band state exactly.
    int numBands = static_cast<int>(m_smoothed.size());
    if (numBands < 1) return;

    // Rows available for bar chart (header=2, footer=1, border=1)
    int barRows = m_height - 4;
    if (barRows < 2) barRows = 2;

    // ----------------------------------------------------------
    // Step 1: Map FFT bins → display bands (log-frequency scale)
    //
    // A linear mapping would waste most columns on bass and
    // cram all highs together.  A log scale distributes bands
    // perceptually evenly across the audible range.
    //
    // For band b:
    //   freqLow  = minFreq * (maxFreq/minFreq)^(b/numBands)
    //   freqHigh = minFreq * (maxFreq/minFreq)^((b+1)/numBands)
    //   binLow   = freqLow  * binCount / (sampleRate/2)
    //   binHigh  = freqHigh * binCount / (sampleRate/2)
    //   value    = average of |X[k]| for bins in [binLow, binHigh)
    // ----------------------------------------------------------
    float minFreq  = MIN_FREQ_HZ;
    float maxFreq  = std::min(MAX_FREQ_HZ, sampleRate * 0.5f);
    float logMin   = log10f(minFreq);
    float logMax   = log10f(maxFreq);
    float freqToBin = static_cast<float>(binCount) / (sampleRate * 0.5f);

    // Stack-allocate at the actual band count for this frame.
    // numBands is always ≤ terminal width which is sane on any real system.
    std::vector<float> rawBands(numBands, 0.0f);

    for (int b = 0; b < numBands; b++) {
        float fLow  = powf(10.0f, logMin + (logMax - logMin) *
                               static_cast<float>(b)   / numBands);
        float fHigh = powf(10.0f, logMin + (logMax - logMin) *
                               static_cast<float>(b+1) / numBands);

        int kLow  = std::max(1,           static_cast<int>(fLow  * freqToBin));
        int kHigh = std::min(binCount - 1, static_cast<int>(fHigh * freqToBin) + 1);

        // Use the MAXIMUM bin magnitude in this band rather than the average.
        // Averaging dilutes narrow spectral peaks: a strong 10 kHz tone occupying
        // one bin gets divided across ~50 bins at high frequencies, making it
        // invisible. Max preserves any peak that falls within the band.
        float peak = 0.0f;
        for (int k = kLow; k <= kHigh; k++) {
            if (magnitudes[k] > peak) peak = magnitudes[k];
        }

        // High-frequency pre-emphasis: music has far less energy per bin at
        // high frequencies than at bass.  Boost by log10(fCenter/100) so that
        // a 1 kHz signal and a 100 Hz signal of similar perceptual loudness
        // produce similar bar heights.  Bass bands (fCenter < 100 Hz) get a
        // small attenuation instead, which prevents them from always clipping.
        float fCenter = sqrtf(fLow * fHigh);            // geometric mean
        float emphasis = 1.0f + EMPHASIS_STRENGTH * log10f(std::max(fCenter, MIN_FREQ_HZ) / EMPHASIS_PIVOT_HZ);
        emphasis = std::max(0.5f, emphasis);            // floor at 0.5×

        rawBands[b] = std::min(1.0f, peak * MAGNITUDE_SCALE * emphasis);
    }

    // ----------------------------------------------------------
    // Step 2: Smooth bar heights and track peaks
    //
    // Bars rise quickly (RISE_ALPHA) and fall slowly (FALL_DECAY)
    // giving the characteristic "bounce" look.
    //
    // Peak dots:
    //   When a bar exceeds the current peak, reset the hold timer.
    //   The dot stays for PEAK_HOLD_FRAMES frames, then falls
    //   at a gentle PEAK_DECAY rate.
    // ----------------------------------------------------------
    for (int b = 0; b < numBands; b++) {
        float target = rawBands[b];

        if (target > m_smoothed[b]) {
            // Rise: blend quickly toward new peak
            m_smoothed[b] += RISE_ALPHA * (target - m_smoothed[b]);
        } else {
            // Fall: exponential decay
            m_smoothed[b] *= FALL_DECAY;
        }
        m_smoothed[b] = std::max(0.0f, std::min(1.0f, m_smoothed[b]));

        // Peak tracking
        if (m_smoothed[b] >= m_peaks[b]) {
            m_peaks[b]     = m_smoothed[b];
            m_peakTimer[b] = PEAK_HOLD_FRAMES;
        } else {
            if (m_peakTimer[b] > 0) {
                --m_peakTimer[b];           // hold
            } else {
                m_peaks[b] *= PEAK_DECAY_RATE;   // decay
            }
        }
        m_peaks[b] = std::max(0.0f, std::min(1.0f, m_peaks[b]));

        // Silence threshold: once a bar decays below this floor, snap it and
        // its peak to exactly zero.  Without this, exponential decay never
        // reaches zero, leaving a faint residual that renders as a solid
        // horizontal line of half-block characters and keeps peak dots floating.
        if (m_smoothed[b] < SILENCE_THRESHOLD) {
            m_smoothed[b]  = 0.0f;
            m_peaks[b]     = 0.0f;
            m_peakTimer[b] = 0;
        }
    }

    // ----------------------------------------------------------
    // Step 3: Build the frame string
    //
    // We write everything into a std::string and emit it in one
    // fwrite() call.  This dramatically reduces the number of
    // write() syscalls and keeps the frame visually coherent.
    // ----------------------------------------------------------
    std::string frame;
    // Reserve enough space: ~16 bytes/column/row + escape codes
    frame.reserve(static_cast<size_t>(m_width) * m_height * 20);

    // Move cursor to top-left (no erase → no flicker)
    frame += "\033[H";

    // ---- Header bar ----
    frame += "\033[1;97m";   // bold bright-white
    frame += " \xe2\x96\x88 cwave ";   // ▓ title
    frame += "\033[0;90m";   // dark gray
    // Pad header to full width
    {
        int hdrLen = 17; // length of " ▓ cwave " in display chars (approx)
        for (int i = hdrLen; i < m_width - 1; i++) frame += '-';
    }
    frame += "\033[0m\r\n";

    // ---- Separator ----
    frame += "\033[90m";
    for (int i = 0; i < m_width; i++) frame += '-';
    frame += "\033[0m\r\n";

    // ---- Bar chart rows ----
    // row 0 = top of the chart, row (barRows-1) = bottom
    for (int row = 0; row < barRows; row++) {
        // Threshold: what fraction of the bar height corresponds to this row?
        // Row 0 is the very top (high values); row barRows-1 is the floor.
        float rowTop    = 1.0f - static_cast<float>(row)     / barRows;
        float rowBottom = 1.0f - static_cast<float>(row + 1) / barRows;
        float rowMid    = (rowTop + rowBottom) * 0.5f;

        for (int b = 0; b < numBands; b++) {
            float val  = m_smoothed[b];
            float peak = m_peaks[b];

            // Determine what to draw in this cell
            if (val >= rowTop) {
                // Cell is fully inside the bar → full block
                frame += BandColor(b, numBands);
                frame += BLOCK_FULL;
            } else if (val >= rowMid) {
                // Cell is at the top of the bar → lower-half block (sub-pixel)
                frame += BandColor(b, numBands);
                frame += BLOCK_LOWER;
            } else if (val == 0.0f && peak >= rowBottom && peak < rowTop) {
                // Peak-hold indicator: draw a thin horizontal bar
                frame += "\033[97m";   // bright white peak dot
                frame += PEAK_CHAR;
            } else {
                // Empty cell
                frame += "\033[0m ";
            }
        }
        frame += "\033[0m\r\n";
    }

    // ---- Frequency labels ----
    // Show approximate Hz markers spread across the width.
    // We precompute label positions based on log scale.
    {
        frame += "\033[0;90m";   // dark gray labels

        // Create a row of spaces, then overlay frequency markers
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
            // Map frequency to column using same log formula
            float norm = (log10f(lbl.freq) - logMin) / (logMax - logMin);
            int   col  = static_cast<int>(norm * numBands);
            if (col < 0 || col + static_cast<int>(strlen(lbl.text)) >= m_width) continue;
            // Write label text into the label row
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
    // fflush not needed every frame if stdout is already line-buffered
    // but explicit flush ensures the terminal receives the data promptly
    fflush(stdout);
}
