# REV-013 reproduction

Compile as C++ because REV-012 independently prevents C compilation:

```sh
g++ -std=c++17 -x c++ -fsanitize=address -g -I. bug_findings/REV-013/main.c -o /tmp/rev-013
/tmp/rev-013
```

Supplying a caller-owned 201-byte output buffer causes Keccak to write 100 binary bytes into the fixed 32-byte local `intBuff`. AddressSanitizer reports a stack-buffer overflow.
