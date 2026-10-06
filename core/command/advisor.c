/* Imhotep - opening an advisor (package cmdrest, docs/commands.md 3.10).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0040A0E0 opens the advisor window 0x00D3B198 (also reached from the military advisor's
 * dispatch 0x0040A450 with advisor 4). Before the window is drawn the original refreshes the
 * figures the advisors show (0x0040A160): the labour allocation, the finance estimates, the
 * ratings and the city's summary flags; these are city data, so the command writes simulation
 * state and is ported here with them. */
#include "command/advisor.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define PARAM1 0x00D3B198u
#define CUSTOM 0x00E38EBCu        /* i32: non-zero outside the campaign missions (assumed) */
#define ADVISOR_ALLOWED 0x006FDC88u /* u8 per advisor: the mission offers it */
#define WINDOW 0x00E38DDAu        /* u8 */
#define REDRAW 0x00E38DCAu        /* u8 */

static void advisor_rating_flag(game_state *st)
{
    /* @bin 0x00426B00 CITY+0x4434 := 0x00705738 */
    S32(st, CURRENT_CITY(st) + 0x4434u) = S32(st, 0x00705738u);
}

void advisor_complex_level(game_state *st)
{
    /* @bin 0x0045A930 with a temple complex (i16 CITY+0x4478), its level (0x004C5120, result not
     * kept); then CITY+0x2C7C := CITY+0x2C74 */
    if (S16(st, CURRENT_CITY(st) + 0x4478u) != 0) {
        GAME_CALL(st, 0x004C5120u, S32(st, CURRENT_CITY(st) + 0x472Cu));
    }
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x2C7Cu) = S32(st, c + 0x2C74u);
}

void advisor_summary_flags(game_state *st)
{
    /* @bin 0x004E9500 the city's summary codes (meanings assumed: what the advisors show first) */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = S32(st, c + 0x4540u);
    int32_t code;
    switch (v) {
    case 1: code = 2; break;
    case 2: code = 1; break;
    case 3: code = 3; break;
    case 4: code = 0; break;
    case 5: code = 4; break;
    default: code = 5; break;
    }
    S32(st, c + 0x4410u) = code;
    /* @bin 0x004E956D the largest of four values (signed), by its index */
    S32(st, c + 0x441Cu) = 0;
    int32_t a = S32(st, c + 0x4328u);
    int32_t m = 0;
    if (a > 0 || S32(st, c + 0x474Cu) > 0) {
        if (a < S32(st, c + 0x474Cu)) {
            S32(st, c + 0x441Cu) = 4;
            m = S32(st, c + 0x474Cu);
        } else {
            S32(st, c + 0x441Cu) = 1;
            m = S32(st, c + 0x4328u);
        }
    }
    if (S32(st, c + 0x4324u) > m) {
        S32(st, c + 0x441Cu) = 2;
        m = S32(st, c + 0x4324u);
    }
    if (S32(st, c + 0x433Cu) > m) {
        S32(st, c + 0x441Cu) = 3;
    }
    /* @bin 0x004E95D3 */
    S32(st, c + 0x4424u) = 0;
    int32_t p = S32(st, c + 0x4310u), q = S32(st, c + 0x4314u);
    if (p > q) {
        S32(st, c + 0x4424u) = 1;
    } else if (p < q) {
        S32(st, c + 0x4424u) = 2;
    } else if (p != 0 || q != 0) {
        S32(st, c + 0x4424u) = 3;
    }
    /* @bin 0x004E960D */
    S32(st, c + 0x4428u) = 0;
    int32_t r = S32(st, c + 0x430Cu);
    if (S32(st, c + 0x4388u) > r) {
        S32(st, c + 0x4428u) = 1;
    } else if (r != 0) {
        S32(st, c + 0x4428u) = 2;
    }
    /* @bin 0x004E9633 */
    S32(st, c + 0x4420u) = 0;
    m = 0;
    if (S32(st, c + 0x4350u) > 0) {
        S32(st, c + 0x4420u) = 1;
        m = S32(st, c + 0x4350u);
    }
    if (S32(st, c + 0x4354u) > m) {
        S32(st, c + 0x4420u) = 2;
        m = S32(st, c + 0x4354u);
    }
    if (S32(st, c + 0x4358u) > m) {
        S32(st, c + 0x4420u) = 3;
    }
}

