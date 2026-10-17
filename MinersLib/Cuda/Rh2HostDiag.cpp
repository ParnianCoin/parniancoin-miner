/**
 * Host (CPU) copy of the RandomHash2 kernel code, used by -gpudiag to check
 * the results computed by the GPU
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * Compiled in its own namespace so its names do not clash with the CPU miner.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <vector>

namespace rh2host
{
#include "cudalib/rh2/rh2_diag.h"
}

// gid-th result of rh2_diag_kernel, computed on the CPU
uint32_t Rh2HostDiag(uint32_t mode, uint32_t param, const uint8_t* header236, uint32_t gid)
{
    using namespace rh2host;
    static thread_local std::vector<U32> arena;
    static thread_local RH2_Thread* t = nullptr;
    if (!t)
    {
        arena.resize(RH2_ARENA_DEFAULT / 4);
        t = new RH2_Thread;
    }
    t->arena = arena.data();
    t->arenaWords = (U32)arena.size();

    RH2_Header h;
    for (int i = 0; i < 8; i++)
        h.midstate[i] = rh2_sha256_IV[i];
    for (int b = 0; b < 3; b++)
        rh2_sha256_block(h.midstate, header236 + 64 * b);
    memcpy(h.tail, header236 + 192, sizeof(h.tail));

    return rh2_diag_compute(t, &h, mode, param, gid);
}
