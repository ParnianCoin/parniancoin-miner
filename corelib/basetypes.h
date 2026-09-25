/**
 * parnianminer base type functions
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

#ifndef PARNIANMINER_BASE_TYPES_H
#define PARNIANMINER_BASE_TYPES_H

#include <stdint.h>

#if defined(__CUDACC__) // NVCC
   #define RH_ALIGN(n) __align__(n)
#elif defined(__GNUC__)
    #define RH_ALIGN(n) __attribute__((aligned(n)))
#elif defined(_WIN32)
    #define RH_ALIGN(n) __declspec(align(n))
#else
    #error !! UNSUPPORTED COMPILER !!
#endif

#define RH_IS_ALIGNED_32(p) (0 == (3 & ((const char*)(p) - (const char*)0)))
#define RH_IS_ALIGNED_64(p) (0 == (7 & ((const char*)(p) - (const char*)0)))

#if defined(__CUDA_ARCH__)
    #define PARNIANMINER_PLATFORM_CUDA
    #define PARNIANMINER_PLATFORM_GPU
    #define PARNIANMINER_PLATFORM_GPU_CODE(...) __VA_ARGS__
    #define PARNIANMINER_PLATFORM_CPU_CODE(...)
#elif defined(__GPU__) //OPENCL code only
    #define PARNIANMINER_PLATFORM_OPENCL
    #define PARNIANMINER_PLATFORM_GPU
    #define PARNIANMINER_PLATFORM_GPU_CODE(...) __VA_ARGS__
    #define PARNIANMINER_PLATFORM_CPU_CODE(...)
#elif (defined(__unix__) || defined(__unix)) || defined(_WIN32) || defined __x86_64 || defined _M_X64 || defined __i386__ || defined _M_IX86
    #define PARNIANMINER_PLATFORM_CPU
    #define PARNIANMINER_PLATFORM_GPU_CODE(...)
    #define PARNIANMINER_PLATFORM_CPU_CODE(...) __VA_ARGS__
#else
    #error Unsupported platform
#endif

#if defined(RANDOMHASH_CUDA)
    #define CUDA_DECL_KERNEL __global__
    #define CUDA_DECL_DEVICE __device__
    #define CUDA_DECL_HOST_AND_DEVICE __host__ __device__
    #define CUDA_DECL_HOST __host__ 
    #define CUDA_DECL_CONST __constant__
    #define CUDA_DECL_EXTERN
    #define PLATFORM_CONST __device__ __constant__
    #define CUDA_SYM_DECL(SYM)  __device__ cuda_##SYM
    #define CUDA_SYM(SYM)  cuda_##SYM
    #define _CM(X) CUDA_SYM(X)
    #define CUDA_ONLY(X) X
    #define CUDA_NOT_ONLY(X)
    #ifdef __CUDA_ARCH__
        #define PLATFORM_MEMSET memset
        #define PLATFORM_MEMCPY(DST, SRC, S) memcpy(DST, SRC, S)
        #define PLATFORM_BARRIER() __syncthreads()
    #else
        #define PLATFORM_MEMSET cudaMemset
        #define PLATFORM_MEMCPY(DST, SRC, S) cudaMemcpy(DST, SRC, S, cudaMemcpyHostToDevice)
        #define PLATFORM_BARRIER()
    #endif
#else
    #define CUDA_DECL_KERNEL inline
    #define CUDA_DECL_DEVICE
    #define CUDA_DECL_HOST_AND_DEVICE
    #define CUDA_DECL_HOST
    #define CUDA_DECL_CONST const
    #define PLATFORM_CONST const
    #define PLATFORM_MEMSET memset
    #define PLATFORM_MEMCPY(DST, SRC, S) memcpy(DST, SRC, S)
    #define PLATFORM_BARRIER()
    #define CUDA_DECL_EXTERN extern
    #define CUDA_SYM_DECL(SYM)  SYM
    #define CUDA_SYM(SYM)  SYM
    #define _CM(X) X
    #define CUDA_ONLY(X)
    #define CUDA_NOT_ONLY(X) X

    //For research purpose only. AVX code is actually slower on most cpu !
    //#define RH_ENABLE_AVX
#endif


#if !defined(PARNIANMINER_NO_SSE4) || defined(PARNIANMINER_COND_SSE4)
    #define PARNIANMINER_ENABLE_SSE4
#endif

#if defined(_DEBUG)
    #ifndef PARNIANMINER_DEBUG
        #define PARNIANMINER_DEBUG
    #endif
#elif defined(NDEBUG)
    #ifndef PARNIANMINER_RELEASE
        #define PARNIANMINER_RELEASE
    #endif
#else
    #define PARNIANMINER_RELEASE
#endif



#define UTIL_LO32(Operand64Bit) ((U32) ((Operand64Bit) & 0xFFFFFFFF))
#define UTIL_HI32(Operand64Bit) ((U32) (((U64)(Operand64Bit) >> 32) & 0xFFFFFFFF))
#define PARNIANMINER_JOIN( X, Y ) PARNIANMINER_DO_JOIN( X, Y )
#define PARNIANMINER_DO_JOIN( X, Y ) X##Y
#define PARNIANMINER_STRINGIZE(x) PARNIANMINER_STRINGIZE2(x)
#define PARNIANMINER_STRINGIZE2(x) #x
#define PARNIANMINER_ARRAY_COUNT(arr) sizeof(arr) / sizeof(arr[0])
#define PARNIANMINER_KB(X) ((X)*1024LLU)
#define PARNIANMINER_MB(X) (PARNIANMINER_KB(X)*1024LLU)
#define PARNIANMINER_GB(X) (PARNIANMINER_MB(X)*1024LLU)
#define PARNIANMINER_TEST_BIT(VALUE, BIT_MASK) (((VALUE) & (BIT_MASK)) == (BIT_MASK))

#define PARNIANMINER_PTR_TO_ADDR(ptr)                   ((size_t)ptr)
#define PARNIANMINER_ADDR_TO_PTR(addr)                  ((void*)((size_t)addr)) 
#define PARNIANMINER_ADD_PTR(type, Ptr, Ofs)            ((type) (((size_t)(Ptr)) + (size_t)(Ofs)))
#define PARNIANMINER_SUB_PTR(type, Ptr, Ofs)            ((type) (((size_t)(Ptr)) - (size_t)(Ofs)))
#define PARNIANMINER_ALIGN_PTR(type, Ptr, Ofs)          ((type) PARNIANMINER_FLOOR((size_t)(Ptr), Ofs))
#define PARNIANMINER_DIFF_PTR(First, Second)            ((size_t) (((size_t)(First)) - (size_t)(Second)))
#define PARNIANMINER_ROUND(Val, Align)                  (((Val) + (Align-1)) & ~(Align-1))
#define PARNIANMINER_CEIL(size,to)	                   (((size)+ (to)-1) &~((to)-1))
#define PARNIANMINER_ALIGN(Val, Align)	               ((((Val) / (Align)) + 1)*(Align))
#define PARNIANMINER_FLOOR(Val, Align)                  ((Val) & ~((Align) - 1))

#ifdef _DEBUG
  #ifdef _WIN32_WINNT
    #define _PDBN __debugbreak();
  #else
    #define _PDBN {printf("debugbreak %s:%d\n", __FILE__, __LINE__); int* a = 0; *a=0;}
  #endif
#else
    #define _PDBN exit(-(__LINE__));
#endif

extern void PrintOut(const char *szFormat, ...);
#ifndef PARNIANMINER_PLATFORM_GPU
#define PARNIANMINER_ASSERT(x) \
    { \
        if (!(x)) \
        { \
            PrintOutCritical("Error. Assert '%s' \n", #x); \
            _PDBN; \
        } \
    }

#else //GPU 
    #define PARNIANMINER_ASSERT(x) \
    { \
        if (!(x)) \
        { \
            printf("Error. Assert '%s' \n", #x); \
            exit(-(__LINE__)); \
        } \
    }
#endif




#if !defined(PARNIANMINER_PLATFORM_GPU) && !defined(RANDOMHASH_CUDA)
    typedef uint8_t byte;
    #include <vector>
    #include <iostream>     // std::cout

    // Binary data types.
    using bytes = std::vector<byte>;
    using strings = std::vector<std::string>;
#endif //!defined(PARNIANMINER_PLATFORM_GPU) && !defined(RANDOMHASH_CUDA)

    typedef int Device16[16];

    typedef int8_t   S8;
    typedef int16_t  S16;
    typedef int32_t  S32;
    typedef int64_t  S64;
    typedef uint8_t  U8;
    typedef uint16_t U16;
    typedef uint32_t U32;
    typedef uint64_t U64;
    typedef float    F32;
    typedef double   F64;

    const S8  S8_Min = -128;                    
    const S8  S8_Max = 127;                     
    const U8  U8_Min = 0;                       
    const U8  U8_Max = 255;                     
    const S16 S16_Min = -32768;                 
    const S16 S16_Max = 32767;                  
    const U16 U16_Min = 0;                      
    const U16 U16_Max = 65535;                  
    const S32 S32_Min = -2147483647;            
    const S32 S32_Max = 2147483647;             
    const U32 U32_Min = 0;                      
    const U32 U32_Max = 4294967295;             
    const S64 S64_Min = 0x8000000000000000;     
    const S64 S64_Max = 0x7FFFFFFFFFFFFFFF;     
    const U64 U64_Min = 0;                      
    const U64 U64_Max = 0xFFFFFFFFFFFFFFFF;     
    const F32 F32_Min = 1.175494351E-38F;       
    const F32 F32_Max = 3.402823466E+38F;       
    const F64 F64_Min = 2.2250738585072014E-308;
    const F64 F64_Max = 1.7976931348623158E+308;

#endif //PARNIANMINER_BASE_TYPES_H
