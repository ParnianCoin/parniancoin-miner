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

#include "precomp.h"
#include "KernelOffsetManager.h"

inline U32 Rand32Range(U32 _min, U32 _max)
{
    U32 spot= 0;
    spot = _min + (rand32() % (_max-_min));
    return spot;
}

extern bool g_forceSequentialNonce;
U64 KernelOffsetManager::m_value = 0;
U32 KernelOffsetManager::m_searchNonce = 0;

void KernelOffsetManager::Reset(U64 val)
{ 
    if (!g_forceSequentialNonce)
        val = Rand32Range(0, U32_Max - PARNIANMINER_KB(10));

    AtomicSet(m_value, val);
}

U64 KernelOffsetManager::Increment(U32 increment)
{ 
    U64 val;
    val = AtomicAdd(m_value, increment) % U32_Max;
    return val;
}

