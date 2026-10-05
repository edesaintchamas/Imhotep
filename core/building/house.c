/* Imhotep - daily passes over the houses (dispatcher steps of 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). */
#include "building/house.h"

#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u /* highest building id in use (loops run over 1 .. it) */

/* +0x00 state (1: in use), +0x05 house size (non-zero: a house) */
static int is_house(game_state *st, uint32_t b)
{
    return U8(st, b) == 1 && U8(st, b + 5) != 0;
}

void house_service_decay(game_state *st)
{
    /* @bin 0x00454E20 the counters at these offsets (bytes) decrease to 0, in this order */
    static const uint8_t counters[] = {0x6C, 0x6D, 0x6F, 0x71, 0x86, 0x70, 0x72, 0x73, 0x75,
                                       0x76, 0x6E, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D};
    for (int32_t id = 1, n = S32(st, BUILDING_HIGHEST); id <= n; id++) {
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
    for (int32_t id = 1, n = S32(st, BUILDING_HIGHEST); id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (!is_house(st, b)) {
            continue;
        }
        U8(st, b + 0x81) = 0;
        /* @bin 0x00454FE3 the mean of five city values 0x0078D5F0 .. 0x0078D600, divided by 5
         * again (signed divisions) */
        int32_t sum = S32(st, 0x0078D600u) + S32(st, 0x0078D5FCu) + S32(st, 0x0078D5F8u) + S32(st, 0x0078D5F4u) +
                      S32(st, 0x0078D5F0u);
        int32_t base = sum / 5 / 5;
        U8(st, b + 0x82) = 0;
        U8(st, b + 0x83) = 0;
        int8_t r = (int8_t) base;
        U8(st, b + 0x80) = (uint8_t) r;
        if (U8(st, b + 0x6C) != 0) {
            r = (int8_t) (r + 10);
            U8(st, b + 0x80) = (uint8_t) r;
        }
        if (U8(st, b + 0x6D) != 0) {
            U8(st, b + 0x80) += 20;
        }
        if (U8(st, b + 0x6F) != 0) {
            U8(st, b + 0x80) += 30;
        }
        if (U8(st, b + 0x71) != 0) {
            U8(st, b + 0x80) += 40;
        }
        if (U8(st, b + 0x86) != 0) { /* @bin 0x0045504C signed compare with 60 */
            if (S8(st, b + 0x80) > 0x3C) {
                U8(st, b + 0x80) = 100;
            } else {
                U8(st, b + 0x80) += 40;
            }
        }
        if (S8(st, b + 0x80) > 100) {
            U8(st, b + 0x80) = 100;
        }
        if (U8(st, b + 0x72) != 0 || U8(st, b + 0x73) != 0) {
            U8(st, b + 0x81) = 1;
            if (U8(st, b + 0x72) != 0 && U8(st, b + 0x73) != 0) {
                U8(st, b + 0x81) = 2;
            }
        }
        static const uint8_t count3[] = {0x79, 0x7A, 0x7B, 0x7C, 0x7D};
        for (unsigned i = 0; i < sizeof(count3); i++) {
            if (U8(st, b + count3[i]) != 0) {
                U8(st, b + 0x83) += 1;
            }
        }
        if (U8(st, b + 0x6E) != 0) {
            U8(st, b + 0x82) += 1;
        }
        if (U8(st, b + 0x78) != 0) {
            U8(st, b + 0x82) += 1;
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
