# cwave

A real-time terminal audio visualizer written in C++.
Captures system audio output and renders a live frequency-domain bar chart directly in your terminal using ANSI escape codes.

Runs on **Windows**, **macOS**, and **Linux**. No external libraries required.

---

## Features

| Feature | Details |
|---|---|
| Audio source | System output — captures whatever is currently playing |
| Frequency analysis | Manual Cooley-Tukey radix-2 DIT FFT (2048-point) with Hann windowing |
| Display | Log-scale frequency bars, color-coded by band, with peak-hold indicators |
| Colors | Bass = red · Low-mid = yellow · Mid = green · High-mid = cyan · High = blue |
| Frame rate | ~60 fps |
| Flicker reduction | Double-buffered: entire frame built as a string, flushed in one write |
| Silence | Bars smoothly decay to zero when no audio is playing |

---

## Requirements

### Windows
- **Windows 10 or later** (ANSI escape code support required)
- **Windows Terminal** (recommended) or any terminal with ANSI/VT support
- **CMake 3.15+**
- **Visual Studio 2019/2022** with the "Desktop development with C++" workload (or any MSVC toolchain)

### macOS — native loopback (default, macOS 14.2+)
- **macOS 14.2 (Sonoma) or later**
- **CMake 3.15+**
- **Xcode Command Line Tools** (`xcode-select --install`)
- **Screen & System Audio Recording** permission granted to your terminal app
  (System Settings → Privacy & Security → Screen & System Audio Recording)

