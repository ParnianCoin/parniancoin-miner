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
#include "MinersLib/Miner.h"
#include "GpuManager.h"
#include "parnianminer/CommandLineManager.h"

#define OPENCL_PLATFORM_UNKNOWN 0
#define OPENCL_PLATFORM_NVIDIA  1
#define OPENCL_PLATFORM_AMD     2



typedef vector<std::pair<bytes,strings> > KernelCodeAndFuctions;
typedef std::function<void(string& code)> PreBuildFunctor;
enum PrepareWorkStatus {PrepareWork_NewWork, PrepareWork_WaitForNewWork, PrepareWork_Timeout, PrepareWork_Nothing};

class CLMinerBase: public Miner
{
    
public:
	CLMinerBase(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
	~CLMinerBase();

    /////////////////////////////////////////////////////////
    // Static CL Stuff
    static void addDefinition(string& _source, char const* _id, unsigned _value);

    //Base API
    void            ClearKernels();
    virtual bool    BuildKernels(const ParnianWorkSptr& work);
    void            AddPreBuildFunctor(PreBuildFunctor f) { m_OnPrebuildKernel.push_back(f); }
    virtual bool    ShouldInitialize(const ParnianWorkSptr& work);
    virtual void    InitFromFarm(U32 i);
    virtual bool    WorkLoopStep() { PARNIANMINER_EXIT_APP("9"); return false; }

    virtual void    UpdateWorkSize(U32 absoluteVal);
    virtual PlatformType GetPlatformType() { return PlatformType_OpenCL; }

    void SetOfflineCompileOptions(const string& deviceOptions, const string& definesCombination);

protected:
    // create buffers and init states
    virtual bool                        init( const ParnianWorkSptr& work);
    virtual KernelCodeAndFuctions       GetKernelsCodeAndFunctions() = 0;

    cl::Kernel  GetKernel(U32 codeID, const char* name);
    void        RemoveKernel(U32 codeID, const char* name);

    //////////////////////////////////////////////////////////////////////////////////////////
    //  generic form
    //
    cl::Context                     m_context;
	cl::CommandQueue                m_queue;
    std::vector<U32>                m_lastCodeCRC;
    std::vector<std::vector<pair<string, cl::Kernel>>> m_kernels;
    std::vector<PreBuildFunctor>    m_OnPrebuildKernel;
        
    std::vector<std::auto_ptr<bytes>> m_queuedBuffers;
    void    FreeQueuedBuffers();
    bytes*  AllocQueuedBuffer(size_t size);

    //Device Capability
    size_t      m_deviceMaxWorkgroupSize = 0;
    cl_uint     m_deviceMaxComputeUnit = 0;
    std::vector<size_t> m_deviceMaxWorkItemSize;
    cl_ulong    m_deviceMaxMemSize = 0;
    cl_uint     m_MemCacheLineSize = 0;
    cl_ulong    m_MemCacheSize = 0;
    cl_ulong    m_LocalMemCacheSize = 0;
    int         m_devicePreferedVectSize = 0;

    bool        m_isInitialized = false;
    int         m_platformType = 0;
    string      m_deviceOptions;
    string      m_definesCombination;                

};


extern std::mutex*  gs_sequentialBuildMutex;

