#!/bin/sh

set -eu

harness_test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
harness_root=$(CDPATH= cd -- "$harness_test_dir/.." && pwd)
harness_work=$(mktemp -d)
trap 'rm -rf -- "$harness_work"' EXIT HUP INT TERM

harness_compiler=${TEST_HARNESS_CXX:-clang++}
if ! command -v "$harness_compiler" >/dev/null 2>&1; then
    echo "error: Clang compiler not found: $harness_compiler" >&2
    exit 2
fi

harness_profdata=$("$harness_compiler" -print-prog-name=llvm-profdata)
harness_cov=$("$harness_compiler" -print-prog-name=llvm-cov)

if [ ! -x "$harness_profdata" ] || [ ! -x "$harness_cov" ]; then
    echo "error: matching llvm-profdata and llvm-cov tools are required" >&2
    exit 2
fi

"$harness_compiler" \
    -std=c++17 \
    -O0 \
    -fprofile-instr-generate \
    -fcoverage-mapping \
    -pthread \
    -Wall \
    -Wextra \
    -pedantic \
    -I"$harness_root" \
    "$harness_test_dir/test_harness_test.cpp" \
    -o "$harness_work/test_harness_test"

LLVM_PROFILE_FILE="$harness_work/test_harness.profraw" \
    "$harness_work/test_harness_test"

"$harness_profdata" merge \
    -sparse \
    "$harness_work/test_harness.profraw" \
    -o "$harness_work/test_harness.profdata"

harness_report=$("$harness_cov" report \
    "$harness_work/test_harness_test" \
    -instr-profile="$harness_work/test_harness.profdata" \
    "$harness_root/Test.h" \
    --show-region-summary=false \
    --show-branch-summary)

printf '%s\n' "$harness_report"

printf '%s\n' "$harness_report" | awk '
    $1 == "TOTAL" {
        found_total = 1
        if ($3 != 0 || $6 != 0 || $9 != 0) {
            exit 1
        }
    }
    END {
        if (!found_total) {
            exit 1
        }
    }
'
