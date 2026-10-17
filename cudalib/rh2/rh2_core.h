/**
 * Portable RandomHash2 core (host C++ / CUDA device)
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * Straight implementation of PIP-0036 (RandomHash2) in mining mode:
 *   - every evaluated nonce reports its own hash,
 *   - every neighbour nonce that reaches its final round while being evaluated
 *     as a dependency also reports its (free) hash, exactly like the CPU miner.
 *
 * Memory model: one arena per thread. Round outputs are strides appended to the
 * arena and indexed by an entry list. Because PIP-0036 builds every round's
 * output list as  parent outputs + neighbour outputs + own output  in the same
 * order the recursion produces them, the entry list is a single append-only
 * stack and no copying is needed.
 */
#pragma once

#include "rh2_platform.h"
#include "rh2_sha256.h"

//---------------------------------------------------------------------------
// ported hash algorithms
#include "algos/sph_types.h"
#include "algos/sph_sha0.h"
#include "algos/sph_sha1.h"
#include "algos/sph_panama.h"
#include "algos/sph_haval.h"
#include "algos/sha0.h"
#undef RFUN
#undef HASH
#include "algos/sha1.h"
#include "algos/panama.h"
#undef RSTATE
#undef WSTATE
#include "algos/haval.h"
#include "rh2_blake2.h"
#include "algos/rh_grindahl512.h"
#include "algos/rh_haval_5_256.h"
#include "algos/rh_md5.h"
#include "algos/rh_radiogatun32.h"
#include "algos/rh_ripemd160.h"
#include "algos/rh_ripemd256.h"
#include "algos/rh_ripemd320.h"
#include "algos/rh_sha2_512.h"
#include "algos/rh_sha3_512.h"
#include "algos/rh_snefru_8_256.h"
#include "algos/rh_tiger2_5_192.h"
#include "algos/rh_whirlpool.h"
#include "algos/rh_ghost.h"
#include "algos/rh_ghost3411.h"
#include "algos/rh_md2.h"
#undef F
#undef G
#undef H
#undef STEP
#include "algos/rh_md4.h"
#include "algos/rh_panama.h"
#include "algos/rh_has160.h"
#include "algos/rh_sha.h"
#include "algos/rh_grindahl256.h"
#include "algos/rh_haval.h"
#include "algos/rh_ripemd.h"
#include "algos/rh_ripemd128.h"

//---------------------------------------------------------------------------
// limits
#define RH2_MAX_ENTRIES      600          // worst case is 585 outputs for one nonce
#define RH2_WORK_BYTES       512          // hash input buffer (100 bytes + in-place padding slack)
#define RH2_ARENA_DEFAULT    (176*1024)   // covers the theoretical worst case (~165KB)

struct RH2_Header
{
    U32 midstate[8];        // SHA2-256 state after header bytes [0..192)
    U8  tail[RH2_HEADER_SIZE - 192];   // header bytes [192..236), nonce = last 4 bytes
};

struct RH2_Thread
{
    U32* arena;             // stride storage
    U32  arenaWords;        // capacity in U32
    U32  arenaUsed;         // used, in U32
    U32  entryCount;
    U32  overflow;
    U32  entries[RH2_MAX_ENTRIES];                      // offsets (U32 units) into arena
    RH_ALIGN(64) U32 workBytes[(RH_IDEAL_ALIGNMENT + RH2_WORK_BYTES) / 4];
    RH_ALIGN(64) U32 veneerBytes[(RH_IDEAL_ALIGNMENT + 256) / 4];
};

//---------------------------------------------------------------------------
// Mersenne twister (MT19937, PascalCoin TMersenne32), first COUNT outputs only.
// Outputs 0..226 depend only on the initialisation array, so no full twist.
template <int COUNT>
struct RH2_MT
{
    U32 out[COUNT];
    U32 index;

