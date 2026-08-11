# REV-004 reproduction

```sh
gcc -std=c11 -fsanitize=address,undefined -g -I. bug_findings/REV-004/main.c -o /tmp/rev-004
/tmp/rev-004
```

`string_clear` writes the terminator through the null `data` pointer created for an empty string. AddressSanitizer reports a null-pointer store.
