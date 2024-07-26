Most of the programs/header files in here are self explanatory

## Hash.h (C/C++)
- SHA-1
- SHA-2 family
    - SHA-224
    - SHA-256
    - SHA-384
    - SHA-512
    - SHA-512/224
    - SHA-512/256
- SHA-3 family
    - SHA3-224
    - SHA3-256
    - SHA3-384
    - SHA3-512
- Shake-128
- Shake-256
- MD5

The documentation is in the header  
The Shake function can not be updated, data has to be supplied in a single string

## hash.zig or hash.cpp
Hash strings, files or directories. Functionality to print all files that are the same in a directory and search for files based on their hash value (C++ only). It is recommended to use the C++ version, it is newer and has more features.
- SHA-1
- SHA-2 family
    - SHA-224
    - SHA-256
    - SHA-384
    - SHA-512
    - SHA-512/256
- SHA-3 family
    - SHA3-224
    - SHA3-256
    - SHA3-384
    - SHA3-512
- MD5

## hash_benchmark.cpp
Benchmark the performance of the hash implementations in [Hash.h](#hashh-cc)

## hash_test.cpp
Tests for the hash implementations in [Hash.h](#hashh-cc)

## hash_test_generator.py
Generate tests for the hash implementations in [Hash.h](#hashh-cc)

## Hash.dart
Sha256 implementation in Dart

## Core.h
Defines macros to check for things like operating system, platform, compiler, architecture and more.  
Documentation is in the [file](https://github.com/cunterrainer/Miscellaneous/blob/main/Core.h)

Example
``` c++
#include <stdio.h>
#include "Core.h"

int main()
{
    #if CORE_ARCH_X64 // you can use either #if or #ifdef
    puts("X64 Processor");
    #endif

    #if CORE_OS_WINDOWS
    puts("I'm on windows");
    #endif

    #if CORE_PLATFORM_WEB
    puts("Building for web assembly");
    #endif

    #if CORE_COMP_CLANG
    puts("Using clang");
    #endif

    #if CORE_LANG_STDCPP_C17
    puts("Using C++ 17");
    #endif

    #if CORE_LIB_STD_CXX
    puts("Using libc++ as stdlib");
    #endif

    #if CORE_MODE_RELEASE
    puts("In release mode");
    #endif

    #if CORE_HW_SIMD_X86
    puts("Simd x86 supported");
    #endif

    #if CORE_LANG_STDC_C99
    puts("Using version C 99");
    #endif

    #if CORE_LIB_C_GNU
    puts("Using gnu libc");
    #endif

    // Alternatively, works for all types
    puts(CORE_OS_NAME)
    return 0;
}
```

## Profiler.h
Measure the execution time of code snippets e.g. in loops.  
Example (simplified, from hash_benchmark.cpp)
``` c++
for (const std::string& s : strings)
{
    Profiler::Start();
    sha256(s);
    Profiler::End();
}
Profiler::Log(Profiler::Conversion::Milliseconds); // print to stdout
```

## time.cpp
Time utility from Linux for Windows

## ProgressBar.h
A simple progress bar for C/C++
``` c
int main()
{
    ProgressBarInit();

    for (int i = 0; i <= 100; i++)
    {
        ProgressBar(i, 100);
    }
    ProgressBarInit(); // If you want to use it again
}
```

## Result.h
Implementation of the Rust result type in C++
``` c++
Result<std::string> ReadFile(const char* p)
{
    // open file
    if (file_is_open == false)
        return Err("Failed to open file: %s", p);
    // read content
    return content;
    // return Ok(content); // alternative, if not implicitly convertible Ok<std::string>(content)
}

Result<std::string> r = ReadFile("test.txt");
if (r) // if valid, can also use r.IsOk() or r.IsErr()
{
    const std::string& str = r.Ok();
}
else // warning r.Ok() or r.Err() are not allowed to be used if valid or invalid respectively
{
    std::cout << r.Err().what() << std::endl;
}
```

The documentation is in the header  

## snake_console.c
A snake implementation meant to be played in the console, supports Windows, Linux and macOS  
Compiling on Windows:
``` bash
gcc snake_console.c -lUser32
```

Compiling on macOS:
``` bash
gcc snake_console.c -framework Carbon
```

For additional information and to change settings
``` bash
./snake --help
```

## cstring.h
C++17 string interface in C

## array.h
C++17 compliant std::array implementation

## stack.h
C++17 compliant std::stack implementation

## sum.cpp
Command line calculator, has `exprtk.hpp` from this repo as a dependency

## content-size.cpp
List the size of content in a folder, usefull to find the culprit of low storage. Should be compiled with Clang

## data.cpp
Overwrite the free disk space on your drive, usefull to permanently delete "deleted" data

## sizeof/sizeof.c
The size of data types for the current architecture and operating system

## Input/Input.h
Linux implementation to check for keyboard inputs, see example in `Input/main.cpp`

## BrainfuckCCompiler.c
Compile Brainfuck to C

## BrainfuckInterpreter.h
Interpret Brainfuck

## InputHandler.h
Little utility for easier usage of command line arguments

## nks.py
PI approximations shown as a graph

## autoclicker.cpp
Autoclicker for Windows

## hangman.cpp
Hangman in the console, requires `words.txt` from this repo

## ohmslaw.nim
GUI program to calculate very simple electric related stuff

## red_triangle.cpp
Display a red triangle using OpenGL, has [GLFW](https://www.glfw.org/) and [glew](https://glew.sourceforge.net/) as dependencies

## Convert2Binary.cpp
Old and outdated version of [FileAsCode](https://github.com/cunterrainer/FileAsCode)

## mousetracker.cpp
Use [MouseTracker](https://github.com/cunterrainer/MouseTracker) instead
