cmake_minimum_required(VERSION 3.15)
project(cwave LANGUAGES CXX)

# ---------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------
# Platform detection
#
# Auto-detected from the host OS, or overridden explicitly:
#   cmake -DCWAVE_PLATFORM=macos ..
#   cmake -DCWAVE_PLATFORM=windows ..
#
# To add a new platform: add an elseif branch here, a platform
# source file under src/platform/<name>/, and a corresponding
# section in the "Platform-specific" blocks below.
# ---------------------------------------------------------------
if(NOT DEFINED CWAVE_PLATFORM)
    if(WIN32)
        set(CWAVE_PLATFORM "windows")
    elseif(APPLE)
        set(CWAVE_PLATFORM "macos")
    else()
        message(FATAL_ERROR
            "Unknown platform. Set -DCWAVE_PLATFORM explicitly.\n"
            "Supported values: windows, macos")
    endif()
endif()

# ---------------------------------------------------------------
# Common sources (compiled on every platform)
# ---------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/fft.cpp
    src/visualizer.cpp
)

# ---------------------------------------------------------------
# Platform-specific source files
# ---------------------------------------------------------------
if(CWAVE_PLATFORM STREQUAL "windows")
    list(APPEND SOURCES src/platform/windows/audio_capture_wasapi.cpp)
elseif(CWAVE_PLATFORM STREQUAL "macos")
    # Native loopback tap requires macOS 14.2+.
    # Set -DCWAVE_MACOS_NATIVE_LOOPBACK=OFF to use the input-device fallback
    # (works on all macOS versions; captures microphone / BlackHole loopback).
    option(CWAVE_MACOS_NATIVE_LOOPBACK
        "Use native macOS 14.2+ audio tap for system loopback (requires CATapDescription)"
        ON)

    if(CWAVE_MACOS_NATIVE_LOOPBACK)
        list(APPEND SOURCES src/platform/macos/audio_capture_coreaudio_tap.mm)
    else()
        list(APPEND SOURCES src/platform/macos/audio_capture_coreaudio.cpp)
    endif()
else()
    message(FATAL_ERROR "Unsupported CWAVE_PLATFORM value: ${CWAVE_PLATFORM}")
endif()

add_executable(cwave ${SOURCES})

# ---------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------
target_include_directories(cwave PRIVATE src)

# ---------------------------------------------------------------
# Platform-specific compile definitions and libraries
# ---------------------------------------------------------------
if(CWAVE_PLATFORM STREQUAL "windows")
    target_compile_definitions(cwave PRIVATE
        WIN32_LEAN_AND_MEAN   # exclude rarely-used Windows headers
        NOMINMAX              # prevent windows.h from defining min/max macros
        _WIN32_WINNT=0x0A00   # target Windows 10 (0x0A00) for ANSI support
        UNICODE
        _UNICODE
    )
    target_link_libraries(cwave PRIVATE
        ole32   # COM initialisation (CoInitializeEx, CoCreateInstance)
        uuid    # COM interface IIDs (__uuidof support)
    )

elseif(CWAVE_PLATFORM STREQUAL "macos")
    target_link_libraries(cwave PRIVATE
        "-framework CoreAudio"       # AudioObjectGetPropertyData, AudioDevice*
        "-framework AudioToolbox"    # AudioQueue, AudioStreamBasicDescription
        "-framework CoreFoundation"  # CFRunLoop, OSStatus types
    )
    if(CWAVE_MACOS_NATIVE_LOOPBACK)
        target_compile_definitions(cwave PRIVATE CWAVE_MACOS_NATIVE_LOOPBACK=1)
        target_link_libraries(cwave PRIVATE
            "-framework Foundation"  # NSString, NSDictionary (CATapDescription)
        )
    endif()
endif()

# ---------------------------------------------------------------
# Compiler-specific flags
# ---------------------------------------------------------------
if(MSVC)
    target_compile_options(cwave PRIVATE
        /W4          # Warning level 4
        /WX-         # Warnings are NOT errors (for easier iteration)
        /utf-8       # Source and execution charset = UTF-8
        /fp:fast     # Fast floating-point (safe for audio math)
        $<$<CONFIG:Release>:/O2>
    )
    set_target_properties(cwave PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cwave PRIVATE -Wall -Wextra -O2)
    # -mwindows is a MinGW flag that targets Win32 subsystem; only for Windows
    if(CWAVE_PLATFORM STREQUAL "windows")
        target_link_libraries(cwave PRIVATE -mwindows)
    endif()
endif()

# ---------------------------------------------------------------
# Output directory - place the executable in build/bin/
# ---------------------------------------------------------------
set_target_properties(cwave PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ---------------------------------------------------------------
# Configuration summary
# ---------------------------------------------------------------
message(STATUS "----------------------------------------------")
message(STATUS " cwave build configuration")
message(STATUS "   Platform  : ${CWAVE_PLATFORM}")
if(CWAVE_PLATFORM STREQUAL "macos")
    if(CWAVE_MACOS_NATIVE_LOOPBACK)
        message(STATUS "   Loopback  : native tap (macOS 14.2+)")
    else()
        message(STATUS "   Loopback  : input device fallback (pre-14.2)")
    endif()
endif()
message(STATUS "   Compiler  : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "   Standard  : C++${CMAKE_CXX_STANDARD}")
message(STATUS "   Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "   Output    : ${CMAKE_BINARY_DIR}/bin/cwave")
message(STATUS "----------------------------------------------")
