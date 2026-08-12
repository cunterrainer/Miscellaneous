#!/usr/bin/env bash

set -eu
set -o pipefail

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$test_dir/.." && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/misc-tests.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT HUP INT TERM

if [[ -n ${CXX:-} ]]; then
    test_cxx=$CXX
elif command -v clang++-20 >/dev/null 2>&1; then
    test_cxx=clang++-20
elif command -v clang++ >/dev/null 2>&1; then
    test_cxx=clang++
else
    test_cxx=g++
fi

if ! command -v "$test_cxx" >/dev/null 2>&1; then
    echo "error: C++ compiler not found: $test_cxx" >&2
    exit 2
fi

mapfile -d '' -t test_sources < <(
    find "$test_dir" -type f -name '*_test.cpp' -print0 | sort -z
)
if ((${#test_sources[@]} == 0)); then
    echo "error: no *_test.cpp suites found under $test_dir" >&2
    exit 2
fi

common_flags=(
    -std=c++17
    -I"$repo_root"
    -pthread
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

link_flags_for()
{
    local source=$1
    if [[ $(basename "$source") == result_test.cpp ]]; then
        printf '%s\n' -Wl,--wrap=snprintf
    fi
}

run_configuration()
{
    local name=$1
    local flag_string=$2
    local environment_string=${3:-}
    local -a configuration_flags=()
    local -a configuration_environment=()
    local source relative_source suite executable

    read -r -a configuration_flags <<<"$flag_string"
    if [[ -n $environment_string ]]; then
        read -r -a configuration_environment <<<"$environment_string"
    fi

    echo "== $name ($test_cxx) =="
    for source in "${test_sources[@]}"; do
        relative_source=${source#"$test_dir"/}
        suite=${relative_source%.cpp}
        suite=${suite//\//_}
        executable="$work_dir/${name//[^[:alnum:]_]/_}-$suite"
        local -a suite_link_flags=()
        mapfile -t suite_link_flags < <(link_flags_for "$source")

        echo "[build] $suite"
        if ! "$test_cxx" \
            "${common_flags[@]}" \
            "${configuration_flags[@]}" \
            "$source" \
            "${suite_link_flags[@]}" \
            -o "$executable"; then
            echo "error: $name build failed for $suite" >&2
            return 1
        fi

        echo "[run]   $suite"
        if ! env "${configuration_environment[@]}" "$executable"; then
            echo "error: $name test failed for $suite" >&2
            return 1
        fi
    done
}

probe_sanitizer()
{
    local name=$1
    local flag_string=$2
    local environment_string=${3:-}
    local -a sanitizer_flags=()
    local -a sanitizer_environment=()
    local executable="$work_dir/probe-$name"
    local compile_log="$work_dir/probe-$name.compile.log"
    local runtime_log="$work_dir/probe-$name.runtime.log"
    local status

    read -r -a sanitizer_flags <<<"$flag_string"
    if [[ -n $environment_string ]]; then
        read -r -a sanitizer_environment <<<"$environment_string"
    fi

    if ! printf '%s\n' \
        '#include "Test.h"' \
        '#include <thread>' \
        'TEST(sanitizer_runtime_probe) { int value = 0; std::thread worker([&value] { value = 1; }); worker.join(); CHECK_EQUAL(value, 1); }' \
        'int main(int argc, char** argv) { return Test::RunAllTests(argc, argv); }' |
        "$test_cxx" -x c++ -std=c++17 -I"$repo_root" -pthread \
            "${sanitizer_flags[@]}" -o "$executable" - \
            >"$compile_log" 2>&1; then
        echo "[skip] $name: compile probe failed: $(tr '\n' ' ' <"$compile_log" | sed 's/[[:space:]]\+/ /g; s/ $//')"
        return 1
    fi

    if env "${sanitizer_environment[@]}" "$executable" >"$runtime_log" 2>&1; then
        :
    else
        status=$?
        echo "[skip] $name: runtime probe exited $status: $(tr '\n' ' ' <"$runtime_log" | sed 's/[[:space:]]\+/ /g; s/ $//')"
        return 1
    fi

    return 0
}

run_configuration debug '-O0 -g'
run_configuration release '-O3 -DNDEBUG'

asan_environment='ASAN_OPTIONS=halt_on_error=1'
if [[ -r /proc/version ]] && grep -qi microsoft /proc/version; then
    asan_environment='ASAN_OPTIONS=halt_on_error=1:detect_leaks=0'
    echo "note: ASan leak detection disabled on WSL; standalone LSan is probed separately"
fi

sanitizer_names=(address undefined thread memory leak)
sanitizer_flags=(
    '-O1 -g -fno-omit-frame-pointer -fsanitize=address'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=undefined'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=thread'
    '-O1 -g -fno-omit-frame-pointer -fPIE -pie -fsanitize=memory'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=leak'
)
sanitizer_environments=(
    "$asan_environment"
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'TSAN_OPTIONS=halt_on_error=1'
    'MSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'LSAN_OPTIONS=exitcode=23'
)

for index in "${!sanitizer_names[@]}"; do
    name=${sanitizer_names[$index]}
    flags=${sanitizer_flags[$index]}
    configuration_environment=${sanitizer_environments[$index]}
    if probe_sanitizer "$name" "$flags" "$configuration_environment"; then
        run_configuration "$name-sanitizer" "$flags" "$configuration_environment"
    fi
done

echo '== coverage and Core preprocessor matrix =='
"$test_dir/coverage.sh"

echo 'All supported configurations passed.'
