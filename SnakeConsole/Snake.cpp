#include <iostream>

#include "Snake.h"
#include "tc.h"

void printBoard()
{
    tcClearScreen();
    tcMoveCursor(0, 0);
    for(int i = 0; i < BOARD_SIZE_H; ++i)
        putc('#', stdout);
    putc('\n', stdout);

    for(int i = 0; i < BOARD_SIZE_V; ++i)
    {
        putc('#', stdout);
        for(int j = 0; j < BOARD_SIZE_H - 2; ++j)
            putc(' ', stdout);
        putc('#', stdout);
        putc('\n', stdout);
    }

    for(int i = 0; i < BOARD_SIZE_H; ++i)
        putc('#', stdout);
    putc('\n', stdout);
}

//public
void Snake::print()
{
    printBoard();

    tcMoveCursor(m_Head.x_Pos, m_Head.y_Pos);
    putc('#', stdout);
    putc('\n', stdout);

    std::size_t parts = m_Parts.size();
    for(std::size_t i = 0; i < parts; ++i)
    {
        tcMoveCursor(m_Parts[i].x_Pos, m_Parts[i].y_Pos);
        putc('#', stdout);
    }
}

void Snake::update()
{
    if(collided())
    {
        p_Destroyed = true;
        tcMoveCursor(0, BOARD_SIZE_V + 3);
        printf("You failed\n");
        return;
    }
    move();
}

void Snake::setDirection(const char& dir)
{
    switch(dir)
    {
        case UP:
        case RIGHT:
        case DOWN:
        case LEFT:
            m_Direction = dir;
            break;
        default:
            break;
    }
}

//private
void Snake::move()
{
    switch(m_Direction)
    {
        case UP:
            m_Head.y_Pos--;
            break;
        case RIGHT:
            m_Head.x_Pos++;
            break;
        case DOWN:
            m_Head.y_Pos++;
            break;
        case LEFT:
            m_Head.x_Pos--;
            break;
        default:
            break;
    }
}

bool Snake::collided()
{
    if(m_Head.x_Pos == BOARD_SIZE_H)
        return true;
    if(m_Head.x_Pos == 1)
        return true;
    if(m_Head.y_Pos == 1)
        return true;
    if(m_Head.y_Pos == BOARD_SIZE_V + 2)
        return true;
    return false;
}
