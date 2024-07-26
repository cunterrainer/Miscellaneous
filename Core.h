#ifndef CORE_H
#define CORE_H
/*
Documentation:

CORE_LANG_STDCPP  Check if using C++
CORE_MODE_DEBUG   Check if debug build
CORE_MODE_RELEASE Check if release build

CORE_ARCH_[ARCH] Check if on this architecture
    - x86
    - x64
    - arm
    - alpha
    - blackfin
    - convex
    - e2k
    - ia64
    - loongarch
    - m68k
    - mips
    - parisc
    - ppc
    - ptx
    - pyramid
    - riscv
    - rs6k
    - sparc
    - superh
    - sys370
    - sys390
    - z

CORE_COMP_[COMP] Check if using this compiler
    - gcc
    - clang
    - msvc
    - intel
    - borland
    - comeau
    - compaq
    - diab
    - digital mars
    - dignus
    - edg
    - greenhills
    - hp acc
    - iar
    - ibm
    - kai
    - llvm
    - metaware
    - metrowerks
    - microtec
    - mpw
    - nvcc
    - palm
    - pgi
    - sgi mipspro
    - sunpro
    - tendra
    - watcom

CORE_OS_[OS] Check if using this os
    - windows
    - linux
    - macos
    - ios
    - android
    - bds
    - bsdi
    - bds dragonfly
    - bds free
    - bds net
    - bds open
    - aix
    - amigaos
    - beos
    - cygwin
    - haiku
    - hpux
    - irix
    - os400
    - qnxnto
    - solaris
    - unix
    - vms

CORE_PLATFORM_[PLAT] Check if on this platform
    - android
    - cloudabi
    - ios
    - mingw
    - mingw32
    - mingw64
    - windows desktop
    - windows phone
    - windows runtime
    - windows server
    - windows store
    - windows system
    - windows uwp

CORE_LANG_STDCPP_[VER] Check if using C++ version [VER]
    - C++ 98
    - C++ 11
    - C++ 14
    - C++ 17
    - C++ 20

CORE_LANG_STDC_[VER] Check if using C version [VER]
    - C 94
    - C 99
    - C 11
    - C 18

CORE_LIB_STD_[LIB] Check if using C++ standard library [LIB]
    - libc++
    - dunkumware
    - libcomo
    - modena
    - msl
    - msvc
    - rogueware
    - sgi
    - stdcpp3
    - stlport
    - vacpp

CORE_LIBC_C_[LIBC] Check if using [LIBC]
    - cloudabi
    - gnu libc
    - uc
    - vms
    - zos

CORE_HW_SIMD_[PLAT] Check if SIMD for [PLAT] supported
    - x86
    - x86 amd
    - arm
    - ppc

For all above you can always check the version of the tools e.g.
int main()
{
  if (CORE_COMP_GNUC >= CORE_VERSION_NUMBER(4,0,0))
    std::cout << "GCC compiler is at least version 4.0.0" << std::endl;
  else
    std::cout << "GCC compiler is at older than version 4.0.0, or not a GCC compiler" << std::endl;
  return 0;
}

CORE_ARCH_WORD_BITS Detects the native word size, in bits, for the current architecture, also
CORE_ARCH_WORD_BITS_64, CORE_ARCH_WORD_BITS_32, CORE_ARCH_WORD_BITS_16, CORE_ARCH_WORD_BITS (0) available

Detection of endian memory ordering. There are four defined macros
in this header that define the various generally possible endian
memory orderings:

* `CORE_ENDIAN_BIG_BYTE`, byte-swapped big-endian.
* `CORE_ENDIAN_BIG_WORD`, word-swapped big-endian.
* `CORE_ENDIAN_LITTLE_BYTE`, byte-swapped little-endian.
* `CORE_ENDIAN_LITTLE_WORD`, word-swapped little-endian.

If you don't know your configuration you can always use CORE_[TYPE]_NAME e.g. CORE_ARCH_NAME gives the architecture as a readable string

Example:
#include <stdio.h>
#include "Core.h"

int main()
{
    #if CORE_ARCH_X64 // you can use either #if or #ifdef
    puts("X64 Processor");
    #endif

    #if CORE_OS_WINDOWS
    puts("I'm on windows");
    #endif

    #if CORE_PLATFORM_WEB
    puts("Building for web assembly");
    #endif

    #if CORE_COMP_CLANG
    puts("Using clang");
    #endif

    #if CORE_LANG_STDCPP_C17
    puts("Using C++ 17");
    #endif

    #if CORE_LIB_STD_CXX
    puts("Using libc++ as stdlib");
    #endif

    #if CORE_MODE_RELEASE
    puts("In release mode");
    #endif

    #if CORE_HW_SIMD_X86
    puts("Simd x86 supported");
    #endif

    #if CORE_LANG_STDC_C99
    puts("Using version C 99");
    #endif

    #if CORE_LIB_C_GNU
    puts("Using gnu libc");
    #endif

    // Alternatively, works for all types
    puts(CORE_OS_NAME)
    return 0;
}

To reduce compile time turn on/off what is needed.
OS Can NOT be turned off because platform depends on it, if platform is turned off so is OS.
Other includes endianes and wordsize
*/

#define CORE_ENABLE_ARCHITECTURE 1
#define CORE_ENABLE_COMPILER     1
#define CORE_ENABLE_HARDWARE     1
#define CORE_ENABLE_LANGUAGE     1
#define CORE_ENABLE_LIBRARY      1
#define CORE_ENABLE_PLATFORM     1
#define CORE_ENABLE_OTHER        1 && (CORE_ENABLE_ARCHITECTURE && CORE_ENABLE_PLATFORM && CORE_ENABLE_LIBRARY)


/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


// Version numbers ////////////////////////////////////////////////////////////////////////////////
#define CORE_VERSION_NUMBER(major,minor,patch) ((((major)%100)*10000000) + (((minor)%100)*100000) + ((patch)%100000))
#define CORE_VERSION_NUMBER_MAX           CORE_VERSION_NUMBER(99,99,99999)
#define CORE_VERSION_NUMBER_ZERO          CORE_VERSION_NUMBER(0,0,0)
#define CORE_VERSION_NUMBER_MIN           CORE_VERSION_NUMBER(0,0,1)
#define CORE_VERSION_NUMBER_AVAILABLE     CORE_VERSION_NUMBER_MIN
#define CORE_VERSION_NUMBER_NOT_AVAILABLE CORE_VERSION_NUMBER_ZERO

#define CORE_VERSION_NUMBER_MAJOR(N) (((N)/10000000)%100)
#define CORE_VERSION_NUMBER_MINOR(N) (((N)/100000)%100)
#define CORE_VERSION_NUMBER_PATCH(N) ((N)%100000)

#define CORE_PREDEF_MAKE_0X_VRP(V) CORE_VERSION_NUMBER((V&0xF00)>>8,(V&0xF0)>>4,(V&0xF))
#define CORE_PREDEF_MAKE_0X_VVRP(V) CORE_VERSION_NUMBER((V&0xFF00)>>8,(V&0xF0)>>4,(V&0xF))
#define CORE_PREDEF_MAKE_0X_VRPP(V) CORE_VERSION_NUMBER((V&0xF000)>>12,(V&0xF00)>>8,(V&0xFF))
#define CORE_PREDEF_MAKE_0X_VVRR(V) CORE_VERSION_NUMBER((V&0xFF00)>>8,(V&0xFF),0)
#define CORE_PREDEF_MAKE_0X_VRRPPPP(V) CORE_VERSION_NUMBER((V&0xF000000)>>24,(V&0xFF0000)>>16,(V&0xFFFF))
#define CORE_PREDEF_MAKE_0X_VVRRP(V) CORE_VERSION_NUMBER((V&0xFF000)>>12,(V&0xFF0)>>4,(V&0xF))
#define CORE_PREDEF_MAKE_0X_VRRPP000(V) CORE_VERSION_NUMBER((V&0xF0000000)>>28,(V&0xFF00000)>>20,(V&0xFF000)>>12)
#define CORE_PREDEF_MAKE_0X_VVRRPP(V) CORE_VERSION_NUMBER((V&0xFF0000)>>16,(V&0xFF00)>>8,(V&0xFF))
#define CORE_PREDEF_MAKE_10_VPPP(V) CORE_VERSION_NUMBER(((V)/1000)%10,0,(V)%1000)
#define CORE_PREDEF_MAKE_10_VVPPP(V) CORE_VERSION_NUMBER(((V)/1000)%100,0,(V)%1000)
#define CORE_PREDEF_MAKE_10_VR0(V) CORE_VERSION_NUMBER(((V)/100)%10,((V)/10)%10,0)
#define CORE_PREDEF_MAKE_10_VRP(V) CORE_VERSION_NUMBER(((V)/100)%10,((V)/10)%10,(V)%10)
#define CORE_PREDEF_MAKE_10_VRP000(V) CORE_VERSION_NUMBER(((V)/100000)%10,((V)/10000)%10,((V)/1000)%10)
#define CORE_PREDEF_MAKE_10_VRPPPP(V) CORE_VERSION_NUMBER(((V)/100000)%10,((V)/10000)%10,(V)%10000)
#define CORE_PREDEF_MAKE_10_VRPP(V) CORE_VERSION_NUMBER(((V)/1000)%10,((V)/100)%10,(V)%100)
#define CORE_PREDEF_MAKE_10_VRR(V) CORE_VERSION_NUMBER(((V)/100)%10,(V)%100,0)
#define CORE_PREDEF_MAKE_10_VRRPP(V) CORE_VERSION_NUMBER(((V)/10000)%10,((V)/100)%100,(V)%100)
#define CORE_PREDEF_MAKE_10_VRR000(V) CORE_VERSION_NUMBER(((V)/100000)%10,((V)/1000)%100,0)
#define CORE_PREDEF_MAKE_10_VV00(V) CORE_VERSION_NUMBER(((V)/100)%100,0,0)
#define CORE_PREDEF_MAKE_10_VVR_0PPPPP(V, P) CORE_VERSION_NUMBER(((V)/10)%100,(V)%10,(P)%100000)
#define CORE_PREDEF_MAKE_10_VVRR(V) CORE_VERSION_NUMBER(((V)/100)%100,(V)%100,0)
#define CORE_PREDEF_MAKE_10_VVRRP(V) CORE_VERSION_NUMBER(((V)/1000)%100,((V)/10)%100,(V)%10)
#define CORE_PREDEF_MAKE_10_VVRRPP(V) CORE_VERSION_NUMBER(((V)/10000)%100,((V)/100)%100,(V)%100)
#define CORE_PREDEF_MAKE_10_VVRRPPP(V) CORE_VERSION_NUMBER(((V)/100000)%100,((V)/1000)%100,(V)%1000)
#define CORE_PREDEF_MAKE_10_VVRR0PP00(V) CORE_VERSION_NUMBER(((V)/10000000)%100,((V)/100000)%100,((V)/100)%100)
#define CORE_PREDEF_MAKE_10_VVRR0PPPP(V) CORE_VERSION_NUMBER(((V)/10000000)%100,((V)/100000)%100,(V)%10000)
#define CORE_PREDEF_MAKE_10_VVRR00PP00(V) CORE_VERSION_NUMBER(((V)/100000000)%100,((V)/1000000)%100,((V)/100)%100)
#define CORE_PREDEF_MAKE_DATE(Y,M,D) CORE_VERSION_NUMBER((Y)%10000-1970,(M)%100,(D)%100)
#define CORE_PREDEF_MAKE_YYYYMMDD(V) CORE_PREDEF_MAKE_DATE(((V)/10000)%10000,((V)/100)%100,(V)%100)
#define CORE_PREDEF_MAKE_YYYY(V) CORE_PREDEF_MAKE_DATE(V,1,1)
#define CORE_PREDEF_MAKE_YYYYMM(V) CORE_PREDEF_MAKE_DATE((V)/100,(V)%100,1)
// Version numbers end ////////////////////////////////////////////////////////////////////////////


// Mode ///////////////////////////////////////////////////////////////////////////////////////////
#ifdef NDEBUG
    #define CORE_MODE_RELEASE CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_MODE_NAME "Release"
#else
    #define CORE_MODE_DEBUG CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_MODE_NAME "Debug"
#endif
// Mode end ///////////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_ARCHITECTURE
// Architecture start /////////////////////////////////////////////////////////////////////////////
// Architecture WASM /////////////////////////////////////////////////////////////
#ifdef __wasm32__
    #undef CORE_ARCH_NAME
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WASM32 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_NAME "WASM 32"
#elif defined(__wasm64__) || defined(__wasm__)
    #undef CORE_ARCH_NAME
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WASM64 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_NAME "WASM 64"
#endif
// Architecture WASM end /////////////////////////////////////////////////////////


// Architecture x64 //////////////////////////////////////////////////////////////
#if defined(__x86_64) || defined(__x86_64__) ||  defined(__amd64__) || defined(__amd64) ||  defined(_M_X64)
    #define CORE_ARCH_X64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Intel x86-64"
#endif
// Architecture x64 end //////////////////////////////////////////////////////////


// Architecture x86 //////////////////////////////////////////////////////////////
#if defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__i386) || \
    defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__)
    #if !defined(CORE_ARCH_X86) && defined(__I86__)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER(__I86__,0,0)
    #endif
    #if !defined(CORE_ARCH_X86) && defined(_M_IX86)
    #    define CORE_ARCH_X86 CORE_PREDEF_MAKE_10_VV00(_M_IX86)
    #endif
    #if !defined(CORE_ARCH_X86) && defined(__i686__)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER(6,0,0)
    #endif
    #if !defined(CORE_ARCH_X86) && defined(__i586__)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER(5,0,0)
    #endif
    #if !defined(CORE_ARCH_X86) && defined(__i486__)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_X86) && defined(__i386__)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_ARCH_X86)
    #    define CORE_ARCH_X86 CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_X86
    #undef CORE_ARCH_NAME
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_NAME "Intel x86-32"
#endif
// Architecture x86 end //////////////////////////////////////////////////////////


