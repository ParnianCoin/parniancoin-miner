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
#include "MinersLib/Algo/sph-haval.h"

void RandomHash_Haval4(RH_StridePtr roundInput, RH_StridePtr output, U32 bitSize)
{
    U32 msgLen = RH_STRIDE_GET_SIZE(roundInput);
    U32* message = RH_STRIDE_GET_DATA(roundInput);

    sph_haval_context cc;
    haval_init(&cc, bitSize >> 5, 4);
    haval4(&cc, message, msgLen);


    RH_STRIDE_SET_SIZE(output, bitSize >> 3);
    uint32_t* buf = RH_STRIDE_GET_DATA(output);
    haval4_close(&cc, 0, 0, buf);
}

void RandomHash_Haval3(RH_StridePtr roundInput, RH_StridePtr output, U32 bitSize)
{
    U32 msgLen = RH_STRIDE_GET_SIZE(roundInput);
    U32* message = RH_STRIDE_GET_DATA(roundInput);

    sph_haval_context cc;
    haval_init(&cc, bitSize >> 5, 3);
    haval3(&cc, message, msgLen);


    RH_STRIDE_SET_SIZE(output, bitSize >> 3);
    uint32_t* buf = RH_STRIDE_GET_DATA(output);
    haval3_close(&cc, 0, 0, buf);

}

