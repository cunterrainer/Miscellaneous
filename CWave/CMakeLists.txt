cmake_minimum_required(VERSION 3.15)
project(cwave LANGUAGES CXX)

# ---------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------
# Source files
# ---------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/audio_capture.cpp
    src/fft.cpp
    src/visualizer.cpp
)

add_executable(cwave ${SOURCES})

# ---------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------
target_include_directories(cwave PRIVATE src)

# ---------------------------------------------------------------
# Preprocessor definitions
#   WIN32_LEAN_AND_MEAN  - exclude rarely-used Windows headers
#   NOMINMAX             - prevent windows.h from defining min/max macros
#   _WIN32_WINNT         - target Windows 10 (0x0A00) for ANSI support
#   UNICODE / _UNICODE   - use Unicode Windows APIs
# ---------------------------------------------------------------
target_compile_definitions(cwave PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
    UNICODE
    _UNICODE
)

# ---------------------------------------------------------------
# Windows SDK / system libraries
#   ole32  - COM initialisation (CoInitializeEx, CoCreateInstance)
#   uuid   - COM interface IIDs (__uuidof support)
# ---------------------------------------------------------------
target_link_libraries(cwave PRIVATE
    ole32
    uuid
)

# ---------------------------------------------------------------
# Compiler-specific flags
# ---------------------------------------------------------------
if(MSVC)
    target_compile_options(cwave PRIVATE
        /W4          # Warning level 4
        /WX-         # Warnings are NOT errors (for easier iteration)
        /utf-8       # Source and execution charset = UTF-8
        /fp:fast     # Fast floating-point (safe for audio math)
        # /O2 is only applied in Release - it conflicts with /RTC1 in Debug
        $<$<CONFIG:Release>:/O2>
    )

    # Build as a console application (not a Windows GUI app)
    set_target_properties(cwave PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # MinGW / Clang fallback
    target_compile_options(cwave PRIVATE
        -Wall -Wextra -O2
    )
    target_link_libraries(cwave PRIVATE -mwindows)
endif()

# ---------------------------------------------------------------
# Output directory - place the executable in build/bin/
# ---------------------------------------------------------------
set_target_properties(cwave PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ---------------------------------------------------------------
# Print configuration summary
# ---------------------------------------------------------------
message(STATUS "----------------------------------------------")
message(STATUS " cwave build configuration")
message(STATUS "   Compiler  : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "   Standard  : C++${CMAKE_CXX_STANDARD}")
message(STATUS "   Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "   Output    : ${CMAKE_BINARY_DIR}/bin/cwave.exe")
message(STATUS "----------------------------------------------")
