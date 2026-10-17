/**
 * RandomHash2 NVIDIA GPU miner (CUDA driver API, embedded kernel)
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */
#include "precomp.h"

#ifdef PARNIANMINER_CUDA

#include "MinersLib/Parnian/RandomHashCudaMiner.h"
#include "MinersLib/Parnian/RandomHash.h"
#include "MinersLib/Global.h"
#include "corelib/FixedHash.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef RH_COMPILE_CPU_ONLY
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_BOOL(g_kernelactivewaiting, false);
#endif
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuBlockSize, 64)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuStackKB, 16)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuRegs, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuScan, 2)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuOverlap, 1)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuTestSeconds, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuBenchSeconds, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_BOOL(g_gpuDiag, false);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuDiagFrom, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuDiagCount, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuDiagStack, 0)
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_gpuBenchRun, 0)

// host copy of the kernel code (MinersLib/Cuda/Rh2HostDiag.cpp)
extern uint32_t Rh2HostDiag(uint32_t mode, uint32_t param, const uint8_t* header236, uint32_t gid);

// the embedded kernels (cudalib/rh2_fatbin.cpp)
extern "C" const unsigned char rh2_fatbin_data[];
extern "C" const unsigned char rh2_fatbin_end[];
extern "C" const unsigned char rh2_fatbin_r128_data[];
extern "C" const unsigned char rh2_fatbin_r128_end[];
extern "C" const unsigned char rh2_fatbin_r64_data[];
extern "C" const unsigned char rh2_fatbin_r64_end[];

using namespace cudrv;

//------------------------------------------------------------------------------
// SHA2-256 midstate of the first 192 header bytes (mirrors the kernel)
static const U32 c_k256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline U32 ror32(U32 x, U32 n) { return (x >> n) | (x << (32 - n)); }

