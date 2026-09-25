@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM ParnianCoin pool mining - automatic port selection
REM Replace YOUR_PARNIAN_ACCOUNT with your ParnianCoin account and set -cputhreads
:a
parnianminer.exe -v 1 -r 20 -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x -cpu -cputhreads 2
timeout /t 5
goto a
