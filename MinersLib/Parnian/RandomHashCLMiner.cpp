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


#include "precomp.h"
#include "RandomHashCLMiner.h"
#include "MinersLib/Global.h"


constexpr size_t c_maxSearchResults = 1;

RandomHashCLMiner::RandomHashCLMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex) :
    GenericCLMiner(_farm, globalWorkMult, localWorkSize, gpuIndex)
{
}

bool RandomHashCLMiner::init(const ParnianWorkSptr& work)
{
    AddPreBuildFunctor([&](string& code) 
    {
        addDefinition(code, "PAS_FAST", 1);
    });

    auto res = GenericCLMiner::init(work);
    return res;
}

void RandomHashCLMiner::QueueKernel()
{
    GenericCLMiner::QueueKernel();
}
/*
SolutionSptr RandomHashCLMiner::MakeSubmitSolution(const std::vector<U64>& nonces, bool isFromCpuMiner)
{
    ParnianSolution* sol = new ParnianSolution();
    sol->m_results = nonces;
    sol->m_gpuIndex = m_globalIndex;
    sol->m_work = ParnianWorkSptr(m_currentWp->Clone());
    sol->m_isFromCpuMiner = isFromCpuMiner;

    return SolutionSptr(sol);
}

*/
bool RandomHashCLMiner::configureGPU()
{
    return GpuManager::SetupGPU();
}