// Architecture ARM //////////////////////////////////////////////////////////////
#if defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARM) || defined(__arm__) || defined(__arm64) || \
    defined(__thumb__) || defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__) ||  defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6KZ__) || \
    defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) || defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__)
    #if !defined(CORE_ARCH_ARM) && defined(__ARM_ARCH)
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(__ARM_ARCH,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && defined(__TARGET_ARCH_ARM)
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(__TARGET_ARCH_ARM,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && defined(__TARGET_ARCH_THUMB)
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(__TARGET_ARCH_THUMB,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && defined(_M_ARM)
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(_M_ARM,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && (defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__))
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(8,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && (defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__))
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(7,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && (defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6KZ__) || defined(__ARM_ARCH_6T2__))
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(6,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && (defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__))
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(5,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM) && (defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__))
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_ARM)
        #define CORE_ARCH_ARM CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_ARM
    #if CORE_ARCH_ARM >= CORE_VERSION_NUMBER(8,0,0)
        #undef CORE_ARCH_WORD_BITS_64
        #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #else
        #undef CORE_ARCH_WORD_BITS_32
        #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "ARM"
#endif
// Architecture ARM end //////////////////////////////////////////////////////////


// Architecture Alpha ////////////////////////////////////////////////////////////
#if defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA)
    #if !defined(CORE_ARCH_ALPHA) && defined(__alpha_ev4__)
        #define CORE_ARCH_ALPHA CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_ALPHA) && defined(__alpha_ev5__)
        #define CORE_ARCH_ALPHA CORE_VERSION_NUMBER(5,0,0)
    #endif
    #if !defined(CORE_ARCH_ALPHA) && defined(__alpha_ev6__)
        #define CORE_ARCH_ALPHA CORE_VERSION_NUMBER(6,0,0)
    #endif
    #if !defined(CORE_ARCH_ALPHA)
        #define CORE_ARCH_ALPHA CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_ALPHA
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "DEC Alpha"
#endif
// Architecture Alpha end ////////////////////////////////////////////////////////


// Architecture Blackfin /////////////////////////////////////////////////////////
#if defined(__bfin__) || defined(__BFIN__) || defined(bfin) || defined(BFIN)
    #define CORE_ARCH_BLACKFIN CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_16
    #define CORE_ARCH_WORD_BITS_16 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Blackfin"
#endif
// Architecture Blackfin end /////////////////////////////////////////////////////


// Architecture Convex ///////////////////////////////////////////////////////////
#if defined(__convex__)
    #if !defined(CORE_ARCH_CONVEX) && defined(__convex_c1__)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER(1,0,0)
    #endif
    #if !defined(CORE_ARCH_CONVEX) && defined(__convex_c2__)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_ARCH_CONVEX) && defined(__convex_c32__)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER(3,2,0)
    #endif
    #if !defined(CORE_ARCH_CONVEX) && defined(__convex_c34__)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER(3,4,0)
    #endif
    #if !defined(CORE_ARCH_CONVEX) && defined(__convex_c38__)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER(3,8,0)
    #endif
    #if !defined(CORE_ARCH_CONVEX)
        #define CORE_ARCH_CONVEX CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_CONVEX
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Convex Computer"
#endif
// Architecture Convex end ///////////////////////////////////////////////////////


// Architecture e2k //////////////////////////////////////////////////////////////
#if defined(__e2k__)
    #if !defined(CORE_ARCH_E2K) && defined(__iset__)
        #define CORE_ARCH_E2K CORE_VERSION_NUMBER(__iset__,0,0)
    #endif
    #if !defined(CORE_ARCH_E2K)
        #define CORE_ARCH_E2K CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_E2K
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "E2K"
#endif
// Architecture e2k end //////////////////////////////////////////////////////////


// Architecture ia64 /////////////////////////////////////////////////////////////
#if defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
    #define CORE_ARCH_IA64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Intel Itanium 64"
#endif
// Architecture ia64 end /////////////////////////////////////////////////////////


// Architecture loong arch ///////////////////////////////////////////////////////
#if defined(__loongarch__)
    #define CORE_ARCH_LOONGARCH CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "LoongArch"
#endif
// Architecture loong arch end ///////////////////////////////////////////////////


// Architecture m68k /////////////////////////////////////////////////////////////
#if defined(__m68k__) || defined(M68000)
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68060__) || defined(mc68060) || defined(__mc68060))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER(6,0,0)
    #endif
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68040__) || defined(mc68040) || defined(__mc68040))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68030__) || defined(mc68030) || defined(__mc68030))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68020__) || defined(mc68020) || defined(__mc68020))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68010__) || defined(mc68010) || defined(__mc68010))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER(1,0,0)
    #endif
    #if !defined(CORE_ARCH_M68K) && (defined(__mc68000__) || defined(mc68000) || defined(__mc68000))
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #if !defined(CORE_ARCH_M68K)
        #define CORE_ARCH_M68K CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_M68K
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Motorola 68k"
#endif
// Architecture m68k end /////////////////////////////////////////////////////////


// Architecture mips /////////////////////////////////////////////////////////////
#if defined(__mips__) || defined(__mips) || defined(__MIPS__)
    #if !defined(CORE_ARCH_MIPS) && (defined(__mips))
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER(__mips,0,0)
    #endif
    #if !defined(CORE_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS1) || defined(_R3000))
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER(1,0,0)
    #endif
    #if !defined(CORE_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS2) || defined(__MIPS_ISA2__) || defined(_R4000))
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS3) || defined(__MIPS_ISA3__))
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS4) || defined(__MIPS_ISA4__))
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_MIPS)
        #define CORE_ARCH_MIPS CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_MIPS
    #if CORE_ARCH_MIPS >= CORE_VERSION_NUMBER(3,0,0)
        #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #else
        #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "MIPS"
#endif
// Architecture mips end /////////////////////////////////////////////////////////


// Architecture parisc ///////////////////////////////////////////////////////////
#if defined(__hppa__) || defined(__hppa) || defined(__HPPA__)
    #if !defined(CORE_ARCH_PARISC) && (defined(_PA_RISC1_0))
        #define CORE_ARCH_PARISC CORE_VERSION_NUMBER(1,0,0)
    #endif
    #if !defined(CORE_ARCH_PARISC) && (defined(_PA_RISC1_1) || defined(__HPPA11__) || defined(__PA7100__))
        #define CORE_ARCH_PARISC CORE_VERSION_NUMBER(1,1,0)
    #endif
    #if !defined(CORE_ARCH_PARISC) && (defined(_PA_RISC2_0) || defined(__RISC2_0__) || defined(__HPPA20__) || defined(__PA8000__))
        #define CORE_ARCH_PARISC CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_ARCH_PARISC)
        #define CORE_ARCH_PARISC CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_PARISC
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "HP/PA RISC"
#endif
// Architecture parisc end ///////////////////////////////////////////////////////


