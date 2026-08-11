# REV-024 reproduction

```sh
g++ -std=c++17 -fsanitize=address,undefined -g -Iimf AssetFinder.cpp -o /tmp/rev-024
/tmp/rev-024 bug_findings/REV-024/short.bin
```

For an input smaller than 24 bytes, `buffer.size() - 24` underflows and the scanner reads beyond the one-byte allocation.
