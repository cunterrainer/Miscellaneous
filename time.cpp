#include <chrono>
#include <cstdio>
#include <string>
#include <cstdint>
#include <cinttypes>
#include <algorithm>
#include <unordered_map>

#define UNICODE
#define NOMINMAX
#include <Windows.h>

#define DECIMAL_PRECISION 3
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


double get_conversion(uint64_t time)
{
    if (time < 1e6 && time >= 1000) // microseconds
    {
        return 0.001;
    }
    else if (time < 1e9) // milliseconds
    {
        return 0.000001;
    }
    else if (time < 6e10) // seconds
    {
        return 0.000000001;
    }
    else if (time < 3.6e12) // minutes
    {
        return 1.6666666666667E-11;
    }
    else // if (time < 8.64e+13) // hours
    {
        return 2.777777777E-13;
    }
    return 1.0;
}


const char* get_conversion_format(double conversion)
{
    if (conversion == 0.001)
        return "mcs";
    if (conversion == 0.000001)
        return "ms";
    if (conversion == 1.6666666666667E-11)
        return "m";
    if (conversion == 2.777777777E-13)
        return "h";
    return "";
}


int max_time_width(uint64_t time, uint64_t time2, uint64_t time3, uint64_t time4)
{
    const size_t timeWidth = std::to_string(static_cast<uint64_t>(time * get_conversion(time))).size();
    const size_t time2Width = std::to_string(static_cast<uint64_t>(time2 * get_conversion(time2))).size();
    const size_t time3Width = std::to_string(static_cast<uint64_t>(time3 * get_conversion(time3))).size();
    const size_t time4Width = std::to_string(static_cast<uint64_t>(time4 * get_conversion(time4))).size();

    size_t maxWidth = std::max(timeWidth, time2Width);
    maxWidth = std::max(maxWidth, time3Width);
    return (int)std::max(maxWidth, time4Width);
}



void print_time(uint64_t time, const char* fmt, int formatWidth)
{
    if (time < 1000) // nanoseconds
    {
        printf("%s %" PRId64 " ns", fmt, time);
        PRINT_SECONDS_END(time);
        return;
    }

    const double conversion = get_conversion(time);
    if (conversion == 0.000000001)
    {
        printf("%s %*.*f sec\n", fmt, formatWidth + 1 + DECIMAL_PRECISION, DECIMAL_PRECISION, time * conversion);
    }
    else
    {
        printf("%s %*.*f %s", fmt, formatWidth + 1 + DECIMAL_PRECISION, DECIMAL_PRECISION, time * conversion, get_conversion_format(conversion));
        PRINT_SECONDS_END(time);
    }
}


void time_app(LPWSTR args)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const auto t_start = std::chrono::high_resolution_clock::now();
    if (!CreateProcessW(NULL, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        fprintf(stderr, "Failed to create process '%S': %s\n", args, error_as_string().c_str());
        return;
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
    const uint64_t totalCpuTime = kernelTimeFinal + userTimeFinal;
    const uint64_t realTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count());
    puts("");
    const int maxWidth = max_time_width(realTime, userTimeFinal, kernelTimeFinal, totalCpuTime);
    print_time(realTime,        "Real time:     ", maxWidth);
    print_time(userTimeFinal,   "User time:     ", maxWidth);
    print_time(kernelTimeFinal, "Kernel time:   ", maxWidth);
    print_time(totalCpuTime,    "Total CPU time:", maxWidth);
}


BOOL CtrlHandler(DWORD fdwCtrlType)
{
    return fdwCtrlType == CTRL_C_EVENT;
}


int main()
{
    int argc = 0;
    const LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 2)
    {
        std::string error = error_as_string();
        if (error.empty())
            fprintf(stderr, "Usage: %S [command] [arg 1] [arg 2] ...\n", argv[0]);
        else
            fprintf(stderr, "Failed to get command line arguments: %s\n", error.c_str());
        return 0;
    }

    // no real routine needed, because if ctrl + c used
    // subprocess will be killed and the handler will return
    // controll flow to this program
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

    std::basic_string<WCHAR> args;
    for (int i = 1; i < argc; ++i)
    {
        args += argv[i];
        if (i != argc - 1)
            args += ' ';
    }
    time_app(args.data());
}