// Architecture ppc //////////////////////////////////////////////////////////////
#if defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || \
    defined(__PPC__) || defined(__PPC64__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(_ARCH_PPC64) || \
    defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(_XENON) || defined(__ppc)
    #if !defined (CORE_ARCH_PPC) && (defined(__ppc601__) || defined(_ARCH_601))
        #define CORE_ARCH_PPC CORE_VERSION_NUMBER(6,1,0)
    #endif
    #if !defined (CORE_ARCH_PPC) && (defined(__ppc603__) || defined(_ARCH_603))
        #define CORE_ARCH_PPC CORE_VERSION_NUMBER(6,3,0)
    #endif
    #if !defined (CORE_ARCH_PPC) && (defined(__ppc604__) || defined(__ppc604__))
        #define CORE_ARCH_PPC CORE_VERSION_NUMBER(6,4,0)
    #endif
    #if !defined (CORE_ARCH_PPC)
        #define CORE_ARCH_PPC CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_PPC
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "PowerPC"
#endif
// Architecture ppc end //////////////////////////////////////////////////////////


// Architecture ppc64 ////////////////////////////////////////////////////////////
#if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(_ARCH_PPC64)
    #define CORE_ARCH_PPC_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "PowerPC64"
#endif
// Architecture ppc64 end ////////////////////////////////////////////////////////


// Architecture ptx //////////////////////////////////////////////////////////////
#if defined(__CUDA_ARCH__)
    #define CORE_ARCH_PTX CORE_PREDEF_MAKE_10_VR0(__CUDA_ARCH__)
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "PTX"
#endif
// Architecture ptx end //////////////////////////////////////////////////////////


// Architecture pyramid //////////////////////////////////////////////////////////
#if defined(pyr)
    #define CORE_ARCH_PYRAMID CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "Pyramid 9810"
#endif
// Architecture pyramid end //////////////////////////////////////////////////////


// Architecture riscv ////////////////////////////////////////////////////////////
#if defined(__riscv)
    #define CORE_ARCH_RISCV CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "RISC-V"
#endif
// Architecture riscv end ////////////////////////////////////////////////////////


// Architecture rs6k /////////////////////////////////////////////////////////////
#if defined(__THW_RS6000) || defined(_IBMR2) || defined(_POWER) || defined(_ARCH_PWR) || defined(_ARCH_PWR2)
    #define CORE_ARCH_RS6000 CORE_VERSION_NUMBER_AVAILABLE
    #define CORE_ARCH_PWR CORE_ARCH_RS6000
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "RS/6000"
#endif
// Architecture rs6k end /////////////////////////////////////////////////////////


// Architecture sparc ////////////////////////////////////////////////////////////
#if defined(__sparc__) || defined(__sparc)
    #if !defined(CORE_ARCH_SPARC) && (defined(__sparcv9) || defined(__sparc_v9__))
        #define CORE_ARCH_SPARC CORE_VERSION_NUMBER(9,0,0)
    #endif
    #if !defined(CORE_ARCH_SPARC) && (defined(__sparcv8) || defined(__sparc_v8__))
        #define CORE_ARCH_SPARC CORE_VERSION_NUMBER(8,0,0)
    #endif
    #if !defined(CORE_ARCH_SPARC)
        #define CORE_ARCH_SPARC CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_SPARC
    #if CORE_ARCH_SPARC >= CORE_VERSION_NUMBER(9,0,0)
        #undef CORE_ARCH_WORD_BITS_64
        #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #else
        #undef CORE_ARCH_WORD_BITS_32
        #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "SPARC"
#endif
// Architecture sparc end ////////////////////////////////////////////////////////


// Architecture superh ///////////////////////////////////////////////////////////
#if defined(__sh__)
    #if !defined(CORE_ARCH_SH) && (defined(__SH5__))
        #define CORE_ARCH_SH CORE_VERSION_NUMBER(5,0,0)
    #endif
    #if !defined(CORE_ARCH_SH) && (defined(__SH4__))
        #define CORE_ARCH_SH CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_ARCH_SH) && (defined(__sh3__) || defined(__SH3__))
        #define CORE_ARCH_SH CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_ARCH_SH) && (defined(__sh2__))
        #define CORE_ARCH_SH CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_ARCH_SH) && (defined(__sh1__))
        #define CORE_ARCH_SH CORE_VERSION_NUMBER(1,0,0)
    #endif
    #if !defined(CORE_ARCH_SH)
        #define CORE_ARCH_SH CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#ifdef CORE_ARCH_SH
    #if CORE_ARCH_SH >= CORE_VERSION_NUMBER(5,0,0)
        #undef CORE_ARCH_WORD_BITS_64
        #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #elif CORE_ARCH_SH >= CORE_VERSION_NUMBER(3,0,0)
        #undef CORE_ARCH_WORD_BITS_32
        #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #else
        #undef CORE_ARCH_WORD_BITS_16
        #define CORE_ARCH_WORD_BITS_16 CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "SuperH"
#endif
// Architecture superh end ///////////////////////////////////////////////////////


// Architecture sys370 ///////////////////////////////////////////////////////////
#if defined(__370__) || defined(__THW_370__)
    #define CORE_ARCH_SYS370 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "System/370"
#endif
// Architecture sys370 end ///////////////////////////////////////////////////////


// Architecture sys390 ///////////////////////////////////////////////////////////
#if defined(__s390__) || defined(__s390x__)
    #define CORE_ARCH_SYS390 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_32
    #define CORE_ARCH_WORD_BITS_32 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "System/390"
#endif
// Architecture sys390 end ///////////////////////////////////////////////////////


// Architecture z ////////////////////////////////////////////////////////////////
#if defined(__SYSC_ZARCH__)
    #define CORE_ARCH_Z CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_WORD_BITS_64
    #define CORE_ARCH_WORD_BITS_64 CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_ARCH_NAME
    #define CORE_ARCH_NAME "z/Architecture"
#endif
// Architecture z end ////////////////////////////////////////////////////////////
#endif // CORE_ENABLE_ARCHITECTURE

#if !defined(CORE_ARCH_NAME) // Default CORE_ARCH_NAME value
    #define CORE_ARCH_NAME "Unknown"
#endif
// Architecture end ///////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_COMPILER
// Compiler start /////////////////////////////////////////////////////////////////////////////////
// Compiler Borland //////////////////////////////////////////////////////////////
#if defined(__BORLANDC__) || defined(__CODEGEARC__)
    #if !defined(CORE_COMP_BORLAND_DETECTION) && (defined(__CODEGEARC__))
        #define CORE_COMP_BORLAND_DETECTION CORE_PREDEF_MAKE_0X_VVRP(__CODEGEARC__)
    #endif
    #if !defined(CORE_COMP_BORLAND_DETECTION)
        #define CORE_COMP_BORLAND_DETECTION CORE_PREDEF_MAKE_0X_VVRP(__BORLANDC__)
    #endif
#endif

#define CORE_COMP_BORLAND_NAME "Borland C++"
#ifdef CORE_COMP_BORLAND_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_BORLAND_EMULATED CORE_COMP_BORLAND_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_BORLAND_NAME
        #define CORE_COMP_BORLAND CORE_COMP_BORLAND_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler Borland end //////////////////////////////////////////////////////////


// Compiler Emscripten ///////////////////////////////////////////////////////////
#if defined(__EMSCRIPTEN__)
    #define CORE_COMP_EMSCRIPTEN_DETECTION CORE_VERSION_NUMBER(__clang_major__,__clang_minor__,__clang_patchlevel__)
#endif

#define CORE_COMP_EMSCRIPTEN_NAME "Emscripten"
#ifdef CORE_COMP_EMSCRIPTEN_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_EMSCRIPTEN_EMULATED CORE_COMP_EMSCRIPTEN_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_EMSCRIPTEN_NAME
        #define CORE_COMP_EMSCRIPTEN CORE_COMP_EMSCRIPTEN_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler Emscripten end ///////////////////////////////////////////////////////


// Compiler Clang ////////////////////////////////////////////////////////////////
#if defined(__clang__)
    #define CORE_COMP_CLANG_DETECTION CORE_VERSION_NUMBER(__clang_major__,__clang_minor__,__clang_patchlevel__)
#endif

#define CORE_COMP_CLANG_NAME "Clang"
#ifdef CORE_COMP_CLANG_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_CLANG_EMULATED CORE_COMP_CLANG_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_CLANG_NAME
        #define CORE_COMP_CLANG CORE_COMP_CLANG_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler Clang end ////////////////////////////////////////////////////////////


// Compiler gcc_xml //////////////////////////////////////////////////////////////
#if defined(__GCCXML__)
    #define CORE_COMP_GCCXML_DETECTION CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_COMP_GCCXML_NAME "GCC XML"
#ifdef CORE_COMP_GCCXML_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_GCCXML_EMULATED CORE_COMP_GCCXML_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_GCCXML_NAME
        #define CORE_COMP_GCCXML CORE_COMP_GCCXML_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler gcc_xml end //////////////////////////////////////////////////////////


// Compiler gcc //////////////////////////////////////////////////////////////////
#if defined(__GNUC__)
    #if !defined(CORE_COMP_GCC_DETECTION) && defined(__GNUC_PATCHLEVEL__)
        #define CORE_COMP_GCC_DETECTION CORE_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
    #endif
    #if !defined(CORE_COMP_GCC_DETECTION)
        #define CORE_COMP_GCC_DETECTION CORE_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,0)
    #endif
#endif

#define CORE_COMP_GCC_NAME "Gnu GCC C/C++"
#ifdef CORE_COMP_GCC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_GCC_EMULATED CORE_COMP_GCC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_GCC_NAME
        #define CORE_COMP_GCC CORE_COMP_GCC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler gcc end //////////////////////////////////////////////////////////////


// Compiler msvc /////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
    #if !defined (_MSC_FULL_VER)
        #define CORE_COMP_MSVC_BUILD 0
    #else
    /* how many digits does the build number have? */
        #if _MSC_FULL_VER / 10000 == _MSC_VER
            /* four digits */
            #define CORE_COMP_MSVC_BUILD (_MSC_FULL_VER % 10000)
        #elif _MSC_FULL_VER / 100000 == _MSC_VER
            /* five digits */
            #define CORE_COMP_MSVC_BUILD (_MSC_FULL_VER % 100000)
        #else
            #error "Cannot determine build number from _MSC_FULL_VER"
        #endif
    #endif
    /*
    VS2014 was skipped in the release sequence for MS. Which
    means that the compiler and VS product versions are no longer
    in sync. Hence we need to use different formulas for
    mapping from MSC version to VS product version.

    VS2017 is a total nightmare when it comes to version numbers.
    Hence to avoid arguments relating to that both present and
    future.. Any version after VS2015 will use solely the compiler
    version, i.e. cl.exe, as the version number here.
    */
    #if (_MSC_VER > 1900)
        #define CORE_COMP_MSVC_DETECTION CORE_VERSION_NUMBER(_MSC_VER/100, _MSC_VER%100, CORE_COMP_MSVC_BUILD)
    #elif (_MSC_VER >= 1900)
        #define CORE_COMP_MSVC_DETECTION CORE_VERSION_NUMBER(_MSC_VER/100-5, _MSC_VER%100, CORE_COMP_MSVC_BUILD)
    #else
        #define CORE_COMP_MSVC_DETECTION CORE_VERSION_NUMBER(_MSC_VER/100-6, _MSC_VER%100, CORE_COMP_MSVC_BUILD)
    #endif
#endif

#define CORE_COMP_MSVC_NAME "Microsoft Visual C/C++"
#ifdef CORE_COMP_MSVC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_MSVC_EMULATED CORE_COMP_MSVC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_MSVC_NAME
        #define CORE_COMP_MSVC CORE_COMP_MSVC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler msvc end /////////////////////////////////////////////////////////////


// Compiler comeau ///////////////////////////////////////////////////////////////
#if defined(__COMO__)
    #if !defined(CORE_COMP_COMO_DETECTION) && defined(__COMO_VERSION__)
        #define CORE_COMP_COMO_DETECTION CORE_PREDEF_MAKE_0X_VRP(__COMO_VERSION__)
    #endif
    #if !defined(CORE_COMP_COMO_DETECTION)
        #define CORE_COMP_COMO_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_COMO_NAME "Comeau C++"
#ifdef CORE_COMP_COMO_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_COMO_EMULATED CORE_COMP_COMO_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_COMO_NAME
        #define CORE_COMP_COMO CORE_COMP_COMO_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler comeau end ///////////////////////////////////////////////////////////


// Compiler compaq ///////////////////////////////////////////////////////////////
#if defined(__DECC) || defined(__DECCXX)
    #if !defined(CORE_COMP_DEC_DETECTION) && defined(__DECCXX_VER)
        #define CORE_COMP_DEC_DETECTION CORE_PREDEF_MAKE_10_VVRR0PP00(__DECCXX_VER)
    #endif
    #if !defined(CORE_COMP_DEC_DETECTION) && defined(__DECC_VER)
        #define CORE_COMP_DEC_DETECTION CORE_PREDEF_MAKE_10_VVRR0PP00(__DECC_VER)
    #endif
    #if !defined(CORE_COMP_DEC_DETECTION)
        #define CORE_COMP_DEC_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_DEC_NAME "Compaq C/C++"
#ifdef CORE_COMP_DEC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_DEC_EMULATED CORE_COMP_DEC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_DEC_NAME
        #define CORE_COMP_DEC CORE_COMP_DEC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler compaq end ///////////////////////////////////////////////////////////


// Compiler diab /////////////////////////////////////////////////////////////////
#if defined(__DCC__)
    #define CORE_COMP_DIAB_DETECTION CORE_PREDEF_MAKE_10_VRPP(__VERSION_NUMBER__)
#endif

#define CORE_COMP_DIAB_NAME "Diab C/C++"
#ifdef CORE_COMP_DIAB_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_DIAB_EMULATED CORE_COMP_DIAB_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_DEC_NAME
        #define CORE_COMP_DIAB CORE_COMP_DIAB_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler diab end /////////////////////////////////////////////////////////////


// Compiler dignus ///////////////////////////////////////////////////////////////
#if defined(__SYSC__)
#   define CORE_COMP_SYSC_DETECTION CORE_PREDEF_MAKE_10_VRRPP(__SYSC_VER__)
#endif

#define CORE_COMP_SYSC_NAME "Dignus Systems/C++"
#ifdef CORE_COMP_SYSC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_SYSC_EMULATED CORE_COMP_SYSC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_SYSC_NAME
        #define CORE_COMP_SYSC CORE_COMP_SYSC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler dignus end ///////////////////////////////////////////////////////////


// Compiler edg //////////////////////////////////////////////////////////////////
#if defined(__EDG__)
#   define CORE_COMP_EDG_DETECTION CORE_PREDEF_MAKE_10_VRRPP(__EDG_VERSION__)
#endif

#define CORE_COMP_EDG_NAME "EDG C++ Frontend"
#ifdef CORE_COMP_EDG_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_EDG_EMULATED CORE_COMP_EDG_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_EDG_NAME
        #define CORE_COMP_EDG CORE_COMP_EDG_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler edg end //////////////////////////////////////////////////////////////


// Compiler ekopath //////////////////////////////////////////////////////////////
#if defined(__PATHCC__)
    #define CORE_COMP_PATH_DETECTION CORE_VERSION_NUMBER(__PATHCC__,__PATHCC_MINOR__,__PATHCC_PATCHLEVEL__)
#endif

#define CORE_COMP_PATH_NAME "EKOpath"
#ifdef CORE_COMP_PATH_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_PATH_EMULATED CORE_COMP_PATH_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_PATH_NAME
        #define CORE_COMP_PATH CORE_COMP_PATH_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler ekopath end //////////////////////////////////////////////////////////


// Compiler green hills //////////////////////////////////////////////////////////
#if defined(__ghs) || defined(__ghs__)
    #if !defined(CORE_COMP_GHS_DETECTION) && defined(__GHS_VERSION_NUMBER__)
        #define CORE_COMP_GHS_DETECTION CORE_PREDEF_MAKE_10_VRP(__GHS_VERSION_NUMBER__)
    #endif
    #if !defined(CORE_COMP_GHS_DETECTION) && defined(__ghs)
        #define CORE_COMP_GHS_DETECTION CORE_PREDEF_MAKE_10_VRP(__ghs)
    #endif
    #if !defined(CORE_COMP_GHS_DETECTION)
        #define CORE_COMP_GHS_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_GHS_NAME "Green Hills C/C++"
#ifdef CORE_COMP_GHS_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_GHS_EMULATED CORE_COMP_GHS_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_GHS_NAME
        #define CORE_COMP_GHS CORE_COMP_GHS_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler green hills end //////////////////////////////////////////////////////


// Compiler hp_acc ///////////////////////////////////////////////////////////////
#if defined(__HP_aCC)
    #if !defined(CORE_COMP_HPACC_DETECTION) && (__HP_aCC > 1)
        #define CORE_COMP_HPACC_DETECTION CORE_PREDEF_MAKE_10_VVRRPP(__HP_aCC)
    #endif
    #if !defined(CORE_COMP_HPACC_DETECTION)
        #define CORE_COMP_HPACC_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_HPACC_NAME "HP aC++"
#ifdef CORE_COMP_HPACC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_HPACC_EMULATED CORE_COMP_HPACC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_HPACC_NAME
        #define CORE_COMP_HPACC CORE_COMP_HPACC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler hp_acc end ///////////////////////////////////////////////////////////


// Compiler iar //////////////////////////////////////////////////////////////////
#if defined(__IAR_SYSTEMS_ICC__)
    #define CORE_COMP_IAR_DETECTION CORE_PREDEF_MAKE_10_VVRR(__VER__)
#endif

#define CORE_COMP_IAR_NAME "IAR C/C++"
#ifdef CORE_COMP_IAR_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_IAR_EMULATED CORE_COMP_IAR_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_IAR_NAME
        #define CORE_COMP_IAR CORE_COMP_IAR_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler iar end //////////////////////////////////////////////////////////////


// Compiler ibm //////////////////////////////////////////////////////////////////
#if defined(__IBMCPP__) || defined(__xlC__) || defined(__xlc__)
    #if !defined(CORE_COMP_IBM_DETECTION) && defined(__COMPILER_VER__)
        #define CORE_COMP_IBM_DETECTION CORE_PREDEF_MAKE_0X_VRRPPPP(__COMPILER_VER__)
    #endif
    #if !defined(CORE_COMP_IBM_DETECTION) && defined(__xlC__)
        #define CORE_COMP_IBM_DETECTION CORE_PREDEF_MAKE_0X_VVRR(__xlC__)
    #endif
    #if !defined(CORE_COMP_IBM_DETECTION) && defined(__xlc__)
        #define CORE_COMP_IBM_DETECTION CORE_PREDEF_MAKE_0X_VVRR(__xlc__)
    #endif
    #if !defined(CORE_COMP_IBM_DETECTION)
        #define CORE_COMP_IBM_DETECTION CORE_PREDEF_MAKE_10_VRP(__IBMCPP__)
    #endif
#endif

#define CORE_COMP_IBM_NAME "IBM XL C/C++"
#ifdef CORE_COMP_IBM_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_IBM_EMULATED CORE_COMP_IBM_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_IBM_NAME
        #define CORE_COMP_IBM CORE_COMP_IBM_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler ibm end //////////////////////////////////////////////////////////////


// Compiler intel ////////////////////////////////////////////////////////////////
#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
// NOTE: Because of an Intel mistake in the release version numbering when `__INTEL_COMPILER` is `9999` it is detected as version 12.1.0.
    #if !defined(CORE_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER) && (__INTEL_COMPILER == 9999)
        #define CORE_COMP_INTEL_DETECTION CORE_VERSION_NUMBER(12,1,0)
    #endif
    #if !defined(CORE_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER) && defined(__INTEL_COMPILER_UPDATE)
        #define CORE_COMP_INTEL_DETECTION CORE_VERSION_NUMBER(CORE_VERSION_NUMBER_MAJOR(CORE_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)), CORE_VERSION_NUMBER_MINOR(CORE_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)), __INTEL_COMPILER_UPDATE)
    #endif
    #if !defined(CORE_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER)
        #define CORE_COMP_INTEL_DETECTION CORE_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)
    #endif
    #if !defined(CORE_COMP_INTEL_DETECTION)
        #define CORE_COMP_INTEL_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_INTEL_NAME "Intel C/C++"
#ifdef CORE_COMP_INTEL_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_INTEL_EMULATED CORE_COMP_INTEL_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_INTEL_NAME
        #define CORE_COMP_INTEL CORE_COMP_INTEL_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler intel end ////////////////////////////////////////////////////////////


// Compiler kai //////////////////////////////////////////////////////////////////
#if defined(__KCC)
    #define CORE_COMP_KCC_DETECTION CORE_PREDEF_MAKE_0X_VRPP(__KCC_VERSION)
#endif

#define CORE_COMP_KCC_NAME "Kai C++"
#ifdef CORE_COMP_KCC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_KCC_EMULATED CORE_COMP_KCC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_KCC_NAME
        #define CORE_COMP_KCC CORE_COMP_KCC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler kai end //////////////////////////////////////////////////////////////


// Compiler llvm /////////////////////////////////////////////////////////////////
#if defined(__llvm__)
    #define CORE_COMP_LLVM_DETECTION CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_COMP_LLVM_NAME "LLVM"
#ifdef CORE_COMP_LLVM_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_LLVM_EMULATED CORE_COMP_LLVM_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_LLVM_NAME
        #define CORE_COMP_LLVM CORE_COMP_LLVM_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler llvm end /////////////////////////////////////////////////////////////


// Compiler metaware /////////////////////////////////////////////////////////////
#if defined(__HIGHC__)
    #define CORE_COMP_HIGHC_DETECTION CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_COMP_HIGHC_NAME "MetaWare High C/C++"
#ifdef CORE_COMP_HIGHC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_HIGHC_EMULATED CORE_COMP_HIGHC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_HIGHC_NAME
        #define CORE_COMP_HIGHC CORE_COMP_HIGHC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler metaware end /////////////////////////////////////////////////////////


// Compiler metrowerks ///////////////////////////////////////////////////////////
#if defined(__MWERKS__) || defined(__CWCC__)
    #if !defined(CORE_COMP_MWERKS_DETECTION) && defined(__CWCC__)
        #define CORE_COMP_MWERKS_DETECTION CORE_PREDEF_MAKE_0X_VRPP(__CWCC__)
    #endif
    #if !defined(CORE_COMP_MWERKS_DETECTION) && (__MWERKS__ >= 0x4200)
        #define CORE_COMP_MWERKS_DETECTION CORE_PREDEF_MAKE_0X_VRPP(__MWERKS__)
    #endif
    #if !defined(CORE_COMP_MWERKS_DETECTION) && (__MWERKS__ >= 0x3204) // note the "skip": 04->9.3
        #define CORE_COMP_MWERKS_DETECTION CORE_VERSION_NUMBER(9,(__MWERKS__)%100-1,0)
    #endif
    #if !defined(CORE_COMP_MWERKS_DETECTION) && (__MWERKS__ >= 0x3200)
        #define CORE_COMP_MWERKS_DETECTION CORE_VERSION_NUMBER(9,(__MWERKS__)%100,0)
    #endif
    #if !defined(CORE_COMP_MWERKS_DETECTION) && (__MWERKS__ >= 0x3000)
        #define CORE_COMP_MWERKS_DETECTION CORE_VERSION_NUMBER(8,(__MWERKS__)%100,0)
    #endif
    #if !defined(CORE_COMP_MWERKS_DETECTION)
        #define CORE_COMP_MWERKS_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_MWERKS_NAME "Metrowerks CodeWarrior"
#ifdef CORE_COMP_MWERKS_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_MWERKS_EMULATED CORE_COMP_MWERKS_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_MWERKS_NAME
        #define CORE_COMP_MWERKS CORE_COMP_MWERKS_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler metrowerks end ///////////////////////////////////////////////////////


// Compiler microtec /////////////////////////////////////////////////////////////
#if defined(_MRI)
    #define CORE_COMP_MRI_DETECTION CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_COMP_MRI_NAME "Microtec C/C++"
#ifdef CORE_COMP_MRI_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_MRI_EMULATED CORE_COMP_MRI_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_MRI_NAME
        #define CORE_COMP_MRI CORE_COMP_MRI_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler microtec end /////////////////////////////////////////////////////////


// Compiler mpw //////////////////////////////////////////////////////////////////
#if defined(__MRC__) || defined(MPW_C) || defined(MPW_CPLUS)
    #if !defined(CORE_COMP_MPW_DETECTION) && defined(__MRC__)
        #define CORE_COMP_MPW_DETECTION CORE_PREDEF_MAKE_0X_VVRR(__MRC__)
    #endif
    #if !defined(CORE_COMP_MPW_DETECTION)
        #define CORE_COMP_MPW_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_MPW_NAME "MPW C++"
#ifdef CORE_COMP_MPW_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_MPW_EMULATED CORE_COMP_MPW_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_MPW_NAME
        #define CORE_COMP_MPW CORE_COMP_MPW_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler mpw end //////////////////////////////////////////////////////////////


// Compiler nvcc /////////////////////////////////////////////////////////////////
#if defined(__NVCC__)
    #if !defined(__CUDACC_VER_MAJOR__) || !defined(__CUDACC_VER_MINOR__) || !defined(__CUDACC_VER_BUILD__)
        #define CORE_COMP_NVCC_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #else
        #define CORE_COMP_NVCC_DETECTION CORE_VERSION_NUMBER(__CUDACC_VER_MAJOR__, __CUDACC_VER_MINOR__, __CUDACC_VER_BUILD__)
    #endif
#endif

#define CORE_COMP_NVCC_NAME "NVCC"
#ifdef CORE_COMP_NVCC_DETECTION
    #undef CORE_COMP_NAME
    #define CORE_COMP_NVCC CORE_COMP_NVCC_DETECTION
    #define CORE_COMP_NAME CORE_COMP_NVCC_NAME
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler nvcc end /////////////////////////////////////////////////////////////


// Compiler palm /////////////////////////////////////////////////////////////////
#if defined(_PACC_VER)
    #define CORE_COMP_PALM_DETECTION CORE_PREDEF_MAKE_0X_VRRPP000(_PACC_VER)
#endif

#define CORE_COMP_PALM_NAME "Palm C/C++"
#ifdef CORE_COMP_PALM_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_PALM_EMULATED CORE_COMP_PALM_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_PALM_NAME
        #define CORE_COMP_PALM CORE_COMP_PALM_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler palm end /////////////////////////////////////////////////////////////


// Compiler pgi //////////////////////////////////////////////////////////////////
#if defined(__PGI)
    #if !defined(CORE_COMP_PGI_DETECTION) && (defined(__PGIC__) && defined(__PGIC_MINOR__) && defined(__PGIC_PATCHLEVEL__))
        #define CORE_COMP_PGI_DETECTION CORE_VERSION_NUMBER(__PGIC__,__PGIC_MINOR__,__PGIC_PATCHLEVEL__)
    #endif
    #if !defined(CORE_COMP_PGI_DETECTION)
        #define CORE_COMP_PGI_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_PGI_NAME "Portland Group C/C++"
#ifdef CORE_COMP_PGI_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_PGI_EMULATED CORE_COMP_PGI_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_PGI_NAME
        #define CORE_COMP_PGI CORE_COMP_PGI_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler pgi end //////////////////////////////////////////////////////////////


// Compiler sgi mipspro //////////////////////////////////////////////////////////
#if defined(__sgi) || defined(sgi)
    #if !defined(CORE_COMP_SGI_DETECTION) && defined(_SGI_COMPILER_VERSION)
        #define CORE_COMP_SGI_DETECTION CORE_PREDEF_MAKE_10_VRP(_SGI_COMPILER_VERSION)
    #endif
    #if !defined(CORE_COMP_SGI_DETECTION) && defined(_COMPILER_VERSION)
        #define CORE_COMP_SGI_DETECTION CORE_PREDEF_MAKE_10_VRP(_COMPILER_VERSION)
    #endif
    #if !defined(CORE_COMP_SGI_DETECTION)
        #define CORE_COMP_SGI_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_SGI_NAME "SGI MIPSpro"
#ifdef CORE_COMP_SGI_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_SGI_EMULATED CORE_COMP_SGI_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_SGI_NAME
        #define CORE_COMP_SGI CORE_COMP_SGI_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler sgi mipspro end //////////////////////////////////////////////////////


// Compiler sunpro ///////////////////////////////////////////////////////////////
#if defined(__SUNPRO_CC) || defined(__SUNPRO_C)
    #if !defined(CORE_COMP_SUNPRO_DETECTION) && defined(__SUNPRO_CC)
        #if (__SUNPRO_CC < 0x5100)
            #define CORE_COMP_SUNPRO_DETECTION CORE_PREDEF_MAKE_0X_VRP(__SUNPRO_CC)
        #else
            #define CORE_COMP_SUNPRO_DETECTION CORE_PREDEF_MAKE_0X_VVRRP(__SUNPRO_CC)
        #endif
    #endif
    #if !defined(CORE_COMP_SUNPRO_DETECTION) && defined(__SUNPRO_C)
        #if (__SUNPRO_C < 0x5100)
            #define CORE_COMP_SUNPRO_DETECTION CORE_PREDEF_MAKE_0X_VRP(__SUNPRO_C)
        #else
            #define CORE_COMP_SUNPRO_DETECTION CORE_PREDEF_MAKE_0X_VVRRP(__SUNPRO_C)
        #endif
    #endif
    #if !defined(CORE_COMP_SUNPRO_DETECTION)
        #define CORE_COMP_SUNPRO_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_COMP_SUNPRO_NAME "Oracle Solaris Studio"
#ifdef CORE_COMP_SUNPRO_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_SUNPRO_EMULATED CORE_COMP_SUNPRO_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_SUNPRO_NAME
        #define CORE_COMP_SUNPRO CORE_COMP_SUNPRO_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler sunpro end ///////////////////////////////////////////////////////////


// Compiler tendra ///////////////////////////////////////////////////////////////
#if defined(__TenDRA__)
#   define CORE_COMP_TENDRA_DETECTION CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_COMP_TENDRA_NAME "TenDRA C/C++"
#ifdef CORE_COMP_TENDRA_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_TENDRA_EMULATED CORE_COMP_TENDRA_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_TENDRA_NAME
        #define CORE_COMP_TENDRA CORE_COMP_TENDRA_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler tendra end ///////////////////////////////////////////////////////////


// Compiler watcom ///////////////////////////////////////////////////////////////
#if defined(__WATCOMC__)
    #define CORE_COMP_WATCOM_DETECTION CORE_PREDEF_MAKE_10_VVRR(__WATCOMC__)
#endif

#define CORE_COMP_WATCOM_NAME "Watcom C++"
#ifdef CORE_COMP_WATCOM_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_WATCOM_EMULATED CORE_COMP_WATCOM_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_WATCOM_NAME
        #define CORE_COMP_WATCOM CORE_COMP_WATCOM_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler watcom end ///////////////////////////////////////////////////////////


// Compiler digital mars /////////////////////////////////////////////////////////
#if defined(__DMC__)
    #define CORE_COMP_DMC_DETECTION  CORE_PREDEF_MAKE_10_VRPP(__DMC__)
#endif

#define CORE_COMP_DMC_DETECTION "Digital Mars"
#ifdef CORE_COMP_DMC_DETECTION
    #if defined(CORE_PREDEF_DETAIL_COMP_DETECTED)
        #define CORE_COMP_DMC_EMULATED CORE_COMP_DMC_DETECTION
    #else
        #undef CORE_COMP_NAME
        #define CORE_COMP_NAME CORE_COMP_DMC_DETECTION
        #define CORE_COMP_DMC  CORE_COMP_DMC_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_COMP_DETECTED
    #define CORE_PREDEF_DETAIL_COMP_DETECTED 1
    #endif
#endif
// Compiler digital mars end /////////////////////////////////////////////////////
#endif // CORE_ENABLE_COMPILER

#if !defined(CORE_COMP_NAME) // Default CORE_COMP_NAME value
    #define CORE_COMP_NAME "Unknown"
#endif
// Compiler end ///////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_LANGUAGE
// Language ///////////////////////////////////////////////////////////////////////////////////////
// Language stdc /////////////////////////////////////////////////////////////////
#if defined(__STDC__)
    #if defined(__STDC_VERSION__)
        #if (__STDC_VERSION__ > 100)
            #define CORE_LANG_STDC CORE_PREDEF_MAKE_YYYYMM(__STDC_VERSION__)
            #if (__STDC_VERSION__ == 201710L)
                #define CORE_LANG_STDC_NAME "Standard C18"
                #define CORE_LANG_STDC_C18 CORE_VERSION_NUMBER_AVAILABLE
            #elif (__STDC_VERSION__ == 201112L)
                #define CORE_LANG_STDC_NAME "Standard C11"
                #define CORE_LANG_STDC_C11 CORE_VERSION_NUMBER_AVAILABLE
            #elif (__STDC_VERSION__ == 199901L)
                #define CORE_LANG_STDC_NAME "Standard C99"
                #define CORE_LANG_STDC_C99 CORE_VERSION_NUMBER_AVAILABLE
            #elif (__STDC_VERSION__ == 199409L)
                #define CORE_LANG_STDC_NAME "Standard C94"
                #define CORE_LANG_STDC_C94 CORE_VERSION_NUMBER_AVAILABLE
            #endif
        #else
            #define CORE_LANG_STDC CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #else
        #define CORE_LANG_STDC CORE_VERSION_NUMBER_AVAILABLE
    #endif

    #if !defined(CORE_LANG_STDC_NAME)
        #define CORE_LANG_STDC_NAME "Standard C"
    #endif

    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "C"
#endif
// Language stdc end /////////////////////////////////////////////////////////////


// Language stdcpp ///////////////////////////////////////////////////////////////
#if defined(__cplusplus)
    #if (__cplusplus > 100)
        #define CORE_LANG_STDCPP CORE_PREDEF_MAKE_YYYYMM(__cplusplus)
        #if (__cplusplus == 199711L)
            #define CORE_LANG_STDCPP_NAME "Standard C++98"
            #define CORE_LANG_STDCPP_CPP98 CORE_VERSION_NUMBER_AVAILABLE
        #elif (__cplusplus == 201103L)
            #define CORE_LANG_STDCPP_NAME "Standard C++11"
            #define CORE_LANG_STDCPP_CPP11 CORE_VERSION_NUMBER_AVAILABLE
        #elif (__cplusplus == 201402L)
            #define CORE_LANG_STDCPP_NAME "Standard C++14"
            #define CORE_LANG_STDCPP_CPP14 CORE_VERSION_NUMBER_AVAILABLE
        #elif (__cplusplus == 201703L)
            #define CORE_LANG_STDCPP_NAME "Standard C++17"
            #define CORE_LANG_STDCPP_CPP17 CORE_VERSION_NUMBER_AVAILABLE
        #elif (__cplusplus == 202002L)
            #define CORE_LANG_STDCPP_NAME "Standard C++20"
            #define CORE_LANG_STDCPP_CPP20 CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #else
        #define CORE_LANG_STDCPP CORE_VERSION_NUMBER_AVAILABLE
    #endif

    #if !defined(CORE_LANG_STDCPP_NAME)
        #define CORE_LANG_STDCPP_NAME "Standard C++"
    #endif

    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "C++"
#endif

#define CORE_LANG_STDCPPCLI_NAME "Standard C++/CLI"
#if defined(__cplusplus_cli)
    #if (__cplusplus_cli > 100)
        #define CORE_LANG_STDCPPCLI CORE_PREDEF_MAKE_YYYYMM(__cplusplus_cli)
    #else
        #define CORE_LANG_STDCPPCLI CORE_VERSION_NUMBER_AVAILABLE
    #endif

    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "C++/CLI"
#endif

#define CORE_LANG_STDECPP_NAME "Standard Embedded C++"
#if defined(__embedded_cplusplus)
    #define CORE_LANG_STDECPP CORE_VERSION_NUMBER_AVAILABLE
    
    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "Embedded C++"
#endif
// Language stdcpp end ///////////////////////////////////////////////////////////


// Language objc /////////////////////////////////////////////////////////////////
#define CORE_LANG_OBJC_NAME "Objective-C"
#if defined(__OBJC__)
    #define CORE_LANG_OBJC CORE_VERSION_NUMBER_AVAILABLE

    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "Objective-C"
#endif
// Language objc end /////////////////////////////////////////////////////////////


// Language cuda /////////////////////////////////////////////////////////////////
#define CORE_LANG_CUDA_NAME "CUDA C/C++"
#if defined(__CUDACC__) || defined(__CUDA__)
    #include <cuda.h>
    #if defined(CUDA_VERSION)
        #define CORE_LANG_CUDA CORE_PREDEF_MAKE_10_VVRRP(CUDA_VERSION)
    #else
        #define CORE_LANG_CUDA CORE_VERSION_NUMBER_AVAILABLE
    #endif

    #undef CORE_LANG_NAME
    #define CORE_LANG_NAME "CUDA C/C++"
#endif
// Language cuda end /////////////////////////////////////////////////////////////
#endif // CORE_ENABLE_LANGUAGE

#ifndef CORE_LANG_NAME
    #define CORE_LANG_NAME "Unknown"
#endif

#ifndef CORE_LANG_STDC_NAME
    #define CORE_LANG_STDC_NAME "Unknown"
#endif

#ifndef CORE_LANG_STDCPP_NAME
    #define CORE_LANG_STDCPP_NAME "Unknown"
#endif
// Language end ///////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_LIBRARY
// Library std ////////////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
#include <exception>
#endif

// Library std libc++ ////////////////////////////////////////////////////////////
#define CORE_LIB_STD_CXX_NAME "libc++"
#if defined(_LIBCPP_VERSION)
    #define CORE_LIB_STD_CXX CORE_PREDEF_MAKE_10_VVPPP(_LIBCPP_VERSION)
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_CXX_NAME
#endif
// Library std libc++ end ////////////////////////////////////////////////////////


// Library std dinkum ware ///////////////////////////////////////////////////////
#define CORE_LIB_STD_DINKUMWARE_NAME "Dinkumware"
#if (defined(_YVALS) && !defined(__IBMCPP__)) || defined(_CPPLIB_VER)
    #if defined(_CPPLIB_VER)
        #define CORE_LIB_STD_DINKUMWARE CORE_PREDEF_MAKE_10_VVRR(_CPPLIB_VER)
    #else
        #define CORE_LIB_STD_DINKUMWARE CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_DINKUMWARE_NAME
#endif
// Library std dinkum ware end ///////////////////////////////////////////////////


// Library std libcomo ///////////////////////////////////////////////////////////
#define CORE_LIB_STD_COMO_NAME "Comeau Computing"
#if defined(__LIBCOMO__)
    #define CORE_LIB_STD_COMO CORE_VERSION_NUMBER(__LIBCOMO_VERSION__,0,0)
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_COMO_NAME
#endif
// Library std libcomo end ///////////////////////////////////////////////////////


// Library std modena ////////////////////////////////////////////////////////////
#define CORE_LIB_STD_MSIPL_NAME "Modena Software Lib++"
#if defined(MSIPL_COMPILE_H) || defined(__MSIPL_COMPILE_H)
    #define CORE_LIB_STD_MSIPL CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_MSIPL_NAME
#endif
// Library std modena end ////////////////////////////////////////////////////////


// Library std msl ///////////////////////////////////////////////////////////////
#define CORE_LIB_STD_MSL_NAME "Metrowerks"
#if defined(__MSL_CPP__) || defined(__MSL__)
    #if defined(__MSL_CPP__)
        #define CORE_LIB_STD_MSL CORE_PREDEF_MAKE_0X_VRPP(__MSL_CPP__)
    #else
        #define CORE_LIB_STD_MSL CORE_PREDEF_MAKE_0X_VRPP(__MSL__)
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_MSL_NAME
#endif
// Library std msl end ///////////////////////////////////////////////////////////


// Library std msvc //////////////////////////////////////////////////////////////
#define CORE_LIB_STD_MSVC_NAME "Microsoft stdlib"
#if defined(_MSVC_STL_VERSION)
    #define CORE_LIB_STD_MSVC CORE_PREDEF_MAKE_10_VVR_0PPPPP(_MSVC_STL_VERSION, _MSVC_STL_UPDATE)
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_MSVC_NAME
#endif
// Library std msvc end //////////////////////////////////////////////////////////


// Library std roguewave /////////////////////////////////////////////////////////
#define CORE_LIB_STD_RW_NAME "Roguewave"
#if defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)
    #if defined(_RWSTD_VER)
        #if _RWSTD_VER < 0x010000
            #define CORE_LIB_STD_RW CORE_PREDEF_MAKE_0X_VVRRP(_RWSTD_VER)
        #else
            #define CORE_LIB_STD_RW CORE_PREDEF_MAKE_0X_VVRRPP(_RWSTD_VER)
        #endif
    #else
        #define CORE_LIB_STD_RW CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_RW_NAME
#endif
// Library std roguewave end /////////////////////////////////////////////////////


// Library std sgi ///////////////////////////////////////////////////////////////
#define CORE_LIB_STD_SGI_NAME "SGI"
#if defined(__STL_CONFIG_H)
    #if defined(__SGI_STL)
        #define CORE_LIB_STD_SGI CORE_PREDEF_MAKE_0X_VRP(__SGI_STL)
    #else
        #define CORE_LIB_STD_SGI CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_SGI_NAME
#endif
// Library std sgi end ///////////////////////////////////////////////////////////


// Library std sgi ///////////////////////////////////////////////////////////////
#define CORE_LIB_STD_GNU_NAME "GNU"
#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
    #if defined(__GLIBCXX__)
        #define CORE_LIB_STD_GNU CORE_PREDEF_MAKE_YYYYMMDD(__GLIBCXX__)
    #else
        #define CORE_LIB_STD_GNU CORE_PREDEF_MAKE_YYYYMMDD(__GLIBCPP__)
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_GNU_NAME
#endif
// Library std sgi end ///////////////////////////////////////////////////////////


// Library std port //////////////////////////////////////////////////////////////
#define CORE_LIB_STD_STLPORT_NAME "STLport"
#if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
    #if !defined(CORE_LIB_STD_STLPORT) && defined(_STLPORT_MAJOR)
        #define CORE_LIB_STD_STLPORT CORE_VERSION_NUMBER(_STLPORT_MAJOR,_STLPORT_MINOR,_STLPORT_PATCHLEVEL)
    #endif
    #if !defined(CORE_LIB_STD_STLPORT) && defined(_STLPORT_VERSION)
        #define CORE_LIB_STD_STLPORT CORE_PREDEF_MAKE_0X_VRP(_STLPORT_VERSION)
    #endif
    #if !defined(CORE_LIB_STD_STLPORT)
        #define CORE_LIB_STD_STLPORT CORE_PREDEF_MAKE_0X_VRP(__SGI_STL_PORT)
    #endif
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_STLPORT_NAME
#endif
// Library std port end //////////////////////////////////////////////////////////


// Library std port //////////////////////////////////////////////////////////////
#define CORE_LIB_STD_IBM_NAME "IBM VACPP"
#if defined(__IBMCPP__)
    #define CORE_LIB_STD_IBM CORE_VERSION_NUMBER_AVAILABLE
    #undef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME CORE_LIB_STD_IBM_NAME
#endif
// Library std std port end //////////////////////////////////////////////////////


// Default CORE_LIB_STD_NAME value
#ifndef CORE_LIB_STD_NAME
    #define CORE_LIB_STD_NAME "Unknown"
#endif
// Library std end ////////////////////////////////////////////////////////////////////////////////


// Library c //////////////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
#include <cassert>
#else
#include <assert.h>
#endif

// Library c cloudabi ////////////////////////////////////////////////////////////
#if defined(__CloudABI__)
#include <stddef.h>
#endif

#define CORE_LIB_C_CLOUDABI_NAME "cloudlibc"
#if defined(__cloudlibc__)
    #define CORE_LIB_C_CLOUDABI CORE_VERSION_NUMBER(__cloudlibc_major__,__cloudlibc_minor__,0)
    #undef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME CORE_LIB_C_CLOUDABI_NAME
#endif
// Language c cloudabi end ///////////////////////////////////////////////////////


// Library c gnu /////////////////////////////////////////////////////////////////
#if defined(__STDC__)
#include <stddef.h>
#elif defined(__cplusplus)
#include <cstddef>
#endif

#define CORE_LIB_C_GNU_NAME "GNU"
#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
    #if defined(__GLIBC__)
        #define CORE_LIB_C_GNU CORE_VERSION_NUMBER(__GLIBC__,__GLIBC_MINOR__,0)
    #else
        #define CORE_LIB_C_GNU CORE_VERSION_NUMBER(__GNU_LIBRARY__,__GNU_LIBRARY_MINOR__,0)
    #endif
    #undef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME CORE_LIB_C_GNU_NAME
#endif
// Language c gnu end ////////////////////////////////////////////////////////////


// Library c uc //////////////////////////////////////////////////////////////////
#define CORE_LIB_C_UC_NAME "uClibc"
#if defined(__UCLIBC__)
#   define CORE_LIB_C_UC CORE_VERSION_NUMBER(__UCLIBC_MAJOR__,__UCLIBC_MINOR__,__UCLIBC_SUBLEVEL__)
    #undef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME CORE_LIB_C_UC_NAME
#endif
// Language c uc end /////////////////////////////////////////////////////////////


// Library c uc //////////////////////////////////////////////////////////////////
#define CORE_LIB_C_VMS_NAME "VMS"
#if defined(__CRTL_VER)
    #define CORE_LIB_C_VMS CORE_PREDEF_MAKE_10_VVRR0PP00(__CRTL_VER)
    #undef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME CORE_LIB_C_VMS_NAME
#endif
// Language c uc end /////////////////////////////////////////////////////////////


// Library c zos /////////////////////////////////////////////////////////////////
#define CORE_LIB_C_ZOS_NAME "z/OS"
#if defined(__LIBREL__)
    #if !defined(CORE_LIB_C_ZOS) && defined(__LIBREL__)
        #define CORE_LIB_C_ZOS CORE_PREDEF_MAKE_0X_VRRPPPP(__LIBREL__)
    #endif
    #if !defined(CORE_LIB_C_ZOS) && defined(__TARGET_LIB__)
        #define CORE_LIB_C_ZOS CORE_PREDEF_MAKE_0X_VRRPPPP(__TARGET_LIB__)
    #endif
    #if !defined(CORE_LIB_C_ZOS)
        #define CORE_LIB_C_ZOS CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #undef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME CORE_LIB_C_ZOS_NAME
#endif
// Language c zos end ////////////////////////////////////////////////////////////
#endif // CORE_ENABLE_LIBRARY

// Default CORE_LIB_C_NAME value
#ifndef CORE_LIB_C_NAME
    #define CORE_LIB_C_NAME "Unknown"
#endif
// Library c end //////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_PLATFORM
// OS /////////////////////////////////////////////////////////////////////////////////////////////
// OS aix ////////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(_AIX) || defined(__TOS_AIX__))
    #if !defined(CORE_OS_AIX) && defined(_AIX43)
        #define CORE_OS_AIX CORE_VERSION_NUMBER(4,3,0)
    #endif
    #if !defined(CORE_OS_AIX) && defined(_AIX41)
        #define CORE_OS_AIX CORE_VERSION_NUMBER(4,1,0)
    #endif
    #if !defined(CORE_OS_AIX) && defined(_AIX32)
        #define CORE_OS_AIX CORE_VERSION_NUMBER(3,2,0)
    #endif
    #if !defined(CORE_OS_AIX) && defined(_AIX3)
        #define CORE_OS_AIX CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_OS_AIX)
        #define CORE_OS_AIX CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_AIX_NAME "IBM AIX"
#ifdef CORE_OS_AIX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_AIX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS aix end ////////////////////////////////////////////////////////////////////


// OS amigaos ////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(AMIGA) || defined(__amigaos__))
    #define CORE_OS_AMIGAOS CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_AMIGAOS_NAME "AmigaOS"
#ifdef CORE_OS_AMIGAOS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_AMIGAOS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS amigaos end ////////////////////////////////////////////////////////////////


// OS beos ///////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__BEOS__))
    #define CORE_OS_BEOS CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_BEOS_NAME "BeOS"
#ifdef CORE_OS_BEOS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BEOS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS beos end ///////////////////////////////////////////////////////////////////


// OS IOS ////////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__APPLE__) && defined(__MACH__) && defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__))
    #define CORE_OS_IOS (__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__*1000)
#endif

#define CORE_OS_IOS_NAME "IOS"
#ifdef CORE_OS_IOS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_IOS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS IOS end ////////////////////////////////////////////////////////////////////


// OS macos //////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__)))
    #if !defined(CORE_OS_MACOS) && defined(__APPLE__) && defined(__MACH__)
        #define CORE_OS_MACOS CORE_VERSION_NUMBER(10,0,0)
    #endif
    #if !defined(CORE_OS_MACOS)
        #define CORE_OS_MACOS CORE_VERSION_NUMBER(9,0,0)
    #endif
#endif

#define CORE_OS_MACOS_NAME "Mac OS"
#ifdef CORE_OS_MACOS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_MACOS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS macos end //////////////////////////////////////////////////////////////////



// OS BSD ////////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(BSD) || defined(_SYSTYPE_BSD))
    #include <sys/param.h>
    #if !defined(CORE_OS_BSD) && defined(BSD4_4)
        #define CORE_OS_BSD CORE_VERSION_NUMBER(4,4,0)
    #endif
    #if !defined(CORE_OS_BSD) && defined(BSD4_3)
        #define CORE_OS_BSD CORE_VERSION_NUMBER(4,3,0)
    #endif
    #if !defined(CORE_OS_BSD) && defined(BSD4_2)
        #define CORE_OS_BSD CORE_VERSION_NUMBER(4,2,0)
    #endif
    #if !defined(CORE_OS_BSD) && defined(BSD)
        #define CORE_OS_BSD CORE_PREDEF_MAKE_10_VVRR(BSD)
    #endif
    #if !defined(CORE_OS_BSD)
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_BSD_NAME "BSD"
#ifdef CORE_OS_BSD
    #undef CORE_OS_NAME
    #define CORE_OS_BSD_AVAILABLE
    #define CORE_OS_NAME CORE_OS_BSD_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSD end ////////////////////////////////////////////////////////////////////


// OS BSDi ///////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__bsdi__))
    #ifndef CORE_OS_BSD_AVAILABLE
        #undef CORE_OS_BSD
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
        #define CORE_OS_BSD_AVAILABLE
    #endif
    #undef CORE_OS_BSD_BSDI
    #define CORE_OS_BSD_BSDI CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_BSD_BSDI_NAME "BSDi BSD/OS"
#ifdef CORE_OS_BSD_BSDI
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BSD_BSDI_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSDi end ///////////////////////////////////////////////////////////////////


// OS BSD dragonfly //////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__DragonFly__))
    #ifndef CORE_OS_BSD_AVAILABLE
        #undef CORE_OS_BSD
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
        #define CORE_OS_BSD_AVAILABLE
    #endif
    #undef CORE_OS_BSD_DRAGONFLY
    #if defined(__DragonFly__)
        #define CORE_OS_DRAGONFLY_BSD CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_BSD_DRAGONFLY_NAME "DragonFly BSD"
#ifdef CORE_OS_BSD_DRAGONFLY
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BSD_DRAGONFLY_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSD dragonfly end //////////////////////////////////////////////////////////


// OS BSD free ///////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__FreeBSD__))
    #ifndef CORE_OS_BSD_AVAILABLE
        #undef CORE_OS_BSD
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
        #define CORE_OS_BSD_AVAILABLE
    #endif
    #undef CORE_OS_BSD_FREE
    #include <sys/param.h>
    #if defined(__FreeBSD_version)
        #if __FreeBSD_version == 491000
            #define CORE_OS_BSD_FREE CORE_VERSION_NUMBER(4, 10, 0)
        #elif __FreeBSD_version == 492000
            #define CORE_OS_BSD_FREE CORE_VERSION_NUMBER(4, 11, 0)
        #elif __FreeBSD_version < 500000
            #define CORE_OS_BSD_FREE CORE_PREDEF_MAKE_10_VRPPPP(__FreeBSD_version)
        #else
            #define CORE_OS_BSD_FREE CORE_PREDEF_MAKE_10_VVRRPPP(__FreeBSD_version)
        #endif
    #else
        #define CORE_OS_BSD_FREE CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_BSD_FREE_NAME "Free BSD"
#ifdef CORE_OS_BSD_FREE
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BSD_FREE_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSD free end ///////////////////////////////////////////////////////////////


// OS BSD net ////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__NETBSD__) || defined(__NetBSD__))
    #ifndef CORE_OS_BSD_AVAILABLE
        #undef CORE_OS_BSD
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
        #define CORE_OS_BSD_AVAILABLE
    #endif
    #undef CORE_OS_BSD_NET
    #if defined(__NETBSD__)
        #if defined(__NETBSD_version)
            #if __NETBSD_version < 500000
                #define CORE_OS_BSD_NET CORE_PREDEF_MAKE_10_VRP000(__NETBSD_version)
            #else
                #define CORE_OS_BSD_NET CORE_PREDEF_MAKE_10_VRR000(__NETBSD_version)
            #endif
        #else
            #define CORE_OS_BSD_NET CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #elif defined(__NetBSD__)
        #if !defined(CORE_OS_BSD_NET) && defined(NetBSD0_8)
            #define CORE_OS_BSD_NET CORE_VERSION_NUMBER(0,8,0)
        #endif
        #if !defined(CORE_OS_BSD_NET) && defined(NetBSD0_9)
            #define CORE_OS_BSD_NET CORE_VERSION_NUMBER(0,9,0)
        #endif
        #if !defined(CORE_OS_BSD_NET) && defined(NetBSD1_0)
            #define CORE_OS_BSD_NET CORE_VERSION_NUMBER(1,0,0)
        #endif
        #if !defined(CORE_OS_BSD_NET) && defined(__NetBSD_Version)
            #define CORE_OS_BSD_NET CORE_PREDEF_MAKE_10_VVRR00PP00(__NetBSD_Version)
        #endif
        #if !defined(CORE_OS_BSD_NET)
            #define CORE_OS_BSD_NET CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #endif
#endif

#define CORE_OS_BSD_NET_NAME "NetBSD"
#ifdef CORE_OS_BSD_NET
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BSD_NET_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSD net end ////////////////////////////////////////////////////////////////


// OS BSD open ///////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__OpenBSD__))
    #ifndef CORE_OS_BSD_AVAILABLE
        #undef CORE_OS_BSD
        #define CORE_OS_BSD CORE_VERSION_NUMBER_AVAILABLE
        #define CORE_OS_BSD_AVAILABLE
    #endif
    #undef CORE_OS_BSD_OPEN
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_0)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,0,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_1)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,1,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_2)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,2,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_3)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,3,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_4)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,4,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_5)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,5,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_6)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,6,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_7)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,7,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_8)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,8,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD2_9)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(2,9,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_0)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,0,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_1)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,1,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_2)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,2,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_3)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,3,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_4)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,4,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_5)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,5,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_6)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,6,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_7)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,7,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_8)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,8,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD3_9)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(3,9,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_0)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_1)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,1,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_2)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,2,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_3)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,3,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_4)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,4,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_5)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,5,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_6)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,6,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_7)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,7,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_8)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,8,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD4_9)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(4,9,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_0)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,0,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_1)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,1,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_2)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,2,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_3)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,3,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_4)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,4,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_5)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,5,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_6)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,6,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_7)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,7,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_8)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,8,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD5_9)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(5,9,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_0)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,0,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_1)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,1,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_2)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,2,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_3)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,3,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_4)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,4,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_5)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,5,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_6)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,6,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_7)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,7,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_8)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,8,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN) && defined(OpenBSD6_9)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER(6,9,0)
    #endif
    #if !defined(CORE_OS_BSD_OPEN)
        #define CORE_OS_BSD_OPEN CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_BSD_OPEN_NAME  "OpenBSD"
