/* Imhotep - the empire map's trade routes: their end cities and their drawn points (package play).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A route record (chunk 143, 50 records of 0x144 bytes at 0x00776FB0): +0 u8 the step in pixels
 * between drawn points, +4 the point buffer (points of 6 bytes: i16 x, i16 y, u8 segment), +8 the
 * route's corners (i16 x, i16 y at +8 + 6 k, k = 0 .. count), +0x13C / +0x13E the empire cities at its
 * first and last corner, +0x141 u8 the number of the last corner. The point buffer is computed with
 * x87 arithmetic for the empire map (tools/oracle/excluded.py: the tick never reads it); the port
 * reproduces it exactly (core/city/day_x87.h, core/sim/x87_float.h). */
#include "sim/x87_float.h"
#include "state/ported.h"

#define CITIES 0x0077F748u     /* empire cities, records of 0x62 bytes, to 0x007843D8 */
#define CITIES_END 0x007843D8u
#define CITY_SIZES 0x010D6A0Cu /* i16 width, i16 height of a city's picture, records of 64 bytes */
#define CITY_SIZE_COUNT 0x0126517Cu

uint8_t route_find_cities(game_state *st, uint32_t self)
{
    /* @bin 0x00407F90 the cities whose picture holds the route's first corner (+0x13C) and its last
     * one (+0x13E), -1 when none; 0 when the route has no corner past the first. A city counts when
     * it is in use (+1) and of kind 1 (+0); its picture (+4 x, +6 y) is the size of the entry +0xC
     * (u16) of 0x010D6A0C, entry 0 when past the count 0x0126517C (signed compare); bounds inclusive */
    S16(st, self + 0x13E) = -1;
    S16(st, self + 0x13C) = -1;
    uint8_t n = U8(st, self + 0x141);
    if (n == 0) {
        return 0;
    }
    int32_t k = 0;
    for (uint32_t c = CITIES; c < CITIES_END; c += 0x62, k++) {
        if (U8(st, c + 1) == 0 || U8(st, c) != 1) {
            continue;
        }
        int32_t count = S32(st, CITY_SIZE_COUNT);
        int32_t e = U16(st, c + 0xC);
        if (e >= count) {
            e = 0;
        }
        int32_t x0 = S16(st, c + 4), y0 = S16(st, c + 6);
        int32_t x1 = S16(st, CITY_SIZES + (uint32_t) e * 64u) + x0;
        int32_t y1 = S16(st, CITY_SIZES + 2u + (uint32_t) e * 64u) + y0;
        int32_t px = S16(st, self + 8);
        if (px >= x0 && px <= x1) {
            int32_t py = S16(st, self + 0xA);
            if (py >= y0 && py <= y1) {
                S16(st, self + 0x13C) = (int16_t) k;
            }
        }
        uint32_t last = self + (uint32_t) n * 6u;
        int32_t lx = S16(st, last + 2);
        if (lx >= x0 && lx <= x1) {
            int32_t ly = S16(st, last + 4);
            if (ly >= y0 && ly <= y1) {
                S16(st, self + 0x13E) = (int16_t) k;
            }
        }
    }
    return 1;
}

int32_t route_segment_points(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t index)
{
    /* @bin 0x00445D10 the points of the segment from corner a to corner b, every [self] pixels, into
     * the point buffer [self + 4] from point `index` on (x, y, segment a); returns their number.
     * x87 (53-bit precision): dist = (float) fsqrt(dx * dx + dy * dy); 0 when dist == 0.0f
     * (0x0056F480) or the step [self] is 0; the steps dx / dist and dy / dist stored as floats; for
     * k = 0, step, 2 step, ... while k <= dist: x = __ftol(k * stepx + x0), y = __ftol(k * stepy +
     * (float) y0) */
    uint32_t pa = self + (uint32_t) (a * 6), pb = self + (uint32_t) (b * 6);
    int32_t x0 = S16(st, pa + 8), y0 = S16(st, pa + 0xA);
    int32_t dx = S16(st, pb + 8) - x0;
    int32_t dy = S16(st, pb + 0xA) - y0;
    uint64_t s = (uint64_t) ((int64_t) dx * dx) + (uint64_t) ((int64_t) dy * dy); /* exact: below 2^35 */
    x87_val dist = x87_from_float(x87_to_float_bits(x87_sqrt_int(s)));
    x87_val zero = x87_from_int(0);
    if (x87_cmp(dist, zero) == 0) {
        return 0;
    }
    if (U8(st, self) == 0) {
        return 0;
    }
    x87_val stepy = x87_round_float(x87_divv(x87_from_int(dy), dist));
    x87_val stepx = x87_round_float(x87_divv(x87_from_int(dx), dist));
    int32_t count = 0;
    if (x87_cmp(zero, dist) > 0) {
        return 0;
    }
    x87_val fx = x87_from_int(x0);
    x87_val fy = x87_round_float(x87_from_int(y0)); /* fstp dword: exact for an i16 */
    x87_val k = zero;
    int32_t counter = 0;
    uint32_t off = (uint32_t) (index * 6);
    for (;;) {
        int32_t x = x87_ftol(x87_add(x87_mul(k, stepx), fx));
        P16(st, P32(st, self + 4) + off) = (uint16_t) x;
        int32_t y = x87_ftol(x87_add(x87_mul(k, stepy), fy));
        off += 6;
        P16(st, P32(st, self + 4) + off - 4) = (uint16_t) y;
        P8(st, P32(st, self + 4) + off - 2) = (uint8_t) a;
        counter += U8(st, self);
        count++;
        k = x87_from_int(counter);
        if (x87_cmp(k, dist) > 0) {
            break;
        }
    }
    return count;
}

static int32_t run_find_cities(game_state *st, const int32_t *a)
{
    return route_find_cities(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_segment_points(game_state *st, const int32_t *a)
{
    return route_segment_points(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
}

const ported_function route_points_ports[] = {
    {0x00407F90u, "route_find_cities", 0, 8, 0, run_find_cities, 0},
    {0x00445D10u, "route_segment_points", 3, 32, 0, run_segment_points, 0},
};
const int route_points_ports_count = sizeof(route_points_ports) / sizeof(route_points_ports[0]);
