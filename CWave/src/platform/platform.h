#ifndef CWAVE_PLATFORM_H
#define CWAVE_PLATFORM_H

// ============================================================
// platform/platform.h - Compile-time platform detection
//
// Sets exactly one of:
//   PLATFORM_WINDOWS
//   PLATFORM_MACOS
//   PLATFORM_LINUX
//
// Based on standard compiler-provided preprocessor macros.
// A static_assert fires at compile time for unsupported targets.
// To add a new platform, define its PLATFORM_XXX macro here and
// add the corresponding audio backend under platform/xxx/.
// ============================================================

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#else
    static_assert(false, "Unsupported platform. Add a PLATFORM_ define in platform/platform.h.");
#endif

#include <cstdint>

#endif // CWAVE_PLATFORM_H
