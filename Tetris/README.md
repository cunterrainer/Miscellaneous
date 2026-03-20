# Terminal Tetris

A fully-featured Tetris clone that runs in your terminal, written in C++17 with ncurses.

```
╔════════════════════╗  ╔══════════╗
║ · · · · · · · · · ·║  ║ NEXT     ║
║ · · · · · · · · · ·║  ║   ██     ║
║ · · · · · · · · · ·║  ║  ████    ║
║ · · · · · · · · · ·║  ╚══════════╝
║ · · · · · · · · · ·║  ╔══════════╗
║ · · · · · · · · · ·║  ║ HOLD     ║
║ · · · · ██ · · · · ║  ║  ██████  ║
║ · · · · ██ · · · · ║  ║    ██    ║
║ · · · · ██ · · · · ║  ╚══════════╝
║ · · · · ██ · · · · ║  ╔══════════╗
║ · · · · · · · · · ·║  ║ SCORE    ║
║ · · · · · · · · · ·║  ║ 0000012  ║
║ · · · · · · · · · ·║  ║ LEVEL  1 ║
║ · · · · · · · · · ·║  ║ LINES  3 ║
║ · · · · · · · · · ·║  ╚══════════╝
║ · · · · · · · · · ·║
║ · ██████████ · · · ║
║ ██████████████████ ║
╚════════════════════╝
←→:Move ↓:Soft ↑/X:CW Y:CCW SPC:Drop C:Hold P:Pause Q:Quit
```

## Features

- All 7 standard tetrominoes (I, O, T, S, Z, J, L) with individual colors
- **Ghost piece** — shows where the current piece will land
- **Hold piece** — swap the current piece to save for later (once per piece)
- **Next piece preview** — see the upcoming piece
- **Bag-of-7 randomizer** — all 7 pieces appear before any repeats
- **SRS rotation** — Super Rotation System with full wall kick support
- **Score, level, and lines** tracking with standard Tetris scoring
- Speed increases every 10 lines (up to level ~11)
- Pause and restart support
- Terminal-centered layout that adapts to window size

## Requirements

- Linux or macOS
- A terminal with Unicode and 256-color support (most modern terminals)
- `ncurses` development library
- CMake >= 3.16
- A C++17-capable compiler (GCC 7+, Clang 5+)

### Installing ncurses

**Debian/Ubuntu:**
```bash
sudo apt install libncurses-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install ncurses-devel
```

**macOS (Homebrew):**
```bash
brew install ncurses
```

## Building

```bash
cmake -B build
cmake --build build
```

The binary is placed at `build/tetris`.

## Running

```bash
./build/tetris
```

## Controls

| Key | Action |
|-----|--------|
| `←` / `→` | Move left / right |
| `↓` | Soft drop (+1 point per row) |
| `Space` | Hard drop (+2 points per row) |
| `↑` or `X` | Rotate clockwise |
| `Y` | Rotate counter-clockwise |
| `C` | Hold piece |
| `P` | Pause / unpause |
| `Q` / `Esc` | Quit |
| `R` | Restart (on game over screen) |

## Scoring

| Lines cleared | Points |
|---------------|--------|
| 1 | 100 × level |
| 2 | 300 × level |
| 3 | 500 × level |
| 4 (Tetris) | 800 × level |

Soft drop adds **1 point** per row; hard drop adds **2 points** per row.

Level increases by 1 for every 10 lines cleared. Gravity starts at 800 ms per row and decreases by 75 ms per level, capping at 50 ms at level 11.

## Project Structure

```
terminalapps/
├── CMakeLists.txt
└── src/
    ├── main.cpp         # Entry point
    ├── tetromino.h/.cpp # Piece definitions, rotations, bag randomizer
    ├── board.h/.cpp     # 10×20 grid, collision, line clearing
    ├── renderer.h/.cpp  # ncurses rendering (board, panels, game over)
    ├── game.h/.cpp      # Game loop, input, gravity, scoring, hold/ghost
```
