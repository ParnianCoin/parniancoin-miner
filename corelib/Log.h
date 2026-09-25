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

#include <ctime>
#include <chrono>
#include "vector_ref.h"
#include "Common.h"
#include "CommonData.h"
#include "FixedHash.h"

/// The null output stream. Used when logging is disabled.
class NullOutputStream
{
public:
	template <class T> NullOutputStream& operator<<(T const&) { return *this; }
};


class ThreadContext
{
public:
	ThreadContext(std::string const& _info) { push(_info); }
	~ThreadContext() { pop(); }

	static void push(std::string const& _n);
	static void pop();
	static std::string join(std::string const& _prior);
};

/// Set the current thread's log name.
void setThreadName(char const* _n);

/// Set the current thread's log name.
char const* getThreadName();
