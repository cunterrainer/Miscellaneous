# REV-036 reproduction notes

The POSIX build failure is directly visible with:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -fsyntax-only snake_console.c
```

It lacks the feature-test macro needed for `clock_gettime`, `popen`, and `pclose`. On Windows, inspection of the `KEY_W`, `KEY_A`, `KEY_S`, and `KEY_D` definitions shows that boolean `||` is evaluated inside `KEY_PRESSED`, so `GetAsyncKeyState` receives 0 or 1 rather than a virtual-key code.