static void Sha256Block(U32 st[8], const U8* block)
{
    U32 w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((U32)block[i * 4] << 24) | ((U32)block[i * 4 + 1] << 16) | ((U32)block[i * 4 + 2] << 8) | (U32)block[i * 4 + 3];
    for (int i = 16; i < 64; i++)
        w[i] = w[i - 16] + (ror32(w[i - 15], 7) ^ ror32(w[i - 15], 18) ^ (w[i - 15] >> 3)) + w[i - 7] +
               (ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^ (w[i - 2] >> 10));
    U32 a = st[0], b = st[1], c = st[2], d = st[3], e = st[4], f = st[5], g = st[6], h = st[7];
    for (int i = 0; i < 64; i++)
    {
        U32 t1 = h + (ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25)) + ((e & f) ^ (~e & g)) + c_k256[i] + w[i];
        U32 t2 = (ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    st[0] += a; st[1] += b; st[2] += c; st[3] += d; st[4] += e; st[5] += f; st[6] += g; st[7] += h;
}

// RH2_Header blob: SHA2-256 midstate[8] + header tail[44]
static void BuildHeaderBlob(const U8* header236, U8* blob)
{
    U32 st[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    for (int b = 0; b < 3; b++)
        Sha256Block(st, header236 + 64 * b);
    memcpy(blob, st, sizeof(st));
    memcpy(blob + sizeof(st), header236 + 192, 236 - 192);
}

// same final comparison as the CPU miner (RandomHashCPUMiner)
static bool HashMeetsTarget(const U8* hash, const U32 targetFull[8])
{
    U32 work[8], swapped[8];
    memcpy(work, hash, 32);
    for (int i = 0; i < 8; i++)
        swapped[i] = RH_swap_u32(work[7 - i]);
    return IsHashLessThan_32(swapped, targetFull);
}

//==============================================================================
// Rh2Options
Rh2Options Rh2Options::FromCommandLine(U32 threads)
{
    Rh2Options o;
    o.threads = threads;
    o.blockSize = (U32)g_gpuBlockSize;
    o.stackKB = (U32)g_gpuStackKB;
    o.spinWait = g_kernelactivewaiting;
    o.regs = (U32)g_gpuRegs;
    o.scan = (U32)g_gpuScan == 1 ? 2 : (U32)g_gpuScan;
    o.overlap = g_gpuOverlap != 0;
    return o;
}

std::string Rh2Options::ToString() const
{
    return FormatString("-gpuregs %u -gpuscan %u -gpublock %u%s", regs, scan, blockSize, overlap ? "" : " -gpuoverlap 0");
}

U32 Rh2Options::ArenaBytes() const
{
    // measured maxima: full 121 KB, round 3: 23 KB, round 2: 3.3 KB
    if (scan == 2)
        return 8 * 1024;
    if (scan == 3)
        return 40 * 1024;
    return 136 * 1024;
}

//==============================================================================
// Rh2CudaDevice
bool Rh2CudaDevice::Ok(CUresult r, const char* what, std::string& err)
{
    if (r == CUDA_SUCCESS)
        return true;
    err = FormatString("%s: %s", what, ErrorString(r).c_str());
    return false;
}

double Rh2CudaDevice::MemoryGB() const
{
    return (double)m_threads * (RH2_THREAD_BYTES + m_opts.ArenaBytes()) / (1024.0 * 1024.0 * 1024.0);
}

bool Rh2CudaDevice::Open(int ordinal, const Rh2Options& opts, std::string& err)
{
    Close();
    m_opts = opts;
    m_api = cudrv::Get();
    if (!m_api)
    {
        err = "NVIDIA driver (CUDA) not available";
        return false;
    }

    const unsigned char* fatbin = rh2_fatbin_data;
    const unsigned char* fatbinEnd = rh2_fatbin_end;
    if (opts.regs == 128)
    {
        fatbin = rh2_fatbin_r128_data;
        fatbinEnd = rh2_fatbin_r128_end;
    }
    else if (opts.regs == 64)
    {
        fatbin = rh2_fatbin_r64_data;
        fatbinEnd = rh2_fatbin_r64_end;
    }
    else if (opts.regs != 0)
    {
        err = FormatString("-gpuregs %u is not available (use 0, 128 or 64)", opts.regs);
        return false;
    }
    if (fatbinEnd == fatbin)
    {
        err = FormatString("this build has no CUDA kernel for -gpuregs %u", opts.regs);
        return false;
    }

    CUdevice dev;
    if (!Ok(m_api->cuDeviceGet(&dev, ordinal), "cuDeviceGet", err))
        return false;
    if (!Ok(m_api->cuCtxCreate(&m_ctx, opts.spinWait ? CU_CTX_SCHED_SPIN : CU_CTX_SCHED_BLOCKING_SYNC, dev), "cuCtxCreate", err))
        return false;
    // the RandomHash2 recursion needs more stack than the 1KB default; the
    // 64 register kernel spills more registers to the stack (it crashed on a
    // GTX 850M with 16 KB in full mode)
    U32 stackKB = opts.stackKB;
    if (opts.regs == 64 && stackKB < 32)
        stackKB = 32;
    if (!Ok(m_api->cuCtxSetLimit(CU_LIMIT_STACK_SIZE, (size_t)stackKB * 1024), "cuCtxSetLimit", err))
        return false;

    CUresult r = m_api->cuModuleLoadData(&m_module, fatbin);
    if (r != CUDA_SUCCESS)
    {
        int major = 0, minor = 0;
        m_api->cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
        m_api->cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
        err = FormatString("cannot load the CUDA kernel for compute capability %d.%d (%s). "
                           "Update the NVIDIA driver or use a build that supports this GPU",
                           major, minor, ErrorString(r).c_str());
        return false;
    }
    if (!Ok(m_api->cuModuleGetFunction(&m_kernel, m_module, RH2_KERNEL_NAME), "cuModuleGetFunction", err))
        return false;
    m_diagKernel = nullptr;
    m_api->cuModuleGetFunction(&m_diagKernel, m_module, "rh2_diag_kernel");   // main kernel only

    int smCount = 0, maxThreadsPerSM = 0, maxThreadsPerBlockFn = 0;
    m_api->cuDeviceGetAttribute(&smCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
    m_api->cuDeviceGetAttribute(&maxThreadsPerSM, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR, dev);
    m_api->cuFuncGetAttribute(&maxThreadsPerBlockFn, CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, m_kernel);
    m_api->cuFuncGetAttribute(&m_regs, CU_FUNC_ATTRIBUTE_NUM_REGS, m_kernel);

    m_blockSize = opts.blockSize;
    if (maxThreadsPerBlockFn > 0 && m_blockSize > (U32)maxThreadsPerBlockFn)
        m_blockSize = (U32)maxThreadsPerBlockFn;
    m_blockSize = std::max<U32>(32, m_blockSize & ~31u);

    // resident threads per SM are limited by the register file (64K registers)
    U32 residentPerSM = m_regs > 0 ? ((U32)(65536 / m_regs) & ~31u) : 256;
    if (maxThreadsPerSM > 0 && residentPerSM > (U32)maxThreadsPerSM)
        residentPerSM = (U32)maxThreadsPerSM;
    if (!residentPerSM)
        residentPerSM = 32;

    size_t freeMem = 0, totalMem = 0;
    m_api->cuMemGetInfo(&freeMem, &totalMem);
    const U32 arenaBytes = opts.ArenaBytes();
    const U64 perThread = RH2_THREAD_BYTES + arenaBytes;
    U64 memLimit = (U64)(freeMem * 8 / 10) / perThread;

    U32 threads = opts.threads;
    if (!threads)
        threads = (U32)smCount * residentPerSM * 2;     // two waves keep the SMs busy
    if (threads > memLimit)
        threads = (U32)memLimit;
    threads = std::max<U32>(m_blockSize, (threads / m_blockSize) * m_blockSize);
    m_threads = threads;
    m_active = std::max<U32>(m_blockSize, ((threads / 2) / m_blockSize) * m_blockSize);   // start with one wave
    m_arenaWords = arenaBytes / 4;

    if (!Ok(m_api->cuMemAlloc(&m_dHeader, RH2_HEADER_BYTES), "cuMemAlloc(header)", err) ||
        !Ok(m_api->cuMemAlloc(&m_dThreads, (size_t)threads * RH2_THREAD_BYTES), "cuMemAlloc(threads)", err) ||
        !Ok(m_api->cuMemAlloc(&m_dArenas, (size_t)threads * arenaBytes), "cuMemAlloc(arenas)", err))
        return false;
    for (int i = 0; i < 2; i++)
    {
        if (!Ok(m_api->cuMemAlloc(&m_dResults[i], sizeof(RH2_KernelResults)), "cuMemAlloc(results)", err) ||
            !Ok(m_api->cuMemAllocHost((void**)&m_hResults[i], sizeof(RH2_KernelResults)), "cuMemAllocHost", err) ||
            !Ok(m_api->cuEventCreate(&m_events[i], CU_EVENT_BLOCKING_SYNC | CU_EVENT_DISABLE_TIMING), "cuEventCreate", err))
            return false;
    }
    m_launched = m_completed = 0;
    return true;
}

void Rh2CudaDevice::Close()
{
    if (!m_api || !m_ctx)
        return;
    m_api->cuCtxSetCurrent(m_ctx);
    m_api->cuCtxSynchronize();
    if (m_dHeader)  m_api->cuMemFree(m_dHeader);
    if (m_dThreads) m_api->cuMemFree(m_dThreads);
    if (m_dArenas)  m_api->cuMemFree(m_dArenas);
    for (int i = 0; i < 2; i++)
    {
        if (m_dResults[i]) m_api->cuMemFree(m_dResults[i]);
        if (m_hResults[i]) m_api->cuMemFreeHost(m_hResults[i]);
        if (m_events[i])   m_api->cuEventDestroy(m_events[i]);
        m_dResults[i] = 0;
        m_hResults[i] = nullptr;
        m_events[i] = nullptr;
    }
    if (m_module)   m_api->cuModuleUnload(m_module);
    m_api->cuCtxDestroy(m_ctx);
    m_dHeader = m_dThreads = m_dArenas = 0;
    m_module = nullptr;
    m_kernel = nullptr;
    m_diagKernel = nullptr;
    m_ctx = nullptr;
    m_launched = m_completed = 0;
}

bool Rh2CudaDevice::RunDiag(U32 mode, U32 param, U32 threads, std::vector<U32>& out, std::string& err)
{
    if (!m_diagKernel)
    {
        err = "no diagnostic kernel in this build";
        return false;
    }
    threads = std::min(threads, m_threads);
    U32 block = std::min<U32>(threads, 32);
    threads = (threads / block) * block;
    m_api->cuCtxSetCurrent(m_ctx);
    CUdeviceptr dOut = 0;
    if (!Ok(m_api->cuMemAlloc(&dOut, (size_t)threads * 4), "cuMemAlloc(diag)", err))
        return false;
    m_api->cuMemsetD8(dOut, 0, (size_t)threads * 4);
    U32 arenaWords = m_arenaWords;
    void* args[] = { &mode, &param, &m_dHeader, &m_dThreads, &m_dArenas, &arenaWords, &dOut };
    bool ok = Ok(m_api->cuLaunchKernel(m_diagKernel, threads / block, 1, 1, block, 1, 1, 0, nullptr, args, nullptr), "cuLaunchKernel", err) &&
              Ok(m_api->cuCtxSynchronize(), "kernel execution", err);
    if (ok)
    {
        out.resize(threads);
        ok = Ok(m_api->cuMemcpyDtoH(out.data(), dOut, (size_t)threads * 4), "read diag", err);
    }
    m_api->cuMemFree(dOut);
    return ok;
}

void Rh2CudaDevice::AdaptLaunch(U64 launchMs)
{
    if (launchMs > 900 && m_active > m_blockSize)
        m_active = std::max<U32>(m_blockSize, ((m_active / 2) / m_blockSize) * m_blockSize);
    else if (launchMs < 300 && m_active < m_threads)
        m_active = std::min<U32>(m_threads, m_active * 2);
}

bool Rh2CudaDevice::SetHeader(const U8* header236, std::string& err)
{
    U8 blob[RH2_HEADER_BYTES];
    BuildHeaderBlob(header236, blob);
    m_api->cuCtxSetCurrent(m_ctx);
    // synchronous: ordered after the kernels already queued (they keep the old header)
    return Ok(m_api->cuMemcpyHtoD(m_dHeader, blob, sizeof(blob)), "upload header", err);
}

bool Rh2CudaDevice::Launch(U32 startNonce, U32 target, std::string& err)
{
    if (Pending() >= 2)
    {
        err = "too many launches in flight";
        return false;
    }
    m_api->cuCtxSetCurrent(m_ctx);
    int slot = (int)(m_launched % 2);
    // everything on the default stream: memset -> kernel -> copy back -> event
    if (!Ok(m_api->cuMemsetD8Async(m_dResults[slot], 0, sizeof(RH2_KernelResults), nullptr), "reset results", err))
        return false;
    U32 arenaWords = m_arenaWords;
    U32 flags = m_opts.scan & RH2_FLAG_SCAN_MASK;
    void* args[] = { &m_dHeader, &startNonce, &target, &m_dThreads, &m_dArenas, &arenaWords, &m_dResults[slot], &flags };
    if (!Ok(m_api->cuLaunchKernel(m_kernel, m_active / m_blockSize, 1, 1, m_blockSize, 1, 1,
                                  0, nullptr, args, nullptr), "cuLaunchKernel", err))
        return false;
    if (!Ok(m_api->cuMemcpyDtoHAsync(m_hResults[slot], m_dResults[slot], sizeof(RH2_KernelResults), nullptr), "read results", err) ||
        !Ok(m_api->cuEventRecord(m_events[slot], nullptr), "cuEventRecord", err))
        return false;
    m_launched++;
    return true;
}

bool Rh2CudaDevice::Wait(RH2_KernelResults& out, std::string& err)
{
    if (!Pending())
    {
        err = "no launch in flight";
        return false;
    }
    int slot = (int)(m_completed % 2);
    m_api->cuCtxSetCurrent(m_ctx);
    if (!Ok(m_api->cuEventSynchronize(m_events[slot]), "kernel execution", err))
        return false;
    memcpy(&out, m_hResults[slot], sizeof(RH2_KernelResults));
    m_completed++;
    return true;
}

bool Rh2CudaDevice::Run(U32 startNonce, U32 target, RH2_KernelResults& out, std::string& err)
{
    while (Pending())
        if (!Wait(out, err))
            return false;
    return Launch(startNonce, target, err) && Wait(out, err);
}

//==============================================================================
// RandomHashCudaMiner
RandomHashCudaMiner::RandomHashCudaMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex) :
    GenericCLMiner(_farm, globalWorkMult, localWorkSize, gpuIndex)
{
    m_deviceOrdinal = (int)GpuManager::Gpus[gpuIndex].deviceID;
    memset(&m_hostResults, 0, sizeof(m_hostResults));
    memset(m_targetFull, 0, sizeof(m_targetFull));
}

RandomHashCudaMiner::~RandomHashCudaMiner()
{
}

bool RandomHashCudaMiner::configureGPU()
{
    return GpuManager::SetupGPU();
}

void RandomHashCudaMiner::UpdateWorkSize(U32 absoluteVal)
{
    // the real size is decided in init() from the device memory and SM count
    if (absoluteVal)
        m_globalWorkSize = absoluteVal;
    else if (m_gpuInfoCache->setGpuThreadCount != U32_Max && m_gpuInfoCache->setGpuThreadCount)
        m_globalWorkSize = m_gpuInfoCache->setGpuThreadCount;
    else
        m_globalWorkSize = 0;
    m_gpuInfoCache->globalWorkSize = m_globalWorkSize;
}

bool RandomHashCudaMiner::init(const ParnianWorkSptr& work)
{
    std::string err;
    Rh2Options opts = Rh2Options::FromCommandLine(m_globalWorkSize);
    if (!m_dev.Open(m_deviceOrdinal, opts, err))
    {
        PrintOutCritical("GPU%d error: %s\n", m_globalIndex, err.c_str());
        m_failed = true;
        return false;
    }
    m_globalWorkSize = m_dev.MaxThreads();
    m_gpuInfoCache->globalWorkSize = m_dev.MaxThreads();
    m_gpuInfoCache->localWorkSize = m_dev.BlockSize();
    PrintOut("GPU%d: up to %u threads (%u per block), %d registers, %.1f GB, %s\n",
             m_globalIndex, m_dev.MaxThreads(), m_dev.BlockSize(), m_dev.Registers(), m_dev.MemoryGB(), opts.ToString().c_str());

    if (m_hashCountTime == U64_Max || m_hashCountTime == 0)
        m_hashCountTime = TimeGetMilliSec();
    m_isInitialized = true;
    return true;
}

PrepareWorkStatus RandomHashCudaMiner::PrepareWork(const ParnianWorkSptr& workTempl, bool reuseCurrentWP)
{
    PrepareWorkStatus status = GenericCLMiner::PrepareWork(workTempl, reuseCurrentWP);
    if (status == PrepareWork_NewWork && m_currentWp.get() && !m_failed)
    {
        if (m_currentWp->m_fullHeader.size() != RH2_HEADER_SIZE_BYTES)
        {
            PrintOutCritical("GPU%d: unexpected header size %u\n", m_globalIndex, (U32)m_currentWp->m_fullHeader.size());
            m_launchWp.reset();
            return status;
        }
        std::string err;
        if (!m_dev.SetHeader(&m_currentWp->m_fullHeader[0], err))
        {
            PrintOutCritical("GPU%d error: %s\n", m_globalIndex, err.c_str());
            m_failed = true;
            return status;
        }
        m_target = m_currentWp->GetDeviceTargetUpperBits();
        if (m_currentWp->m_isSolo)
            memcpy(m_targetFull, m_currentWp->m_soloTargetPow.data(), 32);
        else
            memcpy(m_targetFull, m_currentWp->m_deviceBoundary.data(), 32);
        m_launchWp = m_currentWp;
        m_nonceBase = rand32();
    }
    return status;
}

void RandomHashCudaMiner::QueueKernel()
{
    if (m_failed || !m_launchWp.get())
    {
        CpuSleep(100);
        return;
    }

    std::string err;

    // queue the next launch first, so the GPU works while we process results
    Inflight inf;
    inf.wp = m_launchWp;
    memcpy(inf.targetFull, m_targetFull, sizeof(inf.targetFull));
    U32 startNonce = m_nonceBase;
    m_nonceBase += m_dev.Threads();
    if (!m_dev.Launch(startNonce, m_target, err))
    {
        PrintOutCritical("GPU%d error: %s\n", m_globalIndex, err.c_str());
        m_failed = true;
        return;
    }
    m_inflight.push_back(inf);

    U32 maxPending = m_dev.Overlap() ? 2 : 1;
    while (m_dev.Pending() >= maxPending)
    {
        if (!m_dev.Wait(m_hostResults, err))
        {
            PrintOutCritical("GPU%d error: %s\n", m_globalIndex, err.c_str());
            m_failed = true;
            return;
        }
        Inflight done = m_inflight.front();
        m_inflight.pop_front();

        U64 now = TimeGetMilliSec();
        if (m_lastDoneMs)
            m_dev.AdaptLaunch(now - m_lastDoneMs);
        m_lastDoneMs = now;

        m_kernelItterations++;
        Miner::AddHashCount(m_hostResults.hashes64);
        m_overflows += m_hostResults.overflows;

        U32 found = std::min<U32>(m_hostResults.count, RH2_MAX_KERNEL_RESULTS);
        if (!found)
            continue;

        // every solution is re-verified on the CPU before it is sent (ParnianWorkPackage::Eval)
        ParnianWorkSptr saved = m_currentWp;
        m_currentWp = done.wp;
        for (U32 i = 0; i < found; i++)
        {
            if (!HashMeetsTarget(m_hostResults.hashes[i], done.targetFull))
                continue;
            std::vector<U64> nonce(1, m_hostResults.nonces[i]);
            m_farm.submitProof(MakeSubmitSolution(nonce, done.wp->m_nonce2, false));
            if (done.wp->m_isSolo)
                break;
        }
        m_currentWp = saved;
    }
}

//==============================================================================
// helpers for -gputest / -gpubench / -gpudiag
static bool CpuHash(const U8* header236, U32 nonce, U8 out[32])
{
    RandomHash_State st;
    RandomHashResult res;
    RandomHash_Create(&st);
    RandomHash_SetHeader(&st, (U8*)header236, 236, 0);
    RandomHash_Search(&st, res, nonce);
    memcpy(out, res.hashes[0], 32);
    RandomHash_Destroy(&st);
    return res.nonces[0] == nonce;
}

// one launch reporting every hash; returns false on a device error,
// bad = hashes that differ from the CPU
static bool CheckKernel(Rh2CudaDevice& dev, U32 maxCheck, U32& checked, U32& bad, std::string& err)
{
    U8 header[236];
    for (U32 i = 0; i < sizeof(header); i++)
        header[i] = (U8)rand32();
    RH2_KernelResults res;
    if (!dev.SetHeader(header, err) || !dev.Run(rand32(), 0xFFFFFFFF, res, err))
        return false;
    U32 n = std::min<U32>(std::min<U32>(res.count, RH2_MAX_KERNEL_RESULTS), maxCheck);
    for (U32 k = 0; k < n; k++)
    {
        U8 cpu[32];
        CpuHash(header, res.nonces[k], cpu);
        checked++;
        if (memcmp(cpu, res.hashes[k], 32) != 0)
            bad++;
    }
    return true;
}

// measures the hash rate for `seconds` with the device options
// (overlapped launches like the miner); returns false on a device error
static bool Benchmark(Rh2CudaDevice& dev, U32 seconds, double& hs, double& msPerLaunch, U64& overflows,
                      std::string& err, bool print)
{
    RH2_KernelResults res;
    U32 nonce = rand32();
    U32 maxPending = dev.Overlap() ? 2 : 1;
    // warm up: let the launch size settle before measuring
    U64 last = TimeGetMilliSec();
    for (int w = 0; w < 8; w++)
    {
        if (!dev.Launch(nonce, 0, err))
            return false;
        nonce += dev.Threads();
        while (dev.Pending() >= maxPending)
        {
            if (!dev.Wait(res, err))
                return false;
            U64 now = TimeGetMilliSec();
            dev.AdaptLaunch(now - last);
            last = now;
        }
    }
    while (dev.Pending())
        if (!dev.Wait(res, err))
            return false;

    U64 hashes = 0, launches = 0;
    overflows = 0;
    U64 t0 = TimeGetMilliSec(), lastPrint = t0;
    last = t0;
    while (TimeGetMilliSec() - t0 < (U64)seconds * 1000)
    {
        if (!dev.Launch(nonce, 0, err))
            return false;
        nonce += dev.Threads();
        while (dev.Pending() >= maxPending)
        {
            if (!dev.Wait(res, err))
                return false;
            U64 now = TimeGetMilliSec();
            dev.AdaptLaunch(now - last);
            last = now;
            hashes += res.hashes64;
            overflows += res.overflows;
            launches++;
        }
        U64 now = TimeGetMilliSec();
        if (print && now - lastPrint >= 10000)
        {
            PrintOutCritical("  %.0f H/s\n", hashes * 1000.0 / (double)(now - t0));
            lastPrint = now;
        }
    }
    while (dev.Pending())
    {
        if (!dev.Wait(res, err))
            return false;
        hashes += res.hashes64;
        overflows += res.overflows;
        launches++;
    }
    U64 dt = std::max<U64>(1, TimeGetMilliSec() - t0);
    hs = hashes * 1000.0 / (double)dt;
    msPerLaunch = launches ? (double)dt / launches : 0.0;
    return true;
}

static std::vector<U32> SelectedNvidiaGpus()
{
    std::vector<U32> gpus;
    for (auto& g : GpuManager::Gpus)
        if (PARNIANMINER_TEST_BIT(g.gpuType, GpuType_NVIDIA) && g.enabled)
            gpus.push_back(g.globalIndex);
    if (gpus.empty())
        for (auto& g : GpuManager::Gpus)
            if (PARNIANMINER_TEST_BIT(g.gpuType, GpuType_NVIDIA))
                gpus.push_back(g.globalIndex);
    return gpus;
}

static int DriverCudaVersion()
{
    const cudrv::Api* api = cudrv::Get();
    int ver = 0;
    if (api)
        api->cuDriverGetVersion(&ver);
    return ver;
}

static std::string SelfExePath()
{
#ifdef _WIN32
    char buf[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return buf;
#else
    char buf[4096] = { 0 };
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    return n > 0 ? std::string(buf, (size_t)n) : std::string("./parnianminer");
#endif
}

// runs this executable with `args` and returns its output lines starting with `prefix`
// (child processes isolate GPU faults: after a fault the GPU is unusable for the process)
static std::vector<std::string> RunSelf(const std::string& args, const char* prefix)
{
    std::string cmd = "\"" + SelfExePath() + "\" " + args;
#ifdef _WIN32
    cmd = "\"" + cmd + " 2>&1\"";
    FILE* f = _popen(cmd.c_str(), "r");
#else
    cmd += " 2>&1";
    FILE* f = popen(cmd.c_str(), "r");
#endif
    std::vector<std::string> out;
    if (!f)
        return out;
    char line[1024];
    size_t plen = strlen(prefix);
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, prefix, plen) != 0)
            continue;
        std::string l(line + plen);
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r'))
            l.pop_back();
        out.push_back(l);
    }
#ifdef _WIN32
    _pclose(f);
#else
    pclose(f);
#endif
    return out;
}

