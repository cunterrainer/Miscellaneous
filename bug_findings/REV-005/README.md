# REV-005 reproduction

```sh
gcc -std=c11 -I. bug_findings/REV-005/main.c -o /tmp/rev-005
/tmp/rev-005
```

The first result incorrectly reports a match at zero. The copy request asks for two characters at position three, but returns/copies five characters (`Xdef` plus the terminator).