    RH2_DEV void Init(U32 seed)
    {
        // need init[0 .. COUNT+397]
        U32 lo[COUNT + 1];
        U32 hi[COUNT];
        U32 v = seed;
        lo[0] = v;
        for (U32 k = 1; k <= COUNT + 396; k++)
        {
            v = 0x6c078965u * (v ^ (v >> 30)) + k;
            if (k <= COUNT)
                lo[k] = v;
            if (k >= 397)
                hi[k - 397] = v;
        }
        for (U32 i = 0; i < COUNT; i++)
        {
            U32 y = (lo[i] & 0x80000000u) | (lo[i + 1] & 0x7fffffffu);
            y = hi[i] ^ (y >> 1) ^ ((y & 1u) ? 0x9908b0dfu : 0u);
            y ^= y >> 11;
            y ^= (y << 7) & 0x9d2c5680u;
            y ^= (y << 15) & 0xefc60000u;
            y ^= y >> 18;
            out[i] = y;
        }
        index = 0;
    }

    RH2_DEV U32 Next() { return out[index++]; }
};

// Streaming variant for long sequences (Compress draws 200 numbers): keeps only
// init[0..COUNT] and produces init[397+i] on the fly -> ~0.8KB instead of 2.4KB.
template <int COUNT>
struct RH2_MTStream
{
    U32 lo[COUNT + 1];
    U32 v;
    U32 k;
    U32 i;

    RH2_DEV void Init(U32 seed)
    {
        v = seed;
        lo[0] = v;
        for (U32 n = 1; n < 397; n++)
        {
            v = 0x6c078965u * (v ^ (v >> 30)) + n;
            if (n <= COUNT)
                lo[n] = v;
        }
        k = 397;
        i = 0;
    }

    RH2_DEV U32 Next()
    {
        v = 0x6c078965u * (v ^ (v >> 30)) + k;  // init[397 + i]
        k++;
        U32 y = (lo[i] & 0x80000000u) | (lo[i + 1] & 0x7fffffffu);
        y = v ^ (y >> 1) ^ ((y & 1u) ? 0x9908b0dfu : 0u);
        i++;
        y ^= y >> 11;
        y ^= (y << 7) & 0x9d2c5680u;
        y ^= (y << 15) & 0xefc60000u;
        y ^= y >> 18;
        return y;
    }
};

#if defined(__CUDACC__)
    #define RH2_NOINLINE __device__ __noinline__
#else
    #define RH2_NOINLINE
#endif

//---------------------------------------------------------------------------
RH2_INLINE U32 rh2_last_dword(const RH_StridePtr s)
{
    U32 size = RH_STRIDE_GET_SIZE(s);
    return *(RH_STRIDE_GET_DATA(s) + ((size - 4) / 4));
}

RH2_INLINE RH_StridePtr rh2_entry(RH2_Thread* t, U32 i)
{
    return t->arena + t->entries[i];
}

// allocate a stride able to hold `capacityBytes` of data (+ slack for algos)
RH2_DEV RH_StridePtr rh2_alloc(RH2_Thread* t, U32 capacityBytes)
{
    U32 words = RH_IDEAL_ALIGNMENT32 + ((capacityBytes + 64 + 15) & ~15u) / 4;
    if (t->arenaUsed + words > t->arenaWords || t->entryCount >= RH2_MAX_ENTRIES)
    {
        t->overflow = 1;
        return 0;
    }
    RH_StridePtr s = t->arena + t->arenaUsed;
    t->entries[t->entryCount++] = t->arenaUsed;
    t->arenaUsed += words;
    RH_STRIDE_SET_SIZE(s, 0);
    return s;
}

//---------------------------------------------------------------------------
// PIP-0036 Compress: 100 random bytes from the outputs [begin, end)
RH2_NOINLINE void rh2_compress(RH2_Thread* t, U32 begin, U32 end, U32 seed, RH_StridePtr result)
{
    RH2_MTStream<200> gen;
    gen.Init(seed);
    U32 count = end - begin;
    U8* dst = RH_STRIDE_GET_DATA8(result);
    for (U32 i = 0; i < 100; i++)
    {
        RH_StridePtr src = rh2_entry(t, begin + gen.Next() % count);
        U32 srcSize = RH_STRIDE_GET_SIZE(src);
        dst[i] = RH_STRIDE_GET_DATA8(src)[gen.Next() % srcSize];
    }
    RH_STRIDE_SET_SIZE(result, 100);
}

