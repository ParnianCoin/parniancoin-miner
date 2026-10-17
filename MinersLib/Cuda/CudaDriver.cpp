/**
 * Minimal CUDA driver API loader
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */
#include "MinersLib/Cuda/CudaDriver.h"

#include <mutex>
#include <cstdio>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    typedef HMODULE LibHandle;
    static LibHandle OpenLib()                         { return LoadLibraryA("nvcuda.dll"); }
    static void* GetSym(LibHandle h, const char* name) { return (void*)GetProcAddress(h, name); }
#else
    #include <dlfcn.h>
    typedef void* LibHandle;
    static LibHandle OpenLib()
    {
        LibHandle h = dlopen("libcuda.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!h)
            h = dlopen("libcuda.so", RTLD_NOW | RTLD_LOCAL);
        return h;
    }
    static void* GetSym(LibHandle h, const char* name) { return dlsym(h, name); }
#endif

namespace cudrv
{
    static Api        s_api;
    static bool       s_loaded = false;
    static std::once_flag s_once;

    template <typename T>
    static bool Load(LibHandle h, T& fn, const char* name, const char* name_v2 = nullptr)
    {
        void* p = nullptr;
        if (name_v2)
            p = GetSym(h, name_v2);
        if (!p)
            p = GetSym(h, name);
        fn = reinterpret_cast<T>(p);
        return p != nullptr;
    }

    static void DoLoad()
    {
        LibHandle h = OpenLib();
        if (!h)
            return;

        bool ok = true;
        ok &= Load(h, s_api.cuInit,               "cuInit");
        ok &= Load(h, s_api.cuDriverGetVersion,   "cuDriverGetVersion");
        ok &= Load(h, s_api.cuDeviceGetCount,     "cuDeviceGetCount");
        ok &= Load(h, s_api.cuDeviceGet,          "cuDeviceGet");
        ok &= Load(h, s_api.cuDeviceGetName,      "cuDeviceGetName");
        ok &= Load(h, s_api.cuDeviceTotalMem,     "cuDeviceTotalMem", "cuDeviceTotalMem_v2");
        ok &= Load(h, s_api.cuDeviceGetAttribute, "cuDeviceGetAttribute");
        ok &= Load(h, s_api.cuCtxCreate,          "cuCtxCreate", "cuCtxCreate_v2");
        ok &= Load(h, s_api.cuCtxDestroy,         "cuCtxDestroy", "cuCtxDestroy_v2");
        ok &= Load(h, s_api.cuCtxSetCurrent,      "cuCtxSetCurrent");
        ok &= Load(h, s_api.cuCtxSetLimit,        "cuCtxSetLimit");
        ok &= Load(h, s_api.cuCtxSynchronize,     "cuCtxSynchronize");
        ok &= Load(h, s_api.cuMemGetInfo,         "cuMemGetInfo", "cuMemGetInfo_v2");
        ok &= Load(h, s_api.cuModuleLoadData,     "cuModuleLoadData");
        ok &= Load(h, s_api.cuModuleUnload,       "cuModuleUnload");
        ok &= Load(h, s_api.cuModuleGetFunction,  "cuModuleGetFunction");
        ok &= Load(h, s_api.cuFuncGetAttribute,   "cuFuncGetAttribute");
        ok &= Load(h, s_api.cuMemAlloc,           "cuMemAlloc", "cuMemAlloc_v2");
        ok &= Load(h, s_api.cuMemFree,            "cuMemFree", "cuMemFree_v2");
        ok &= Load(h, s_api.cuMemcpyHtoD,         "cuMemcpyHtoD", "cuMemcpyHtoD_v2");
        ok &= Load(h, s_api.cuMemcpyDtoH,         "cuMemcpyDtoH", "cuMemcpyDtoH_v2");
        ok &= Load(h, s_api.cuMemsetD8,           "cuMemsetD8", "cuMemsetD8_v2");
        ok &= Load(h, s_api.cuLaunchKernel,       "cuLaunchKernel");
        Load(h, s_api.cuGetErrorString, "cuGetErrorString");   // optional
        ok &= Load(h, s_api.cuMemAllocHost,       "cuMemAllocHost", "cuMemAllocHost_v2");
        ok &= Load(h, s_api.cuMemFreeHost,        "cuMemFreeHost");
        ok &= Load(h, s_api.cuMemsetD8Async,      "cuMemsetD8Async");
        ok &= Load(h, s_api.cuMemcpyDtoHAsync,    "cuMemcpyDtoHAsync", "cuMemcpyDtoHAsync_v2");
        ok &= Load(h, s_api.cuEventCreate,        "cuEventCreate");
        ok &= Load(h, s_api.cuEventRecord,        "cuEventRecord");
        ok &= Load(h, s_api.cuEventSynchronize,   "cuEventSynchronize");
        ok &= Load(h, s_api.cuEventDestroy,       "cuEventDestroy", "cuEventDestroy_v2");

        if (!ok)
            return;
        if (s_api.cuInit(0) != CUDA_SUCCESS)
            return;
        s_loaded = true;
    }

    const Api* Get()
    {
        std::call_once(s_once, DoLoad);
        return s_loaded ? &s_api : nullptr;
    }

    std::string ErrorString(CUresult r)
    {
        const Api* api = Get();
        const char* msg = nullptr;
        if (api && api->cuGetErrorString && api->cuGetErrorString(r, &msg) == CUDA_SUCCESS && msg)
            return msg;
        char buf[32];
        snprintf(buf, sizeof(buf), "CUDA error %d", r);
        return buf;
    }

    std::vector<DeviceInfo> EnumerateDevices()
    {
        std::vector<DeviceInfo> out;
        const Api* api = Get();
        if (!api)
            return out;
        int count = 0;
        if (api->cuDeviceGetCount(&count) != CUDA_SUCCESS)
            return out;
        for (int i = 0; i < count; i++)
        {
            CUdevice dev;
            if (api->cuDeviceGet(&dev, i) != CUDA_SUCCESS)
                continue;
            DeviceInfo inf;
            inf.ordinal = i;
            char name[256] = { 0 };
            api->cuDeviceGetName(name, sizeof(name) - 1, dev);
            inf.name = name;
            api->cuDeviceTotalMem(&inf.totalMem, dev);
            api->cuDeviceGetAttribute(&inf.ccMajor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
            api->cuDeviceGetAttribute(&inf.ccMinor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
            api->cuDeviceGetAttribute(&inf.smCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
            api->cuDeviceGetAttribute(&inf.maxThreadsPerSM, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR, dev);
            api->cuDeviceGetAttribute(&inf.pciBus, CU_DEVICE_ATTRIBUTE_PCI_BUS_ID, dev);
            out.push_back(inf);
        }
        return out;
    }
}
