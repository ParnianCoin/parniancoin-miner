/**
 * RandomHash source code implementation
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

#include "precomp.h"
#include "BuildInfo.h"
#include "MinersLib/Global.h"
#include "MinersLib/GenericMinerClient.h"
#include "MinersLib/StratumClient.h"

struct ActiveClientsData
{
    std::shared_ptr<GenericMinerClient> client;
    StratumClientSptr            stratum;
};

class ClientManager
{
public:
    static ClientManager& I();
    enum eShutdownMode { eShutdownLite, eShutdownRestart, eShutdownFull };

    void Initialize();
    void Shutdown(eShutdownMode esmode);

    ActiveClientsData     ActiveClients;
};
