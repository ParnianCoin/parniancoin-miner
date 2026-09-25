/**
 * RandomHash source code implementation
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
#include "MinersLib/CLMinerBase.h"
#include "MinersLib/Parnian/ParnianCommon.h"
 
class GenericCLMiner: public CLMinerBase
{    
public:
    GenericCLMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
    ~GenericCLMiner();
    void KernelCallBack();
    virtual bool init(const ParnianWorkSptr& work);

protected:
    bool IsWorkStalled();
    virtual bool WorkLoopStep();

    // Get all codes and kernel name
    virtual U32 GetOutputMaxCount(){ return MAX_GPUS; }
    virtual U32 GetOutputBufferSize() {return (GetOutputMaxCount() + 1)*sizeof(U32);}
    virtual U32 GetHeaderBufferSize() { return ParnianHeaderSizeV5 ; }


    // must use m_queue in m_context to load/store buffers
    virtual void                  QueueKernel();
    virtual PrepareWorkStatus     PrepareWork(const ParnianWorkSptr& workTempl, bool reuseCurrentWP = false);
    virtual void                  EvalKernelResult();
    virtual SolutionSptr          MakeSubmitSolution(const std::vector<U64>& nonces, U64 nonce2, bool isFromCpuMiner);
    virtual void                  SetSearchKernelCurrentTarget(U32 paramIndex, cl::Kernel& searchKernel);
    virtual void                  ClearKernelOutputBuffer(); 
    virtual KernelCodeAndFuctions GetKernelsCodeAndFunctions(); 

    //work management
    U64             m_lastWorkStartTimeMs = 0;
    U32             m_kernelItterations = 0;
    U64             m_workOffset = 0; 
    U64             m_startNonce = 0;
    unsigned        m_recycleCount = 0;
    bool            m_sleepWhenWorkFinished= false;
    
    static const int MaxWorkPackageTimeout = (5 * 60);
    unsigned        m_maxRetryCycleCount = 2;

    ParnianWorkSptr m_lastWorkTemplate; 
    ParnianWorkSptr m_currentWp;  

    //make a zero buff for fast reset later on
    bytes            m_zeroBuffer;
    std::vector<U32> m_results;

    cl::Kernel m_searchKernel; //default kernel
    cl::Buffer m_kernelHeader;  
    cl::Buffer m_kernelOutput;
};


