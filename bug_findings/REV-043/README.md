# REV-043 reproduction notes

`data.cpp` is Windows-specific. The failure does not require allocating disk space: inspect the calculations after `std::thread::hardware_concurrency()`. If it returns 1, `threadsToSpawn` becomes zero and both division and remainder use zero. If it returns 0 (explicitly permitted by the standard), unsigned subtraction produces a huge thread count.

Do not execute this utility on a real filesystem merely to reproduce the issue: its stated purpose is filling available disk space with files.
