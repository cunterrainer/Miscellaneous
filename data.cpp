#include <filesystem>
#include <chrono>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cerrno>
#include <Windows.h>

#include "ProgressBar.h"

#define FOLDER "files/"
#define NUM_DOTS  1000000 // ~1MB (1000000)
#define ADD_DONE_BUFF 50


void CreateFolder(const std::filesystem::path& path)
{
    if (!std::filesystem::is_directory(path) || !std::filesystem::exists(path))
    {
        printf("-- Creating folder '%s'", path.u8string().c_str());
        std::filesystem::create_directory(path);
        puts(" | Done");
    }
}


void DeleteFolder(const std::filesystem::path& path)
{
    if(std::filesystem::is_directory(path))
    {
        printf("-- Removing folder '%s'", path.u8string().c_str());
        std::filesystem::remove_all(path);
        puts(" | Done");
    }
}


uint64_t GetFreeDiskSpace()
{
    uint64_t lpFreeBytesAvailableToCaller = 1;
    uint64_t lpTotalNumberOfBytes = 1;
    uint64_t lpTotalNumberOfFreeBytes = 1;

    if(!GetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)&lpFreeBytesAvailableToCaller, (PULARGE_INTEGER)&lpTotalNumberOfBytes, (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes))
    {
        unsigned long error = GetLastError();
        printf("GetDiskFreeSpaceEx failed, Error code: %lu\n", error);
        return 0;
    }

    constexpr float gbDenominator = 1024.f*1024.f*1024.f;
    printf("\n-- Total space               = %llu bytes | %.2f GB\n", lpTotalNumberOfBytes, lpTotalNumberOfBytes / gbDenominator);
    printf("-- Available space to caller = %llu bytes | %.2f GB\n", lpFreeBytesAvailableToCaller, lpFreeBytesAvailableToCaller / gbDenominator);
    printf("-- Free space on drive       = %llu bytes | %.2f GB\n", lpTotalNumberOfFreeBytes, lpTotalNumberOfFreeBytes / gbDenominator);
    return lpTotalNumberOfFreeBytes;
}


char* CreateContent()
{
    char* content = new char[NUM_DOTS + 1];
    for(size_t i = 0; i < NUM_DOTS; ++i)
        content[i] = '.';
    content[NUM_DOTS] = '\0';
    return content;
}


uint64_t UpdateProgressBar(uint16_t toAdd)
{
    static uint64_t filesDone = 0;
    static std::mutex mtx;
    std::scoped_lock<std::mutex> lock(mtx);
    filesDone += toAdd;
    return filesDone;
}


unsigned int FinishedThreads(uint8_t toAdd)
{
    static unsigned int finished = 0;
    static std::mutex mtx;
    std::scoped_lock<std::mutex> lock(mtx);
    finished += toAdd;
    return finished;
}


void WriteFiles(uint64_t filesToCreate, const char* content, unsigned int folderNum)
{
    const std::string folderPath(FOLDER + std::to_string(folderNum) + '/');
    std::string filePath;
    filePath.reserve(31);

    uint64_t i = 0;
    for(; i < filesToCreate; ++i)
    {
        filePath = folderPath;
        filePath += std::to_string(i) + ".txt";

        FILE* fp = fopen(filePath.c_str(), "w");
        if(fp == NULL) {
            printf("\nThread<%u>: Failed to create file %llu (File pointer was NULL)\n", folderNum, i);
            break;
        }

        if(fprintf(fp, content) < 0)
        {
            printf("\nThread<%u>: Failed to write content into file Error: %s\n", folderNum, strerror(errno));
            break;
        }
        fclose(fp);
        if((i + 1) % ADD_DONE_BUFF == 0)
            UpdateProgressBar(ADD_DONE_BUFF);
    }
    UpdateProgressBar(i % ADD_DONE_BUFF);
    FinishedThreads(1);
}


void CreateAndWrite(const char* content, unsigned int numOfThreads, uint64_t filesToCreate, uint64_t filesPerThread, uint8_t remainingFiles)
{
    // create one folder per thread
    for(unsigned int i = 0; i < numOfThreads; ++i)
    {
        const std::string folderPath(FOLDER + std::to_string(i) + '/');
        CreateFolder(folderPath);
    }

    puts("");
    ProgressBarInit();
    uint64_t filesDone = UpdateProgressBar(0);

    std::vector<std::thread> threads;
    threads.reserve(numOfThreads);

    for(unsigned int i = 0; i < numOfThreads; ++i)
    {
        threads.emplace_back(WriteFiles, filesPerThread + remainingFiles, content, i);
        remainingFiles = 0;
    }

    while(filesDone < filesToCreate && FinishedThreads(0) < numOfThreads)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        filesDone = UpdateProgressBar(0);
        ProgressBar(filesDone, filesToCreate);
    }
    if(filesDone < filesToCreate)
        ProgressBar(UpdateProgressBar(0), filesToCreate);

    for(std::thread& t : threads)
        t.join();
    puts("");
}


int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    if(argc == 1 || argc > 3)
    {
        printf("Usage: %s [iterations] [option]\n", argv[0]);
        puts("       -k: keep the folder and created files");
        return 1;
    }
    const int iterations = std::stoi(argv[1]);
    bool keepFolder = false;
    if(argc == 3)
    {
        const std::string argv2(argv[2]);
        if(argv2 == "-k" || argv2 == "-K")
        {
            keepFolder = true;
            puts("-- Keep folder after iterations");
        }
    }
    DeleteFolder(FOLDER);

    const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    uint64_t freeDiskSpace = GetFreeDiskSpace();
    if(freeDiskSpace == 0)
    {
        puts("Either failed to retrieve available disk space or no space is available");
        return 1;
    }
    const char* content = CreateContent();

    const unsigned int numOfThreads = std::thread::hardware_concurrency();
    const unsigned int threadsToSpawn = numOfThreads - 1;
    const uint64_t filesToCreate  = freeDiskSpace / (NUM_DOTS * sizeof(char));
    const uint64_t filesPerThread = filesToCreate / threadsToSpawn;
    const uint8_t  remainingFiles = filesToCreate % threadsToSpawn;
    printf("\n-- Threads: %u\n-- Threads to spawn: %u\n-- Files to create: %llu\n-- Files per thread: %llu\n-- Remaining: %d\n\n", numOfThreads, threadsToSpawn, filesToCreate, filesPerThread, remainingFiles);
    
    for(int i = 0; i < iterations; ++i)
    {
        DeleteFolder(FOLDER);
        CreateFolder(FOLDER);
        CreateAndWrite(content, threadsToSpawn, filesToCreate, filesPerThread, remainingFiles);
    }
    if(!keepFolder)
        DeleteFolder(FOLDER);

    delete[] content;
    printf("Execution time: %.2lf sec(s)\n", std::chrono::duration<double, std::ratio<1>>(std::chrono::high_resolution_clock::now() - startTime).count());
    return 0;
}