//==============================================================================
// -gputest
void RandomHashCudaMiner::RunSelfTests(U32 seconds)
{
    std::vector<U32> gpus = SelectedNvidiaGpus();
    if (gpus.empty())
    {
        PrintOutCritical("No NVIDIA GPU found (is the NVIDIA driver installed?)\n");
        return;
    }

    for (U32 gi : gpus)
    {
        const GpuManager::GPUInfos& g = GpuManager::Gpus[gi];
        int ver = DriverCudaVersion();
        PrintOutCritical("\nGPU%u %s, NVIDIA driver supports CUDA %d.%d\n", gi, g.description.c_str(), ver / 1000, (ver % 1000) / 10);

        Rh2CudaDevice dev;
        std::string err;
        Rh2Options opts = Rh2Options::FromCommandLine((g.setGpuThreadCount != U32_Max) ? g.setGpuThreadCount : 0);
        if (!dev.Open((int)g.deviceID, opts, err))
        {
            PrintOutCritical("  error: %s\n", err.c_str());
            continue;
        }
        PrintOutCritical("  up to %u threads, %u per block, %d registers, %.1f GB, %s\n",
                         dev.MaxThreads(), dev.BlockSize(), dev.Registers(), dev.MemoryGB(), opts.ToString().c_str());

        // 1) correctness: every hash the GPU reports must equal the CPU result
        U32 checked = 0, bad = 0;
        bool crashed = false;
        for (int round = 0; round < 3 && !bad; round++)
        {
            if (!CheckKernel(dev, 64, checked, bad, err))
            {
                PrintOutCritical("  kernel check FAILED, the GPU stopped with an error: %s\n", err.c_str());
                PrintOutCritical("  Run 'parnianminer -gpudiag' and send its output to the ParnianCoin developers.\n");
                crashed = true;
                break;
            }
        }
        if (crashed)
            continue;
        if (bad)
        {
            PrintOutCritical("  kernel check FAILED (%u of %u hashes wrong). Do not mine with this GPU.\n", bad, checked);
            PrintOutCritical("  Run 'parnianminer -gpudiag' and send its output to the ParnianCoin developers.\n");
            continue;
        }
        PrintOutCritical("  kernel check OK: %u GPU hashes identical to the CPU algorithm\n", checked);

        // 2) benchmark
        if (!seconds)
            continue;
        double hs = 0, ms = 0;
        U64 overflows = 0;
        if (!Benchmark(dev, seconds, hs, ms, overflows, err, true))
        {
            PrintOutCritical("  error: %s\n", err.c_str());
            continue;
        }
        PrintOutCritical("  launch size: %u threads\n", dev.Threads());
        PrintOutCritical("  RESULT: %.0f H/s  (%.0f ms per launch, %llu dropped nonces)\n",
                         hs, ms, (unsigned long long)overflows);
    }
}

