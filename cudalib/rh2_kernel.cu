/**
 * ParnianMiner RandomHash2 CUDA kernel
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * Built offline into a fatbin (see cudalib/build_kernel.sh) that is embedded in
 * parnianminer and loaded at run time through the CUDA driver API, so the miner
 * needs neither the CUDA toolkit nor cudart: only the NVIDIA display driver.
 */
#include "rh2/rh2_core.h"
#include "rh2_kernel_api.h"
#ifndef RH2_NO_DIAG
#include "rh2/rh2_diag.h"
#endif

static_assert(sizeof(RH2_Header) == RH2_HEADER_BYTES, "header layout");
static_assert(sizeof(RH2_Thread) <= RH2_THREAD_BYTES, "thread scratch too big");

struct RH2_GpuReporter
{
    RH2_KernelResults* results;
    U32 target;         // upper 32 bits of the device target (big endian compare)
    U32 count;          // hashes produced by this thread

    __device__ void Report(U32 nonce, const U8* hash)
    {
        count++;
        U32 head = ((U32)hash[0] << 24) | ((U32)hash[1] << 16) | ((U32)hash[2] << 8) | (U32)hash[3];
        if (head <= target)
        {
            U32 slot = atomicAdd(&results->count, 1u);
            if (slot < RH2_MAX_KERNEL_RESULTS)
            {
                results->nonces[slot] = nonce;
                for (int i = 0; i < 32; i++)
                    results->hashes[slot][i] = hash[i];
            }
        }
    }
};

extern "C" __global__ void rh2_search_kernel(
    const RH2_Header* __restrict__ header,
    U32 startNonce,
    U32 target,
    RH2_Thread* threads,
    U32* arenas,
    U32 arenaWords,
    RH2_KernelResults* results,
    U32 flags)
{
    const U32 gid = blockIdx.x * blockDim.x + threadIdx.x;

    RH2_Thread* t = (RH2_Thread*)((U8*)threads + (size_t)gid * RH2_THREAD_BYTES);
    t->arena = arenas + (size_t)gid * arenaWords;
    t->arenaWords = arenaWords;

    RH2_Header h = *header;

    RH2_GpuReporter rep;
    rep.results = results;
    rep.target = target;
    rep.count = 0;

    U32 topRound = (flags & RH2_FLAG_SCAN_MASK) ? (flags & RH2_FLAG_SCAN_MASK) : RH2_MAX_N;
    if (rh2_search(t, &h, startNonce + gid, rep, topRound) == RH2_SEARCH_OVERFLOW)
        atomicAdd(&results->overflows, 1u);

    atomicAdd((unsigned long long*)&results->hashes64, (unsigned long long)rep.count);
}

#ifndef RH2_NO_DIAG
//------------------------------------------------------------------------------
// Diagnostics (-gpudiag), see rh2/rh2_diag.h
extern "C" __global__ void rh2_diag_kernel(
    U32 mode,
    U32 param,
    const RH2_Header* __restrict__ header,
    RH2_Thread* threads,
    U32* arenas,
    U32 arenaWords,
    U32* out)
{
    const U32 gid = blockIdx.x * blockDim.x + threadIdx.x;
    RH2_Thread* t = (RH2_Thread*)((U8*)threads + (size_t)gid * RH2_THREAD_BYTES);
    t->arena = arenas + (size_t)gid * arenaWords;
    t->arenaWords = arenaWords;
    RH2_Header h = *header;
    out[gid] = rh2_diag_compute(t, &h, mode, param, gid);
}
#endif // RH2_NO_DIAG
