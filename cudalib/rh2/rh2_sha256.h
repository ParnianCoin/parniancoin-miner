/**
 * Portable SHA2-256 / SHA2-224 for RandomHash2 (host C++ / CUDA device)
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

RH2_CONST U32 rh2_sha256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

RH2_CONST U32 rh2_sha256_IV[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

RH2_CONST U32 rh2_sha224_IV[8] = {
    0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
};

// compress one 64 byte block (big endian message words)
RH2_DEV void rh2_sha256_block(U32 state[8], const U8* block)
{
    U32 w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((U32)block[i * 4] << 24) | ((U32)block[i * 4 + 1] << 16) | ((U32)block[i * 4 + 2] << 8) | (U32)block[i * 4 + 3];
    for (int i = 16; i < 64; i++)
    {
        U32 s0 = ROTR32(w[i - 15], 7) ^ ROTR32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        U32 s1 = ROTR32(w[i - 2], 17) ^ ROTR32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    U32 a = state[0], b = state[1], c = state[2], d = state[3];
    U32 e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; i++)
    {
        U32 S1 = ROTR32(e, 6) ^ ROTR32(e, 11) ^ ROTR32(e, 25);
        U32 ch = (e & f) ^ (~e & g);
        U32 t1 = h + S1 + ch + rh2_sha256_K[i] + w[i];
        U32 S0 = ROTR32(a, 2) ^ ROTR32(a, 13) ^ ROTR32(a, 22);
        U32 mj = (a & b) ^ (a & c) ^ (b & c);
        U32 t2 = S0 + mj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// finish a message whose first `doneBytes` bytes (multiple of 64) were already
// compressed into `state`; `tail` holds the remaining `tailLen` bytes
RH2_DEV void rh2_sha256_finish(U32 state[8], U64 doneBytes, const U8* tail, U32 tailLen, U8* out, U32 outWords)
{
    U8 block[64];
    U64 totalBits = (doneBytes + tailLen) * 8;
    while (tailLen >= 64)
    {
        rh2_sha256_block(state, tail);
        tail += 64;
        tailLen -= 64;
    }
    memset(block, 0, 64);
    memcpy(block, tail, tailLen);
    block[tailLen] = 0x80;
    if (tailLen >= 56)
    {
        rh2_sha256_block(state, block);
        memset(block, 0, 64);
    }
    for (int i = 0; i < 8; i++)
        block[63 - i] = (U8)(totalBits >> (8 * i));
    rh2_sha256_block(state, block);
    for (U32 i = 0; i < outWords; i++)
    {
        out[i * 4 + 0] = (U8)(state[i] >> 24);
        out[i * 4 + 1] = (U8)(state[i] >> 16);
        out[i * 4 + 2] = (U8)(state[i] >> 8);
        out[i * 4 + 3] = (U8)(state[i]);
    }
}

RH2_DEV void rh2_sha256_bytes(const U8* data, U32 len, U8* out, bool is224)
{
    U32 st[8];
    for (int i = 0; i < 8; i++)
        st[i] = is224 ? rh2_sha224_IV[i] : rh2_sha256_IV[i];
    rh2_sha256_finish(st, 0, data, len, out, is224 ? 7 : 8);
}

// stride interface used by the RandomHash2 algorithm switch
RH2_DEV void RandomHash_SHA2_256(RH_StridePtr roundInput, RH_StridePtr output, bool is224)
{
    rh2_sha256_bytes(RH_STRIDE_GET_DATA8(roundInput), RH_STRIDE_GET_SIZE(roundInput),
                     RH_STRIDE_GET_DATA8(output), is224);
    RH_STRIDE_SET_SIZE(output, is224 ? 28 : 32);
}
