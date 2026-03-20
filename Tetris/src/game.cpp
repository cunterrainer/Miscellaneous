#include "game.h"
#include <ncurses.h>
#include <chrono>
#include <cstring>

using Clock = std::chrono::steady_clock;
using Ms = std::chrono::milliseconds;

// SRS wall kick data for J, L, S, T, Z pieces (non-I)
// [fromRot][kickIndex] = {dx, dy}
static const int KICKS[4][5][2] = {
    {{0,0},{-1,0},{-1,-1},{0,2},{-1,2}},   // 0->1
    {{0,0},{1,0},{1,1},{0,-2},{1,-2}},     // 1->2
    {{0,0},{1,0},{1,-1},{0,2},{1,2}},      // 2->3
    {{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}},  // 3->0
};

// SRS wall kick data for I piece
static const int KICKS_I[4][5][2] = {
    {{0,0},{-2,0},{1,0},{-2,1},{1,-2}},    // 0->1
    {{0,0},{-1,0},{2,0},{-1,-2},{2,1}},    // 1->2
    {{0,0},{2,0},{-1,0},{2,-1},{-1,2}},    // 2->3
    {{0,0},{1,0},{-2,0},{1,2},{-2,-1}},    // 3->0
};

Game::Game()
    : hold(nullptr), holdUsed(false), gameOver(false), paused(false),
      score(0), level(1), linesCleared(0)
{
    current = randomPiece();
    next    = randomPiece();
    computeGhost();
}

int Game::gravityMs() const {
    int ms = 800 - (level - 1) * 75;
    return ms < 50 ? 50 : ms;
}

void Game::computeGhost() {
    ghost = current;
    while (true) {
        Tetromino moved = ghost;
        moved.y += 1;
        if (board.isValidPosition(moved)) {
            ghost = moved;
        } else {
            break;
        }
    }
}

void Game::spawnNext() {
    current = next;
    current.x = 3;
    current.y = 0;
    next = randomPiece();
    holdUsed = false;
    computeGhost();
    if (!board.isValidPosition(current)) {
        gameOver = true;
    }
}

void Game::doHold() {
    if (holdUsed) return;
    if (hold == nullptr) {
        holdPiece = current;
        holdPiece.x = 0; holdPiece.y = 0; holdPiece.rotation = 0;
        hold = &holdPiece;
        spawnNext();
    } else {
        Tetromino tmp = *hold;
        holdPiece = current;
        holdPiece.x = 0; holdPiece.y = 0; holdPiece.rotation = 0;
        hold = &holdPiece;
        current = tmp;
        current.x = 3; current.y = 0; current.rotation = 0;
        computeGhost();
    }
    holdUsed = true;
}

void Game::applyScore(int lines, int softDrop, int hardDrop) {
    static const int lineScores[] = {0, 100, 300, 500, 800};
    if (lines >= 0 && lines <= 4) {
        score += lineScores[lines] * level;
    }
    score += softDrop;
    score += hardDrop * 2;
}

bool tryRotate(Board& board, Tetromino& t, bool cw) {
    Tetromino rotated = cw ? rotatedCW(t) : rotatedCCW(t);
    int fromRot = t.rotation;
    int kickDir = cw ? fromRot : (fromRot + 3) % 4;

    bool isI = (t.type == TetrominoType::I);
    const int (*kicks)[2] = isI ? KICKS_I[kickDir] : KICKS[kickDir];

    for (int i = 0; i < 5; ++i) {
        Tetromino test = rotated;
        test.x += kicks[i][0];
        test.y -= kicks[i][1]; // ncurses y is inverted
        if (board.isValidPosition(test)) {
            t = test;
            return true;
        }
    }
    return false;
}

void Game::handleInput(int key) {
    if (key == 'q' || key == 'Q' || key == 27) { // 27 = ESC
        gameOver = true;
        return;
    }
    if (key == 'p' || key == 'P') {
        paused = !paused;
        return;
    }
    if (paused) return;

    Tetromino moved = current;

    switch (key) {
        case KEY_LEFT:
            moved.x -= 1;
            if (board.isValidPosition(moved)) { current = moved; computeGhost(); }
            break;
        case KEY_RIGHT:
            moved.x += 1;
            if (board.isValidPosition(moved)) { current = moved; computeGhost(); }
            break;
        case KEY_DOWN:
            moved.y += 1;
            if (board.isValidPosition(moved)) {
                current = moved;
                applyScore(0, 1, 0);
                computeGhost();
            }
            break;
        case KEY_UP:
        case 'x': case 'X':
            if (tryRotate(board, current, true)) computeGhost();
            break;
        case 'y': case 'Y':
            if (tryRotate(board, current, false)) computeGhost();
            break;
        case ' ': {
            // Hard drop
            int dropped = 0;
            while (true) {
                Tetromino d = current;
                d.y += 1;
                if (board.isValidPosition(d)) { current = d; ++dropped; }
                else break;
            }
            applyScore(0, 0, dropped);
            board.lockPiece(current);
            int cleared = board.clearLines();
            linesCleared += cleared;
            applyScore(cleared, 0, 0);
            level = 1 + linesCleared / 10;
            spawnNext();
            break;
        }
        case 'c': case 'C':
            doHold();
            break;
    }
}

void Game::tick() {
    if (paused || gameOver) return;
    Tetromino moved = current;
    moved.y += 1;
    if (board.isValidPosition(moved)) {
        current = moved;
        computeGhost();
    } else {
        // Lock piece
        board.lockPiece(current);
        int cleared = board.clearLines();
        linesCleared += cleared;
        applyScore(cleared, 0, 0);
        level = 1 + linesCleared / 10;
        spawnNext();
    }
}

void Game::run() {
    auto lastGravity = Clock::now();
    auto lastDraw    = Clock::now();

    while (!gameOver) {
        auto now = Clock::now();

        // Gravity
        auto gravInterval = Ms(gravityMs());
        if (now - lastGravity >= gravInterval) {
            tick();
            lastGravity = now;
        }

        // Input (non-blocking)
        int key = getch();
        if (key != ERR) {
            handleInput(key);
        }

        // Draw at ~60fps
        if (now - lastDraw >= Ms(16)) {
            renderer.drawAll(board, current, ghost, hold, next, score, level, linesCleared, paused);
            lastDraw = now;
        }
    }

    // Game over screen
    bool quit = false;
    while (!quit) {
        renderer.drawGameOver(score, level, linesCleared);
        int key = getch();
        if (key == 'r' || key == 'R') {
            // Restart
            board.reset();
            hold = nullptr;
            holdUsed = false;
            gameOver = false;
            paused = false;
            score = 0;
            level = 1;
            linesCleared = 0;
            current = randomPiece();
            next = randomPiece();
            computeGhost();
            run(); // recursive restart
            return;
        }
        if (key == 'q' || key == 'Q' || key == 27) {
            quit = true;
        }
    }
}
