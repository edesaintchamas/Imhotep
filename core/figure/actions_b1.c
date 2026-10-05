/* Imhotep - figure actions, group b: service roamers and other short actions.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/crt_rand.h"
#include "sim/call.h"

int32_t figb_dir_rel(game_state *st)
{
    /* direction relative to the view (0x00E94E00), kept in 0..7 */
    int32_t d = S8(st, FIGB_F(st) + 0x11) - S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
    }
    return d;
}

int32_t figb_dir_rel_prev(game_state *st)
{
    /* the same with the previous direction (+0x12) when +0x11 is not a direction (8 and up) */
    uint32_t f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11);
    if (d >= 8) {
        d = S8(st, f + 0x12);
    }
    d -= S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
    }
    return d;
}

void figb_walker_image(game_state *st, uint32_t walk_base, uint32_t die_base)
{
    /* the image of a walker: dying frames (action 4) by +0x36 / 2, else 8 frames per direction */
    int32_t d = figb_dir_rel(st);
    uint32_t f = FIGB_F(st);
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, die_base);
        return;
    }
    U32(st, f + 0x04) = U32(st, walk_base) + 8u * U8(st, f + 0x01) + (uint32_t) d;
}

void figb_anim_advance(game_state *st, uint32_t f, int limit)
{
    uint8_t a = (uint8_t) (U8(st, f + 0x01) + 1);
    U8(st, f + 0x01) = a;
    if (a >= limit) {
        U8(st, f + 0x01) = 0;
    }
}

void figb_roamer_action(game_state *st)
{
    /* @bin 0x00493610 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    uint8_t type = U8(st, f + 0x0A);
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * type); /* @bin 0x0049363D property +0x20 */
    int16_t b = S16(st, f + 0x5A);
    uint8_t action = U8(st, f + 0x38);
    if (action == 5) {
        GAME_CALL(st, 0x004B17D0u, id); /* fighting */
        return;
    }
    if (action == 4) {
        GAME_CALL(st, 0x004B1D50u, id); /* dying */
        return;
    }
    if (action == 1) {
        /* roaming: when the roam length reaches its maximum, head back to the building's road */
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            uint32_t ba = BUILDING_ADDR(b);
            int32_t r = GAME_CALL(st, 0x0047FE60u, S16(st, ba + 0x06), S16(st, ba + 0x08), U8(st, ba + 0x03), 2);
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + 0x0E) = 2;
                GAME_CALL(st, 0x004B1EF0u, id, 1);
                return;
            }
            U8(st, f + 0x38) = 2;
            U16(st, f + 0x24) = U16(st, 0x00D3B4E4u); /* the road tile found */
            U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
            GAME_CALL(st, 0x004B3250u, id);
            id = FIGB_CUR(st);
            S16(st, FIGURE_ADDR(id) + 0x44) = 0;
        }
        GAME_CALL(st, 0x004B1EF0u, id, 1);
        return;
    }
    if (action == 2) {
        /* returning: removed when arrived (8) or lost (9, 10) */
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 9 || dir == 10 || dir == 8) {
            U8(st, f + 0x0E) = 2;
        }
        return;
    }
    U8(st, f + 0x38) = 1;
    S16(st, f + 0x44) = S16(st, f + 0x42);
}

/* common start of the roamer actions: terrain usage 1, +0x0C cleared, roam length */
static uint32_t roamer_start(game_state *st, int16_t max_roam)
{
    uint32_t f = FIGB_F(st);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = max_roam;
    return f;
}

/* the building is alive (state 1) and has this figure at +0x2A */
static int home_ok(game_state *st, uint32_t f, int32_t id)
{
    uint32_t ba = BUILDING_ADDR(S16(st, f + 0x5A));
    return U8(st, ba) == 1 && S16(st, ba + 0x2A) == id;
}

/* the plain service roamer: roam 0x180 (or max_roam), die with the building, image groups */
static void plain_roamer(game_state *st, int16_t max_roam, uint32_t walk, uint32_t die)
{
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, max_roam);
    if (!home_ok(st, f, id)) {
        U8(st, f + 0x0E) = 2;
    }
    figb_anim_advance(st, f, 12);
    figb_roamer_action(st);
    figb_walker_image(st, walk, die);
}

