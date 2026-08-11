# REV-009 reproduction

```sh
g++ -std=c++17 -I. bug_findings/REV-009/main.cpp -o /tmp/rev-009
/tmp/rev-009
```

The program prints `1`: equality compares only the left range and never compares logical sizes.
