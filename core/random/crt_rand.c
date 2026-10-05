/* Imhotep - the C runtime's rand() as statically linked into Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "random/crt_rand.h"

#include "state/ported.h"

int32_t crt_rand(game_state *st)
{
    /* @bin 0x00562348 */
    uint32_t s = U32(st, CRT_RAND_STATE) * 0x343FDu + 0x269EC3u;
    U32(st, CRT_RAND_STATE) = s;
    return (int32_t) ((s >> 16) & 0x7FFF);
}

void crt_srand(game_state *st, uint32_t seed)
{
    U32(st, CRT_RAND_STATE) = seed; /* @bin 0x0056233E */
}

static int32_t run_rand(game_state *st, const int32_t *a)
{
    (void) a;
    return crt_rand(st);
}

static int32_t run_srand(game_state *st, const int32_t *a)
{
    crt_srand(st, (uint32_t) a[0]);
    return 0;
}

const ported_function random_ports[] = {
    {0x00562348u, "crt_rand", 0, 32, 0, run_rand, 0},
    {0x0056233Eu, "crt_srand", 1, 0, 0, run_srand, 0},
};
const int random_ports_count = sizeof(random_ports) / sizeof(random_ports[0]);
