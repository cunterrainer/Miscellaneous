# REV-014 reproduction

```sh
g++ -std=c++17 -x c++ -DNDEBUG -fsanitize=address -g -I. bug_findings/REV-014/main.c -o /tmp/rev-014
/tmp/rev-014
```

With assertion-only validation compiled out, `hash_sha512t_hexdigest` writes its terminator at index 1024 of a 513-byte static buffer.
