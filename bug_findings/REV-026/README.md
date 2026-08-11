# REV-026 reproduction

```sh
g++ -std=c++17 -I. -fsyntax-only Convert2Binary.cpp
```

On a conforming non-MSVC toolchain the `sprintf_s` call is undeclared. Independently, code inspection at the final partial-line loop shows that it increments the input index to the configured line width without checking the actual input length.
