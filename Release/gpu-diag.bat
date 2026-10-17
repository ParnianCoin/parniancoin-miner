@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM GPU diagnostics: runs the kernel step by step and compares every result with the CPU
parnianminer.exe -gpudiag > gpudiag.txt 2>&1
type gpudiag.txt
echo.
echo The result was saved to gpudiag.txt
pause
