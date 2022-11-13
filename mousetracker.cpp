#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <Windows.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH      1920
#define HEIGHT     1080
#define WIDTH_IDX  (WIDTH - 1)
#define HEIGHT_IDX (HEIGHT - 1)

inline bool KeyIsPressed(int key)
{
    return GetAsyncKeyState(key) & 0x01; // If the most significant bit is set, the key is down
}


inline std::string GetTime()
{
    const std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss(std::ctime(&end_time));
    std::string segment;
    std::vector<std::string> seglist;

    while(std::getline(ss, segment, ' '))
        seglist.push_back(segment);
    return seglist[3];
}


constexpr int GetPos(int x, int y, int gridcols)
{
    if(x < 0 || y < 0 || x > WIDTH_IDX || y > HEIGHT_IDX)
        return -1;
    return (gridcols * y) + x;
}


inline void WriteImage(unsigned char* data)
{
    if(stbi_write_png("tracking.png", WIDTH, HEIGHT, 1, data, WIDTH) == 0)
    {
        std::cerr << '[' << GetTime() << "] Error writing image" << std::endl;
        std::cin.ignore();
    }
}


int main()
{
    unsigned char* data = new unsigned char[WIDTH*HEIGHT];
    for(size_t i = 0; i < WIDTH*HEIGHT; ++i)
        data[i] = 255;

    std::cout << "Tracking started..." << std::endl;
    POINT pos;
    do
    {
        if(GetCursorPos(&pos) == 0)
        {
            std::cerr << "[" << GetTime() << "] GetCursorPos() error: " << GetLastError() << std::endl;
            std::cin.ignore();
            WriteImage(data);
            delete[] data;
            return 1;
        }

        const int index = GetPos(pos.x, pos.y, WIDTH);
        if(index != -1)
            data[index] = 0;
    } while(!KeyIsPressed(VK_F12));

    WriteImage(data);
    delete[] data;
    std::cout << "Tracking ended!" << std::endl;
    std::cin.ignore();
    return 0;
} 