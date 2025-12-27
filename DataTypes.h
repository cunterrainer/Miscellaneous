#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#if __cplusplus
    #define C_AT_LEAST_99 0
    #define HAS_MAX_ALIGN_T 1 // C++11 and onward have it

    #if defined(_MSVC_LANG)
        #define CPP_STD_VERSION _MSVC_LANG
    #else
        #define CPP_STD_VERSION __cplusplus
    #endif

    #define CPP_AT_LEAST_11 (CPP_STD_VERSION >= 201103L)
    #define CPP_AT_LEAST_17 (CPP_STD_VERSION >= 201703L)

    #if !CPP_AT_LEAST_11
        #error "This header requires at least C++11"
    #endif

    #include <cstddef>
    #include <cstdint>

    #define DT_TYPEDEF(alias, type) using alias = std::type
#else
    #define CPP_AT_LEAST_11 0
    #define CPP_AT_LEAST_17 0

    #if defined(__STDC_VERSION__)
        #define C_STD_VERSION __STDC_VERSION__
    #else
        #define C_STD_VERSION 0L
    #endif

    #define C_AT_LEAST_99 (C_STD_VERSION >= 199901L)
    #define HAS_MAX_ALIGN_T (C_STD_VERSION >= 201112L)
    
    #if !C_AT_LEAST_99
        #error "This header requires at least C99"
    #endif

    #include <stddef.h>
    #include <stdint.h>

    #define DT_TYPEDEF(alias, type) typedef type alias
#endif

DT_TYPEDEF(size,    size_t);
DT_TYPEDEF(ptrdiff, ptrdiff_t);

#if HAS_MAX_ALIGN_T
    DT_TYPEDEF(max_align,   max_align_t);
#endif

#ifdef INTPTR_MAX
    DT_TYPEDEF(intptr,  intptr_t);
    DT_TYPEDEF(uintptr, uintptr_t);
#else
    #error "This platform does not support intptr_t / uintptr_t!"
#endif

#ifdef INT8_MAX
    DT_TYPEDEF(int8,  int8_t);
    DT_TYPEDEF(uint8, uint8_t);
#else
    #error "This platform does not support 8-bit integers!"
#endif

#ifdef INT16_MAX
    DT_TYPEDEF(int16,  int16_t);
    DT_TYPEDEF(uint16, uint16_t);
#else
    #error "This platform does not support 16-bit integers!"
#endif

#ifdef INT32_MAX
    DT_TYPEDEF(int32,  int32_t);
    DT_TYPEDEF(uint32, uint32_t);
#else
    #error "This platform does not support 32-bit integers!"
#endif

#ifdef INT64_MAX
    DT_TYPEDEF(int64,  int64_t);
    DT_TYPEDEF(uint64, uint64_t);
#else
    #error "This platform does not support 64-bit integers!"
#endif

DT_TYPEDEF(int_least8,   int_least8_t);
DT_TYPEDEF(int_least16,  int_least16_t);
DT_TYPEDEF(int_least32,  int_least32_t);
DT_TYPEDEF(int_least64,  int_least64_t);

DT_TYPEDEF(uint_least8,  uint_least8_t);
DT_TYPEDEF(uint_least16, uint_least16_t);
DT_TYPEDEF(uint_least32, uint_least32_t);
DT_TYPEDEF(uint_least64, uint_least64_t);

DT_TYPEDEF(int_fast8,   int_fast8_t);
DT_TYPEDEF(int_fast16,  int_fast16_t);
DT_TYPEDEF(int_fast32,  int_fast32_t);
DT_TYPEDEF(int_fast64,  int_fast64_t);

DT_TYPEDEF(uint_fast8,  uint_fast8_t);
DT_TYPEDEF(uint_fast16, uint_fast16_t);
DT_TYPEDEF(uint_fast32, uint_fast32_t);
DT_TYPEDEF(uint_fast64, uint_fast64_t);

DT_TYPEDEF(intmax,  intmax_t);
DT_TYPEDEF(uintmax, uintmax_t);


#if __cplusplus
    DT_TYPEDEF(nullptrt, nullptr_t);

    #if CPP_AT_LEAST_17
        DT_TYPEDEF(byte, byte);
    #endif
#endif


#ifdef DT_TYPEDEF
    #undef DT_TYPEDEF
#endif

#ifdef CPP_STD_VERSION
    #undef CPP_STD_VERSION
#endif

#ifdef CPP_AT_LEAST_11
    #undef CPP_AT_LEAST_11
#endif

#ifdef CPP_AT_LEAST_17
    #undef CPP_AT_LEAST_17
#endif

#ifdef HAS_MAX_ALIGN_T
    #undef HAS_MAX_ALIGN_T
#endif

#ifdef C_STD_VERSION
    #undef C_STD_VERSION
#endif

#ifdef C_AT_LEAST_99
    #undef C_AT_LEAST_99
#endif

#endif // DATA_TYPES_H