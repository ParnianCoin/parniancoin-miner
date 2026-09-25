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

#include "precomp.h"
#include "corelib/Log.h"
#include "corelib/Guards.h"
#include <time.h>


using namespace std;

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogName
{
	ThreadLocalLogName(char const* _name) { name = _name; }
	thread_local static char const* name;
};

thread_local char const* ThreadLocalLogName::name;

char const* getThreadName()
{
	return ThreadLocalLogName::name ? ThreadLocalLogName::name : "Log";
}

void setThreadName(char const* _n)
{
	ThreadLocalLogName::name = _n;
}
