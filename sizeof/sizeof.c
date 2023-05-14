#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#define cpp "C++"
#else
#define cpp "C"
#endif

#ifdef _WIN32
    #define platform "Windows"
#elif defined linux
    #define platform "Linux"
#elif defined __APPLE__
    #define platform "MacOS"
#else
    #define platform "Undefined"
#endif

#ifdef __GNUC__
    #define compiler "GCC"
#elif defined __clang__
    #define compiler "CLANG"
#elif defined _MSC_VER
    #define compiler "MSVC"
#else
    #define compiler "Undefined"
#endif

// The later ones are for MSVC
// GCC and CLANG user the former
#if defined __x86_64__ || defined _M_X64
    #define architecture "x64"
    #define print_size(type) printf("%lld\n", sizeof(type));
#elif defined __i386__ || defined _M_IX86
    #define architecture "x86"
    #define print_size(type) printf("%d\n", sizeof(type));
#elif defined __arm__ || defined _M_ARM
    #define architecture "ARM"
    #define print_size(type) printf("%d\n", sizeof(type));
#else
    #define architecture "Undefined"
    #define print_size(type) printf("%d\n", sizeof(type));
#endif

int length;
int chars_to_size = 21;
#define print(type, strtype) \
        length = strlen(strtype) + 1;\
        length = chars_to_size - length;\
        printf("%s:", strtype);\
        for(int i = 0; i < length; ++i) printf(" ");\
        print_size(type)

int main()
{
    printf("-------------INFO-------------\n");
    printf("Platform:       %s\n", platform);
    printf("Architecture:   %s\n", architecture);
    printf("Compiler:       %s\n", compiler);
    printf("Language:       %s\n", cpp);
    printf("------------------------------\n");

    #ifdef __cplusplus
        printf("Size of C++ types in Bytes\n");
    #else
        printf("Size of C types in Bytes\n");
    #endif

    printf("Standard types:\n");
    print(void*, "   void*")
    //print(void, "void")
    print(bool, "   bool")
    print(char, "   char")
    print(short, "   short")
    print(wchar_t, "   wchar_t")
    print(int, "   int")
    print(float, "   float")
    print(double, "   double")
    print(long double, "   ldouble")
    print(long, "   long")
    print(long long, "   long long")
    print(size_t, "   size_t")

    printf("\nUnsigned:\n");
    print(unsigned char, "   uchar")
    print(unsigned short, "   ushort")
    print(unsigned int, "   uint")
    print(unsigned long, "   ulong")
    print(unsigned long long, "   ulong long")

    printf("\nSigned:\n");
    print(signed char, "   schar")

    printf("\nStdint:\n");
    print(int8_t, "   int8_t")
    print(int16_t, "   int16_t")
    print(int32_t, "   int32_t")
    print(int64_t, "   int64_t")
    print(uint8_t, "   uint8_t")
    print(uint16_t, "   uint16_t")
    print(uint32_t, "   uint32_t")
    print(uint64_t, "   uint64_t")
    printf("\n");
    print(int_least8_t, "   int_least8_t")
    print(int_least16_t, "   int_least16_t")
    print(int_least32_t, "   int_least32_t")
    print(int_least64_t, "   int_least64_t")
    print(uint_least8_t, "   uint_least8_t")
    print(uint_least16_t, "   uint_least16_t")
    print(uint_least32_t, "   uint_least32_t")
    print(uint_least64_t, "   uint_least64_t")
    printf("\n");
    print(int_fast8_t, "   int_fast8_t")
    print(int_fast16_t, "   int_fast16_t")
    print(int_fast32_t, "   int_fast32_t")
    print(int_fast64_t, "   int_fast64_t")
    print(uint_fast8_t, "   uint_fast8_t")
    print(uint_fast16_t, "   uint_fast16_t")
    print(uint_fast32_t, "   uint_fast32_t")
    print(uint_fast64_t, "   uint_fast64_t")
    printf("\n");
    print(intmax_t, "   intmax_t")
    print(uintmax_t, "   uintmax_t")

    getchar();
}