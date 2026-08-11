# REV-012 reproduction

```sh
gcc -std=c11 -I. -fsyntax-only bug_findings/REV-012/main.c
```

Despite the enabled C interface, C compilation fails because the header uses C++ namespace-qualified library names in the C section.
