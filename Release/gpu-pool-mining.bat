@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM ParnianCoin pool mining (GPU build) - automatic port selection
:a
parnianminer.exe -v 1 -r 20 -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x -gpu 0 -gputhreads 100 -cpu -cputhreads 2
timeout /t 5
goto a
