#!/usr/bin/env bash
set -euo pipefail

# Core.h is a preprocessor-only facility.  This script is its coverage oracle:
# every named scenario is compiled as a separate translation unit by both host
# compilers, so native predefined macros cannot leak between scenarios.

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
core_test_dir="${root_dir}/test/core"
case_source="${core_test_dir}/core_matrix_case.cpp"

compilers=()
for candidate in "${CXX_GCC:-g++}" "${CXX_CLANG:-clang++}"; do
    if command -v "${candidate}" >/dev/null 2>&1; then
        compilers+=("${candidate}")
    else
        printf 'Core matrix requires %s\n' "${candidate}" >&2
        exit 1
    fi
done

all_disabled=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=0
)
arch_only=(
    -DCORE_ENABLE_ARCHITECTURE=1 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=1
)
arch_platform=(
    -DCORE_ENABLE_ARCHITECTURE=1 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=1 -DCORE_ENABLE_OTHER=1
)
arch_hardware=(
    -DCORE_ENABLE_ARCHITECTURE=1 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=1 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=1
)
compiler_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=1
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=0
)
language_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=1
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=0
)
library_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=1 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=0
)
platform_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=1 -DCORE_ENABLE_OTHER=0
)
other_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=0 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=1
)
hardware_only=(
    -DCORE_ENABLE_ARCHITECTURE=0 -DCORE_ENABLE_COMPILER=0
    -DCORE_ENABLE_HARDWARE=1 -DCORE_ENABLE_LANGUAGE=0
    -DCORE_ENABLE_LIBRARY=0 -DCORE_ENABLE_PLATFORM=0 -DCORE_ENABLE_OTHER=0
)
arch_neutral=(
    -U__x86_64 -U__x86_64__ -U__amd64 -U__amd64__ -U_M_X64
    -U__i386 -U__i386__ -U__i486__ -U__i586__ -U__i686__
)
os_neutral=(
    -Ulinux -U__linux -U__linux__ -U__gnu_linux__
    -Uunix -U__unix -U__unix__ -U_POSIX_SOURCE -U_XOPEN_SOURCE
)
endian_neutral=(
    -U__BYTE_ORDER__ -U__ORDER_BIG_ENDIAN__ -U__ORDER_LITTLE_ENDIAN__
    -U__ORDER_PDP_ENDIAN__ -U__BYTE_ORDER -U_BYTE_ORDER
    -U__BIG_ENDIAN -U__BIG_ENDIAN__ -U_BIG_ENDIAN
    -U__LITTLE_ENDIAN -U__LITTLE_ENDIAN__ -U_LITTLE_ENDIAN
    -U__PDP_ENDIAN -U_PDP_ENDIAN
)
simd_neutral=(
    -U__MIC__ -U__AVX2__ -U__AVX__ -U__FMA__ -U__SSE4_2__
    -U__SSE4_1__ -U__SSSE3__ -U__SSE3__ -U__SSE2__ -U__SSE__ -U__MMX__
    -U__XOP__ -U__FMA4__ -U__SSE4A__
)
compiler_neutral=(
    -U__clang__ -U__clang_major__ -U__clang_minor__ -U__clang_patchlevel__
    -U__GNUC__ -U__GNUC_MINOR__ -U__GNUC_PATCHLEVEL__
)

run_case()
{
    local compiler="$1"
    local number="$2"
    local name="$3"
    shift 3
    "${compiler}" -std=c++17 -fsyntax-only -Wall -Wextra -Wpedantic -Werror \
        -I"${root_dir}" -DCORE_MATRIX_SCENARIO="${number}" "$@" "${case_source}"
    printf '[PASS] %-8s %s\n' "$(basename "${compiler}")" "${name}"
}