//---------------------------------------------------------------------------
// PIP-0036 Expand, in place: output grows by factor * M bytes
RH2_NOINLINE void rh2_expand(RH_StridePtr output, U32 factor, U32 seed)
{
    if (factor == 0)
        return;
    RH2_MT<4> gen;
    gen.Init(seed);
    U8* data = RH_STRIDE_GET_DATA8(output);
    S32 bytesToAdd = (S32)(factor * RH2_M);
    while (bytesToAdd > 0)
    {
        U32 size = RH_STRIDE_GET_SIZE(output);
        U32 chunkSize = size;
        if ((S32)chunkSize > bytesToAdd)
            chunkSize = (U32)bytesToAdd;
        U8* chunk = data + size;
        U32 half = chunkSize >> 1;
        switch (gen.Next() % 8)
        {
            case 0: // MemTransform1: XorShift32 random byte pick
            {
                U32 x = (U32)data[chunkSize - 4] | ((U32)data[chunkSize - 3] << 8) |
                        ((U32)data[chunkSize - 2] << 16) | ((U32)data[chunkSize - 1] << 24);
                if (!x)
                    x = 1;
                for (U32 i = 0; i < chunkSize; i++)
                {
                    x ^= x << 13;
                    x ^= x >> 17;
                    x ^= x << 5;
                    chunk[i] = data[x % chunkSize];
                }
                break;
            }
            case 1: // MemTransform2: swap halves
                for (U32 i = 0; i < half; i++)
                {
                    chunk[i] = data[half + i];
                    chunk[half + i] = data[i];
                }
                break;
            case 2: // MemTransform3: reverse
                for (U32 i = 0; i < chunkSize; i++)
                    chunk[i] = data[chunkSize - i - 1];
                break;
            case 3: // MemTransform4: interleave left/right
                for (U32 i = 0; i < half; i++)
                {
                    chunk[i * 2] = data[i];
                    chunk[i * 2 + 1] = data[i + half];
                }
                break;
            case 4: // MemTransform5: interleave right/left
                for (U32 i = 0; i < half; i++)
                {
                    chunk[i * 2] = data[i + half];
                    chunk[i * 2 + 1] = data[i];
                }
                break;
            case 5: // MemTransform6: xor folds
                for (U32 i = 0; i < half; i++)
                {
                    chunk[i] = data[i * 2] ^ data[i * 2 + 1];
                    chunk[i + half] = data[i] ^ data[chunkSize - i - 1];
                }
                break;
            case 6: // MemTransform7: rotate left
                for (U32 i = 0; i < chunkSize; i++)
                    chunk[i] = ROTL8(data[i], chunkSize - i);
                break;
            case 7: // MemTransform8: rotate right
                for (U32 i = 0; i < chunkSize; i++)
                    chunk[i] = ROTR8(data[i], chunkSize - i);
                break;
        }
        RH_STRIDE_SET_SIZE(output, size + chunkSize);
        bytesToAdd -= (S32)chunkSize;
    }
}

//---------------------------------------------------------------------------
#ifndef RH2_ALGO_HOOK
    #define RH2_ALGO_HOOK(algo)
#endif