/* @bin 0x00494200 .. 0x004946B0 six service roamers, identical but for their sprite groups */
void figb_action_1d(game_state *st) { plain_roamer(st, 0x180, 0x00F55EB4u, 0x00F55EBCu); } /* @bin 0x00494200 */
void figb_action_1e(game_state *st) { plain_roamer(st, 0x180, 0x00F56334u, 0x00F5633Cu); } /* @bin 0x004942F0 */
void figb_action_1f(game_state *st) { plain_roamer(st, 0x180, 0x00F5629Cu, 0x00F562A4u); } /* @bin 0x004943E0 */
void figb_action_20(game_state *st) { plain_roamer(st, 0x180, 0x00F55F24u, 0x00F55F2Cu); } /* @bin 0x004944D0 */
void figb_action_21(game_state *st) { plain_roamer(st, 0x180, 0x00F5628Cu, 0x00F56294u); } /* @bin 0x004945C0 */
void figb_action_22(game_state *st) { plain_roamer(st, 0x180, 0x00F56304u, 0x00F5630Cu); } /* @bin 0x004946B0 */
/* @bin 0x0049B2A0 the same with a roam length of 0xC0 */
void figb_action_40(game_state *st) { plain_roamer(st, 0xC0, 0x00F5635Cu, 0x00F56364u); }

void figb_action_1a(game_state *st)
{
    /* @bin 0x00493EC0 a roamer whose building may hold it at +0x2A or +0xB4; it returns early
     * after 0x60 ticks when its building has nothing left (0x004578B0 + 0x004579D0 <= 0) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    int32_t b = S16(st, f + 0x5A);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba) != 1) {
        U8(st, f + 0x0E) = 2;
    }
    if (S16(st, ba + 0x2A) != id && S16(st, ba + 0xB4) != id) {
        U8(st, f + 0x0E) = 2;
    }
    figb_anim_advance(st, f, 12);
    if (U8(st, f + 0x38) == 1) {
        int32_t n = GAME_CALL(st, 0x004578B0u, b);
        n += GAME_CALL(st, 0x004579D0u, b);
        f = FIGB_F(st);
        if (S16(st, f + 0x44) >= 0x60 && n <= 0) {
            S16(st, f + 0x44) = S16(st, f + 0x42);
        }
    }
    figb_roamer_action(st);
    figb_walker_image(st, 0x00F55D7Cu, 0x00F55D84u);
}

void figb_action_1b(game_state *st)
{
    /* @bin 0x00494010 a roamer removed when its building is not alive, has +0x4A set or another
     * figure; its sprite groups depend on 0x004C5120(building) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    int32_t b = S16(st, f + 0x5A);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba) != 1 || S16(st, ba + 0x4A) != 0 || S16(st, ba + 0x2A) != id) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    figb_anim_advance(st, f, 12);
    if (U8(st, f + 0x38) == 8) {
        U8(st, f + 0x38) = 1;
    } else {
        figb_roamer_action(st);
    }
    figb_dir_rel(st);
    uint32_t kind = (uint32_t) GAME_CALL(st, 0x004C5120u, b);
    int walk, die; /* @bin 0x00494190 jump table */
    switch (kind) {
    case 1: walk = 0xF9; die = 0xFA; break;
    case 2: walk = 0xE2; die = 0xE3; break;
    case 3: walk = 0xE8; die = 0xE9; break;
    case 4: walk = 0xF7; die = 0xF8; break;
    default: walk = 0xEC; die = 0xED; break;
    }
    f = FIGB_F(st);
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, FIGB_SPRITE(die));
        return;
    }
    U32(st, f + 0x04) = U32(st, FIGB_SPRITE(walk)) + 8u * U8(st, f + 0x01) + U32(st, FIGB_DIR_REL);
}

void figb_action_1c(game_state *st)
{
    /* @bin 0x004941B0 an animation of 0x17 frames, then removed */
    uint32_t f = FIGB_F(st);
    int16_t t = S16(st, f + 0x36);
    S16(st, f + 0x36) = (int16_t) (t + 1);
    if (t > 0x15) {
        U8(st, f + 0x0E) = 2;
    }
    U32(st, f + 0x04) = (uint32_t) (S16(st, f + 0x36) + S32(st, 0x00F56E14u) + 1);
}

void figb_action_23(game_state *st)
{
    /* @bin 0x004947A0 only checks its building */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    if (!home_ok(st, f, id)) {
        U8(st, f + 0x0E) = 2;
    }
}

