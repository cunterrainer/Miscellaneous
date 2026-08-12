#!/usr/bin/env bash

set -eu
set -o pipefail

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$test_dir/.." && pwd)
coverage_work=$(mktemp -d "${TMPDIR:-/tmp}/misc-coverage.XXXXXX")
trap 'rm -rf -- "$coverage_work"' EXIT HUP INT TERM

coverage_cxx=${COVERAGE_CXX:-clang++-20}
coverage_profdata=${LLVM_PROFDATA:-llvm-profdata-20}
coverage_cov=${LLVM_COV:-llvm-cov-20}

for tool in "$coverage_cxx" "$coverage_profdata" "$coverage_cov"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: required Clang 20 coverage tool not found: $tool" >&2
        exit 2
    fi
done

if ! "$coverage_cxx" --version | head -n 1 | grep -Eq 'clang version 20([.]| )'; then
    echo "error: coverage requires Clang 20; $coverage_cxx reports:" >&2
    "$coverage_cxx" --version | head -n 1 >&2
    exit 2
fi

mapfile -d '' -t coverage_sources < <(
    find "$test_dir" -type f -name '*_test.cpp' -print0 | sort -z
)
if ((${#coverage_sources[@]} == 0)); then
    echo "error: no *_test.cpp suites found under $test_dir" >&2
    exit 2
fi

coverage_flags=(
    -std=c++17
    -O0
    -g
    -DNDEBUG
    -I"$repo_root"
    -pthread
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -fprofile-instr-generate
    -fcoverage-mapping
    -fcoverage-mcdc
)

gate_report()
{
    local executable=$1
    local profile=$2
    local header=$3
    local report

    report=$("$coverage_cov" report \
        "$executable" \
        -instr-profile="$profile" \
        --show-region-summary=false \
        --show-branch-summary \
        --show-mcdc-summary \
        --sources "$header") || return 1
    printf '%s\n' "$report"
    printf '%s\n' "$report" | awk '
        $1 == "TOTAL" {
            found = 1
            if ($3 != 0 || $6 != 0 || $9 != 0 || $12 != 0)
                exit 1
        }
        END {
            if (!found)
                exit 1
        }
    '
}

for source in "${coverage_sources[@]}"; do
    relative_source=${source#"$test_dir"/}
    suite=${relative_source%.cpp}
    suite=${suite//\//_}
    suite_file=$(basename "$source" .cpp)
    executable="$coverage_work/$suite"
    raw_profile="$coverage_work/$suite.profraw"
    indexed_profile="$coverage_work/$suite.profdata"
    link_flags=()
    if [[ $suite_file == result_test ]]; then
        link_flags=(-Wl,--wrap=snprintf)
    fi

    echo "[coverage build] $suite"
    "$coverage_cxx" \
        "${coverage_flags[@]}" \
        "$source" \
        "${link_flags[@]}" \
        -o "$executable"
    echo "[coverage run]   $suite"
    LLVM_PROFILE_FILE="$raw_profile" "$executable"
    "$coverage_profdata" merge -sparse "$raw_profile" -o "$indexed_profile"

    case "$suite_file" in
        stack_vector_test)
            echo '[coverage gate] stack_vector.h'
            gate_report "$executable" "$indexed_profile" "$repo_root/stack_vector.h"
            ;;
        result_test)
            echo '[coverage gate] Result.h'
            gate_report "$executable" "$indexed_profile" "$repo_root/Result.h"
            ;;
    esac
done

if [[ ! -x $test_dir/core/core_matrix.sh ]]; then
    echo "error: required Core.h scenario matrix is missing or not executable: $test_dir/core/core_matrix.sh" >&2
    exit 2
fi

echo '[Core matrix] native and synthetic preprocessor scenarios'
"$test_dir/core/core_matrix.sh"

echo 'Coverage thresholds and Core.h scenario matrix passed.'
