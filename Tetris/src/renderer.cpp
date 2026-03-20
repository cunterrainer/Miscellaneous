#include "renderer.h"
#include <ncurses.h>
#include <string>
#include <cstring>
#include <locale.h>

// Color pair indices
// 1-7: piece colors (I,O,T,S,Z,J,L)
// 8: ghost (dim)
// 9: border
// 10: panel text

static void initColors() {
    start_color();
    use_default_colors();
    // Piece colors: I=cyan, O=yellow, T=magenta, S=green, Z=red, J=blue, L=white(orange approx)
    init_pair(1, COLOR_CYAN,    COLOR_CYAN);    // I
    init_pair(2, COLOR_YELLOW,  COLOR_YELLOW);  // O
    init_pair(3, COLOR_MAGENTA, COLOR_MAGENTA); // T
    init_pair(4, COLOR_GREEN,   COLOR_GREEN);   // S
    init_pair(5, COLOR_RED,     COLOR_RED);     // Z
    init_pair(6, COLOR_BLUE,    COLOR_BLUE);    // J
    init_pair(7, COLOR_WHITE,   COLOR_WHITE);   // L
    init_pair(8, COLOR_BLACK,   COLOR_BLACK);   // ghost (dark)
    init_pair(9, COLOR_WHITE,   -1);            // border text
    init_pair(10, COLOR_WHITE,  -1);            // panel text
}

Renderer::Renderer() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    if (has_colors()) {
        initColors();
    }
    // Compute centering offset
    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    // Board area: 2 + BOARD_H rows tall, (2 + BOARD_W*2) cols wide
    // Panel: ~12 cols to the right
    int totalW = 2 + BOARD_W * 2 + 2 + 12; // board borders + board + gap + panel
    int totalH = 2 + BOARD_H + 2; // +2 for controls line
    offsetY = (termH - totalH) / 2;
    offsetX = (termW - totalW) / 2;
    if (offsetY < 0) offsetY = 0;
    if (offsetX < 0) offsetX = 0;
}

Renderer::~Renderer() {
    endwin();
}

void Renderer::drawCell(int screenY, int screenX, int colorIdx) {
    if (colorIdx == 0) {
        // Empty cell
        attron(COLOR_PAIR(9));
        mvaddstr(screenY, screenX, "  ");
        attroff(COLOR_PAIR(9));
        return;
    }
    if (colorIdx == 8) {
        // Ghost
        attron(COLOR_PAIR(9) | A_DIM);
        mvaddstr(screenY, screenX, "[]");
        attroff(COLOR_PAIR(9) | A_DIM);
        return;
    }
    attron(COLOR_PAIR(colorIdx) | A_BOLD);
    mvaddstr(screenY, screenX, "\u2588\u2588");
    attroff(COLOR_PAIR(colorIdx) | A_BOLD);
}

void Renderer::drawBoard(const Board& board, const Tetromino& current, const Tetromino& ghost) {
    int by = offsetY + 1; // inside border
    int bx = offsetX + 2; // inside border (1 border char + 1 space... actually border is '║')

    // Collect current and ghost cells
    auto curCells = cells(current);
    auto ghoCells = cells(ghost);

    // Draw border
    attron(COLOR_PAIR(9));
    // Top
    mvaddstr(offsetY, offsetX, "\u2554");
    for (int c = 0; c < BOARD_W; ++c) mvaddstr(offsetY, offsetX + 1 + c*2, "\u2550\u2550");
    mvaddstr(offsetY, offsetX + 1 + BOARD_W*2, "\u2557");
    // Bottom
    mvaddstr(offsetY + BOARD_H + 1, offsetX, "\u255a");
    for (int c = 0; c < BOARD_W; ++c) mvaddstr(offsetY + BOARD_H + 1, offsetX + 1 + c*2, "\u2550\u2550");
    mvaddstr(offsetY + BOARD_H + 1, offsetX + 1 + BOARD_W*2, "\u255d");
    // Sides
    for (int r = 0; r < BOARD_H; ++r) {
        mvaddstr(offsetY + 1 + r, offsetX, "\u2551");
        mvaddstr(offsetY + 1 + r, offsetX + 1 + BOARD_W*2, "\u2551");
    }
    attroff(COLOR_PAIR(9));

    // Draw cells
    for (int row = 0; row < BOARD_H; ++row) {
        for (int col = 0; col < BOARD_W; ++col) {
            int sy = by + row;
            int sx = offsetX + 1 + col * 2;
            int cell = board.cellAt(row, col);

            // Check ghost
            bool isGhost = false;
            for (auto [gc, gr] : ghoCells) {
                if (gc == col && gr == row) { isGhost = true; break; }
            }
            // Check current
            bool isCurrent = false;
            int curColor = 0;
            for (auto [cc, cr] : curCells) {
                if (cc == col && cr == row) {
                    isCurrent = true;
                    curColor = static_cast<int>(current.type) + 1;
                    break;
                }
            }

            if (isCurrent) {
                drawCell(sy, sx, curColor);
            } else if (cell != 0) {
                drawCell(sy, sx, cell);
            } else if (isGhost) {
                drawCell(sy, sx, 8);
            } else {
                // Empty — draw dot or space
                attron(COLOR_PAIR(9) | A_DIM);
                mvaddstr(sy, sx, "\xc2\xb7 "); // middle dot
                attroff(COLOR_PAIR(9) | A_DIM);
            }
        }
    }
}

