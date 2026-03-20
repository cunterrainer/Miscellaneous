#pragma once
#include "board.h"
#include "tetromino.h"
#include "renderer.h"
#include <memory>

class Game {
public:
    Game();
    void run();

private:
    Board board;
    Tetromino current;
    Tetromino next;
    Tetromino ghost;
    Tetromino* hold;      // nullptr if empty
    Tetromino holdPiece;  // storage
    bool holdUsed;
    bool gameOver;
    bool paused;

    int score;
    int level;
    int linesCleared;

    Renderer renderer;

    void handleInput(int key);
    void tick();
    void spawnNext();
    void computeGhost();
    void doHold();
    void applyScore(int lines, int softDrop, int hardDrop);
    void showGameOver();
    int gravityMs() const;
};
