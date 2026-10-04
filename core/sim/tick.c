/* Imhotep - the game tick (Pharaoh.exe 0x004D7090), the day dispatcher (0x00451C50) and the
 * day / month / year cascade (0x00452370), from their disassembly.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Every call goes through sim_call (core/sim/call.h). Check: tools/oracle/lockstep.py. */
#include "sim/tick.h"

#include "random/random.h"

#define TICK 0x0078D7CCu        /* tick within the day, 0..50 (chunk 34) */
#define DAY 0x0078D7C4u         /* day within the month, 0..15 (chunk 35) */
#define MONTH 0x0078D7C0u       /* 0..11 (chunk 36) */
#define YEAR 0x0078D7BCu        /* chunk 37 */
#define TOTAL_DAYS 0x0078D7C8u  /* chunk 38 */
#define PAUSED 0x00605A3Cu
#define CURRENT_PLAYER 0x00EAAC28u

static uint32_t city(game_state *st)
{
    return 0x00EA1870u + 0x49D8u * U32(st, CURRENT_PLAYER);
}

static int32_t call(game_state *st, const sim_env *env, uint32_t address)
{
    return sim_call(st, env, address, NULL, 0, 0);
}

static int32_t call1(game_state *st, const sim_env *env, uint32_t address, int32_t a)
{
    return sim_call(st, env, address, &a, 1, 0);
}

static void call_n(game_state *st, const sim_env *env, uint32_t address, int32_t a, int32_t b, int32_t c,
                   int32_t d, int32_t e, int n)
{
    int32_t args[5] = {a, b, c, d, e};
    sim_call(st, env, address, args, n, 0);
}

/* @bin 0x00451FCC .. 0x00452012 a sound timer: when it runs out, the sound object 0x00E92BF8 stops
 * channel 2 (0x0055F561) and 0x00F55B94 := 0 */
static void sound_timer(game_state *st, const sim_env *env, uint32_t timer)
{
    if (S32(st, timer) > 0 && --S32(st, timer) == 0) {
        int32_t two = 2;
        sim_call(st, env, 0x0055F561u, &two, 1, 0x00E92BF8u);
        S32(st, 0x00F55B94u) = 0;
    }
}

void day_cascade(game_state *st, const sim_env *env)
{
    /* @bin 0x00452370 */
    S32(st, TOTAL_DAYS) += 1;
    S32(st, DAY) += 1;
    if (S32(st, DAY) > 0xF) { /* a new month */
        S32(st, DAY) = 0;
        uint32_t c = city(st);
        S32(st, c + 0x475C) = S32(st, c + 0x273C); /* 0x00EA5FCC := 0x00EA3FAC */
        S32(st, c + 0x273C) = 0;
        S32(st, c + 0x4760) = 0;
        S32(st, c + 0x4764) = 0;
        S32(st, c + 0x438C) += 1; /* 0x00EA5BFC */
        static const uint32_t A[] = {0x004E81D0u, 0x004599D0u, 0x0045A2E0u, 0x0045A350u, 0x0045A3F0u,
                                     0x00452160u, 0x004474E0u, 0x0045E420u, 0x004464F0u};
        for (unsigned i = 0; i < sizeof(A) / sizeof(A[0]); i++) {
            call(st, env, A[i]);
        }
        sim_call(st, env, 0x0044BA90u, NULL, 0, 0x00785D70u);
        call(st, env, 0x004EA740u);
        call(st, env, 0x004B7960u);
        call(st, env, 0x004E1850u);
        call(st, env, 0x0047B570u);
        call_n(st, env, 0x00475CC0u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1, 0, 5);
        call(st, env, 0x00517370u);
        call(st, env, 0x004E1640u);
        S32(st, MONTH) += 1;
        call(st, env, 0x004E7700u);
        call(st, env, 0x004BC7A0u);
        call(st, env, 0x00452200u);
        if (S32(st, MONTH) > 0xB) { /* a new year */
            S32(st, MONTH) = 0;
            call(st, env, 0x0051F780u);
            U8(st, 0x00E3908Cu) = 0;
            S32(st, 0x00BC9694u) = 0;
            S32(st, YEAR) += 1;
            static const uint32_t Y[] = {0x004E8630u, 0x00459E50u, 0x0045A450u, 0x0045A490u, 0x0045A4D0u,
                                         0x0045A500u, 0x0045A530u, 0x0045A560u, 0x0045A5A0u, 0x0045A5D0u,
                                         0x004448E0u, 0x00455530u};
            for (unsigned i = 0; i < sizeof(Y) / sizeof(Y[0]); i++) {
                call(st, env, Y[i]);
            }
            call1(st, env, 0x0046E180u, 1);
            call(st, env, 0x004C5300u);
        } else {
            call1(st, env, 0x0046E180u, 0);
        }
        if (S32(st, MONTH) == 0 || S32(st, MONTH) == 6) {
            S32(st, city(st) + 0x471C) = 2; /* 0x00EA5F8C */
        }
        call(st, env, 0x004E9700u);
        call(st, env, 0x004EA000u);
        if (U8(st, 0x00E3908Cu) == 0 && U8(st, 0x00E38EC2u) == 1) {
            call1(st, env, 0x004D4E20u, (int32_t) 0x005DBB94u); /* the autosave, "last.sav" */
        }
    }
    if (S32(st, DAY) == 0 || S32(st, DAY) == 8) {
        call(st, env, 0x004E8840u);
        if (S32(st, DAY) == 8) {
            sim_call(st, env, 0x004BE640u, NULL, 0, 0x00D05720u);
        }
    }
    call(st, env, 0x004E8DB0u);
    uint32_t c = city(st);
    if (U8(st, 0x007027B8u) != 0 && S32(st, c + 0x4720) == 0) { /* 0x00EA5F90 */
        S32(st, c + 0x4720) = 1;
        S32(st, 0x00E950DCu) = 1;
    }
    if (U8(st, 0x007027BEu) != 0 && S32(st, c + 0x4724) == 0) { /* 0x00EA5F94 */
        S32(st, c + 0x4724) = 1;
        S32(st, 0x00E950DCu) = 1;
        call_n(st, env, 0x004E0CF0u, 0x13E, 0, 0, 0, 0, 5);
        c = city(st);
    }
    if (U8(st, 0x007027B9u) == 0 && S32(st, c + 0x24) >= 0x96) {
        call1(st, env, 0x00417AE0u, 1);
    }
}

