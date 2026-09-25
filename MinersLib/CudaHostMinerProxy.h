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

#include "MinersLib/CLMinerBase.h"
#include "cudalib/RandomHashCUDAMiner.h"


//Hold the RandomHashCUDAMiner instance
class CudaHostMinerProxy
{
public:
    CudaHostMinerProxy(RandomHashCUDAMiner* _cudaMiner):miner(_cudaMiner){}
    ~CudaHostMinerProxy()
    {
        DestroyCudaMiner(miner);
        miner = 0;
    }

    RandomHashCUDAMiner* operator->() const { return miner; }

protected:
    RandomHashCUDAMiner*   miner;
};
