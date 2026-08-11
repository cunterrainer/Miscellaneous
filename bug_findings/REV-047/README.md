# REV-047 reproduction

```sh
gcc -std=c11 -Wall -Wextra -Wformat=2 -fsyntax-only sizeof/sizeof.c
```

The compiler reports repeated format/type mismatches: `sizeof` has type `size_t`, but the program passes it to `%lld`, `%llu`, or `%d` depending on platform macros.
