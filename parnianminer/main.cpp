/**
 * parnianminer code
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
#include "BuildInfo.h"
#include "ClientManager.h"
#include "MinersLib/Global.h"
#include "MinersLib/GenericMinerClient.h"
#include "MinersLib/AutoPort.h"
#include <atomic>


#ifndef RH_COMPILE_CPU_ONLY
#include "cuda_runtime.h"
#else
#include "corelib/miniweb.h"
#endif

PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("v", g_logVerbosity, "General", "Log verbosity. From 0 to 3.\n0 no log, 1 normal log, 2 include warnings. 3 network and silent logs.\nDefault is 1",0, 3);
PARNIANMINER_COMMAND_LINE_DEFINE_GLOBAL_INT(g_logVerbosity, 1)
bool g_ExitApplication = false;
extern bool g_apiRemote;

void DisplayHelp(CmdLineManager& cmdline)
{ 
    printf("Usage: parnianminer [options]\n");
    cmdline.List();

    printf("\n"
        "ParnianCoin quick start:\n"
        "\n"
        "  Pool mining, automatic port (recommended):\n"
        "    parnianminer -cpu -cputhreads 4 -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x\n"
        "\n"
        "  Pool mining, fixed port:\n"
        "    parnianminer -cpu -cputhreads 4 -s stratum.parniancoin.com:38008 -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x\n"
        "\n"
        "  Solo mining on a ParnianCoin node:\n"
        "    parnianminer -cpu -cputhreads 4 -s http://node.parniancoin.com:38009\n"
        "\n"
        "  Pool ports (starting difficulty is tuned for each hashrate range):\n");
    for (int i = 0; i < g_poolPortTierCount; i++)
        printf("    %s  %s\n", g_poolPortTiers[i].port, g_poolPortTiers[i].descr);
    printf("\n"
        "  With 'auto' the miner starts on port %s, measures its hashrate and moves\n"
        "  to the matching port. The active port is shown in the log.\n"
        "  With a numeric port the miner stays on that port only.\n"
        "\n"
        "  Website: https://parniancoin.com\n\n",
        g_poolPortTiers[0].port);
    exit(0);
}

static void PrintWelcome()
{
    printf("  ------------------------------------------------------------------\n"
           "   Welcome to parnianminer!\n"
           "   The ParnianCoin team wishes you high hashrates and profitable mining.\n"
           "   https://parniancoin.com\n"
           "  ------------------------------------------------------------------\n\n");
}

using namespace std;
using namespace boost::algorithm;
void HandleExit();

#ifdef _WIN32_WINNT
BOOL WINAPI ConsoleHandler(DWORD signal);
long   __stdcall   GlobalExpCallback(_EXCEPTION_POINTERS*   excp);
#endif
 
bool g_appActive = true;
 
#ifdef RH_SCREEN_SAVER_MODE
int main_init(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    ////////////////////////////////////////////////////////////////////
    //
    //      App header
    //
#ifndef RH_COMPILE_CPU_ONLY 
    printf("\n  parnianminer v%s for CPU and NVIDIA GPUs\n", RH_PROJECT_VERSION);
    printf("  Build %s (CUDA SDK %d.%d) %s %s\n\n", RH_BUILD_TYPE, CUDART_VERSION/1000, (CUDART_VERSION % 1000)/10, __DATE__, __TIME__);
#else
    printf("\n  parnianminer v%s for CPU\n", RH_PROJECT_VERSION);
    printf("  Build %s %s %s %s \n\n", RH_OS_NAME, RH_BUILD_TYPE, __DATE__, __TIME__);
#endif    
    PrintWelcome();


#ifdef _WIN32_WINNT
    std::atexit(HandleExit);
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) 
    {
        printf("\nError: Could not set control handler"); 
        return 14454;
    }

    SetUnhandledExceptionFilter(GlobalExpCallback);

    // Initialize Winsock
    WSAData wsa_data;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (iResult != 0) {
        printf("WSAStartup() failed with Error. %d\n", iResult);
        return 1;
    }

#endif

    // Set env vars controlling GPU driver behavior.
	setenv("GPU_MAX_HEAP_SIZE", "100");
	setenv("GPU_MAX_ALLOC_PERCENT", "100");
	setenv("GPU_SINGLE_ALLOC_PERCENT", "100");
    rand32_reseed((U32)(TimeGetMilliSec())^0xF5E8A1C4);

	//Preparse log file and config filename name cuz we need it prior init
    for (int i = 0; i < argc; i++)
    {
        if (stristr(argv[i], "logfilename") && i + 1 < argc)
        {
            SetLogFileName(argv[i + 1]);
        }

        if (strcmp(argv[i], "-v") == 0 && i + 1 < argc)
        {
            g_logVerbosity = ToUInt(argv[i + 1]);
        }

        if (stristr(argv[i], "configfile") && i + 1 < argc)
        {
            CmdLineManager::LoadFromXml(argv[i + 1]);
        }
        
        if (stristr(argv[i], "restarted"))
        {
            CpuSleep(1000);
            CmdLineManager::LoadFromXml("config.txt");
        }
    }

    if (argc == 1)
        CmdLineManager::LoadFromXml("config.txt");

    GlobalMiningPreset::I().Initialize(argv, argc);

    bool displayHelp = false;
    CmdLineManager::GlobalOptions().RegisterFlag("h",           "General", "Display Help", [&]() { displayHelp = true; });
    CmdLineManager::GlobalOptions().RegisterFlag("help",        "General", "Display Help", [&]() { displayHelp = true; });
    CmdLineManager::GlobalOptions().RegisterFlag("?",           "General", "Display Help", [&]() { displayHelp = true; });

    setThreadName("Log");

    //set the coin count right in GpuManager::Gpus
    GpuManager::LoadGPUMap();

    //DISPLAY HELP
    CmdLineManager::GlobalOptions().Parse(argc, argv, true);
    if (displayHelp)
        DisplayHelp(CmdLineManager::GlobalOptions()); //exit app

    PrintOutSilent("Build %s %s %s %s \n", RH_OS_NAME, RH_BUILD_TYPE, __DATE__, __TIME__);

    GpuManager::SetPostCommandLineOptions();

    //warning
    if (g_apiPort && g_apiPW.length())
        printf("\nWARNING: You enabled the remote control API on port %d.\n"
            "         Be sure to NOT start parnianminer from a script in a loop.\n"
            "         This will cause multiple instances of parnianminer to run.\n\n", g_apiPort);
    if (g_apiPort && g_apiRemote)
        printf("WARNING: The API is reachable from the network (-apiremote). Use it only on a trusted network.\n\n");

    KernelOffsetManager::Reset(0);

#ifdef _WIN32_WINNT
    PrintOut("Process priority %d\n", g_setProcessPrio);
    if (g_setProcessPrio == 0)
    {
        BOOL res = SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
        if (!res)
        {
            PrintOut("Error. %d Cannot set priority to background mode. Using IDLE.\n", GetLastError());
            SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
            g_setProcessPrio = 1;
        }
        // NOTE: the console window stays visible. A miner that hides itself
        //       looks like malware to antivirus software and to the user.
    }
    else if (g_setProcessPrio == 1)
        SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    else if (g_setProcessPrio == 2)
        //Force relax mode 
        g_setProcessPrio = 1;
    else
    {
        //High priority mode : Miner threads and stratum have High priority
    }
#endif


#ifdef RH_SCREEN_SAVER_MODE
    //just passing by
#else
    ClientManager::I().Initialize();

    while(g_appActive)
    {
        CpuSleep(200);
        //do stuffs !
    }

    
  #ifdef _WIN32_WINNT
    if (g_setProcessPrio == 0)
        SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);
  #endif

#endif

    CpuSleep(200);
	return 0;
}

#ifdef _WIN32_WINNT
void HandleExit()
{
    g_ExitApplication = true;
    if (g_setProcessPrio == 0)
        SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);

    ClientManager::I().Shutdown(ClientManager::eShutdownLite);
}

bool isCtrlC = false;
BOOL WINAPI ConsoleHandler(DWORD signal) 
{ 
     if ((signal == CTRL_C_EVENT ||
        signal == CTRL_BREAK_EVENT ||
        signal == CTRL_CLOSE_EVENT) && !isCtrlC)
    {
        isCtrlC = true;
        if (g_setProcessPrio == 0)
            SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);

        exit(0);
    }

    return TRUE;
}

long  __stdcall GlobalExpCallback(_EXCEPTION_POINTERS* excp)
{
    //printf("Error. Global exception 0x%X\n", excp->ExceptionRecord->ExceptionCode);

#if defined(RH_SCREEN_SAVER_MODE)
    OutputDebugString("Error. Global exception\n");
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
