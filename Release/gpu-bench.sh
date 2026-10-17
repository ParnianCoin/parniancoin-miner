#!/bin/bash
# parnianminer - Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com>
# Tests every GPU kernel configuration (about 6 minutes) and prints the fastest options
./parnianminer -gpubench 8 2>&1 | tee gpubench.txt
