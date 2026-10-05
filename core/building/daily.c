/* Imhotep - daily passes over the buildings.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/daily.h"

#include "map/route_search.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define ENTRY_X 0x00784C8Cu /* i16 */
#define ENTRY_Y 0x00784C8Eu /* i16 */

void buildings_entry_road_check(game_state *st)
{
    /* @bin 0x00457A40 */
    route_distances_ed4140(st, S16(st, ENTRY_X), S16(st, ENTRY_Y));
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + 0x10);
        if (U8(st, b) != 1 || U8(st, b + 5) != 0 || (type != 0x4B && type != 0xC0)) {
            continue;
        }
        U8(st, b + 0x35) = route_road_reached(st, S16(st, b + 6), S16(st, b + 8), 3) ? 1 : 0;
    }
}

static int32_t run_entry_road_check(game_state *st, const int32_t *a)
{
    (void) a;
    buildings_entry_road_check(st);
    return 0;
}

const ported_function building_daily_ports[] = {
    {0x00457A40u, "buildings_entry_road_check", 0, 0, 0, run_entry_road_check, 0},
};
const int building_daily_ports_count = sizeof(building_daily_ports) / sizeof(building_daily_ports[0]);