### macOS — input-device fallback (all macOS versions)
- Any macOS version
- **CMake 3.15+**
- **Xcode Command Line Tools**
- **[BlackHole](https://github.com/ExistingSound/BlackHole)** virtual audio driver installed and set as the default input device

### Linux
- Any modern desktop distribution (Ubuntu, Fedora, Arch, etc.)
- **PulseAudio** or **PipeWire** with PulseAudio compatibility (`pipewire-pulse`) — installed by default on most desktop distros
- **libpulse development headers** (see build instructions below)
- **CMake 3.15+**
- **GCC 8+** or **Clang 7+** with C++17 support
- **pkg-config**

---

## Build

### Windows

#### 1. Open a Developer Command Prompt
Use the **x64 Native Tools Command Prompt for VS 2022** (or 2019).

#### 2. Configure with CMake

```bat
cd cwave
cmake -B build -G "Visual Studio 17 2022" -A x64
```

For Visual Studio 2019:
```bat
cmake -B build -G "Visual Studio 16 2019" -A x64
```

#### 3. Build

```bat
cmake --build build --config Release
```

Executable: `build\bin\Release\cwave.exe`

---

### macOS

#### 1. Configure and build

```sh
cd cwave
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

Executable: `build/bin/cwave`

#### CMake options

| Option | Default | Description |
|---|---|---|
| `CWAVE_MACOS_NATIVE_LOOPBACK` | `ON` | Use the native macOS 14.2+ system audio tap (`CATapDescription`). Set to `OFF` to use the AudioQueue input-device fallback (requires BlackHole on older systems). |

Example — build the fallback for older macOS:
```sh
cmake -B build -DCWAVE_MACOS_NATIVE_LOOPBACK=OFF
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

---

### Linux

#### 1. Install dependencies

Debian / Ubuntu:
```sh
sudo apt install cmake g++ pkg-config libpulse-dev
```

Fedora:
```sh
sudo dnf install cmake gcc-c++ pkgconf-pkg-config pulseaudio-libs-devel
```

Arch Linux:
```sh
sudo pacman -S cmake gcc pkgconf libpulse
```

#### 2. Configure and build

```sh
cd cwave
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Executable: `build/bin/cwave`

---

## Run

### Windows

1. Make sure audio is playing (music, video, browser tab, etc.).
2. Open **Windows Terminal** (or another ANSI-capable terminal).
3. Run:

```bat
build\bin\Release\cwave.exe
```

### macOS (native loopback)

1. Grant your terminal app **Screen & System Audio Recording** permission:
   System Settings → Privacy & Security → Screen & System Audio Recording → add your terminal.
   Restart the terminal after granting permission.
2. Make sure audio is playing.
3. Run:

```sh
./build/bin/cwave
```

### macOS (input-device fallback)

1. Install [BlackHole](https://github.com/ExistingSound/BlackHole) and set it as the macOS default input device.
2. Route audio through BlackHole (e.g. via a Multi-Output Device in Audio MIDI Setup).
3. Run:

```sh
./build/bin/cwave
```

### Linux

1. Make sure audio is playing (music, video, browser tab, etc.).
2. Run:

```sh
./build/bin/cwave
```

CWave automatically captures the default PulseAudio sink's monitor source — no additional configuration needed. On PipeWire systems, the PulseAudio compatibility layer handles this transparently.

Press **Ctrl+C** to exit cleanly on all platforms.

---

## What you will see

```
 █ cwave ────────────────────────────────────────────
──────────────────────────────────────────────────────────────

          █
          █
    █     █  █      ―
    █  █  █  █   ―  █  ―                ―
█   █  █  █  █   █  █  █  ―  ―         █    ―   ―
█  ██  █  █  █   █  █  █  █  █  ―  ―   █    █   █   ―   ―
60  150  300  600  1k   2k   4k   8k  16k
```

- Bars rise fast, fall slowly — giving the classic "bounce" effect.
- A bright white `―` dot marks the recent peak of each band.
- The color shifts from **red** (bass, left) through **yellow → green → cyan** to **blue** (treble, right).

---

## Project structure

```
cwave/
├── src/
│   ├── audio_capture.h            Abstract audio capture interface
│   ├── fft.h / fft.cpp            Cooley-Tukey FFT + Hann window
│   ├── visualizer.h / visualizer.cpp  Terminal bar-chart renderer
│   ├── main.cpp                   Entry point & main loop
│   └── platform/
│       ├── platform.h             Platform detection macros
│       ├── windows/
│       │   ├── audio_capture_wasapi.h    WASAPI loopback (Windows)
│       │   └── audio_capture_wasapi.cpp
│       ├── macos/
│       │   ├── audio_capture_coreaudio_tap.mm   Native tap (macOS 14.2+)
│       │   └── audio_capture_coreaudio.cpp      AudioQueue fallback
│       └── linux/
│           ├── audio_capture_pulseaudio.h       PulseAudio monitor (Linux)
│           └── audio_capture_pulseaudio.cpp
├── CMakeLists.txt
└── README.md
```

---

## Tuning

All tunable parameters are in **`src/config.h`** — it is the only file you need to edit.
Every constant has a detailed explanation comment. Rebuild after any change.

---

## Troubleshooting

### Windows

| Problem | Solution |
|---|---|
| "Could not start audio capture" | Check Windows Sound settings → Playback devices. Ensure a default output device is set. |
| Bars don't move | Make sure audio is actually playing. WASAPI loopback only captures active output. |
| Block characters look garbled | Use Windows Terminal. The legacy `cmd.exe` window may not support UTF-8 block characters. |
| Flickering | Ensure your terminal supports `ENABLE_VIRTUAL_TERMINAL_PROCESSING`. Windows Terminal does. |

### macOS

| Problem | Solution |
|---|---|
| Bars don't move (native loopback) | Grant **Screen & System Audio Recording** permission to your terminal app in System Settings → Privacy & Security. Restart the terminal after granting it. macOS silently delivers silence when permission is denied. |
| "AudioHardwareCreateProcessTap failed" | Requires macOS 14.2+. Rebuild with `-DCWAVE_MACOS_NATIVE_LOOPBACK=OFF` for older systems. |
| Bars don't move (input fallback) | Install [BlackHole](https://github.com/ExistingSound/BlackHole) and set it as the default macOS input device. |
| Bars are too short | Increase `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |
| Bars clip at the top | Decrease `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |

### Linux

| Problem | Solution |
|---|---|
| Build error: libpulse not found | Install `libpulse-dev` (Debian/Ubuntu), `pulseaudio-libs-devel` (Fedora), or `libpulse` (Arch). |
| "PulseAudio connection failed" | Ensure PulseAudio or PipeWire (with `pipewire-pulse`) is running. Check with `pactl info`. |
| "Could not find a default audio output device" | Check with `pactl list sinks short` and ensure a default output device is configured. |
| Bars don't move | Verify audio is actually playing through the default sink. The monitor source only captures from the default output. |
| Bars are too short | Increase `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |
| Bars clip at the top | Decrease `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |

---
