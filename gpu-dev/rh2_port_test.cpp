// Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
// GNU General Public License version 3, see the LICENSE file.
//
// Build and run (from the repository root):
//   g++ -O2 -std=c++14 -I. gpu-dev/rh2_port_test.cpp -o rh2_port_test
//   ./rh2_port_test vectors.txt
// Host-side verification of the portable RandomHash2 core (cudalib/rh2)
// against reference vectors produced by the production CPU miner code.
//
//   rh2_port_test <vectors.txt> [maxLines]
//
// Each vector line: <236-byte header hex, nonce in last 4 bytes> <32-byte hash hex>
#include "cudalib/rh2/rh2_core.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool fromhex(const char* s, U8* out, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        int a = hexval(s[2 * i]), b = hexval(s[2 * i + 1]);
        if (a < 0 || b < 0) return false;
        out[i] = (U8)(a * 16 + b);
    }
    return true;
}

static void make_header(const U8* header, RH2_Header& h)
{
    for (int i = 0; i < 8; i++) h.midstate[i] = rh2_sha256_IV[i];
    for (int b = 0; b < 3; b++) rh2_sha256_block(h.midstate, header + 64 * b);
    memcpy(h.tail, header + 192, sizeof(h.tail));
}

struct Collect
{
    struct R { U32 nonce; U8 hash[32]; };
    std::vector<R> results;
    void Report(U32 nonce, const U8* hash)
    {
        R r; r.nonce = nonce; memcpy(r.hash, hash, 32);
        results.push_back(r);
    }
};

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s vectors.txt [maxLines]\n", argv[0]); return 2; }
    size_t maxLines = argc > 2 ? (size_t)atol(argv[2]) : (size_t)-1;
    FILE* f = fopen(argv[1], "r");
    if (!f) { perror("open"); return 2; }

    RH2_Thread* t = new RH2_Thread;
    std::vector<U32> arena(RH2_ARENA_DEFAULT / 4);
    t->arena = arena.data();
    t->arenaWords = (U32)arena.size();

    char line[1024];
    size_t n = 0, ok = 0, bad = 0, freeChecked = 0, freeBad = 0, overflow = 0;
    U32 maxUsed = 0, maxEntries = 0;
    auto t0 = std::chrono::steady_clock::now();
    while (n < maxLines && fgets(line, sizeof(line), f))
    {
        U8 header[RH2_HEADER_SIZE], expect[32];
        if (strlen(line) < RH2_HEADER_SIZE * 2 + 1 + 64) continue;
        if (!fromhex(line, header, RH2_HEADER_SIZE) || !fromhex(line + RH2_HEADER_SIZE * 2 + 1, expect, 32))
            continue;
        n++;
        RH2_Header h;
        make_header(header, h);
        U32 nonce;
        memcpy(&nonce, header + RH2_HEADER_SIZE - 4, 4);

        Collect c;
        if (rh2_search(t, &h, nonce, c) != RH2_SEARCH_OK) { overflow++; continue; }
        if (t->arenaUsed > maxUsed) maxUsed = t->arenaUsed;
        if (t->entryCount > maxEntries) maxEntries = t->entryCount;

        const Collect::R& root = c.results.back();
        if (root.nonce == nonce && memcmp(root.hash, expect, 32) == 0)
            ok++;
        else
        {
            bad++;
            if (bad <= 5)
            {
                printf("MISMATCH line %zu nonce %08x\n  got ", n, nonce);
                for (int i = 0; i < 32; i++) printf("%02x", root.hash[i]);
                printf("\n  exp ");
                for (int i = 0; i < 32; i++) printf("%02x", expect[i]);
                printf("\n");
            }
        }

        // every free neighbour hash must equal a direct evaluation of that nonce
        if ((n % 25) == 0)
        {
            for (size_t k = 0; k + 1 < c.results.size(); k++)
            {
                Collect c2;
                rh2_search(t, &h, c.results[k].nonce, c2);
                freeChecked++;
                if (memcmp(c2.results.back().hash, c.results[k].hash, 32) != 0)
                    freeBad++;
            }
        }
    }
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    // nonce scanning (topRound 2 and 3): every reported hash must equal the
    // full evaluation of that nonce
    size_t scanBad = 0;
    for (U32 top = 2; top <= RH2_MAX_N; top++)
    {
        rewind(f);
        size_t lines = 0, done = 0, reported = 0;
        while (lines < 2000 && fgets(line, sizeof(line), f))
        {
            U8 header[RH2_HEADER_SIZE], expect[32];
            if (strlen(line) < RH2_HEADER_SIZE * 2 + 1 + 64) continue;
            if (!fromhex(line, header, RH2_HEADER_SIZE) || !fromhex(line + RH2_HEADER_SIZE * 2 + 1, expect, 32))
                continue;
            lines++;
            RH2_Header h;
            make_header(header, h);
            U32 nonce;
            memcpy(&nonce, header + RH2_HEADER_SIZE - 4, 4);
            Collect c;
            int r = rh2_search(t, &h, nonce, c, top);
            if (r == RH2_SEARCH_OK)
            {
                done++;
                if (memcmp(c.results.back().hash, expect, 32) != 0)
                    scanBad++;
            }
            reported += c.results.size();
            if ((lines % 50) == 0)
                for (auto& x : c.results)
                {
                    Collect c2;
                    rh2_search(t, &h, x.nonce, c2);
                    if (memcmp(c2.results.back().hash, x.hash, 32) != 0)
                        scanBad++;
                }
        }
        printf("topRound %u: %zu searches, %zu completed, %zu hashes\n", top, lines, done, reported);
    }
    printf("nonce scanning check: %zu wrong\n", scanBad);
    if (scanBad)
        return 1;
    printf("vectors=%zu ok=%zu bad=%zu overflow=%zu | free-hash checks=%zu bad=%zu | max arena=%u bytes, max entries=%u | %.2fs\n",
           n, ok, bad, overflow, freeChecked, freeBad, maxUsed * 4, maxEntries, secs);
    return (bad || freeBad || overflow) ? 1 : 0;
}
