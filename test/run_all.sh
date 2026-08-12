#!/usr/bin/env bash

set -u
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

matrix_failures=0

record_failure()
{
    echo "[FAIL] $*" >&2
    ((matrix_failures += 1))
}

link_flags_for()
{
    local source=$1
    if [[ $(basename "$source") == result_test.cpp ]]; then
        printf '%s\n' -Wl,--wrap=snprintf
    fi
}

verify_test_count()
{
    local executable=$1
    local run_log=$2
    local suite=$3
    local configuration=$4
    local list_log="$work_dir/list-${configuration//[^[:alnum:]_]/_}-${suite//[^[:alnum:]_]/_}.log"
    local listed executed

    if ! "$executable" --list-tests >"$list_log" 2>&1; then
        cat "$list_log"
        record_failure "$configuration --list-tests failed for $suite"
        return
    fi
    listed=$(wc -l <"$list_log")
    executed=$(sed -n 's/^Tests: \([0-9][0-9]*\),.*/\1/p' "$run_log" | tail -n 1)
    if [[ -z $executed ]]; then
        record_failure "$configuration did not report an executed test count for $suite"
    elif [[ $listed -ne $executed ]]; then
        record_failure "$configuration count mismatch for $suite: --list-tests=$listed, executed=$executed"
    else
        echo "[count] $suite: listed=$listed, executed=$executed"
    fi
}

run_configuration()
{
    local name=$1
    local flag_string=$2
    local environment_string=${3:-}
    local verify_counts=${4:-false}
    local -a configuration_flags=()
    local -a configuration_environment=()
    local source relative_source suite executable run_log status

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
        run_log="$work_dir/${name//[^[:alnum:]_]/_}-$suite.run.log"
        local -a suite_link_flags=()
        mapfile -t suite_link_flags < <(link_flags_for "$source")

        echo "[build] $suite"
        if ! "$test_cxx" \
            "${common_flags[@]}" \
            "${configuration_flags[@]}" \
            "$source" \
            "${suite_link_flags[@]}" \
            -o "$executable"; then
            record_failure "$name build failed for $suite"
            continue
        fi

        echo "[run]   $suite"
        env "${configuration_environment[@]}" "$executable" >"$run_log" 2>&1
        status=$?
        cat "$run_log"
        if [[ $status -ne 0 ]]; then
            record_failure "$name test failed for $suite (exit $status)"
        fi
        if [[ $verify_counts == true ]]; then
            verify_test_count "$executable" "$run_log" "$suite" "$name"
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
    local status reason

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
        reason=$(tr '\n' ' ' <"$compile_log" | sed 's/[[:space:]]\+/ /g; s/ $//')
        echo "[skip] $name: compile probe failed: $reason"
        return 1
    fi

    env "${sanitizer_environment[@]}" "$executable" >"$runtime_log" 2>&1
    status=$?
    if [[ $status -ne 0 ]]; then
        reason=$(tr '\n' ' ' <"$runtime_log" | sed 's/[[:space:]]\+/ /g; s/ $//')
        echo "[skip] $name: runtime probe exited $status: $reason"
        return 1
    fi

    return 0
}

run_ub_probe()
{
    local sanitizer_name=$1
    local flag_string=$2
    local environment_string=$3
    local probe_name=$4
    local diagnostic_pattern=$5
    local source="$test_dir/ub/$probe_name.cpp"
    local executable="$work_dir/ub-$sanitizer_name-$probe_name"
    local compile_log="$executable.compile.log"
    local runtime_log="$executable.runtime.log"
    local -a probe_flags=()
    local -a probe_environment=()
    local status reason

    if [[ ! -f $source ]]; then
        record_failure "missing UB probe source: $source"
        return
    fi
    read -r -a probe_flags <<<"$flag_string"
    if [[ -n $environment_string ]]; then
        read -r -a probe_environment <<<"$environment_string"
    fi

    echo "[UB probe] $probe_name ($sanitizer_name)"
    if ! "$test_cxx" "${common_flags[@]}" "${probe_flags[@]}" "$source" -o "$executable" \
        >"$compile_log" 2>&1; then
        reason=$(tr '\n' ' ' <"$compile_log" | sed 's/[[:space:]]\+/ /g; s/ $//')
        record_failure "$probe_name failed to compile with supported $sanitizer_name: $reason"
        return
    fi

    env "${probe_environment[@]}" "$executable" >"$runtime_log" 2>&1
    status=$?
    if [[ $status -eq 0 ]]; then
        record_failure "$probe_name unexpectedly completed under $sanitizer_name"
    elif ! grep -Eiq "$diagnostic_pattern" "$runtime_log"; then
        reason=$(tr '\n' ' ' <"$runtime_log" | sed 's/[[:space:]]\+/ /g; s/ $//')
        record_failure "$probe_name exited $status without the expected $sanitizer_name diagnostic: $reason"
    else
        echo "[PASS] $probe_name produced a matching diagnostic and exited $status"
    fi
}

