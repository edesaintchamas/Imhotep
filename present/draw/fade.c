/* Imhotep - the screen-shake fade on the lock object 0x006DE518 (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * While the screen shakes (0x0078D7A0, every 0x88: core/frame/frame.c frame_present), the present
 * 0x004D0280 runs 0x004D05F0 and eight steps of 0x004D0650 on the lock object 0x006DE518 (filled by
 * 0x004D0480, core/frame/display.c): +0 steps left, +4 brightness delta, +8 width, +0x0C height,
 * +0x10 pixel format (1: RGB 565, 0: 555, -1: unknown), +0x14 mode, +0x18 random jitter (u8),
 * +0x24 / +0x28 the primary's / the back buffer's pixels. Each step draws one game random number
 * (a step of the game's generator 0x004D7090, written into the state like the tick's), so it is
 * ported whole here. The rows are walked width pixels apart (no pitch), from row 0x1E, left of the
 * sidebar's x (0x00F53F64, or 0x00F53F60 while 0x00F53F78 is set). */
#include "present_layout.h"
#include "random/random.h"
#include "state/ported.h"

int32_t fade_clamp_4d0630(int32_t v, int32_t lo, int32_t hi)
{
    /* @bin 0x004D0630 v below lo: lo; above hi: hi; else v (signed) */
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

void fade_step_4d0650(game_state *st, uint32_t self, int32_t surface)
{
    /* @bin 0x004D0650 (thiscall; surface 0 primary, 1 back buffer) one step while steps are left
     * and the format is known */
    if (S32(st, self) <= 0 || S32(st, self + 0x10) == -1) {
        return;
    }
    int32_t delta = S32(st, self + 4);
    S32(st, self) = S32(st, self) - 1;
    if (U8(st, self + 0x18) != 0) {
        /* @bin 0x004D0682 jitter: one random step; delta += (15-bit value % 16) - 8 */
        random_next_state(st);
        delta += (S32(st, G_RANDOM_R1_15BIT) % 16) - 8;
    }
    int32_t h = S32(st, self + 0x0C);
    uint32_t p = U32(st, self + 0x24 + 4u * (uint32_t) surface);
    int32_t w = S32(st, self + 8);
    int32_t limit = S32(st, G_SIDEBAR_COLLAPSED) != 0 ? S32(st, G_SIDEBAR_X_COLLAPSED) : S32(st, G_SIDEBAR_X);
    int32_t format = S32(st, self + 0x10);
    /* the primary's pixels (+0x24) are the platform's screen in the port (0x004D0480 leaves 0):
     * the random steps are kept, the pixels are not drawn */
    int draw = p != 0;
    if (S32(st, self + 0x14) == 0) {
        /* @bin 0x004D07A1 grey: v = clamp(((pixel >> 11) + (pixel & 0x1F)) / 2 + delta, 0, 31)
         * into the three channels (565: v | v << 6 | v << 11; 555: v | v << 5 | v << 10); another
         * format: nothing */
        if (format != 1 && format != 0) {
            return;
        }
        for (int32_t y = 0; y < h; y++) {
            for (int32_t x = 0; x < w; x++, p += 2) {
                if (x < limit && y >= 0x1E && draw) {
                    uint32_t px = (uint16_t) P16(st, p);
                    int32_t v = fade_clamp_4d0630((int32_t) (((px >> 11) + (px & 0x1F)) >> 1) + delta, 0, 0x1F);
                    uint32_t c = format == 1 ? ((((uint32_t) v << 5) | (uint32_t) v) << 6) | (uint32_t) v
                                             : ((((uint32_t) v << 5) | (uint32_t) v) << 5) | (uint32_t) v;
                    P16(st, p) = (int16_t) (uint16_t) c;
                }
            }
        }
        return;
    }
    /* @bin 0x004D0891 streaks: per row a random step; at (x + 15-bit value) % 128 == 0 a white
     * pixel (0xFFFF; 0x7FFF in 555), elsewhere the pixel halved (555: (p >> 1) & 0x3DEF | p &
     * 0x8000; 565: per channel) */
    for (int32_t y = 0; y < h; y++) {
        random_next_state(st);
        int32_t r = S32(st, G_RANDOM_R1_15BIT);
        for (int32_t x = 0; x < w; x++, p += 2) {
            if (x >= limit || y < 0x1E || !draw) {
                continue;
            }
            if ((x + r) % 128 == 0) {
                P16(st, p) = (int16_t) 0xFFFF;
                if (format == 0) {
                    P16(st, p) = 0x7FFF;
                }
                continue;
            }
            uint32_t px = (uint16_t) P16(st, p);
            uint32_t c;
            if (format == 0) {
                c = ((px >> 1) & 0x3DEFu) | (px & 0x8000u);
            } else {
                uint32_t v = (px & 0xFFE0u) | ((px >> 1) & 0xFu);
                v = (v & 0xF81Fu) | ((v >> 1) & 0x3E0u);
                c = ((v >> 1) & 0x7800u) | (v & 0x7FFu);
            }
            P16(st, p) = (int16_t) (uint16_t) c;
        }
    }
}

void fade_start_4d05f0(game_state *st, uint32_t self, int32_t surface, int32_t steps, int32_t delta, uint8_t jitter,
                       int32_t mode)
{
    /* @bin 0x004D05F0 (thiscall; surface, steps, delta, jitter, mode) +0 := steps clamped to 0 ..
     * 15 (0x004D0630), +4 := delta, +0x18 := jitter, +0x14 := mode; then the first step */
    S32(st, self + 4) = delta;
    S32(st, self) = fade_clamp_4d0630(steps, 0, 0x0F);
    U8(st, self + 0x18) = jitter;
    S32(st, self + 0x14) = mode;
    fade_step_4d0650(st, self, surface);
}

static uint32_t self_of(const int32_t *a)
{
    return a[PORT_THIS_ARG] ? (uint32_t) a[PORT_THIS_ARG] : G_SURFACE_LOCK;
}

static int32_t run_clamp(game_state *st, const int32_t *a)
{
    (void) st;
    return fade_clamp_4d0630(a[0], a[1], a[2]);
}

static int32_t run_step(game_state *st, const int32_t *a)
{
    fade_step_4d0650(st, self_of(a), a[0]);
    return 0;
}

static int32_t run_start(game_state *st, const int32_t *a)
{
    fade_start_4d05f0(st, self_of(a), a[0], a[1], a[2], (uint8_t) a[3], a[4]);
    return 0;
}

const ported_function fade_ports[] = {
    {0x004D0630u, "fade_clamp_4d0630", 3, 32, 0, run_clamp, 0},
    {0x004D0650u, "fade_step_4d0650", 1, 0, G_SURFACE_LOCK, run_step, 0},
    {0x004D05F0u, "fade_start_4d05f0", 5, 0, G_SURFACE_LOCK, run_start, 0},
};
const int fade_ports_count = sizeof(fade_ports) / sizeof(fade_ports[0]);
