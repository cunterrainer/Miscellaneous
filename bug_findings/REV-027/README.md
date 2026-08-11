# REV-027 reproduction

Run from the repository root:

```sh
sh bug_findings/REV-027/reproduce.sh
```

The script confines all writes to a new temporary directory. It places the destination under the source; the backup then sees the destination copy while enumerating the source and deletes the original source file during destination mirroring.
