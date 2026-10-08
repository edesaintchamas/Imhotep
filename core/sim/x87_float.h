/* Imhotep - single-precision stores of x87 values (fst / fstp dword), next to core/city/day_x87.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The x87 rounds a value stored to a float to 24 bits, nearest-even (@bin 0x005640EC the control
 * word the CRT start-up sets: 53-bit precision, round to nearest; core/city/day_x87.h); the register itself keeps its 53-bit value.
 * Integer reproduction: the bits of the float. Normal results only (the game's values: screen
 * coordinates, sizes, speeds); zero keeps its sign. */
#ifndef IMHOTEP_SIM_X87_FLOAT_H
#define IMHOTEP_SIM_X87_FLOAT_H

#include <stdint.h>

#include "city/day_x87.h"
#include "city/monument_rating_x87.h"

/* fst dword: the float nearest to x (ties to even), as its bits */
static inline uint32_t x87_to_float_bits(x87_val x)
{
    uint32_t sign = x.neg ? 0x80000000u : 0;
    if (x.m == 0) {
        return sign;
    }
    uint64_t m = x.m;
    int e = x.e;
    int n = x87_bits(m);
    if (n > 24) {
        int s = n - 24;
        uint64_t rem = m & ((1ull << s) - 1);
        uint64_t half = 1ull << (s - 1);
        m >>= s;
        e += s;
        if (rem > half || (rem == half && (m & 1))) {
            m++;
        }
        if (m >> 24) {
            m >>= 1;
            e++;
        }
    } else if (n < 24) {
        m <<= 24 - n;
        e -= 24 - n;
    }
    uint32_t biased = (uint32_t) (e + 23 + 127);
    return sign | (biased << 23) | (uint32_t) (m & 0x7FFFFFu);
}

/* fld dword of a float given by its bits, then fst dword: a float value rounded through x87 */
static inline x87_val x87_round_float(x87_val x)
{
    return x87_from_float(x87_to_float_bits(x));
}

#endif
