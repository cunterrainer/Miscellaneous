# REV-006 reproduction

```sh
g++ -std=c++17 -fsanitize=address,undefined -g -I. bug_findings/REV-006/main.cpp -o /tmp/rev-006
/tmp/rev-006
```

The formatting constructor writes bytes after `reserve()` without resizing the string. The visible buffer may contain `number 42`, while the string's logical size remains zero.