RH2_NOINLINE void rh2_hash_algo(U32 algo, RH_StridePtr input, RH_StridePtr output)
{
    RH2_ALGO_HOOK(algo);
    switch (algo)
    {
        case RH2_Blake2B_160:       RandomHash_blake2b(input, output, 160); break;
        case RH2_Blake2B_256:       RandomHash_blake2b(input, output, 256); break;
        case RH2_Blake2B_512:       RandomHash_blake2b(input, output, 512); break;
        case RH2_Blake2B_384:       RandomHash_blake2b(input, output, 384); break;
        case RH2_Blake2S_128:       RandomHash_blake2s(input, output, 128); break;
        case RH2_Blake2S_160:       RandomHash_blake2s(input, output, 160); break;
        case RH2_Blake2S_224:       RandomHash_blake2s(input, output, 224); break;
        case RH2_Blake2S_256:       RandomHash_blake2s(input, output, 256); break;
        case RH2_Gost:              RandomHash_Ghost(input, output); break;
        case RH2_GOST3411_2012_256: RandomHash_Ghost3411(input, output, 256); break;
        case RH2_GOST3411_2012_512: RandomHash_Ghost3411(input, output, 512); break;
        case RH2_Grindahl256:       RandomHash_Grindahl256(input, output); break;
        case RH2_Grindahl512:       RandomHash_Grindahl512(input, output); break;
        case RH2_HAS160:            RandomHash_HAS160(input, output); break;
        case RH2_Haval_3_128:       RandomHash_Haval3(input, output, 128); break;
        case RH2_Haval_3_160:       RandomHash_Haval3(input, output, 160); break;
        case RH2_Haval_3_192:       RandomHash_Haval3(input, output, 192); break;
        case RH2_Haval_3_224:       RandomHash_Haval3(input, output, 224); break;
        case RH2_Haval_3_256:       RandomHash_Haval3(input, output, 256); break;
        case RH2_Haval_4_128:       RandomHash_Haval4(input, output, 128); break;
        case RH2_Haval_4_160:       RandomHash_Haval4(input, output, 160); break;
        case RH2_Haval_4_192:       RandomHash_Haval4(input, output, 192); break;
        case RH2_Haval_4_224:       RandomHash_Haval4(input, output, 224); break;
        case RH2_Haval_4_256:       RandomHash_Haval4(input, output, 256); break;
        case RH2_Haval_5_128:       RandomHash_Haval_5_256(input, output, 128); break;
        case RH2_Haval_5_160:       RandomHash_Haval_5_256(input, output, 160); break;
        case RH2_Haval_5_192:       RandomHash_Haval_5_256(input, output, 192); break;
        case RH2_Haval_5_224:       RandomHash_Haval_5_256(input, output, 224); break;
        case RH2_Haval_5_256:       RandomHash_Haval_5_256(input, output, 256); break;
        case RH2_Keccak_224:        _RandomHash_SHA3_512(input, output, 224/8, false); break;
        case RH2_Keccak_256:        _RandomHash_SHA3_512(input, output, 256/8, false); break;
        case RH2_Keccak_288:        _RandomHash_SHA3_512(input, output, 288/8, false); break;
        case RH2_Keccak_384:        _RandomHash_SHA3_512(input, output, 384/8, false); break;
        case RH2_Keccak_512:        _RandomHash_SHA3_512(input, output, 512/8, false); break;
        case RH2_MD2:               RandomHash_MD2(input, output); break;
        case RH2_MD5:               RandomHash_MD5(input, output); break;
        case RH2_MD4:               RandomHash_MD4(input, output); break;
        case RH2_Panama:            RandomHash_Panama(input, output); break;
        case RH2_RadioGatun32:      RandomHash_RadioGatun32(input, output); break;
        case RH2_RIPEMD:            RandomHash_RIPEMD(input, output); break;
        case RH2_RIPEMD128:         RandomHash_RIPEMD128(input, output); break;
        case RH2_RIPEMD160:         RandomHash_RIPEMD160(input, output); break;
        case RH2_RIPEMD256:         RandomHash_RIPEMD256(input, output); break;
        case RH2_RIPEMD320:         RandomHash_RIPEMD320(input, output); break;
        case RH2_SHA0:              RandomHash_SHA0(input, output); break;
        case RH2_SHA1:              RandomHash_SHA1(input, output); break;
        case RH2_SHA2_224:          RandomHash_SHA2_256(input, output, true); break;
        case RH2_SHA2_256:          RandomHash_SHA2_256(input, output, false); break;
        case RH2_SHA2_384:          RandomHash_SHA2_512(input, output, SHA2_512_MODE_384); break;
        case RH2_SHA2_512:          RandomHash_SHA2_512(input, output, SHA2_512_MODE_512); break;
        case RH2_SHA2_512_224:      RandomHash_SHA2_512(input, output, SHA2_512_MODE_512_224); break;
        case RH2_SHA2_512_256:      RandomHash_SHA2_512(input, output, SHA2_512_MODE_512_256); break;
        case RH2_SHA3_224:          RandomHash_SHA3_224(input, output); break;
        case RH2_SHA3_256:          RandomHash_SHA3_256(input, output); break;
        case RH2_SHA3_384:          RandomHash_SHA3_384(input, output); break;
        case RH2_SHA3_512:          RandomHash_SHA3_512(input, output); break;
        case RH2_Snefru_8_128:      RandomHash_Snefru_8_256(input, output, 16); break;
        case RH2_Snefru_8_256:      RandomHash_Snefru_8_256(input, output, 32); break;
        case RH2_Tiger_3_128:       RandomHash_Tiger(input, output, 3, 128, false); break;
        case RH2_Tiger_3_160:       RandomHash_Tiger(input, output, 3, 160, false); break;
        case RH2_Tiger_3_192:       RandomHash_Tiger(input, output, 3, 192, false); break;
        case RH2_Tiger_4_128:       RandomHash_Tiger(input, output, 4, 128, false); break;
        case RH2_Tiger_4_160:       RandomHash_Tiger(input, output, 4, 160, false); break;
        case RH2_Tiger_4_192:       RandomHash_Tiger(input, output, 4, 192, false); break;
        case RH2_Tiger_5_128:       RandomHash_Tiger(input, output, 5, 128, false); break;
        case RH2_Tiger_5_160:       RandomHash_Tiger(input, output, 5, 160, false); break;
        case RH2_Tiger_5_192:       RandomHash_Tiger(input, output, 5, 192, false); break;
        case RH2_Tiger2_3_128:      RandomHash_Tiger(input, output, 3, 128, true); break;
        case RH2_Tiger2_3_160:      RandomHash_Tiger(input, output, 3, 160, true); break;
        case RH2_Tiger2_3_192:      RandomHash_Tiger(input, output, 3, 192, true); break;
        case RH2_Tiger2_4_128:      RandomHash_Tiger(input, output, 4, 128, true); break;
        case RH2_Tiger2_4_160:      RandomHash_Tiger(input, output, 4, 160, true); break;
        case RH2_Tiger2_4_192:      RandomHash_Tiger(input, output, 4, 192, true); break;
        case RH2_Tiger2_5_128:      RandomHash_Tiger(input, output, 5, 128, true); break;
        case RH2_Tiger2_5_160:      RandomHash_Tiger(input, output, 5, 160, true); break;
        case RH2_Tiger2_5_192:      RandomHash_Tiger(input, output, 5, 192, true); break;
        case RH2_WhirlPool:         RandomHash_WhirlPool(input, output); break;
    }
}

