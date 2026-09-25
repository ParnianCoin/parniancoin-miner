@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM ParnianCoin solo mining on a node (GPU build)
:a
parnianminer.exe -v 1 -r 20 -s http://node.parniancoin.com:38009 -gpu 0 -gputhreads 100
timeout /t 5
goto a
