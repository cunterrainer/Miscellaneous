#include <chrono>
#include <cstdio>
#include <string>
#include <cstdint>
#include <cinttypes>

#undef UNICODE
#include <Windows.h>

#define DECIMAL_PRECISION  3
#define FORMAT_TOTAL_WIDTH 5 + DECIMAL_PRECISION
#define PRINT_SECONDS_END(time) printf(" (%.*f sec)\n", DECIMAL_PRECISION, time * 0.000000001)

std::string error_as_string()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0)
        return std::string();

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}


void print_time(uint64_t time, const char* fmt)
{
    if (time < 1000) // nanoseconds
    {
        printf("%s %*" PRId64 " ns", fmt, 4, time);
        PRINT_SECONDS_END(time);
    }
    else if (time < 1e6) // microseconds
    {
        printf("%s %*.*f mcs", fmt, FORMAT_TOTAL_WIDTH, DECIMAL_PRECISION, time * 0.001);
        PRINT_SECONDS_END(time);
    }
    else if (time < 1e9) // milliseconds
    {
        printf("%s %*.*f ms", fmt, FORMAT_TOTAL_WIDTH, DECIMAL_PRECISION, time * 0.000001);
        PRINT_SECONDS_END(time);
    }
    else if (time < 6e10) // seconds
    {
        printf("%s %*.*f sec\n", fmt, FORMAT_TOTAL_WIDTH, DECIMAL_PRECISION, time * 0.000000001);
    }
    else if (time < 3.6e12) // minutes
    {
        printf("%s %*.*f min", fmt, FORMAT_TOTAL_WIDTH, DECIMAL_PRECISION, time * 1.6666666666667E-11);
        PRINT_SECONDS_END(time);
    }
    else // if (time < 8.64e+13) // hours
    {
        printf("%s %*.*f h", fmt, FORMAT_TOTAL_WIDTH, DECIMAL_PRECISION, time * 2.777777777E-13);
        PRINT_SECONDS_END(time);
    }
}


BOOL CtrlHandler(DWORD fdwCtrlType)
{
    return fdwCtrlType == CTRL_C_EVENT;
}


int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s [command] [arg 1] [arg 2] ...\n", argv[0]);
        return 0;
    }

    std::string args;
    for (int i = 1; i < argc; ++i)
    {
        args += argv[i];
        args += ' ';
    }

    // no real routine needed, because if ctrl + c used
    // subprocess will be killed and the handler will return
    // controll flow to this program
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const auto t_start = std::chrono::high_resolution_clock::now();
    if (!CreateProcess(NULL, args.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        printf("Failed to create process '%s': %s\n", args.c_str(), error_as_string().c_str());
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    const auto t_end = std::chrono::high_resolution_clock::now();

    // Measure CPU time
    FILETIME creationTime, exitTime, kernelTime, userTime;
    GetProcessTimes(pi.hProcess, &creationTime, &exitTime, &kernelTime, &userTime);

    ULARGE_INTEGER userTimeInt;
    userTimeInt.LowPart = userTime.dwLowDateTime;
    userTimeInt.HighPart = userTime.dwHighDateTime;

    ULARGE_INTEGER kernelTimeInt;
    kernelTimeInt.LowPart = kernelTime.dwLowDateTime;
    kernelTimeInt.HighPart = kernelTime.dwHighDateTime;

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Calculate CPU time in nanoseconds
    const uint64_t userTimeFinal = userTimeInt.QuadPart * 100;
    const uint64_t kernelTimeFinal = kernelTimeInt.QuadPart * 100;
    const uint64_t realTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count());
    puts("");
    print_time(realTime,                        "Real time:    ");
    print_time(userTimeFinal,                   "User time:    ");
    print_time(kernelTimeFinal,                 "Kernel time:  ");
    print_time(kernelTimeFinal + userTimeFinal, "Combined time:");
    return 0;
}
