# REV-002 reproduction

From the repository root:

```sh
g++ -std=c++17 -I. -fsyntax-only bug_findings/REV-002/main.cpp
```

The compiler rejects `String.h` at the dependent `allocator_traits` names before the example can be instantiated.
