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

#include "precomp.h"
#include "sph_sha2.h"
#include "MinersLib/Parnian/ParnianCommon.h"

void ParnianHashV3(void *state, const void *input)
{
  sph_sha256_context ctx_sha;
  uint32_t hash[16];

  sph_sha256_init(&ctx_sha);
  sph_sha256(&ctx_sha, input, ParnianHeaderSize);
  sph_sha256_close(&ctx_sha, hash);

  sph_sha256_init(&ctx_sha);
  sph_sha256(&ctx_sha, hash, 32);
  sph_sha256_close(&ctx_sha, hash);

  memcpy(state, hash, 32);
}