#ifdef CORE_OS_BSD_OPEN
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_BSD_OPEN_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS BSD open end ///////////////////////////////////////////////////////////////


// OS cygwin /////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__CYGWIN__))
    #include <cygwin/version.h>
    #define CORE_OS_CYGWIN CORE_VERSION_NUMBER(CYGWIN_VERSION_API_MAJOR, CYGWIN_VERSION_API_MINOR, 0)
#endif

#define CORE_OS_CYGWIN_NAME "Cygwin"
#ifdef CORE_OS_CYGWIN
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_CYGWIN_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS cygwin end /////////////////////////////////////////////////////////////////


// OS haiku //////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__HAIKU__))
    #define CORE_OS_HAIKU CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_HAIKU_NAME "Haiku"
#ifdef CORE_OS_HAIKU
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_HAIKU_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS haiku end //////////////////////////////////////////////////////////////////


// OS hpux ///////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(hpux) || defined(_hpux) || defined(__hpux))
    #define CORE_OS_HPUX CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_HPUX_NAME "HP_UX"
#ifdef CORE_OS_HPUX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_HPUX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS hpux end ///////////////////////////////////////////////////////////////////


// OS irix ///////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(sgi) || defined(__sgi))
    #define CORE_OS_IRIX CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_IRIX_NAME "IRIX"
