/* Imhotep - the boot: WinMain's table files and later steps (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * trade_recommends.txt (0x00541310) and "auto reason phrases.txt" (0x0044EFA0) are read into the
 * spare screen [0x00E92E1C] (memory outside the state, the boot's host) and parsed with the game's text
 * helpers (0x004CBE30, 0x004CC0E0, 0x004CC070: core/command/models.c), which read it through
 * state_ptr. */
#include "boot/boot_internal.h"
#include "boot/monuments.h"
#include "building/rest_redraw.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define SPARE 0x00E92E1Cu

static int32_t find_char(game_state *st, uint32_t p, int32_t c, int32_t n)
{
    return GAME_CALL(st, 0x004CBE30u, (int32_t) p, c, n);
}

/* @bin 0x004CC0E0 then 0x004CC070: the next number from p; p moves to it */
static int32_t next_number(game_state *st, uint32_t *p)
{
    *p += (uint32_t) GAME_CALL(st, 0x004CC0E0u, (int32_t) *p);
    return GAME_CALL(st, 0x004CC070u, (int32_t) *p);
}

/* the file `name` (a string of the state) into the cleared spare screen; its size, or -1 (logged:
 * `missing`) */
static int32_t read_table(game_state *st, uint32_t name, uint32_t missing)
{
    GAME_CALL(st, 0x0040F8F0u, (int32_t) U32(st, SPARE), 100000, 0);
    if (GAME_CALL(st, 0x004CEB60u, (int32_t) name, (int32_t) U32(st, SPARE), 0) == 0) {
        platform_log(st, missing, 0, 0);
        return -1;
    }
    return GAME_CALL(st, 0x004CED50u, (int32_t) name);
}

int32_t boot_541310(game_state *st, const boot_host *host)
{
    (void) host;
    /* @bin 0x00541310 trade_recommends.txt (0x00601DC0): 4 records of 6 numbers into 0x007758A0 */
    if (U32(st, SPARE) == 0) {
        platform_log(st, 0x00601BA8u, 0, 0);
        return 0;
    }
    int32_t size = read_table(st, 0x00601DC0u, 0x00601D9Cu);
    if (size < 0) {
        return 0;
    }
    /* @bin 0x0054139D the '{' counted (at most 0x12B) */
    uint32_t p = U32(st, SPARE);
    int32_t count = 0;
    for (int32_t left = 0x12B; left > 0; left--) {
        int32_t k = find_char(st, p, 0x7B, size);
        if (k == 0) {
            break;
        }
        count++;
        p += (uint32_t) k;
    }
    if (count != 4) {
        /* @bin 0x005413BA a Windows message box asks whether to go on (MessageBoxA, yes / no): the
         * game's files have 4 records; this path needs the platform's question */
        state_unported(st, 0x005413FAu);
        return 0;
    }
    p = U32(st, SPARE);
    uint32_t out = 0x007758A0u;
    for (int32_t left = 0x12B; left > 0; left--) {
        if (out >= 0x00775900u) { /* signed compare of the addresses: the same here */
            break;
        }
        p += (uint32_t) find_char(st, p, 0x7B, size);
        for (int i = 0; i < 6; i++) {
            S32(st, out) = next_number(st, &p);
            p += (uint32_t) find_char(st, p, 0x2C, size);
            out += 4;
        }
    }
    platform_log(st, 0x00601D10u, 0, 0);
    return 1;
}

int32_t boot_44efa0(game_state *st, const boot_host *host)
{
    (void) host;
    /* @bin 0x0044EFA0 "auto reason phrases.txt" (0x005DA280): 0x36 records of 0x24 bytes into
     * 0x007854A8 (up to 0x00785C40) */
    if (U32(st, SPARE) == 0) {
        platform_log(st, 0x005DA298u, 0, 0);
        return 0;
    }
    int32_t size = read_table(st, 0x005DA280u, 0x005DA260u);
    if (size < 0) {
        return 0;
    }
    uint32_t p = U32(st, SPARE);
    int32_t count = 0;
    int32_t k = find_char(st, p, 0x7B, size);
    int ok = 0;
    if (k != 0) {
        for (;;) {
            count++;
            if (count > 0x36) {
                break;
            }
            p += (uint32_t) k;
            k = find_char(st, p, 0x7B, size);
            if (k == 0) {
                ok = count == 0x36;
                break;
            }
        }
    }
    if (!ok) {
        state_unported(st, 0x0044F08Au); /* MessageBoxA: the platform's question (see 0x00541310) */
        return 0;
    }
    p = U32(st, SPARE);
    for (uint32_t out = 0x007854A8u; out < 0x00785C40u; out += 0x24) {
        p += (uint32_t) find_char(st, p, 0x7B, size);
        for (uint32_t i = 0; i < 0x24; i++) {
            U8(st, out + i) = (uint8_t) next_number(st, &p);
            p += (uint32_t) find_char(st, p, 0x2C, size);
        }
    }
    platform_log(st, 0x005DA1CCu, 0, 0);
    return 1;
}

