# REV-008 reproduction

```sh
g++ -std=c++17 -I. -fsyntax-only bug_findings/REV-008/main.cpp
```

Instantiation exposes the misspelled `array::_Elems` member and the free `stack::swap` attempting to mutate const operands.