//==============================================================================
// -gpubench: every combination of the kernel options, each in its own process
// (a crash of one variant must not stop the others)
static std::vector<Rh2Options> BenchItems()
{
    std::vector<Rh2Options> v;
    const U32 regs[] = { 0, 128, 64 };
    const U32 scans[] = { 0, 3, 2 };
    const U32 blocks[] = { 32, 64, 128 };
    for (U32 r : regs)
        for (U32 sc : scans)
            for (U32 b : blocks)
            {
                Rh2Options o;
                o.regs = r;
                o.scan = sc;
                o.blockSize = b;
                o.overlap = true;
                v.push_back(o);
            }
    // the first GPU version: no overlapped launches
    Rh2Options base;
    base.scan = 0;
    base.overlap = false;
    v.push_back(base);
    return v;
}

static void RunBenchChild(U32 index, U32 seconds)
{
    std::vector<Rh2Options> items = BenchItems();
#ifdef RH2_DIAG_SELFTEST
    if (index == 5) { printf("BENCH %u CRASH fake fault\n", index); return; }
    if (index == 7) { printf("BENCH %u WRONG 3 of 24 hashes differ from the CPU\n", index); return; }
    printf("BENCH %u OK %.0f %u %d %.0f %llu\n", index, 10000.0 + index * 137.0, 1280u + index, 255, 480.0, 0ull);
    return;
#endif
    std::vector<U32> gpus = SelectedNvidiaGpus();
    if (gpus.empty() || index >= items.size())
    {
        printf("BENCH %u UNAVAILABLE no NVIDIA GPU\n", index);
        return;
    }
    const GpuManager::GPUInfos& g = GpuManager::Gpus[gpus[0]];
    Rh2Options o = items[index];
    o.stackKB = (U32)g_gpuStackKB;
    o.spinWait = g_kernelactivewaiting;
    Rh2CudaDevice dev;
    std::string err;
    if (!dev.Open((int)g.deviceID, o, err))
    {
        printf("BENCH %u UNAVAILABLE %s\n", index, err.c_str());
        return;
    }
    U32 checked = 0, bad = 0;
    if (!CheckKernel(dev, 24, checked, bad, err))
    {
        printf("BENCH %u CRASH %s\n", index, err.c_str());
        return;
    }
    if (bad)
    {
        printf("BENCH %u WRONG %u of %u hashes differ from the CPU\n", index, bad, checked);
        return;
    }
    double hs = 0, ms = 0;
    U64 overflows = 0;
    if (!Benchmark(dev, seconds, hs, ms, overflows, err, false))
    {
        printf("BENCH %u CRASH %s\n", index, err.c_str());
        return;
    }
    printf("BENCH %u OK %.0f %u %d %.0f %llu\n", index, hs, dev.Threads(), dev.Registers(), ms, (unsigned long long)overflows);
    fflush(stdout);
}

