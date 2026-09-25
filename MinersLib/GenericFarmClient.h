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
#include "corelib/ParnianWork.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4290)
#endif

//SOLO interface
class FarmFace;
class GenericFarmClient
{
public:
    GenericFarmClient(FarmFace* farm, string farmURL, U32 port, string farmFailOverURL, string email)
    {
        m_farm = farm;
        m_farmURL = farmURL;
        m_port = port;
        m_farmFailOverURL = farmFailOverURL;
        m_email = email;
    }

    virtual ParnianWorkSptr getWork()
    {
    	return ParnianWorkSptr();
    }

    virtual bool submitWork(SolutionSptr sol)
    {
        return false;
    }

    virtual Json::Value awaitNewWork()
    {
    	return Json::Value();
    }
    
    virtual bool progress()
    {
    	return false;
    }

protected:
    FarmFace*   m_farm;
    string      m_farmURL;
    U32         m_port;
    string      m_farmFailOverURL;
    string      m_email;
};

typedef std::shared_ptr<GenericFarmClient> GenericFarmClientSptr;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
