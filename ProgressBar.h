#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define PROGRESS_BAR_SIZE 31 // 31: msys2 default
#define PROGRESS_BAR_REAL_SIZE (PROGRESS_BAR_SIZE + 3)

typedef struct
{
    char* bar;
    size_t size;
} ProgressBarS;


static ProgressBarS* ProgressBarGet()
{
    static char bar[PROGRESS_BAR_REAL_SIZE];
    static ProgressBarS pBar = { bar, 0 };
    return &pBar;
}


static void ProgressBarInit()
{
    ProgressBarS* pBar = ProgressBarGet();
    pBar->size = 0;
    char* bar = pBar->bar;
    bar[0] = '[';
    bar[PROGRESS_BAR_REAL_SIZE - 2] = ']';
    bar[PROGRESS_BAR_REAL_SIZE - 1] = '\0';
    for(size_t i = 1; i < PROGRESS_BAR_REAL_SIZE - 2; ++i)
        bar[i] = '-';
}


static void ProgressBarAdd(size_t blocks)
{
    ProgressBarS* pBar = ProgressBarGet();
    blocks = blocks - pBar->size;
    for(size_t i = 1; i < PROGRESS_BAR_SIZE + 1; ++i)
    {
        if(pBar->bar[i] == '-')
        {
            if(i + blocks > (PROGRESS_BAR_SIZE + 1))
                blocks = PROGRESS_BAR_SIZE - pBar->size;
            memset((void*)&pBar->bar[i], '#', blocks * sizeof(char));
            pBar->size += blocks;
            return;
        }
    }
}


static void ProgressBar(float current, float hundred)
{
    const float percentDone = (current / hundred) * 100.f;
    //#define PROGRESS_BAR_BLOCK_OFFSET (100.f / (float)PROGRESS_BAR_SIZE)
    //const size_t newBlocksToAdd = (size_t)(percentDone / PROGRESS_BAR_BLOCK_OFFSET); // mine the result of both should be the same
    const size_t newBlocksToAdd = (size_t)(percentDone * PROGRESS_BAR_SIZE / 100); // Jacob Sorber
    ProgressBarAdd(newBlocksToAdd);

    printf("%s %lld%% (%.2f | %.2f)\r", ProgressBarGet()->bar, (size_t)percentDone, current, hundred);
    if(percentDone == 100.f)
        printf("\n");
}

#endif