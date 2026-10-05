/* Imhotep - small arithmetic helpers of the game.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_SIM_MATH_H
#define IMHOTEP_SIM_MATH_H

#include <stdint.h>

/* @bin 0x004CDE40 value * percent / 100 (32-bit product, wrapping; signed division) */
static inline int32_t calc_percentage(int32_t value, int32_t percent)
{
    return (int32_t) ((uint32_t) value * (uint32_t) percent) / 100;
}

#endif
