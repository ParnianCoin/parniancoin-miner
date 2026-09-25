#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# ParnianCoin solo mining on a node
while true
do
    ./parnianminer -s http://node.parniancoin.com:38009 -cpu -cputhreads 4 -r 40
    sleep 5s
done
