# REV-010 reproduction

```sh
g++ -std=c++17 -I. -fsyntax-only bug_findings/REV-010/main.cpp
```

This is one minimal instantiation of an advertised vector operation that the compiler rejects. Other affected APIs and their lines are listed in `report.md`.
