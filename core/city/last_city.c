/* Imhotep - city helpers left over by the earlier packages (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/last_city.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

void city_population_remove(game_state *st, int32_t player, int32_t amount)
{
    /* @bin 0x004E91E0 (player, amount) the city's +0x26F8 -= amount, 0x004E9400(player, amount),
     * then +0x24 := the sum of the 100 words at +0x25C4 (movsx), kept in +0x24 while summing;
     * +0x469C := that sum when greater (signed) */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + 0x26F8) = (int32_t) ((uint32_t) S32(st, c + 0x26F8) - (uint32_t) amount);
    GAME_CALL(st, 0x004E9400u, player, amount);
    S32(st, c + 0x24) = 0;
    for (uint32_t i = 0; i < 100; i++) {
        S32(st, c + 0x24) = (int32_t) ((uint32_t) S32(st, c + 0x24) + (uint32_t) (int32_t) S16(st, c + 0x25C4 + 2 * i));
    }
    int32_t sum = S32(st, c + 0x24);
    if (sum > S32(st, c + 0x469C)) {
        S32(st, c + 0x469C) = sum;
    }
}

static int32_t run_pop_remove(game_state *st, const int32_t *a)
{
    city_population_remove(st, a[0], a[1]);
    return 0;
}

const ported_function last_city_ports[] = {
    {0x004E91E0u, "city_population_remove", 2, 0, 0, run_pop_remove, 0},
};
const int last_city_ports_count = sizeof(last_city_ports) / sizeof(last_city_ports[0]);
