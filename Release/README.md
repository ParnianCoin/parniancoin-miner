# parnianminer 2.4 binaries

Prebuilt binaries of the ParnianCoin RandomHash2 CPU miner.
Download: https://parniancoin.com

| Package                               | Platform      |
|---------------------------------------|---------------|
| parnianminer-2.4-windows-x64.zip      | Windows 64 bit|
| parnianminer-2.4-windows-x86.zip      | Windows 32 bit|
| parnianminer-2.4-linux-x64.tar.gz     | Linux x64, any distribution (glibc 2.17+: Ubuntu 16.04+, Debian 9+, CentOS 7+) |
| parnianminer-2.4-linux-x64-modern.tar.gz | Linux x64, about 7% faster, needs glibc 2.38+ (Ubuntu 24.04+, Debian 13+) |

No package needs to be installed: jsoncpp, boost and the C++ runtime are built into the executable.

Always check the SHA-256 checksum published on parniancoin.com before running the miner.

Quick start (pool, automatic port):
```
parnianminer -cpu -cputhreads 4 -s stratum.parniancoin.com:auto -su YOUR_PARNIAN_ACCOUNT.rig1 -pw x
```
Solo mining:
```
parnianminer -cpu -cputhreads 4 -s http://node.parniancoin.com:38009
```
Run `parnianminer -help` for all options.
