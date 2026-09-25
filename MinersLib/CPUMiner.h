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

#include "corelib/Worker.h"
#include "corelib/ParnianWork.h"

struct CPUKernelData
{
    struct RH_ALIGN(64) DataPackage
    {
        union
        {
            U8  asU8[256];
            U32 asU32[256/4];
        }                         m_header;
        RH_ALIGN(64) U8           m_targetFull[32];
        RH_ALIGN(64) U64          m_target;
        RH_ALIGN(64) U64          m_requestPause;
        RH_ALIGN(64) U8           m_workID[64];
        RH_ALIGN(64) U64          m_nonce2;
        RH_ALIGN(64) U64          m_rndVal;
        RH_ALIGN(64) U64          m_headerSize;
        RH_ALIGN(64) U8           m_work1[4096+1024];       //return state for RandomHash
    };
    
    static const int PackagesCount = 16;
    RH_ALIGN(64) U64             m_hashes = 0;
    RH_ALIGN(64) DataPackage     m_packages[PackagesCount];
    RH_ALIGN(64) U32             m_abortThread = 0;
    RH_ALIGN(64) U64             m_isSolo;
    RH_ALIGN(64) U64             m_packageID = 0;
    RH_ALIGN(64) U32             m_id;               //id in the array of cpu kernels
    std::thread*                 m_thread;    
};