for compiler in "${compilers[@]}"; do
    run_case "${compiler}" 1  feature-gates-all-disabled "${all_disabled[@]}"
    run_case "${compiler}" 2  arch-wasm32-web "${arch_platform[@]}" \
        "${arch_neutral[@]}" "${os_neutral[@]}" -D__wasm32__=1
    run_case "${compiler}" 3  arch-arm64-neon "${arch_hardware[@]}" \
        "${arch_neutral[@]}" "${simd_neutral[@]}" -D__aarch64__=1
    run_case "${compiler}" 4  arch-riscv64-endian "${arch_only[@]}" "${arch_neutral[@]}" \
        "${endian_neutral[@]}" -D__riscv=1 -D__riscv_xlen=64
    run_case "${compiler}" 5  arch-powerpc64-precedence "${arch_only[@]}" "${arch_neutral[@]}" \
        -D__powerpc64__=1
    run_case "${compiler}" 6  arch-superh16 "${arch_only[@]}" "${arch_neutral[@]}" \
        -D__sh__=1 -D__sh2__=1
    run_case "${compiler}" 7  arch-loongarch64 "${arch_only[@]}" "${arch_neutral[@]}" \
        "${endian_neutral[@]}" -D__loongarch__=1 -D__loongarch_grlen=64

    run_case "${compiler}" 8  compiler-clang-over-gcc "${compiler_only[@]}" "${compiler_neutral[@]}" \
        -D__clang__=1 -D__clang_major__=17 -D__clang_minor__=2 -D__clang_patchlevel__=1 \
        -D__GNUC__=13 -D__GNUC_MINOR__=2 -D__GNUC_PATCHLEVEL__=1
    run_case "${compiler}" 9  compiler-emscripten-precedence "${compiler_only[@]}" "${compiler_neutral[@]}" \
        -D__EMSCRIPTEN__=1 -D__clang__=1 -D__clang_major__=18 -D__clang_minor__=1 \
        -D__clang_patchlevel__=2 -D__GNUC__=12 -D__GNUC_MINOR__=3 -D__GNUC_PATCHLEVEL__=0
    run_case "${compiler}" 10 compiler-compaq-name "${compiler_only[@]}" "${compiler_neutral[@]}" \
        -D__DECCXX=1 -D__DECCXX_VER=70301000
    run_case "${compiler}" 11 compiler-diab-name "${compiler_only[@]}" "${compiler_neutral[@]}" \
        -D__DCC__=1 -D__VERSION_NUMBER__=4567

    run_case "${compiler}" 12 language-cpp98 "${language_only[@]}" -U__cplusplus -D__cplusplus=199711L
    run_case "${compiler}" 13 language-cpp-cli-precedence "${language_only[@]}" \
        -D__cplusplus_cli=201103L
    run_case "${compiler}" 14 language-embedded-precedence "${language_only[@]}" \
        -D__embedded_cplusplus=1

    run_case "${compiler}" 15 library-native-gnu "${library_only[@]}"
    run_case "${compiler}" 16 library-overrides "${library_only[@]}" \
        -D_STLPORT_VERSION=0x521 -D_STLPORT_MAJOR=5 -D_STLPORT_MINOR=2 \
        -D_STLPORT_PATCHLEVEL=1 -D__CRTL_VER=80320000

    run_case "${compiler}" 17 os-android-over-linux-unix "${platform_only[@]}" "${os_neutral[@]}" \
        -D__ANDROID__=1 -D__linux__=1 -D__unix=1
    run_case "${compiler}" 18 os-linux-over-unix "${platform_only[@]}" "${os_neutral[@]}" \
        -D__linux__=1 -D__unix=1
    run_case "${compiler}" 19 os-freebsd-over-generic-bsd "${platform_only[@]}" "${os_neutral[@]}" \
        -I"${core_test_dir}/stubs/freebsd" -D__FreeBSD__=1 -DBSD=199506
    run_case "${compiler}" 20 os-openbsd-over-generic-bsd "${platform_only[@]}" "${os_neutral[@]}" \
        -D__OpenBSD__=1 -DOpenBSD6_9=1 -DBSD=199506
    run_case "${compiler}" 21 os-macos-over-unix "${platform_only[@]}" "${os_neutral[@]}" \
        -D__APPLE__=1 -D__MACH__=1 -D__unix=1
    run_case "${compiler}" 22 os-generic-unix-fallback "${platform_only[@]}" "${os_neutral[@]}" -D__unix=1
    run_case "${compiler}" 23 platform-mingw32-version "${platform_only[@]}" "${os_neutral[@]}" \
        -I"${core_test_dir}/stubs/mingw" -D__MINGW32__=1
    run_case "${compiler}" 24 platform-windows-phone-runtime "${platform_only[@]}" "${os_neutral[@]}" \
        -I"${core_test_dir}/stubs/windows" -D_WIN32=1 -DWINAPI_FAMILY=2 \
        -DWINAPI_FAMILY_PHONE_APP=2
    run_case "${compiler}" 25 platform-windows-server-name "${platform_only[@]}" "${os_neutral[@]}" \
        -I"${core_test_dir}/stubs/windows" -D_WIN32=1 -DWINAPI_FAMILY=5 \
        -DWINAPI_FAMILY_SERVER=5

    run_case "${compiler}" 26 endian-standard-big "${other_only[@]}" "${endian_neutral[@]}" \
        -D__BYTE_ORDER__=4321 -D__ORDER_BIG_ENDIAN__=4321 -D__ORDER_LITTLE_ENDIAN__=1234
    run_case "${compiler}" 27 endian-standard-little "${other_only[@]}" "${endian_neutral[@]}" \
        -D__BYTE_ORDER__=1234 -D__ORDER_BIG_ENDIAN__=4321 -D__ORDER_LITTLE_ENDIAN__=1234
    run_case "${compiler}" 28 endian-standard-pdp "${other_only[@]}" "${endian_neutral[@]}" \
        -D__BYTE_ORDER__=3412 -D__ORDER_BIG_ENDIAN__=4321 -D__ORDER_LITTLE_ENDIAN__=1234 \
        -D__ORDER_PDP_ENDIAN__=3412
    run_case "${compiler}" 29 endian-architecture-big-fallback "${arch_only[@]}" "${arch_neutral[@]}" \
        "${endian_neutral[@]}" -D__m68k__=1
    run_case "${compiler}" 30 endian-x64-fallback "${arch_only[@]}" "${arch_neutral[@]}" \
        "${endian_neutral[@]}" -D__x86_64__=1

    run_case "${compiler}" 31 simd-x86-amd-max "${hardware_only[@]}" "${simd_neutral[@]}" \
        -D__AVX2__=1 -D__SSE4A__=1
    run_case "${compiler}" 32 simd-ppc-precedence "${hardware_only[@]}" "${simd_neutral[@]}" \
        -D__VECTOR4DOUBLE__=1
    run_case "${compiler}" 33 platform-disable-forces-other-default-off \
        -DCORE_ENABLE_ARCHITECTURE=1 -DCORE_ENABLE_COMPILER=1 -DCORE_ENABLE_HARDWARE=1 \
        -DCORE_ENABLE_LANGUAGE=1 -DCORE_ENABLE_LIBRARY=1 -DCORE_ENABLE_PLATFORM=0
done

printf 'Core matrix: %d scenarios x %d compilers passed\n' 33 "${#compilers[@]}"
