#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <Windows.h>

#define KEY_PICK   VK_F4
#define KEY_START  VK_F8
#define KEY_ESCAPE VK_ESCAPE

bool KeyIsPressed(int key)
{
    return GetAsyncKeyState(key) & 0x01; // If the most significant bit is set, the key is down
}


void ClearLine()
{
    std::string line(100, ' ');
    std::cout << '\r' << line;
}


POINT PickLocation()
{
    ClearLine();
    std::cout << "\rPicking location... (Press F4 to pick)";

    POINT pos;
    do
    {
        GetCursorPos(&pos);
    } while(!KeyIsPressed(KEY_PICK));

    ClearLine();
    std::cout << "\rx: " << pos.x << " y: " << pos.y;
    return pos;
}


void Click(POINT pos, bool locPicked)
{
    INPUT Inputs[2] = {0};
    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    std::cout << "clicking\n";
    while(!KeyIsPressed(KEY_START))
    {
        if(locPicked)
            SetCursorPos(pos.x, pos.y);
        SendInput(2, Inputs, sizeof(INPUT));
    }
    std::cout << "end";
}


int main()
{
    std::cout << "F4 [Pick location] | F8 [Start]" << std::endl;
    POINT pos;
    bool locPicked = false;

    while(!KeyIsPressed(KEY_ESCAPE))
    {
        if(KeyIsPressed(KEY_PICK))
        {
            pos = PickLocation();
            locPicked = true;
        }

        if(KeyIsPressed(KEY_START))
            Click(pos, locPicked);

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }


    //INPUT Inputs[2] = {0};
    //Inputs[0].type = INPUT_MOUSE;
    //Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
//
    //Inputs[1].type = INPUT_MOUSE;
    //Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
//
    //SetCursorPos(-1634, 494);
    //SendInput(2, Inputs, sizeof(INPUT));
//
    //POINT pos;
    //GetCursorPos(&pos);
    //std::cout << pos.x << " " << pos.y << std::endl;

    return 0;
} 