void RandomHashCudaMiner::RunBench(U32 seconds)
{
    if (g_gpuBenchRun)
    {
        RunBenchChild((U32)g_gpuBenchRun - 1, seconds);
        return;
    }
    std::vector<U32> gpus = SelectedNvidiaGpus();
#ifdef RH2_DIAG_SELFTEST
    if (gpus.empty())
        gpus.push_back(0);
#endif
    if (gpus.empty())
    {
        PrintOutCritical("No NVIDIA GPU found (is the NVIDIA driver installed?)\n");
        return;
    }
    const GpuManager::GPUInfos& g = GpuManager::Gpus[gpus[0]];
    int ver = DriverCudaVersion();
    std::vector<Rh2Options> items = BenchItems();
    PrintOutCritical("GPU%u %s, NVIDIA driver supports CUDA %d.%d\n", g.globalIndex, g.description.c_str(), ver / 1000, (ver % 1000) / 10);
    PrintOutCritical("Testing %u kernel configurations, %u seconds each (about %u minutes)\n",
                     (U32)items.size(), seconds, (U32)((items.size() * (seconds + 4) + 59) / 60));
    PrintOutCritical("  regs scan block overlap |     H/s | threads | ms/launch\n");

    double bestHs = 0, baseHs = 0;
    int best = -1;
    for (U32 i = 0; i < items.size(); i++)
    {
        const Rh2Options& o = items[i];
        std::vector<std::string> lines = RunSelf(FormatString("-gpubench %u -gpubenchrun %u -gpu %u -gpustack %u",
                                                              seconds, i + 1, g.globalIndex, (U32)g_gpuStackKB), "BENCH ");
        std::string r = lines.empty() ? std::string() : lines.back();
        std::string cfg = FormatString("  %4s %4u %5u %7s |", o.regs ? FormatString("%u", o.regs) : "auto",
                                       o.scan, o.blockSize, o.overlap ? "yes" : "no");
        double hs = 0, ms = 0;
        unsigned threads = 0, idx = 0;
        int regs = 0;
        unsigned long long ovf = 0;
        if (r.empty())
            PrintOutCritical("%s no answer\n", cfg.c_str());
        else if (sscanf(r.c_str(), "%u OK %lf %u %d %lf %llu", &idx, &hs, &threads, &regs, &ms, &ovf) == 6)
        {
            PrintOutCritical("%s %7.0f | %7u | %6.0f    (%d registers%s)\n", cfg.c_str(), hs, threads, ms, regs,
                             ovf ? FormatString(", %llu dropped", ovf) : "");
            if (hs > bestHs)
            {
                bestHs = hs;
                best = (int)i;
            }
            if (!o.overlap)
                baseHs = hs;
        }
        else
        {
            size_t p = r.find(' ');
            PrintOutCritical("%s %s\n", cfg.c_str(), p == std::string::npos ? r.c_str() : r.c_str() + p + 1);
        }
    }
    if (best >= 0)
    {
        PrintOutCritical("\nBest: %.0f H/s with  %s\n", bestHs, items[best].ToString().c_str());
        if (baseHs > 0)
            PrintOutCritical("First GPU version (no options): %.0f H/s -> %+.0f%%\n", baseHs, (bestHs / baseHs - 1.0) * 100.0);
        PrintOutCritical("Add these options to your mining command line.\n");
    }
    PrintOutCritical("\nPlease send this whole output to the ParnianCoin developers.\n");
}

