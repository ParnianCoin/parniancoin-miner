/**
 * parnianminer code
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

///
/// @file
/// @copyright Amir Reza Zamani <amirrezazamani@gmail.com>
/// @author Amir Reza Zamani <amirrezazamani@gmail.com>
///
/// Automatic pool port selection ("-s stratum+tcp://host:auto").
/// Each ParnianCoin pool port has a starting difficulty tuned for a hashrate range.
/// In auto mode the miner starts on the lowest port, measures its real hashrate
/// and moves to the port that best matches it.

#pragma once

#include <string>
#include <cstring>

struct PoolPortTier
{
    const char*        port;
    unsigned long long hashrate;   // nominal hashrate of the tier, in H/s
    const char*        descr;
};

// Must stay in sync with the "Mining Ports" table of the ParnianCoin pool.
static const PoolPortTier g_poolPortTiers[] =
{
    { "38008",   10000ULL, "Variable difficulty (10 kH/s)" },
    { "38018",   50000ULL, "Variable difficulty (50 kH/s)" },
    { "38028",  200000ULL, "Variable difficulty (200 kH/s)" },
    { "38038",  500000ULL, "Variable difficulty (500 kH/s)" },
    { "38048", 1500000ULL, "Variable difficulty (1.5 MH/s)" },
    { "38058", 3000000ULL, "Variable difficulty (3 MH/s or greater)" },
};
static const int g_poolPortTierCount = (int)(sizeof(g_poolPortTiers) / sizeof(g_poolPortTiers[0]));

// Timing of the auto selection
static const unsigned long long AUTOPORT_WARMUP_MS   = 60ULL * 1000;       // measure at least 60s before first decision
static const unsigned long long AUTOPORT_EVAL_MS     = 5ULL * 60 * 1000;   // re-evaluate every 5 min
static const unsigned long long AUTOPORT_MIN_SWITCH_MS = 10ULL * 60 * 1000; // never switch more than once per 10 min
static const float              AUTOPORT_DOWN_MARGIN = 0.80f;               // go down only if below 80% of current tier

inline bool IsAutoPortKeyword(const std::string& port)
{
    if (port.length() != 4)
        return false;
    char p[5];
    for (int i = 0; i < 4; i++)
        p[i] = (char)tolower((unsigned char)port[i]);
    p[4] = 0;
    return strcmp(p, "auto") == 0;
}

inline int FindPoolPortTier(const std::string& port)
{
    for (int i = 0; i < g_poolPortTierCount; i++)
        if (port == g_poolPortTiers[i].port)
            return i;
    return -1;
}

// Highest tier whose nominal hashrate is <= the measured hashrate
inline int SelectPoolPortTier(double hashrate)
{
    int best = 0;
    for (int i = 0; i < g_poolPortTierCount; i++)
        if (hashrate >= (double)g_poolPortTiers[i].hashrate)
            best = i;
    return best;
}