void day_dispatch(game_state *st, const sim_env *env)
{
    /* @bin 0x00451C50 one step per tick of the day, by the tick counter */
    uint32_t c;
    switch (S32(st, TICK)) {
    case 0: call(st, env, 0x0051F780u); break;
    case 1: call1(st, env, 0x004E9890u, 1); break;
    case 2: call(st, env, 0x00443330u); break;
    case 3: S32(st, 0x00F53FD0u) = 1; break;
    case 4: call(st, env, 0x004460B0u); break;
    case 5: call1(st, env, 0x004B7790u, 0); break;
    case 6: call(st, env, 0x0051F780u); break;
    case 7: call(st, env, 0x00468860u); break;
    case 8: call(st, env, 0x0045E040u); break;
    case 9: call(st, env, 0x00452150u); break;
    case 0xA: call(st, env, 0x0045AA30u); break;
    case 0xB: { /* city +0x24 := sum of the 100 i16 at +0x25C4; its maximum at +0x469C */
        c = city(st);
        S32(st, c + 0x24) = 0;
        for (uint32_t i = 0; i < 100; i++) {
            S32(st, c + 0x24) += S16(st, c + 0x25C4 + 2 * i);
        }
        if (S32(st, c + 0x24) > S32(st, c + 0x469C)) {
            S32(st, c + 0x469C) = S32(st, c + 0x24);
        }
        break;
    }
    case 0xC: call(st, env, 0x004558B0u); break;
    case 0x10: call(st, env, 0x0045AAA0u); break;
    case 0x11: call(st, env, 0x0045D5D0u); break;
    case 0x12: call(st, env, 0x0045E6C0u); break;
    case 0x13: call(st, env, 0x00457A40u); break;
    case 0x14: call(st, env, 0x00455900u); call(st, env, 0x004860D0u); break;
    case 0x15: call(st, env, 0x00457AF0u); break;
    case 0x16: call(st, env, 0x00458440u); break;
    case 0x17: call(st, env, 0x00458540u); break;
    case 0x18: call(st, env, 0x00459070u); break;
    case 0x19:
        call(st, env, 0x00459100u);
        call(st, env, 0x00459350u);
        call(st, env, 0x004595A0u);
        call(st, env, 0x0051F780u);
        call(st, env, 0x0051F780u);
        call(st, env, 0x00459670u);
        break;
    case 0x1B: call(st, env, 0x004555D0u); break;
    case 0x1C: call(st, env, 0x00455430u); break;
    case 0x1D: call1(st, env, 0x004B7790u, 1); break;
    case 0x1E: S32(st, 0x00F53FD0u) = 1; break;
    case 0x1F: call(st, env, 0x00462180u); break;
    case 0x20: call(st, env, 0x00444C20u); call(st, env, 0x0046F200u); break;
    case 0x21: call(st, env, 0x004677C0u); call(st, env, 0x00468170u); break;
    case 0x22: call(st, env, 0x004684E0u); break;
    case 0x23:
        c = city(st);
        if (S16(st, c + 0x4478) == 0 || call1(st, env, 0x004C5120u, S32(st, c + 0x472C)) != 4) {
            call(st, env, 0x00454E20u);
        } else if (S32(st, city(st) + 0x4990) != 0) { /* 0x00EA6200: every other day */
            S32(st, city(st) + 0x4990) = 0;
        } else {
            call(st, env, 0x00454E20u);
            S32(st, city(st) + 0x4990) = 10;
        }
        break;
    case 0x24: call(st, env, 0x00454FA0u); break;
    case 0x25: call(st, env, 0x0046A0C0u); call(st, env, 0x0046A140u); break;
    case 0x26: call(st, env, 0x004539E0u); break;
    case 0x27: call(st, env, 0x004528B0u); break;
    case 0x28: call(st, env, 0x004525F0u); break;
    case 0x2B: call(st, env, 0x00469730u); break;
    case 0x2C: call(st, env, 0x00468B70u); break;
    case 0x2D: call(st, env, 0x00469B90u); call(st, env, 0x0047E290u); break;
    case 0x2E: call(st, env, 0x0051F780u); break;
    case 0x2F: call(st, env, 0x0046A060u); break;
    case 0x30: call(st, env, 0x004599A0u); break;
    case 0x31: call(st, env, 0x004550E0u); break;
    case 0x32: call(st, env, 0x0051F780u); break;
    default: break; /* 0xD..0xF, 0x1A, 0x29, 0x2A: nothing */
    }
    sound_timer(st, env, 0x0078D79Cu);
    sound_timer(st, env, 0x0078D7A8u);
    if (U8(st, 0x00E38DDAu) == 1) {
        sound_timer(st, env, 0x0078D7A0u);
    }
    /* @bin 0x0045203E the map is revealed column by column while 0x0078D7B0 counts down */
    if (S32(st, 0x0078D7B0u) > 0) {
        int32_t left = --S32(st, 0x0078D7B0u);
        int32_t w = S32(st, 0x00BCA2D0u), h = S32(st, 0x00BCA2D4u);
        if (left == 0) {
            call_n(st, env, 0x00475CC0u, 0, 0, h - 1, w - 1, 1, 5);
            call_n(st, env, 0x004776F0u, 0, 0, h - 1, w - 1, 0, 4);
            call(st, env, 0x0046D840u);
            for (int32_t id = 1; id < 2000; id++) {
                uint32_t f = 0x00BCD5F8u + 0x184u * (uint32_t) id;
                if (U8(st, f + 0x0E) != 0 && U8(st, f + 0x0A) == 0x5F) {
                    call1(st, env, 0x004B52F0u, id);
                }
            }
        } else {
            int32_t k = (S32(st, 0x0078D7ACu) - left) / 2;
            if (k < 0xE4) {
                call_n(st, env, 0x00475CC0u, 0xE5 - k, 0, 0xE6 - k, h - 1, 1, 5);
                call_n(st, env, 0x004776F0u, 0xE5 - k, 0, 0xE6 - k, h - 1, 0, 4);
                call(st, env, 0x0046D840u);
            }
        }
        U8(st, 0x00E38DCAu) = 1;
    }
    if (S32(st, 0x0078D7A4u) > 0 && --S32(st, 0x0078D7A4u) == 0) {
        U8(st, 0x00EAAF5Cu) = 0;
    }
    S32(st, TICK) += 1;
    if (S32(st, TICK) >= 0x33) {
        S32(st, TICK) = 0;
        day_cascade(st, env);
    }
    if (U8(st, 0x00EAAF5Cu) != 0) {
        call(st, env, 0x004F3890u);
    }
}

void game_tick(game_state *st, const sim_env *env)
{
    /* @bin 0x004D7090 the RNG step (inline in the original), two calls, the day when the game is not
     * paused, then four calls (0x004EA2C0 is a tail jump) */
    random_next_state(st);
    call(st, env, 0x004CDC70u);
    call(st, env, 0x00485DC0u);
    if (S32(st, PAUSED) == 0) {
        day_dispatch(st, env);
        call(st, env, 0x004E7E40u);
        sim_call(st, env, 0x004BE770u, NULL, 0, 0x00D05720u);
        if (S32(st, 0x00D386D4u) == 3 && S32(st, city(st) + 0x4998) == 2) { /* 0x00EA6208 */
            S32(st, city(st) + 0x4998) = 0;
        }
    }
    call(st, env, 0x00489B70u);
    call(st, env, 0x0041FF60u);
    sim_call(st, env, 0x0044BCA0u, NULL, 0, 0x00785D70u);
    call(st, env, 0x004EA2C0u);
}
