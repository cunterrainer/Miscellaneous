#!/usr/bin/env bash

set -u
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

coverage_failures=0

record_failure()
{
    echo "[FAIL] $*" >&2
    ((coverage_failures += 1))
}

gate_report()
{
    local executable=$1
    local profile=$2
    local header=$3
    local report

    if ! report=$("$coverage_cov" report \
        "$executable" \
        -instr-profile="$profile" \
        --show-region-summary=false \
        --show-branch-summary \
        --show-mcdc-summary \
        --sources "$header"); then
        return 1
    fi
    printf '%s\n' "$report"
    printf '%s\n' "$report" | awk '
        $1 == "TOTAL" {
            found = 1
            if ($11 == 0)
                print "MC/DC: not applicable (no formal conditions)"
            if ($3 != 0 || $6 != 0 || $9 != 0 || $12 != 0)
                failed = 1
        }
        END {
            if (!found || failed)
                exit 1
        }
    '
}

header_for_suite()
{
    case "$1" in
        array_test) printf '%s\n' array.h ;;
        stack_test) printf '%s\n' stack.h ;;
        string_test) printf '%s\n' String.h ;;
        vector_test) printf '%s\n' Vector.h ;;
        stack_vector_test) printf '%s\n' stack_vector.h ;;
        result_test) printf '%s\n' Result.h ;;
    esac
}

for source in "${coverage_sources[@]}"; do
    relative_source=${source#"$test_dir"/}
    suite=${relative_source%.cpp}
    suite=${suite//\//_}
    suite_file=$(basename "$source" .cpp)
    executable="$coverage_work/$suite"
    raw_profile="$coverage_work/$suite.profraw"
    indexed_profile="$coverage_work/$suite.profdata"
    run_log="$coverage_work/$suite.run.log"
    link_flags=()
    if [[ $suite_file == result_test ]]; then
        link_flags=(-Wl,--wrap=snprintf)
    fi

    echo "[coverage build] $suite"
    if ! "$coverage_cxx" \
        "${coverage_flags[@]}" \
        "$source" \
        "${link_flags[@]}" \
        -o "$executable"; then
        record_failure "coverage build failed for $suite"
        continue
    fi

    echo "[coverage run]   $suite"
    LLVM_PROFILE_FILE="$raw_profile" "$executable" >"$run_log" 2>&1
    run_status=$?
    cat "$run_log"
    if [[ $run_status -ne 0 ]]; then
        record_failure "coverage test failed for $suite (exit $run_status)"
    fi
    if [[ ! -f $raw_profile ]]; then
        record_failure "coverage profile was not produced for $suite"
        continue
    fi
    if ! "$coverage_profdata" merge -sparse "$raw_profile" -o "$indexed_profile"; then
        record_failure "coverage profile merge failed for $suite"
        continue
    fi

    header=$(header_for_suite "$suite_file")
    if [[ -n $header ]]; then
        echo "[coverage gate] $header"
        if ! gate_report "$executable" "$indexed_profile" "$repo_root/$header"; then
            record_failure "$header did not reach zero missed functions, lines, branches, and MC/DC conditions"
        fi
    fi
done

if [[ ! -x $test_dir/core/core_matrix.sh ]]; then
    record_failure "required Core.h scenario matrix is missing or not executable: $test_dir/core/core_matrix.sh"
else
    echo '[Core matrix] native and synthetic preprocessor scenarios'
    if ! "$test_dir/core/core_matrix.sh"; then
        record_failure 'Core.h scenario matrix failed'
    fi
fi

if ((coverage_failures != 0)); then
    echo "Coverage/Core matrix completed with $coverage_failures failure(s)." >&2
    exit 1
fi

echo 'Coverage thresholds and Core.h scenario matrix passed.'
