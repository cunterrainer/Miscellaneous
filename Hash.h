#ifndef HASH_H
#define HASH_H
/*
    Use the C functions if you want to prevent heap allocations entirely
    In the C++ interface the returned string is allocated in C it's either the
    internal stack buffer or a provided one

    C interface:
    all functions start with hash_ and structs with Hash_ private classes start with Hash_Private_
    hash_private_ functions are for the implementation and should not be called
    either use the provided functions, all have the same scheme:

        hash_sha256("Hello world", buffer); // returns a const char* (internal buffer if buffer == NULL)
        hash_sha256_file("main.c", "rb", buffer); // a user provided buffer must hold at least HASH_SHA256_BUFFER_SIZE+1 chars, always get null terminated

        hash_sha256_easy("Hello world"); // returns internal buffer
        hash_sha256_file_easy("main.c", "rb"); // buffered file hash (Shake128, Shake256 and Sha512t are not buffered)

        hash_sha256_binary("Hello world", 11, buffer); // the binary function take the size of the string e.g. for binary files

    or if you need to update the hash e.g. while reading chunks from a file (not for Shake128 and Shake256)

        Hash_Sha256 s;
        hash_sha256_init(s);
        hash_sha256_update_binary(s, str, size);
        hash_sha256_finalize(s);
        const char* hash = hash_sha256_hexdigest(s, buffer);

    C++ interface: (needs C++17)
    all functions and classes are in the namespace Hash
    either use the provided functions, all have the same scheme:

        Hash::sha256("Hello world"); // returns std::string
        Hash::File::HashFile<Hash::Sha256>("main.c"); // buffered file hash using std::ios::binary (Shake128, Shake256 and Sha512t are not buffered, Sha512<T> is buffered)

    or if you need to update the hash e.g. while reading chunks from a file (not for Shake128 and Shake256)

        Hash::Sha256 s;
        s.Update("Hello world");
        s.Finalize();
        std::string hash = s.Hexdigest();
*/
#define HASH_ENABLE_MD5    1 // md5
#define HASH_ENABLE_SHA1   1 // sha1
#define HASH_ENABLE_SHA2   1 // sha224, sha256, sha384, sha512, sha512/t
#define HASH_ENABLE_SHA3   1 // sha3-224, sha3-256, sha3-384, sha3-512
#define HASH_ENABLE_SHAKE  1 // shake128 and shake256
#define HASH_ENABLE_C_INTERFACE   1
#define HASH_ENABLE_CPP_INTERFACE 1
#define HASH_KECCAK_LITTLE_ENDIAN 1 // true for most systems (e.g. windows, linux, macos)
#define HASH_ENABLE_LOOP_UNROLLING 1 // On some systems not manually unrolling the loops might be faster, if the compiler is able to vectorize properly, see hash_benchmark.cpp, can also be achieved by disabling vectorization
#define HASH_FILE_READ_BUFFER_SIZE 4069
#define HASH_SHAKE_128_MALLOC_LIMIT 64 // if outsizeBytes is greater and no buffer is provided we will heap allocate
#define HASH_SHAKE_256_MALLOC_LIMIT 64 // if outsizeBytes is greater and no buffer is provided we will heap allocate

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996) // fopen is deprecated
#include <intrin.h> // compiler intrinsics for endian swap and rotate
#endif

#if defined(__cplusplus) && HASH_ENABLE_CPP_INTERFACE == 1
#include <ios>
#include <string>
#include <locale>
#include <cstdio>
#include <limits>
#include <climits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <codecvt>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cinttypes>
#include <type_traits>
#include <string_view>
#define HASH_INLINE inline
#else
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#define HASH_INLINE static
#endif // __cplusplus && HASH_ENABLE_CPP_INTERFACE

#if HASH_ENABLE_C_INTERFACE == 1
#define HASH_MD5_SIZE 32
#define HASH_SHA1_SIZE 40
#define HASH_SHA224_SIZE 56
#define HASH_SHA256_SIZE 64
#define HASH_SHA384_SIZE 96
#define HASH_SHA512_SIZE 128
#define HASH_SHA3_224_SIZE 56
#define HASH_SHA3_256_SIZE 64
#define HASH_SHA3_384_SIZE 96
#define HASH_SHA3_512_SIZE 128
#define HASH_MIN(a, b) (((a)<(b))?(a):(b))
// ================================Util====================================
HASH_INLINE void hash_util_char_array_to_hex_string(unsigned char* data, size_t size, char* out)
{
    static const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    size_t k = 0;
    for (size_t i = 0; i < size; ++i)
    {
        char const byte = data[i];
        out[k++] = hex_chars[(byte & 0xF0) >> 4];
        out[k++] = hex_chars[(byte & 0x0F) >> 0];
    }
    out[k] = 0;
}


typedef void (*hash_util_init_func)(void*);
typedef void (*hash_util_update_func)(void*, const char*, size_t);
typedef void (*hash_util_finalize_func)(void*);
typedef const char* (*hash_util_hexdigest_func)(void*, char*);
HASH_INLINE const char* hash_util_hash_file(const char* path, const char* mode, void* hasher, char* out_buff, hash_util_init_func init_fn, hash_util_update_func update_fn, hash_util_finalize_func final_fn, hash_util_hexdigest_func hex_fn)
{
    FILE* fp = fopen(path, mode);
    if (fp == NULL)
        return "";
    
    char buffer[HASH_FILE_READ_BUFFER_SIZE];
    size_t bytes_read = 0;
    init_fn(hasher);

    while((bytes_read = fread(buffer, sizeof(char), sizeof(buffer)-1, fp)))
    {
        update_fn(hasher, buffer, bytes_read);
    }
    fclose(fp);
    final_fn(hasher);
    return hex_fn(hasher, out_buff);
}


HASH_INLINE char* hash_util_load_file(const char* path, const char* mode, long* fsize)
{
    FILE* f = fopen(path, mode);
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* string = (char*)malloc((size_t)*fsize);
    if (string == NULL) return NULL;
    fread(string, *fsize, 1, f);
    fclose(f);
    return string;
}


HASH_INLINE uint32_t hash_util_swap_endian_uint32_t(uint32_t u)
{
    #if defined(__clang__) || defined(__GNUC__)
        return __builtin_bswap32(u);
    #elif defined(_MSC_VER)
        return _byteswap_ulong(u);
    #else
        // Generic fallback - reverse bytes manually
        union
        {
            uint32_t u;
            uint8_t u8[sizeof(uint32_t)];
        } source, dest;

        source.u = u;
        for (int k = 0; k < sizeof(uint32_t); ++k)
        {
            dest.u8[k] = source.u8[sizeof(uint32_t) - k - 1];
        }
        return dest.u;
    #endif
}


HASH_INLINE uint64_t hash_util_swap_endian_uint64_t(uint64_t u)
{
    #if defined(__clang__) || defined(__GNUC__)
        return __builtin_bswap64(u);
    #elif defined(_MSC_VER)
        return _byteswap_uint64(u);
    #else
        // Generic fallback - reverse bytes manually
        union
        {
            uint64_t u;
            uint8_t u8[sizeof(uint64_t)];
        } source, dest;

        source.u = u;
        for (int k = 0; k < sizeof(uint64_t); ++k)
        {
            dest.u8[k] = source.u8[sizeof(uint64_t) - k - 1];
        }
        return dest.u;
    #endif
}

HASH_INLINE int hash_util_is_little_endian()
{
    #if defined(_MSC_VER)
        return 1; // Windows x86/x64 is always little endian
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return 1;
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        return 0;
    #else
        assert(0 && "Unknown endianness");
        return 0;
    #endif
}

HASH_INLINE uint32_t hash_util_right_rotate_u32(uint32_t n, size_t c)
{
    return (n >> c) | (n << (32 - c));
}

HASH_INLINE uint64_t hash_util_right_rotate_u64(uint64_t n, size_t c)
{
    return (n >> c) | (n << (64 - c));
}

HASH_INLINE uint32_t hash_util_left_rotate_u32(uint32_t n, size_t c)
{
    return (n << c) | (n >> (32 - c));
}

HASH_INLINE uint64_t hash_util_left_rotate_u64(uint64_t n, size_t c)
{
    return (n << c) | (n >> (64 - c));
}
// ================================Util====================================


#if HASH_ENABLE_SHA2 == 1
// ===============================Hash_Sha256===================================
typedef struct
{
    size_t   bitlen;
    int      bufferSize;
    uint8_t  buffer[64];
    uint32_t h[8];
} Hash_Private_Sha256;
typedef Hash_Private_Sha256 Hash_Sha256[1];


