/**
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

///
/// @file
/// @copyright Amir Reza Zamani <amirrezazamani@gmail.com>
/// @author Amir Reza Zamani <amirrezazamani@gmail.com>

#include "RandomHash_core.h"
#include "MinersLib/Algo/sph_sha0.h"
#include "MinersLib/Algo/sph_sha1.h"

void RandomHash_SHA0(RH_StridePtr roundInput, RH_StridePtr output)
{
    U32 msgLen = RH_STRIDE_GET_SIZE(roundInput);
    U64* message = RH_STRIDE_GET_DATA64(roundInput);

    sph_sha0_context ctx;
    sph_sha0_init(&ctx);
    sph_sha0(&ctx, message, msgLen);

    //get the hash result
    sph_sha0_close(&ctx, RH_STRIDE_GET_DATA(output));
    RH_STRIDE_SET_SIZE(output, 20);
}


void RandomHash_SHA1(RH_StridePtr roundInput, RH_StridePtr output)
{
    U32 msgLen = RH_STRIDE_GET_SIZE(roundInput);
    U64* message = RH_STRIDE_GET_DATA64(roundInput);

    sph_sha1_context ctx;
    sph_sha1_init(&ctx);
    sph_sha1(&ctx, message, msgLen);

    //get the hash result
    sph_sha1_close(&ctx, RH_STRIDE_GET_DATA(output));
    RH_STRIDE_SET_SIZE(output, 20);
}
