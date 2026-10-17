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
#pragma once

#ifdef PARNIANMINER_CUDA

#include "MinersLib/GenericCLMiner.h"
#include "MinersLib/Cuda/CudaDriver.h"
#include "cudalib/rh2_kernel_api.h"
#include <deque>

#ifdef RH_COMPILE_CPU_ONLY
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_BOOL("kernelactivewaiting", g_kernelactivewaiting, "Gpu", "Spin the cpu while a GPU kernel runs (more stable on multi-gpu, uses one cpu core per GPU)");
#endif
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpublock", g_gpuBlockSize, "Gpu", "CUDA threads per block (32-256). Default 64", 32, 256);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpustack", g_gpuStackKB, "Gpu", "CUDA stack per thread in KB (8-64). Default 16", 4, 64);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpuregs", g_gpuRegs, "Gpu", "GPU kernel register limit: 0 (automatic), 128 or 64.\nFewer registers let more threads run at once. Use -gpubench to find the best value.", 0, 255);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpuscan", g_gpuScan, "Gpu", "GPU nonce scanning: 2 (default), 3 or 0 (off).\nOnly nonces that finish within 2 (or 3) RandomHash2 rounds are completed: less work and much less\nmemory per hash, fewer hashes per nonce. The hashes are identical. Use -gpubench to find the best value.", 0, 3);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpuoverlap", g_gpuOverlap, "Gpu", "Queue the next GPU kernel while the results of the previous one are processed: 1 (default) or 0", 0, 1);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gputest", g_gpuTestSeconds, "Gpu", "Verify the GPU kernel against the CPU algorithm, then benchmark it for the given seconds and exit.\nUses the GPUs selected with -gpu (all NVIDIA GPUs if none). ex: -gputest 60", 0, 3600);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpubench", g_gpuBenchSeconds, "Gpu", "Test every GPU kernel configuration (-gpuregs, -gpuscan, -gpublock, -gpuoverlap) for the given\nseconds each, verify the results against the CPU and print the fastest options. Then exit. ex: -gpubench 8", 0, 600);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_BOOL("gpudiag", g_gpuDiag, "Gpu", "Run the GPU kernel step by step (memory, SHA-256, each of the 77 algorithms, full search with several stack sizes),\ncompare every result with the CPU and print where the GPU fails. Then exit.");
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpudiagfrom", g_gpuDiagFrom, "Debug", "Internal: -gpudiag child process, first step + 1", 0, 100000);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpudiagcount", g_gpuDiagCount, "Debug", "Internal: -gpudiag child process, step count (0 = all)", 0, 100000);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpudiagstack", g_gpuDiagStack, "Debug", "Internal: -gpudiag child process, stack KB", 0, 64);
PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("gpubenchrun", g_gpuBenchRun, "Debug", "Internal: -gpubench child process, configuration + 1", 0, 1000);

// GPU kernel options
struct Rh2Options
{
    U32  threads = 0;       // 0: automatic
    U32  blockSize = 64;
    U32  stackKB = 16;
    bool spinWait = false;
    U32  regs = 0;          // 0 (compiler choice), 128, 64
    U32  scan = 2;          // nonce scanning: 2 (default), 3, 0 (off)
    bool overlap = true;    // two launches in flight

    static Rh2Options FromCommandLine(U32 threads);
    std::string ToString() const;
    U32 ArenaBytes() const;  // per thread
};

// One GPU: context, embedded kernel module, buffers and launches.
// Launch() queues a kernel (results copied back asynchronously), Wait()
// returns the results of the oldest launch; at most two are in flight.
class Rh2CudaDevice
{
public:
    ~Rh2CudaDevice() { Close(); }

