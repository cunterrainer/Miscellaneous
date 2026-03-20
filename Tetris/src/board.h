#pragma once
#include "tetromino.h"

constexpr int BOARD_W = 10;
constexpr int BOARD_H = 20;

class Board {
public:
    Board();
    bool isValidPosition(const Tetromino& t) const;
    void lockPiece(const Tetromino& t);
    int clearLines();
    bool isGameOver() const;
    int cellAt(int row, int col) const;
    void reset();

private:
    int grid[BOARD_H][BOARD_W];
};
