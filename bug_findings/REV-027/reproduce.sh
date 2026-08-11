#!/bin/sh
set -eu
work="$(mktemp -d)"
mkdir -p "$work/source"
printf 'must survive\n' > "$work/source/original.txt"
g++ -std=c++17 -O2 backup.cpp -o "$work/backup"
"$work/backup" -s "$work/source" -d "$work/source/nested-destination" -nw -n
if test -e "$work/source/original.txt"; then
    echo "source survived (unexpected)"
else
    echo "source was deleted"
fi
echo "temporary reproduction retained at $work"
