#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# ParnianCoin pool mining - automatic port selection
# Replace YOUR_PARNIAN_ACCOUNT with your ParnianCoin account and set -cputhreads
while true
do
    ./parnianminer -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x -cpu -cputhreads 4 -r 40
    sleep 5s
done