#ifdef CORE_OS_IRIX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_IRIX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS irix end ///////////////////////////////////////////////////////////////////


// OS irix ///////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(sgi) || defined(__sgi))
    #define CORE_OS_IRIX CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_IRIX_NAME "IRIX"
#ifdef CORE_OS_IRIX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_IRIX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS irix end ///////////////////////////////////////////////////////////////////


// OS unix ///////////////////////////////////////////////////////////////////////
#if defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
    #define CORE_OS_UNIX CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_UNIX_NAME "Unix Environment"
#ifdef CORE_OS_UNIX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_UNIX_NAME
#endif
// OS unix end ///////////////////////////////////////////////////////////////////


// OS android ////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__ANDROID__))
    #define CORE_OS_ANDROID CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_ANDROID_NAME "Android"
#ifdef CORE_OS_ANDROID
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_ANDROID_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS android end ////////////////////////////////////////////////////////////////


// OS linux //////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__))
    #define CORE_OS_LINUX CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_LINUX_NAME "Linux"
#ifdef CORE_OS_LINUX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_LINUX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS linux end //////////////////////////////////////////////////////////////////


// OS os400 //////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__OS400__))
    #define CORE_OS_OS400 CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_OS400_NAME "IBM OS/400"
#ifdef CORE_OS_OS400
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_OS400_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS os400 end //////////////////////////////////////////////////////////////////