//==============================================================================
// -gpudiag
#define RH2_MAX_ALGO_COUNT 77
static const char* c_algoNames[RH2_MAX_ALGO_COUNT] = {
    "Blake2B_160", "Blake2B_256", "Blake2B_512", "Blake2B_384", "Blake2S_128", "Blake2S_160", "Blake2S_224", "Blake2S_256",
    "Gost", "GOST3411_2012_256", "GOST3411_2012_512", "Grindahl256", "Grindahl512", "HAS160",
    "Haval_3_128", "Haval_3_160", "Haval_3_192", "Haval_3_224", "Haval_3_256",
    "Haval_4_128", "Haval_4_160", "Haval_4_192", "Haval_4_224", "Haval_4_256",
    "Haval_5_128", "Haval_5_160", "Haval_5_192", "Haval_5_224", "Haval_5_256",
    "Keccak_224", "Keccak_256", "Keccak_288", "Keccak_384", "Keccak_512",
    "MD2", "MD5", "MD4", "Panama", "RadioGatun32", "RIPEMD", "RIPEMD128", "RIPEMD160", "RIPEMD256", "RIPEMD320",
    "SHA0", "SHA1", "SHA2_224", "SHA2_256", "SHA2_384", "SHA2_512", "SHA2_512_224", "SHA2_512_256",
    "SHA3_224", "SHA3_256", "SHA3_384", "SHA3_512", "Snefru_8_128", "Snefru_8_256",
    "Tiger_3_128", "Tiger_3_160", "Tiger_3_192", "Tiger_4_128", "Tiger_4_160", "Tiger_4_192",
    "Tiger_5_128", "Tiger_5_160", "Tiger_5_192", "Tiger2_3_128", "Tiger2_3_160", "Tiger2_3_192",
    "Tiger2_4_128", "Tiger2_4_160", "Tiger2_4_192", "Tiger2_5_128", "Tiger2_5_160", "Tiger2_5_192", "WhirlPool" };

