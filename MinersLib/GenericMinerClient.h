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

#include "GenericFarmClient.h"
#include "StratumClient.h"

using namespace std;

PARNIANMINER_COMMAND_LINE_DECLARE_GLOBAL_INT("displayspeedtimeout", g_DisplaySpeedTimeout, "General", "Display mining speeds every x seconds.\nDefault is 10", 0, S32_Max)

class BaseMinerClient: public Worker
{
public:
    BaseMinerClient(const char* threadName) :Worker(threadName){}
    virtual ~BaseMinerClient() {};
    bool IsRunning() {return m_running;}

protected:
    virtual void WorkLoop()
    {
        try
        {
		    doStratum();
        }
        catch (...)
        {
            PrintOut("Error. Exception in ");
        }
        m_running = false;
    }
    virtual void doStratum() = 0;

	/// Mining options
	bool m_running = true;
};

//------------------------------------------------------------------------------------------------------------------

class GenericMinerClient: public BaseMinerClient
{
public:
    GenericMinerClient();
    template <typename CL_MINER>
    void InitGpu()
    {
        if (!CL_MINER::configureGPU())
        {
            PARNIANMINER_EXIT_APP("No gpu enabled or found.\n");
        }
    }

    template <typename STRATUM_CLIENT>
    void SetStratumClient(StratumClientSptr& stratumAutoPtrRef)
    {
        FarmPreset* farmInfos = GlobalMiningPreset::I().Get();
        if ((farmInfos->m_farmURL.empty() || 
             farmInfos->m_port.empty()) &&
            !g_testPerformance)
        {
            PrintOut("No url provided to mine.\n"); 
            PARNIANMINER_EXIT_APP("");
        }


        if (stratumAutoPtrRef.get() == 0)
        {
            stratumAutoPtrRef = StratumClientSptr(new STRATUM_CLIENT( 
                StratumInit(&m_farm,
                            farmInfos->m_farmURL, 
                            farmInfos->m_port, 
                            farmInfos->m_user, 
                            farmInfos->m_pass, 
                            farmInfos->m_maxFarmRetries+1, 
                            farmInfos->m_email, 
                            farmInfos->m_soloOvertStratum)));
        }

        m_stratumClient = stratumAutoPtrRef;
    }


    StratumClientSptr    GetStratum() { return m_stratumClient; }

    Farm&   GetFarm() { return m_farm; }

private:
    virtual void doStratum();
    void PushMiniWebData(SolutionStats& farmSol, WorkingProgress& mp);
    
    StratumClientSptr           m_stratumClient;
    GenericFarmClientSptr       m_farmCLient;
    Farm                        m_farm;

    static const U32            m_speedSMACount = 20;
    std::vector<U64>            m_speedSMA;
    
    //pool auto port selection
    void                        HandleAutoPort(double hashrate, const string& activePort);
    U64                         m_autoPortStartMS = 0;
    U64                         m_autoPortLastEvalMS = 0;
    U64                         m_autoPortLastSwitchMS = 0;
    int                         m_autoPortCandidate = -1;
    bool                        m_autoPortDecided = false;

    //watchdogs
    std::thread*                m_WatchdogDevFee = 0;
    U64                         m_zeroSpeedWD = 0;
};