// OS qnxnto /////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(__QNX__) || defined(__QNXNTO__))
    #if !defined(CORE_OS_QNX) && defined(_NTO_VERSION)
        #define CORE_OS_QNX CORE_PREDEF_MAKE_10_VVRR(_NTO_VERSION)
    #endif
    #if !defined(CORE_OS_QNX) && defined(__QNX__)
        #define CORE_OS_QNX CORE_VERSION_NUMBER(4,0,0)
    #endif
    #if !defined(CORE_OS_QNX)
        #define CORE_OS_QNX CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_QNX_NAME "QNX"
#ifdef CORE_OS_QNX
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_QNX_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS qnxnto end /////////////////////////////////////////////////////////////////


// OS solaris ////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(sun) || defined(__sun))
    #define CORE_OS_SOLARIS CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_SOLARIS_NAME "Solaris"
#ifdef CORE_OS_SOLARIS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_SOLARIS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS solaris end ////////////////////////////////////////////////////////////////


// OS vms ////////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(VMS) || defined(__VMS))
    #if defined(__VMS_VER)
        #define CORE_OS_VMS CORE_PREDEF_MAKE_10_VVRR00PP00(__VMS_VER)
    #else
        #define CORE_OS_VMS CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_OS_VMS_NAME "VMS"
#ifdef CORE_OS_VMS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_VMS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS vms end ////////////////////////////////////////////////////////////////////


// OS windows ////////////////////////////////////////////////////////////////////
#if !defined(CORE_PREDEF_DETAIL_OS_DETECTED) && (defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__))
    #define CORE_OS_WINDOWS CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_OS_WINDOWS_NAME "Microsoft Windows"
