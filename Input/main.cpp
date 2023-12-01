#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "Snake.h"
#include "tc.h"
#include "Input.h"

void test(void* er)
{
    std::cout << *(int*)er << std::endl;
}

int main()
{
    //Snake snake;
    //snake.setDirection(RIGHT);
    //tcEchoOn();
    tcEchoOff();
    KEYL* keyl = klInitKeyboardListener();
    while(1)
    {
        int k = klKeyPressed();
        if(k == KEY_Z)
            printf("uhu\n");
        if(k == KEY_9)
            break;
    }
    klCloseKeyboardListener(keyl);
    tcEchoOn();
    //while(!snake.p_Destroyed)
    //{
    //    snake.print();
    //    snake.update();
    //    //printf("yes\n");
    //    std::this_thread::sleep_for(std::chrono::seconds(1));
    //    //getchar();
    //}
}