void advisor_houses_count(game_state *st)
{
    /* @bin 0x004F6AD0 0x00EADB28 := 0x004EA280(0x00784C38) */
    S32(st, 0x00EADB28u) = GAME_CALL(st, 0x004EA280u, S32(st, 0x00784C38u));
}

void advisor_ratings(game_state *st)
{
    /* @bin 0x0046E1D0 */
    GAME_CALL0(st, 0x0046E1C0u);
    advisor_rating_flag(st);
}

void advisors_refresh(game_state *st)
{
    /* @bin 0x0040A160 the advisors' figures, in this order */
    GAME_CALL0(st, 0x00459400u);
    GAME_CALL0(st, 0x004595A0u);
    GAME_CALL0(st, 0x0051F780u);
    GAME_CALL0(st, 0x0051F780u);
    GAME_CALL0(st, 0x00459670u);
    GAME_CALL0(st, 0x00459D20u);
    GAME_CALL0(st, 0x0045A8C0u);
    advisor_complex_level(st);
    GAME_CALL0(st, 0x0045A970u);
    GAME_CALL0(st, 0x0045A990u);
    GAME_CALL0(st, 0x0045A7B0u);
    advisor_summary_flags(st);
    GAME_CALL0(st, 0x00468170u);
    GAME_CALL0(st, 0x0045D5D0u);
    GAME_CALL0(st, 0x004B7C80u);
    GAME_CALL0(st, 0x004B8AA0u);
    advisor_ratings(st);
    GAME_CALL0(st, 0x0046E410u);
    GAME_CALL0(st, 0x0046EC70u);
    advisor_houses_count(st);
}

void advisor_open(game_state *st)
{
    /* @bin 0x0040A0E0 an advisor the mission does not offer (outside custom games) posts notice
     * 0xD (closing the message window 0x004C7F30 first when it is open, window 5) */
    int32_t custom = S32(st, CUSTOM);
    int32_t adv = S32(st, PARAM1);
    if (custom == 0 && U8(st, ADVISOR_ALLOWED + (uint32_t) adv) == 0) {
        if (U8(st, WINDOW) == 5) {
            GAME_CALL0(st, 0x004C7F30u);
            U8(st, WINDOW) = 1;
        }
        GAME_CALL(st, 0x004E2970u, 0xD, 0, 0);
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    U8(st, WINDOW) = 7;
    S32(st, 0x00605A44u) = adv;
    S32(st, 0x00E38EA4u) = adv;
    U8(st, c + 0x49CCu) = 0;
    advisors_refresh(st);
    U8(st, REDRAW) = 1;
    S32(st, 0x00F53F48u) = 6;
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_open, advisor_open)
V(run_refresh, advisors_refresh)
V(run_complex, advisor_complex_level)
V(run_summary, advisor_summary_flags)
V(run_houses, advisor_houses_count)
V(run_ratings, advisor_ratings)
V(run_rating_flag, advisor_rating_flag)

const ported_function advisor_ports[] = {
    {0x0040A0E0u, "advisor_open", 0, 0, 0, run_open, 0},
    {0x0040A160u, "advisors_refresh", 0, 0, 0, run_refresh, 0},
    {0x0045A930u, "advisor_complex_level", 0, 0, 0, run_complex, 0},
    {0x004E9500u, "advisor_summary_flags", 0, 0, 0, run_summary, 0},
    {0x004F6AD0u, "advisor_houses_count", 0, 0, 0, run_houses, 0},
    {0x0046E1D0u, "advisor_ratings", 0, 0, 0, run_ratings, 0},
    {0x00426B00u, "advisor_rating_flag", 0, 0, 0, run_rating_flag, 0},
};
const int advisor_ports_count = sizeof(advisor_ports) / sizeof(advisor_ports[0]);
