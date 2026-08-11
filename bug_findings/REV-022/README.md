# REV-022 reproduction

```sh
gcc -std=c11 -fsanitize=address,undefined -g BrainfuckInterpreter.c -o /tmp/rev-022
/tmp/rev-022 bug_findings/REV-022/program.bf
```

The unsigned 16-bit tape index wraps from zero to 65535 and accesses outside the 30,000-byte tape. AddressSanitizer reports the invalid access.
