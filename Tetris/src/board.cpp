#include "board.h"
#include <cstring>

Board::Board() {
    reset();
}

void Board::reset() {
    memset(grid, 0, sizeof(grid));
}

bool Board::isValidPosition(const Tetromino& t) const {
    for (auto [col, row] : cells(t)) {
        if (col < 0 || col >= BOARD_W) return false;
        if (row >= BOARD_H) return false;
        if (row < 0) continue; // above board is ok
        if (grid[row][col] != 0) return false;
    }
    return true;
}

void Board::lockPiece(const Tetromino& t) {
    int colorIdx = static_cast<int>(t.type) + 1; // 1-7
    for (auto [col, row] : cells(t)) {
        if (row >= 0 && row < BOARD_H && col >= 0 && col < BOARD_W) {
            grid[row][col] = colorIdx;
        }
    }
}

int Board::clearLines() {
    int cleared = 0;
    for (int row = BOARD_H - 1; row >= 0; ) {
        bool full = true;
        for (int col = 0; col < BOARD_W; ++col) {
            if (grid[row][col] == 0) { full = false; break; }
        }
        if (full) {
            // Shift everything above down
            for (int r = row; r > 0; --r) {
                memcpy(grid[r], grid[r-1], sizeof(grid[r]));
            }
            memset(grid[0], 0, sizeof(grid[0]));
            ++cleared;
            // Don't decrement row — recheck same row
        } else {
            --row;
        }
    }
    return cleared;
}

bool Board::isGameOver() const {
    // Check if top 2 rows have any blocks
    for (int col = 0; col < BOARD_W; ++col) {
        if (grid[0][col] != 0 || grid[1][col] != 0) return true;
    }
    return false;
}

int Board::cellAt(int row, int col) const {
    if (row < 0 || row >= BOARD_H || col < 0 || col >= BOARD_W) return 0;
    return grid[row][col];
}
