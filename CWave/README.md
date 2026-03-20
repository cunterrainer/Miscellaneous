# Audio Visualizer

A real-time terminal audio visualizer written in C++.
Captures system audio output (whatever is currently playing) via **WASAPI loopback** and renders a live frequency-domain bar chart directly in your terminal using ANSI escape codes.

No external libraries are required - only the Windows SDK and the C++ standard library.

---

## Features

| Feature | Details |
|---|---|
| Audio source | System output (WASAPI loopback) - captures speakers/headphones in real-time |
| Frequency analysis | Manual Cooley-Tukey radix-2 DIT FFT (2048-point) with Hann windowing |
| Display | Log-scale frequency bars, color-coded by band, with peak-hold indicators |
| Colors | Bass = red · Low-mid = yellow · Mid = green · High-mid = cyan · High = blue |
| Frame rate | ~60 fps |
| Flicker reduction | Double-buffered: entire frame built as a string, flushed in one write |
| Silence | Bars smoothly decay to zero when no audio is playing |

---

## Requirements

- **Windows 10 or later** (ANSI escape code support is required)
- **Windows Terminal** (recommended) or any terminal with ANSI/VT support
- **CMake 3.15+**
- **Visual Studio 2019/2022** with the "Desktop development with C++" workload
  (or any MSVC toolchain)

---

## Build

### 1. Open a Developer Command Prompt
Use the **x64 Native Tools Command Prompt for VS 2022** (or 2019).

### 2. Configure with CMake

```bat
cd cwave
cmake -B build -G "Visual Studio 17 2022" -A x64
```

For Visual Studio 2019:
```bat
cmake -B build -G "Visual Studio 16 2019" -A x64
```

### 3. Build

```bat
cmake --build build --config Release
```

The executable is placed at:
```
build\bin\Release\cwave.exe
```

---

## Run

1. Make sure audio is playing (music, video, browser tab, etc.).
2. Open **Windows Terminal** (or another ANSI-capable terminal).
3. Run the executable:

```bat
build\bin\Release\cwave.exe
```

4. Press **Ctrl+C** to exit cleanly.

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

- Bars rise fast, fall slowly - giving the classic "bounce" effect.
- A bright white `―` dot marks the recent peak of each band.
- The color shifts from **red** (bass, left) through **yellow → green → cyan** to **blue** (treble, right).

---

## Project structure

```
cwave/
├── src/
│   ├── audio_capture.h   WASAPI loopback interface
│   ├── audio_capture.cpp WASAPI loopback implementation
│   ├── fft.h             Cooley-Tukey FFT interface
│   ├── fft.cpp           FFT + Hann window implementation
│   ├── visualizer.h      Terminal renderer interface
│   ├── visualizer.cpp    ASCII bar chart renderer
│   └── main.cpp          Entry point & main loop
├── CMakeLists.txt
└── README.md
```

---

## Tuning

All tunable parameters are in **`src/config.h`** - it is the only file you need to edit.
Every constant has a detailed explanation comment. Rebuild after any change.

---

## Troubleshooting

| Problem | Solution |
|---|---|
| "Could not start audio capture" | Check Windows Sound settings → Playback devices. Ensure a default output device is set. |
| Bars don't move | Make sure audio is actually playing. WASAPI loopback only captures active output. |
| Block characters look garbled | Use Windows Terminal. The legacy `cmd.exe` window may not support UTF-8 block characters. |
| Bars are too short | Increase `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |
| Bars clip at the top | Decrease `MAGNITUDE_SCALE` in `src/config.h` and rebuild. |
| Flickering | Ensure your terminal supports `ENABLE_VIRTUAL_TERMINAL_PROCESSING`. Windows Terminal does. |

---

## License

Public domain - use freely.