//---------------------------------------------------------------------------
// final "veneer" round: SHA2-256 of the compression of outputs [begin, end)
RH2_NOINLINE void rh2_veneer(RH2_Thread* t, U32 begin, U32 end, U8 outHash[32])
{
    U32 seed = rh2_last_dword(rh2_entry(t, end - 1));
    rh2_compress(t, begin, end, seed, t->veneerBytes);
    rh2_sha256_bytes(RH_STRIDE_GET_DATA8(t->veneerBytes), 100, outHash, false);
}

// SHA2-256 of the block header with `nonce` in its last 4 bytes
RH2_NOINLINE void rh2_header_hash(const RH2_Header* h, U32 nonce, U8 out[32])
{
    U8 tail[RH2_HEADER_SIZE - 192];
    memcpy(tail, h->tail, sizeof(tail));
    tail[sizeof(tail) - 4] = (U8)(nonce);
    tail[sizeof(tail) - 3] = (U8)(nonce >> 8);
    tail[sizeof(tail) - 2] = (U8)(nonce >> 16);
    tail[sizeof(tail) - 1] = (U8)(nonce >> 24);
    U32 st[8];
    for (int i = 0; i < 8; i++)
        st[i] = h->midstate[i];
    rh2_sha256_finish(st, 192, tail, sizeof(tail), out, 8);
}

