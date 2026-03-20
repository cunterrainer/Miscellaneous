#pragma once

// ============================================================
// fft.h - Manual Cooley-Tukey radix-2 DIT FFT
//
// Implements:
//   - Hann window to reduce spectral leakage
//   - In-place complex FFT (radix-2, decimation-in-time)
//   - Magnitude helper
//
// No external libraries used. Only <cmath> and <cstdlib>.
// ============================================================

#include "config.h"   // FFT_SIZE

namespace FFT {

    // Apply a Hann (raised-cosine) window to 'data' in-place.
    // This tapers the signal to zero at both ends, preventing
    // spectral leakage caused by the FFT's implicit periodicity.
    //   w(n) = 0.5 * (1 - cos(2π·n / (N-1)))
    void applyHannWindow(float* data, int n);

    // In-place Cooley-Tukey radix-2 DIT FFT.
    // Transforms N complex samples from time → frequency domain.
    //   real[k], imag[k] hold the k-th complex input/output.
    //   N must be a power of 2.
    // After the call, real[k] + j·imag[k] is X[k].
    void compute(float* real, float* imag, int n);

    // Compute magnitude (absolute value) of a complex number.
    //   |z| = sqrt(re² + im²)
    float magnitude(float re, float im);

} // namespace FFT
