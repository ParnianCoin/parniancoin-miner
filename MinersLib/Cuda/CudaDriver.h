/**
 * Minimal CUDA driver API loader (no CUDA SDK needed at build or run time)
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * The driver library (libcuda.so.1 / nvcuda.dll) ships with the NVIDIA display
 * driver, so a miner built with mingw or gcc can use NVIDIA GPUs without any
 * extra installation. If the library is missing, GPU mining is simply disabled.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(_WIN64)
    #define RH_CUDAAPI __stdcall
#else
    #define RH_CUDAAPI
#endif

namespace cudrv
{
    typedef int                 CUresult;
    typedef int                 CUdevice;
    typedef struct CUctx_st*    CUcontext;
    typedef struct CUmod_st*    CUmodule;
    typedef struct CUfunc_st*   CUfunction;
    typedef struct CUstream_st* CUstream;
    typedef unsigned long long  CUdeviceptr;
    typedef struct CUevent_st*  CUevent;

    enum
    {
        CUDA_SUCCESS                                    = 0,
        CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK       = 1,
        CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT        = 16,
        CU_DEVICE_ATTRIBUTE_PCI_BUS_ID                  = 33,
        CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR = 39,
        CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR    = 75,
        CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR    = 76,
        CU_LIMIT_STACK_SIZE                             = 0,
        CU_CTX_SCHED_AUTO                               = 0,
        CU_CTX_SCHED_SPIN                               = 1,
        CU_CTX_SCHED_YIELD                              = 2,
        CU_CTX_SCHED_BLOCKING_SYNC                      = 4,
        CU_EVENT_DEFAULT                                = 0,
        CU_EVENT_BLOCKING_SYNC                          = 1,
        CU_EVENT_DISABLE_TIMING                         = 2,
        CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK         = 0,
        CU_FUNC_ATTRIBUTE_NUM_REGS                      = 4,
    };

    struct Api
    {
        CUresult (RH_CUDAAPI *cuInit)(unsigned int);
        CUresult (RH_CUDAAPI *cuDriverGetVersion)(int*);
        CUresult (RH_CUDAAPI *cuDeviceGetCount)(int*);
        CUresult (RH_CUDAAPI *cuDeviceGet)(CUdevice*, int);
        CUresult (RH_CUDAAPI *cuDeviceGetName)(char*, int, CUdevice);
        CUresult (RH_CUDAAPI *cuDeviceTotalMem)(size_t*, CUdevice);
        CUresult (RH_CUDAAPI *cuDeviceGetAttribute)(int*, int, CUdevice);
        CUresult (RH_CUDAAPI *cuCtxCreate)(CUcontext*, unsigned int, CUdevice);
        CUresult (RH_CUDAAPI *cuCtxDestroy)(CUcontext);
        CUresult (RH_CUDAAPI *cuCtxSetCurrent)(CUcontext);
        CUresult (RH_CUDAAPI *cuCtxSetLimit)(int, size_t);
        CUresult (RH_CUDAAPI *cuCtxSynchronize)(void);
        CUresult (RH_CUDAAPI *cuMemGetInfo)(size_t*, size_t*);
        CUresult (RH_CUDAAPI *cuModuleLoadData)(CUmodule*, const void*);
        CUresult (RH_CUDAAPI *cuModuleUnload)(CUmodule);
        CUresult (RH_CUDAAPI *cuModuleGetFunction)(CUfunction*, CUmodule, const char*);
        CUresult (RH_CUDAAPI *cuFuncGetAttribute)(int*, int, CUfunction);
        CUresult (RH_CUDAAPI *cuMemAlloc)(CUdeviceptr*, size_t);
        CUresult (RH_CUDAAPI *cuMemFree)(CUdeviceptr);
        CUresult (RH_CUDAAPI *cuMemcpyHtoD)(CUdeviceptr, const void*, size_t);
        CUresult (RH_CUDAAPI *cuMemcpyDtoH)(void*, CUdeviceptr, size_t);
        CUresult (RH_CUDAAPI *cuMemsetD8)(CUdeviceptr, unsigned char, size_t);
        CUresult (RH_CUDAAPI *cuLaunchKernel)(CUfunction, unsigned int, unsigned int, unsigned int,
                                   unsigned int, unsigned int, unsigned int,
                                   unsigned int, CUstream, void**, void**);
        CUresult (RH_CUDAAPI *cuGetErrorString)(CUresult, const char**);
        // asynchronous launches (overlapped kernels)
        CUresult (RH_CUDAAPI *cuMemAllocHost)(void**, size_t);
        CUresult (RH_CUDAAPI *cuMemFreeHost)(void*);
        CUresult (RH_CUDAAPI *cuMemsetD8Async)(CUdeviceptr, unsigned char, size_t, CUstream);
        CUresult (RH_CUDAAPI *cuMemcpyDtoHAsync)(void*, CUdeviceptr, size_t, CUstream);
        CUresult (RH_CUDAAPI *cuEventCreate)(CUevent*, unsigned int);
        CUresult (RH_CUDAAPI *cuEventRecord)(CUevent, CUstream);
        CUresult (RH_CUDAAPI *cuEventSynchronize)(CUevent);
        CUresult (RH_CUDAAPI *cuEventDestroy)(CUevent);
    };

    // Loads the driver library once; returns nullptr if unavailable.
    const Api* Get();

    // Human readable error text
    std::string ErrorString(CUresult r);

    struct DeviceInfo
    {
        int         ordinal = 0;
        std::string name;
        size_t      totalMem = 0;
        int         ccMajor = 0;
        int         ccMinor = 0;
        int         smCount = 0;
        int         maxThreadsPerSM = 0;
        int         pciBus = 0;
    };

    // All NVIDIA devices visible to the driver (empty if none / no driver)
    std::vector<DeviceInfo> EnumerateDevices();
}
