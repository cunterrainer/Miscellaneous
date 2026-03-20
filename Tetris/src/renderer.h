#pragma once
#include "board.h"
#include "tetromino.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void drawAll(const Board& board,
                 const Tetromino& current,
                 const Tetromino& ghost,
                 const Tetromino* hold,
                 const Tetromino& next,
                 int score, int level, int lines,
                 bool paused);

    void drawGameOver(int score, int level, int lines);
    void drawPaused();

private:
    void drawBoard(const Board& board, const Tetromino& current, const Tetromino& ghost);
    void drawPanel(const Tetromino* hold, const Tetromino& next, int score, int level, int lines);
    void drawCell(int screenY, int screenX, int colorIdx);
    void drawPiece(const Tetromino& t, int originY, int originX);

    // Terminal offset for centering
    int offsetY, offsetX;
};
