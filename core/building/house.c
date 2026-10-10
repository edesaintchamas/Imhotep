/* Imhotep - daily passes over the houses (dispatcher steps of 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). */
#include "building/house.h"

#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


/* +0x00 state (1: in use), +0x05 house size (non-zero: a house) */
static int is_house(game_state *st, uint32_t b)
{
    return U8(st, b + BLD_STATE) == 1 && U8(st, b + BLD_HOUSE_SIZE) != 0;
}

void house_service_decay(game_state *st)
{
    /* @bin 0x00454E20 the counters at these offsets (bytes) decrease to 0, in this order */
    static const uint8_t counters[] = {
        BLD_ACCESS_JUGGLER, BLD_ACCESS_MUSICIAN, BLD_ACCESS_DANCER, BLD_ACCESS_SENET,
        BLD_ACCESS_ZOOKEEPER, BLD_ACCESS_MAGISTRATE, BLD_ACCESS_TEACHER, BLD_ACCESS_LIBRARIAN,
        BLD_ACCESS_DENTIST, BLD_ACCESS_HERBALIST, BLD_ACCESS_PHYSICIAN, BLD_ACCESS_WATER,
        BLD_ACCESS_EMBALMER, BLD_ACCESS_PRIEST, BLD_ACCESS_PRIEST + 1, BLD_ACCESS_PRIEST + 2,
        BLD_ACCESS_PRIEST + 3, BLD_ACCESS_PRIEST + 4};
    for (int32_t id = 1, n = S32(st, G_BUILDING_HIGHEST); id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (!is_house(st, b)) {
            continue;
        }
        for (unsigned i = 0; i < sizeof(counters); i++) {
            uint8_t v = U8(st, b + counters[i]);
            U8(st, b + counters[i]) = v > 1 ? (uint8_t) (v - 1) : 0;
        }
    }
}

void house_access_ratings(game_state *st)
{
    /* @bin 0x00454FA0 */
    for (int32_t id = 1, n = S32(st, G_BUILDING_HIGHEST); id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (!is_house(st, b)) {
            continue;
        }
        U8(st, b + BLD_EDUCATION) = 0;
        /* @bin 0x00454FE3 the mean of five city values 0x0078D5F0 .. 0x0078D600, divided by 5
         * again (signed divisions) */
        int32_t sum = S32(st, G_COVERAGE + 4 * 4) + S32(st, G_COVERAGE + 4 * 3) + S32(st, G_COVERAGE + 4 * 2) + S32(st, G_COVERAGE + 4 * 1) +
                      S32(st, G_COVERAGE);
        int32_t base = sum / 5 / 5;
        U8(st, b + BLD_HEALTH) = 0;
        U8(st, b + BLD_RELIGION) = 0;
        int8_t r = (int8_t) base;
        U8(st, b + BLD_ENTERTAINMENT) = (uint8_t) r;
        if (U8(st, b + BLD_ACCESS_JUGGLER) != 0) {
            r = (int8_t) (r + 10);
            U8(st, b + BLD_ENTERTAINMENT) = (uint8_t) r;
        }
        if (U8(st, b + BLD_ACCESS_MUSICIAN) != 0) {
            U8(st, b + BLD_ENTERTAINMENT) += 20;
        }
        if (U8(st, b + BLD_ACCESS_DANCER) != 0) {
            U8(st, b + BLD_ENTERTAINMENT) += 30;
        }
        if (U8(st, b + BLD_ACCESS_SENET) != 0) {
            U8(st, b + BLD_ENTERTAINMENT) += 40;
        }
        if (U8(st, b + BLD_ACCESS_ZOOKEEPER) != 0) { /* @bin 0x0045504C signed compare with 60 */
            if (S8(st, b + BLD_ENTERTAINMENT) > 0x3C) {
                U8(st, b + BLD_ENTERTAINMENT) = 100;
            } else {
                U8(st, b + BLD_ENTERTAINMENT) += 40;
            }
        }
        if (S8(st, b + BLD_ENTERTAINMENT) > 100) {
            U8(st, b + BLD_ENTERTAINMENT) = 100;
        }
        if (U8(st, b + BLD_ACCESS_TEACHER) != 0 || U8(st, b + BLD_ACCESS_LIBRARIAN) != 0) {
            U8(st, b + BLD_EDUCATION) = 1;
            if (U8(st, b + BLD_ACCESS_TEACHER) != 0 && U8(st, b + BLD_ACCESS_LIBRARIAN) != 0) {
                U8(st, b + BLD_EDUCATION) = 2;
            }
        }
        static const uint8_t count3[] = {BLD_ACCESS_PRIEST, BLD_ACCESS_PRIEST + 1, BLD_ACCESS_PRIEST + 2,
                                         BLD_ACCESS_PRIEST + 3, BLD_ACCESS_PRIEST + 4};
        for (unsigned i = 0; i < sizeof(count3); i++) {
            if (U8(st, b + count3[i]) != 0) {
                U8(st, b + BLD_RELIGION) += 1;
            }
        }
        if (U8(st, b + BLD_ACCESS_PHYSICIAN) != 0) {
            U8(st, b + BLD_HEALTH) += 1;
        }
        if (U8(st, b + BLD_ACCESS_EMBALMER) != 0) {
            U8(st, b + BLD_HEALTH) += 1;
        }
    }
}

static int32_t run_service_decay(game_state *st, const int32_t *a)
{
    (void) a;
    house_service_decay(st);
    return 0;
}

static int32_t run_access_ratings(game_state *st, const int32_t *a)
{
    (void) a;
    house_access_ratings(st);
    return 0;
}

const ported_function house_ports[] = {
    {0x00454E20u, "house_service_decay", 0, 0, 0, run_service_decay, 0},
    {0x00454FA0u, "house_access_ratings", 0, 0, 0, run_access_ratings, 0},
};
const int house_ports_count = sizeof(house_ports) / sizeof(house_ports[0]);