    bool Open(int ordinal, const Rh2Options& opts, std::string& err);
    void Close();
    bool SetHeader(const U8* header236, std::string& err);
    bool Launch(U32 startNonce, U32 target, std::string& err);
    bool Wait(RH2_KernelResults& out, std::string& err);
    U32  Pending() const { return (U32)(m_launched - m_completed); }
    // Launch + Wait
    bool Run(U32 startNonce, U32 target, RH2_KernelResults& out, std::string& err);
    // diagnostic kernel (cudalib/rh2/rh2_diag.h), threads <= MaxThreads()
    bool RunDiag(U32 mode, U32 param, U32 threads, std::vector<U32>& out, std::string& err);

    // threads launched per run: adapted so one run stays well below the
    // Windows TDR limit (2 s) on slow / laptop GPUs
    U32    Threads() const   { return m_active; }
    U32    MaxThreads() const { return m_threads; }
    void   AdaptLaunch(U64 launchMs);
    U32    BlockSize() const { return m_blockSize; }
    int    Registers() const { return m_regs; }
    bool   Overlap() const   { return m_opts.overlap; }
    double MemoryGB() const;

private:
    bool Ok(cudrv::CUresult r, const char* what, std::string& err);

    const cudrv::Api*   m_api = nullptr;
    Rh2Options          m_opts;
    cudrv::CUcontext    m_ctx = nullptr;
    cudrv::CUmodule     m_module = nullptr;
    cudrv::CUfunction   m_kernel = nullptr;
    cudrv::CUfunction   m_diagKernel = nullptr;
    cudrv::CUdeviceptr  m_dHeader = 0;
    cudrv::CUdeviceptr  m_dThreads = 0;
    cudrv::CUdeviceptr  m_dArenas = 0;
    cudrv::CUdeviceptr  m_dResults[2] = { 0, 0 };
    RH2_KernelResults*  m_hResults[2] = { nullptr, nullptr };   // pinned host memory
    cudrv::CUevent      m_events[2] = { nullptr, nullptr };
    U64                 m_launched = 0;
    U64                 m_completed = 0;
    U32                 m_threads = 0;      // allocated
    U32                 m_active = 0;       // launched
    U32                 m_blockSize = 64;
    U32                 m_arenaWords = 0;
    int                 m_regs = 0;
};

class RandomHashCudaMiner : public GenericCLMiner
{
public:
    RandomHashCudaMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
    ~RandomHashCudaMiner() override;

    static bool configureGPU();
    virtual PlatformType GetPlatformType() { return PlatformType_CUDA; }
    virtual bool init(const ParnianWorkSptr& work);
    virtual void UpdateWorkSize(U32 absoluteVal);

    // -gputest: correctness check against the CPU algorithm + benchmark
    static void RunSelfTests(U32 seconds);
    // -gpudiag
    static void RunDiagnostics();
    // -gpubench
    static void RunBench(U32 seconds);

protected:
    virtual PrepareWorkStatus PrepareWork(const ParnianWorkSptr& workTempl, bool reuseCurrentWP = false);
    virtual void QueueKernel();
    virtual void EvalKernelResult() {}
    virtual void ClearKernelOutputBuffer() {}
    virtual void AddHashCount(U64 hashes) {}   // counted from the real kernel results
    virtual KernelCodeAndFuctions GetKernelsCodeAndFunctions() { return KernelCodeAndFuctions(); }

private:
    struct Inflight
    {
        ParnianWorkSptr wp;
        U32 targetFull[8];
    };

    Rh2CudaDevice       m_dev;
    int                 m_deviceOrdinal = 0;
    bool                m_failed = false;
    U32                 m_target = 0;
    U32                 m_targetFull[8];
    U32                 m_nonceBase = 0;
    U64                 m_overflows = 0;
    U64                 m_lastDoneMs = 0;
    ParnianWorkSptr     m_launchWp;           // work package the next launch is for
    std::deque<Inflight> m_inflight;          // work packages of the launches in flight
    RH2_KernelResults   m_hostResults;
};

#endif // PARNIANMINER_CUDA
