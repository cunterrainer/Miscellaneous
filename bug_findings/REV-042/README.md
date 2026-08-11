# REV-042 reproduction

```sh
g++ -std=c++17 -I. -fsyntax-only bug_findings/REV-042/main.cpp
```

On LP64 systems, the library's `std::uint64_t` write callback has type `unsigned long`, which does not match miniz's `unsigned long long` callback signature. Compilation fails at the callback registrations.
