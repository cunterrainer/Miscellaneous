#pragma once

// ============================================================
// visualizer.h - Real-time terminal audio visualizer interface
//
// Renders a frequency-domain bar chart directly in the Windows
// console using ANSI escape codes and UTF-8 block characters.
//
// Features:
//   - Fully responsive to terminal size (width and height)
//   - Log-scale frequency mapping (bass/mid/high perceptually even)
//   - Color coding by frequency band
//   - Exponential smoothing + peak-hold indicators
//   - Double-buffered output (one fwrite per frame) to minimize flicker
// ============================================================

#include <string>
#include <vector>
#include "config.h"

class Visualizer
{
public:
    Visualizer();
    ~Visualizer();

    // Enable ANSI codes, UTF-8 output, hide cursor, clear screen.
    bool Init();

    // Restore cursor visibility and clean up.
    void Shutdown();

    // Build and flush one frame.
    //   magnitudes : FFT magnitude array, bins 0 .. binCount-1
    //   binCount   : number of valid bins (usually FFT_SIZE / 2)
    //   sampleRate : audio sample rate in Hz (for frequency mapping)
    void Render(const float* magnitudes, int binCount, float sampleRate);

private:
    // Query the current console window size.
    // Returns true if the dimensions changed since the last call.
    bool UpdateDimensions();

    // Return ANSI color escape for a band index (0 = bass, high = treble)
    static const char* BandColor(int band, int numBands);

    // Per-band state - sized dynamically to match the terminal width.
    // Resized (and zeroed) automatically whenever the terminal is resized.
    std::vector<float> m_smoothed;   // exponentially smoothed bar height [0,1]
    std::vector<float> m_peaks;      // peak-hold value [0,1]
    std::vector<int>   m_peakTimer;  // frames to hold peak before decaying

    // ---- Terminal dimensions ----
    int m_width     = 0;   // current terminal width  in columns
    int m_height    = 0;   // current terminal height in rows
    int m_prevWidth = 0;   // width  from last frame (resize detection)
    int m_prevHeight= 0;   // height from last frame (resize detection)

    bool m_initialized = false;
};
