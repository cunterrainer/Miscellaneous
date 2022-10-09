#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <Windows.h>

#define KEY_PICK   VK_F4
#define KEY_MPOS   VK_F2
#define KEY_START  VK_F8
#define KEY_ESCAPE VK_ESCAPE

bool KeyIsPressed(int key)
{
    return GetAsyncKeyState(key) & 0x01; // If the most significant bit is set, the key is down
}


std::ostream& ClearLine()
{
    static const std::string line(100, ' ');
    std::cout << '\r' << line << '\r';
    return std::cout;
}


POINT PickLocation()
{
    ClearLine() << "Picking location... (Press F4 to pick)";

    POINT pos;
    do
    {
        GetCursorPos(&pos);
    } while(!KeyIsPressed(KEY_PICK));

    ClearLine() << "x: " << pos.x << " y: " << pos.y;
    return pos;
}


void Click(POINT pos, bool locPicked)
{
    INPUT Inputs[2] = {0};
    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    while(!KeyIsPressed(KEY_START) && !KeyIsPressed(KEY_ESCAPE))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if(locPicked)
        {
            SetCursorPos(pos.x, pos.y);
        }
        SendInput(2, Inputs, sizeof(INPUT));
    }
}


int main()
{
    std::cout << "F2 [Use mouse position (default)] | F4 [Pick location] | F8 [Start] | ESC [End]" << std::endl;
    std::cout << "Current mouse position";
    POINT pos;
    bool locPicked = false;

    while(!KeyIsPressed(KEY_ESCAPE))
    {
        if(KeyIsPressed(KEY_PICK))
        {
            pos = PickLocation();
            locPicked = true;
        }

        if(KeyIsPressed(KEY_MPOS))
        {
            ClearLine() << "Current mouse position";
            locPicked = false;
        }

        if(KeyIsPressed(KEY_START))
            Click(pos, locPicked);

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    return 0;
} 