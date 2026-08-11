# REV-048 reproduction

```sh
python3 nks.py
```

The program terminates during import because `pi_million.py` (or an installed `pi_million` module) is absent from the repository and declared build/runtime dependencies.