#ifdef CORE_OS_WINDOWS
    #undef CORE_OS_NAME
    #define CORE_OS_NAME CORE_OS_WINDOWS_NAME
    #ifndef CORE_PREDEF_DETAIL_OS_DETECTED
    #define CORE_PREDEF_DETAIL_OS_DETECTED 1
    #endif
#endif
// OS windows end ////////////////////////////////////////////////////////////////
// OS end /////////////////////////////////////////////////////////////////////////////////////////


// Platform ///////////////////////////////////////////////////////////////////////////////////////
// Platform Web //////////////////////////////////////////////////////////////////
#if defined(__wasm__) || defined(__wasm32__) || defined(__wasm64__)
    #define CORE_PLAT_WEB CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WEB_NAME "Web"
#ifdef CORE_PLAT_WEB
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WEB_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform Web //////////////////////////////////////////////////////////////////


// Platform android //////////////////////////////////////////////////////////////
#if defined(__ANDROID__)
    #define CORE_PLAT_ANDROID CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_ANDROID_NAME "Android"
#ifdef CORE_PLAT_ANDROID
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_ANDROID_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform android end //////////////////////////////////////////////////////////


// Platform cloudabi /////////////////////////////////////////////////////////////
#if defined(__CloudABI__)
    #define CORE_PLAT_CLOUDABI CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_CLOUDABI_NAME "CloudABI"
#ifdef CORE_PLAT_CLOUDABI
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_CLOUDABI_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform cloudabi end /////////////////////////////////////////////////////////


// Platform IOS //////////////////////////////////////////////////////////////////
#ifdef CORE_OS_IOS
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_SIMULATOR) && (TARGET_OS_SIMULATOR == 1)
        #define CORE_PLAT_IOS_SIMULATOR CORE_VERSION_NUMBER_AVAILABLE
    #elif defined(TARGET_IPHONE_SIMULATOR) && (TARGET_IPHONE_SIMULATOR == 1)
        #define CORE_PLAT_IOS_SIMULATOR CORE_VERSION_NUMBER_AVAILABLE
    #else
        #define CORE_PLAT_IOS_DEVICE CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_IOS_SIMULATOR_NAME "IOS Simulator"
#define CORE_PLAT_IOS_DEVICE_NAME "IOS Device"
#ifdef CORE_PLAT_IOS_SIMULATOR
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_IOS_SIMULATOR_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif

#ifdef CORE_PLAT_IOS_DEVICE
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_IOS_DEVICE_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform IOS end //////////////////////////////////////////////////////////////


// Platform MinGW ////////////////////////////////////////////////////////////////
#if defined(__MINGW32__) || defined(__MINGW64__)
    #include <_mingw.h>
    #if !defined(CORE_PLAT_MINGW_DETECTION) && (defined(__MINGW64_VERSION_MAJOR) && defined(__MINGW64_VERSION_MINOR))
        #define CORE_PLAT_MINGW_DETECTION CORE_VERSION_NUMBER(__MINGW64_VERSION_MAJOR,__MINGW64_VERSION_MINOR,0)
    #endif
    #if !defined(CORE_PLAT_MINGW_DETECTION) && (defined(__MINGW32_VERSION_MAJOR) && defined(__MINGW32_VERSION_MINOR))
        #define CORE_PLAT_MINGW_DETECTION CORE_VERSION_NUMBER(__MINGW32_MAJOR_VERSION,__MINGW32_MINOR_VERSION,0)
    #endif
    #if !defined(CORE_PLAT_MINGW_DETECTION)
        #define CORE_PLAT_MINGW_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_MINGW_NAME "MinGW (any variety)"
#ifdef CORE_PLAT_MINGW_DETECTION
    #if defined(CORE_PREDEF_DETAIL_PLAT_DETECTED)
        #define CORE_PLAT_MINGW_EMULATED CORE_PLAT_MINGW_DETECTION
    #else
        #undef CORE_PLAT_NAME
        #define CORE_PLAT_NAME CORE_PLAT_MINGW_NAME
        #define CORE_PLAT_MINGW CORE_PLAT_MINGW_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform MinGW end ////////////////////////////////////////////////////////////


// Platform MinGW32 //////////////////////////////////////////////////////////////
#if defined(__MINGW32__)
    #include <_mingw.h>
    #if !defined(CORE_PLAT_MINGW32_DETECTION) && (defined(__MINGW32_VERSION_MAJOR) && defined(__MINGW32_VERSION_MINOR))
        #define CORE_PLAT_MINGW32_DETECTION CORE_VERSION_NUMBER(__MINGW32_VERSION_MAJOR,__MINGW32_VERSION_MINOR,0)
    #endif
    #if !defined(CORE_PLAT_MINGW32_DETECTION)
        #define CORE_PLAT_MINGW32_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_MINGW32_NAME "MinGW"
#ifdef CORE_PLAT_MINGW32_DETECTION
    #if defined(CORE_PREDEF_DETAIL_PLAT_DETECTED)
        #define CORE_PLAT_MINGW32_EMULATED CORE_PLAT_MINGW32_DETECTION
    #else
        #undef CORE_PLAT_NAME
        #define CORE_PLAT_NAME CORE_PLAT_MINGW32_NAME
        #define CORE_PLAT_MINGW32 CORE_PLAT_MINGW32_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform MinGW32 end //////////////////////////////////////////////////////////


// Platform MinGW64 //////////////////////////////////////////////////////////////
#if defined(__MINGW64__)
    #include <_mingw.h>
    #if !defined(CORE_PLAT_MINGW64_DETECTION) && (defined(__MINGW64_VERSION_MAJOR) && defined(__MINGW64_VERSION_MINOR))
        #define CORE_PLAT_MINGW64_DETECTION CORE_VERSION_NUMBER(__MINGW64_VERSION_MAJOR,__MINGW64_VERSION_MINOR,0)
    #endif
    #if !defined(CORE_PLAT_MINGW64_DETECTION)
        #define CORE_PLAT_MINGW64_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_MINGW64_NAME "MinGW-w64"
#ifdef CORE_PLAT_MINGW64_DETECTION
    #if defined(CORE_PREDEF_DETAIL_PLAT_DETECTED)
        #define CORE_PLAT_MINGW64_EMULATED CORE_PLAT_MINGW64_DETECTION
    #else
        #undef CORE_PLAT_NAME
        #define CORE_PLAT_NAME CORE_PLAT_MINGW64_NAME
        #define CORE_PLAT_MINGW64 CORE_PLAT_MINGW64_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform MinGW64 end //////////////////////////////////////////////////////////


// Platform MinGW64 //////////////////////////////////////////////////////////////
#if defined(__MINGW64__)
    #include <_mingw.h>
    #if !defined(CORE_PLAT_MINGW64_DETECTION) && (defined(__MINGW64_VERSION_MAJOR) && defined(__MINGW64_VERSION_MINOR))
        #define CORE_PLAT_MINGW64_DETECTION CORE_VERSION_NUMBER(__MINGW64_VERSION_MAJOR,__MINGW64_VERSION_MINOR,0)
    #endif
    #if !defined(CORE_PLAT_MINGW64_DETECTION)
        #define CORE_PLAT_MINGW64_DETECTION CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_MINGW64_NAME "MinGW-w64"
#ifdef CORE_PLAT_MINGW64_DETECTION
    #if defined(CORE_PREDEF_DETAIL_PLAT_DETECTED)
        #define CORE_PLAT_MINGW64_EMULATED CORE_PLAT_MINGW64_DETECTION
    #else
        #undef CORE_PLAT_NAME
        #define CORE_PLAT_NAME CORE_PLAT_MINGW64_NAME
        #define CORE_PLAT_MINGW64 CORE_PLAT_MINGW64_DETECTION
    #endif
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform MinGW64 end //////////////////////////////////////////////////////////


// Platform windows uwp //////////////////////////////////////////////////////////
#ifdef CORE_OS_WINDOWS
    //  MinGW (32-bit), WinCE, and wineg++ don't have a ntverp.h header
    #if !defined(__MINGW32__) && !defined(_WIN32_WCE) && !defined(__WINE__)
        #include <ntverp.h>
        #define CORE_PLAT_WINDOWS_SDK_VERSION CORE_VERSION_NUMBER(0, 0, VER_PRODUCTBUILD)
    #endif

    // 9200 is Windows SDK 8.0 from ntverp.h which introduced family support
    #if ((CORE_PLAT_WINDOWS_SDK_VERSION >= CORE_VERSION_NUMBER(0, 0, 9200)) || (defined(__MINGW64__) && __MINGW64_VERSION_MAJOR >= 3))
        #define CORE_PLAT_WINDOWS_UWP CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

#define CORE_PLAT_WINDOWS_UWP_NAME "Universal Windows Platform"
#ifdef CORE_PLAT_WINDOWS_UWP
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_UWP_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
    #include <winapifamily.h>    // Windows SDK
#endif
// Platform windows uwp end //////////////////////////////////////////////////////


// Platform windows desktop //////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && ((defined(WINAPI_FAMILY_DESKTOP_APP) && WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) || !CORE_PLAT_WINDOWS_UWP)
    #define CORE_PLAT_WINDOWS_DESKTOP CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_DESKTOP_NAME "Windows Desktop"
#ifdef CORE_PLAT_WINDOWS_DESKTOP
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_DESKTOP_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows desktop end //////////////////////////////////////////////////


// Platform windows phone ////////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    #define CORE_PLAT_WINDOWS_PHONE CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_PHONE_NAME "Windows Phone"
#ifdef CORE_PLAT_WINDOWS_PHONE
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_PHONE_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows phone end ////////////////////////////////////////////////////


// Platform windows store ////////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && ((defined(WINAPI_FAMILY_PC_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PC_APP) || (defined(WINAPI_FAMILY_APP)    && WINAPI_FAMILY == WINAPI_FAMILY_APP))
    #define CORE_PLAT_WINDOWS_STORE CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_STORE_NAME "Windows Store"
#ifdef CORE_PLAT_WINDOWS_STORE
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_STORE_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows store end ////////////////////////////////////////////////////


// Platform windows runtime //////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && (CORE_PLAT_WINDOWS_STORE || CORE_PLAT_WINDOWS_PHONE)
    #define CORE_PLAT_WINDOWS_RUNTIME CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_RUNTIME_NAME "Windows Runtime"
#ifdef CORE_PLAT_WINDOWS_RUNTIME
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_RUNTIME_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows runtime end //////////////////////////////////////////////////


// Platform windows server ///////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && defined(WINAPI_FAMILY_SERVER) && WINAPI_FAMILY == WINAPI_FAMILY_SERVER
    #define CORE_PLAT_WINDOWS_SERVER CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_SERVER_NAME "Windows Server"
#ifdef CORE_PLAT_WINDOWS_RUNTIME
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_SERVER_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows server end ///////////////////////////////////////////////////


// Platform windows system ///////////////////////////////////////////////////////
#if defined(CORE_OS_WINDOWS) && defined(WINAPI_FAMILY_SYSTEM) && WINAPI_FAMILY == WINAPI_FAMILY_SYSTEM
    #define CORE_PLAT_WINDOWS_SYSTEM CORE_VERSION_NUMBER_AVAILABLE
#endif

#define CORE_PLAT_WINDOWS_SYSTEM_NAME "Windows Drivers and Tools"
#ifdef CORE_PLAT_WINDOWS_SYSTEM
    #undef CORE_PLAT_NAME
    #define CORE_PLAT_NAME CORE_PLAT_WINDOWS_SYSTEM_NAME
    #ifndef CORE_PREDEF_DETAIL_PLAT_DETECTED
    #define CORE_PREDEF_DETAIL_PLAT_DETECTED 1
    #endif
#endif
// Platform windows system end ///////////////////////////////////////////////////
#endif // CORE_ENABLE_PLATFORM

// Default CORE_OS_NAME value
#ifndef CORE_OS_NAME
    #define CORE_OS_NAME "Unknown"
#endif

// Default CORE_PLAT_NAME value
#ifndef CORE_PLAT_NAME
    #define CORE_PLAT_NAME "Unknown"
#endif
// Platform end ///////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_OTHER
// Other //////////////////////////////////////////////////////////////////////////////////////////
// Other endian //////////////////////////////////////////////////////////////////
#if !CORE_ENDIAN_BIG_BYTE && !CORE_ENDIAN_BIG_WORD && !CORE_ENDIAN_LITTLE_BYTE && !CORE_ENDIAN_LITTLE_WORD
    #if CORE_LIB_C_GNU || CORE_PLAT_ANDROID || CORE_OS_BSD_OPEN
        #include <endian.h>
    #else
        #if CORE_OS_MACOS
            #include <machine/endian.h>
        #else
            #if CORE_OS_BSD
                #include <sys/endian.h>
            #endif
        #endif
    #endif
    #if defined(__BYTE_ORDER)
        #if defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
            #define CORE_ENDIAN_BIG_BYTE CORE_VERSION_NUMBER_AVAILABLE
        #endif
        #if defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
            #define CORE_ENDIAN_LITTLE_BYTE CORE_VERSION_NUMBER_AVAILABLE
        #endif
        #if defined(__PDP_ENDIAN) && (__BYTE_ORDER == __PDP_ENDIAN)
            #define CORE_ENDIAN_LITTLE_WORD CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #endif
    #if !defined(__BYTE_ORDER) && defined(_BYTE_ORDER)
        #if defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN)
            #define CORE_ENDIAN_BIG_BYTE CORE_VERSION_NUMBER_AVAILABLE
        #endif
        #if defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
            #define CORE_ENDIAN_LITTLE_BYTE CORE_VERSION_NUMBER_AVAILABLE
        #endif
        #if defined(_PDP_ENDIAN) && (_BYTE_ORDER == _PDP_ENDIAN)
            #define CORE_ENDIAN_LITTLE_WORD CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #endif
