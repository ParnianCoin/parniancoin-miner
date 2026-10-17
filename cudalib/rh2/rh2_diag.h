/**
 * RandomHash2 diagnostics shared by the CUDA kernel and the host check
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * Runs parts of the algorithm in isolation so a device fault or a wrong result
 * can be located on GPUs that are not available to the developers:
 *   mode 0: plain memory write
 *   mode 1: SHA2-256 of the header (round 1 input)
 *   mode 2: one hash algorithm (param = algo index) on a 100 byte input
 *   mode 3: Expand + Compress on synthetic strides
 *   mode 4: full RandomHash2 search (same as the mining kernel)
 * Basic building blocks (to locate faults below the algorithm level):
 *   mode 10: per-thread local memory (array in thread stack)
 *   mode 11: call of a non-inlined function with a local array
 *   mode 12: read of the constant tables in global memory
 *   mode 13: memcpy from the header (global) into local memory
 *   mode 14: SHA2-256 of a local 100 byte buffer
 *   mode 15: SHA2-256 of the header, without touching the thread state
 * Returns a 32-bit checksum of the produced data.
 */
#pragma once

#include "rh2_core.h"

struct RH2_DiagReporter
{
    U32 sum;
    RH2_DEV void Report(U32 nonce, const U8* hash)
    {
        for (int i = 0; i < 32; i++)
            sum = sum * 31u + hash[i];
        sum ^= nonce;
    }
};

RH2_NOINLINE U32 rh2_diag_noinline(U32 x)
{
    U32 a[32];
    for (U32 i = 0; i < 32; i++)
        a[i] = x * (i + 1) + (i << 3);
    U32 s = 0;
    for (U32 i = 0; i < 32; i++)
        s = s * 31u + a[(i * 7 + x) & 31];
    return s;
}

RH2_DEV U32 rh2_diag_compute(RH2_Thread* t, const RH2_Header* h, U32 mode, U32 param, U32 gid)
{
    if (mode == 0)
        return gid * 2654435761u;
    if (mode == 10)
    {
        U32 a[64];   // runtime indices keep it in thread local memory
        for (U32 i = 0; i < 64; i++)
            a[(i * 7 + gid) & 63] = i * gid + 1;
        U32 s = 0;
        for (U32 i = 0; i < 64; i++)
            s = s * 31u + a[i];
        return s;
    }
    if (mode == 11)
        return rh2_diag_noinline(gid);
    if (mode == 12)
    {
        U32 s = 0;
        for (U32 i = 0; i < 64; i++)
            s = s * 31u + rh2_sha256_K[(i + gid) & 63];
        for (U32 i = 0; i < RH2_MAX_ALGO; i++)
            s = s * 31u + c_AlgoSize[i];
        return s;
    }
    if (mode == 13)
    {
        U8 tail[RH2_HEADER_SIZE - 192];
        memcpy(tail, h->tail, sizeof(tail));
        U32 s = 0;
        for (U32 i = 0; i < sizeof(tail); i++)
            s = s * 31u + tail[(i + gid) % sizeof(tail)];
        return s;
    }
    if (mode == 14)
    {
        U8 buf[100], out[32];
        for (U32 i = 0; i < 100; i++)
            buf[i] = (U8)(i * gid + 7);
        rh2_sha256_bytes(buf, 100, out, false);
        U32 s = 0;
        for (int i = 0; i < 32; i++)
            s = s * 31u + out[i];
        return s;
    }
    if (mode == 15)
    {
        U8 hh[32];
        rh2_header_hash(h, gid, hh);
        U32 s = 0;
        for (int i = 0; i < 32; i++)
            s = s * 31u + hh[i];
        return s;
    }

    t->arenaUsed = 0;
    t->entryCount = 0;
    t->overflow = 0;
    U32 sum = 0;

    if (mode == 1)
    {
        U8 hh[32];
        rh2_header_hash(h, gid, hh);
        for (int i = 0; i < 32; i++)
            sum = sum * 31u + hh[i];
    }
    else if (mode == 2 || mode == 3)
    {
        U8 hh[32];
        rh2_header_hash(h, gid, hh);
        RH_StridePtr input = t->workBytes;
        U8* in8 = RH_STRIDE_GET_DATA8(input);
        for (int i = 0; i < 100; i++)
            in8[i] = hh[i % 32] ^ (U8)i;
        RH_STRIDE_SET_SIZE(input, 100);

        if (mode == 2)
        {
            U32 algo = param % RH2_MAX_ALGO;
            RH_StridePtr output = rh2_alloc(t, c_AlgoSize[algo] + 3 * RH2_M);
            if (output)
            {
                rh2_hash_algo(algo, input, output);
                U32 n = RH_STRIDE_GET_SIZE(output);
                U8* o = RH_STRIDE_GET_DATA8(output);
                for (U32 i = 0; i < n && i < 512; i++)
                    sum = sum * 31u + o[i];
                sum += n;
            }
        }
        else
        {
            for (U32 k = 0; k < 6; k++)
            {
                RH_StridePtr s = rh2_alloc(t, 64 + 3 * RH2_M);
                if (!s)
                    break;
                U8* d = RH_STRIDE_GET_DATA8(s);
                for (int i = 0; i < 32; i++)
                    d[i] = hh[i] ^ (U8)(k * 17);
                RH_STRIDE_SET_SIZE(s, 32);
                rh2_expand(s, 3, gid + k);
            }
            rh2_compress(t, 0, t->entryCount, gid, input);
            for (int i = 0; i < 100; i++)
                sum = sum * 31u + RH_STRIDE_GET_DATA8(input)[i];
        }
    }
    else
    {
        RH2_DiagReporter rep;
        rep.sum = 0;
        rh2_search(t, h, gid * 7919u, rep);
        sum = rep.sum ^ t->overflow;
    }
    return sum;
}