static void hash_private_sha256_compress(Hash_Sha256 s, const uint32_t* const w)
{
    static const uint32_t k[64] =
    {
        UINT32_C(0x428a2f98), UINT32_C(0x71374491), UINT32_C(0xb5c0fbcf), UINT32_C(0xe9b5dba5), UINT32_C(0x3956c25b), UINT32_C(0x59f111f1), UINT32_C(0x923f82a4), UINT32_C(0xab1c5ed5),
        UINT32_C(0xd807aa98), UINT32_C(0x12835b01), UINT32_C(0x243185be), UINT32_C(0x550c7dc3), UINT32_C(0x72be5d74), UINT32_C(0x80deb1fe), UINT32_C(0x9bdc06a7), UINT32_C(0xc19bf174),
        UINT32_C(0xe49b69c1), UINT32_C(0xefbe4786), UINT32_C(0x0fc19dc6), UINT32_C(0x240ca1cc), UINT32_C(0x2de92c6f), UINT32_C(0x4a7484aa), UINT32_C(0x5cb0a9dc), UINT32_C(0x76f988da),
        UINT32_C(0x983e5152), UINT32_C(0xa831c66d), UINT32_C(0xb00327c8), UINT32_C(0xbf597fc7), UINT32_C(0xc6e00bf3), UINT32_C(0xd5a79147), UINT32_C(0x06ca6351), UINT32_C(0x14292967),
        UINT32_C(0x27b70a85), UINT32_C(0x2e1b2138), UINT32_C(0x4d2c6dfc), UINT32_C(0x53380d13), UINT32_C(0x650a7354), UINT32_C(0x766a0abb), UINT32_C(0x81c2c92e), UINT32_C(0x92722c85),
        UINT32_C(0xa2bfe8a1), UINT32_C(0xa81a664b), UINT32_C(0xc24b8b70), UINT32_C(0xc76c51a3), UINT32_C(0xd192e819), UINT32_C(0xd6990624), UINT32_C(0xf40e3585), UINT32_C(0x106aa070),
        UINT32_C(0x19a4c116), UINT32_C(0x1e376c08), UINT32_C(0x2748774c), UINT32_C(0x34b0bcb5), UINT32_C(0x391c0cb3), UINT32_C(0x4ed8aa4a), UINT32_C(0x5b9cca4f), UINT32_C(0x682e6ff3),
        UINT32_C(0x748f82ee), UINT32_C(0x78a5636f), UINT32_C(0x84c87814), UINT32_C(0x8cc70208), UINT32_C(0x90befffa), UINT32_C(0xa4506ceb), UINT32_C(0xbef9a3f7), UINT32_C(0xc67178f2)
    };

    uint32_t a = s->h[0];
    uint32_t b = s->h[1];
    uint32_t c = s->h[2];
    uint32_t d = s->h[3];
    uint32_t e = s->h[4];
    uint32_t f = s->h[5];
    uint32_t g = s->h[6];
    uint32_t h = s->h[7];
    for (size_t i = 0; i < 64; ++i)
    {
        const uint32_t s1 = hash_util_right_rotate_u32(e, 6) ^ hash_util_right_rotate_u32(e, 11) ^ hash_util_right_rotate_u32(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t temp1 = h + s1 + ch + k[i] + w[i];
        const uint32_t s0 = hash_util_right_rotate_u32(a, 2) ^ hash_util_right_rotate_u32(a, 13) ^ hash_util_right_rotate_u32(a, 22);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t temp2 = s0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    s->h[0] += a;
    s->h[1] += b;
    s->h[2] += c;
    s->h[3] += d;
    s->h[4] += e;
    s->h[5] += f;
    s->h[6] += g;
    s->h[7] += h;
}


static void hash_private_sha256_transform(Hash_Sha256 s)
{
    uint32_t w[64];
    for (size_t i = 0; i < 16; ++i)
    {
        /*
            This code does the same as the memcpy
            uint8_t* c = (uint8_t*)&w[i];
            c[0] = s->buffer[4 * i];
            c[1] = s->buffer[4 * i + 1];
            c[2] = s->buffer[4 * i + 2];
            c[3] = s->buffer[4 * i + 3];
        */
        memcpy(&w[i], s->buffer + 4 * i, sizeof(uint32_t));
        if (hash_util_is_little_endian())
        {
            w[i] = hash_util_swap_endian_uint32_t(w[i]);
        }
    }

    for (size_t i = 16; i < 64; ++i)
    {
        const uint32_t s0 = hash_util_right_rotate_u32(w[i - 15], 7) ^ hash_util_right_rotate_u32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = hash_util_right_rotate_u32(w[i - 2], 17) ^ hash_util_right_rotate_u32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    hash_private_sha256_compress(s, w);
}




HASH_INLINE void hash_sha256_init(Hash_Sha256 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT32_C(0x6a09e667);
    s->h[1] = UINT32_C(0xbb67ae85);
    s->h[2] = UINT32_C(0x3c6ef372);
    s->h[3] = UINT32_C(0xa54ff53a);
    s->h[4] = UINT32_C(0x510e527f);
    s->h[5] = UINT32_C(0x9b05688c);
    s->h[6] = UINT32_C(0x1f83d9ab);
    s->h[7] = UINT32_C(0x5be0cd19);
};


static void hash_sha256_update_binary(Hash_Sha256 s, const char* data, size_t size)
{
    /*
        The code below does the same as this loop just more efficient
        const uint8_t* d = (const uint8_t*)data;
        for (size_t i = 0; i < size; ++i)
        {
            s->buffer[s->bufferSize++] = d[i];
            if (s->bufferSize == 64)
            {
                hash_private_sha256_transform(s);
                s->bufferSize = 0;
                s->bitlen += 512;
            }
        }
    */
    if (s->bufferSize)
    {
        size_t toCopy = HASH_MIN(64 - s->bufferSize, size);
        memcpy(s->buffer + s->bufferSize, data, toCopy);
        s->bufferSize += toCopy;
        data += toCopy;
        size -= toCopy;

        if (s->bufferSize == 64)
        {
            hash_private_sha256_transform(s);
            s->bitlen += 512;
            s->bufferSize = 0;
        }
    }

    while (size >= 64)
    {
        memcpy(s->buffer, data, 64);
        hash_private_sha256_transform(s);
        s->bitlen += 512;
        data += 64;
        size -= 64;
    }

    if (size > 0)
    {
        memcpy(s->buffer, data, size);
        s->bufferSize = size;
    }
}


HASH_INLINE void hash_sha256_update(Hash_Sha256 s, const char* data)
{
    hash_sha256_update_binary(s, data, strlen(data));
}


static void hash_sha256_finalize(Hash_Sha256 s)
{
    uint8_t start = s->bufferSize;
    uint8_t end = s->bufferSize < 56 ? 56 : 64;

    s->buffer[start++] = 0b10000000;
    memset(&s->buffer[start], 0, end - start);

    if (s->bufferSize >= 56)
    {
        hash_private_sha256_transform(s);
        memset(s->buffer, 0, 56);
    }

    s->bitlen += s->bufferSize * 8;
    uint64_t* const size = (uint64_t*)&s->buffer[64 - 8];
    *size = hash_util_is_little_endian() ? hash_util_swap_endian_uint64_t(s->bitlen) : s->bitlen;
    hash_private_sha256_transform(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 65
HASH_INLINE const char* hash_sha256_hexdigest(const Hash_Sha256 s, char* buffer)
{
    static char hex[HASH_SHA256_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (int i = 0; i < 8; ++i)
    {
        snprintf(&buff[i * 8], HASH_SHA256_SIZE+1, "%08" PRIx32, s->h[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 65
HASH_INLINE const char* hash_sha256_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha256 s;
    hash_sha256_init(s);
    hash_sha256_update_binary(s, str, size);
    hash_sha256_finalize(s);
    return hash_sha256_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha256(const char* str, char* buffer)
{
    return hash_sha256_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha256_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha256 s;
    return hash_util_hash_file(path, mode, s, buffer, (hash_util_init_func)hash_sha256_init, (hash_util_update_func)hash_sha256_update_binary, (hash_util_finalize_func)hash_sha256_finalize, (hash_util_hexdigest_func)hash_sha256_hexdigest);
}

HASH_INLINE const char* hash_sha256_easy(const char* str)
{
    return hash_sha256_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha256_file_easy(const char* path, const char* mode)
{
    return hash_sha256_file(path, mode, NULL);
}
// ===============================Hash_Sha256===================================


// ===============================Hash_Sha224===================================
typedef Hash_Sha256 Hash_Sha224;

HASH_INLINE void hash_sha224_init(Hash_Sha224 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT32_C(0xC1059ED8);
    s->h[1] = UINT32_C(0x367CD507);
    s->h[2] = UINT32_C(0x3070DD17);
    s->h[3] = UINT32_C(0xF70E5939);
    s->h[4] = UINT32_C(0xFFC00B31);
    s->h[5] = UINT32_C(0x68581511);
    s->h[6] = UINT32_C(0x64F98FA7);
    s->h[7] = UINT32_C(0xBEFA4FA4);
};


HASH_INLINE void hash_sha224_update_binary(Hash_Sha224 s, const char* data, size_t size)
{
    hash_sha256_update_binary(s, data, size);
}

HASH_INLINE void hash_sha224_update(Hash_Sha224 s, const char* data)
{
    hash_sha224_update_binary(s, data, strlen(data));
}

HASH_INLINE void hash_sha224_finalize(Hash_Sha224 s)
{
    hash_sha256_finalize(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 57
HASH_INLINE const char* hash_sha224_hexdigest(const Hash_Sha224 s, char* buffer)
{
    static char hex[HASH_SHA224_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (int i = 0; i < 7; ++i)
    {
        snprintf(&buff[i * 8], HASH_SHA224_SIZE+1, "%08" PRIx32, s->h[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 57
HASH_INLINE const char* hash_sha224_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha224 s;
    hash_sha224_init(s);
    hash_sha224_update_binary(s, str, size);
    hash_sha224_finalize(s);
    return hash_sha224_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha224(const char* str, char* buffer)
{
    return hash_sha224_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha224_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha224 s;
    return hash_util_hash_file(path, mode, s, buffer, (hash_util_init_func)hash_sha224_init, (hash_util_update_func)hash_sha224_update_binary, (hash_util_finalize_func)hash_sha224_finalize, (hash_util_hexdigest_func)hash_sha224_hexdigest);
}

HASH_INLINE const char* hash_sha224_easy(const char* str)
{
    return hash_sha224_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha224_file_easy(const char* path, const char* mode)
{
    return hash_sha224_file(path, mode, NULL);
}
// ===============================Hash_Sha224===================================


// ===============================Hash_Sha512===================================
typedef struct
{
    uint64_t bitlen;
    uint8_t bufferSize;
    uint8_t buffer[128];
    uint64_t h[8];
    size_t t; // only used for sha512t
} Hash_Private_Sha512;
typedef Hash_Private_Sha512 Hash_Sha512[1];


HASH_INLINE void hash_private_sha512_compress(Hash_Sha512 s, const uint64_t* const w)
{
    static const uint64_t k[80] =
    {
       UINT64_C(0x428a2f98d728ae22), UINT64_C(0x7137449123ef65cd), UINT64_C(0xb5c0fbcfec4d3b2f), UINT64_C(0xe9b5dba58189dbbc), UINT64_C(0x3956c25bf348b538),
       UINT64_C(0x59f111f1b605d019), UINT64_C(0x923f82a4af194f9b), UINT64_C(0xab1c5ed5da6d8118), UINT64_C(0xd807aa98a3030242), UINT64_C(0x12835b0145706fbe),
       UINT64_C(0x243185be4ee4b28c), UINT64_C(0x550c7dc3d5ffb4e2), UINT64_C(0x72be5d74f27b896f), UINT64_C(0x80deb1fe3b1696b1), UINT64_C(0x9bdc06a725c71235),
       UINT64_C(0xc19bf174cf692694), UINT64_C(0xe49b69c19ef14ad2), UINT64_C(0xefbe4786384f25e3), UINT64_C(0x0fc19dc68b8cd5b5), UINT64_C(0x240ca1cc77ac9c65),
       UINT64_C(0x2de92c6f592b0275), UINT64_C(0x4a7484aa6ea6e483), UINT64_C(0x5cb0a9dcbd41fbd4), UINT64_C(0x76f988da831153b5), UINT64_C(0x983e5152ee66dfab),
       UINT64_C(0xa831c66d2db43210), UINT64_C(0xb00327c898fb213f), UINT64_C(0xbf597fc7beef0ee4), UINT64_C(0xc6e00bf33da88fc2), UINT64_C(0xd5a79147930aa725),
       UINT64_C(0x06ca6351e003826f), UINT64_C(0x142929670a0e6e70), UINT64_C(0x27b70a8546d22ffc), UINT64_C(0x2e1b21385c26c926), UINT64_C(0x4d2c6dfc5ac42aed),
       UINT64_C(0x53380d139d95b3df), UINT64_C(0x650a73548baf63de), UINT64_C(0x766a0abb3c77b2a8), UINT64_C(0x81c2c92e47edaee6), UINT64_C(0x92722c851482353b),
       UINT64_C(0xa2bfe8a14cf10364), UINT64_C(0xa81a664bbc423001), UINT64_C(0xc24b8b70d0f89791), UINT64_C(0xc76c51a30654be30), UINT64_C(0xd192e819d6ef5218),
       UINT64_C(0xd69906245565a910), UINT64_C(0xf40e35855771202a), UINT64_C(0x106aa07032bbd1b8), UINT64_C(0x19a4c116b8d2d0c8), UINT64_C(0x1e376c085141ab53),
       UINT64_C(0x2748774cdf8eeb99), UINT64_C(0x34b0bcb5e19b48a8), UINT64_C(0x391c0cb3c5c95a63), UINT64_C(0x4ed8aa4ae3418acb), UINT64_C(0x5b9cca4f7763e373),
       UINT64_C(0x682e6ff3d6b2b8a3), UINT64_C(0x748f82ee5defb2fc), UINT64_C(0x78a5636f43172f60), UINT64_C(0x84c87814a1f0ab72), UINT64_C(0x8cc702081a6439ec),
       UINT64_C(0x90befffa23631e28), UINT64_C(0xa4506cebde82bde9), UINT64_C(0xbef9a3f7b2c67915), UINT64_C(0xc67178f2e372532b), UINT64_C(0xca273eceea26619c),
       UINT64_C(0xd186b8c721c0c207), UINT64_C(0xeada7dd6cde0eb1e), UINT64_C(0xf57d4f7fee6ed178), UINT64_C(0x06f067aa72176fba), UINT64_C(0x0a637dc5a2c898a6),
       UINT64_C(0x113f9804bef90dae), UINT64_C(0x1b710b35131c471b), UINT64_C(0x28db77f523047d84), UINT64_C(0x32caab7b40c72493), UINT64_C(0x3c9ebe0a15c9bebc),
       UINT64_C(0x431d67c49c100d4c), UINT64_C(0x4cc5d4becb3e42b6), UINT64_C(0x597f299cfc657e2a), UINT64_C(0x5fcb6fab3ad6faec), UINT64_C(0x6c44198c4a475817)
    };

    uint64_t a = s->h[0];
    uint64_t b = s->h[1];
    uint64_t c = s->h[2];
    uint64_t d = s->h[3];
    uint64_t e = s->h[4];
    uint64_t f = s->h[5];
    uint64_t g = s->h[6];
    uint64_t h = s->h[7];
    for (size_t i = 0; i < 80; ++i)
    {
        const uint64_t s1 = hash_util_right_rotate_u64(e, 14) ^ hash_util_right_rotate_u64(e, 18) ^ hash_util_right_rotate_u64(e, 41);
        const uint64_t ch = (e & f) ^ (~e & g);
        const uint64_t temp1 = h + s1 + ch + k[i] + w[i];
        const uint64_t s0 = hash_util_right_rotate_u64(a, 28) ^ hash_util_right_rotate_u64(a, 34) ^ hash_util_right_rotate_u64(a, 39);
        const uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint64_t temp2 = s0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    s->h[0] += a;
    s->h[1] += b;
    s->h[2] += c;
    s->h[3] += d;
    s->h[4] += e;
    s->h[5] += f;
    s->h[6] += g;
    s->h[7] += h;
}


HASH_INLINE void hash_private_sha512_transform(Hash_Sha512 s)
{
    uint64_t w[80];
    for (size_t i = 0; i < 16; ++i)
    {
        uint8_t* c = (uint8_t*)&w[i];
        c[0] = s->buffer[8 * i];
        c[1] = s->buffer[8 * i + 1];
        c[2] = s->buffer[8 * i + 2];
        c[3] = s->buffer[8 * i + 3];
        c[4] = s->buffer[8 * i + 4];
        c[5] = s->buffer[8 * i + 5];
        c[6] = s->buffer[8 * i + 6];
        c[7] = s->buffer[8 * i + 7];
        w[i] = hash_util_is_little_endian() ? hash_util_swap_endian_uint64_t(w[i]) : w[i];
    }

    for (size_t i = 16; i < 80; ++i) // Extend the first 16 words
    {
        const uint64_t s0 = hash_util_right_rotate_u64(w[i - 15], 1) ^ hash_util_right_rotate_u64(w[i - 15], 8) ^ (w[i - 15] >> 7);
        const uint64_t s1 = hash_util_right_rotate_u64(w[i - 2], 19) ^ hash_util_right_rotate_u64(w[i - 2], 61) ^ (w[i - 2] >> 6);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    hash_private_sha512_compress(s, w);
}




HASH_INLINE void hash_sha512_init(Hash_Sha512 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT64_C(0x6a09e667f3bcc908);
    s->h[1] = UINT64_C(0xbb67ae8584caa73b);
    s->h[2] = UINT64_C(0x3c6ef372fe94f82b);
    s->h[3] = UINT64_C(0xa54ff53a5f1d36f1);
    s->h[4] = UINT64_C(0x510e527fade682d1);
    s->h[5] = UINT64_C(0x9b05688c2b3e6c1f);
    s->h[6] = UINT64_C(0x1f83d9abfb41bd6b);
    s->h[7] = UINT64_C(0x5be0cd19137e2179);
};

HASH_INLINE void hash_sha512_reset(Hash_Sha512 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
}

HASH_INLINE void hash_sha512_update_binary(Hash_Sha512 s, const char* data, size_t size)
{
    const uint8_t* d = (const uint8_t*)data;
    for (size_t i = 0; i < size; ++i)
    {
        s->buffer[s->bufferSize++] = d[i];
        if (s->bufferSize == 128)
        {
            hash_private_sha512_transform(s);
            s->bufferSize = 0;
            s->bitlen += 1024;
        }
    }
}

HASH_INLINE void hash_sha512_update(Hash_Sha512 s, const char* data)
{
    hash_sha512_update_binary(s, data, strlen(data));
}

HASH_INLINE void hash_sha512_finalize(Hash_Sha512 s)
{
    uint8_t start = s->bufferSize;
    uint8_t end = s->bufferSize < 112 ? 120 : 128; // 120 instead of 112 because m_Bitlen is a 64 bit uint

    s->buffer[start++] = 0b10000000;
    memset(&s->buffer[start], 0, end - start);

    if (s->bufferSize >= 112)
    {
        hash_private_sha512_transform(s);
        memset(s->buffer, 0, 120);
    }

    s->bitlen += s->bufferSize * 8;
    uint64_t* const size = (uint64_t*)&s->buffer[128 - 8]; // -8 instead of -16 because we use an uint64 instead of uint128
    *size = hash_util_is_little_endian() ? hash_util_swap_endian_uint64_t(s->bitlen) : s->bitlen;
    hash_private_sha512_transform(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 129
HASH_INLINE const char* hash_sha512_hexdigest(const Hash_Sha512 s, char* buffer)
{
    static char hex[HASH_SHA512_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < 8; ++i)
    {
        snprintf(&buff[i * 16], HASH_SHA512_SIZE+1, "%016" PRIx64, s->h[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 129
HASH_INLINE const char* hash_sha512_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha512 s;
    hash_sha512_init(s);
    hash_sha512_update_binary(s, str, size);
    hash_sha512_finalize(s);
    return hash_sha512_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha512(const char* str, char* buffer)
{
    return hash_sha512_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha512_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha512 s;
    return hash_util_hash_file(path, mode, s, buffer, (hash_util_init_func)hash_sha512_init, (hash_util_update_func)hash_sha512_update_binary, (hash_util_finalize_func)hash_sha512_finalize, (hash_util_hexdigest_func)hash_sha512_hexdigest);
}

HASH_INLINE const char* hash_sha512_easy(const char* str)
{
    return hash_sha512_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha512_file_easy(const char* path, const char* mode)
{
    return hash_sha512_file(path, mode, NULL);
}
// ===============================Hash_Sha512===================================


// ===============================Hash_Sha512T==================================
typedef Hash_Sha512 Hash_Sha512T;
HASH_INLINE void hash_sha512t_init(Hash_Sha512T s, size_t t)
{
    assert(t != 384 && "t = 384 is not allowed use Hash_Sha384 instead!");
    assert(t >= 4 && t <= 2048 && "t must satisfy t >= 4 && t <= 2048!");
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT64_C(0xcfac43c256196cad);
    s->h[1] = UINT64_C(0x1ec20b20216f029e);
    s->h[2] = UINT64_C(0x99cb56d75b315d8e);
    s->h[3] = UINT64_C(0x00ea509ffab89354);
    s->h[4] = UINT64_C(0xf4abf7da08432774);
    s->h[5] = UINT64_C(0x3ea0cd298e9bc9ba);
    s->h[6] = UINT64_C(0xba267c0e5ee418ce);
    s->h[7] = UINT64_C(0xfe4568bcb6db84dc);
    s->t = t;

    char str[13], buff[129];
    memset(str, 0, 13);
    snprintf(str, 13, "SHA-512/%u", (unsigned int)t);

    hash_sha512_update(s, str);
    hash_sha512_finalize(s);
    hash_sha512_hexdigest(s, buff);
    hash_sha512_reset(s);

    size_t k = 0;
    for (size_t i = 0; i < 128; i += 16)
    {
        char hex[17];
        hex[16] = 0;
        memcpy(hex, &buff[i], 16);
        s->h[k++] = strtoull(hex, NULL, 16);
    }
};


HASH_INLINE void hash_sha512t_update_binary(Hash_Sha512T s, const char* data, size_t size)
{
    hash_sha512_update_binary(s, data, size);
}

HASH_INLINE void hash_sha512t_update(Hash_Sha512T s, const char* data)
{
    hash_sha512t_update_binary(s, data, strlen(data));
}

HASH_INLINE void hash_sha512t_finalize(Hash_Sha512T s)
{
    hash_sha512_finalize(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least (t/4)+1
HASH_INLINE const char* hash_sha512t_hexdigest(const Hash_Sha512T s, char* buffer)
{
    static char hex[513]; // use max allowed size to avoid memory allocation
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < 8; ++i)
    {
        snprintf(&buff[i * 16], 513, "%016" PRIx64, s->h[i]);
    }
    buff[s->t / 4] = 0;
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least (t/4)+1
HASH_INLINE const char* hash_sha512t_binary(size_t t, const char* str, size_t size, char* buffer)
{
    Hash_Sha512T s;
    hash_sha512t_init(s, t);
    hash_sha512t_update_binary(s, str, size);
    hash_sha512t_finalize(s);
    return hash_sha512t_hexdigest(s, buffer);
}

// if buffer == NULL returns internal buffer, buffer size must be at least (t/4)+1 (Null term char)
HASH_INLINE const char* hash_sha512t(size_t t, const char* str, char* buffer)
{
    return hash_sha512t_binary(t, str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha512t_easy(size_t t, const char* str)
{
    return hash_sha512t_binary(t, str, strlen(str), NULL);
}

// if buffer == NULL returns internal buffer, buffer size must be at least (t/4)+1 (Null term char)
HASH_INLINE const char* hash_sha512t_file(size_t t, const char* path, const char* mode, char* buffer)
{
    long fsize;
    char* content = hash_util_load_file(path, mode, &fsize);
    if (content == NULL) return "";
    const char* hash = hash_sha512t_binary(t, content, fsize, buffer);
    free(content);
    return hash;
}


HASH_INLINE const char* hash_sha512t_file_easy(size_t t, const char* path, const char* mode)
{
    return hash_sha512t_file(t, path, mode, NULL);
}
// ===============================Hash_Sha512T==================================


// ===============================Hash_Sha384===================================
typedef Hash_Sha512 Hash_Sha384;
HASH_INLINE void hash_sha384_init(Hash_Sha384 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT64_C(0xcbbb9d5dc1059ed8);
    s->h[1] = UINT64_C(0x629a292a367cd507);
    s->h[2] = UINT64_C(0x9159015a3070dd17);
    s->h[3] = UINT64_C(0x152fecd8f70e5939);
    s->h[4] = UINT64_C(0x67332667ffc00b31);
    s->h[5] = UINT64_C(0x8eb44a8768581511);
    s->h[6] = UINT64_C(0xdb0c2e0d64f98fa7);
    s->h[7] = UINT64_C(0x47b5481dbefa4fa4);
};


HASH_INLINE void hash_sha384_update_binary(Hash_Sha384 s, const char* data, size_t size)
{
    hash_sha512_update_binary(s, data, size);
}

HASH_INLINE void hash_sha384_update(Hash_Sha384 s, const char* data)
{
    hash_sha384_update_binary(s, data, strlen(data));
}

HASH_INLINE void hash_sha384_finalize(Hash_Sha384 s)
{
    hash_sha512_finalize(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 97
HASH_INLINE const char* hash_sha384_hexdigest(const Hash_Sha384 s, char* buffer)
{
    static char hex[HASH_SHA384_SIZE+1]; // use max allowed size to avoid memory allocation
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < 6; ++i)
    {
        snprintf(&buff[i * 16], HASH_SHA384_SIZE+1, "%016" PRIx64, s->h[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 97
HASH_INLINE const char* hash_sha384_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha384 s;
    hash_sha384_init(s);
    hash_sha384_update_binary(s, str, size);
    hash_sha384_finalize(s);
    return hash_sha384_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha384(const char* str, char* buffer)
{
    return hash_sha384_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha384_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha384 s;
    return hash_util_hash_file(path, mode, s, buffer, (hash_util_init_func)hash_sha384_init, (hash_util_update_func)hash_sha384_update_binary, (hash_util_finalize_func)hash_sha384_finalize, (hash_util_hexdigest_func)hash_sha384_hexdigest);
}

HASH_INLINE const char* hash_sha384_easy(const char* str)
{
    return hash_sha384_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha384_file_easy(const char* path, const char* mode)
{
    return hash_sha384_file(path, mode, NULL);
}
// ===============================Hash_Sha384===================================
#endif // HASH_ENABLE_SHA2


#if HASH_ENABLE_SHA1 == 1
// ================================Hash_Sha1====================================
typedef struct
{
    size_t   bitlen;
    int      bufferSize;
    uint8_t  buffer[64];
    uint32_t h[5];
} Hash_Private_Sha1;
typedef Hash_Private_Sha1 Hash_Sha1[1];


static void hash_private_hash_sha1_transform(Hash_Sha1 s)
{
    uint32_t w[80];
    for (int i = 0; i < 16; ++i)
    {
        /*
            This code does the same as the memcpy
            uint8_t* ptr = (uint8_t*)&w[i];
            ptr[0] = s->buffer[4 * i];
            ptr[1] = s->buffer[4 * i + 1];
            ptr[2] = s->buffer[4 * i + 2];
            ptr[3] = s->buffer[4 * i + 3];
        */
        memcpy(&w[i], s->buffer + 4 * i, sizeof(uint32_t));
        if (hash_util_is_little_endian())
        {
            w[i] = hash_util_swap_endian_uint32_t(w[i]);
        }
    }

    #if HASH_ENABLE_LOOP_UNROLLING
        for (int i = 16; i < 80; i += 8)
        {
            w[i] = hash_util_left_rotate_u32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            w[i+1] = hash_util_left_rotate_u32(w[i+1 - 3] ^ w[i+1 - 8] ^ w[i+1 - 14] ^ w[i+1 - 16], 1);
            w[i+2] = hash_util_left_rotate_u32(w[i+2 - 3] ^ w[i+2 - 8] ^ w[i+2 - 14] ^ w[i+2 - 16], 1);
            w[i+3] = hash_util_left_rotate_u32(w[i+3 - 3] ^ w[i+3 - 8] ^ w[i+3 - 14] ^ w[i+3 - 16], 1);
            w[i+4] = hash_util_left_rotate_u32(w[i+4 - 3] ^ w[i+4 - 8] ^ w[i+4 - 14] ^ w[i+4 - 16], 1);
            w[i+5] = hash_util_left_rotate_u32(w[i+5 - 3] ^ w[i+5 - 8] ^ w[i+5 - 14] ^ w[i+5 - 16], 1);
            w[i+6] = hash_util_left_rotate_u32(w[i+6 - 3] ^ w[i+6 - 8] ^ w[i+6 - 14] ^ w[i+6 - 16], 1);
            w[i+7] = hash_util_left_rotate_u32(w[i+7 - 3] ^ w[i+7 - 8] ^ w[i+7 - 14] ^ w[i+7 - 16], 1);
        }
    #else
        for (int i = 16; i < 80; ++i)
        {
            w[i] = hash_util_left_rotate_u32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }
    #endif

    uint32_t a = s->h[0];
    uint32_t b = s->h[1];
    uint32_t c = s->h[2];
    uint32_t d = s->h[3];
    uint32_t e = s->h[4];
    uint32_t k, f;

    for (int i = 0; i < 20; ++i)
    {
        f = (b & c) | ((~b) & d);
        k = UINT32_C(0x5A827999);

        const uint32_t tmp = hash_util_left_rotate_u32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = hash_util_left_rotate_u32(b, 30);
        b = a;
        a = tmp;
    }

    for (int i = 20; i < 40; ++i)
    {
        f = b ^ c ^ d;
        k = UINT32_C(0x6ED9EBA1);

        const uint32_t tmp = hash_util_left_rotate_u32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = hash_util_left_rotate_u32(b, 30);
        b = a;
        a = tmp;
    }

    for (int i = 40; i < 60; ++i)
    {
        f = (b & c) | (b & d) | (c & d);
        k = UINT32_C(0x8F1BBCDC);

        const uint32_t tmp = hash_util_left_rotate_u32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = hash_util_left_rotate_u32(b, 30);
        b = a;
        a = tmp;
    }

    for (int i = 60; i < 80; ++i)
    {
        f = b ^ c ^ d;
        k = UINT32_C(0xCA62C1D6);

        const uint32_t tmp = hash_util_left_rotate_u32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = hash_util_left_rotate_u32(b, 30);
        b = a;
        a = tmp;
    }

    s->h[0] = s->h[0] + a;
    s->h[1] = s->h[1] + b;
    s->h[2] = s->h[2] + c;
    s->h[3] = s->h[3] + d;
    s->h[4] = s->h[4] + e;
}




HASH_INLINE void hash_sha1_init(Hash_Sha1 s)
{
    s->bitlen = 0;
    s->bufferSize = 0;
    s->h[0] = UINT32_C(0x67452301);
    s->h[1] = UINT32_C(0xEFCDAB89);
    s->h[2] = UINT32_C(0x98BADCFE);
    s->h[3] = UINT32_C(0x10325476);
    s->h[4] = UINT32_C(0xC3D2E1F0);
}


static void hash_sha1_update_binary(Hash_Sha1 s, const char* data, size_t size)
{
    /*
        The code below does the same as this loop just more efficient
        const uint8_t* d = (const uint8_t*)data;
        for (size_t i = 0; i < size; ++i)
        {
            s->buffer[s->bufferSize++] = d[i];
            if (s->bufferSize == 64)
            {
                hash_private_hash_sha1_transform(s);
                s->bufferSize = 0;
                s->bitlen += 512;
            }
        }
    */
    if (s->bufferSize)
    {
        size_t toCopy = HASH_MIN(64 - s->bufferSize, size);
        memcpy(s->buffer + s->bufferSize, data, toCopy);
        s->bufferSize += toCopy;
        data += toCopy;
        size -= toCopy;

        if (s->bufferSize == 64)
        {
            hash_private_hash_sha1_transform(s);
            s->bitlen += 512;
            s->bufferSize = 0;
        }
    }

    while (size >= 64)
    {
        memcpy(s->buffer, data, 64);
        hash_private_hash_sha1_transform(s);
        s->bitlen += 512;
        data += 64;
        size -= 64;
    }

    if (size > 0)
    {
        memcpy(s->buffer, data, size);
        s->bufferSize = size;
    }
}


HASH_INLINE void hash_sha1_update(Hash_Sha1 s, const char* data)
{
    hash_sha1_update_binary(s, data, strlen(data));
}


static void hash_sha1_finalize(Hash_Sha1 s)
{
    uint8_t start = s->bufferSize;
    uint8_t end = s->bufferSize < 56 ? 56 : 64;

    s->buffer[start++] = 0b10000000;
    memset(&s->buffer[start], 0, end - start);

    if (s->bufferSize >= 56)
    {
        hash_private_hash_sha1_transform(s);
        memset(s->buffer, 0, 56);
    }

    s->bitlen += s->bufferSize * 8;
    uint64_t* const size = (uint64_t*)&s->buffer[64 - 8];
    *size = hash_util_is_little_endian() ? hash_util_swap_endian_uint64_t(s->bitlen) : s->bitlen;
    hash_private_hash_sha1_transform(s);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 41
HASH_INLINE const char* hash_sha1_hexdigest(const Hash_Sha1 s, char* buffer)
{
    static char hex[HASH_SHA1_SIZE+1]; // use max allowed size to avoid memory allocation
    char* buff = buffer == NULL ? hex : buffer;
    for (int i = 0; i < 5; ++i)
    {
        snprintf(&buff[i * 8], HASH_SHA1_SIZE+1, "%08" PRIx32, s->h[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 41
HASH_INLINE const char* hash_sha1_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha1 s;
    hash_sha1_init(s);
    hash_sha1_update_binary(s, str, size);
    hash_sha1_finalize(s);
    return hash_sha1_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha1(const char* str, char* buffer)
{
    return hash_sha1_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha1_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha1 s;
    return hash_util_hash_file(path, mode, s, buffer, (hash_util_init_func)hash_sha1_init, (hash_util_update_func)hash_sha1_update_binary, (hash_util_finalize_func)hash_sha1_finalize, (hash_util_hexdigest_func)hash_sha1_hexdigest);
}

HASH_INLINE const char* hash_sha1_easy(const char* str)
{
    return hash_sha1_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha1_file_easy(const char* path, const char* mode)
{
    return hash_sha1_file(path, mode, NULL);
}
// ================================Hash_Sha1====================================
#endif // HASH_ENABLE_SHA1


#if HASH_ENABLE_MD5 == 1
// =================================Hash_MD5====================================
#define HASH_PRIVATE_MD5_BLOCKSIZE 64
typedef struct
{
    uint8_t buffer[HASH_PRIVATE_MD5_BLOCKSIZE]; // bytes that didn't fit in last 64 byte chunk
    uint32_t count[2];   // 64bit counter for number of bits (lo, hi)
    uint32_t state[4];   // digest so far
    uint8_t digest[16]; // the result
} Hash_Private_MD5;
typedef Hash_Private_MD5 Hash_MD5[1];


// low level logic operations
///////////////////////////////////////////////
// F, G, H and I are basic Hash_MD5 functions.
HASH_INLINE uint32_t hash_private_md5_F(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) | (~x & z);
}

HASH_INLINE uint32_t hash_private_md5_G(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & z) | (y & ~z);
}

HASH_INLINE uint32_t hash_private_md5_H(uint32_t x, uint32_t y, uint32_t z)
{
    return x ^ y ^ z;
}

HASH_INLINE uint32_t hash_private_md5_I(uint32_t x, uint32_t y, uint32_t z)
{
    return y ^ (x | ~z);
}

// rotate_left rotates x left n bits.
HASH_INLINE uint32_t hash_private_md5_rotate_left(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}


// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
HASH_INLINE uint32_t hash_private_md5_FF(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
    return hash_private_md5_rotate_left(a + hash_private_md5_F(b, c, d) + x + ac, s) + b;
}

HASH_INLINE uint32_t hash_private_md5_GG(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
    return hash_private_md5_rotate_left(a + hash_private_md5_G(b, c, d) + x + ac, s) + b;
}

HASH_INLINE uint32_t hash_private_md5_HH(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
    return hash_private_md5_rotate_left(a + hash_private_md5_H(b, c, d) + x + ac, s) + b;
}

HASH_INLINE uint32_t hash_private_md5_II(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
    return hash_private_md5_rotate_left(a + hash_private_md5_I(b, c, d) + x + ac, s) + b;
}


// decodes input (unsigned char) into output (uint32_t). Assumes len is a multiple of 4.
HASH_INLINE void hash_private_md5_decode(uint32_t output[], const uint8_t input[], uint32_t len)
{
    for (unsigned int i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j + 1]) << 8) |
        (((uint32_t)input[j + 2]) << 16) | (((uint32_t)input[j + 3]) << 24);
}

//////////////////////////////

// encodes input (uint32_t) into output (unsigned char). Assumes len is
// a multiple of 4.
HASH_INLINE void hash_private_md5_encode(uint8_t output[], const uint32_t input[], uint32_t len)
{
    for (uint32_t i = 0, j = 0; j < len; i++, j += 4)
    {
        output[j] = input[i] & 0xff;
        output[j + 1] = (input[i] >> 8) & 0xff;
        output[j + 2] = (input[i] >> 16) & 0xff;
        output[j + 3] = (input[i] >> 24) & 0xff;
    }
}


// apply Hash_MD5 algo on a block
HASH_INLINE void hash_private_md5_transform(Hash_MD5 m, const uint8_t block[HASH_PRIVATE_MD5_BLOCKSIZE])
{
    // Constants for Hash_MD5Transform routine.
    static const uint32_t S11 = 7;
    static const uint32_t S12 = 12;
    static const uint32_t S13 = 17;
    static const uint32_t S14 = 22;
    static const uint32_t S21 = 5;
    static const uint32_t S22 = 9;
    static const uint32_t S23 = 14;
    static const uint32_t S24 = 20;
    static const uint32_t S31 = 4;
    static const uint32_t S32 = 11;
    static const uint32_t S33 = 16;
    static const uint32_t S34 = 23;
    static const uint32_t S41 = 6;
    static const uint32_t S42 = 10;
    static const uint32_t S43 = 15;
    static const uint32_t S44 = 21;

    uint32_t a = m->state[0], b = m->state[1], c = m->state[2], d = m->state[3], x[16];
    hash_private_md5_decode(x, block, HASH_PRIVATE_MD5_BLOCKSIZE);

    /* Round 1 */
    a = hash_private_md5_FF(a, b, c, d, x[0],  S11, UINT32_C(0xd76aa478)); /* 1 */
    d = hash_private_md5_FF(d, a, b, c, x[1],  S12, UINT32_C(0xe8c7b756)); /* 2 */
    c = hash_private_md5_FF(c, d, a, b, x[2],  S13, UINT32_C(0x242070db)); /* 3 */
    b = hash_private_md5_FF(b, c, d, a, x[3],  S14, UINT32_C(0xc1bdceee)); /* 4 */
    a = hash_private_md5_FF(a, b, c, d, x[4],  S11, UINT32_C(0xf57c0faf)); /* 5 */
    d = hash_private_md5_FF(d, a, b, c, x[5],  S12, UINT32_C(0x4787c62a)); /* 6 */
    c = hash_private_md5_FF(c, d, a, b, x[6],  S13, UINT32_C(0xa8304613)); /* 7 */
    b = hash_private_md5_FF(b, c, d, a, x[7],  S14, UINT32_C(0xfd469501)); /* 8 */
    a = hash_private_md5_FF(a, b, c, d, x[8],  S11, UINT32_C(0x698098d8)); /* 9 */
    d = hash_private_md5_FF(d, a, b, c, x[9],  S12, UINT32_C(0x8b44f7af)); /* 10 */
    c = hash_private_md5_FF(c, d, a, b, x[10], S13, UINT32_C(0xffff5bb1)); /* 11 */
    b = hash_private_md5_FF(b, c, d, a, x[11], S14, UINT32_C(0x895cd7be)); /* 12 */
    a = hash_private_md5_FF(a, b, c, d, x[12], S11, UINT32_C(0x6b901122)); /* 13 */
    d = hash_private_md5_FF(d, a, b, c, x[13], S12, UINT32_C(0xfd987193)); /* 14 */
    c = hash_private_md5_FF(c, d, a, b, x[14], S13, UINT32_C(0xa679438e)); /* 15 */
    b = hash_private_md5_FF(b, c, d, a, x[15], S14, UINT32_C(0x49b40821)); /* 16 */

    /* Round 2 */
    a = hash_private_md5_GG(a, b, c, d, x[1],  S21, UINT32_C(0xf61e2562)); /* 17 */
    d = hash_private_md5_GG(d, a, b, c, x[6],  S22, UINT32_C(0xc040b340)); /* 18 */
    c = hash_private_md5_GG(c, d, a, b, x[11], S23, UINT32_C(0x265e5a51)); /* 19 */
    b = hash_private_md5_GG(b, c, d, a, x[0],  S24, UINT32_C(0xe9b6c7aa)); /* 20 */
    a = hash_private_md5_GG(a, b, c, d, x[5],  S21, UINT32_C(0xd62f105d)); /* 21 */
    d = hash_private_md5_GG(d, a, b, c, x[10], S22, UINT32_C(0x02441453)); /* 22 */
    c = hash_private_md5_GG(c, d, a, b, x[15], S23, UINT32_C(0xd8a1e681)); /* 23 */
    b = hash_private_md5_GG(b, c, d, a, x[4],  S24, UINT32_C(0xe7d3fbc8)); /* 24 */
    a = hash_private_md5_GG(a, b, c, d, x[9],  S21, UINT32_C(0x21e1cde6)); /* 25 */
    d = hash_private_md5_GG(d, a, b, c, x[14], S22, UINT32_C(0xc33707d6)); /* 26 */
    c = hash_private_md5_GG(c, d, a, b, x[3],  S23, UINT32_C(0xf4d50d87)); /* 27 */
    b = hash_private_md5_GG(b, c, d, a, x[8],  S24, UINT32_C(0x455a14ed)); /* 28 */
    a = hash_private_md5_GG(a, b, c, d, x[13], S21, UINT32_C(0xa9e3e905)); /* 29 */
    d = hash_private_md5_GG(d, a, b, c, x[2],  S22, UINT32_C(0xfcefa3f8)); /* 30 */
    c = hash_private_md5_GG(c, d, a, b, x[7],  S23, UINT32_C(0x676f02d9)); /* 31 */
    b = hash_private_md5_GG(b, c, d, a, x[12], S24, UINT32_C(0x8d2a4c8a)); /* 32 */

    /* Round 3 */
    a = hash_private_md5_HH(a, b, c, d, x[5],  S31, UINT32_C(0xfffa3942)); /* 33 */
    d = hash_private_md5_HH(d, a, b, c, x[8],  S32, UINT32_C(0x8771f681)); /* 34 */
    c = hash_private_md5_HH(c, d, a, b, x[11], S33, UINT32_C(0x6d9d6122)); /* 35 */
    b = hash_private_md5_HH(b, c, d, a, x[14], S34, UINT32_C(0xfde5380c)); /* 36 */
    a = hash_private_md5_HH(a, b, c, d, x[1],  S31, UINT32_C(0xa4beea44)); /* 37 */
    d = hash_private_md5_HH(d, a, b, c, x[4],  S32, UINT32_C(0x4bdecfa9)); /* 38 */
    c = hash_private_md5_HH(c, d, a, b, x[7],  S33, UINT32_C(0xf6bb4b60)); /* 39 */
    b = hash_private_md5_HH(b, c, d, a, x[10], S34, UINT32_C(0xbebfbc70)); /* 40 */
    a = hash_private_md5_HH(a, b, c, d, x[13], S31, UINT32_C(0x289b7ec6)); /* 41 */
    d = hash_private_md5_HH(d, a, b, c, x[0],  S32, UINT32_C(0xeaa127fa)); /* 42 */
    c = hash_private_md5_HH(c, d, a, b, x[3],  S33, UINT32_C(0xd4ef3085)); /* 43 */
    b = hash_private_md5_HH(b, c, d, a, x[6],  S34, UINT32_C(0x04881d05)); /* 44 */
    a = hash_private_md5_HH(a, b, c, d, x[9],  S31, UINT32_C(0xd9d4d039)); /* 45 */
    d = hash_private_md5_HH(d, a, b, c, x[12], S32, UINT32_C(0xe6db99e5)); /* 46 */
    c = hash_private_md5_HH(c, d, a, b, x[15], S33, UINT32_C(0x1fa27cf8)); /* 47 */
    b = hash_private_md5_HH(b, c, d, a, x[2],  S34, UINT32_C(0xc4ac5665)); /* 48 */

    /* Round 4 */
    a = hash_private_md5_II(a, b, c, d, x[0],  S41, UINT32_C(0xf4292244)); /* 49 */
    d = hash_private_md5_II(d, a, b, c, x[7],  S42, UINT32_C(0x432aff97)); /* 50 */
    c = hash_private_md5_II(c, d, a, b, x[14], S43, UINT32_C(0xab9423a7)); /* 51 */
    b = hash_private_md5_II(b, c, d, a, x[5],  S44, UINT32_C(0xfc93a039)); /* 52 */
    a = hash_private_md5_II(a, b, c, d, x[12], S41, UINT32_C(0x655b59c3)); /* 53 */
    d = hash_private_md5_II(d, a, b, c, x[3],  S42, UINT32_C(0x8f0ccc92)); /* 54 */
    c = hash_private_md5_II(c, d, a, b, x[10], S43, UINT32_C(0xffeff47d)); /* 55 */
    b = hash_private_md5_II(b, c, d, a, x[1],  S44, UINT32_C(0x85845dd1)); /* 56 */
    a = hash_private_md5_II(a, b, c, d, x[8],  S41, UINT32_C(0x6fa87e4f)); /* 57 */
    d = hash_private_md5_II(d, a, b, c, x[15], S42, UINT32_C(0xfe2ce6e0)); /* 58 */
    c = hash_private_md5_II(c, d, a, b, x[6],  S43, UINT32_C(0xa3014314)); /* 59 */
    b = hash_private_md5_II(b, c, d, a, x[13], S44, UINT32_C(0x4e0811a1)); /* 60 */
    a = hash_private_md5_II(a, b, c, d, x[4],  S41, UINT32_C(0xf7537e82)); /* 61 */
    d = hash_private_md5_II(d, a, b, c, x[11], S42, UINT32_C(0xbd3af235)); /* 62 */
    c = hash_private_md5_II(c, d, a, b, x[2],  S43, UINT32_C(0x2ad7d2bb)); /* 63 */
    b = hash_private_md5_II(b, c, d, a, x[9],  S44, UINT32_C(0xeb86d391)); /* 64 */

    m->state[0] += a;
    m->state[1] += b;
    m->state[2] += c;
    m->state[3] += d;

    // Zeroize sensitive information.
    memset(x, 0, sizeof x);
}


HASH_INLINE void hash_private_md5_update_binary(Hash_MD5 m, const unsigned char* input, size_t size)
{
    const uint32_t s = (uint32_t)size;
    // compute number of bytes mod 64
    uint32_t index = m->count[0] / 8 % HASH_PRIVATE_MD5_BLOCKSIZE;

    // Update number of bits
    if ((m->count[0] += (s << 3)) < (s << 3))
        m->count[1]++;
    m->count[1] += (s >> 29);

    // number of bytes we need to fill in buffer
    uint32_t firstpart = 64 - index;

    uint32_t i;

    // transform as many times as possible.
    if (s >= firstpart)
    {
        // fill buffer first, transform
        memcpy(&m->buffer[index], input, firstpart);
        hash_private_md5_transform(m, m->buffer);

        // transform chunks of blocksize (64 bytes)
        for (i = firstpart; i + HASH_PRIVATE_MD5_BLOCKSIZE <= s; i += HASH_PRIVATE_MD5_BLOCKSIZE)
            hash_private_md5_transform(m, &input[i]);

        index = 0;
    }
    else
        i = 0;

    // buffer remaining input
    memcpy(&m->buffer[index], &input[i], s - i);
}



HASH_INLINE void hash_md5_init(Hash_MD5 m)
{
    m->count[0] = 0;
    m->count[1] = 0;

    // load magic initialization constants.
    m->state[0] = UINT32_C(0x67452301);
    m->state[1] = UINT32_C(0xefcdab89);
    m->state[2] = UINT32_C(0x98badcfe);
    m->state[3] = UINT32_C(0x10325476);
}


HASH_INLINE void hash_md5_update_binary(Hash_MD5 m, const char* data, size_t size)
{
    hash_private_md5_update_binary(m, (const unsigned char*)data, size);
}


HASH_INLINE void hash_md5_update(Hash_MD5 m, const char* data)
{
    hash_md5_update_binary(m, data, strlen(data));
}


// Hash_MD5 finalization. Ends an Hash_MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
HASH_INLINE void  hash_md5_finalize(Hash_MD5 m)
{
    static const unsigned char padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // Save number of bits
    unsigned char bits[8];
    hash_private_md5_encode(bits, m->count, 8);

    // pad out to 56 mod 64.
    uint32_t index = m->count[0] / 8 % 64;
    uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
    hash_private_md5_update_binary(m, padding, padLen);

    // Append length (before padding)
    hash_private_md5_update_binary(m, bits, 8);

    // Store state in digest
    hash_private_md5_encode(m->digest, m->state, 16);

    // Zeroize sensitive information.
    memset(m->buffer, 0, sizeof m->buffer);
    memset(m->count, 0, sizeof m->count);
}


// if buffer == NULL returns internal buffer, buffer size must be at least 33
HASH_INLINE const char* hash_md5_hexdigest(const Hash_MD5 m, char* buffer)
{
    static char hex[HASH_MD5_SIZE+1];
    char* buf = buffer == NULL ? hex : buffer;
    for (int i = 0; i < 16; i++)
    {
        snprintf(&buf[i * 2], HASH_MD5_SIZE+1, "%02x", m->digest[i]);
    }
    return buf;
}


// if buffer == NULL returns internal buffer, buffer size must be at least 33
HASH_INLINE const char* hash_md5_binary(const char* str, size_t size, char* buffer)
{
    Hash_MD5 m;
    hash_md5_init(m);
    hash_md5_update_binary(m, str, size);
    hash_md5_finalize(m);
    return hash_md5_hexdigest(m, buffer);
}

HASH_INLINE const char* hash_md5(const char* str, char* buffer)
{
    return hash_md5_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_md5_file(const char* path, const char* mode, char* buffer)
{
    Hash_MD5 m;
    return hash_util_hash_file(path, mode, m, buffer, (hash_util_init_func)hash_md5_init, (hash_util_update_func)hash_md5_update_binary, (hash_util_finalize_func)hash_md5_finalize, (hash_util_hexdigest_func)hash_md5_hexdigest);
}

HASH_INLINE const char* hash_md5_easy(const char* str)
{
    return hash_md5_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_md5_file_easy(const char* path, const char* mode)
{
    return hash_md5_file(path, mode, NULL);
}
#undef HASH_PRIVATE_MD5_BLOCKSIZE
// =================================Hash_MD5====================================
#endif // HASH_ENABLE_MD5



#if HASH_ENABLE_SHA3 == 1
// ================================Hash_Sha3====================================
#define HASH_PRIVATE_KECCAK_SPONGE_WORDS (((1600)/8/*bits to byte*/)/sizeof(uint64_t))

typedef struct
{
    uint64_t saved;       // the portion of the input message that we didn't consume yet
    union {                     // Keccak's state
        uint64_t s[HASH_PRIVATE_KECCAK_SPONGE_WORDS];
        uint8_t sb[HASH_PRIVATE_KECCAK_SPONGE_WORDS * 8];
    } u;
    size_t byteIndex;      // 0..7--the next byte after the set one (starts from 0; 0--none are buffered)
    size_t wordIndex;      // 0..24--the next word to integrate input (starts from 0)
    size_t capacityWords;  // the double size of the hash output in words (e.g. 16 for Keccak 512)
} Hash_Private_Sha3_Template;
typedef Hash_Private_Sha3_Template Hash_Private_Sha3;


HASH_INLINE size_t hash_private_sha3_cw(size_t x)
{
    static const size_t UseKeccakFlag = 0x80000000;
    return (x) & (~UseKeccakFlag);
}


HASH_INLINE void hash_private_sha3_keccakf(uint64_t s[25])
{
    static const uint8_t keccakf_rotc[24] = {
        1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
        18, 39, 61, 20, 44
    };

    static const uint8_t keccakf_piln[24] = {
        10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
        14, 22, 9, 6, 1
    };

    static const uint64_t keccakf_rndc[24] = {
       UINT64_C(0x0000000000000001), UINT64_C(0x0000000000008082),
       UINT64_C(0x800000000000808a), UINT64_C(0x8000000080008000),
       UINT64_C(0x000000000000808b), UINT64_C(0x0000000080000001),
       UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008009),
       UINT64_C(0x000000000000008a), UINT64_C(0x0000000000000088),
       UINT64_C(0x0000000080008009), UINT64_C(0x000000008000000a),
       UINT64_C(0x000000008000808b), UINT64_C(0x800000000000008b),
       UINT64_C(0x8000000000008089), UINT64_C(0x8000000000008003),
       UINT64_C(0x8000000000008002), UINT64_C(0x8000000000000080),
       UINT64_C(0x000000000000800a), UINT64_C(0x800000008000000a),
       UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008080),
       UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008008)
    };


    uint64_t bc[5];
    const uint8_t KeccakRounds = 24;

    for(uint8_t round = 0; round < KeccakRounds; round++)
    {

        /* Theta */
        for(uint8_t i = 0; i < 5; i++)
            bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

        for(uint8_t i = 0; i < 5; i++)
        {
            uint64_t t = bc[(i + 4) % 5] ^ hash_util_left_rotate_u64(bc[(i + 1) % 5], 1);
            for(uint8_t j = 0; j < 25; j += 5)
                s[j + i] ^= t;
        }

        /* Rho Pi */
        uint64_t t = s[1];
        for(uint8_t i = 0; i < 24; i++)
        {
            uint8_t j = keccakf_piln[i];
            bc[0] = s[j];
            s[j] = hash_util_left_rotate_u64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        /* Chi */
        for(uint8_t j = 0; j < 25; j += 5)
        {
            for(uint8_t i = 0; i < 5; i++)
                bc[i] = s[j + i];
            for(uint8_t i = 0; i < 5; i++)
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        /* Iota */
        s[0] ^= keccakf_rndc[round];
    }
}


HASH_INLINE void hash_private_sha3_init(Hash_Private_Sha3* s, size_t bitlen)
{
    s->saved = 0;
    s->byteIndex = 0;
    s->wordIndex = 0;
    s->capacityWords = 2 * bitlen / (8 * sizeof(uint64_t));
    memset((void*)s->u.sb, 0, HASH_PRIVATE_KECCAK_SPONGE_WORDS*8*sizeof(uint8_t));
}


HASH_INLINE void hash_private_sha3_update(Hash_Private_Sha3* s, const uint8_t *data, size_t size)
{
    /* 0...7 -- how much is needed to have a word */
    size_t old_tail = (8 - s->byteIndex) & 7;

    if(size < old_tail)
    {        /* have no complete word or haven't started 
                                * the word yet */
        /* endian-independent code follows: */
        while (size--)
            s->saved |= (uint64_t) (*(data++)) << ((s->byteIndex++) * 8);
        return;
    }

    if(old_tail)
    {              /* will have one word to process */
        /* endian-independent code follows: */
        size -= old_tail;
        while (old_tail--)
            s->saved |= (uint64_t) (*(data++)) << ((s->byteIndex++) * 8);

        /* now ready to add saved to the sponge */
        s->u.s[s->wordIndex] ^= s->saved;
        s->byteIndex = 0;
        s->saved = 0;
        if(++s->wordIndex == (HASH_PRIVATE_KECCAK_SPONGE_WORDS - hash_private_sha3_cw(s->capacityWords)))
        {
            hash_private_sha3_keccakf(s->u.s);
            s->wordIndex = 0;
        }
    }


    size_t words = size / sizeof(uint64_t);
    size_t tail = size - words * sizeof(uint64_t);

    for(size_t i = 0; i < words; i++, data += sizeof(uint64_t))
    {
        const uint64_t t = (uint64_t) (data[0]) |
                ((uint64_t) (data[1]) << 8 * 1) |
                ((uint64_t) (data[2]) << 8 * 2) |
                ((uint64_t) (data[3]) << 8 * 3) |
                ((uint64_t) (data[4]) << 8 * 4) |
                ((uint64_t) (data[5]) << 8 * 5) |
                ((uint64_t) (data[6]) << 8 * 6) |
                ((uint64_t) (data[7]) << 8 * 7);

        s->u.s[s->wordIndex] ^= t;
        if(++s->wordIndex == (HASH_PRIVATE_KECCAK_SPONGE_WORDS - hash_private_sha3_cw(s->capacityWords)))
        {
            hash_private_sha3_keccakf(s->u.s);
            s->wordIndex = 0;
        }
    }


    while (tail--)
    {
        s->saved |= (uint64_t) (*(data++)) << ((s->byteIndex++) * 8);
    }
}


HASH_INLINE void hash_private_sha3_finalize(Hash_Private_Sha3* s)
{
    /* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
    * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
    * Overall, we feed 0, then 1, and finally 1 to start padding. Without
    * M || 01, we would simply use 1 to start padding. */

    uint64_t t;

    //if(s->capacityWords & UseKeccakFlag)
    //{
    //    /* Keccak version */
    //    t = (uint64_t)(((uint64_t) 1) << (s->byteIndex * 8));
    //}
    //else
    //{
        /* SHA3 version */
        t = (uint64_t)(((uint64_t)(0x02 | (1 << 2))) << ((s->byteIndex) * 8));
    //}

    s->u.s[s->wordIndex] ^= s->saved ^ t;

    s->u.s[HASH_PRIVATE_KECCAK_SPONGE_WORDS - hash_private_sha3_cw(s->capacityWords) - 1] ^= UINT64_C(0x8000000000000000);
    hash_private_sha3_keccakf(s->u.s);

    // This conversion is not needed for little-endian platforms
    if (!hash_util_is_little_endian())
    {
        uint32_t i;
        for(i = 0; i < HASH_PRIVATE_KECCAK_SPONGE_WORDS; i++)
        {
            const uint32_t t1 = (uint32_t) s->u.s[i];
            const uint32_t t2 = (uint32_t) ((s->u.s[i] >> 16) >> 16);
            s->u.sb[i * 8 + 0] = (uint8_t) (t1);
            s->u.sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
            s->u.sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
            s->u.sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
            s->u.sb[i * 8 + 4] = (uint8_t) (t2);
            s->u.sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
            s->u.sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
            s->u.sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
        }
    }
}


// ==============================Hash_Sha3_224==================================
typedef Hash_Private_Sha3 Hash_Sha3_224[1];
HASH_INLINE void hash_sha3_224_init(Hash_Sha3_224 s)
{
    hash_private_sha3_init(s, 224);
}

HASH_INLINE void hash_sha3_224_update_binary(Hash_Sha3_224 s, const char* data, size_t size)
{
    hash_private_sha3_update(s, (const uint8_t*)data, size);
}

HASH_INLINE void hash_sha3_224_update(Hash_Sha3_224 s, const char* data)
{
    hash_private_sha3_update(s, (const uint8_t*)data, strlen(data));
}

HASH_INLINE void hash_sha3_224_finalize(Hash_Sha3_224 s)
{
    hash_private_sha3_finalize(s);
}

// if buffer == NULL returns internal buffer, buffer size must be at least 57
HASH_INLINE const char* hash_sha3_224_hexdigest(const Hash_Sha3_224 s, char* buffer)
{
    static char hex[HASH_SHA3_224_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < HASH_SHA3_224_SIZE/2; i++)
    {
        snprintf(&buff[i*2], HASH_SHA3_224_SIZE+1, "%02" PRIx32, (uint32_t)((uint8_t*)s->u.s)[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 57
HASH_INLINE const char* hash_sha3_224_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha3_224 s;
    hash_sha3_224_init(s);
    hash_sha3_224_update_binary(s, str, size);
    hash_sha3_224_finalize(s);
    return hash_sha3_224_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha3_224(const char* str, char* buffer)
{
    return hash_sha3_224_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha3_224_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha3_224 m;
    return hash_util_hash_file(path, mode, m, buffer, (hash_util_init_func)hash_sha3_224_init, (hash_util_update_func)hash_sha3_224_update_binary, (hash_util_finalize_func)hash_sha3_224_finalize, (hash_util_hexdigest_func)hash_sha3_224_hexdigest);
}

HASH_INLINE const char* hash_sha3_224_easy(const char* str)
{
    return hash_sha3_224_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha3_224_file_easy(const char* path, const char* mode)
{
    return hash_sha3_224_file(path, mode, NULL);
}
// ==============================Hash_Sha3_224==================================


// ==============================Hash_Sha3_256==================================
typedef Hash_Private_Sha3 Hash_Sha3_256[1];
HASH_INLINE void hash_sha3_256_init(Hash_Sha3_256 s)
{
    hash_private_sha3_init(s, 256);
}

HASH_INLINE void hash_sha3_256_update_binary(Hash_Sha3_256 s, const char* data, size_t size)
{
    hash_private_sha3_update(s, (const uint8_t*)data, size);
}

HASH_INLINE void hash_sha3_256_update(Hash_Sha3_256 s, const char* data)
{
    hash_private_sha3_update(s, (const uint8_t*)data, strlen(data));
}

HASH_INLINE void hash_sha3_256_finalize(Hash_Sha3_256 s)
{
    hash_private_sha3_finalize(s);
}

// if buffer == NULL returns internal buffer, buffer size must be at least 65
HASH_INLINE const char* hash_sha3_256_hexdigest(const Hash_Sha3_256 s, char* buffer)
{
    static char hex[HASH_SHA3_256_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < HASH_SHA3_256_SIZE/2; i++)
    {
        snprintf(&buff[i*2], HASH_SHA3_256_SIZE+1, "%02" PRIx32, (uint32_t)((uint8_t*)s->u.s)[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 65
HASH_INLINE const char* hash_sha3_256_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha3_256 s;
    hash_sha3_256_init(s);
    hash_sha3_256_update_binary(s, str, size);
    hash_sha3_256_finalize(s);
    return hash_sha3_256_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha3_256(const char* str, char* buffer)
{
    return hash_sha3_256_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha3_256_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha3_256 m;
    return hash_util_hash_file(path, mode, m, buffer, (hash_util_init_func)hash_sha3_256_init, (hash_util_update_func)hash_sha3_256_update_binary, (hash_util_finalize_func)hash_sha3_256_finalize, (hash_util_hexdigest_func)hash_sha3_256_hexdigest);
}

HASH_INLINE const char* hash_sha3_256_easy(const char* str)
{
    return hash_sha3_256_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha3_256_file_easy(const char* path, const char* mode)
{
    return hash_sha3_256_file(path, mode, NULL);
}
// ==============================Hash_Sha3_256==================================


// ==============================Hash_Sha3_384==================================
typedef Hash_Private_Sha3 Hash_Sha3_384[1];
HASH_INLINE void hash_sha3_384_init(Hash_Sha3_384 s)
{
    hash_private_sha3_init(s, 384);
}

HASH_INLINE void hash_sha3_384_update_binary(Hash_Sha3_384 s, const char* data, size_t size)
{
    hash_private_sha3_update(s, (const uint8_t*)data, size);
}

HASH_INLINE void hash_sha3_384_update(Hash_Sha3_384 s, const char* data)
{
    hash_private_sha3_update(s, (const uint8_t*)data, strlen(data));
}

HASH_INLINE void hash_sha3_384_finalize(Hash_Sha3_384 s)
{
    hash_private_sha3_finalize(s);
}

// if buffer == NULL returns internal buffer, buffer size must be at least 97
HASH_INLINE const char* hash_sha3_384_hexdigest(const Hash_Sha3_384 s, char* buffer)
{
    static char hex[HASH_SHA3_384_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < HASH_SHA3_384_SIZE/2; i++)
    {
        snprintf(&buff[i*2], HASH_SHA3_384_SIZE+1, "%02" PRIx32, (uint32_t)((uint8_t*)s->u.s)[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 97
HASH_INLINE const char* hash_sha3_384_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha3_384 s;
    hash_sha3_384_init(s);
    hash_sha3_384_update_binary(s, str, size);
    hash_sha3_384_finalize(s);
    return hash_sha3_384_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha3_384(const char* str, char* buffer)
{
    return hash_sha3_384_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha3_384_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha3_384 m;
    return hash_util_hash_file(path, mode, m, buffer, (hash_util_init_func)hash_sha3_384_init, (hash_util_update_func)hash_sha3_384_update_binary, (hash_util_finalize_func)hash_sha3_384_finalize, (hash_util_hexdigest_func)hash_sha3_384_hexdigest);
}

HASH_INLINE const char* hash_sha3_384_easy(const char* str)
{
    return hash_sha3_384_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha3_384_file_easy(const char* path, const char* mode)
{
    return hash_sha3_384_file(path, mode, NULL);
}
// ==============================Hash_Sha3_384==================================


// ==============================Hash_Sha3_512==================================
typedef Hash_Private_Sha3 Hash_Sha3_512[1];
HASH_INLINE void hash_sha3_512_init(Hash_Sha3_512 s)
{
    hash_private_sha3_init(s, 512);
}

HASH_INLINE void hash_sha3_512_update_binary(Hash_Sha3_512 s, const char* data, size_t size)
{
    hash_private_sha3_update(s, (const uint8_t*)data, size);
}

HASH_INLINE void hash_sha3_512_update(Hash_Sha3_512 s, const char* data)
{
    hash_private_sha3_update(s, (const uint8_t*)data, strlen(data));
}

HASH_INLINE void hash_sha3_512_finalize(Hash_Sha3_512 s)
{
    hash_private_sha3_finalize(s);
}

// if buffer == NULL returns internal buffer, buffer size must be at least 129
HASH_INLINE const char* hash_sha3_512_hexdigest(const Hash_Sha3_512 s, char* buffer)
{
    static char hex[HASH_SHA3_512_SIZE+1];
    char* buff = buffer == NULL ? hex : buffer;
    for (size_t i = 0; i < HASH_SHA3_512_SIZE/2; i++)
    {
        snprintf(&buff[i*2], HASH_SHA3_512_SIZE+1, "%02" PRIx32, (uint32_t)((uint8_t*)s->u.s)[i]);
    }
    return buff;
}

// if buffer == NULL returns internal buffer, buffer size must be at least 129
HASH_INLINE const char* hash_sha3_512_binary(const char* str, size_t size, char* buffer)
{
    Hash_Sha3_512 s;
    hash_sha3_512_init(s);
    hash_sha3_512_update_binary(s, str, size);
    hash_sha3_512_finalize(s);
    return hash_sha3_512_hexdigest(s, buffer);
}

HASH_INLINE const char* hash_sha3_512(const char* str, char* buffer)
{
    return hash_sha3_512_binary(str, strlen(str), buffer);
}

HASH_INLINE const char* hash_sha3_512_file(const char* path, const char* mode, char* buffer)
{
    Hash_Sha3_512 m;
    return hash_util_hash_file(path, mode, m, buffer, (hash_util_init_func)hash_sha3_512_init, (hash_util_update_func)hash_sha3_512_update_binary, (hash_util_finalize_func)hash_sha3_512_finalize, (hash_util_hexdigest_func)hash_sha3_512_hexdigest);
}

HASH_INLINE const char* hash_sha3_512_easy(const char* str)
{
    return hash_sha3_512_binary(str, strlen(str), NULL);
}

HASH_INLINE const char* hash_sha3_512_file_easy(const char* path, const char* mode)
{
    return hash_sha3_512_file(path, mode, NULL);
}
// ==============================Hash_Sha3_512==================================
// ================================Hash_Sha3====================================
#endif // HASH_ENABLE_SHA3
#endif // HASH_ENABLE_C_INTERFACE


#if defined(__cplusplus) && HASH_ENABLE_CPP_INTERFACE == 1
namespace Hash
{
    namespace Encode
    {
        // C++11 - 17 only | for u16 or u32 strings/string literals
        template <class String>
        inline std::string ToUtf8String(const String& str)
        {
            // Create a locale that uses the codecvt_utf8 facet
            //std::locale loc(std::locale(), new std::codecvt_utf8<char>());
            // Create a wstring_convert object using the locale
            std::wstring_convert<std::codecvt_utf8<typename String::value_type>, typename String::value_type> convert;
            // Decode the string as a sequence of UTF-8 code points
            return convert.to_bytes(str);
        }


        // only for strings with characters from -127 - 127
        // encode iso-8859-1
        inline std::string Iso88591ToUtf8(const std::string_view& str)
        {
            std::string strOut;
            for (std::string_view::const_iterator it = str.begin(); it != str.end(); ++it)
            {
                std::uint8_t ch = *it;
                if (ch < 0x80)
                {
                    strOut.push_back(ch);
                }
                else
                {
                    strOut.push_back(0xc0 | ch >> 6);
                    strOut.push_back(0x80 | (ch & 0x3f));
                }
            }
            return strOut;
        }
    }


    namespace File
    {
        // buffered file hashing, returns empty string on failure
        template <typename Hasher>
        inline std::string HashFile(std::string_view path, std::ios::openmode flag = std::ios::binary)
        {
            std::ifstream file(path.data(), flag);
            if (!file.is_open())
                return std::string();

            Hasher hasher;
            char buffer[HASH_FILE_READ_BUFFER_SIZE];

            do
            {
                file.read(buffer, HASH_FILE_READ_BUFFER_SIZE);
                hasher.Update(buffer, file.gcount());
            } while (file.gcount() > 0);

            hasher.Finalize();
            return hasher.Hexdigest();
        }
    }


    namespace Util
    {
        inline std::string CharArrayToHexString(unsigned char* data, std::size_t size)
        {
            static constexpr char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
            std::string string;
            for (std::size_t i = 0; i < size; ++i)
            {
                const char byte = data[i];

                string += hex_chars[(byte & 0xF0) >> 4];
                string += hex_chars[(byte & 0x0F) >> 0];
            }
            return string;
        }


        inline std::string LoadFile(const char* path, std::ios::openmode flag) // std::ios::binary
        {
            std::ifstream infile(path, flag);
            if (!infile.is_open())
                return "";

            infile.seekg(0, std::ios::end);
            std::size_t size = static_cast<std::size_t>(infile.tellg());
            infile.seekg(0, std::ios::beg);

            std::string data(size, ' ');
            infile.read(data.data(), size);
            return data;
        }


        template <typename T>
        constexpr inline T SwapEndian(T u) noexcept
        {
            static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
            static_assert(std::is_integral_v<T>, "SwapEndian requires integral type");

            #if defined(__clang__) || defined(__GNUC__)
                if constexpr (sizeof(T) == 1)
                {
                    return u; // no swap needed for 1 byte
                }
                else if constexpr (sizeof(T) == 2)
                {
                    return __builtin_bswap16(u);
                }
                else if constexpr (sizeof(T) == 4)
                {
                    return __builtin_bswap32(u);
                }
                else if constexpr (sizeof(T) == 8)
                {
                    return __builtin_bswap64(u);
                }
                else
                {
                    // fallback for uncommon sizes
                }
            #elif defined(_MSC_VER)
                if constexpr (sizeof(T) == 1)
                {
                    return u; // no swap needed
                }
                else if constexpr (sizeof(T) == 2)
                {
                    return static_cast<T>(_byteswap_ushort(static_cast<std::uint16_t>(u)));
                }
                else if constexpr (sizeof(T) == 4)
                {
                    return static_cast<T>(_byteswap_ulong(static_cast<std::uint32_t>(u)));
                }
                else if constexpr (sizeof(T) == 8)
                {
                    return static_cast<T>(_byteswap_uint64(static_cast<std::uint64_t>(u)));
                }
                else
                {
                    // fallback for uncommon sizes
                }
            #endif
            // Generic fallback - reverse bytes manually
            union
            {
                T u;
                std::uint8_t u8[sizeof(T)];
            } source, dest;

            source.u = u;
            for (std::size_t k = 0; k < sizeof(T); ++k)
            {
                dest.u8[k] = source.u8[sizeof(T) - k - 1];
            }
            return dest.u;
        }


        constexpr inline bool IsLittleEndian() noexcept
        {
            #if defined(_MSC_VER)
                return true; // Windows x86/x64 is always little endian
            #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                return true;
            #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                return false;
            #else
                static_assert(false, "Unknown endianness");
                return false;
            #endif
        }

        template <typename T> constexpr inline T RightRotate(T n, std::size_t c) noexcept
        {
            //const unsigned int mask = (CHAR_BIT * sizeof(n) - 1); // doesn't loose bits
            //c &= mask;
            //return (n >> c) | (n << ((-c) & mask));
            return (n >> c) | (n << (std::numeric_limits<T>::digits - c));
        }

        template <typename T> constexpr inline T LeftRotate(T n, std::size_t c) noexcept
        {
            //const unsigned int mask = (CHAR_BIT * sizeof(n) - 1); // doesn't loose bits
            //c &= mask;
            //return (n << c) | (n >> ((-c) & mask));
            return (n << c) | (n >> (std::numeric_limits<T>::digits - c));
        }
    }

#if HASH_ENABLE_SHA2 == 1
    class Sha256
    {
    public:
        static constexpr std::size_t Size = 64;
    private:
        static constexpr int BufferSize = 64;
        std::uint64_t m_Bitlen = 0;
        int           m_BufferSize = 0;
        std::uint8_t  m_Buffer[BufferSize];
    protected:
        // FracPartsSqareRoots
        std::uint32_t m_H[8] =
        {
           UINT32_C(0x6a09e667),
           UINT32_C(0xbb67ae85),
           UINT32_C(0x3c6ef372),
           UINT32_C(0xa54ff53a),
           UINT32_C(0x510e527f),
           UINT32_C(0x9b05688c),
           UINT32_C(0x1f83d9ab),
           UINT32_C(0x5be0cd19)
        };
    private:
        // FracPartsCubeRoots
        static constexpr std::uint32_t s_K[64] =
        {
            UINT32_C(0x428a2f98), UINT32_C(0x71374491), UINT32_C(0xb5c0fbcf), UINT32_C(0xe9b5dba5), UINT32_C(0x3956c25b), UINT32_C(0x59f111f1), UINT32_C(0x923f82a4), UINT32_C(0xab1c5ed5),
            UINT32_C(0xd807aa98), UINT32_C(0x12835b01), UINT32_C(0x243185be), UINT32_C(0x550c7dc3), UINT32_C(0x72be5d74), UINT32_C(0x80deb1fe), UINT32_C(0x9bdc06a7), UINT32_C(0xc19bf174),
            UINT32_C(0xe49b69c1), UINT32_C(0xefbe4786), UINT32_C(0x0fc19dc6), UINT32_C(0x240ca1cc), UINT32_C(0x2de92c6f), UINT32_C(0x4a7484aa), UINT32_C(0x5cb0a9dc), UINT32_C(0x76f988da),
            UINT32_C(0x983e5152), UINT32_C(0xa831c66d), UINT32_C(0xb00327c8), UINT32_C(0xbf597fc7), UINT32_C(0xc6e00bf3), UINT32_C(0xd5a79147), UINT32_C(0x06ca6351), UINT32_C(0x14292967),
            UINT32_C(0x27b70a85), UINT32_C(0x2e1b2138), UINT32_C(0x4d2c6dfc), UINT32_C(0x53380d13), UINT32_C(0x650a7354), UINT32_C(0x766a0abb), UINT32_C(0x81c2c92e), UINT32_C(0x92722c85),
            UINT32_C(0xa2bfe8a1), UINT32_C(0xa81a664b), UINT32_C(0xc24b8b70), UINT32_C(0xc76c51a3), UINT32_C(0xd192e819), UINT32_C(0xd6990624), UINT32_C(0xf40e3585), UINT32_C(0x106aa070),
            UINT32_C(0x19a4c116), UINT32_C(0x1e376c08), UINT32_C(0x2748774c), UINT32_C(0x34b0bcb5), UINT32_C(0x391c0cb3), UINT32_C(0x4ed8aa4a), UINT32_C(0x5b9cca4f), UINT32_C(0x682e6ff3),
            UINT32_C(0x748f82ee), UINT32_C(0x78a5636f), UINT32_C(0x84c87814), UINT32_C(0x8cc70208), UINT32_C(0x90befffa), UINT32_C(0xa4506ceb), UINT32_C(0xbef9a3f7), UINT32_C(0xc67178f2)
        };
    private:
        void Compress(const std::uint32_t* const w) noexcept
        {
            std::uint32_t a = m_H[0];
            std::uint32_t b = m_H[1];
            std::uint32_t c = m_H[2];
            std::uint32_t d = m_H[3];
            std::uint32_t e = m_H[4];
            std::uint32_t f = m_H[5];
            std::uint32_t g = m_H[6];
            std::uint32_t h = m_H[7];
            for (std::size_t i = 0; i < 64; ++i)
            {
                const std::uint32_t s1 = Util::RightRotate(e, 6) ^ Util::RightRotate(e, 11) ^ Util::RightRotate(e, 25);
                const std::uint32_t ch = (e & f) ^ (~e & g);
                const std::uint32_t temp1 = h + s1 + ch + s_K[i] + w[i];
                const std::uint32_t s0 = Util::RightRotate(a, 2) ^ Util::RightRotate(a, 13) ^ Util::RightRotate(a, 22);
                const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                const std::uint32_t temp2 = s0 + maj;
                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            m_H[0] += a;
            m_H[1] += b;
            m_H[2] += c;
            m_H[3] += d;
            m_H[4] += e;
            m_H[5] += f;
            m_H[6] += g;
            m_H[7] += h;
        }


        void Transform() noexcept
        {
            std::uint32_t w[64];
            for (std::size_t i = 0; i < 16; ++i)
            {
                /*
                    This code does the same as the memcpy
                    std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(&w[i]);
                    ptr[0] = m_Buffer[4 * i];
                    ptr[1] = m_Buffer[4 * i + 1];
                    ptr[2] = m_Buffer[4 * i + 2];
                    ptr[3] = m_Buffer[4 * i + 3];
                */
                std::memcpy(&w[i], m_Buffer + 4 * i, sizeof(std::uint32_t));
                if constexpr (Util::IsLittleEndian())
                {
                    w[i] = Util::SwapEndian(w[i]);
                }
            }

            for (std::size_t i = 16; i < 64; ++i)
            {
                const std::uint32_t s0 = Util::RightRotate(w[i - 15], 7) ^ Util::RightRotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
                const std::uint32_t s1 = Util::RightRotate(w[i - 2], 17) ^ Util::RightRotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            Compress(w);
        }
    public:
        inline Sha256() noexcept = default;
        inline Sha256(std::uint32_t h0, std::uint32_t h1, std::uint32_t h2, std::uint32_t h3, std::uint32_t h4, std::uint32_t h5, std::uint32_t h6, std::uint32_t h7) noexcept : m_H{ h0, h1, h2, h3, h4, h5, h6, h7 } {}

        void Update(const std::uint8_t* data, std::size_t size) noexcept
        {
            /*
                The code below does the same as this loop just more efficient
                for (std::size_t i = 0; i < size; ++i)
                {
                    m_Buffer[m_BufferSize++] = data[i];
                    if (m_BufferSize == 64)
                    {
                        Transform();
                        m_BufferSize = 0;
                        m_Bitlen += 512;
                    }
                }
            */
            if (m_BufferSize)
            {
                std::size_t toCopy = std::min<std::size_t>(BufferSize - m_BufferSize, size);
                std::memcpy(m_Buffer + m_BufferSize, data, toCopy);
                m_BufferSize += toCopy;
                data += toCopy;
                size -= toCopy;

                if (m_BufferSize == BufferSize)
                {
                    Transform();
                    m_Bitlen += 512;
                    m_BufferSize = 0;
                }
            }

            while (size >= BufferSize)
            {
                std::memcpy(m_Buffer, data, BufferSize);
                Transform();
                m_Bitlen += 512;
                data += BufferSize;
                size -= BufferSize;
            }

            if (size > 0)
            {
                std::memcpy(m_Buffer, data, size);
                m_BufferSize = size;
            }
        }

        inline void Update(const char* data, std::size_t size) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data), size);
        }

        inline void Update(std::string_view data) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        }


        void Finalize() noexcept
        {
            std::uint8_t start = m_BufferSize;
            std::uint8_t end = m_BufferSize < 56 ? 56 : 64;

            m_Buffer[start++] = 0b10000000;
            std::memset(&m_Buffer[start], 0, end - start);

            if (m_BufferSize >= 56)
            {
                Transform();
                std::memset(m_Buffer, 0, 56);
            }

            m_Bitlen += m_BufferSize * 8;
            std::uint64_t* const size = reinterpret_cast<std::uint64_t*>(&m_Buffer[64 - 8]);
            *size = Util::IsLittleEndian() ? Util::SwapEndian(m_Bitlen) : m_Bitlen;
            Transform();
        }


        inline std::string Hexdigest() const
        {
            char buff[Size+1];
            for (int i = 0; i < 8; ++i)
            {
                std::snprintf(&buff[i * 8], Size+1, "%08" PRIx32, m_H[i]);
            }
            return std::string(buff);
        }
    };


    // if you have any kind of unicode string, use the Hash::encode functions beforehand to convert the string
    inline std::string sha256(const char* str, std::size_t size)
    {
        Sha256 s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha256(std::string_view str)
    {
        return sha256(str.data(), str.size());
    }




    class Sha224 : public Sha256
    {
    public:
        static constexpr std::size_t Size = 56;
    public:
        inline Sha224() noexcept : Sha256(UINT32_C(0xC1059ED8), UINT32_C(0x367CD507), UINT32_C(0x3070DD17), UINT32_C(0xF70E5939), UINT32_C(0xFFC00B31), UINT32_C(0x68581511), UINT32_C(0x64F98FA7), UINT32_C(0xBEFA4FA4)) {}
        inline std::string Hexdigest() const
        {
            char buff[Size+1];
            for (int i = 0; i < 7; ++i)
            {
                std::snprintf(&buff[i * 8], Size+1, "%08" PRIx32, m_H[i]);
            }
            return std::string(buff);
        }
    };


    // if you have any kind of unicode string, use the Hash::encode functions beforehand to convert the string
    inline std::string sha224(const char* str, std::size_t size)
    {
        Sha224 s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha224(std::string_view str)
    {
        return sha224(str.data(), str.size());
    }




    class Sha512
    {
    public:
        static constexpr std::size_t Size = 128;
    private:
        std::uint64_t m_Bitlen = 0;
        std::uint8_t m_BufferSize = 0;
        std::uint8_t m_Buffer[128];
    protected:
        std::uint64_t m_H[8] =
        {
            UINT64_C(0x6a09e667f3bcc908),
            UINT64_C(0xbb67ae8584caa73b),
            UINT64_C(0x3c6ef372fe94f82b),
            UINT64_C(0xa54ff53a5f1d36f1),
            UINT64_C(0x510e527fade682d1),
            UINT64_C(0x9b05688c2b3e6c1f),
            UINT64_C(0x1f83d9abfb41bd6b),
            UINT64_C(0x5be0cd19137e2179)
        };
    private:
        // round constants
        static constexpr std::uint64_t s_K[80] =
        {
            UINT64_C(0x428a2f98d728ae22), UINT64_C(0x7137449123ef65cd), UINT64_C(0xb5c0fbcfec4d3b2f), UINT64_C(0xe9b5dba58189dbbc), UINT64_C(0x3956c25bf348b538),
            UINT64_C(0x59f111f1b605d019), UINT64_C(0x923f82a4af194f9b), UINT64_C(0xab1c5ed5da6d8118), UINT64_C(0xd807aa98a3030242), UINT64_C(0x12835b0145706fbe),
            UINT64_C(0x243185be4ee4b28c), UINT64_C(0x550c7dc3d5ffb4e2), UINT64_C(0x72be5d74f27b896f), UINT64_C(0x80deb1fe3b1696b1), UINT64_C(0x9bdc06a725c71235),
            UINT64_C(0xc19bf174cf692694), UINT64_C(0xe49b69c19ef14ad2), UINT64_C(0xefbe4786384f25e3), UINT64_C(0x0fc19dc68b8cd5b5), UINT64_C(0x240ca1cc77ac9c65),
            UINT64_C(0x2de92c6f592b0275), UINT64_C(0x4a7484aa6ea6e483), UINT64_C(0x5cb0a9dcbd41fbd4), UINT64_C(0x76f988da831153b5), UINT64_C(0x983e5152ee66dfab),
            UINT64_C(0xa831c66d2db43210), UINT64_C(0xb00327c898fb213f), UINT64_C(0xbf597fc7beef0ee4), UINT64_C(0xc6e00bf33da88fc2), UINT64_C(0xd5a79147930aa725),
            UINT64_C(0x06ca6351e003826f), UINT64_C(0x142929670a0e6e70), UINT64_C(0x27b70a8546d22ffc), UINT64_C(0x2e1b21385c26c926), UINT64_C(0x4d2c6dfc5ac42aed),
            UINT64_C(0x53380d139d95b3df), UINT64_C(0x650a73548baf63de), UINT64_C(0x766a0abb3c77b2a8), UINT64_C(0x81c2c92e47edaee6), UINT64_C(0x92722c851482353b),
            UINT64_C(0xa2bfe8a14cf10364), UINT64_C(0xa81a664bbc423001), UINT64_C(0xc24b8b70d0f89791), UINT64_C(0xc76c51a30654be30), UINT64_C(0xd192e819d6ef5218),
            UINT64_C(0xd69906245565a910), UINT64_C(0xf40e35855771202a), UINT64_C(0x106aa07032bbd1b8), UINT64_C(0x19a4c116b8d2d0c8), UINT64_C(0x1e376c085141ab53),
            UINT64_C(0x2748774cdf8eeb99), UINT64_C(0x34b0bcb5e19b48a8), UINT64_C(0x391c0cb3c5c95a63), UINT64_C(0x4ed8aa4ae3418acb), UINT64_C(0x5b9cca4f7763e373),
            UINT64_C(0x682e6ff3d6b2b8a3), UINT64_C(0x748f82ee5defb2fc), UINT64_C(0x78a5636f43172f60), UINT64_C(0x84c87814a1f0ab72), UINT64_C(0x8cc702081a6439ec),
            UINT64_C(0x90befffa23631e28), UINT64_C(0xa4506cebde82bde9), UINT64_C(0xbef9a3f7b2c67915), UINT64_C(0xc67178f2e372532b), UINT64_C(0xca273eceea26619c),
            UINT64_C(0xd186b8c721c0c207), UINT64_C(0xeada7dd6cde0eb1e), UINT64_C(0xf57d4f7fee6ed178), UINT64_C(0x06f067aa72176fba), UINT64_C(0x0a637dc5a2c898a6),
            UINT64_C(0x113f9804bef90dae), UINT64_C(0x1b710b35131c471b), UINT64_C(0x28db77f523047d84), UINT64_C(0x32caab7b40c72493), UINT64_C(0x3c9ebe0a15c9bebc),
            UINT64_C(0x431d67c49c100d4c), UINT64_C(0x4cc5d4becb3e42b6), UINT64_C(0x597f299cfc657e2a), UINT64_C(0x5fcb6fab3ad6faec), UINT64_C(0x6c44198c4a475817)
        };
    private:
        inline void Compress(const std::uint64_t* const w) noexcept
        {
            std::uint64_t a = m_H[0];
            std::uint64_t b = m_H[1];
            std::uint64_t c = m_H[2];
            std::uint64_t d = m_H[3];
            std::uint64_t e = m_H[4];
            std::uint64_t f = m_H[5];
            std::uint64_t g = m_H[6];
            std::uint64_t h = m_H[7];
            for (std::size_t i = 0; i < 80; ++i)
            {
                const std::uint64_t s1 = Util::RightRotate(e, 14) ^ Util::RightRotate(e, 18) ^ Util::RightRotate(e, 41);
                const std::uint64_t ch = (e & f) ^ (~e & g);
                const std::uint64_t temp1 = h + s1 + ch + s_K[i] + w[i];
                const std::uint64_t s0 = Util::RightRotate(a, 28) ^ Util::RightRotate(a, 34) ^ Util::RightRotate(a, 39);
                const std::uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
                const std::uint64_t temp2 = s0 + maj;
                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            m_H[0] += a;
            m_H[1] += b;
            m_H[2] += c;
            m_H[3] += d;
            m_H[4] += e;
            m_H[5] += f;
            m_H[6] += g;
            m_H[7] += h;
        }


        inline void Transform() noexcept
        {
            std::uint64_t w[80];
            for (std::size_t i = 0; i < 16; ++i)
            {
                std::uint8_t* c = reinterpret_cast<std::uint8_t*>(&w[i]);
                c[0] = m_Buffer[8 * i];
                c[1] = m_Buffer[8 * i + 1];
                c[2] = m_Buffer[8 * i + 2];
                c[3] = m_Buffer[8 * i + 3];
                c[4] = m_Buffer[8 * i + 4];
                c[5] = m_Buffer[8 * i + 5];
                c[6] = m_Buffer[8 * i + 6];
                c[7] = m_Buffer[8 * i + 7];
                w[i] = Util::IsLittleEndian() ? Util::SwapEndian(w[i]) : w[i];
            }

            for (std::size_t i = 16; i < 80; ++i) // Extend the first 16 words
            {
                const std::uint64_t s0 = Util::RightRotate(w[i - 15], 1) ^ Util::RightRotate(w[i - 15], 8) ^ (w[i - 15] >> 7);
                const std::uint64_t s1 = Util::RightRotate(w[i - 2], 19) ^ Util::RightRotate(w[i - 2], 61) ^ (w[i - 2] >> 6);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            Compress(w);
        }
    public:
        inline Sha512() noexcept = default;
        inline explicit Sha512(std::uint64_t h0, std::uint64_t h1, std::uint64_t h2, std::uint64_t h3, std::uint64_t h4, std::uint64_t h5, std::uint64_t h6, std::uint64_t h7) noexcept : m_H{ h0, h1, h2, h3, h4, h5, h6, h7 } {}

        inline void Reset() noexcept
        {
            m_Bitlen = 0;
            m_BufferSize = 0;
        }

        inline void Update(const std::uint8_t* data, std::size_t size) noexcept
        {
            for (std::size_t i = 0; i < size; ++i)
            {
                m_Buffer[m_BufferSize++] = data[i];
                if (m_BufferSize == 128)
                {
                    Transform();
                    m_BufferSize = 0;
                    m_Bitlen += 1024;
                }
            }
        }

        inline void Update(const char* data, std::size_t size) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data), size);
        }

        inline void Update(std::string_view data) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        }


        inline void Finalize() noexcept
        {
            std::uint8_t start = m_BufferSize;
            std::uint8_t end = m_BufferSize < 112 ? 120 : 128; // 120 instead of 112 because m_Bitlen is a 64 bit uint

            m_Buffer[start++] = 0b10000000;
            std::memset(&m_Buffer[start], 0, end - start);

            if (m_BufferSize >= 112)
            {
                Transform();
                std::memset(m_Buffer, 0, 120);
            }

            m_Bitlen += m_BufferSize * 8;
            std::uint64_t* const size = reinterpret_cast<std::uint64_t*>(&m_Buffer[128 - 8]); // -8 instead of -16 because we use an uint64 instead of uint128
            *size = Util::IsLittleEndian() ? Util::SwapEndian(m_Bitlen) : m_Bitlen;
            Transform();
        }


        inline std::string Hexdigest() const
        {
            char buff[Size+1];
            for (std::size_t i = 0; i < 8; ++i)
            {
                std::snprintf(&buff[i * 16], Size+1, "%016" PRIx64, m_H[i]);
            }
            return std::string(buff);
        }
    };


    // if you have any kind of unicode string, use the Hash::encode functions beforehand to convert the string
    inline std::string sha512(const char* str, std::size_t size)
    {
        Sha512 s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha512(std::string_view str)
    {
        return sha512(str.data(), str.size());
    }




    class Sha512T : public Sha512
    {
    private:
        std::size_t m_T;
    public:
        inline explicit Sha512T(std::size_t t) : Sha512(UINT64_C(0xcfac43c256196cad), UINT64_C(0x1ec20b20216f029e), UINT64_C(0x99cb56d75b315d8e), UINT64_C(0x00ea509ffab89354), UINT64_C(0xf4abf7da08432774), UINT64_C(0x3ea0cd298e9bc9ba), UINT64_C(0xba267c0e5ee418ce), UINT64_C(0xfe4568bcb6db84dc)), m_T(t)
        {
            assert(t != 384 && "t = 384 is not allowed use Sha384 instead!");
            assert(t >= 4 && t <= 2048 && "t must satisfy t >= 4 && t <= 2048!");

            std::string s = "SHA-512/" + std::to_string(m_T);
            Update(s);
            Finalize();
            s = Sha512::Hexdigest();
            Reset();

            std::size_t k = 0;
            for (std::size_t i = 0; i < s.size(); i += 16)
            {
                std::stringstream ss;
                ss << std::hex << std::string_view(&s.c_str()[i], 16);
                ss >> m_H[k++];
            }
        }

        inline std::string Hexdigest() const
        {
            return Sha512::Hexdigest().substr(0, m_T / 4);
        }
    };


    template <std::size_t T>
    class Sha512_T : public Sha512T
    {
        static_assert(T != 384, "T = 384 is not allowed use Sha384 instead!");
        static_assert(T >= 4 && T <= 2048 * 4, "T must satisfy T >= 4 && T <= 2048!");
    public:
        inline Sha512_T() : Sha512T(T) {}
    };
    using Sha512_224 = Sha512_T<224>;
    using Sha512_256 = Sha512_T<256>;


    inline std::string sha512t(std::size_t t, const char* str, std::size_t size)
    {
        Sha512T s(t);
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha512t(std::size_t t, std::string_view str)
    {
        return sha512t(t, str.data(), str.size());
    }

    template <std::size_t T> inline std::string sha512t(const char* str, std::size_t size)
    {
        Sha512_T<T> s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    template <std::size_t T> inline std::string sha512t(std::string_view str)
    {
        return sha512t<T>(str.data(), str.size());
    }

    inline std::string sha512_224(std::string_view str) { return sha512t<224>(str.data(), str.size()); }
    inline std::string sha512_256(std::string_view str) { return sha512t<256>(str.data(), str.size()); }
    inline std::string sha512_224(const char* str, std::size_t size) { return sha512t<224>(str, size); }
    inline std::string sha512_256(const char* str, std::size_t size) { return sha512t<256>(str, size); }

    namespace File
    {
        inline std::string sha512t(std::size_t t, const char* path, std::ios::openmode flag = std::ios::binary) { return Hash::sha512t(t, Util::LoadFile(path, flag)); }
        inline std::string sha512t(std::size_t t, std::string_view path, std::ios::openmode flag = std::ios::binary) { return Hash::sha512t(t, Util::LoadFile(path.data(), flag)); }
    }




    class Sha384 : public Sha512
    {
    public:
        static constexpr std::size_t Size = 96;
    public:
        inline Sha384() noexcept : Sha512(UINT64_C(0xcbbb9d5dc1059ed8), UINT64_C(0x629a292a367cd507), UINT64_C(0x9159015a3070dd17), UINT64_C(0x152fecd8f70e5939), UINT64_C(0x67332667ffc00b31), UINT64_C(0x8eb44a8768581511), UINT64_C(0xdb0c2e0d64f98fa7), UINT64_C(0x47b5481dbefa4fa4)) {}
        inline std::string Hexdigest() const
        {
            char buff[Size+1];
            for (std::size_t i = 0; i < 6; ++i)
            {
                std::snprintf(&buff[i * 16], Size+1, "%016" PRIx64, m_H[i]);
            }
            return std::string(buff);
        }
    };


    // if you have any kind of unicode string, use the Hash::encode functions beforehand to convert the string
    inline std::string sha384(const char* str, std::size_t size)
    {
        Sha384 s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha384(std::string_view str)
    {
        return sha384(str.data(), str.size());
    }
#endif // HASH_ENABLE_SHA2



#if HASH_ENABLE_SHA1 == 1
    class Sha1
    {
    public:
        static constexpr std::size_t Size = 40;
    private:
        static constexpr int BufferSize = 64;
        std::size_t  m_Bitlen = 0;
        int          m_BufferSize = 0;
        std::uint8_t m_Buffer[BufferSize];

        std::uint32_t m_H[5] =
        {
            UINT32_C(0x67452301),
            UINT32_C(0xEFCDAB89),
            UINT32_C(0x98BADCFE),
            UINT32_C(0x10325476),
            UINT32_C(0xC3D2E1F0)
        };
    private:
        void Transform() noexcept
        {
            std::uint32_t w[80];
            for (int i = 0; i < 16; ++i)
            {
                /*
                    This code does the same as the memcpy
                    std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(&w[i]);
                    ptr[0] = m_Buffer[4 * i];
                    ptr[1] = m_Buffer[4 * i + 1];
                    ptr[2] = m_Buffer[4 * i + 2];
                    ptr[3] = m_Buffer[4 * i + 3];
                */
                std::memcpy(&w[i], m_Buffer + 4 * i, sizeof(std::uint32_t));
                if constexpr (Util::IsLittleEndian())
                {
                    w[i] = Util::SwapEndian(w[i]);
                }
            }

            #if HASH_ENABLE_LOOP_UNROLLING
                for (int i = 16; i < 80; i += 8)
                {
                    w[i] = (Util::LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1));
                    w[i+1] = (Util::LeftRotate(w[i+1 - 3] ^ w[i+1 - 8] ^ w[i+1 - 14] ^ w[i+1 - 16], 1));
                    w[i+2] = (Util::LeftRotate(w[i+2 - 3] ^ w[i+2 - 8] ^ w[i+2 - 14] ^ w[i+2 - 16], 1));
                    w[i+3] = (Util::LeftRotate(w[i+3 - 3] ^ w[i+3 - 8] ^ w[i+3 - 14] ^ w[i+3 - 16], 1));
                    w[i+4] = (Util::LeftRotate(w[i+4 - 3] ^ w[i+4 - 8] ^ w[i+4 - 14] ^ w[i+4 - 16], 1));
                    w[i+5] = (Util::LeftRotate(w[i+5 - 3] ^ w[i+5 - 8] ^ w[i+5 - 14] ^ w[i+5 - 16], 1));
                    w[i+6] = (Util::LeftRotate(w[i+6 - 3] ^ w[i+6 - 8] ^ w[i+6 - 14] ^ w[i+6 - 16], 1));
                    w[i+7] = (Util::LeftRotate(w[i+7 - 3] ^ w[i+7 - 8] ^ w[i+7 - 14] ^ w[i+7 - 16], 1));
                }
            #else
                for (int i = 16; i < 80; ++i)
                {
                    w[i] = (Util::LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1));
                }
            #endif


            std::uint32_t a = m_H[0];
            std::uint32_t b = m_H[1];
            std::uint32_t c = m_H[2];
            std::uint32_t d = m_H[3];
            std::uint32_t e = m_H[4];
            std::uint32_t k, f;

            for (int i = 0; i < 20; ++i)
            {
                f = (b & c) | ((~b) & d);
                k = UINT32_C(0x5A827999);

                const std::uint32_t tmp = Util::LeftRotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = Util::LeftRotate(b, 30);
                b = a;
                a = tmp;
            }

            for (int i = 20; i < 40; ++i)
            {
                f = b ^ c ^ d;
                k = UINT32_C(0x6ED9EBA1);

                const std::uint32_t tmp = Util::LeftRotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = Util::LeftRotate(b, 30);
                b = a;
                a = tmp;
            }

            for (int i = 40; i < 60; ++i)
            {
                f = (b & c) | (b & d) | (c & d);
                k = UINT32_C(0x8F1BBCDC);

                const std::uint32_t tmp = Util::LeftRotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = Util::LeftRotate(b, 30);
                b = a;
                a = tmp;
            }

            for (int i = 60; i < 80; ++i)
            {
                f = b ^ c ^ d;
                k = UINT32_C(0xCA62C1D6);

                const std::uint32_t tmp = Util::LeftRotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = Util::LeftRotate(b, 30);
                b = a;
                a = tmp;
            }

            m_H[0] = m_H[0] + a;
            m_H[1] = m_H[1] + b;
            m_H[2] = m_H[2] + c;
            m_H[3] = m_H[3] + d;
            m_H[4] = m_H[4] + e;
        }
    public:
        void Update(const std::uint8_t* data, std::size_t size) noexcept
        {
            /*
                The code below does the same as this loop just more efficient
                for (std::size_t i = 0; i < size; ++i)
                {
                    m_Buffer[m_BufferSize++] = data[i];
                    if (m_BufferSize == 64)
                    {
                        Transform();
                        m_BufferSize = 0;
                        m_Bitlen += 512;
                    }
                }
            */
            if (m_BufferSize)
            {
                std::size_t toCopy = std::min<std::size_t>(BufferSize - m_BufferSize, size);
                std::memcpy(m_Buffer + m_BufferSize, data, toCopy);
                m_BufferSize += toCopy;
                data += toCopy;
                size -= toCopy;

                if (m_BufferSize == BufferSize)
                {
                    Transform();
                    m_Bitlen += 512;
                    m_BufferSize = 0;
                }
            }

            while (size >= BufferSize)
            {
                std::memcpy(m_Buffer, data, BufferSize);
                Transform();
                m_Bitlen += 512;
                data += BufferSize;
                size -= BufferSize;
            }

            if (size > 0)
            {
                std::memcpy(m_Buffer, data, size);
                m_BufferSize = size;
            }
        }

        inline void Update(const char* data, std::size_t size) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data), size);
        }

        inline void Update(std::string_view data) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        }

        void Finalize() noexcept
        {
            std::uint8_t start = m_BufferSize;
            std::uint8_t end = m_BufferSize < 56 ? 56 : 64;

            m_Buffer[start++] = 0b10000000;
            std::memset(&m_Buffer[start], 0, end - start);

            if (m_BufferSize >= 56)
            {
                Transform();
                std::memset(m_Buffer, 0, 56);
            }

            m_Bitlen += m_BufferSize * 8;
            std::uint64_t* const size = (std::uint64_t*)&m_Buffer[64 - 8];
            *size = Util::IsLittleEndian() ? Util::SwapEndian(m_Bitlen) : m_Bitlen;
            Transform();
        }


        inline std::string Hexdigest() const
        {
            char buff[Size+1];
            for (int i = 0; i < 5; ++i)
            {
                std::snprintf(&buff[i * 8], Size+1, "%08" PRIx32, m_H[i]);
            }
            return std::string(buff);
        }
    };

    inline std::string sha1(const char* str, std::size_t size)
    {
        Sha1 s;
        s.Update(str, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha1(std::string_view str)
    {
        return sha1(str.data(), str.size());
    }
#endif // HASH_ENABLE_SHA1



#if HASH_ENABLE_MD5 == 1
    //=============================================================================
    /* MD5
     converted to C++ class by Frank Thilo (thilo@unix-ag.org)
     for bzflag (http://www.bzflag.org)

       based on:

       md5.h and md5.c
       reference implemantion of RFC 1321

       Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
    rights reserved.

    License to copy and use this software is granted provided that it
    is identified as the "RSA Data Security, Inc. MD5 Message-Digest
    Algorithm" in all material mentioning or referencing this software
    or this function.

    License is also granted to make and use derivative works provided
    that such works are identified as "derived from the RSA Data
    Security, Inc. MD5 Message-Digest Algorithm" in all material
    mentioning or referencing the derived work.

    RSA Data Security, Inc. makes no representations concerning either
    the merchantability of this software or the suitability of this
    software for any particular purpose. It is provided "as is"
    without express or implied warranty of any kind.

    These notices must be retained in any copies of any part of this
    documentation and/or software.
    */

    // a small class for calculating MD5 hashes of strings or byte arrays
    // it is not meant to be fast or secure
    //
    // usage: 1) feed it blocks of uchars with update()
    //      2) finalize()
    //      3) get hexdigest() string
    //      or
    //      MD5(std::string).hexdigest()
    //
    // assumes that char is 8 bit and int is 32 bit
    class MD5
    {
    public:
        static constexpr std::size_t Size = 32;
    private:
        static constexpr std::size_t BlockSize = 64;

        std::uint8_t buffer[BlockSize]; // bytes that didn't fit in last 64 byte chunk
        std::uint32_t count[2];         // 64bit counter for number of bits (lo, hi)
        std::uint32_t state[4];         // digest so far
        std::uint8_t digest[16];        // the result
    private:
        // Constants for MD5 Transform routine.
        static constexpr std::uint32_t S11 = 7;
        static constexpr std::uint32_t S12 = 12;
        static constexpr std::uint32_t S13 = 17;
        static constexpr std::uint32_t S14 = 22;
        static constexpr std::uint32_t S21 = 5;
        static constexpr std::uint32_t S22 = 9;
        static constexpr std::uint32_t S23 = 14;
        static constexpr std::uint32_t S24 = 20;
        static constexpr std::uint32_t S31 = 4;
        static constexpr std::uint32_t S32 = 11;
        static constexpr std::uint32_t S33 = 16;
        static constexpr std::uint32_t S34 = 23;
        static constexpr std::uint32_t S41 = 6;
        static constexpr std::uint32_t S42 = 10;
        static constexpr std::uint32_t S43 = 15;
        static constexpr std::uint32_t S44 = 21;
    public:
        // default ctor, just initailize
        MD5() noexcept
        {
            Init();
        }

        //////////////////////////////////////////////

        // nifty shortcut ctor, compute Hash_MD5 for string and finalize it right away
        MD5(const std::string& text) noexcept
        {
            Init();
            Update(text.c_str(), static_cast<std::uint32_t>(text.length()));
            Finalize();
        }

        MD5(std::string_view text) noexcept
        {
            Init();
            Update(text.data(), static_cast<std::uint32_t>(text.length()));
            Finalize();
        }

        //////////////////////////////

        // MD5 block update operation. Continues an Hash_MD5 message-digest
        // operation, processing another message block
        void Update(const std::uint8_t input[], std::uint32_t length) noexcept
        {
            // compute number of bytes mod 64
            std::uint32_t index = count[0] / 8 % BlockSize;

            // Update number of bits
            if ((count[0] += (length << 3)) < (length << 3))
                count[1]++;
            count[1] += (length >> 29);

            // number of bytes we need to fill in buffer
            std::uint32_t firstpart = 64 - index;

            std::uint32_t i;

            // transform as many times as possible.
            if (length >= firstpart)
            {
                // fill buffer first, transform
                std::memcpy(&buffer[index], input, firstpart);
                Transform(buffer);

                // transform chunks of blocksize (64 bytes)
                for (i = firstpart; i + BlockSize <= length; i += BlockSize)
                    Transform(&input[i]);

                index = 0;
            }
            else
                i = 0;

            // buffer remaining input
            std::memcpy(&buffer[index], &input[i], length - i);
        }

        //////////////////////////////

        // for convenience provide a verson with signed char
        inline void Update(const char input[], std::uint32_t length) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(input), length);
        }

        inline void Update(std::string_view data) noexcept
        {
            Update(reinterpret_cast<const std::uint8_t*>(data.data()), static_cast<std::uint32_t>(data.size()));
        }

        //////////////////////////////

        // MD5 finalization. Ends an MD5 message-digest operation, writing the
        // the message digest and zeroizing the context.
        void Finalize() noexcept
        {
            static constexpr std::uint8_t padding[64] = {
                0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };

            // Save number of bits
            std::uint8_t bits[8];
            Encode(bits, count, 8);

            // pad out to 56 mod 64.
            std::uint32_t index = count[0] / 8 % 64;
            std::uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
            Update(padding, padLen);

            // Append length (before padding)
            Update(bits, 8);

            // Store state in digest
            Encode(digest, state, 16);

            // Zeroize sensitive information.
            std::memset(buffer, 0, sizeof buffer);
            std::memset(count, 0, sizeof count);
        }

        //////////////////////////////

        // return hex representation of digest as string
        std::string Hexdigest() const
        {
            char buf[Size+1];
            for (int i = 0; i < 16; i++)
            {
                std::snprintf(buf + i * 2, Size+1, "%02x", digest[i]);
            }
            return std::string(buf);
        }
    private:
        void Init() noexcept
        {
            count[0] = 0;
            count[1] = 0;

            // load magic initialization constants.
            state[0] = UINT32_C(0x67452301);
            state[1] = UINT32_C(0xefcdab89);
            state[2] = UINT32_C(0x98badcfe);
            state[3] = UINT32_C(0x10325476);
        }

        //////////////////////////////

        // apply MD5 algo on a block
        void Transform(const std::uint8_t block[BlockSize]) noexcept
        {
            std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
            Decode(x, block, BlockSize);

            /* Round 1 */
            a = FF(a, b, c, d, x[0],  S11, UINT32_C(0xd76aa478)); /* 1 */
            d = FF(d, a, b, c, x[1],  S12, UINT32_C(0xe8c7b756)); /* 2 */
            c = FF(c, d, a, b, x[2],  S13, UINT32_C(0x242070db)); /* 3 */
            b = FF(b, c, d, a, x[3],  S14, UINT32_C(0xc1bdceee)); /* 4 */
            a = FF(a, b, c, d, x[4],  S11, UINT32_C(0xf57c0faf)); /* 5 */
            d = FF(d, a, b, c, x[5],  S12, UINT32_C(0x4787c62a)); /* 6 */
            c = FF(c, d, a, b, x[6],  S13, UINT32_C(0xa8304613)); /* 7 */
            b = FF(b, c, d, a, x[7],  S14, UINT32_C(0xfd469501)); /* 8 */
            a = FF(a, b, c, d, x[8],  S11, UINT32_C(0x698098d8)); /* 9 */
            d = FF(d, a, b, c, x[9],  S12, UINT32_C(0x8b44f7af)); /* 10 */
            c = FF(c, d, a, b, x[10], S13, UINT32_C(0xffff5bb1)); /* 11 */
            b = FF(b, c, d, a, x[11], S14, UINT32_C(0x895cd7be)); /* 12 */
            a = FF(a, b, c, d, x[12], S11, UINT32_C(0x6b901122)); /* 13 */
            d = FF(d, a, b, c, x[13], S12, UINT32_C(0xfd987193)); /* 14 */
            c = FF(c, d, a, b, x[14], S13, UINT32_C(0xa679438e)); /* 15 */
            b = FF(b, c, d, a, x[15], S14, UINT32_C(0x49b40821)); /* 16 */

            /* Round 2 */
            a = GG(a, b, c, d, x[1],  S21, UINT32_C(0xf61e2562)); /* 17 */
            d = GG(d, a, b, c, x[6],  S22, UINT32_C(0xc040b340)); /* 18 */
            c = GG(c, d, a, b, x[11], S23, UINT32_C(0x265e5a51)); /* 19 */
            b = GG(b, c, d, a, x[0],  S24, UINT32_C(0xe9b6c7aa)); /* 20 */
            a = GG(a, b, c, d, x[5],  S21, UINT32_C(0xd62f105d)); /* 21 */
            d = GG(d, a, b, c, x[10], S22, UINT32_C(0x02441453)); /* 22 */
            c = GG(c, d, a, b, x[15], S23, UINT32_C(0xd8a1e681)); /* 23 */
            b = GG(b, c, d, a, x[4],  S24, UINT32_C(0xe7d3fbc8)); /* 24 */
            a = GG(a, b, c, d, x[9],  S21, UINT32_C(0x21e1cde6)); /* 25 */
            d = GG(d, a, b, c, x[14], S22, UINT32_C(0xc33707d6)); /* 26 */
            c = GG(c, d, a, b, x[3],  S23, UINT32_C(0xf4d50d87)); /* 27 */
            b = GG(b, c, d, a, x[8],  S24, UINT32_C(0x455a14ed)); /* 28 */
            a = GG(a, b, c, d, x[13], S21, UINT32_C(0xa9e3e905)); /* 29 */
            d = GG(d, a, b, c, x[2],  S22, UINT32_C(0xfcefa3f8)); /* 30 */
            c = GG(c, d, a, b, x[7],  S23, UINT32_C(0x676f02d9)); /* 31 */
            b = GG(b, c, d, a, x[12], S24, UINT32_C(0x8d2a4c8a)); /* 32 */

            /* Round 3 */
            a = HH(a, b, c, d, x[5],  S31, UINT32_C(0xfffa3942)); /* 33 */
            d = HH(d, a, b, c, x[8],  S32, UINT32_C(0x8771f681)); /* 34 */
            c = HH(c, d, a, b, x[11], S33, UINT32_C(0x6d9d6122)); /* 35 */
            b = HH(b, c, d, a, x[14], S34, UINT32_C(0xfde5380c)); /* 36 */
            a = HH(a, b, c, d, x[1],  S31, UINT32_C(0xa4beea44)); /* 37 */
            d = HH(d, a, b, c, x[4],  S32, UINT32_C(0x4bdecfa9)); /* 38 */
            c = HH(c, d, a, b, x[7],  S33, UINT32_C(0xf6bb4b60)); /* 39 */
            b = HH(b, c, d, a, x[10], S34, UINT32_C(0xbebfbc70)); /* 40 */
            a = HH(a, b, c, d, x[13], S31, UINT32_C(0x289b7ec6)); /* 41 */
            d = HH(d, a, b, c, x[0],  S32, UINT32_C(0xeaa127fa)); /* 42 */
            c = HH(c, d, a, b, x[3],  S33, UINT32_C(0xd4ef3085)); /* 43 */
            b = HH(b, c, d, a, x[6],  S34, UINT32_C(0x04881d05)); /* 44 */
            a = HH(a, b, c, d, x[9],  S31, UINT32_C(0xd9d4d039)); /* 45 */
            d = HH(d, a, b, c, x[12], S32, UINT32_C(0xe6db99e5)); /* 46 */
            c = HH(c, d, a, b, x[15], S33, UINT32_C(0x1fa27cf8)); /* 47 */
            b = HH(b, c, d, a, x[2],  S34, UINT32_C(0xc4ac5665)); /* 48 */

            /* Round 4 */
            a = II(a, b, c, d, x[0],  S41, UINT32_C(0xf4292244)); /* 49 */
            d = II(d, a, b, c, x[7],  S42, UINT32_C(0x432aff97)); /* 50 */
            c = II(c, d, a, b, x[14], S43, UINT32_C(0xab9423a7)); /* 51 */
            b = II(b, c, d, a, x[5],  S44, UINT32_C(0xfc93a039)); /* 52 */
            a = II(a, b, c, d, x[12], S41, UINT32_C(0x655b59c3)); /* 53 */
            d = II(d, a, b, c, x[3],  S42, UINT32_C(0x8f0ccc92)); /* 54 */
            c = II(c, d, a, b, x[10], S43, UINT32_C(0xffeff47d)); /* 55 */
            b = II(b, c, d, a, x[1],  S44, UINT32_C(0x85845dd1)); /* 56 */
            a = II(a, b, c, d, x[8],  S41, UINT32_C(0x6fa87e4f)); /* 57 */
            d = II(d, a, b, c, x[15], S42, UINT32_C(0xfe2ce6e0)); /* 58 */
            c = II(c, d, a, b, x[6],  S43, UINT32_C(0xa3014314)); /* 59 */
            b = II(b, c, d, a, x[13], S44, UINT32_C(0x4e0811a1)); /* 60 */
            a = II(a, b, c, d, x[4],  S41, UINT32_C(0xf7537e82)); /* 61 */
            d = II(d, a, b, c, x[11], S42, UINT32_C(0xbd3af235)); /* 62 */
            c = II(c, d, a, b, x[2],  S43, UINT32_C(0x2ad7d2bb)); /* 63 */
            b = II(b, c, d, a, x[9],  S44, UINT32_C(0xeb86d391)); /* 64 */

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;

            // Zeroize sensitive information.
            std::memset(x, 0, sizeof x);
        }

        //////////////////////////////

        // decodes input (unsigned char) into output (uint4). Assumes len is a multiple of 4.
        inline void Decode(std::uint32_t output[], const std::uint8_t input[], std::uint32_t len) noexcept
        {
            for (std::uint32_t i = 0, j = 0; j < len; i++, j += 4)
                output[i] = ((std::uint32_t)input[j]) | (((std::uint32_t)input[j + 1]) << 8) |
                (((std::uint32_t)input[j + 2]) << 16) | (((std::uint32_t)input[j + 3]) << 24);
        }

        //////////////////////////////

        // encodes input (uint4) into output (unsigned char). Assumes len is
        // a multiple of 4.
        inline void Encode(std::uint8_t output[], const std::uint32_t input[], std::uint32_t len) noexcept
        {
            for (std::uint32_t i = 0, j = 0; j < len; i++, j += 4)
            {
                output[j] = input[i] & 0xff;
                output[j + 1] = (input[i] >> 8) & 0xff;
                output[j + 2] = (input[i] >> 16) & 0xff;
                output[j + 3] = (input[i] >> 24) & 0xff;
            }
        }

        //////////////////////////////

        // low level logic operations
        ///////////////////////////////////////////////
        // F, G, H and I are basic Hash_MD5 functions.
        inline std::uint32_t F(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
        {
            return (x & y) | (~x & z);
        }

        inline std::uint32_t G(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
        {
            return (x & z) | (y & ~z);
        }

        inline std::uint32_t H(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
        {
            return x ^ y ^ z;
        }

        inline std::uint32_t I(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
        {
            return y ^ (x | ~z);
        }


        // FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
        // Rotation is separate from addition to prevent recomputation.
        inline std::uint32_t FF(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac) const noexcept
        {
            return Util::LeftRotate<std::uint32_t>(a + F(b, c, d) + x + ac, s) + b;
        }

        inline std::uint32_t GG(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac) const noexcept
        {
            return Util::LeftRotate<std::uint32_t>(a + G(b, c, d) + x + ac, s) + b;
        }

        inline std::uint32_t HH(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac) const noexcept
        {
            return Util::LeftRotate<std::uint32_t>(a + H(b, c, d) + x + ac, s) + b;
        }

        inline std::uint32_t II(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac) const noexcept
        {
            return Util::LeftRotate<std::uint32_t>(a + I(b, c, d) + x + ac, s) + b;
        }

        //////////////////////////////////////////////
    };

    inline std::string md5(std::string_view str)
    {
        MD5 md5 = MD5(str);
        return md5.Hexdigest();
    }

    inline std::string md5(const char* str, std::size_t size)
    {
        MD5 md5;
        md5.Update(str, static_cast<std::uint32_t>(size));
        md5.Finalize();
        return md5.Hexdigest();
    }
    //=============================================================================
#endif // HASH_ENABLE_MD5


#if HASH_ENABLE_SHA3
    namespace Private
    {
        template <std::size_t BitSize>
        struct Sha3
        {
            static_assert(BitSize == 224 || BitSize == 256 || BitSize == 384 || BitSize == 512, "BitSize must be 224, 256, 384 or 512");
        public:
            static constexpr std::size_t Size = BitSize / 4;
        private:
            static constexpr std::size_t UseKeccakFlag = 0x80000000;
            static constexpr std::size_t KeccakSpongeWords = (((1600)/8/*bits to byte*/)/sizeof(std::uint64_t));
        private:
            std::uint64_t m_Saved = 0;       // the portion of the input message that we didn't consume yet
            union {                     // Keccak's state
                std::uint64_t s[KeccakSpongeWords];
                std::uint8_t sb[KeccakSpongeWords * 8] = {0};
            } u;
            std::size_t m_ByteIndex = 0;      // 0..7--the next byte after the set one (starts from 0; 0--none are buffered)
            std::size_t m_WordIndex = 0;      // 0..24--the next word to integrate input (starts from 0)
            std::size_t m_CapacityWords = 2 * BitSize / (8 * sizeof(std::uint64_t));  // the double size of the hash output in words (e.g. 16 for Keccak 512)
        private:
            inline std::size_t CW(std::size_t x) const noexcept
            {
                return (x) & (~UseKeccakFlag);
            }

            void keccakf(std::uint64_t s[25]) const noexcept
            {
                static constexpr std::uint8_t keccakf_rotc[24] = {
                    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
                    18, 39, 61, 20, 44
                };

                static constexpr std::uint8_t keccakf_piln[24] = {
                    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
                    14, 22, 9, 6, 1
                };

                static constexpr std::uint64_t keccakf_rndc[24] = {
                    UINT64_C(0x0000000000000001), UINT64_C(0x0000000000008082),
                    UINT64_C(0x800000000000808a), UINT64_C(0x8000000080008000),
                    UINT64_C(0x000000000000808b), UINT64_C(0x0000000080000001),
                    UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008009),
                    UINT64_C(0x000000000000008a), UINT64_C(0x0000000000000088),
                    UINT64_C(0x0000000080008009), UINT64_C(0x000000008000000a),
                    UINT64_C(0x000000008000808b), UINT64_C(0x800000000000008b),
                    UINT64_C(0x8000000000008089), UINT64_C(0x8000000000008003),
                    UINT64_C(0x8000000000008002), UINT64_C(0x8000000000000080),
                    UINT64_C(0x000000000000800a), UINT64_C(0x800000008000000a),
                    UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008080),
                    UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008008)
                };


                std::uint64_t bc[5];
                constexpr std::uint8_t KeccakRounds = 24;

                for(std::uint8_t round = 0; round < KeccakRounds; round++)
                {
                    /* Theta */
                    for(std::uint8_t i = 0; i < 5; i++)
                        bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];

                    for(std::uint8_t i = 0; i < 5; i++)
                    {
                        std::uint64_t t = bc[(i + 4) % 5] ^ Hash::Util::LeftRotate<std::uint64_t>(bc[(i + 1) % 5], 1);
                        for(std::uint8_t j = 0; j < 25; j += 5)
                            s[j + i] ^= t;
                    }

                    /* Rho Pi */
                    std::uint64_t t = s[1];
                    for(std::uint8_t i = 0; i < 24; i++)
                    {
                        std::uint8_t j = keccakf_piln[i];
                        bc[0] = s[j];
                        s[j] = Hash::Util::LeftRotate<std::uint64_t>(t, keccakf_rotc[i]);
                        t = bc[0];
                    }

                    /* Chi */
                    for(std::uint8_t j = 0; j < 25; j += 5)
                    {
                        for(std::uint8_t i = 0; i < 5; i++)
                            bc[i] = s[j + i];
                        for(std::uint8_t i = 0; i < 5; i++)
                            s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
                    }

                    /* Iota */
                    s[0] ^= keccakf_rndc[round];
                }
            }
        public:
            void Update(const std::uint8_t *data, std::size_t size) noexcept
            {
                /* 0...7 -- how much is needed to have a word */
                std::size_t old_tail = (8 - m_ByteIndex) & 7;

                if(size < old_tail)
                {        /* have no complete word or haven't started 
                                            * the word yet */
                    /* endian-independent code follows: */
                    while (size--)
                        m_Saved |= (std::uint64_t) (*(data++)) << ((m_ByteIndex++) * 8);
                    return;
                }

                if(old_tail)
                {              /* will have one word to process */
                    /* endian-independent code follows: */
                    size -= old_tail;
                    while (old_tail--)
                        m_Saved |= (std::uint64_t) (*(data++)) << ((m_ByteIndex++) * 8);

                    /* now ready to add saved to the sponge */
                    u.s[m_WordIndex] ^= m_Saved;
                    m_ByteIndex = 0;
                    m_Saved = 0;
                    if(++m_WordIndex == (KeccakSpongeWords - CW(m_CapacityWords)))
                    {
                        keccakf(u.s);
                        m_WordIndex = 0;
                    }
                }


                std::size_t words = size / sizeof(std::uint64_t);
                std::size_t tail = size - words * sizeof(std::uint64_t);

                for(std::size_t i = 0; i < words; i++, data += sizeof(std::uint64_t))
                {
                    const std::uint64_t t = (std::uint64_t) (data[0]) |
                            ((std::uint64_t) (data[1]) << 8 * 1) |
                            ((std::uint64_t) (data[2]) << 8 * 2) |
                            ((std::uint64_t) (data[3]) << 8 * 3) |
                            ((std::uint64_t) (data[4]) << 8 * 4) |
                            ((std::uint64_t) (data[5]) << 8 * 5) |
                            ((std::uint64_t) (data[6]) << 8 * 6) |
                            ((std::uint64_t) (data[7]) << 8 * 7);

                    u.s[m_WordIndex] ^= t;
                    if(++m_WordIndex == (KeccakSpongeWords - CW(m_CapacityWords)))
                    {
                        keccakf(u.s);
                        m_WordIndex = 0;
                    }
                }


                while (tail--)
                {
                    m_Saved |= (std::uint64_t) (*(data++)) << ((m_ByteIndex++) * 8);
                }
            }


            inline void Update(const char* data, std::size_t size) noexcept
            {
                Update(reinterpret_cast<const std::uint8_t*>(data), size);
            }


            inline void Update(std::string_view data) noexcept
            {
                Update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
            }


            inline void Finalize() noexcept
            {
                /* Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
                * use 1<<2 below. The 0x02 below corresponds to the suffix 01.
                * Overall, we feed 0, then 1, and finally 1 to start padding. Without
                * M || 01, we would simply use 1 to start padding. */

                std::uint64_t t;

                //if(m_CapacityWords & UseKeccakFlag)
                //{
                //    /* Keccak version */
                //    t = (uint64_t)(((uint64_t) 1) << (m_ByteIndex * 8));
                //}
                //else
                //{
                    /* SHA3 version */
                    t = (std::uint64_t)(((std::uint64_t)(0x02 | (1 << 2))) << ((m_ByteIndex) * 8));
                //}

                u.s[m_WordIndex] ^= m_Saved ^ t;

                u.s[KeccakSpongeWords - CW(m_CapacityWords) - 1] ^= UINT64_C(0x8000000000000000);
                keccakf(u.s);

                // This conversion is not needed for little-endian platforms
                if (!Util::IsLittleEndian())
                {
                    std::uint32_t i;
                    for(i = 0; i < KeccakSpongeWords; i++)
                    {
                        const std::uint32_t t1 = (std::uint32_t) u.s[i];
                        const std::uint32_t t2 = (std::uint32_t) ((u.s[i] >> 16) >> 16);
                        u.sb[i * 8 + 0] = (std::uint8_t) (t1);
                        u.sb[i * 8 + 1] = (std::uint8_t) (t1 >> 8);
                        u.sb[i * 8 + 2] = (std::uint8_t) (t1 >> 16);
                        u.sb[i * 8 + 3] = (std::uint8_t) (t1 >> 24);
                        u.sb[i * 8 + 4] = (std::uint8_t) (t2);
                        u.sb[i * 8 + 5] = (std::uint8_t) (t2 >> 8);
                        u.sb[i * 8 + 6] = (std::uint8_t) (t2 >> 16);
                        u.sb[i * 8 + 7] = (std::uint8_t) (t2 >> 24);
                    }
                }
            }


            inline std::string Hexdigest() const
            {
                char buff[Size+1];
                for (std::size_t i = 0; i < Size/2; i++)
                {
                    std::snprintf(&buff[i*2], Size+1, "%02" PRIx32, static_cast<std::uint32_t>(reinterpret_cast<std::uint8_t*>(const_cast<std::uint64_t*>(u.s))[i]));
                }
                return std::string(buff);
            }
        };
    } // namespace Private
    using Sha3_224 = Private::Sha3<224>;
    using Sha3_256 = Private::Sha3<256>;
    using Sha3_384 = Private::Sha3<384>;
    using Sha3_512 = Private::Sha3<512>;


    inline std::string sha3_224(const unsigned char* data, std::size_t size)
    {
        Sha3_224 s;
        s.Update(data, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha3_256(const unsigned char* data, std::size_t size)
    {
        Sha3_256 s;
        s.Update(data, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha3_384(const unsigned char* data, std::size_t size)
    {
        Sha3_384 s;
        s.Update(data, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha3_512(const unsigned char* data, std::size_t size)
    {
        Sha3_512 s;
        s.Update(data, size);
        s.Finalize();
        return s.Hexdigest();
    }

    inline std::string sha3_224(const char* data, std::size_t size) { return sha3_224(reinterpret_cast<const unsigned char*>(data), size); }
    inline std::string sha3_256(const char* data, std::size_t size) { return sha3_256(reinterpret_cast<const unsigned char*>(data), size); }
    inline std::string sha3_384(const char* data, std::size_t size) { return sha3_384(reinterpret_cast<const unsigned char*>(data), size); }
    inline std::string sha3_512(const char* data, std::size_t size) { return sha3_512(reinterpret_cast<const unsigned char*>(data), size); }

    inline std::string sha3_224(std::string_view data) { return sha3_224(data.data(), data.size()); }
    inline std::string sha3_256(std::string_view data) { return sha3_256(data.data(), data.size()); }
    inline std::string sha3_384(std::string_view data) { return sha3_384(data.data(), data.size()); }
    inline std::string sha3_512(std::string_view data) { return sha3_512(data.data(), data.size()); }
#endif // HASH_ENABLE_SHA3
} // namespace Hash


#if HASH_ENABLE_SHAKE == 1
    //=============================================================================
    // source: https://github.com/XKCP/XKCP/blob/master/Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c
    /*
    Implementation by the Keccak Team, namely, Guido Bertoni, Joan Daemen,
    Michaël Peeters, Gilles Van Assche and Ronny Van Keer,
    hereby denoted as "the implementer".

    For more information, feedback or questions, please refer to our website:
    https://keccak.team/

    To the extent possible under law, the implementer has waived all copyright
    and related or neighboring rights to the source code in this file.
    http://creativecommons.org/publicdomain/zero/1.0/
    */

    /*
    ================================================================
    The purpose of this source file is to demonstrate a readable and compact
    implementation of all the Keccak instances approved in the FIPS 202 standard,
    including the hash functions and the extendable-output functions (XOFs).

    We focused on clarity and on source-code compactness,
    rather than on the performance.

    The advantages of this implementation are:
        + The source code is compact, after removing the comments, that is. :-)
        + There are no tables with arbitrary constants.
        + For clarity, the comments link the operations to the specifications using
            the same notation as much as possible.
        + There is no restriction in cryptographic features. In particular,
            the SHAKE128 and SHAKE256 XOFs can produce any output length.
        + The code does not use much RAM, as all operations are done in place.

    The drawbacks of this implementation are:
        - There is no message queue. The whole message must be ready in a buffer.
        - It is not optimized for performance.

    The implementation is even simpler on a little endian platform. Just define the
    HASH_KECCAK_LITTLE_ENDIAN symbol in that case.

    For a more complete set of implementations, please refer to
    the Keccak Code Package at https://github.com/gvanas/KeccakCodePackage

    For more information, please refer to:
        * [Keccak Reference] https://keccak.team/files/Keccak-reference-3.0.pdf
        * [Keccak Specifications Summary] https://keccak.team/keccak_specs_summary.html

    This file uses UTF-8 encoding, as some comments use Greek letters.
    ================================================================
    */

    /**
      * Function to compute the Keccak[r, c] sponge function over a given input.
      * @param  rate            The value of the rate r.
      * @param  capacity        The value of the capacity c.
      * @param  input           Pointer to the input message.
      * @param  inputByteLen    The number of input bytes provided in the input message.
      * @param  delimitedSuffix Bits that will be automatically appended to the end
      *                         of the input message, as in domain separation.
      *                         This is a byte containing from 0 to 7 bits
      *                         These <i>n</i> bits must be in the least significant bit positions
      *                         and must be delimited with a bit 1 at position <i>n</i>
      *                         (counting from 0=LSB to 7=MSB) and followed by bits 0
      *                         from position <i>n</i>+1 to position 7.
      *                         Some examples:
      *                             - If no bits are to be appended, then @a delimitedSuffix must be 0x01.
      *                             - If the 2-bit sequence 0,1 is to be appended (as for SHA3-*), @a delimitedSuffix must be 0x06.
      *                             - If the 4-bit sequence 1,1,1,1 is to be appended (as for SHAKE*), @a delimitedSuffix must be 0x1F.
      *                             - If the 7-bit sequence 1,1,0,1,0,0,0 is to be absorbed, @a delimitedSuffix must be 0x8B.
      * @param  output          Pointer to the buffer where to store the output.
      * @param  outputByteLen   The number of output bytes desired.
      * @pre    One must have r+c=1600 and the rate a multiple of 8 bits in this implementation.
      */
inline void hash_private_keccak_Keccak(unsigned int rate, unsigned int capacity, const unsigned char* input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char* output, unsigned long long int outputByteLen);

namespace Hash
{
    inline std::string shake128(const unsigned char* data, std::size_t size, std::size_t outsizeBytes)
    {
        std::string buff(outsizeBytes / 2, ' ');
        hash_private_keccak_Keccak(1344, 256, data, size, 0x1F, reinterpret_cast<unsigned char*>(buff.data()), outsizeBytes / 2);
        return Util::CharArrayToHexString(reinterpret_cast<unsigned char*>(buff.data()), outsizeBytes / 2);
    }


    inline std::string shake256(const unsigned char* data, std::size_t size, std::size_t outsizeBytes)
    {
        std::string buff(outsizeBytes / 2, ' ');
        hash_private_keccak_Keccak(1088, 512, data, size, 0x1F, reinterpret_cast<unsigned char*>(buff.data()), outsizeBytes / 2);
        return Util::CharArrayToHexString(reinterpret_cast<unsigned char*>(buff.data()), outsizeBytes / 2);
    }

    inline std::string shake128(const char* data, std::size_t size, std::size_t outsizeBytes) { return shake128(reinterpret_cast<const unsigned char*>(data), size, outsizeBytes); }
    inline std::string shake256(const char* data, std::size_t size, std::size_t outsizeBytes) { return shake256(reinterpret_cast<const unsigned char*>(data), size, outsizeBytes); }
    inline std::string shake128(std::string_view data, std::size_t outsizeBytes) { return shake128(data.data(), data.size(), outsizeBytes); }
    inline std::string shake256(std::string_view data, std::size_t outsizeBytes) { return shake256(data.data(), data.size(), outsizeBytes); }


    template <std::size_t outsizeBytes> inline std::string shake128(const unsigned char* data, std::size_t size)
    {
        static_assert(outsizeBytes > 0, "outsizeBytes must be greater than 0!");
        unsigned char buff[outsizeBytes / 2];
        hash_private_keccak_Keccak(1344, 256, data, size, 0x1F, buff, outsizeBytes / 2);
        return Util::CharArrayToHexString(buff, outsizeBytes / 2);
    }


    template <std::size_t outsizeBytes> inline std::string shake256(const unsigned char* data, std::size_t size)
    {
        static_assert(outsizeBytes > 0, "outsizeBytes must be greater than 0!");
        unsigned char buff[outsizeBytes / 2];
        hash_private_keccak_Keccak(1088, 512, data, size, 0x1F, buff, outsizeBytes / 2);
        return Util::CharArrayToHexString(buff, outsizeBytes / 2);
    }

    template <std::size_t outsizeBytes> inline std::string shake128(const char* data, std::size_t size) { return shake128<outsizeBytes>(reinterpret_cast<const unsigned char*>(data), size); }
    template <std::size_t outsizeBytes> inline std::string shake256(const char* data, std::size_t size) { return shake256<outsizeBytes>(reinterpret_cast<const unsigned char*>(data), size); }
    template <std::size_t outsizeBytes> inline std::string shake128(std::string_view data) { return shake128<outsizeBytes>(data.data(), data.size()); }
    template <std::size_t outsizeBytes> inline std::string shake256(std::string_view data) { return shake256<outsizeBytes>(data.data(), data.size()); }


    namespace File
    {
        inline std::string shake128(const char* path,      std::size_t outsizeBytes, std::ios::openmode flag = std::ios::binary) { return Hash::shake128(Util::LoadFile(path, flag),        outsizeBytes); }
        inline std::string shake128(std::string_view path, std::size_t outsizeBytes, std::ios::openmode flag = std::ios::binary) { return Hash::shake128(Util::LoadFile(path.data(), flag), outsizeBytes); }
        inline std::string shake256(const char* path,      std::size_t outsizeBytes, std::ios::openmode flag = std::ios::binary) { return Hash::shake256(Util::LoadFile(path, flag),        outsizeBytes); }
        inline std::string shake256(std::string_view path, std::size_t outsizeBytes, std::ios::openmode flag = std::ios::binary) { return Hash::shake256(Util::LoadFile(path.data(), flag), outsizeBytes); }

        template <std::size_t outsizeBytes> inline std::string shake128(const char* path,      std::ios::openmode flag = std::ios::binary) { return Hash::shake128<outsizeBytes>(Util::LoadFile(path,        flag)); }
        template <std::size_t outsizeBytes> inline std::string shake128(std::string_view path, std::ios::openmode flag = std::ios::binary) { return Hash::shake128<outsizeBytes>(Util::LoadFile(path.data(), flag)); }
        template <std::size_t outsizeBytes> inline std::string shake256(const char* path,      std::ios::openmode flag = std::ios::binary) { return Hash::shake256<outsizeBytes>(Util::LoadFile(path,        flag)); }
        template <std::size_t outsizeBytes> inline std::string shake256(std::string_view path, std::ios::openmode flag = std::ios::binary) { return Hash::shake256<outsizeBytes>(Util::LoadFile(path.data(), flag)); }
    }


    struct Shake128
    {
        std::size_t OutputSize = 64;
        inline std::string Hash(std::string_view data)                       const noexcept { return shake128(data, OutputSize);       }
        inline std::string Hash(const char* data, std::size_t size)          const noexcept { return shake128(data, size, OutputSize); }
        inline std::string Hash(const unsigned char* data, std::size_t size) const noexcept { return shake128(data, size, OutputSize); }
        inline std::string Hash(std::string_view data, std::size_t outsize)                       const noexcept { return shake128(data, outsize);       }
        inline std::string Hash(const char* data, std::size_t size, std::size_t outsize)          const noexcept { return shake128(data, size, outsize); }
        inline std::string Hash(const unsigned char* data, std::size_t size, std::size_t outsize) const noexcept { return shake128(data, size, outsize); }
    };

    struct Shake256
    {
        std::size_t OutputSize = 64;
        inline std::string Hash(std::string_view data)                       const noexcept { return shake256(data, OutputSize);       }
        inline std::string Hash(const char* data, std::size_t size)          const noexcept { return shake256(data, size, OutputSize); }
        inline std::string Hash(const unsigned char* data, std::size_t size) const noexcept { return shake256(data, size, OutputSize); }
        inline std::string Hash(std::string_view data, std::size_t outsize)                       const noexcept { return shake256(data, outsize);       }
        inline std::string Hash(const char* data, std::size_t size, std::size_t outsize)          const noexcept { return shake256(data, size, outsize); }
        inline std::string Hash(const unsigned char* data, std::size_t size, std::size_t outsize) const noexcept { return shake256(data, size, outsize); }
    };
}
#endif // HASH_ENABLE_SHAKE
#endif // defined(__cplusplus) && HASH_ENABLE_CPP_INTERFACE


#if HASH_ENABLE_SHAKE == 1
#if HASH_ENABLE_C_INTERFACE == 1
HASH_INLINE void hash_private_keccak_Keccak(unsigned int rate, unsigned int capacity, const unsigned char* input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char* output, unsigned long long int outputByteLen);

// heap allocated if outsizeBytes > HASH_SHAKE_128_MALLOC_LIMIT
HASH_INLINE const char* hash_shake128_binary(const char* data, size_t size, size_t outsizeBytes, char* buffer /*outsizeBytes+1*/)
{
    static char hex[HASH_SHAKE_128_MALLOC_LIMIT + 1];
    unsigned char intBuff[HASH_SHAKE_128_MALLOC_LIMIT / 2];
    char* out = buffer;
    unsigned char* buff = intBuff;

    if (buffer == NULL)
    {
        if (outsizeBytes > HASH_SHAKE_128_MALLOC_LIMIT)
        {
            char* tmp = (char*)malloc(outsizeBytes * 3 + 1);
            buff = (unsigned char*)tmp;
            out = &tmp[outsizeBytes];
        }
        else
            out = hex;
    }

    hash_private_keccak_Keccak(1344, 256, (const unsigned char*)data, size, 0x1F, buff, outsizeBytes / 2);
    hash_util_char_array_to_hex_string(buff, outsizeBytes / 2, out);
    return out;
}

// heap allocated if outsizeBytes > HASH_SHAKE_256_MALLOC_LIMIT
HASH_INLINE const char* hash_shake256_binary(const char* data, size_t size, size_t outsizeBytes, char* buffer /*outsizeBytes+1*/)
{
    static char hex[HASH_SHAKE_256_MALLOC_LIMIT + 1];
    unsigned char intBuff[HASH_SHAKE_256_MALLOC_LIMIT / 2];
    char* out = buffer;
    unsigned char* buff = intBuff;

    if (buffer == NULL)
    {
        if (outsizeBytes > HASH_SHAKE_256_MALLOC_LIMIT)
        {
            char* tmp = (char*)malloc(outsizeBytes * 3 + 1);
            buff = (unsigned char*)tmp;
            out = &tmp[outsizeBytes];
        }
        else
            out = hex;
    }

    hash_private_keccak_Keccak(1088, 512, (const unsigned char*)data, size, 0x1F, buff, outsizeBytes / 2);
    hash_util_char_array_to_hex_string(buff, outsizeBytes / 2, out);
    return out;
}

HASH_INLINE const char* hash_shake128(const char* data, size_t outsizeBytes, char* buffer) { return hash_shake128_binary(data, strlen(data), outsizeBytes, buffer); }
HASH_INLINE const char* hash_shake256(const char* data, size_t outsizeBytes, char* buffer) { return hash_shake256_binary(data, strlen(data), outsizeBytes, buffer); }

HASH_INLINE const char* hash_shake128_easy(const char* data, size_t outsizeBytes) { return hash_shake128_binary(data, strlen(data), outsizeBytes, NULL); }
HASH_INLINE const char* hash_shake256_easy(const char* data, size_t outsizeBytes) { return hash_shake256_binary(data, strlen(data), outsizeBytes, NULL); }

HASH_INLINE const char* hash_shake128_file(const char* path, const char* mode, size_t outsizeBytes, char* buffer)
{
    long fsize;
    char* content = hash_util_load_file(path, mode, &fsize);
    if (content == NULL) return "";
    const char* hash = hash_shake128_binary(content, fsize, outsizeBytes, buffer);
    free(content);
    return hash;
}

HASH_INLINE const char* hash_shake256_file(const char* path, const char* mode, size_t outsizeBytes, char* buffer)
{
    long fsize;
    char* content = hash_util_load_file(path, mode, &fsize);
    if (content == NULL) return "";
    const char* hash = hash_shake256_binary(content, fsize, outsizeBytes, buffer);
    free(content);
    return hash;
}

HASH_INLINE const char* hash_shake128_file_easy(const char* path, const char* mode, size_t outsizeBytes) { return hash_shake128_file(path, mode, outsizeBytes, NULL); }
HASH_INLINE const char* hash_shake256_file_easy(const char* path, const char* mode, size_t outsizeBytes) { return hash_shake256_file(path, mode, outsizeBytes, NULL); }
#endif // HASH_ENABLE_C_INTERFACE

/*
================================================================
Technicalities
================================================================
*/

#if HASH_KECCAK_LITTLE_ENDIAN != 1
/** Function to load a 64-bit value using the little-endian (LE) convention.
  * On a LE platform, this could be greatly simplified using a cast.
  */
HASH_INLINE uint64_t hash_private_keccak_load64(const uint8_t* x)
{
    int i;
    uint64_t u = 0;

    for (i = 7; i >= 0; --i)
    {
        u <<= 8;
        u |= x[i];
    }
    return u;
}

/** Function to store a 64-bit value using the little-endian (LE) convention.
  * On a LE platform, this could be greatly simplified using a cast.
  */
HASH_INLINE void hash_private_keccak_store64(uint8_t* x, uint64_t u)
{
    unsigned int i;

    for (i = 0; i < 8; ++i)
    {
        x[i] = (uint8_t)u;
        u >>= 8;
    }
}

/** Function to XOR into a 64-bit value using the little-endian (LE) convention.
  * On a LE platform, this could be greatly simplified using a cast.
  */
HASH_INLINE void hash_private_keccak_xor64(uint8_t* x, uint64_t u)
{
    unsigned int i;

    for (i = 0; i < 8; ++i)
    {
        x[i] ^= u;
        u >>= 8;
    }
}
#endif



#define HASH_PRIVATE_KECCAK_ROL64(a, offset) ((((uint64_t)a) << offset) ^ (((uint64_t)a) >> (64-offset)))
#define HASH_PRIVATE_KECCAK_I(x, y) ((x)+5*(y))

#if HASH_KECCAK_LITTLE_ENDIAN == 1
#define HASH_PRIVATE_KECCAK_READLANE(x, y)          (((uint64_t*)state)[HASH_PRIVATE_KECCAK_I(x, y)])
#define hash_private_keccak_WRITELANE(x, y, lane)   (((uint64_t*)state)[HASH_PRIVATE_KECCAK_I(x, y)]) = (lane)
#define hash_private_keccak_XORLANE(x, y, lane)     (((uint64_t*)state)[HASH_PRIVATE_KECCAK_I(x, y)]) ^= (lane)
#else
#define HASH_PRIVATE_KECCAK_READLANE(x, y)          hash_private_keccak_load64((uint8_t*)state+sizeof(uint64_t)*HASH_PRIVATE_KECCAK_I(x, y))
#define hash_private_keccak_WRITELANE(x, y, lane)   hash_private_keccak_store64((uint8_t*)state+sizeof(uint64_t)*HASH_PRIVATE_KECCAK_I(x, y), lane)
#define hash_private_keccak_XORLANE(x, y, lane)     hash_private_keccak_xor64((uint8_t*)state+sizeof(uint64_t)*HASH_PRIVATE_KECCAK_I(x, y), lane)
#endif

/**
  * Function that computes the linear feedback shift register (LFSR) used to
  * define the round constants (see [Keccak Reference, Section 1.2]).
  */
HASH_INLINE int hash_private_keccak_LFSR86540(uint8_t* LFSR)
{
    int result = ((*LFSR) & 0x01) != 0;
    if (((*LFSR) & 0x80) != 0)
        /* Primitive polynomial over GF(2): x^8+x^6+x^5+x^4+1 */
        (*LFSR) = ((*LFSR) << 1) ^ 0x71;
    else
        (*LFSR) <<= 1;
    return result;
}

/**
 * Function that computes the Keccak-f[1600] permutation on the given state.
 */
HASH_INLINE void hash_private_keccak_KeccakF1600_StatePermute(void* state)
{
    unsigned int round, x, y, j, t;
    uint8_t LFSRstate = 0x01;

    for (round = 0; round < 24; round++)
    {
        {   /* === θ step (see [Keccak Reference, Section 2.3.2]) === */
            uint64_t C[5], D;

            /* Compute the parity of the columns */
            for (x = 0; x < 5; x++)
                C[x] = HASH_PRIVATE_KECCAK_READLANE(x, 0) ^ HASH_PRIVATE_KECCAK_READLANE(x, 1) ^ HASH_PRIVATE_KECCAK_READLANE(x, 2) ^ HASH_PRIVATE_KECCAK_READLANE(x, 3) ^ HASH_PRIVATE_KECCAK_READLANE(x, 4);
            for (x = 0; x < 5; x++)
            {
                /* Compute the θ effect for a given column */
                D = C[(x + 4) % 5] ^ HASH_PRIVATE_KECCAK_ROL64(C[(x + 1) % 5], 1);
                /* Add the θ effect to the whole column */
                for (y = 0; y < 5; y++)
                    hash_private_keccak_XORLANE(x, y, D);
            }
        }

        {   /* === ρ and π steps (see [Keccak Reference, Sections 2.3.3 and 2.3.4]) === */
            uint64_t current, temp;
            /* Start at coordinates (1 0) */
            x = 1; y = 0;
            current = HASH_PRIVATE_KECCAK_READLANE(x, y);
            /* Iterate over ((0 1)(2 3))^t * (1 0) for 0 ≤ t ≤ 23 */
            for (t = 0; t < 24; t++)
            {
                /* Compute the rotation constant r = (t+1)(t+2)/2 */
                unsigned int r = ((t + 1) * (t + 2) / 2) % 64;
                /* Compute ((0 1)(2 3)) * (x y) */
                unsigned int Y = (2 * x + 3 * y) % 5; x = y; y = Y;
                /* Swap current and state(x,y), and rotate */
                temp = HASH_PRIVATE_KECCAK_READLANE(x, y);
                hash_private_keccak_WRITELANE(x, y, HASH_PRIVATE_KECCAK_ROL64(current, r));
                current = temp;
            }
        }

        {   /* === χ step (see [Keccak Reference, Section 2.3.1]) === */
            uint64_t temp[5];
            for (y = 0; y < 5; y++)
            {
                /* Take a copy of the plane */
                for (x = 0; x < 5; x++)
                    temp[x] = HASH_PRIVATE_KECCAK_READLANE(x, y);
                /* Compute χ on the plane */
                for (x = 0; x < 5; x++)
                    hash_private_keccak_WRITELANE(x, y, temp[x] ^ ((~temp[(x + 1) % 5]) & temp[(x + 2) % 5]));
            }
        }

        {   /* === ι step (see [Keccak Reference, Section 2.3.5]) === */
            for (j = 0; j < 7; j++)
            {
                unsigned int bitPosition = (1 << j) - 1; /* 2^j-1 */
                if (hash_private_keccak_LFSR86540(&LFSRstate))
                    hash_private_keccak_XORLANE(0, 0, (uint64_t)1 << bitPosition);
            }
        }
    }
}

/*
================================================================
A readable and compact implementation of the Keccak sponge functions
that use the Keccak-f[1600] permutation.
================================================================
*/

#define HASH_PRIVATE_KECCAK_MIN(a, b) ((a) < (b) ? (a) : (b))
HASH_INLINE void hash_private_keccak_Keccak(unsigned int rate, unsigned int capacity, const unsigned char* input, unsigned long long int inputByteLen, unsigned char delimitedSuffix, unsigned char* output, unsigned long long int outputByteLen)
{
    uint8_t state[200];
    unsigned int rateInBytes = rate / 8;
    unsigned int blockSize = 0;
    unsigned int i;

    if (((rate + capacity) != 1600) || ((rate % 8) != 0))
        return;

    /* === Initialize the state === */
    memset(state, 0, sizeof(state));

    /* === Absorb all the input blocks === */
    while (inputByteLen > 0)
    {
        blockSize = (unsigned int)HASH_PRIVATE_KECCAK_MIN(inputByteLen, rateInBytes);
        for (i = 0; i < blockSize; i++)
            state[i] ^= input[i];
        input += blockSize;
        inputByteLen -= blockSize;

        if (blockSize == rateInBytes)
        {
            hash_private_keccak_KeccakF1600_StatePermute(state);
            blockSize = 0;
        }
    }

    /* === Do the padding and switch to the squeezing phase === */
    /* Absorb the last few bits and add the first bit of padding (which coincides with the delimiter in delimitedSuffix) */
    state[blockSize] ^= delimitedSuffix;
    /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */
    if (((delimitedSuffix & 0x80) != 0) && (blockSize == (rateInBytes - 1)))
        hash_private_keccak_KeccakF1600_StatePermute(state);
    /* Add the second bit of padding */
    state[rateInBytes - 1] ^= 0x80;
    /* Switch to the squeezing phase */
    hash_private_keccak_KeccakF1600_StatePermute(state);

    /* === Squeeze out all the output blocks === */
    while (outputByteLen > 0)
    {
        blockSize = (unsigned int)HASH_PRIVATE_KECCAK_MIN(outputByteLen, rateInBytes);
        memcpy(output, state, blockSize);
        output += blockSize;
        outputByteLen -= blockSize;

        if (outputByteLen > 0)
            hash_private_keccak_KeccakF1600_StatePermute(state);
    }
}
#undef HASH_PRIVATE_KECCAK_I
#undef HASH_PRIVATE_KECCAK_MIN
#undef HASH_PRIVATE_KECCAK_ROL64
#undef hash_private_keccak_XORLANE
#undef HASH_PRIVATE_KECCAK_READLANE
#undef hash_private_keccak_WRITELANE
//=============================================================================
#endif // HASH_ENABLE_SHAKE
#endif // HASH_H

#undef HASH_MIN
#undef HASH_INLINE
#undef HASH_ENABLE_MD5
#undef HASH_ENABLE_SHA1
#undef HASH_ENABLE_SHA2
#undef HASH_ENABLE_SHA3
#undef HASH_ENABLE_SHAKE
#undef HASH_ENABLE_C_INTERFACE
#undef HASH_ENABLE_CPP_INTERFACE
#undef HASH_KECCAK_LITTLE_ENDIAN
#undef HASH_FILE_READ_BUFFER_SIZE
#undef HASH_ENABLE_LOOP_UNROLLING
#undef HASH_SHAKE_128_MALLOC_LIMIT
#undef HASH_SHAKE_256_MALLOC_LIMIT
#undef HASH_DEFINE_UTIL_SWAP_ENDIAN
#undef HASH_PRIVATE_KECCAK_SPONGE_WORDS

#ifdef _MSC_VER
#pragma warning( pop ) // 4996
#endif