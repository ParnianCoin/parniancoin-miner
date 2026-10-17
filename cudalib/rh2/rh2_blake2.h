/**
 * Portable BLAKE2b / BLAKE2s (unkeyed, sequential) for RandomHash2
 * (host C++ / CUDA device)
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

#include "rh2_platform.h"

RH2_CONST U8 rh2_blake2_sigma[12][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

RH2_CONST U64 rh2_blake2b_IV[8] = {
    0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull, 0xa54ff53a5f1d36f1ull,
    0x510e527fade682d1ull, 0x9b05688c2b3e6c1full, 0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull
};

RH2_CONST U32 rh2_blake2s_IV[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

RH2_DEV void rh2_blake2b_compress(U64 h[8], const U8* block, U64 t, bool last)
{
    U64 m[16], v[16];
    for (int i = 0; i < 16; i++)
    {
        U64 w = 0;
        for (int b = 7; b >= 0; b--)
            w = (w << 8) | block[i * 8 + b];
        m[i] = w;
    }
    for (int i = 0; i < 8; i++)
    {
        v[i] = h[i];
        v[i + 8] = rh2_blake2b_IV[i];
    }
    v[12] ^= t;
    if (last)
        v[14] = ~v[14];

#define RH2_B2B_G(a, b, c, d, x, y)              \
    v[a] = v[a] + v[b] + (x); v[d] = ROTR64(v[d] ^ v[a], 32); \
    v[c] = v[c] + v[d];       v[b] = ROTR64(v[b] ^ v[c], 24); \
    v[a] = v[a] + v[b] + (y); v[d] = ROTR64(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];       v[b] = ROTR64(v[b] ^ v[c], 63);

    for (int r = 0; r < 12; r++)
    {
        const U8* s = rh2_blake2_sigma[r];
        RH2_B2B_G(0, 4,  8, 12, m[s[0]],  m[s[1]]);
        RH2_B2B_G(1, 5,  9, 13, m[s[2]],  m[s[3]]);
        RH2_B2B_G(2, 6, 10, 14, m[s[4]],  m[s[5]]);
        RH2_B2B_G(3, 7, 11, 15, m[s[6]],  m[s[7]]);
        RH2_B2B_G(0, 5, 10, 15, m[s[8]],  m[s[9]]);
        RH2_B2B_G(1, 6, 11, 12, m[s[10]], m[s[11]]);
        RH2_B2B_G(2, 7,  8, 13, m[s[12]], m[s[13]]);
        RH2_B2B_G(3, 4,  9, 14, m[s[14]], m[s[15]]);
    }
#undef RH2_B2B_G
    for (int i = 0; i < 8; i++)
        h[i] ^= v[i] ^ v[i + 8];
}

RH2_DEV void RandomHash_blake2b(RH_StridePtr roundInput, RH_StridePtr output, U32 bitSize)
{
    const U32 outlen = bitSize / 8;
    const U8* in = RH_STRIDE_GET_DATA8(roundInput);
    U32 len = RH_STRIDE_GET_SIZE(roundInput);
    U64 h[8];
    for (int i = 0; i < 8; i++)
        h[i] = rh2_blake2b_IV[i];
    h[0] ^= 0x01010000ull ^ outlen;

    U64 t = 0;
    while (len > 128)
    {
        t += 128;
        rh2_blake2b_compress(h, in, t, false);
        in += 128;
        len -= 128;
    }
    U8 block[128];
    memset(block, 0, 128);
    memcpy(block, in, len);
    t += len;
    rh2_blake2b_compress(h, block, t, true);

    U8* out = RH_STRIDE_GET_DATA8(output);
    for (U32 i = 0; i < outlen; i++)
        out[i] = (U8)(h[i >> 3] >> (8 * (i & 7)));
    RH_STRIDE_SET_SIZE(output, outlen);
}

RH2_DEV void rh2_blake2s_compress(U32 h[8], const U8* block, U32 t, bool last)
{
    U32 m[16], v[16];
    for (int i = 0; i < 16; i++)
        m[i] = (U32)block[i * 4] | ((U32)block[i * 4 + 1] << 8) | ((U32)block[i * 4 + 2] << 16) | ((U32)block[i * 4 + 3] << 24);
    for (int i = 0; i < 8; i++)
    {
        v[i] = h[i];
        v[i + 8] = rh2_blake2s_IV[i];
    }
    v[12] ^= t;
    if (last)
        v[14] = ~v[14];

#define RH2_B2S_G(a, b, c, d, x, y)              \
    v[a] = v[a] + v[b] + (x); v[d] = ROTR32(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];       v[b] = ROTR32(v[b] ^ v[c], 12); \
    v[a] = v[a] + v[b] + (y); v[d] = ROTR32(v[d] ^ v[a], 8);  \
    v[c] = v[c] + v[d];       v[b] = ROTR32(v[b] ^ v[c], 7);

    for (int r = 0; r < 10; r++)
    {
        const U8* s = rh2_blake2_sigma[r];
        RH2_B2S_G(0, 4,  8, 12, m[s[0]],  m[s[1]]);
        RH2_B2S_G(1, 5,  9, 13, m[s[2]],  m[s[3]]);
        RH2_B2S_G(2, 6, 10, 14, m[s[4]],  m[s[5]]);
        RH2_B2S_G(3, 7, 11, 15, m[s[6]],  m[s[7]]);
        RH2_B2S_G(0, 5, 10, 15, m[s[8]],  m[s[9]]);
        RH2_B2S_G(1, 6, 11, 12, m[s[10]], m[s[11]]);
        RH2_B2S_G(2, 7,  8, 13, m[s[12]], m[s[13]]);
        RH2_B2S_G(3, 4,  9, 14, m[s[14]], m[s[15]]);
    }
#undef RH2_B2S_G
    for (int i = 0; i < 8; i++)
        h[i] ^= v[i] ^ v[i + 8];
}

RH2_DEV void RandomHash_blake2s(RH_StridePtr roundInput, RH_StridePtr output, U32 bitSize)
{
    const U32 outlen = bitSize / 8;
    const U8* in = RH_STRIDE_GET_DATA8(roundInput);
    U32 len = RH_STRIDE_GET_SIZE(roundInput);
    U32 h[8];
    for (int i = 0; i < 8; i++)
        h[i] = rh2_blake2s_IV[i];
    h[0] ^= 0x01010000u ^ outlen;

    U32 t = 0;
    while (len > 64)
    {
        t += 64;
        rh2_blake2s_compress(h, in, t, false);
        in += 64;
        len -= 64;
    }
    U8 block[64];
    memset(block, 0, 64);
    memcpy(block, in, len);
    t += len;
    rh2_blake2s_compress(h, block, t, true);

    U8* out = RH_STRIDE_GET_DATA8(output);
    for (U32 i = 0; i < outlen; i++)
        out[i] = (U8)(h[i >> 2] >> (8 * (i & 3)));
    RH_STRIDE_SET_SIZE(output, outlen);
}
