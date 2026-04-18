#pragma once

// ============================================================
// config.h - Audio Visualizer tuning parameters
//
// This is the single place to adjust the behaviour and look of
// the visualizer.  All values are constexpr - the compiler
// will catch type errors and there is zero runtime overhead.
// Rebuild after any change here (all source files include this).
// ============================================================


// ------------------------------------------------------------
// FFT
// ------------------------------------------------------------

// Number of audio samples used per FFT frame.
// MUST be a power of 2 (512, 1024, 2048, 4096, 8192, ...).
//
// Frequency resolution = SampleRate / FFT_SIZE
//   2048 @ 48 kHz → ~23 Hz per bin  (default, good balance)
//   4096 @ 48 kHz → ~12 Hz per bin  (finer, +42 ms latency)
//   1024 @ 48 kHz → ~47 Hz per bin  (coarser, very responsive)
//
// Increasing this makes low-frequency bars narrower and more
// accurate, but adds latency because more samples must be
// collected before each frame can be processed.
constexpr int FFT_SIZE = 2048;


// ------------------------------------------------------------
// Display
// ------------------------------------------------------------

// Maximum number of frequency bands (display columns).
// The visualizer uses the full terminal width automatically -
// this value is only a hard safety cap in case the OS reports
// an unreasonably large terminal size.  300 covers any monitor
// at any realistic font size; raise it if you have an unusually
// wide terminal.
constexpr int MAX_BANDS = 300;

// Lowest frequency shown on the display, in Hz.
// Human hearing starts around 20 Hz.  Bass content below this
// (e.g. infrasound, DC offset) is intentionally excluded.
constexpr float MIN_FREQ_HZ = 20.0f;

// Highest frequency shown on the display, in Hz.
// Human hearing tops out around 20,000 Hz.  The display is
// also hard-capped at Nyquist (SampleRate / 2), whichever is lower.
constexpr float MAX_FREQ_HZ = 20000.0f;


// ------------------------------------------------------------
// Frame rate
// ------------------------------------------------------------

// Target display frames per second.
// 60 is smooth and matches most monitor refresh rates.
// Lower (e.g. 30) reduces CPU usage; higher (e.g. 120) is
// only useful if your terminal can keep up.
constexpr int TARGET_FPS = 60;


// ------------------------------------------------------------
// Amplitude scaling
// ------------------------------------------------------------

// Multiplier applied to raw FFT magnitudes before display.
// Increase  → bars appear taller for the same audio level.
// Decrease  → bars appear shorter (useful if they always clip).
// Start here and adjust by ±1 until the display feels right
// for your typical listening volume.
constexpr float MAGNITUDE_SCALE = 4.5f;

// High-frequency pre-emphasis strength.
// Music naturally has far less energy at high frequencies than
// at bass.  This factor boosts high bands so they light up
// visibly even for treble-light content.
//   0.0 = flat (no boost, highs will look very quiet)
//   1.8 = default (gentle, perceptually balanced)
//   3.0 = aggressive (highs always prominent)
constexpr float EMPHASIS_STRENGTH = 1.8f;

// The pivot frequency for pre-emphasis, in Hz.
// Bands centred at this frequency receive exactly 1× (no boost or cut).
// Bands above get boosted; bands below get a slight attenuation.
// Changing this shifts where the boost "starts".
constexpr float EMPHASIS_PIVOT_HZ = 100.0f;


// ------------------------------------------------------------
// Bar smoothing
// ------------------------------------------------------------

// How quickly bars RISE toward a new louder value.
// Range: 0.0 (bars never move) → 1.0 (bars jump instantly).
// Higher values feel snappier; lower values feel laggy on loud transients.
constexpr float RISE_ALPHA = 0.45f;

// Multiplicative decay applied every frame when a bar is falling.
// Range: 0.0 (bars drop to zero immediately) → 1.0 (bars never fall).
// Lower values make bars fall faster after a sound ends.
//   0.70 = fast fall    0.82 = default    0.92 = slow, floaty fall
constexpr float FALL_DECAY = 0.82f;

// Bar values below this threshold are snapped to exactly zero.
// This prevents bars from lingering near the baseline as near-invisible
// slivers after audio stops (exponential decay never truly reaches 0).
// Raise if bars seem to linger; lower if quiet audio is cut off early.
constexpr float SILENCE_THRESHOLD = 0.008f;


// ------------------------------------------------------------
// Peak-hold indicators
// ------------------------------------------------------------

// Number of frames a peak dot stays at the highest point before
// it starts to fall.  At 60 fps, 25 frames ≈ 0.4 seconds.
// Set to 0 to disable peak hold entirely.
constexpr int PEAK_HOLD_FRAMES = 25;

// Multiplicative decay applied to a peak dot every frame after
// the hold period expires.  Same range and logic as FALL_DECAY.
//   0.90 = fast-falling peaks    0.93 = default    0.97 = very slow
constexpr float PEAK_DECAY_RATE = 0.93f;


// ------------------------------------------------------------
// Runtime configuration struct
// ------------------------------------------------------------
// Holds runtime copies of every tuning parameter above.
// Default-initialised from the constexpr values so that:
//   - The help page always shows the compile-time defaults.
//   - Command-line overrides only touch the fields the user specifies.
//   - Changing a constexpr above automatically updates both.

enum class CenterStyle { Disk, Ring, Empty };

struct Config {
    int   fftSize          = FFT_SIZE;
    int   maxBands         = MAX_BANDS;
    float minFreqHz        = MIN_FREQ_HZ;
    float maxFreqHz        = MAX_FREQ_HZ;
    int   targetFps        = TARGET_FPS;
    float magnitudeScale   = MAGNITUDE_SCALE;
    float emphasisStrength = EMPHASIS_STRENGTH;
    float emphasisPivotHz  = EMPHASIS_PIVOT_HZ;
    float riseAlpha        = RISE_ALPHA;
    float fallDecay        = FALL_DECAY;
    float silenceThreshold = SILENCE_THRESHOLD;
    int   peakHoldFrames   = PEAK_HOLD_FRAMES;
    float peakDecayRate    = PEAK_DECAY_RATE;
    bool        circular    = false;
    CenterStyle centerStyle = CenterStyle::Disk;
};
