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

#include "corelib/Log.h"
#include "corelib/Worker.h"
#include "corelib/ParnianWork.h"

using namespace std;

struct KernelOffsetManager
{
    static U64      GetCurrentValue() { return AtomicGet(m_value);  }
    static void     Reset(U64 val);
    static U64      Increment(U32 increment); //return val += inc;
    static U32      GetNextSearchNonce(){ return AtomicIncrement(m_searchNonce);  }
    static void     ResetSearchNonce(U32 v) { AtomicSet(m_searchNonce, v);  }

protected:
    static U64 m_value;
    static U32 m_searchNonce;
};

