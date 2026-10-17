# NVIDIA GPU support (RandomHash2 CUDA kernel)

Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com

## How it works

* `rh2/` is a portable C++ implementation of RandomHash2 (PIP-0036) that
  compiles both as normal host code and as CUDA device code. The 77 hash
  algorithms in `rh2/algos/` are generated from the CPU miner sources by
  `gpu-dev/port_algos.py`, so CPU and GPU always share the same algorithms.
* `rh2_kernel.cu` runs one RandomHash2 search per GPU thread in mining mode:
  every neighbour nonce that is fully evaluated as a side effect is reported
  too (the same trick the CPU miner uses, about 4.4 hashes per search).
* `build_kernel.sh` compiles the kernels into `bin/*.fatbin` for Maxwell
  (GTX 750/900, 800M/900M) and newer: sm_50 sm_60 sm_70 sm_75 sm_80 sm_86
  sm_89 sm_90 sm_100 sm_120, plus PTX for future GPUs. The fatbins are
  embedded into parnianminer and loaded at run time through the CUDA
  **driver** API (`MinersLib/Cuda/CudaDriver.*`).

Consequences:

* Building parnianminer needs **no CUDA toolkit** (gcc / mingw / zig as before).
* Running it needs only the **NVIDIA display driver**; no CUDA runtime DLLs.
* Without an NVIDIA driver the miner runs CPU-only as before.
* 64-bit builds only (NVIDIA has no 32-bit CUDA driver).
* Every GPU share is re-verified on the CPU before it is submitted.

Measured with `-gpubench` (default options `-gpuscan 2`):

| GPU | driver | H/s | first GPU version |
|---|---|---|---|
| RTX 4090 (Linux) | CUDA 13.2 | ~640,000 | 421,000 |
| GTX 850M laptop (Windows) | CUDA 8.0 | ~12,600 | 11,500 |

(the i7-4710HQ CPU of the same laptop: ~84 kH/s). Nonce scanning 2 was the
fastest on both; the register limits (`-gpuregs 128/64`) were slower and the
block size made no measurable difference.

## Usage

    parnianminer -gputest 60                    # check + 60 s benchmark, then exit
    parnianminer -gpubench 8                    # find the fastest kernel options (~6 min)
    parnianminer -gpudiag                       # step by step check, locates GPU faults
    parnianminer -s stratum.parniancoin.com:auto -su ACCOUNT.rig -pw x -gpu 0
    parnianminer ... -gpu 0,1 -cpu -cputhreads 4   # GPUs and CPU together

Options:

| option | values | meaning |
|---|---|---|
| `-gpuregs` | 0 (auto), 128, 64 | register limit of the kernel: fewer registers, more threads at once |
| `-gpuscan` | 2 (default), 3, 0 (off) | nonce scanning (PIP-0036): only nonces that end within 2 (3) rounds are completed; identical hashes, much less memory per thread |
| `-gpublock` | 32..256 (64) | CUDA threads per block |
| `-gpuoverlap` | 1 (default), 0 | queue the next kernel while the previous results are processed |
| `-gputhreads` | N (auto) | threads per launch |
| `-gpustack` | KB (16) | stack per GPU thread, 8 KB minimum measured |
| `-kernelactivewaiting` | | spin instead of sleeping while a kernel runs |

`-gpubench` runs every combination of `-gpuregs`, `-gpuscan` and `-gpublock`
(each in its own process, with a CPU check of the results) and prints the
fastest options for the GPU it runs on.

## Rebuilding the kernels

Only needed when `rh2_kernel.cu` or `rh2/` change:

    pip install nvidia-cuda-nvcc nvidia-cuda-runtime nvidia-cuda-crt nvidia-nvvm   # CUDA 13
    pip download --no-deps nvidia-cuda-nvcc-cu12==12.9.86 && unzip nvidia_cuda_nvcc_cu12-*.whl -d cu12
    pip download --no-deps nvidia-cuda-nvcc-cu11==11.8.89 && unzip nvidia_cuda_nvcc_cu11-*.whl -d cu11
    NV=$(python3 -c "import nvidia,os;print(os.path.join(nvidia.__path__[0],'cu13','bin'))")
    cudalib/build_kernel.sh $NV cu12/nvidia/cuda_nvcc/bin/ptxas cu11/nvidia/cuda_nvcc/bin/ptxas

nvcc of CUDA 13 compiles the kernel to PTX once; the CUDA 11.8 `ptxas` then
assembles it for every GPU up to sm_90 and the CUDA 12.9 `ptxas` for
Blackwell. Code assembled by a newer toolkit than the installed driver can
load but crash (found on the GTX 850M laptop with a CUDA 8.0 driver), so the
oldest assembler that supports a GPU is used.

## Laptops and Windows

Windows resets a GPU whose kernel runs longer than 2 seconds (TDR). The miner
adapts the launch size to stay below one second per launch. If a slow GPU still
resets, raise the limit: registry
`HKLM\System\CurrentControlSet\Control\GraphicsDrivers`, DWORD `TdrDelay` = 10,
then reboot.

Task Manager shows CUDA work under the `Compute_0` / `Cuda` engine, not `3D`.

## Verifying the portable core on the CPU

    gpu-dev/rh2_cpu_vectors.cpp   # writes reference hashes with the production CPU code
    gpu-dev/rh2_port_test.cpp     # checks rh2/ (the kernel code) against them, incl. nonce scanning

See the build lines at the top of each file.
