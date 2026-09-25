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

#include "MinersLib/GenericCLMiner.h"
#include "MinersLib/CPUMiner.h"
#include "MinersLib/Parnian/RandomHash.h"
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("cputhrottling", g_cputhrottling, "General", "Slow down mining by resting the cpu part of the time.\nUseful to keep the computer responsive or cooler.\nMin-Max are 0 and 99.\nEx. -cputhrottling 12 will rest the cpu 12% of the time", 0, 99);

class RandomHashCPUMiner: public GenericCLMiner
{
    
public:
    RandomHashCPUMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
    ~RandomHashCPUMiner();

    virtual bool init(const ParnianWorkSptr& work);
    virtual void InitFromFarm(U32 relativeIndex);
    static bool configureGPU();
    virtual PlatformType GetPlatformType() { return PlatformType_CPU; }

    virtual void Pause();
    virtual void Kill();
    virtual void SetWork(ParnianWorkSptr _work);


protected:
    
    vector<CPUKernelData*>   m_cpuKernels; //paged alloc
    Event  m_firstKernelCycleDone;
    U32    m_setWorkComming = 0;
    U32    m_waitingForKernel = 1;
    U32    m_lastIttCount = 0;
    U32    m_isPaused = 0;
    std::mutex  m_pauseMutex;
    U32    m_globalWorkSizePerCPUMiner = 0;
    mersenne_twister_state   m_rnd32;

    virtual KernelCodeAndFuctions GetKernelsCodeAndFunctions() { return KernelCodeAndFuctions(); }
    virtual void ClearKernelOutputBuffer() {}
    virtual void EvalKernelResult() {}

    virtual PrepareWorkStatus PrepareWork(const ParnianWorkSptr& workTempl, bool reuseCurrentWP = false);
    virtual void SendWorkPackageToKernels(ParnianWorkPackage* wp, bool requestPause = false);
    virtual void QueueKernel();
    virtual void AddHashCount(U64 hashes);
    virtual U64 GetHashRatePerSec();
    std::vector<U64> m_lastHashReading;

    void PauseCpuKernel();
    void UpdateWorkSize(U32 absoluteVal);
    void RandomHashCpuKernel(CPUKernelData* kernelData); 
    RandomHash_State* m_randomHash2Array = 0;    
};

