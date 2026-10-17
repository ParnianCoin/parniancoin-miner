/**
 * Portable RandomHash2 platform layer (host C++ / CUDA device)
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

//--------------------------------------------------------------------------
// compilation target
#if defined(__CUDACC__)
    #define RH2_DEV          __device__
    #define RH2_CONST        __device__ const
    #define RH2_INLINE       __device__ __forceinline__
    #define RH_ALIGN(n)      __align__(n)
#else
    #define RH2_DEV
    #define RH2_CONST        static const
    #define RH2_INLINE       static inline
    #if defined(_MSC_VER)
        #define RH_ALIGN(n)  __declspec(align(n))
    #else
        #define RH_ALIGN(n)  __attribute__((aligned(n)))
    #endif
#endif

// sph (sphlib) configuration: no inline x86 asm, no unaligned word access
// (unaligned loads fault on GPUs; host uses the same paths for verification)
#define SPH_NO_ASM      1
#define SPH_UNALIGNED   0

// names used by the ported CPU sources
#define PLATFORM_CONST          RH2_CONST
#define RH_DECL_DEVICE          RH2_DEV
#define RH_DECL_HOST
#define RH_DECL_HOST_DEVICE     RH2_DEV
#define RH_DECL_FORCEINLINE     inline
#define CUDA_DECL_DEVICE        RH2_DEV
#define CUDA_DECL_HOST_AND_DEVICE RH2_DEV
#define RH_ASSERT(...)          while(0){}
#define PARNIANMINER_ASSERT(...) while(0){}
#define RH_PREFETCH_MEM(addr)
#define BIG_CONSTANT(x)         (x)

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef uint8_t  byte;

#define U32_Max 0xFFFFFFFFu
#define RH_IS_ALIGNED_32(p) (0 == (3 & ((size_t)(const void*)(p))))
#define RH_IS_ALIGNED_64(p) (0 == (7 & ((size_t)(const void*)(p))))

#define PARNIANMINER_CEIL(size,to)       (((size)+ (to)-1) &~((to)-1))
#define PARNIANMINER_ALIGN(Val, Align)   ((((Val) / (Align)) + 1)*(Align))
#define PARNIANMINER_ARRAY_COUNT(arr)    (sizeof(arr) / sizeof(arr[0]))

//--------------------------------------------------------------------------
// rotations
RH2_INLINE U32 rh2_rotl32(U32 x, U32 n) { n &= 31; return n ? (x << n) | (x >> (32 - n)) : x; }
RH2_INLINE U32 rh2_rotr32(U32 x, U32 n) { n &= 31; return n ? (x >> n) | (x << (32 - n)) : x; }
RH2_INLINE U64 rh2_rotl64(U64 x, U32 n) { n &= 63; return n ? (x << n) | (x >> (64 - n)) : x; }
RH2_INLINE U64 rh2_rotr64(U64 x, U32 n) { n &= 63; return n ? (x >> n) | (x << (64 - n)) : x; }
RH2_INLINE U8  rh2_rotl8(U8 x, U32 n)   { n &= 7;  return n ? (U8)((x << n) | (x >> (8 - n))) : x; }
RH2_INLINE U8  rh2_rotr8(U8 x, U32 n)   { n &= 7;  return n ? (U8)((x >> n) | (x << (8 - n))) : x; }

#define ROTL32(x,y)   rh2_rotl32((U32)(x),(U32)(y))
#define ROTR32(x,y)   rh2_rotr32((U32)(x),(U32)(y))
#define ROTL64(x,y)   rh2_rotl64((U64)(x),(U32)(y))
#define ROTR64(x,y)   rh2_rotr64((U64)(x),(U32)(y))
#define ROTL8(x,y)    rh2_rotl8((U8)(x),(U32)((y) % 8))
#define ROTR8(x,y)    rh2_rotr8((U8)(x),(U32)((y) % 8))
#define _rotl(x,n)    rh2_rotl32((U32)(x),(U32)(n))
#define _rotr(x,n)    rh2_rotr32((U32)(x),(U32)(n))
#define _rotl64(x,n)  rh2_rotl64((U64)(x),(U32)(n))
#define _rotr64(x,n)  rh2_rotr64((U64)(x),(U32)(n))
#define _rotl8(x,n)   rh2_rotl8((U8)(x),(U32)(n))
#define _rotr8(x,n)   rh2_rotr8((U8)(x),(U32)(n))

//--------------------------------------------------------------------------
// byte helpers
RH2_INLINE void ReadUInt32AsBytesLE(const uint64_t a_in, uint8_t* a_out)
{
    a_out[0] = uint8_t(a_in);
    a_out[1] = uint8_t(a_in >> 8);
    a_out[2] = uint8_t(a_in >> 16);
    a_out[3] = uint8_t(a_in >> 24);
}

RH2_INLINE void ReadUInt64AsBytesLE(const uint64_t a_in, uint8_t* a_out)
{
    for (int i = 0; i < 8; i++)
        a_out[i] = (uint8_t)(a_in >> (8 * i));
}

RH2_INLINE uint64_t ReverseBytesUInt64(const uint64_t value)
{
    return  (value & uint64_t(0x00000000000000FF)) << 56 |
            (value & uint64_t(0x000000000000FF00)) << 40 |
            (value & uint64_t(0x0000000000FF0000)) << 24 |
            (value & uint64_t(0x00000000FF000000)) << 8 |
            (value & uint64_t(0x000000FF00000000)) >> 8 |
            (value & uint64_t(0x0000FF0000000000)) >> 24 |
            (value & uint64_t(0x00FF000000000000)) >> 40 |
            (value & uint64_t(0xFF00000000000000)) >> 56;
}

RH2_INLINE uint32_t ReverseBytesUInt32(const uint32_t value)
{
    return (value & uint32_t(0x000000FF)) << 24 |
           (value & uint32_t(0x0000FF00)) << 8 |
           (value & uint32_t(0x00FF0000)) >> 8 |
           (value & uint32_t(0xFF000000)) >> 24;
}

#define PARNIANMINER_T64(x)    ((x) & uint64_t(0xFFFFFFFFFFFFFFFF))

#define RH_swap_u32(x)   ReverseBytesUInt32((U32)(x))
#define RH_swap_u64(x)   ReverseBytesUInt64((U64)(x))

// byte-exact equivalents of the CPU helpers in RandomHash_core.h
RH2_INLINE void swap_copy_str_to_u32(const void *src, void *dest, const int32_t length)
{
    const uint8_t* s = (const uint8_t*)src;
    for (int32_t i = 0; i < length; i++)
        ((uint8_t *)dest)[i ^ 3] = s[i];
}

RH2_INLINE void swap_copy_str_to_u64(const void *src, void *dest, const int32_t length)
{
    const uint8_t* s = (const uint8_t*)src;
    for (int32_t i = 0; i < length; i++)
        ((uint8_t *)dest)[i ^ 7] = s[i];
}

#define RH_memzero_8(ptr, size)  { memset((void*)(ptr), 0, 8); }
#define RH_memzero_16(ptr, size) { memset((void*)(ptr), 0, 16); }
#define RH_memzero_32(ptr, size) { memset((void*)(ptr), 0, 32); }
#define RH_memzero_64(ptr, size) { memset((void*)(ptr), 0, 64); }
#define RH_memzero_of8(ptr, s)   { memset((void*)(ptr), 0, (s)); }
#define RH_memzero_of16(ptr, s)  { memset((void*)(ptr), 0, (s)); }

#define copy4(dst, src) {(dst)[0] = (src)[0]; (dst)[1] = (src)[1]; (dst)[2] = (src)[2]; (dst)[3] = (src)[3];}
#define copy4_op(dst, src, op) {(dst)[0] = op((src)[0]); (dst)[1] = op((src)[1]); (dst)[2] = op((src)[2]); (dst)[3] = op((src)[3]);}
#define copy8(dst, src) {for (int _ci = 0; _ci < 8; _ci++) (dst)[_ci] = (src)[_ci];}
#define copy8_op(dst, src, op) {for (int _ci = 0; _ci < 8; _ci++) (dst)[_ci] = op((src)[_ci]);}
#define copy6_op(dst, src, op) {for (int _ci = 0; _ci < 6; _ci++) (dst)[_ci] = op((src)[_ci]);}

//--------------------------------------------------------------------------
// RandomHash2 constants (must match MinersLib/Parnian/ParnianCommon.h)
#define RH2_HEADER_SIZE      236
#define RH2_MIN_N            2
#define RH2_MAX_N            4
#define RH2_MIN_J            1
#define RH2_MAX_J            8
#define RH2_M                64

// strides: [size u32][pad to RH_IDEAL_ALIGNMENT][data ...]
#define RH_IDEAL_ALIGNMENT   64
#define RH_IDEAL_ALIGNMENT32 (RH_IDEAL_ALIGNMENT/4)

typedef U32* RH_StridePtr;
#define RH_STRIDE_GET_SIZE(strideVar)       (*(strideVar))
#define RH_STRIDE_SET_SIZE(strideVar, val)  {*(strideVar) = (U32)(val);}
#define RH_STRIDE_RESET(strideVar)          {*(strideVar) = 0;}
#define RH_STRIDE_GET_DATA(strideVar)       ((strideVar) + RH_IDEAL_ALIGNMENT32)
#define RH_STRIDE_GET_DATA8(strideVar)      reinterpret_cast<U8*>((strideVar) + RH_IDEAL_ALIGNMENT32)
#define RH_STRIDE_GET_DATA64(strideVar)     reinterpret_cast<U64*>((strideVar) + RH_IDEAL_ALIGNMENT32)
#define RH_STRIDE_INIT_INTEGRITY(strideVar) {}
#define RH_STRIDE_CHECK_INTEGRITY(strideVar) {}

enum RandomHash2Algos
{
    RH2_Blake2B_160 = 0, RH2_Blake2B_256, RH2_Blake2B_512, RH2_Blake2B_384,
    RH2_Blake2S_128, RH2_Blake2S_160, RH2_Blake2S_224, RH2_Blake2S_256,
    RH2_Gost, RH2_GOST3411_2012_256, RH2_GOST3411_2012_512,
    RH2_Grindahl256, RH2_Grindahl512, RH2_HAS160,
    RH2_Haval_3_128, RH2_Haval_3_160, RH2_Haval_3_192, RH2_Haval_3_224, RH2_Haval_3_256,
    RH2_Haval_4_128, RH2_Haval_4_160, RH2_Haval_4_192, RH2_Haval_4_224, RH2_Haval_4_256,
    RH2_Haval_5_128, RH2_Haval_5_160, RH2_Haval_5_192, RH2_Haval_5_224, RH2_Haval_5_256,
    RH2_Keccak_224, RH2_Keccak_256, RH2_Keccak_288, RH2_Keccak_384, RH2_Keccak_512,
    RH2_MD2, RH2_MD5, RH2_MD4, RH2_Panama, RH2_RadioGatun32,
    RH2_RIPEMD, RH2_RIPEMD128, RH2_RIPEMD160, RH2_RIPEMD256, RH2_RIPEMD320,
    RH2_SHA0, RH2_SHA1, RH2_SHA2_224, RH2_SHA2_256, RH2_SHA2_384, RH2_SHA2_512,
    RH2_SHA2_512_224, RH2_SHA2_512_256, RH2_SHA3_224, RH2_SHA3_256, RH2_SHA3_384, RH2_SHA3_512,
    RH2_Snefru_8_128, RH2_Snefru_8_256,
    RH2_Tiger_3_128, RH2_Tiger_3_160, RH2_Tiger_3_192, RH2_Tiger_4_128, RH2_Tiger_4_160,
    RH2_Tiger_4_192, RH2_Tiger_5_128, RH2_Tiger_5_160, RH2_Tiger_5_192,
    RH2_Tiger2_3_128, RH2_Tiger2_3_160, RH2_Tiger2_3_192, RH2_Tiger2_4_128, RH2_Tiger2_4_160,
    RH2_Tiger2_4_192, RH2_Tiger2_5_128, RH2_Tiger2_5_160, RH2_Tiger2_5_192,
    RH2_WhirlPool,
    RH2_MAX_ALGO = 77
};

// output size in bytes of each algorithm
RH2_CONST U32 c_AlgoSize[RH2_MAX_ALGO] = {
    20, 32, 64, 48, 16, 20, 28, 32,           // blake2
    32, 32, 64,                               // gost
    32, 64, 20,                               // grindahl, has160
    16, 20, 24, 28, 32, 16, 20, 24, 28, 32, 16, 20, 24, 28, 32, // haval
    28, 32, 36, 48, 64,                       // keccak
    16, 16, 16, 32, 32,                       // md2 md5 md4 panama radiogatun
    16, 16, 20, 32, 40,                       // ripemd
    20, 20, 28, 32, 48, 64, 28, 32,           // sha0 sha1 sha2
    28, 32, 48, 64,                           // sha3
    16, 32,                                   // snefru
    16, 20, 24, 16, 20, 24, 16, 20, 24,       // tiger
    16, 20, 24, 16, 20, 24, 16, 20, 24,       // tiger2
    64                                        // whirlpool
};

enum SHA2_512_MODE
{
    SHA2_512_MODE_384,
    SHA2_512_MODE_512,
    SHA2_512_MODE_512_224,
    SHA2_512_MODE_512_256,
};

RH2_CONST uint32_t RH_RIPEMD_C1 = 0x50A28BE6;
RH2_CONST uint32_t RH_RIPEMD_C2 = 0x5A827999;
RH2_CONST uint32_t RH_RIPEMD_C3 = 0x5C4DD124;
RH2_CONST uint32_t RH_RIPEMD_C4 = 0x6ED9EBA1;
RH2_CONST uint32_t RH_RIPEMD_C5 = 0x6D703EF3;
RH2_CONST uint32_t RH_RIPEMD_C6 = 0x8F1BBCDC;
RH2_CONST uint32_t RH_RIPEMD_C7 = 0x7A6D76E9;
RH2_CONST uint32_t RH_RIPEMD_C8 = 0xA953FD4E;
RH2_CONST uint64_t Tiger_C1 = 0xA5A5A5A5A5A5A5A5ull;
RH2_CONST uint64_t Tiger_C2 = 0x0123456789ABCDEFull;

// Generic MD-style main loop. NOTE: pads in place after the input data,
// so every hash input buffer must have >= 128 bytes of slack.
#define RandomHash_MD_BASE_MAIN_LOOP(ALGO_BLOCK_SIZE, ALGO_FUNC, BE_LE_CONVERT)      { \
    int32_t len = (int32_t)RH_STRIDE_GET_SIZE(roundInput);                             \
    uint32_t blockCount = len / ALGO_BLOCK_SIZE;                                       \
    uint32_t *dataPtr = (uint32_t *)RH_STRIDE_GET_DATA(roundInput);                    \
    uint64_t bits = len * 8;                                                           \
    while(blockCount > 0)                                                              \
    {                                                                                  \
        ALGO_FUNC(dataPtr, state);                                                     \
        len -= ALGO_BLOCK_SIZE;                                                        \
        dataPtr += ALGO_BLOCK_SIZE / 4;                                                \
        blockCount--;                                                                  \
    }                                                                                  \
    {                                                                                  \
        int32_t padindex;                                                              \
        RH_ALIGN(64) uint8_t pad[80];                                                  \
        if (len < 56)                                                                  \
            padindex = 56 - len;                                                       \
        else                                                                           \
            padindex = 120 - len;                                                      \
        memset(pad, 0, sizeof(pad));                                                   \
        pad[0] = 0x80;                                                                 \
        bits = BE_LE_CONVERT(bits);                                                    \
        ReadUInt64AsBytesLE(bits, pad+padindex);                                       \
        padindex = padindex + 8;                                                       \
        memcpy(((uint8_t*)dataPtr) + len, pad, padindex);                              \
        ALGO_FUNC(dataPtr, state);                                                     \
        padindex -= ALGO_BLOCK_SIZE;                                                   \
        if (padindex > 0)                                                              \
            ALGO_FUNC(dataPtr+(ALGO_BLOCK_SIZE/4), state);                             \
    } }