// The diagnostic steps. A device fault leaves the GPU unusable for the rest of
// the process (on Windows even a new context fails), so the steps run in child
// processes ("-gpudiagfrom"): a child runs steps until one crashes, then the
// parent starts a new child after the crashed step.
struct DiagItem { U32 mode; U32 param; U32 threads; std::string name; };

static std::vector<DiagItem> DiagItems()
{
    std::vector<DiagItem> v;
    v.push_back({ 0, 0, 64,  "memory write" });
    v.push_back({ 10, 0, 64, "thread local memory" });
    v.push_back({ 11, 0, 64, "function call" });
    v.push_back({ 12, 0, 64, "constant tables" });
    v.push_back({ 13, 0, 64, "memcpy header" });
    v.push_back({ 14, 0, 64, "SHA2-256 local buffer" });
    v.push_back({ 15, 0, 64, "SHA2-256 header" });
    v.push_back({ 1, 0, 64,  "SHA2-256 header+state" });
    v.push_back({ 3, 0, 32,  "Expand + Compress" });
    for (U32 a = 0; a < RH2_MAX_ALGO_COUNT; a++)
        v.push_back({ 2, a, 32, FormatString("algo %2u %s", a, c_algoNames[a]) });
    v.push_back({ 4, 0, 1,   "full search 1 thread" });
    v.push_back({ 4, 0, 32,  "full search 32 threads" });
    v.push_back({ 4, 0, 256, "full search 256 threads" });
    return v;
}

static void DiagHeader(U8* header)
{
    for (U32 i = 0; i < 236; i++)
        header[i] = (U8)(i * 37 + 11);
}

