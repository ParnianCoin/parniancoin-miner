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

#include "RandomHashCLMiner.h"

#ifndef RH_COMPILE_CPU_ONLY
#include "MinersLib/CudaHostMinerProxy.h"

class RandomHashHostCudaMiner: public RandomHashCLMiner
{
    
public:
	RandomHashHostCudaMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
	~RandomHashHostCudaMiner() override;

    static bool  configureGPU();
    virtual U32  GetOutputBufferSize() {return (/*CUDA_SEARCH_RESULT_BUFFER_SIZE*/1024 + 1)*sizeof(U32);}
    
    virtual void ClearKernelOutputBuffer() { m_cudaMinerProxy->ClearKernelOutputBuffer();  }
    virtual PlatformType GetPlatformType() { return PlatformType_CUDA; }

    virtual bool init(const ParnianWorkSptr& work);
    virtual void InitFromFarm(U32 relativeIndex);

protected:
    virtual PrepareWorkStatus PrepareWork(const ParnianWorkSptr& workTempl, bool reuseCurrentWP = false);

    bool BuildKernels(const ParnianWorkSptr& work) { return true; }

    virtual void QueueKernel()
    {
        m_cudaMinerProxy->QueueKernel();

        //inc stats
        m_kernelItterations++;

        m_workOffset = KernelOffsetManager::Increment(m_globalWorkSize) - m_globalWorkSize;
        m_cudaMinerProxy->SetStartNonce(m_workOffset);
    }

    virtual void EvalKernelResult();
    CudaHostMinerProxy m_cudaMinerProxy;
};

#endif //RH_COMPILE_CPU_ONLY