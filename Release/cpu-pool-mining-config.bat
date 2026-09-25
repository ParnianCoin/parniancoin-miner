@echo off
REM parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
REM ParnianCoin mining with the settings of config.txt
:a
parnianminer.exe -configfile config.txt
timeout /t 5
goto a