#endif

/* Built-in byte-swapped big-endian macros.
 */
#if !CORE_ENDIAN_BIG_BYTE && !CORE_ENDIAN_BIG_WORD && !CORE_ENDIAN_LITTLE_BYTE && !CORE_ENDIAN_LITTLE_WORD
    #if (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || \
       (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || \
        defined(__ARMEB__) || \
        defined(__THUMBEB__) || \
        defined(__AARCH64EB__) || \
        defined(_MIPSEB) || \
        defined(__MIPSEB) || \
        defined(__MIPSEB__)
        #define CORE_ENDIAN_BIG_BYTE CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

/* Built-in byte-swapped little-endian macros.
 */
#if !CORE_ENDIAN_BIG_BYTE && !CORE_ENDIAN_BIG_WORD && !CORE_ENDIAN_LITTLE_BYTE && !CORE_ENDIAN_LITTLE_WORD
    #if (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
       (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) || \
        defined(__ARMEL__) || \
        defined(__THUMBEL__) || \
        defined(__AARCH64EL__) || \
        defined(__loongarch__) || \
        defined(_MIPSEL) || \
        defined(__MIPSEL) || \
        defined(__MIPSEL__) || \
        defined(__riscv) || \
        defined(__e2k__)
        #define CORE_ENDIAN_LITTLE_BYTE CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

/* Some architectures are strictly one endianess (as opposed
 * the current common bi-endianess).
 */
#if !CORE_ENDIAN_BIG_BYTE && !CORE_ENDIAN_BIG_WORD && \
    !CORE_ENDIAN_LITTLE_BYTE && !CORE_ENDIAN_LITTLE_WORD
    #if CORE_ARCH_M68K || \
        CORE_ARCH_PARISC || \
        CORE_ARCH_SPARC || \
        CORE_ARCH_SYS370 || \
        CORE_ARCH_SYS390 || \
        CORE_ARCH_Z
        #define CORE_ENDIAN_BIG_BYTE CORE_VERSION_NUMBER_AVAILABLE
    #endif
    #if CORE_ARCH_IA64 || \
        CORE_ARCH_X86 || \
        CORE_ARCH_BLACKFIN
        #define CORE_ENDIAN_LITTLE_BYTE CORE_VERSION_NUMBER_AVAILABLE
    #endif
#endif

/* Windows on ARM, if not otherwise detected/specified, is always
 * byte-swapped little-endian.
 */
#if !CORE_ENDIAN_BIG_BYTE && !CORE_ENDIAN_BIG_WORD && \
    !CORE_ENDIAN_LITTLE_BYTE && !CORE_ENDIAN_LITTLE_WORD
    #ifdef CORE_ARCH_ARM
        #include <boost/predef/os/windows.h>
        #ifdef CORE_OS_WINDOWS
            #define CORE_ENDIAN_LITTLE_BYTE CORE_VERSION_NUMBER_AVAILABLE
        #endif
    #endif
#endif


#define CORE_ENDIAN_BIG_BYTE_NAME "Byte-Swapped Big-Endian"
#define CORE_ENDIAN_BIG_WORD_NAME "Word-Swapped Big-Endian"
#define CORE_ENDIAN_LITTLE_BYTE_NAME "Byte-Swapped Little-Endian"
#define CORE_ENDIAN_LITTLE_WORD_NAME "Word-Swapped Little-Endian"
#if CORE_ENDIAN_BIG_BYTE
    #define CORE_ENDIAN_NAME CORE_ENDIAN_BIG_BYTE_NAME
#endif
#if CORE_ENDIAN_BIG_WORD
    #define CORE_ENDIAN_NAME CORE_ENDIAN_BIG_WORD_NAME
#endif
#if CORE_ENDIAN_LITTLE_BYTE
    #define CORE_ENDIAN_NAME CORE_ENDIAN_LITTLE_BYTE_NAME
#endif
#if CORE_ENDIAN_LITTLE_WORD
    #define CORE_ENDIAN_NAME CORE_ENDIAN_LITTLE_WORD_NAME
#endif
// Other endian end //////////////////////////////////////////////////////////////


// Other wordsize ////////////////////////////////////////////////////////////////
#define CORE_ARCH_WORD_BITS_0_NAME "Word Bits"
#define CORE_ARCH_WORD_BITS_16_NAME "16-bit Word Size"
#define CORE_ARCH_WORD_BITS_32_NAME "32-bit Word Size"
#define CORE_ARCH_WORD_BITS_64_NAME "64-bit Word Size"

#if defined(CORE_ARCH_WORD_BITS_64)
    #define CORE_ARCH_WORD_BITS_NAME CORE_ARCH_WORD_BITS_64_NAME
    #define CORE_ARCH_WORD_BITS 64
#endif
#if defined(CORE_ARCH_WORD_BITS_32)
    #define CORE_ARCH_WORD_BITS_NAME CORE_ARCH_WORD_BITS_32_NAME
    #define CORE_ARCH_WORD_BITS 32
#endif
#if defined(CORE_ARCH_WORD_BITS_16)
    #define CORE_ARCH_WORD_BITS_NAME CORE_ARCH_WORD_BITS_16_NAME
    #define CORE_ARCH_WORD_BITS 16
#endif
#if !defined(CORE_ARCH_WORD_BITS)
    #define CORE_ARCH_WORD_BITS_NAME CORE_ARCH_WORD_BITS_0_NAME
    #define CORE_ARCH_WORD_BITS 0
#endif
// Other wordsize end ////////////////////////////////////////////////////////////
#endif // CORE_ENABLE_OTHER

#ifndef CORE_ENDIAN_NAME
    #define CORE_ENDIAN_NAME "Unknown"
#endif

#ifndef CORE_ARCH_WORD_BITS_NAME
    #define CORE_ARCH_WORD_BITS_NAME "Unknown"
#endif
// Other end //////////////////////////////////////////////////////////////////////////////////////


#if CORE_ENABLE_HARDWARE
// Hardware SIMD //////////////////////////////////////////////////////////////////////////////////
// Hardware SIMD ARM /////////////////////////////////////////////////////////////
// The https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_.28NEON.29[NEON] ARM extension version number.
#define CORE_HW_SIMD_ARM_NEON_VERSION CORE_VERSION_NUMBER(1, 0, 0)
#define CORE_HW_SIMD_ARM_NAME "ARM SIMD"
#if !defined(CORE_HW_SIMD_ARM) && (defined(__ARM_NEON__) || defined(__aarch64__) || defined (_M_ARM) || defined (_M_ARM64))
    #define CORE_HW_SIMD_ARM CORE_HW_SIMD_ARM_NEON_VERSION
    #define CORE_HW_SIMD_NAME CORE_HW_SIMD_ARM_NAME
#endif
// Hardware SIMD ARM end /////////////////////////////////////////////////////////


// Hardware SIMD ppc /////////////////////////////////////////////////////////////
#define CORE_HW_SIMD_PPC_NAME "PPC SIMD"
#if !defined(CORE_HW_SIMD_PPC) && defined(__VECTOR4DOUBLE__)
#   define CORE_HW_SIMD_PPC CORE_VERSION_NUMBER(2, 0, 0)
#endif
#if !defined(CORE_HW_SIMD_PPC) && defined(__VSX__)
#   define CORE_HW_SIMD_PPC CORE_VERSION_NUMBER(1, 1, 0)
#endif
#if !defined(CORE_HW_SIMD_PPC) && (defined(__ALTIVEC__) || defined(__VEC__))
#   define CORE_HW_SIMD_PPC CORE_VERSION_NUMBER(1, 0, 0)
#endif

#ifdef CORE_HW_SIMD_PPC
    #define CORE_HW_SIMD_NAME CORE_HW_SIMD_PPC_NAME
#endif
// Hardware SIMD ppc end /////////////////////////////////////////////////////////


// Hardware SIMD x86 /////////////////////////////////////////////////////////////
#define CORE_HW_SIMD_X86_MMX_VERSION CORE_VERSION_NUMBER(0, 99, 0) // The https://en.wikipedia.org/wiki/MMX_(instruction_set)[MMX] x86 extension version number.
#define CORE_HW_SIMD_X86_SSE_VERSION CORE_VERSION_NUMBER(1, 0, 0) // The https://en.wikipedia.org/wiki/Streaming_SIMD_Extensions[SSE] x86 extension version number.
#define CORE_HW_SIMD_X86_SSE2_VERSION CORE_VERSION_NUMBER(2, 0, 0) // The https://en.wikipedia.org/wiki/SSE2[SSE2] x86 extension version number.
#define CORE_HW_SIMD_X86_SSE3_VERSION CORE_VERSION_NUMBER(3, 0, 0) // The https://en.wikipedia.org/wiki/SSE3[SSE3] x86 extension version number.
#define CORE_HW_SIMD_X86_SSSE3_VERSION CORE_VERSION_NUMBER(3, 1, 0) // The https://en.wikipedia.org/wiki/SSSE3[SSSE3] x86 extension version number.
#define CORE_HW_SIMD_X86_SSE4_1_VERSION CORE_VERSION_NUMBER(4, 1, 0) // The https://en.wikipedia.org/wiki/SSE4#SSE4.1[SSE4_1] x86 extension version number.
#define CORE_HW_SIMD_X86_SSE4_2_VERSION CORE_VERSION_NUMBER(4, 2, 0) // The https://en.wikipedia.org/wiki/SSE4##SSE4.2[SSE4_2] x86 extension version number.
#define CORE_HW_SIMD_X86_AVX_VERSION CORE_VERSION_NUMBER(5, 0, 0) // The https://en.wikipedia.org/wiki/Advanced_Vector_Extensions[AVX] x86 extension version number.
#define CORE_HW_SIMD_X86_FMA3_VERSION CORE_VERSION_NUMBER(5, 2, 0) // The https://en.wikipedia.org/wiki/FMA_instruction_set[FMA3] x86 extension version number.
#define CORE_HW_SIMD_X86_AVX2_VERSION CORE_VERSION_NUMBER(5, 3, 0) // The https://en.wikipedia.org/wiki/Advanced_Vector_Extensions#Advanced_Vector_Extensions_2[AVX2] x86 extension version number.
#define CORE_HW_SIMD_X86_MIC_VERSION CORE_VERSION_NUMBER(9, 0, 0) // The https://en.wikipedia.org/wiki/Xeon_Phi[MIC] (Xeon Phi) x86 extension version number.


#if !defined(CORE_HW_SIMD_X86) && defined(__MIC__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_MIC_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__AVX2__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_AVX2_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__AVX__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_AVX_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__FMA__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_FMA_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__SSE4_2__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSE4_2_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__SSE4_1__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSE4_1_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__SSSE3__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSSE3_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__SSE3__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSE3_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && (defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSE2_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && (defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_SSE_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86) && defined(__MMX__)
    #define CORE_HW_SIMD_X86 CORE_HW_SIMD_X86_MMX_VERSION
#endif

#define CORE_HW_SIMD_X86_NAME "x86 SIMD"
#ifdef CORE_HW_SIMD_X86
    #define CORE_HW_SIMD_NAME CORE_HW_SIMD_X86_NAME
#endif
// Hardware SIMD x86 end /////////////////////////////////////////////////////////


// Hardware SIMD x86 arm /////////////////////////////////////////////////////////
#define CORE_HW_SIMD_X86_AMD_SSE4A_VERSION CORE_VERSION_NUMBER(4, 0, 0) // https://en.wikipedia.org/wiki/SSE4##SSE4A[SSE4A] x86 extension (AMD specific).
#define CORE_HW_SIMD_X86_AMD_FMA4_VERSION CORE_VERSION_NUMBER(5, 1, 0) // https://en.wikipedia.org/wiki/FMA_instruction_set#FMA4_instruction_set[FMA4] x86 extension (AMD specific).
#define CORE_HW_SIMD_X86_AMD_XOP_VERSION CORE_VERSION_NUMBER(5, 1, 1) // https://en.wikipedia.org/wiki/XOP_instruction_set[XOP] x86 extension (AMD specific).

#if !defined(CORE_HW_SIMD_X86_AMD) && defined(__XOP__)
    #define CORE_HW_SIMD_X86_AMD CORE_HW_SIMD_X86_AMD_XOP_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86_AMD) && defined(__FMA4__)
    #define CORE_HW_SIMD_X86_AMD CORE_HW_SIMD_X86_AMD_FMA4_VERSION
#endif
#if !defined(CORE_HW_SIMD_X86_AMD) && defined(__SSE4A__)
    #define CORE_HW_SIMD_X86_AMD CORE_HW_SIMD_X86_AMD_SSE4A_VERSION
#endif

#define CORE_HW_SIMD_X86_AMD_NAME "x86 (AMD) SIMD"
#ifdef CORE_HW_SIMD_X86_AMD
    // At this point, we know that we have an AMD CPU, we do need to check for
    // other x86 extensions to determine the final version number.
    #include <boost/predef/hardware/simd/x86.h>
    #if CORE_HW_SIMD_X86 > CORE_HW_SIMD_X86_AMD
        #undef CORE_HW_SIMD_X86_AMD
        #define CORE_HW_SIMD_X86_AMD CORE_HW_SIMD_X86
    #endif
    #define CORE_HW_SIMD_NAME CORE_HW_SIMD_X86_AMD_NAME 

#endif
// Hardware SIMD x86 arm end /////////////////////////////////////////////////////
// Hardware SIMD end //////////////////////////////////////////////////////////////////////////////
#endif // CORE_ENABLE_HARDWARE
#ifndef CORE_HW_SIMD_NAME
    #define CORE_HW_SIMD_NAME "Unknown"
#endif
#endif // CORE_H