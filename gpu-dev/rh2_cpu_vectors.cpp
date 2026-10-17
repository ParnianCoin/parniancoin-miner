// Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
// GNU General Public License version 3, see the LICENSE file.
//
// Build (from the repository root):
//   g++ -O2 -std=c++14 -msse4.1 -pthread -DPARNIANMINER_COND_SSE4 -DRH_COMPILE_CPU_ONLY \
//       -DBOOST_TIMER_ENABLE_DEPRECATED -DBOOST_BIND_GLOBAL_PLACEHOLDERS -DBOOST_ALLOW_DEPRECATED_HEADERS \
//       -I. -Iparnianminer -Ijsoncpp-1.8.0/include gpu-dev/rh2_cpu_vectors.cpp gpu-dev/stubs.cpp \
//       MinersLib/Parnian/RandomHash_Cpu.cpp MinersLib/Algo/{blake,blake2b,blake2s,haval,sha0,sha1,sha2,sph_panama}.cpp \
//       -o rh2_cpu_vectors
//   ./rh2_cpu_vectors 60 100 > vectors.txt
// Reference vector generator: uses the production CPU RandomHash2 code.
// Output lines: <header236hex(with nonce)> <hash32hex>
#include "precomp.h"
#include "MinersLib/Parnian/RandomHash.h"
#include <cstdio>
#include <random>

int main(int argc, char** argv)
{
    U32 headers = argc > 1 ? atoi(argv[1]) : 20;
    U32 searches = argc > 2 ? atoi(argv[2]) : 50;
    std::mt19937 rng(12345);
    RandomHash_State* st = nullptr;
    RandomHash_CreateMany(&st, 1);
    RandomHashResult* res = new RandomHashResult;
    U64 totalSearch = 0, totalHashes = 0;
    for (U32 h = 0; h < headers; h++)
    {
        U8 header[ParnianHeaderSizeV5];
        for (U32 i = 0; i < ParnianHeaderSizeV5; i++) header[i] = (U8)rng();
        RandomHash_SetHeader(st, header, ParnianHeaderSizeV5, 0);
        for (U32 s = 0; s < searches; s++)
        {
            U32 start = rng();
            RandomHash_Search(st, *res, start);
            totalSearch++;
            for (U32 k = 0; k < res->count; k++)
            {
                totalHashes++;
                U8 hh[ParnianHeaderSizeV5];
                memcpy(hh, header, ParnianHeaderSizeV5);
                memcpy(hh + ParnianHeaderSizeV5 - 4, &res->nonces[k], 4);
                for (U32 i = 0; i < ParnianHeaderSizeV5; i++) printf("%02x", hh[i]);
                printf(" ");
                U8* hp = (U8*)res->hashes[k];
                for (U32 i = 0; i < 32; i++) printf("%02x", hp[i]);
                printf("\n");
            }
        }
    }
    fprintf(stderr, "searches=%llu hashes=%llu avg_hashes_per_search=%.3f\n",
        (unsigned long long)totalSearch, (unsigned long long)totalHashes, (double)totalHashes / totalSearch);
    return 0;
}