probe_optional_ub_detector()
{
    local sanitizer_name=$1
    local flag_string=$2
    local environment_string=$3
    local probe_name=$4
    local diagnostic_pattern=$5
    local source="$test_dir/ub/$probe_name.cpp"
    local executable="$work_dir/optional-ub-$sanitizer_name-$probe_name"
    local runtime_log="$executable.runtime.log"
    local -a probe_flags=()
    local -a probe_environment=()
    local probe_exit

    read -r -a probe_flags <<<"$flag_string"
    if [[ -n $environment_string ]]; then
        read -r -a probe_environment <<<"$environment_string"
    fi
    if ! "$test_cxx" "${common_flags[@]}" "${probe_flags[@]}" "$source" -o "$executable"; then
        echo "[skip] $probe_name ($sanitizer_name): UB detector compile probe failed"
        return
    fi
    env "${probe_environment[@]}" "$executable" >"$runtime_log" 2>&1
    probe_exit=$?
    if [[ $probe_exit -ne 0 ]] && grep -Eiq "$diagnostic_pattern" "$runtime_log"; then
        echo "[PASS] $probe_name produced a matching $sanitizer_name diagnostic and exited $probe_exit"
    elif [[ $probe_exit -eq 0 ]]; then
        echo "[skip] $probe_name ($sanitizer_name): runtime probe completed without a sanitizer diagnostic"
    else
        echo "[skip] $probe_name ($sanitizer_name): runtime probe exited $probe_exit without a sanitizer diagnostic"
    fi
}

run_configuration debug '-O0 -g' '' true
run_configuration release '-O3 -DNDEBUG' '' true

asan_environment='ASAN_OPTIONS=halt_on_error=1:detect_leaks=0'

sanitizer_names=(
    address undefined thread memory leak bounds integer implicit-conversion
    nullability object-size pointer-overflow float-divide-by-zero
)
sanitizer_flags=(
    '-O1 -g -fno-omit-frame-pointer -fsanitize=address'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=undefined'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=thread'
    '-O1 -g -fno-omit-frame-pointer -fPIE -pie -fsanitize=memory'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=leak'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=bounds'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=integer'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=implicit-conversion'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=nullability'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=object-size'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=pointer-overflow'
    '-O1 -g -fno-omit-frame-pointer -fsanitize=float-divide-by-zero'
)
sanitizer_environments=(
    "$asan_environment"
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'TSAN_OPTIONS=halt_on_error=1'
    'MSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'LSAN_OPTIONS=exitcode=23'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
    'UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1'
)

for index in "${!sanitizer_names[@]}"; do
    name=${sanitizer_names[$index]}
    flags=${sanitizer_flags[$index]}
    configuration_environment=${sanitizer_environments[$index]}
    if probe_sanitizer "$name" "$flags" "$configuration_environment"; then
        run_configuration "$name-sanitizer" "$flags" "$configuration_environment"
        case "$name" in
            address)
                run_ub_probe "$name" "$flags" "$configuration_environment" \
                    array_unchecked_access 'AddressSanitizer|buffer-overflow'
                ;;
            undefined)
                run_ub_probe "$name" "$flags" "$configuration_environment" \
                    array_zero_dereference 'UndefinedBehaviorSanitizer|runtime error:.*null'
                run_ub_probe "$name" "$flags" "$configuration_environment" \
                    stack_empty_top 'UndefinedBehaviorSanitizer|runtime error:'
                probe_optional_ub_detector "$name" "$flags" "$configuration_environment" \
                    stack_empty_pop 'UndefinedBehaviorSanitizer|runtime error:'
                ;;
        esac
    fi
done

echo '== coverage and Core preprocessor matrix =='
if ! "$test_dir/coverage.sh"; then
    record_failure 'coverage or Core preprocessor matrix failed'
fi

if ((matrix_failures != 0)); then
    echo "Matrix completed with $matrix_failures failure(s)." >&2
    exit 1
fi

echo 'All supported configurations passed.'