//---------------------------------------------------------------------------
// Result sink: implemented by the caller (host test or GPU kernel)
//   REPORT::Report(U32 nonce, const U8 hash[32])

template <typename REPORT>
RH2_DEV bool rh2_calc_round(RH2_Thread* t, const RH2_Header* h, U32 nonce, U32 round, REPORT& rep)
{
    RH2_MT<12> gen;
    U32 begin = t->entryCount;
    RH_StridePtr input = t->workBytes;

    if (round == 1)
    {
        rh2_header_hash(h, nonce, RH_STRIDE_GET_DATA8(input));
        RH_STRIDE_SET_SIZE(input, 32);
        gen.Init(rh2_last_dword(input));
    }
    else
    {
        if (rh2_calc_round(t, h, nonce, round - 1, rep))
            return true;    // parent round was final: its outputs are ours
        if (t->overflow)
            return true;

        gen.Init(rh2_last_dword(rh2_entry(t, t->entryCount - 1)));

        U32 numNeighbours = (gen.Next() % (RH2_MAX_J - RH2_MIN_J)) + RH2_MIN_J;
        for (U32 i = 0; i < numNeighbours; i++)
        {
            U32 neighbourNonce = gen.Next();
            U32 nbegin = t->entryCount;
            bool neighbourFinal = rh2_calc_round(t, h, neighbourNonce, round - 1, rep);
            if (t->overflow)
                return true;
            if (neighbourFinal)
            {
                // neighbour fully evaluated as a side effect: free hash
                U8 nh[32];
                rh2_veneer(t, nbegin, t->entryCount, nh);
                rep.Report(neighbourNonce, nh);
            }
        }
        rh2_compress(t, begin, t->entryCount, gen.Next(), input);
    }

    U32 algo = gen.Next() % RH2_MAX_ALGO;
    U32 factor = RH2_MAX_N - round;
    RH_StridePtr output = rh2_alloc(t, c_AlgoSize[algo] + factor * RH2_M);
    if (!output)
        return true;
    rh2_hash_algo(algo, input, output);
    rh2_expand(output, factor, gen.Next());

    return (round == RH2_MAX_N) || ((round >= RH2_MIN_N) && ((rh2_last_dword(output) % RH2_MAX_N) == 0));
}

// rh2_search results
#define RH2_SEARCH_OK        0   // nonce hash reported
#define RH2_SEARCH_OVERFLOW  1   // arena too small, results of this nonce dropped
#define RH2_SEARCH_SKIPPED   2   // nonce scanning: nonce needs more than topRound rounds

// Evaluate `nonce` (mining mode). Reports the nonce's hash plus every free
// neighbour hash.
// topRound < RH2_MAX_N enables "nonce scanning" (PIP-0036): only nonces whose
// evaluation ends at a round <= topRound are completed, the others are skipped
// after round topRound. The reported hashes are identical to a full evaluation,
// the work per nonce and the memory per thread are much smaller.
template <typename REPORT>
RH2_DEV int rh2_search(RH2_Thread* t, const RH2_Header* h, U32 nonce, REPORT& rep, U32 topRound = RH2_MAX_N)
{
    t->arenaUsed = 0;
    t->entryCount = 0;
    t->overflow = 0;
    bool final = rh2_calc_round(t, h, nonce, topRound, rep);
    if (t->overflow)
        return RH2_SEARCH_OVERFLOW;
    if (!final)
        return RH2_SEARCH_SKIPPED;
    U8 hash[32];
    rh2_veneer(t, 0, t->entryCount, hash);
    rep.Report(nonce, hash);
    return RH2_SEARCH_OK;
}
