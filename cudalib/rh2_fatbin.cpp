/**
 * Embeds cudalib/bin/rh2_kernel.fatbin into the miner executable
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 *
 * RH2_FATBIN_PATH is set by CMake. The assembler .incbin directive is supported
 * by gcc and mingw (GNU as), so no generated C array is needed.
 */
#ifndef RH2_FATBIN_PATH
    #error RH2_FATBIN_PATH must point to rh2_kernel.fatbin
#endif

#define RH2_STR2(x) #x
#define RH2_STR(x) RH2_STR2(x)

#if defined(_WIN32)
    #define RH2_RODATA ".section .rdata,\"dr\"\n"
#else
    #define RH2_RODATA ".section .rodata\n"
#endif

#if defined(_WIN32) && !defined(_WIN64)
    #define RH2_SYM(x) "_" #x       // 32-bit COFF symbols carry a leading underscore
#else
    #define RH2_SYM(x) #x
#endif

// embeds a file between the symbols <name>_data and <name>_end
#define RH2_EMBED(name, path) __asm__(                  \
    RH2_RODATA                                          \
    ".balign 64\n"                                      \
    ".global " RH2_SYM(name##_data) "\n"                \
    RH2_SYM(name##_data) ":\n"                          \
    ".incbin \"" RH2_STR(path) "\"\n"                   \
    ".global " RH2_SYM(name##_end) "\n"                 \
    RH2_SYM(name##_end) ":\n"                           \
    ".byte 0\n"                                         \
    ".text\n")

// empty blob (variant not built)
#define RH2_EMPTY(name) __asm__(                        \
    RH2_RODATA                                          \
    ".global " RH2_SYM(name##_data) "\n"                \
    RH2_SYM(name##_data) ":\n"                          \
    ".global " RH2_SYM(name##_end) "\n"                 \
    RH2_SYM(name##_end) ":\n"                           \
    ".byte 0\n"                                         \
    ".text\n")

// main kernel (compiler chosen registers, includes -gpudiag)
RH2_EMBED(rh2_fatbin, RH2_FATBIN_PATH);

// register limited search kernels (-gpuregs / -gpubench)
#ifdef RH2_FATBIN_R128_PATH
RH2_EMBED(rh2_fatbin_r128, RH2_FATBIN_R128_PATH);
#else
RH2_EMPTY(rh2_fatbin_r128);
#endif

#ifdef RH2_FATBIN_R64_PATH
RH2_EMBED(rh2_fatbin_r64, RH2_FATBIN_R64_PATH);
#else
RH2_EMPTY(rh2_fatbin_r64);
#endif
