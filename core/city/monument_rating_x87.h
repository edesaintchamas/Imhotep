/* Imhotep - x87 square root for the monument rating (package month2), next to core/city/day_x87.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONUMENT_RATING_X87_H
#define IMHOTEP_CITY_MONUMENT_RATING_X87_H

#include <stdint.h>

#include "city/day_x87.h"

/* @bin 0x004F78A0 fsqrt at 53-bit precision, from floor(sqrt(n)) of a 128-bit integer and whether
 * the root is inexact */
static inline unsigned __int128 x87_isqrt128(unsigned __int128 n, int *inexact)
{
    unsigned __int128 r = 0, bit = ((unsigned __int128) 1) << 126;
    unsigned __int128 v = n;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    *inexact = v != 0;
    return r;
}

/* fsqrt of a non-negative integer s < 2^53 (x87 precision 53 bits: the root correctly rounded to
 * nearest-even). s * 4^k is given 111..112 bits, so its integer root has 56 bits: two bits below
 * the 53 kept and a sticky bit (the remainder) decide the rounding exactly as for the real root,
 * which is never halfway between two 53-bit values */
static inline x87_val x87_sqrt_int(uint64_t s)
{
    x87_val z = {0, 0, 0};
    if (s == 0) {
        return z;
    }
    int k = (112 - x87_bits(s)) / 2;
    int inexact;
    unsigned __int128 q = x87_isqrt128(((unsigned __int128) s) << (2 * k), &inexact);
    return x87_round(0, q, -k, inexact);
}

/* fld dword: the value of a float given by its bits (normal numbers and zero) */
static inline x87_val x87_from_float(uint32_t bits)
{
    x87_val r = {(int) (bits >> 31), 0, 0};
    uint32_t exp = (bits >> 23) & 0xFF;
    if (exp == 0) {
        return r;
    }
    r.m = (bits & 0x7FFFFFu) | 0x800000u;
    r.e = (int) exp - 150;
    return r;
}

#endif
