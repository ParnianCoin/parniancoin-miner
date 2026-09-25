/**
 * Global miner data source code implementation
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


#include "precomp.h"
#include "MinersLib/Global.h"
#include "corelib/Worker.h"
#include "corelib/ParnianWork.h"
#include "MinersLib/CLMinerBase.h"
#include "MinersLib/StratumClient.h"
#include "parnianminer/ClientManager.h"
#include "corelib/miniweb.h"
#include "MinersLib/Parnian/RandomHashCLMiner.h"
#include "MinersLib/Parnian/RandomHashCPUMiner.h"
#include "MinersLib/AutoPort.h"

#ifndef RH_COMPILE_CPU_ONLY
#include "MinersLib/Parnian/RandomHashHostCudaMiner.h"
#endif

PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_STRING(g_logFileNameDummy, "");
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_STRING(g_configFileNameDummy, "");
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_BOOL(g_useCPU, true);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_BOOL(g_restared, false);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_testPerformance, 0);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_testPerformanceThreads, 0);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_setProcessPrio, 3);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_memoryBoostLevel, RH_OPT_UNSET);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_sseOptimization, 0); 

bool g_useGPU = false;
U32  g_cpuMinerThreads = 0;
bool g_disableMaxGpuThreadSafety = false;
extern void StopMiniWeb();
extern bool g_appActive;

const U64 t1M = 1000 * 60;
const U64 t1H = t1M * 60;
const U64 t24H = t1H * 24;
const U64 t3_8M = (U64)(3.8f*t1M); 
const U64 t45S = (U64)(45*1000);

GlobalMiningPreset& GlobalMiningPreset::I()
{
    static GlobalMiningPreset I;
    return I;
}


GlobalMiningPreset::GlobalMiningPreset()
{
    devFeeMutex = new std::mutex;
    m_startTimeMS = TimeGetMilliSec();

#ifndef RH_COMPILE_CPU_ONLY
    CmdLineManager::GlobalOptions().RegisterValue("gputhreads", "Gpu", "Cuda thread count. ex: -gputhreads 100 launches 100 threads on selected gpu", [&](const string& val) 
    { 
        std::vector<string> istr = GetTokens(val, ",");
        if (istr.size() == 1)
        {
            for (auto& g : GpuManager::Gpus)
            {
                g.setGpuThreadCount = ToUInt(istr[0]);
            }
        }
        else if (istr.size() > 1)
        {
            for (U32 i = 0; i < istr.size(); i++)
            {
                if (i < GpuManager::Gpus.size())
                    GpuManager::Gpus[i].setGpuThreadCount = ToUInt(istr[i]);
            }
        }
    });

    CmdLineManager::GlobalOptions().RegisterValue("gpu", "Gpu", "Enable individual GPU by their index. GPU not in the list will be disabled. ex: -gpu 0,3,4.", [&](const string& val) 
    { 
        //List GPU : AMD, NVidia and the rests
        std::vector<string> idstr = GetTokens(val, ",");
        
        for(auto& i : idstr)
        {
            U32 id = ToUInt(i);
            if (id < GpuManager::Gpus.size())
            {
                GpuManager::Gpus[id].enabled = true;
            }
            else
            {
                PrintOut("GPU%d does not exist\n", id);
                PARNIANMINER_EXIT_APP("");
            }
        }
    });
#endif //RH_COMPILE_CPU_ONLY
    CmdLineManager::GlobalOptions().RegisterFlag("list", "General", "List all gpu and cpu in the system", [&]() 
    {
        GpuManager::listGPU(); 
        exit(0);
    });

    CmdLineManager::GlobalOptions().RegisterFlag("completelist", "General", "Exhaustive list of all devices in the system", [&]() { GpuManager::listDevices(); });
}

void GlobalMiningPreset::FailOverURL(const string& val)
{
    string url = val;
    ReplaceStringALL(url, "stratum+tcp://", "");
    ReplaceStringALL(url, "http://", "");

    size_t p = url.find_last_of(":");
    if (p != string::npos)
    {
        m_presets.m_farmFailOverURL = url.substr(0, p);
        if (p + 1 <= url.length())
            m_presets.m_fport = url.substr(p + 1);
    }
    else
    {
        m_presets.m_farmFailOverURL = url;
    }

    // Failover pool can also use automatic port selection
    m_presets.m_fAutoPort = false;
    if (IsAutoPortKeyword(m_presets.m_fport))
    {
        if (val.find("http://") != string::npos)
        {
            PrintOut("Error. 'auto' port is only available for pool mining, not for solo mining (http://).\n");
            PARNIANMINER_EXIT_APP("");
        }
        m_presets.m_fAutoPort = true;
        m_presets.m_fport = g_poolPortTiers[0].port;
    }
}

void GlobalMiningPreset::SetStratumInfo(const string& val)
{
    CmdLineManager::GlobalOptions().PreParseSymbol("su");

    if (val.find("http://") != string::npos)
        m_presets.m_soloOvertStratum = true;

    string url = val;
    ReplaceStringALL(url, "stratum+tcp://", "");
    ReplaceStringALL(url, "http://", "");

    int userPos = (int)url.find("/");
    if (userPos != url.npos)
    {
        if (m_presets.m_user.empty())
        {
            string user = url.substr(userPos + 1);
            if (user.length())
                m_presets.m_user = user;
        }
        url = url.substr(0, userPos);
    }

    size_t p = url.find_last_of(":");
    if (p != string::npos)
    {
        m_presets.m_farmURL = url.substr(0, p);
        if (p + 1 <= url.length())
            m_presets.m_port = url.substr(p + 1);
    }
    else
    {
        m_presets.m_farmURL = url;
        m_presets.m_port = "1379";
    }

    // Pool mode: "-s stratum+tcp://host:auto" lets the miner pick the port
    // that matches its hashrate. Solo mining (http://) is left unchanged.
    m_presets.m_autoPort = false;
    if (IsAutoPortKeyword(m_presets.m_port))
    {
        if (m_presets.m_soloOvertStratum)
        {
            PrintOut("Error. 'auto' port is only available for pool mining, not for solo mining (http://).\n"
                     "       For solo mining give the node port, ex: -s http://node.parniancoin.com:38009\n");
            PARNIANMINER_EXIT_APP("");
        }
        m_presets.m_autoPort = true;
        m_presets.m_port = g_poolPortTiers[0].port;
    }
}


void GlobalMiningPreset::Initialize(char** argv, int argc)
{
    CmdLineManager::GlobalOptions().RegisterValue("s", "Network", "Pool or node address.\n"
        "Pool mining : host:port or host:auto (the stratum+tcp:// prefix is optional)\n"
        "              'auto' selects the pool port that matches your hashrate\n"
        "              and keeps adjusting it. A numeric port is used as is.\n"
        "              ex: -s stratum.parniancoin.com:auto\n"
        "Solo mining : http://host:port of a ParnianCoin node (wallet).\n"
        "              ex: -s http://node.parniancoin.com:38009", [&](const string& val) 
    { 
        SetStratumInfo(val); 
    });

    CmdLineManager::GlobalOptions().RegisterValue("su", "Network", "Pool user: YOUR_PARNIAN_ACCOUNT.WORKER_NAME\nex: -su YOUR_PARNIAN_ACCOUNT.rig1", [&](const string& val) {  m_presets.m_user = val; });
    CmdLineManager::GlobalOptions().RegisterValue("pw", "Network", "Pool password. Usually any value, ex: -pw x", [&](const string& val) { m_presets.m_pass = val; });
    CmdLineManager::GlobalOptions().RegisterValue("fo", "Network", "Failover pool/node address. Same format as -s ('auto' port allowed for pools)", [&](const string& val) { FailOverURL(val); });
    CmdLineManager::GlobalOptions().RegisterValue("fou", "Network", "Failover pool user. Default is the -su value", [&](const string& val) { m_presets.m_fuser = val; });
    CmdLineManager::GlobalOptions().RegisterValue("fop", "Network", "Failover pool password", [&](const string& val) { m_presets.m_fpass = val; });
    CmdLineManager::GlobalOptions().RegisterValue("r", "Network", "Connection retries before switching to failover (or exiting)", [&](const string& val) { m_presets.m_maxFarmRetries = ToInt(val); });
    
    CmdLineManager::GlobalOptions().RegisterValueMultiple("diff", "General", "Set local difficulty. ex: -diff 999", [&](const string& val)
    { 
        m_localDifficulty = ToFloat(val);
        if (m_localDifficulty != 0.0f)
            PrintOut("Setting local difficulty to %.4f\n", m_localDifficulty);
    });

    CmdLineManager::GlobalOptions().RegisterValueMultiple("cputhreads", "Gpu", "Number of CPU miner threads when mining with CPU. ex: -cpu -cputhreads 4.\nNOTE: adding + before thread count will disable the maximum thread count safety of one thread per core/hyperthread.\nUse this option at your own risk.", [&](const string& val)
    {
        string cmt = val;
        if (cmt.length() && cmt[0] == '+')
        {
            g_disableMaxGpuThreadSafety = true;
            cmt = cmt.substr(1);
        }

        g_cpuMinerThreads = ToUInt(cmt);
    });

    CmdLineManager::GlobalOptions().RegisterValueMultiple("processorsaffinity", "General", "On windows only. Force miner to only run on selected logical core processors.\nex: -processorsaffinity 0,3 will make the miner run only on logical core #0 and #3.\nWARNING: Changing this value will affect GPU mining.", [&](const string& val)
    { 
#ifdef _WIN32_WINNT
        CmdLineManager::GlobalOptions().PreParseSymbol("cputhreads");

        strings values = GetTokens(val, ",");
        std::vector<U32> iVals;
        if (iVals.size() > GpuManager::CpuInfos.numberOfProcessors)
        {
            PrintOutCritical("Error. You selected %d logical cores while there is %d on this system", iVals.size(), GpuManager::CpuInfos.numberOfProcessors);
            PARNIANMINER_EXIT_APP("");
        }
        
        for(auto& v:values)
        {
            U64 core = (U64)ToUInt(v);
            if (core < GpuManager::CpuInfos.numberOfProcessors)
            {
                GpuManager::CpuInfos.UserSelectedCores |= (1LLU << core);
                GpuManager::CpuInfos.UserSelectedCoresCount++;
            }
        }
        PrintOut("Warning: Setting processor affinity WILL affect gpu mining. Use with caution.\n");

        if (GpuManager::CpuInfos.UserSelectedCoresCount > g_cpuMinerThreads)
            PrintOut("Warning: You selected %d logical cores to mine but only %d working threads.\n", GpuManager::CpuInfos.UserSelectedCoresCount, g_cpuMinerThreads);
#else
        PrintOut("-processorsaffinity not implemented yet\n");
#endif
    });
}

FarmPreset* GlobalMiningPreset::Get()
{
    return &m_presets;
};


U64 GetTimeRangeRnd(U64 minMS, U64 maxMS)
{
    U64 spot= 0;
    spot = minMS + (rand32() % (maxMS-minMS));
    return spot;
}

bool GlobalMiningPreset::DetectDevfeeOvertime()
{
    // parnianminer: dev fee removed - the miner never enters dev mode
    return false;
}

U32 GlobalMiningPreset::GetUpTimeMS()
{
    return (U32)(TimeGetMilliSec() - m_startTimeMS);
}

Miner* GlobalMiningPreset::CreateMiner(CreatorClasType type, FarmFace& _farm, U32 gpuIndex)
{
    // parnianminer: dev fee removed - dev-mode timer is disabled permanently
    m_devFeeTimer24hMS = U64_Max;


#ifndef RH_COMPILE_CPU_ONLY
    if (type == ClassOpenCL)
        return new RandomHashCLMiner(_farm, 0, 0, gpuIndex);
    if (type == ClassNvidia)
        return new RandomHashHostCudaMiner(_farm, 0, 0, gpuIndex);
#endif
    if (type == ClassCPU)
        return new RandomHashCPUMiner(_farm, 0, 0, gpuIndex);

    PARNIANMINER_EXIT_APP("critical");
}

void GlobalMiningPreset::SetRestart(ERestartMode val)
{
    AtomicSet(m_requestRestart, (U32)val);
    if (val == eExternalRestart)
    {
#ifdef _WIN32_WINNT
        char exeFN[MAX_PATH];
        *exeFN;
        GetModuleFileName(0, exeFN, sizeof(exeFN));
        if (strlen(exeFN))
        {
            char dir[MAX_PATH];
            char f[MAX_PATH];
            char fn[MAX_PATH];
            char ex[MAX_PATH];
            _splitpath(exeFN, dir, f, fn, ex);
            strncat(dir, f, sizeof(dir));

            string cmd;
            if (GlobalMiningPreset::I().GetPendingConfigFile().length()) 
                cmd += " -restarted ";

            cmd += CmdLineManager::GlobalOptions().GetArgsList();
            PrintOutSilent("Restarting to %s\n", cmd.c_str());

            char cwdDir[1024] = "";
            __getcwd(cwdDir, sizeof(cwdDir));

            STARTUPINFO si = {};
            si.cb = sizeof si;
            PROCESS_INFORMATION pi = {};
            if (!CreateProcess(exeFN, (char*)cmd.c_str(), 0, FALSE, 0, 0, 0, cwdDir, &si, &pi))
            {
                PrintOutCritical("Cannot Restart parnianminer.\n");
            }
            else
            {
                exit(0);
            }
        }
        else
            PrintOutCritical("Cannot get module filename for restart. Restart aborted.\n");

#else
        int*   processId = new int;
        char  *exec_path_name = (char*)malloc(strlen(CmdLineManager::GlobalOptions().GetArgv()[0]) + 1);
        strcpy(exec_path_name, CmdLineManager::GlobalOptions().GetArgv()[0]);

        //stop all now!
        StopMiniWeb();
        CloseLog();
        g_appActive = true;

        char** agvI = CmdLineManager::GlobalOptions().GetArgv();
        char *argv2[128 + 1];
        char** agvI2 = argv2;
        while (*agvI)
        {
            *agvI2 = (char*)malloc(strlen(*agvI) + 1);
            strcpy(*agvI2, *agvI);
            agvI++;
            agvI2++;
        }

        if (GlobalMiningPreset::I().GetPendingConfigFile().length())
        {
            const char* restart = "-restarted";
            *agvI2 = (char*)malloc(strlen(restart) + 1);
            strcpy(*agvI2, restart);
            agvI2++;
        }
        *agvI2 = 0;

        pid_t pid;
        pid = fork();
        if (pid == 0)
        {
            char *envp[] = { NULL };
            int err = execve(exec_path_name, argv2, envp);
            if (err != 0)
                printf("exec error %d\n", errno);
        }
        exit(0);
#endif
    }
}

void GlobalMiningPreset::RequestReboot()
{
    U32 lastVal = AtomicSet(m_requestReboot, 1);
    if (lastVal == 1)
        return;

    char basePath[1024] = "";
    __getcwd(basePath, sizeof(basePath));

#ifdef _WIN32_WINNT    
    strncat(basePath, "\\", sizeof(basePath)-1);
    strncat(basePath, "reboot.bat", sizeof(basePath) - 1);
#else
    strncat(basePath, "/", sizeof(basePath) - 1);
    strncat(basePath, "reboot.sh", sizeof(basePath) - 1);
#endif
    if (GetFileSize(basePath) == U64_Max)
        PrintOut("Launch Error. file. %s does not exists\n", basePath);
    else
    {
        string cmd;
#ifdef _WIN32_WINNT
        cmd = "start \"reboot\" \"";
        cmd += basePath;
        cmd += "\"";
#else
        cmd = "sh ";
        cmd += basePath;
#endif
        system(cmd.c_str());
    }
}

void GlobalMiningPreset::DoPerformanceTest()
{
    vector<RandomHashResult> out_hash2;

    mersenne_twister_state rnd;
    merssen_twister_seed(0xF923A401, &rnd);
    
    if (g_testPerformanceThreads == 0 || (U32)g_testPerformanceThreads > GpuManager::CpuInfos.numberOfProcessors)
        g_testPerformanceThreads = GpuManager::CpuInfos.numberOfProcessors;
        
    const size_t ThreadCount = g_testPerformanceThreads;
    out_hash2.resize(g_testPerformanceThreads);
    RandomHash_State* g_threadsData2=0;
    RandomHash_CreateMany(&g_threadsData2, ThreadCount);

    U32 nonce2 = 0;
    
    PrintOut("CPU: %s\n", GpuManager::CpuInfos.cpuBrandName.c_str());
    PrintOut("Testing raw cpu performance for %d sec on %d threads\n", g_testPerformance, ThreadCount);
    
    U64 timeout[] = { 5 * 1000, (U64)g_testPerformance * 1000 };

    std::vector<U64> hashes;
    hashes.resize(ThreadCount);

    auto kernelFunc = [&](void* allStates, U32 startNonce, U64 to)
    {
        U32 gid = 0;
        while (TimeGetMilliSec() < to)
        {
            RandomHash_Search((RandomHash_State*)allStates, out_hash2[startNonce], startNonce + gid++);
            hashes[startNonce] += out_hash2[startNonce].count;
        }
    };

    PrintOut("Warming up...\n");
    for(U32 timeoutID = 0; timeoutID < 2; timeoutID++)
    {
        U32 input[ParnianHeaderSizeV5/4];
        for (int i = 0; i < ParnianHeaderSizeV5 / 4; i++)
            input[i] = merssen_twister_rand(&rnd);

        input[ParnianHeaderNoncePosV4(ParnianHeaderSizeV5) / 4] = 0;

        for(int i=0; i < ThreadCount; i++)
        {
            RandomHash_SetHeader(&g_threadsData2[i], (U8*)input, ParnianHeaderSizeV5, nonce2);
        }

        {
            std::vector<std::thread> threads(ThreadCount);
            U32 gid=0;
            for(int i = 0; i < ThreadCount; i++) 
            {
                threads[i] = std::thread([&] 
                {
                    U32 _gid = AtomicIncrement(gid);
                    RH_SetThreadPriority(RH_ThreadPrio_High);
                    kernelFunc((void*)&g_threadsData2[_gid-1], _gid-1, TimeGetMilliSec() + timeout[timeoutID]); 
                }
                );
            }
            for(std::thread & thread : threads) 
                thread.join();
        }
        PrintOut("Testing performance...\n");
        CpuSleep(20);
        if (timeoutID == 0)
        {
            for (auto& h : hashes)
                h = 0;
        }
    }

    U64 hashCnt = 0;
    for (auto h : hashes)
        hashCnt += h;
    PrintOut("RandomHash speed is %.2f H/S\n", hashCnt / (float)g_testPerformance);
    
    exit(0);
}

bool GlobalMiningPreset::UpdateToDevModeState(string& connectionParams)
{
    // parnianminer: dev fee removed.
    // Originally this rotated the miner onto the developer's pool ~1% of the time.
    // It now never switches: mining always stays on the user's own pool.
    connectionParams = "";
    return false;
}