static void drawMiniPiece(const Tetromino* t, int originY, int originX) {
    if (!t) return;
    int type = static_cast<int>(t->type);
    // Find bounding box
    int minRow = 4, maxRow = 0, minCol = 4, maxCol = 0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (SHAPES[type][0][r][c]) {
                if (r < minRow) minRow = r;
                if (r > maxRow) maxRow = r;
                if (c < minCol) minCol = c;
                if (c > maxCol) maxCol = c;
            }
    int h = maxRow - minRow + 1;
    int w = maxCol - minCol + 1;
    int startY = originY + (2 - h) / 2;
    int startX = originX + (4 - w);

    int colorIdx = type + 1;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (SHAPES[type][0][r][c]) {
                int sy = startY + (r - minRow);
                int sx = startX + (c - minCol) * 2;
                attron(COLOR_PAIR(colorIdx) | A_BOLD);
                mvaddstr(sy, sx, "\u2588\u2588");
                attroff(COLOR_PAIR(colorIdx) | A_BOLD);
            }
        }
    }
}

void Renderer::drawPanel(const Tetromino* hold, const Tetromino& next, int score, int level, int lines) {
    int px = offsetX + 2 + BOARD_W * 2 + 2; // panel x
    int py = offsetY;

    attron(COLOR_PAIR(9));

    // NEXT box  (box is 12 cols wide: ║ + 10 inner + ║)
    mvaddstr(py,   px, "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557");
    mvaddstr(py+1, px, "\u2551 NEXT     \u2551");
    mvaddstr(py+2, px, "\u2551          \u2551");
    mvaddstr(py+3, px, "\u2551          \u2551");
    mvaddstr(py+4, px, "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d");

    // HOLD box
    mvaddstr(py+5, px, "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557");
    mvaddstr(py+6, px, "\u2551 HOLD     \u2551");
    mvaddstr(py+7, px, "\u2551          \u2551");
    mvaddstr(py+8, px, "\u2551          \u2551");
    mvaddstr(py+9, px, "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d");

    // STATS box
    mvaddstr(py+10, px, "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557");
    mvaddstr(py+11, px, "\u2551 SCORE    \u2551");
    mvaddstr(py+12, px, "\u2551          \u2551");
    mvaddstr(py+13, px, "\u2551          \u2551");
    mvaddstr(py+14, px, "\u2551          \u2551");
    mvaddstr(py+15, px, "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d");

    attroff(COLOR_PAIR(9));

    // Draw next piece
    drawMiniPiece(&next, py + 2, px + 2);

    // Draw hold piece
    drawMiniPiece(hold, py + 7, px + 2);

    // Score values
    attron(COLOR_PAIR(10) | A_BOLD);
    char buf[32];
    snprintf(buf, sizeof(buf), "%010d", score);
    mvaddstr(py+12, px+1, buf);
    snprintf(buf, sizeof(buf), " LEVEL %3d", level);
    mvaddstr(py+13, px+1, buf);
    snprintf(buf, sizeof(buf), " LINES %3d", lines);
    mvaddstr(py+14, px+1, buf);
    attroff(COLOR_PAIR(10) | A_BOLD);

    // Controls at bottom
    int cy = offsetY + BOARD_H + 2;
    attron(COLOR_PAIR(9) | A_DIM);
    mvaddstr(cy, offsetX, "\u2190\u2192:Move \u2193:Soft \u2191/X:CW Y:CCW SPC:Drop C:Hold P:Pause Q:Quit");
    attroff(COLOR_PAIR(9) | A_DIM);
}

void Renderer::drawAll(const Board& board, const Tetromino& current, const Tetromino& ghost,
                       const Tetromino* hold, const Tetromino& next,
                       int score, int level, int lines, bool paused) {
    erase();
    drawBoard(board, current, ghost);
    drawPanel(hold, next, score, level, lines);
    if (paused) {
        int cy = offsetY + BOARD_H / 2;
        int cx = offsetX + 1;
        attron(COLOR_PAIR(9) | A_BOLD | A_REVERSE);
        mvaddstr(cy, cx, "     PAUSED     ");
        attroff(COLOR_PAIR(9) | A_BOLD | A_REVERSE);
    }
    refresh();
}

void Renderer::drawGameOver(int score, int level, int lines) {
    erase();
    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    int cy = termH / 2 - 3;
    int cx = termW / 2 - 10;

    attron(COLOR_PAIR(5) | A_BOLD);
    mvaddstr(cy,   cx, "  *** GAME OVER ***  ");
    attroff(COLOR_PAIR(5) | A_BOLD);

    attron(COLOR_PAIR(10));
    char buf[64];
    snprintf(buf, sizeof(buf), "  Score: %d", score);
    mvaddstr(cy+2, cx, buf);
    snprintf(buf, sizeof(buf), "  Level: %d", level);
    mvaddstr(cy+3, cx, buf);
    snprintf(buf, sizeof(buf), "  Lines: %d", lines);
    mvaddstr(cy+4, cx, buf);
    mvaddstr(cy+6, cx, "  Press R to restart or Q to quit");
    attroff(COLOR_PAIR(10));

    refresh();
}