void figb_action_28(game_state *st)
{
    /* @bin 0x00496F20 a roamer (0x80) that only needs its building alive */
    uint32_t f = roamer_start(st, 0x80);
    if (U8(st, BUILDING_ADDR(S16(st, f + 0x5A))) != 1) {
        U8(st, f + 0x0E) = 2;
    }
    figb_anim_advance(st, f, 12);
    figb_roamer_action(st);
    figb_walker_image(st, 0x00F56324u, 0x00F5632Cu);
}

void figb_action_3f(game_state *st)
{
    /* @bin 0x0049B280 a fixed image */
    U32(st, FIGB_F(st) + 0x04) = U32(st, 0x00F55DA4u);
}

void figb_action_41(game_state *st)
{
    /* @bin 0x0049B390 */
    uint32_t f = FIGB_F(st);
    U8(st, f + 0x72) = 0;
    U8(st, f + 0x65) = 1;
    U8(st, f + 0x0C) = 1;
}

int32_t figb_chain_head_27(game_state *st, int32_t id)
{
    /* @bin 0x0049B3C0 follow +0x68 up to 8 times to a figure of type 0x27 (0 if none) */
    int32_t l = S16(st, FIGURE_ADDR(id) + 0x68);
    for (int i = 0; i < 8; i++) {
        uint32_t lf = FIGURE_ADDR(l);
        if (U8(st, lf + 0x0A) == 0x27) {
            return l;
        }
        l = S16(st, lf + 0x68);
    }
    return 0;
}

void figb_action_42(game_state *st)
{
    /* @bin 0x0049B400 a follower of a type 0x27 (or 0x42) figure */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t leader = S16(st, f + 0x68);
    int32_t b = S16(st, f + 0x5A);
    int32_t head = figb_chain_head_27(st, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 1;
    figb_anim_advance(st, f, 12);
    U32(st, f + 0x180) = 0;
    uint32_t lf = FIGURE_ADDR(leader);
    uint32_t hf = FIGURE_ADDR(head);
    if (leader <= 0) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, lf + 0x38) == 4) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, lf + 0x0E) != 1) {
        /* @bin 0x0049B4C0 the leader is gone: its building gets 100 of the resource back */
        U8(st, f + 0x0E) = 2;
        S16(st, BUILDING_ADDR(b) + 0x56 + 2u * U8(st, f + 0x7A)) += 100;
    } else if (U8(st, lf + 0x0A) != 0x27 && U8(st, lf + 0x0A) != 0x42) {
        U8(st, f + 0x0E) = 2;
    } else if (U8(st, hf + 0x38) != 7 && U8(st, hf + 0x38) != 6) {
        GAME_CALL(st, 0x004B22E0u, id, S16(st, f + 0x68), 1);
        id = FIGB_CUR(st);
    }
    if (U8(st, hf + 0x38) == 7) {
        U8(st, FIGURE_ADDR(id) + 0x01) = 0;
    }
    if (U8(st, hf + 0x38) == 6) {
        U8(st, FIGURE_ADDR(id) + 0x65) = 1;
    }
    if (U8(st, lf + 0x65) != 0) {
        U8(st, FIGURE_ADDR(id) + 0x65) = 1;
    }
    f = FIGURE_ADDR(id);
    int32_t d = S8(st, f + 0x11);
    if ((int8_t) d >= 8) {
        d = S8(st, f + 0x12);
    }
    d -= S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 8;
        S32(st, FIGB_DIR_REL) = d;
    }
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F55D34u) + 0x60u;
        return;
    }
    U32(st, f + 0x04) = U32(st, 0x00F55D34u) + 8u * U8(st, f + 0x01) + (uint32_t) d;
}

