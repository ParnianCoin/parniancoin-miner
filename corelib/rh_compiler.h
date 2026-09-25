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

// Visual Studio doesn't support the inline keyword in C mode
#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif

// pretend restrict is a standard keyword
#if defined(_MSC_VER)
#define restrict __restrict
#else
#define restrict __restrict__
#endif


//----------------------------------------------------------------------------
#include <exception>

#if __cplusplus >= 201103L
#define RH_OVERRIDE override
#define RH_NOEXCEPT noexcept
#elif defined(_MSC_VER) && _MSC_VER > 1600 && _MSC_VER < 1900
#define RH_OVERRIDE override
#define RH_NOEXCEPT throw()
#elif defined(_MSC_VER) && _MSC_VER >= 1900
#define RH_OVERRIDE override
#define RH_NOEXCEPT noexcept
#else
#define RH_OVERRIDE
#define RH_NOEXCEPT throw()
#endif

#if defined(_WIN32)    || defined(_WIN64) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    #define RH_OS_NAME "Windows"
#elif defined(sun) || defined(__sun) || defined(__SVR4) || defined(__svr4__)
    #define RH_OS_NAME "Solaris"
#else
    #if defined(__linux__)
        #define RH_OS_NAME "Linux"
    #elif defined(BSD)
        #if defined(MACOS_X) || (defined(__APPLE__) & defined(__MACH__))
            #define RH_OS_NAME "MacOS"
        #elif defined(macintosh) || defined(Macintosh)
            #define RH_OS_NAME "Macintosh"
        #elif defined(__OpenBSD__)
            #define RH_OS_NAME "OpenBSD"
        #else
            #define RH_OS_NAME "BSD"
        #endif
    #else
        #define RH_OS_NAME "OS"
    #endif
#endif


#define RH_BUILD_TYPE PARNIANMINER_STRINGIZE(RH_ARCH)

class RH_Exception : public std::exception 
{
public:
    explicit RH_Exception(const char* const& msg): msg_(msg)  {}
    ~RH_Exception() RH_NOEXCEPT {}
    virtual char const* what() const RH_NOEXCEPT RH_OVERRIDE  {return msg_; }

protected:
  const char* msg_;
};
