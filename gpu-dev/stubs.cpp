// Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
// GNU General Public License version 3, see the LICENSE file.
// Minimal logging/allocation stubs for the stand-alone test tools in gpu-dev/
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
void PrintOut(const char* f, ...){va_list a;va_start(a,f);vfprintf(stderr,f,a);va_end(a);}
void PrintOutCritical(const char* f, ...){va_list a;va_start(a,f);vfprintf(stderr,f,a);va_end(a);}
void* RH_SysAlloc(size_t s){void* p=nullptr; if(posix_memalign(&p,4096,s)) return nullptr; return p;}
void RH_SysFree(void* p){free(p);}
