# REV-016 reproduction

```sh
g++ -std=c++20 -O2 hash.cpp -o /tmp/rev-016
/tmp/rev-016 --no-decorator --text abc
/tmp/rev-016 --sha256 --search e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 .
```

The documented long `--no-decorator` option is treated as input because the parser literal contains a leading space. The valid 64-character SHA-256 search digest is rejected because it is compared with a 32-byte digest size.
