@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM Tests every GPU kernel configuration (about 6 minutes) and prints the fastest options
parnianminer.exe -gpubench 8 > gpubench.txt 2>&1
type gpubench.txt
echo.
echo The result was saved to gpubench.txt
pause
