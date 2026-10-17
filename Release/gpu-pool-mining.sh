#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# ParnianCoin pool mining on NVIDIA GPU 0 - automatic port selection
# Replace YOUR_PARNIAN_ACCOUNT with your ParnianCoin account.
# Use -gpu 0,1 for several GPUs; add -cpu -cputhreads N to also mine on the CPU.
while true
do
    ./parnianminer -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x -gpu 0 -r 40
    sleep 5s
done
