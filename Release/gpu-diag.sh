#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# GPU diagnostics: runs the kernel step by step and compares every result with the CPU
./parnianminer -gpudiag 2>&1 | tee gpudiag.txt
