#!/bin/bash
# Builds the RandomHash2 CUDA kernels into cudalib/bin/*.fatbin
#
# The fatbins are embedded into parnianminer at build time (no CUDA toolkit is
# needed to build or run the miner itself). Re-run this script only when the
# kernel sources (cudalib/rh2_kernel.cu, cudalib/rh2/*) change.
#
# Usage:
#   cudalib/build_kernel.sh <cuda13 bin dir> [cuda12 ptxas] [cuda11 ptxas]
#
#   <cuda13 bin dir>  directory with nvcc, ptxas and fatbinary of CUDA 13
#   [cuda12 ptxas]    ptxas of CUDA 12.8 or newer 12.x (recommended).
#                     Assembles Blackwell (sm_100/sm_120), and every GPU when
#                     no CUDA 11 ptxas is given (Maxwell, Pascal and Volta are
#                     no longer supported by CUDA 13).
#                     Without it only sm_75+ is built and NVIDIA driver r580+
#                     (CUDA 13) is required.
#   [cuda11 ptxas]    ptxas of CUDA 11.8 (recommended). Assembles every GPU up
#                     to sm_90. Code assembled by a toolkit newer than the
#                     installed driver can load but crash (found on a GTX 850M
#                     laptop with a CUDA 8.0 driver); CUDA 11.8 code works there.
#
# How: nvcc translates the kernel once to PTX, then ptxas assembles it for each
# GPU generation. The PTX header is lowered to the PTX ISA of the assembler
# (7.8 for CUDA 11.8, 8.8 for CUDA 12.8/12.9); the kernel uses plain integer
# instructions only.
#
# Output:
#   bin/rh2_kernel.fatbin       search + diagnostic kernels, compiler chosen registers
#   bin/rh2_kernel_r128.fatbin  search kernel limited to 128 registers (-gpuregs 128)
#   bin/rh2_kernel_r64.fatbin   search kernel limited to 64 registers  (-gpuregs 64)
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
CU13="${1:?usage: build_kernel.sh <cuda13 bin dir> [cuda12 ptxas] [cuda11 ptxas]}"
PTXAS12="${2:-}"
PTXAS11="${3:-}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "nvcc: $("$CU13/nvcc" --version | tail -1)"
mkdir -p "$HERE/bin"
JOBS="${RH2_JOBS:-2}"      # each ptxas needs ~1.5 GB of memory

# build_fatbin <output> <extra nvcc flags> <extra ptxas flags>
build_fatbin() {
    local OUTF="$1" NVFLAGS="$2" PXFLAGS="$3"
    local IMAGES=() PIDS=()
    rm -f "$TMP"/rh2*
    "$CU13/nvcc" -std=c++14 -O3 -ptx -arch=compute_75 $NVFLAGS -o "$TMP/rh2.ptx" "$HERE/rh2_kernel.cu"

    # assemble <ptxas> <ptx version> <arch>
    assemble() {
        local PTXAS="$1" VER="$2" a="$3"
        echo "  sm_$a  ($("$PTXAS" --version | tail -1 | sed 's/.*cuda_\([0-9.]*\).*/CUDA \1/')) $PXFLAGS"
        sed -e "s/^\.version .*/.version $VER/" -e "s/^\.target sm_.*/.target sm_$a/" \
            -e '/\.pragma "used_bytes_mask/d' "$TMP/rh2.ptx" > "$TMP/rh2_$a.ptx"
        "$PTXAS" -O3 $PXFLAGS -arch=sm_$a "$TMP/rh2_$a.ptx" -o "$TMP/rh2_$a.cubin" 2>"$TMP/rh2_$a.log" &
        PIDS+=($!)
        IMAGES+=("--image3=kind=elf,sm=$a,file=$TMP/rh2_$a.cubin")
        if [ ${#PIDS[@]} -ge $JOBS ]; then
            for p in "${PIDS[@]}"; do wait $p || { cat "$TMP"/*.log; echo "ptxas failed"; exit 1; }; done
            PIDS=()
        fi
    }

    # a cubin also runs on later GPUs of the same major generation (sm_50 -> 5.2, sm_60 -> 6.1)
    if [ -n "$PTXAS11" ]; then
        for a in 50 60 70 75 80 86 89 90; do assemble "$PTXAS11" 7.8 $a; done
        for a in 100 120; do assemble "${PTXAS12:-$CU13/ptxas}" 8.8 $a; done
    elif [ -n "$PTXAS12" ]; then
        for a in 50 60 70 75 80 86 89 90 100 120; do assemble "$PTXAS12" 8.8 $a; done
    else
        for a in 75 80 86 89 90 100 120; do assemble "$CU13/ptxas" "$(grep -m1 '^\.version' "$TMP/rh2.ptx" | cut -d' ' -f2)" $a; done
    fi
    for p in "${PIDS[@]}"; do wait $p || { cat "$TMP"/*.log; echo "ptxas failed"; exit 1; }; done
    # PTX for future GPUs (JIT compiled by the driver)
    IMAGES+=("--image3=kind=ptx,sm=75,file=$TMP/rh2_75.ptx")

    # LZ4 ("speed") compression: readable by older NVIDIA drivers too (zstd modes need r570+)
    "$CU13/fatbinary" --64 --compress-all --compress-mode=speed --create="$OUTF" "${IMAGES[@]}"
    ls -la "$OUTF"
}

# main kernel (+ diagnostics), compiler chosen registers
echo "== rh2_kernel.fatbin"
build_fatbin "$HERE/bin/rh2_kernel.fatbin" "" ""
# register limited variants for -gpuregs / -gpubench (search kernel only)
for R in ${RH2_REG_VARIANTS:-128 64}; do
    echo "== rh2_kernel_r$R.fatbin"
    build_fatbin "$HERE/bin/rh2_kernel_r$R.fatbin" "-DRH2_NO_DIAG" "-maxrregcount=$R"
done
