/* Imhotep - the daily step of the moving scenario events (package day2): 0x0044BE30, called by
 * 0x0044BCA0 (day_basic.c) on every event record of type 3.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_events.h"

#include "city/month_events.h"
#include "city/month_message.h"
#include "sim/call.h"
#include "state/ported.h"

#define POPUP 0x00E950DCu
#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define TERRAIN_GRID 0x00AF598Cu

int32_t event_tile_free(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0044BD20 terrain bits 0x206 clear at (x, y) */
    uint32_t o = (uint32_t) (S32(st, MAP_START) + y * 0xE4 + x);
    return (U32(st, TERRAIN_GRID + 4u * o) & 0x206u) == 0;
}

void event_tile_hit(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0044BD50 the tile (x, y) is wiped: its building collapses (0x0046D100, then its
     * linked parts, 0x004695D0, and sound 0x11), what is left there gets state 5, the terrain
     * becomes 0; then the terrain and routing passes */
    int32_t t = S32(st, MAP_START) + y * 0xE4 + x;
    int32_t b = S16(st, 0x00B4E6FCu + 2u * (uint32_t) t);
    if (b != 0) {
        GAME_CALL(st, 0x0046D100u, b, 0);
        GAME_CALL(st, 0x004695D0u, b, 1);
        GAME_CALL(st, 0x0053B090u, 0x11);
    }
    b = S16(st, 0x00B4E6FCu + 2u * (uint32_t) t);
    if (b != 0) {
        U8(st, 0x007999D8u + 0x108u * (uint32_t) b) = 5;
    }
    U32(st, TERRAIN_GRID + 4u * (uint32_t) t) = 0;
    GAME_CALL(st, 0x004798E0u, x, y, 1);
    GAME_CALL0(st, 0x00476530u);
    GAME_CALL0(st, 0x0047B570u);
    GAME_CALL(st, 0x00475720u, 0, 0, S32(st, MAP_WIDTH) - 1, S32(st, MAP_HEIGHT) - 1, 1);
    GAME_CALL(st, 0x00516F90u, x - 1, y - 1, x + 1, y + 1, 1);
    GAME_CALL(st, 0x005173A0u, x - 1, y - 1, x + 1, y + 1);
    GAME_CALL0(st, 0x005178E0u);
    GAME_CALL(st, 0x0048C0E0u, x, y, 1, 1);
}

int32_t event_move_step(game_state *st, uint32_t e)
{
    /* @bin 0x0044BE30 (thiscall) +0x30 0 waiting, 1 moving, 2 done */
    if (U8(st, e + 2) != 3) {
        return 0;
    }
    if (S16(st, e + 0x30) == 0) {
        if ((uint8_t) event_due(st, e, 0) == 0) {
            return 0;
        }
        /* @bin 0x0044BE6C it starts: the first point's tile is hit, message 0x105 there */
        U8(st, e + 0x32) = 1;
        U32(st, e + 0x28) |= 4;
        S16(st, e + 0x30) = 1;
        S16(st, e + 0x3A) = 0;
        S16(st, e + 0x3E) = 0;
        event_tile_hit(st, S16(st, e + 0x40), S16(st, e + 0x42));
        int32_t tile = S32(st, MAP_START) + S16(st, e + 0x40) + S16(st, e + 0x42) * 0xE4;
        S32(st, POPUP) = 1;
        /* @bin 0x0044BEAD the block: the frame's 0x20 bytes; +0x00..+0x1D are all written (here
         * and by 0x0044E6F0), and message_post copies only those 30 bytes */
        uint8_t blk[32] = {0};
        blk[2] = 0xD7;
        blk[0] = 0xD8;
        int32_t kind = event_message_kind(st, e);
        blk[0x10] = (uint8_t) kind;
        blk[0x11] = (uint8_t) (kind >> 8);
        event_fill_block(st, e, blk);
        message_post(st, 0x105, 0, tile, blk, 0);
        return 1;
    }
    if (S16(st, e + 0x30) == 2) {
        return 0;
    }
    S16(st, e + 0x3E) = (int16_t) (S16(st, e + 0x3E) + 1);
    if (S16(st, e + 0x3E) < S16(st, e + 0x3C)) {
        return 1;
    }
    S16(st, e + 0x3E) = 0;
    int32_t steps = S16(st, e + 0xC) * 25; /* @bin 0x0044BF31 at least 25 */
    if (steps < 0x19) {
        steps = 0x19;
    }
    S16(st, e + 0x3A) = (int16_t) (S16(st, e + 0x3A) + 1);
    if (S16(st, e + 0x3A) >= steps) {
        S16(st, e + 0x30) = 2;
        U8(st, e + 0x32) = 0;
    }
    /* @bin 0x0044BF55 the direction (low 4 bits of 0x00D3B374): which of the four points (+0x40
     * + 4 n) moves, and by how much */
    uint16_t d = (uint16_t) (U32(st, 0x00D3B374u) & 0xF);
    S16(st, e + 0x50) = (int16_t) d;
    int n, dx, dy;
    switch (d) {
    case 0: case 4: n = 0; dx = 0; dy = -1; break;
    case 5: n = 0; dx = -1; dy = 0; break;
    case 6: n = 0; dx = 1; dy = 0; break;
    case 1: case 7: n = 1; dx = 1; dy = 0; break;
    case 8: n = 1; dx = 0; dy = -1; break;
    case 9: n = 1; dx = 0; dy = 1; break;
    case 2: case 10: n = 2; dx = 0; dy = 1; break;
    case 11: n = 2; dx = -1; dy = 0; break;
    case 12: n = 2; dx = 1; dy = 0; break;
    case 3: case 13: n = 3; dx = -1; dy = 0; break;
    case 14: n = 3; dx = 0; dy = -1; break;
    default: n = 3; dx = 0; dy = 1; break; /* 15 */
    }
    uint32_t p = e + 0x40 + 4u * (uint32_t) n;
    int32_t x = S16(st, p) + dx, y = S16(st, p + 2) + dy;
    if (x < 0) {
        x = 0;
    }
    if (x > S32(st, MAP_WIDTH) - 1) {
        x = S32(st, MAP_WIDTH) - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > S32(st, MAP_HEIGHT) - 1) {
        y = S32(st, MAP_HEIGHT) - 1;
    }
    if (event_tile_free(st, x, y) != 0) {
        S16(st, p) = (int16_t) x;
        S16(st, p + 2) = (int16_t) y;
        event_tile_hit(st, x, y);
    }
    return 1;
}

static int32_t run_tile_free(game_state *st, const int32_t *a)
{
    return event_tile_free(st, a[0], a[1]);
}

static int32_t run_move_step(game_state *st, const int32_t *a)
{
    return event_move_step(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_tile_hit(game_state *st, const int32_t *a)
{
    event_tile_hit(st, a[0], a[1]);
    return 0;
}

const ported_function day2_events_ports[] = {
    {0x0044BD50u, "event_tile_hit", 2, 0, 0, run_tile_hit, 0},
    {0x0044BD20u, "event_tile_free", 2, 32, 0, run_tile_free, 0},
    {0x0044BE30u, "event_move_step", 0, 8, 0, run_move_step, 0},
};
const int day2_events_ports_count = sizeof(day2_events_ports) / sizeof(day2_events_ports[0]);