// child process: run steps [from, from+count), print one "DIAG" line per step
static void RunDiagChild(U32 from, U32 count, U32 stackKB)
{
    std::vector<DiagItem> items = DiagItems();
    int ordinal = -1;
    for (auto& g : GpuManager::Gpus)
        if (PARNIANMINER_TEST_BIT(g.gpuType, GpuType_NVIDIA) && (g.enabled || ordinal < 0))
            ordinal = (int)g.deviceID;
#ifdef RH2_DIAG_SELFTEST
    for (U32 i = from; i < std::min<U32>((U32)items.size(), from + (count ? count : 100000)); i++)
    {
        if (i == 6 || i == 40 || (i >= items.size() - 2 && stackKB < 16)) { printf("DIAG %u CRASH fake fault\n", i); fflush(stdout); return; }
        printf("DIAG %u OK\n", i); fflush(stdout);
    }
    return;
#endif
    if (ordinal < 0)
    {
        printf("DIAG -1 OPEN no NVIDIA GPU\n");
        return;
    }
    U8 header[236];
    DiagHeader(header);
    Rh2CudaDevice dev;
    std::string err;
    Rh2Options o;
    o.scan = 0;             // the full search step needs the full arena
    o.threads = 256;
    o.blockSize = 32;
    o.stackKB = stackKB;
    if (!dev.Open(ordinal, o, err) || !dev.SetHeader(header, err))
    {
        printf("DIAG -1 OPEN %s\n", err.c_str());
        return;
    }
    U32 end = std::min<U32>((U32)items.size(), from + (count ? count : 100000));
    for (U32 i = from; i < end; i++)
    {
        const DiagItem& it = items[i];
        std::vector<U32> out;
        if (!dev.RunDiag(it.mode, it.param, it.threads, out, err))
        {
            printf("DIAG %u CRASH %s\n", i, err.c_str());
            fflush(stdout);
            return;
        }
        U32 bad = 0;
        for (U32 gid = 0; gid < out.size(); gid++)
            if (out[gid] != Rh2HostDiag(it.mode, it.param, header, gid))
                bad++;
        if (bad)
            printf("DIAG %u WRONG %u of %u results differ from the CPU\n", i, bad, (U32)out.size());
        else
            printf("DIAG %u OK\n", i);
        fflush(stdout);
    }
}

struct DiagResult { int index; std::string status; std::string detail; };

static std::vector<DiagResult> RunDiagProcess(U32 from, U32 count, U32 stackKB)
{
    std::vector<DiagResult> res;
    for (const std::string& l : RunSelf(FormatString("-gpudiagfrom %u -gpudiagcount %u -gpudiagstack %u",
                                                     from + 1, count, stackKB), "DIAG "))
    {
        DiagResult r;
        size_t p1 = l.find(' ');
        r.index = atoi(l.substr(0, p1).c_str());
        size_t p2 = l.find(' ', p1 + 1);
        r.status = l.substr(p1 + 1, p2 == std::string::npos ? std::string::npos : p2 - p1 - 1);
        r.detail = p2 == std::string::npos ? "" : l.substr(p2 + 1);
        res.push_back(r);
    }
    return res;
}

void RandomHashCudaMiner::RunDiagnostics()
{
    // child mode
    if (g_gpuDiagFrom)
    {
        RunDiagChild((U32)g_gpuDiagFrom - 1, (U32)g_gpuDiagCount, g_gpuDiagStack ? (U32)g_gpuDiagStack : (U32)g_gpuStackKB);
        return;
    }

    bool found = false;
    for (auto& g : GpuManager::Gpus)
        if (PARNIANMINER_TEST_BIT(g.gpuType, GpuType_NVIDIA))
        {
            PrintOutCritical("GPU%u %s - diagnostics\n", g.globalIndex, g.description.c_str());
            found = true;
            break;
        }
#ifndef RH2_DIAG_SELFTEST
    if (!found)
    {
        PrintOutCritical("No NVIDIA GPU found (is the NVIDIA driver installed?)\n");
        return;
    }
#endif

    std::vector<DiagItem> items = DiagItems();
    U32 stackKB = (U32)g_gpuStackKB;
    int ver = DriverCudaVersion();
    PrintOutCritical("  NVIDIA driver supports CUDA %d.%d\n", ver / 1000, (ver % 1000) / 10);
    U32 next = 0, okAlgos = 0, algoCount = 0;
    int restarts = 0;
    while (next < items.size() && restarts < 120)
    {
        std::vector<DiagResult> res = RunDiagProcess(next, 0, stackKB);
        restarts++;
        if (res.empty())
        {
            PrintOutCritical("  step %u (%s): no answer from the child process\n", next, items[next].name.c_str());
            next++;
            continue;
        }
        bool advanced = false;
        for (const DiagResult& r : res)
        {
            if (r.index < 0)
            {
                PrintOutCritical("  cannot open the GPU: %s %s\n", r.status.c_str(), r.detail.c_str());
                next++;
                advanced = true;
                continue;
            }
            const DiagItem& it = items[r.index];
            if (it.mode == 2)
            {
                algoCount++;
                if (r.status == "OK")
                    okAlgos++;
                else
                    PrintOutCritical("  %-28s %s %s\n", it.name.c_str(), r.status.c_str(), r.detail.c_str());
                if (it.param == RH2_MAX_ALGO_COUNT - 1)
                    PrintOutCritical("  %-28s %u of %u OK\n", "77 hash algorithms", okAlgos, algoCount);
            }
            else
                PrintOutCritical("  %-28s %s %s\n", it.name.c_str(), r.status.c_str(), r.detail.c_str());
            next = (U32)r.index + 1;
            advanced = true;
        }
        if (!advanced)
            next++;
    }

    // stack size sweep for the full search (32 threads)
    U32 fullIdx = (U32)items.size() - 2;
    const U32 stacks[] = { 4, 8, 16, 32, 64 };
    for (U32 sk : stacks)
    {
        std::vector<DiagResult> res = RunDiagProcess(fullIdx, 1, sk);
        if (res.empty())
            PrintOutCritical("  full search, stack %2u KB      no answer\n", sk);
        else
            PrintOutCritical("  full search, stack %2u KB      %s %s\n", sk, res[0].status.c_str(), res[0].detail.c_str());
    }
    PrintOutCritical("\nPlease send this whole output to the ParnianCoin developers.\n");
}

#endif // PARNIANMINER_CUDA
