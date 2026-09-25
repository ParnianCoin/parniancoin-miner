#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# ParnianCoin mining with the settings of config.txt
while true
do
    ./parnianminer -configfile config.txt
    sleep 5s
done