void figb_action_27(game_state *st)
{
    /* @bin 0x00496140 the leader of a type 0x42 chain: walks to a building and back */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x320);
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    int32_t b = S16(st, f + 0x5A);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba) != 1) {
        U8(st, f + 0x0E) = 2;
    }
    if (S16(st, ba + 0x2C) != id && S16(st, ba + 0xB6) != id) {
        U8(st, f + 0x0E) = 2;
    }
    figb_anim_advance(st, f, 12);
    int back = 0; /* @bin 0x00496235 return to the source tile */
    int8_t dir;
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 6:
        U32(st, f + 0x04) = 0;
        break;
    case 7:
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 100) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, id) == 0) {
                U8(st, FIGB_F(st) + 0x38) = 9;
            }
        }
        break;
    case 8:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 8) {
            GAME_CALL(st, 0x00496060u, FIGB_CUR(st), b, S16(st, f + 0x5E));
        } else if (dir > 8 && dir <= 10) {
            back = 1;
        }
        break;
    case 9:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 8 || dir == 10) {
            U8(st, f + 0x0E) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        }
        break;
    default:
        back = 1;
        break;
    }
    if (back) {
        f = FIGB_F(st);
        U8(st, f + 0x38) = 9;
        U16(st, f + 0x24) = U16(st, f + 0x2C);
        U16(st, f + 0x26) = U16(st, f + 0x2E);
        GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
    }
    figb_walker_image(st, 0x00F55D6Cu, 0x00F55D74u);
}

