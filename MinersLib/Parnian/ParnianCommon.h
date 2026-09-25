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

#pragma once
#include "corelib/basetypes.h"

const static U32 ParnianHeaderSize        = 200; //Fixed header size for pool mining RandomHash1
const static U32 ParnianHeaderSizeV5      = 236; //Fixed header size for pool mining RandomHash2
#define          ParnianHeaderNoncePosV4(headerSize) (headerSize - 4)

#define RH2_StrideArrayCount         (1024+64)

#define RH2_MIN_N 2
#define RH2_MAX_N 4
#define RH2_MIN_J 1
#define RH2_MAX_J 8
#define RH2_M 64
#define RH2_StrideSize           (208896)


#define RH_CheckerSize          (sizeof(U64))

#define RH_IDEAL_ALIGNMENT      64  
#define RH_IDEAL_ALIGNMENT32    (RH_IDEAL_ALIGNMENT/4)


