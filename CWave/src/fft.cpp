// ============================================================
// fft.cpp - Cooley-Tukey radix-2 DIT FFT implementation
//
// Algorithm overview:
//   1. Bit-reversal permutation reorders the input array so that
//      the butterfly stages can operate in-place.
//   2. log2(N) butterfly stages combine pairs of complex values
//      using twiddle factors W_N^k = e^(-2πi·k/N).
//   3. After all stages, real[k]+j·imag[k] holds X[k].
//
// Time complexity: O(N log N)
// Space complexity: O(1) extra (in-place)
// ============================================================

#include "fft.h"
#include <cmath>
#include <algorithm>   // std::swap

static constexpr float PI = 3.14159265358979323846f;

// ------------------------------------------------------------
// applyHannWindow
// w(n) = 0.5 * (1 - cos(2π·n/(N-1)))
// Multiplying by this window shapes the signal so both ends
// smoothly taper to zero, greatly reducing the "spectral
// smearing" that happens when a non-periodic signal is fed
// into an FFT that assumes periodic input.
// ------------------------------------------------------------
void FFT::applyHannWindow(float* data, int n)
{
    for (int i = 0; i < n; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * PI * static_cast<float>(i)
                                       / static_cast<float>(n - 1)));
        data[i] *= w;
    }
}

// ------------------------------------------------------------
// bitReverse
// Reorders the array so that element at index i ends up at the
// index formed by reversing the binary representation of i.
// Example with N=8 (3 bits): index 6 = 110₂ → 011₂ = 3.
// This permutation is needed so the butterfly stages can
// combine adjacent pairs without index conflicts.
// ------------------------------------------------------------
static void bitReverse(float* real, float* imag, int n)
{
    // Count how many bits are in n-1
    int bits = 0;
    while ((1 << bits) < n) ++bits;

    for (int i = 0; i < n; i++) {
        // Compute bit-reversed index of i
        int rev = 0;
        for (int b = 0; b < bits; b++) {
            if (i & (1 << b))
                rev |= (1 << (bits - 1 - b));
        }
        // Swap only once per pair (when rev > i) to avoid double-swapping
        if (rev > i) {
            std::swap(real[i], real[rev]);
            std::swap(imag[i], imag[rev]);
        }
    }
}

// ------------------------------------------------------------
// compute
// Cooley-Tukey DIT (Decimation-In-Time) radix-2 FFT.
//
// After bit-reversal, the algorithm performs log2(N) stages.
// In each stage s, groups of size len = 2^s are processed.
// Each group splits into two halves connected by a "butterfly":
//
//   X[u] ←  X[u] + W·X[v]
//   X[v] ←  X[u] - W·X[v]
//
// where W = e^(-2πi·j/len) is the twiddle factor for position j.
// ------------------------------------------------------------
void FFT::compute(float* real, float* imag, int n)
{
    // Step 1: reorder inputs via bit-reversal permutation
    bitReverse(real, imag, n);

    // Step 2: butterfly stages - one per power of 2 up to n
    for (int len = 2; len <= n; len <<= 1) {

        // Base twiddle factor for this stage: W_len = e^(-2πi/len)
        float angle = -2.0f * PI / static_cast<float>(len);
        float wBaseRe = cosf(angle);   // real part of W_len
        float wBaseIm = sinf(angle);   // imaginary part of W_len

        // Process each group of 'len' consecutive elements
        for (int i = 0; i < n; i += len) {

            // Current twiddle factor, starts at W^0 = 1+0i
            float curRe = 1.0f;
            float curIm = 0.0f;

            // Butterfly over the first half of the group
            for (int j = 0; j < len / 2; j++) {
                int u = i + j;             // upper butterfly index
                int v = i + j + len / 2;   // lower butterfly index

                // Multiply lower element by twiddle factor W^j
                //   (curRe + i·curIm) * (real[v] + i·imag[v])
                float tRe = curRe * real[v] - curIm * imag[v];
                float tIm = curRe * imag[v] + curIm * real[v];

                // Butterfly combine
                real[v] = real[u] - tRe;
                imag[v] = imag[u] - tIm;
                real[u] = real[u] + tRe;
                imag[u] = imag[u] + tIm;

                // Advance twiddle factor: cur *= W_len
                float nextRe = curRe * wBaseRe - curIm * wBaseIm;
                float nextIm = curRe * wBaseIm + curIm * wBaseRe;
                curRe = nextRe;
                curIm = nextIm;
            }
        }
    }
}

// ------------------------------------------------------------
// magnitude
// Returns |z| = sqrt(re² + im²) for complex z = re + i·im.
// ------------------------------------------------------------
float FFT::magnitude(float re, float im)
{
    return sqrtf(re * re + im * im);
}
