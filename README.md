# parnianminer

CPU miner for the **ParnianCoin** RandomHash2 proof-of-work.
Supports pool (stratum) mining with **automatic port selection** and solo mining on a ParnianCoin node.
Runs on Windows (64/32 bit) and Linux x64.

Website: https://parniancoin.com

**Current version is 2.4.1**

## Quick start

Pool mining, automatic port (recommended):
```
parnianminer -cpu -cputhreads 4 -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x
```

Pool mining on a fixed port (the miner stays on this port only):
```
parnianminer -cpu -cputhreads 4 -s stratum.parniancoin.com:38008 -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x
```

Solo mining on a ParnianCoin node:
```
parnianminer -cpu -cputhreads 4 -s http://node.parniancoin.com:38009
```

* `-su` is your ParnianCoin account followed by a worker name of your choice (`rig1`, `rig2`, ...), so each computer shows separately on the pool.
* `-pw` is not checked by the pool; `x` is fine.
* Run `parnianminer -completelist` to see how many logical cores you have (good maximum for `-cputhreads`).

## Pool ports and the `auto` mode

| Port  | Starting difficulty | For hashrate        |
|-------|--------------------:|---------------------|
| 38008 | 0.000069850         | 10 kH/s             |
| 38018 | 0.000349250         | 50 kH/s             |
| 38028 | 0.001396980         | 200 kH/s            |
| 38038 | 0.003492460         | 500 kH/s            |
| 38048 | 0.010477380         | 1.5 MH/s            |
| 38058 | 0.020954760         | 3 MH/s              |
| 38068 | 0.034924600         | 5 MH/s or greater (fixed difficulty) |

Ports 38008 to 38058 use variable difficulty; the port only sets the starting point.
Port 38068 keeps a fixed difficulty.

With `:auto` the miner:
1. connects on port 38008,
2. measures its real hashrate for about one minute,
3. moves to the port that matches it: a port is entered when your hashrate reaches 95% of the port's hashrate,
4. re-checks every 5 minutes and leaves a port downward only when the hashrate falls below 85% of it
   (a change must be confirmed by two checks in a row, at most one switch per 10 minutes).

| Port  | Entered from | Left below |
|-------|-------------:|-----------:|
| 38008 | 0            | -          |
| 38018 | 47.5 kH/s    | 42.5 kH/s  |
| 38028 | 190 kH/s     | 170 kH/s   |
| 38038 | 475 kH/s     | 425 kH/s   |
| 38048 | 1.425 MH/s   | 1.275 MH/s |
| 38058 | 2.85 MH/s    | 2.55 MH/s  |
| 38068 | 4.75 MH/s    | 4.25 MH/s  |

The active port is written in the log, e.g. `Auto port: active port is 38018 - Variable difficulty (50 kH/s)` and at the end of every speed line (`Port 38018 (auto)`).
`auto` is also accepted for the failover pool (`-fo`). It is not available for solo mining.

## Config file

Running `parnianminer` with no option loads `config.txt`. Edit `s`, `su` and `cputhreads` in it,
or use `parnianminer -configfile myconfig.txt`.
All options are listed with `parnianminer -help`.

## Remote API

Disabled by default. Enable with `-apiport 7111`.
The API listens on 127.0.0.1 only; add `-apiremote` to open it to the network (trusted networks only).
Without `-apipw` the API is read-only. See Release/API.txt.

## Antivirus warnings

Many antivirus products flag **every** cryptocurrency miner as "CoinMiner / PUA / Riskware",
because malware often installs miners secretly. parnianminer does not hide itself,
does not install itself, and has no developer fee. Download it only from parniancoin.com and
compare the SHA-256 checksum published there.

## Developer fee
There is **no developer fee**. 100% of your hashrate mines to your own account.

## Copyright and licence
Copyright (C) 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
Released under the GNU General Public License v3, see LICENSE.

Based on rhminer (https://github.com/polyminer1/rhminer). Third-party components
(sph hash functions, BLAKE2, jsoncpp, OpenCL headers, ...) keep their own licenses
as stated in their source files.
