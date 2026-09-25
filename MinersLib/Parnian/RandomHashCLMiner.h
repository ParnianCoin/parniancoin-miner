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
#include "MinersLib/GenericCLMiner.h"




class RandomHashCLMiner: public GenericCLMiner
{
    
public:
    RandomHashCLMiner(FarmFace& _farm, unsigned globalWorkMult, unsigned localWorkSize, U32 gpuIndex);
    virtual bool init(const ParnianWorkSptr& work);

    //called only once
    static bool configureGPU();

protected:
    virtual void            QueueKernel();
};