void boot_541d50(game_state *st)
{
    /* @bin 0x00541D50 the parts of the table 0x00601DD8 (100 records of 0x34 bytes) laid out as
     * 0x004E5190 does */
    for (uint32_t r = 0x00601DD8u; r < 0x00603208u; r += 0x34) {
        monument_part_layout(st, r);
    }
}

void boot_4dee10(game_state *st)
{
    /* @bin 0x004DEE10 116 records of 0x34 bytes at 0x005E2AE8: for part k, the place of the part in
     * the four orientations 0, 2, 4, 6 (0x004DED50(out, k, d, d): out[0] x, out[1] y) as offsets, with
     * h1 = [0x0070E45C] + 1 and l = [0x0070E44C] (32-bit products) */
    uint32_t h1 = U32(st, 0x0070E45Cu) + 1u, l = U32(st, 0x0070E44Cu);
    int32_t k = 0;
    for (uint32_t r = 0x005E2AE8u; r < 0x005E4278u; r += 0x34, k++) {
        int32_t out[13];
        uint32_t x, y;
        monument_part_layout_4ded50(st, out, k, 0, 0);
        x = (uint32_t) out[0];
        y = (uint32_t) out[1];
        U32(st, r) = x + 228u * y;
        U32(st, r + 4) = (x - y) * h1;
        U32(st, r + 0x14) = (y + x) * l;
        monument_part_layout_4ded50(st, out, k, 2, 2);
        x = (uint32_t) out[0];
        y = (uint32_t) out[1];
        U32(st, r + 8) = (y + x) * h1;
        U32(st, r + 0x18) = (y - x) * l;
        monument_part_layout_4ded50(st, out, k, 4, 4);
        x = (uint32_t) out[0];
        y = (uint32_t) out[1];
        U32(st, r + 0xC) = (y - x) * h1;
        U32(st, r + 0x1C) = 0u - (y + x) * l;
        monument_part_layout_4ded50(st, out, k, 6, 6);
        x = (uint32_t) out[0];
        y = (uint32_t) out[1];
        U32(st, r + 0x10) = 0u - (y + x) * h1;
        U32(st, r + 0x20) = (x - y) * l;
    }
}

void boot_4cf8e0(game_state *st)
{
    /* @bin 0x004CF8E0 a timing of the machine: three calls of 0x0051F780 (it returns at once) and three
     * presents (0x004D0280), each group timed with timeGetTime (the platform's clock) */
    uint32_t t = platform_clock_ms(st);
    for (int i = 0; i < 3; i++) {
        int32_t r = S32(st, 0x00E38E68u);
        if (r == 1 || r == 2 || r == 3) {
            GAME_CALL(st, 0x0051F780u, r == 1 ? 0x14 : r == 2 ? 0x1E : 0x28);
        }
    }
    S32(st, 0x00E390FCu) = (int32_t) (platform_clock_ms(st) - t);
    t = platform_clock_ms(st);
    for (int i = 0; i < 3; i++) {
        GAME_CALL(st, 0x004D0280u, 1);
    }
    S32(st, 0x00E390F4u) = (int32_t) (platform_clock_ms(st) - t);
    S32(st, 0x00E39104u) = S32(st, 0x00E390F4u) + S32(st, 0x00E390FCu);
}

static int32_t run_541310(game_state *st, const int32_t *a)
{
    (void) a;
    return boot_541310(st, NULL);
}

static int32_t run_44efa0(game_state *st, const int32_t *a)
{
    (void) a;
    return boot_44efa0(st, NULL);
}

static int32_t run_541d50(game_state *st, const int32_t *a)
{
    (void) a;
    boot_541d50(st);
    return 0;
}

static int32_t run_4dee10(game_state *st, const int32_t *a)
{
    (void) a;
    boot_4dee10(st);
    return 0;
}

static int32_t run_4cf8e0(game_state *st, const int32_t *a)
{
    (void) a;
    boot_4cf8e0(st);
    return 0;
}

const ported_function boot_step_ports[] = {
    {0x00541310u, "trade_recommends_read", 0, 32, 0, run_541310, 0},
    {0x0044EFA0u, "reason_phrases_read", 0, 8, 0, run_44efa0, 0},
    {0x00541D50u, "boot_541d50", 0, 0, 0, run_541d50, 0},
    {0x004DEE10u, "boot_4dee10", 0, 0, 0, run_4dee10, 0},
    {0x004CF8E0u, "boot_4cf8e0", 0, 0, 0, run_4cf8e0, 0},
};
const int boot_step_ports_count = sizeof(boot_step_ports) / sizeof(boot_step_ports[0]);
