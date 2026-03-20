#pragma once
#include <vector>
#include <utility>

enum class TetrominoType { I=0, O, T, S, Z, J, L };

// All 7 pieces, 4 rotations, 4x4 bitmask (row-major)
extern const int SHAPES[7][4][4][4];

struct Tetromino {
    TetrominoType type;
    int x, y;      // board col, row of top-left of 4x4 bounding box
    int rotation;  // 0-3
};

Tetromino randomPiece();
Tetromino rotatedCW(const Tetromino& t);
Tetromino rotatedCCW(const Tetromino& t);
std::vector<std::pair<int,int>> cells(const Tetromino& t); // returns (col, row) pairs