void figb_action_24(game_state *st)
{
    /* @bin 0x00494800 a figure placed on a tile from tables at 0x00784B38.. by its index at +0x0B,
     * an 8-frame animation and an overlay image (+0x180) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    figb_anim_advance(st, f, 8);
    U32(st, f + 0x04) = U8(st, f + 0x01) + U32(st, 0x00F56874u);
    GAME_CALL(st, 0x004B5420u, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    uint8_t k = U8(st, f + 0x0B);
    uint32_t over;
    uint16_t x, y;
    if (k == 1) {
        x = U8(st, 0x00784C60u);
        y = U8(st, 0x00784C62u);
        over = U32(st, 0x00F5687Cu);
    } else if (k == 2) {
        x = U8(st, 0x00784C64u);
        y = U8(st, 0x00784C66u);
        over = U32(st, 0x00F5687Cu) + 2u;
    } else if (k == 3) {
        x = U8(st, 0x00784C68u);
        y = U8(st, 0x00784C6Au);
        over = U32(st, 0x00F5687Cu) + 3u;
    } else if (k == 0x0C) {
        x = U8(st, 0x00784C8Cu);
        y = U8(st, 0x00784C8Eu);
        over = U32(st, 0x00F5687Cu) + 4u;
    } else if (k == 0x0D) {
        x = U8(st, 0x00784C90u);
        y = U8(st, 0x00784C92u);
        over = U32(st, 0x00F5687Cu) + 5u;
    } else if (k >= 0x0E && k <= 0x15) {
        x = U8(st, 0x00784B58u + 2u * k);
        y = U8(st, 0x00784B68u + 2u * k);
        over = U32(st, 0x00F571FCu) + 7u;
    } else if (k >= 0x16 && k <= 0x19) {
        x = U8(st, 0x00784B38u + 2u * k);
        y = U8(st, 0x00784B40u + 2u * k);
        over = U32(st, 0x00F56814u) + 1u;
    } else if (k >= 0x1A && k <= 0x1D) {
        x = U8(st, 0x00784C58u + 4u * k);
        y = U8(st, 0x00784C68u + 4u * k);
        over = U32(st, 0x00F56814u) + 5u;
    } else if (k >= 0x1E && k <= 0x20) {
        x = U8(st, 0x00784D4Cu + 4u * k);
        y = U8(st, 0x00784D58u + 4u * k);
        over = U32(st, 0x00F569A4u);
    } else {
        x = U8(st, 0x00784B5Cu + 2u * k);
        y = U8(st, 0x00784B7Cu + 2u * k);
        over = (int32_t) k - 0x32 < 8 ? U32(st, 0x00F5687Cu) + 1u : U32(st, 0x00F56694u);
    }
    U16(st, f + 0x14) = x;
    U16(st, f + 0x16) = y;
    U32(st, f + 0x180) = over;
    S16(st, f + 0x4A) = (int16_t) (S16(st, f + 0x14) * 15);
    S16(st, f + 0x4C) = (int16_t) (S16(st, f + 0x16) * 15);
    U32(st, f + 0x20) = U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14) + 228u * (uint32_t) S16(st, f + 0x16);
    S16(st, f + 0x4A) += 7;
    S16(st, f + 0x4C) += 7;
    GAME_CALL(st, 0x004B52F0u, id);
}

void figb_action_25(game_state *st)
{
    /* @bin 0x00494B30 a figure that goes from the point at 0x00784C8C to the one at 0x00784C90 and
     * waits a random time (CRT rand) by how long the trip took; images by its kind at +0x0B */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x76) = 2;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    if (S16(st, 0x00784C90u) == -1 || S16(st, 0x00784C92u) == -1) {
        return;
    }
    U8(st, f + 0x65) = 0;
    U32(st, f + 0x180) = 0;
    U8(st, f + 0x72) = 0;
    switch (U8(st, f + 0x38)) {
    case 8:
        U8(st, f + 0x65) = 1;
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) <= 0) {
            uint32_t c = CURRENT_CITY(st);
            U8(st, f + 0x38) = 9;
            S16(st, f + 0x36) = 0;
            if (S32(st, c + 0x4700) != 0 && U8(st, f + 0x0B) == 0) { /* 0x00EA5F70 */
                U8(st, f + 0x66) = 1;
                S32(st, c + 0x4700) = 0;
            }
            U16(st, f + 0x24) = U8(st, 0x00784C90u);
            U16(st, f + 0x26) = U8(st, 0x00784C92u);
        }
        break;
    case 9:
        if (U8(st, f + 0x03) == 0) {
            U8(st, f + 0x03) = 1;
            S16(st, f + 0x36) += 1;
            GAME_CALL(st, 0x004A5DA0u, id, speed);
            f = FIGB_F(st);
            U8(st, f + 0x65) = 0;
            U8(st, f + 0x77) = 0;
            int8_t dir = S8(st, f + 0x11);
            if (dir == 8 || dir == 9 || dir == 10) {
                U8(st, f + 0x38) = 0x0A;
            }
        } else {
            U8(st, f + 0x03) = 0;
        }
        break;
    case 0x0A: {
        U8(st, f + 0x65) = 1;
        U8(st, f + 0x66) = 0;
        U8(st, f + 0x38) = 8;
        int16_t t = S16(st, f + 0x36);
        int32_t w;
        if (t >= 400) {
            w = crt_rand(st) & 7;
        } else if (t >= 200) {
            w = (crt_rand(st) & 0xF) + 0x32;
        } else if (t >= 100) {
            w = (crt_rand(st) & 0x1F) + 100;
        } else if (t >= 0x32) {
            w = (crt_rand(st) & 0x3F) + 200;
        } else {
            w = crt_rand(st) + 300;
        }
        S16(st, FIGB_F(st) + 0x36) = (int16_t) w;
        GAME_CALL(st, 0x004B5420u, FIGB_CUR(st));
        f = FIGB_F(st);
        U16(st, f + 0x14) = U8(st, 0x00784C8Cu);
        U16(st, f + 0x16) = U8(st, 0x00784C8Eu);
        U32(st, f + 0x20) = U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14) + 228u * (uint32_t) S16(st, f + 0x16);
        S16(st, f + 0x4A) = (int16_t) (S16(st, f + 0x14) * 15);
        S16(st, f + 0x4C) = (int16_t) (S16(st, f + 0x16) * 15);
        break;
    }
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    f = FIGB_F(st);
    uint8_t k = U8(st, f + 0x0B);
    if (k == 0) {
        figb_anim_advance(st, f, 12);
        uint32_t fr = U32(st, 0x005DC7ECu + 4u * U8(st, f + 0x01));
        U32(st, f + 0x04) = fr + U32(st, U8(st, f + 0x66) ? 0x00F56934u : 0x00F568E4u);
    } else if (k == 1 || k == 2) {
        figb_anim_advance(st, f, 0x18);
        U32(st, f + 0x04) = U32(st, 0x005DC81Cu + 4u * U8(st, f + 0x01)) + U32(st, k == 1 ? 0x00F568ECu : 0x00F568F4u);
    } else if (k == 3) {
        figb_anim_advance(st, f, 0x18);
        int32_t fr = S32(st, 0x005DC87Cu + 4u * U8(st, f + 0x01));
        if (fr == -1) {
            U32(st, f + 0x04) = 0;
        } else {
            U32(st, f + 0x04) = (uint32_t) fr + U32(st, 0x00F568FCu);
        }
    }
    if (S32(st, 0x00605A3Cu) == 1 && U8(st, 0x00784CB1u) == 0) {
        U32(st, f + 0x04) = 0;
    }
}
