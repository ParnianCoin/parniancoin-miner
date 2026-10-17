/**
 * Data shared between the RandomHash2 CUDA kernel and the host miner
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

#define RH2_MAX_KERNEL_RESULTS  64
#define RH2_KERNEL_NAME         "rh2_search_kernel"
#define RH2_HEADER_SIZE_BYTES   236     // PascalCoin V5 mining header, nonce in the last 4 bytes
#define RH2_HEADER_BYTES        76      // sizeof(RH2_Header): SHA-256 midstate + header tail
#define RH2_THREAD_BYTES        4096    // device memory reserved per thread for RH2_Thread

// rh2_search_kernel flags
#define RH2_FLAG_SCAN_MASK      0x7     // 0: full evaluation, 2 or 3: nonce scanning up to that round

// layout must be identical on host and device
struct RH2_KernelResults
{
    uint32_t count;                                 // candidates found (may exceed capacity)
    uint32_t overflows;                             // nonces dropped: arena too small
    uint64_t hashes64;                              // total hashes computed in this launch
    uint32_t nonces[RH2_MAX_KERNEL_RESULTS];
    uint8_t  hashes[RH2_MAX_KERNEL_RESULTS][32];
};
