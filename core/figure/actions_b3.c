/* Imhotep - figure actions, group b: figure type 0x26.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "sim/call.h"

/* @bin 0x00495310 after the moves to the destination: removed if the destination is gone */
static void check_destination(game_state *st)
{
    uint32_t f = FIGB_F(st);
    if (U8(st, BUILDING_ADDR(S16(st, f + 0x5E))) != 1) {
        U8(st, f + 0x0E) = 2;
    }
}

/* walk; returns the direction after the move (8 arrived, 9 reroute, 10 lost) */
static int8_t walk(game_state *st, int32_t speed)
{
    GAME_CALL(st, 0x0048B4A0u, FIGB_CUR(st));
    GAME_CALL(st, 0x004A5DA0u, FIGB_CUR(st), speed);
    return S8(st, FIGB_F(st) + 0x11);
}

/* @bin 0x004954BB back to the source tile (action 0xE) */
static void head_back(game_state *st, uint32_t f)
{
    U8(st, f + 0x38) = 0x0E;
    S16(st, f + 0x36) = 0;
    U16(st, f + 0x24) = U16(st, f + 0x2C);
    U16(st, f + 0x26) = U16(st, f + 0x2E);
}

void figb_action_26(game_state *st)
{
    /* @bin 0x00494F70 a figure of a type 0x4B / 0x4C / 0xC0 building: takes turns at the building
     * (+0x54 holds the figure being served), delivers to a destination and comes back */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    figb_anim_advance(st, f, 12);
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    U32(st, f + 0x180) = 0;
    U8(st, f + 0x65) = 0;
    int32_t b = S16(st, f + 0x5A);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba) != 1) {
        U8(st, f + 0x0E) = 2;
    }
    int16_t bt = S16(st, ba + 0x10);
    if (bt != 0x4B && bt != 0x4C && bt != 0xC0) {
        U8(st, f + 0x0E) = 2;
    }
    if (U8(st, ba + 0xA7) != 0) {
        U8(st, ba + 0xA7) -= 1;
    }
    int16_t w = S16(st, ba + 0xB4);
    if (w != 0) {
        uint32_t wf = FIGURE_ADDR(w);
        if (U8(st, wf + 0x0E) != 1 || U8(st, wf + 0x0A) != 0x14 || U8(st, wf + 0x38) == 0x0D) {
            S16(st, ba + 0xB4) = 0;
        }
    }
    U8(st, f + 0x72) = 1;
    int8_t dir;
    switch (U8(st, f + 0x38)) {
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 8:
        U8(st, f + 0x0B) = 0;
        U32(st, f + 0x180) = 0;
        if (GAME_CALL(st, 0x00495840u, b) == 0) {
            GAME_CALL(st, 0x00495950u, b);
        }
        f = FIGB_F(st);
        U8(st, f + 0x01) = 0;
        /* @bin 0x004950F6 terrain bit 0x40 under the figure */
        if ((U8(st, 0x00AF598Cu + 4u * U32(st, f + 0x20)) & 0x40) == 0) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    case 9:
        U32(st, f + 0x180) = 0;
        U8(st, f + 0x01) = 0;
        if (S16(st, ba + 0x54) <= 0) {
            S16(st, ba + 0x54) = (int16_t) id;
            S16(st, f + 0x36) = 0;
        }
        if (S16(st, ba + 0x54) == id) {
            S16(st, f + 0x36) += 1;
            U8(st, ba + 0xA7) = 0x78;
            if (S16(st, f + 0x36) >= 0x50) {
                U8(st, f + 0x38) = 0x0B;
                S16(st, f + 0x36) = 0;
                GAME_CALL(st, 0x0048B4A0u, id);
                S16(st, ba + 0x54) = 0;
            }
        } else {
            /* @bin 0x00495187 the served figure must be one of the three at +0xAE, waiting */
            int found = 0;
            for (int i = 0; i < 3; i++) {
                int16_t o = S16(st, ba + 0xAE + 2u * (uint32_t) i);
                if (o == 0) {
                    continue;
                }
                uint32_t of = FIGURE_ADDR(o);
                if (U8(st, of + 0x0E) != 1) {
                    continue;
                }
                uint8_t oa = U8(st, of + 0x38);
                if ((oa == 9 || oa == 0x0A) && S16(st, ba + 0x54) == o) {
                    found = 1;
                }
            }
            if (found != 1) {
                S16(st, ba + 0x54) = 0;
            }
        }
        break;
    case 0x0A:
        GAME_CALL(st, 0x0048B4A0u, id);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (S16(st, ba + 0x54) <= 0) {
            S16(st, ba + 0x54) = (int16_t) id;
            S16(st, f + 0x36) = 0;
        }
        if (S16(st, ba + 0x54) == id) {
            U8(st, ba + 0xA7) = 0x78;
            S16(st, f + 0x36) += 1;
            if (S16(st, f + 0x36) >= 0x50) {
                U8(st, f + 0x38) = 8;
                S16(st, f + 0x36) = 0;
                U32(st, f + 0x04) = 0;
                U32(st, f + 0x180) = 0;
                S16(st, ba + 0x54) = 0;
            }
        }
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) >= 0x14) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = 0;
        }
        U8(st, f + 0x01) = 0;
        break;
    case 0x0B:
    case 0x0C:
    case 0x0D: {
        uint8_t a = U8(st, f + 0x38);
        dir = walk(st, speed);
        f = FIGB_F(st);
        if (dir == 8) {
            if (a == 0x0B) {
                U8(st, f + 0x38) = 0x0F;
            } else if (a == 0x0C) {
                U8(st, f + 0x38) = 0x10;
            } else {
                U8(st, f + 0x38) = 0x0A;
                S16(st, f + 0x36) = 0;
            }
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + 0x0E) = 2;
        }
        check_destination(st);
        break;
    }
    case 0x0F:
        GAME_CALL(st, 0x0048B4A0u, id);
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 10) {
            int32_t o = S16(st, ba + 0xB4);
            int32_t v = o != 0 ? U8(st, FIGURE_ADDR(o) + 0x6E) : 0;
            int32_t dest = S16(st, f + 0x5E);
            int16_t dt = S16(st, BUILDING_ADDR(dest) + 0x10);
            if ((dt == 0x48 || dt == 0xC1) && GAME_CALL(st, 0x00445980u, dest, U8(st, f + 0x0B), v, o) != 0) {
                f = FIGB_F(st);
                head_back(st, f);
                U8(st, f + 0x0B) = 0;
                GAME_CALL(st, 0x00495950u, b);
            } else {
                head_back(st, FIGB_F(st));
            }
            f = FIGB_F(st);
            S16(st, f + 0x36) = 0;
        }
        U8(st, f + 0x01) = 0;
        break;
    case 0x10: {
        GAME_CALL(st, 0x0048B4A0u, id);
        int32_t o = S16(st, ba + 0xB4);
        int32_t v = o != 0 ? U8(st, FIGURE_ADDR(o) + 0x6E) : 0;
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 10) {
            head_back(st, f);
            S16(st, f + 0x36) = 0;
            if (GAME_CALL(st, 0x00445E50u, S16(st, f + 0x5E), U8(st, f + 0x0B), v, o) != 0) {
                U8(st, FIGB_F(st) + 0x38) = 0x0D;
            } else {
                GAME_CALL(st, 0x00495950u, b);
            }
        }
        U8(st, FIGB_F(st) + 0x01) = 0;
        break;
    }
    case 0x0E:
        dir = walk(st, speed);
        f = FIGB_F(st);
        if (dir == 8) {
            U8(st, f + 0x38) = 8;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    case 7:
        U32(st, f + 0x04) = U32(st, 0x00F560DCu) + U32(st, FIGB_DIR_REL) + 1u;
        U8(st, f + 0x01) = 0;
        GAME_CALL(st, 0x0048B4A0u, id);
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 100) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, FIGB_CUR(st)) == 0) {
                U8(st, FIGB_F(st) + 0x38) = 8;
            }
        }
        break;
    case 6:
        U32(st, f + 0x04) = 0;
        if (U32(st, f + 0x180) != 0) {
            U32(st, f + 0x180) = 0;
        }
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    /* @bin 0x0049507B image, and the carried-resource overlay (+0x180, +0x6C, +0x6D) */
    f = FIGB_F(st);
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
    uint8_t act = U8(st, f + 0x38);
    if (act == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F560E4u);
        U32(st, f + 0x180) = 0;
    } else if (act != 6) {
        U32(st, f + 0x04) = U32(st, 0x00F560DCu) + 8u * U8(st, f + 0x01) + (uint32_t) d;
    }
    uint32_t over = U32(st, f + 0x180);
    if (over == 0) {
        U32(st, f + 0x04) = 0;
        return;
    }
    U32(st, f + 0x180) = over + (uint32_t) d;
    uint8_t r = U8(st, f + 0x0B);
    int32_t k = d;
    /* @bin 0x00495828 resources drawn from the opposite side */
    if (r == 0x18 || r == 0x19 || r == 0x1A || r == 0x1E || r == 0x23 || r == 0x0C) {
        k = d + 4;
        if (k >= 8) {
            k -= 8;
        }
    }
    U8(st, f + 0x6C) = U8(st, 0x005DC23Cu + 2u * (uint32_t) k);
    U8(st, f + 0x6D) = U8(st, 0x005DC23Du + 2u * (uint32_t) k);
}

void figb_action_2a(game_state *st)
{
    /* @bin 0x00497450 a figure that leaves its building (+0x2A) to work on water tiles (terrain
     * 0x4000 / 0x8000 under it), with a timed action 0xA (property +0x1C / +0x18) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 4;
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 1;
    U8(st, f + 0x77) = 10;
    S16(st, f + 0x42) = 800;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    uint32_t ba = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, ba) != 1 || S16(st, ba + 0x2A) != id) {
        U8(st, f + 0x0E) = 2;
    }
    figb_anim_advance(st, f, 12);
    uint8_t a = U8(st, f + 0x38);
    if (a != 0x0C && a != 7 && a != 6) {
        GAME_CALL(st, 0x00497CF0u, id);
    }
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    int8_t dir;
    switch (U8(st, f + 0x38)) {
    case 5:
        GAME_CALL(st, 0x0053B4A0u, id, 9, 2);
        GAME_CALL(st, 0x004B17D0u, FIGB_CUR(st));
        break;
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 8:
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) <= 0x28) {
            break;
        }
        S16(st, f + 0x36) = 0;
        if (GAME_CALL(st, 0x00497A30u, id) == 0) {
            break;
        }
        f = FIGB_F(st);
        U8(st, f + 0x38) = 9;
        U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
        U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
        GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        break;
    case 0x0C:
        U8(st, f + 0x72) = 10;
        U8(st, f + 0x65) = 0;
        U8(st, f + 0x77) = 0;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9 || dir == 10) {
            U8(st, f + 0x0E) = 2;
        } else if (dir == 8) {
            /* @bin 0x0049761F back in the building: placed on its tile */
            GAME_CALL(st, 0x004B5420u, FIGB_CUR(st));
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            uint16_t x = U16(st, ba + 0x06), y = U16(st, ba + 0x08);
            U16(st, f + 0x14) = x;
            U16(st, f + 0x24) = x;
            U16(st, f + 0x16) = y;
            U16(st, f + 0x26) = y;
            U32(st, f + 0x20) = U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14) + 228u * (uint32_t) S16(st, f + 0x16);
            GAME_CALL(st, 0x004B52F0u, id);
            U8(st, FIGB_F(st) + 0x38) = 8;
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        }
        break;
    case 9:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9 || dir == 10) {
            U8(st, f + 0x38) = 8;
        } else if (dir == 8) {
            U8(st, f + 0x38) = 0x0B;
            U16(st, f + 0x24) = U16(st, f + 0x2C);
            U16(st, f + 0x26) = U16(st, f + 0x2E);
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        }
        break;
    case 0x0B:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 9 || dir == 10) {
            U8(st, f + 0x0E) = 2;
        } else if (dir == 8) {
            U8(st, f + 0x38) = 8;
        }
        break;
    case 0x0A: {
        GAME_CALL(st, 0x004B28A0u, id, 1);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        uint8_t n = (uint8_t) (U8(st, f + 0x6B) + 1);
        U8(st, f + 0x6B) = n;
        uint32_t props = FIGB_PROPS + 0x38u * U8(st, f + 0x0A);
        if ((int32_t) n <= S32(st, props + 0x1C)) {
            break;
        }
        if (GAME_CALL(st, 0x00499B50u, id, S32(st, props + 0x18)) != 0) {
            GAME_CALL(st, 0x0053B4A0u, FIGB_CUR(st), 9, 2);
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            uint32_t tx = U32(st, 0x00D3B4E4u), ty = U32(st, 0x00D3B4E0u);
            int32_t d = GAME_CALL(st, 0x0051CD80u, S16(st, f + 0x14), S16(st, f + 0x16), (int32_t) tx, (int32_t) ty);
            U8(st, f + 0x11) = (uint8_t) d;
            U8(st, f + 0x6B) = 0;
            GAME_CALL(st, 0x004B45C0u, FIGB_CUR(st), S16(st, f + 0x14), S16(st, f + 0x16),
                      (int32_t) U32(st, 0x00D3B4E4u), (int32_t) U32(st, 0x00D3B4E0u), 0x3C);
        } else {
            f = FIGB_F(st);
            U8(st, f + 0x38) = 0x0B;
            U16(st, f + 0x24) = U16(st, f + 0x2C);
            U16(st, f + 0x26) = U16(st, f + 0x2E);
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        }
        break;
    }
    case 6:
        U8(st, f + 0x72) = 10;
        U32(st, f + 0x04) = 0;
        U32(st, f + 0x180) = 0;
        break;
    case 7:
        U8(st, f + 0x72) = 10;
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 100) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, id) == 0) {
                U8(st, FIGB_F(st) + 0x0E) = 2;
            }
        }
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    /* @bin 0x00497727 the terrain under the figure */
    f = FIGB_F(st);
    uint32_t t = U32(st, 0x00AF598Cu + 4u * U32(st, f + 0x20));
    if (t & 0x4000) {
        U8(st, f + 0x78) = 0x12;
    } else if (t & 0x8000) {
        U8(st, f + 0x40) = 0x18;
    } else {
        uint8_t act = U8(st, f + 0x38);
        if (act != 0x0C && act != 7 && act != 6) {
            U8(st, f + 0x0E) = 2;
        }
    }
    if (U8(st, f + 0x40) != 0) {
        U8(st, f + 0x40) -= 1;
        U8(st, f + 0x77) = 0;
    }
    int32_t d = figb_dir_rel(st);
    uint8_t act = U8(st, f + 0x38);
    if (act == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F55EA4u);
    } else if (act == 0x0A) {
        U32(st, f + 0x04) = U32(st, 0x00F55EACu) + 8u * U8(st, 0x005D050Cu + (U8(st, f + 0x6B) >> 1)) + (uint32_t) d;
    } else {
        U32(st, f + 0x04) = U32(st, 0x00F55E9Cu) + 8u * U8(st, f + 0x01) + (uint32_t) d;
    }
}

void figb_action_3e(game_state *st)
{
    /* @bin 0x0049AEB0 a figure standing at the corner of its building (+0x30) facing the view,
     * acting on targets found by 0x00499B50 (property +0x18 range, +0x1C delay) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int shooting = 0;
    U8(st, f + 0x72) = 4;
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 1;
    U8(st, f + 0x77) = 10;
    U8(st, f + 0x78) = 0x2D;
    uint32_t ba = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, ba) != 1 || S16(st, ba + 0x30) != id) {
        U8(st, f + 0x0E) = 2;
    }
    if (S16(st, ba + 0x40) <= 0) {
        U8(st, f + 0x0E) = 2;
    }
    if (S16(st, ba + 0x2A) <= 0) {
        U8(st, f + 0x0E) = 2;
    }
    GAME_CALL(st, 0x004B5420u, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    /* @bin 0x0049AF4E the building tile nearest to the viewer */
    int32_t view = S32(st, FIGB_VIEW_DIR);
    uint16_t bx = U16(st, ba + 0x06), by = U16(st, ba + 0x08);
    if (view == 0) {
        U16(st, f + 0x14) = bx;
        U16(st, f + 0x16) = by;
    } else if (view == 2) {
        U16(st, f + 0x14) = (uint16_t) (bx + 1);
        U16(st, f + 0x16) = by;
    } else if (view == 4) {
        U16(st, f + 0x14) = (uint16_t) (bx + 1);
        U16(st, f + 0x16) = (uint16_t) (by + 1);
    } else if (view == 6) {
        U16(st, f + 0x14) = bx;
        U16(st, f + 0x16) = (uint16_t) (by + 1);
    }
    U32(st, f + 0x20) = U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14) + 228u * (uint32_t) S16(st, f + 0x16);
    GAME_CALL(st, 0x004B52F0u, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    uint32_t props = FIGB_PROPS + 0x38u * U8(st, f + 0x0A);
    switch (U8(st, f + 0x38)) {
    case 4:
        U8(st, f + 0x0E) = 2;
        break;
    case 8:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x14) {
            S16(st, f + 0x36) = 0;
            if (GAME_CALL(st, 0x00499B50u, id, S32(st, props + 0x18)) != 0) {
                f = FIGB_F(st);
                U8(st, f + 0x38) = 9;
                U8(st, f + 0x6B) = U8(st, FIGB_PROPS + 0x38u * U8(st, f + 0x0A) + 0x1C);
            }
        }
        break;
    case 9: {
        uint8_t n = (uint8_t) (U8(st, f + 0x6B) + 1);
        U8(st, f + 0x6B) = n;
        if ((int32_t) n <= S32(st, props + 0x1C)) {
            break;
        }
        shooting = 1;
        if (GAME_CALL(st, 0x00499B50u, id, S32(st, props + 0x18)) == 0) {
            U8(st, FIGB_F(st) + 0x38) = 8;
            break;
        }
        f = FIGB_F(st);
        S16(st, f + 0x36) += 1;
        int32_t d = GAME_CALL(st, 0x0051CD80u, S16(st, f + 0x14), S16(st, f + 0x16),
                              (int32_t) U32(st, 0x00D3B4E4u), (int32_t) U32(st, 0x00D3B4E0u));
        U8(st, f + 0x11) = (uint8_t) d;
        if (S16(st, f + 0x36) == 10) {
            GAME_CALL(st, 0x004B45C0u, FIGB_CUR(st), S16(st, f + 0x14), S16(st, f + 0x16),
                      (int32_t) U32(st, 0x00D3B4E4u), (int32_t) U32(st, 0x00D3B4E0u), 0x3C);
            GAME_CALL(st, 0x0047D350u, 0x0D);
        } else if (S16(st, f + 0x36) >= 0x18) {
            S16(st, f + 0x36) = 0;
            U8(st, f + 0x6B) = 0;
        }
        break;
    }
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    if (U8(st, f + 0x38) == 9 && shooting) {
        U32(st, f + 0x04) = U32(st, 0x00F56E54u) + 8u * (uint32_t) (S16(st, f + 0x36) / 2) + (uint32_t) d;
        return;
    }
    U32(st, f + 0x04) = U32(st, 0x00F56E44u) + (uint32_t) d;
}

/* @bin 0x0049A20B find a building to go to (0x004450C0); sets the destination */
static int find_destination(game_state *st)
{
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t b = GAME_CALL(st, 0x004450C0u, id, S16(st, f + 0x14), S16(st, f + 0x16), 0, -1);
    if (b == 0) {
        return 0;
    }
    f = FIGB_F(st);
    S16(st, f + 0x5E) = (int16_t) b;
    U8(st, f + 0x38) = 8;
    S16(st, f + 0x36) = 0;
    U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
    U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
    U8(st, f + 0x01) = 0;
    return 1;
}

void figb_action_3a(game_state *st)
{
    /* @bin 0x0049A150 a carrier: finds a building (0x004450C0), unloads its resource there
     * (0x0045CF50 / 0x0045D010 / 0x00444BD0), returns */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 0;
    int32_t speed = S32(st, FIGB_PROPS + 0x20u + 0x38u * U8(st, f + 0x0A));
    figb_anim_advance(st, f, 12);
    U32(st, f + 0x180) = 0;
    int8_t dir;
    switch (U8(st, f + 0x38)) {
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 0x0A:
        U8(st, f + 0x65) = 1;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 10) {
            S16(st, f + 0x36) = 0;
            if (find_destination(st)) {
                break;
            }
            f = FIGB_F(st);
            U8(st, f + 0x0E) = 2;
        }
        U8(st, f + 0x01) = 0;
        break;
    case 8:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + 0x11);
        if (dir == 8) {
            U8(st, f + 0x38) = 0x0B;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + 0x0E) = 2;
            U8(st, f + 0x65) = 1;
        }
        f = FIGB_F(st);
        if (U8(st, BUILDING_ADDR(S16(st, f + 0x5E))) != 1) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    case 0x0B:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 10) {
            S16(st, f + 0x36) = 0;
            if (GAME_CALL(st, 0x0045CF50u, id, U8(st, f + 0x0F), S16(st, f + 0x5E), 0) != 0) {
                f = FIGB_F(st);
                int32_t v = GAME_CALL(st, 0x0045D010u, U8(st, f + 0x0F), S16(st, f + 0x5E), 0);
                GAME_CALL(st, 0x00444BD0u, FIGB_CUR(st), v);
                f = FIGB_F(st);
                U8(st, f + 0x6F) += 3;
                U8(st, f + 0x01) = 0;
                break;
            }
            if (find_destination(st)) {
                break;
            }
            f = FIGB_F(st);
            U8(st, f + 0x38) = 9;
            S16(st, f + 0x36) = 0;
            U16(st, f + 0x24) = U16(st, f + 0x2C);
            U16(st, f + 0x26) = U16(st, f + 0x2E);
        }
        U8(st, f + 0x01) = 0;
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
        U8(st, f + 0x38) = 0x0A;
        break;
    }
    int32_t d = figb_dir_rel_prev(st);
    f = FIGB_F(st);
    if (U8(st, f + 0x38) == 4) {
        U32(st, f + 0x04) = U8(st, FIGB_DIE_FRAMES + S16(st, f + 0x36) / 2) + U32(st, 0x00F55E4Cu);
        U32(st, f + 0x180) = 0;
    } else {
        U32(st, f + 0x04) = U32(st, 0x00F55E44u) + 8u * U8(st, f + 0x01) + (uint32_t) d;
    }
    /* @bin 0x0049A55C the carried resource, drawn at offsets by direction (0x005DC23C) */
    uint32_t over = U32(st, 0x00F55D5Cu) + 8u * U8(st, f + 0x0B) + 8u;
    U32(st, f + 0x180) = over;
    if (over != 0) {
        U32(st, f + 0x180) = over + (uint32_t) d;
        U8(st, f + 0x6C) = U8(st, 0x005DC23Cu + 2u * (uint32_t) d);
        U8(st, f + 0x6D) = U8(st, 0x005DC23Du + 2u * (uint32_t) d);
    }
}

/* @bin 0x0049A5FD, 0x0049AC5D a missile hits figure t: damage by the missile's attack (property
 * +0x14) against the target's defence (+0x10, the player's formation experience bonus, facing and
 * formation state at 0x00BCB9CC), up to the target's maximum (+0x04); returns t's formation */
static int32_t missile_damage(game_state *st, int32_t t)
{
    uint32_t tf = FIGURE_ADDR(t);
    int32_t form = S16(st, tf + 0x60);
    uint32_t rec = 0x90u * (uint32_t) form;
    int32_t bonus = 0;
    if (U8(st, tf + 0x0F) == U32(st, CURRENT_PLAYER_ADDR)) {
        bonus = (S16(st, 0x00BCBA42u + rec) + 10) / 20;
    }
    uint32_t tprops = FIGB_PROPS + 0x38u * U8(st, tf + 0x0A);
    int32_t maxdmg = S32(st, tprops + 0x04);
    int32_t def = S32(st, tprops + 0x10) + bonus;
    if (def >= 0x14) {
        def = 0x14;
    }
    int32_t attack = S32(st, FIGB_PROPS + 0x14u + 0x38u * U8(st, FIGB_F(st) + 0x0A));
    if (S16(st, 0x00BCBA22u + rec) != 0) {
        int16_t m = S16(st, 0x00BCB9CCu + rec);
        if (m >= 0x0B && m <= 0x0C) {
            if (GAME_CALL(st, 0x004B1D10u, S8(st, tf + 0x13), U8(st, 0x00BCBA33u + rec)) == 0) {
                def -= 4;
            } else {
                int16_t s = S16(st, 0x00BCB9F2u + rec);
                if (s >= 3 && s <= 4) {
                    def += 4;
                } else if (s == 6) {
                    def -= 2;
                }
            }
        } else if (m == 0x0D) {
            if (GAME_CALL(st, 0x004B1D10u, S8(st, tf + 0x13), U8(st, 0x00BCBA33u + rec)) == 0) {
                def -= 4;
            } else if (S16(st, 0x00BCB9F2u + rec) == 6) {
                def -= 2;
            }
        }
    }
    if (def < 0) {
        def = 0;
    }
    if (def > 0x14) {
        def = 0x14;
    }
    int32_t dmg = 0;
    if (attack != 0) {
        dmg = (int32_t) ((uint32_t) ((30 - def) / 20) * (uint32_t) attack);
        if (dmg < 0) {
            dmg = 0;
        }
    }
    int32_t total = U8(st, tf + 0x1D) + dmg;
    if (total > maxdmg) {
        U8(st, tf + 0x1D) = (uint8_t) (maxdmg + 1);
        U8(st, tf + 0x38) = 4;
        S16(st, tf + 0x36) = 0;
        GAME_CALL(st, 0x004B5050u, t);
        GAME_CALL(st, 0x004B7AB0u, form);
    } else {
        U8(st, tf + 0x1D) = (uint8_t) total;
    }
    return form;
}

void figb_action_3b(game_state *st)
{
    /* @bin 0x0049A5A0 a missile: flies (0x004B3FF0) and hits a figure (0x004B5770) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x0C) = 1;
    int8_t age = (int8_t) (U8(st, f + 0x39) + 1);
    U8(st, f + 0x39) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + 0x0E) = 2;
    }
    int32_t landed = GAME_CALL(st, 0x004B3FF0u, id, 4);
    int32_t t = GAME_CALL(st, 0x004B5770u, FIGB_CUR(st));
    if (t == 0) {
        if (landed != 0) {
            U8(st, FIGB_F(st) + 0x0E) = 2;
        }
    } else {
        int32_t form = missile_damage(st, t);
        f = FIGB_F(st);
        U8(st, f + 0x0E) = 2;
        if (form != 0) {
            uint32_t rec = 0x90u * (uint32_t) form;
            S16(st, 0x00BCBA26u + rec) = 6;
            S16(st, 0x00BCBA28u + rec) = S16(st, FIGURE_ADDR(S16(st, f + 0x5A)) + 0x60);
        }
        GAME_CALL(st, 0x0047D350u, 0x0B);
    }
    /* @bin 0x0049A82E 16 directions */
    f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11) - 2 * S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 0x10;
        S32(st, FIGB_DIR_REL) = d;
    }
    U32(st, f + 0x04) = U32(st, 0x00F56424u) + 0x10u + (uint32_t) d;
}

/* @bin 0x0049A964 figure types whose hit points are at +0x128 (maximum +0x12C) */
static int has_hit_points(uint8_t t)
{
    return t == 0x5D || t == 0x5C || t == 0x64 || t == 0x65;
}

void figb_action_3c(game_state *st)
{
    /* @bin 0x0049A880 a missile shot by the figure at +0x5A: on a hit (0x004B56D0) deals the
     * attack of the shooter's kind (0x3E, 0x4E, or its formation's) against the target's defence */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x0C) = 1;
    int8_t age = (int8_t) (U8(st, f + 0x39) + 1);
    U8(st, f + 0x39) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + 0x0E) = 2;
    }
    int32_t landed = GAME_CALL(st, 0x004B3FF0u, id, 4);
    int32_t t = GAME_CALL(st, 0x004B56D0u, FIGB_CUR(st));
    f = FIGB_F(st);
    uint32_t sf = FIGURE_ADDR(S16(st, f + 0x5A)); /* the shooter */
    int32_t sform = S16(st, sf + 0x60);
    int dead = 0;
    if (t == 0) {
        dead = landed != 0;
    } else {
        uint32_t tf = FIGURE_ADDR(t);
        int32_t tform = S16(st, tf + 0x60);
        int32_t kind;
        uint8_t stype = U8(st, sf + 0x0A);
        if (stype == 0x3E) {
            kind = 0x3E;
        } else if (stype == 0x4E && U8(st, sf + 0x0E) == 2) {
            kind = -1;
        } else if (stype == 0x4E) {
            kind = 0x4E;
        } else {
            kind = S16(st, 0x00BCB9CCu + 0x90u * (uint32_t) sform);
        }
        if (kind >= 0) {
            uint8_t ttype = U8(st, tf + 0x0A);
            int32_t maxv = has_hit_points(ttype) ? S32(st, tf + 0x12C) : S32(st, FIGB_PROPS + 0x04u + 0x38u * ttype);
            int32_t def = S32(st, FIGB_PROPS + 0x10u + 0x38u * ttype);
            if (def >= 0x14) {
                def = 0x14;
            }
            int32_t atk = S32(st, FIGB_PROPS + 0x14u + 0x38u * (uint32_t) kind);
            int32_t dmg = 0;
            if (atk > 0) {
                dmg = (int32_t) ((uint32_t) (0x14 - def) * (uint32_t) atk) / 20;
                if (dmg < 0) {
                    dmg = 0;
                }
            }
            if (has_hit_points(ttype)) {
                int32_t v = S32(st, tf + 0x128) - dmg;
                if (v <= 0) {
                    S32(st, tf + 0x128) = 0;
                    U8(st, tf + 0x38) = 4;
                    S16(st, tf + 0x36) = 0;
                    GAME_CALL(st, 0x004B5050u, t);
                    GAME_CALL(st, 0x004B7AB0u, tform);
                } else {
                    S32(st, tf + 0x128) = v;
                }
            } else {
                int32_t total = U8(st, tf + 0x1D) + dmg;
                if (total > maxv) {
                    U8(st, tf + 0x1D) = (uint8_t) (maxv + 1);
                    U8(st, tf + 0x38) = 4;
                    S16(st, tf + 0x36) = 0;
                    GAME_CALL(st, 0x004B5050u, t);
                    GAME_CALL(st, 0x004B7AB0u, tform);
                    /* @bin 0x0049AA80 a kill by the player's formation raises its experience */
                    if ((int8_t) GAME_CALL(st, 0x004EA9C0u, U8(st, tf + 0x0A)) == 0 &&
                        (int8_t) GAME_CALL(st, 0x004EA9A0u, U8(st, tf + 0x0A)) == 0 && U8(st, tf + 0x0A) != 0x17 &&
                        U8(st, sf + 0x0F) == U32(st, CURRENT_PLAYER_ADDR)) {
                        uint32_t a = 0x00BCBA42u + 0x90u * (uint32_t) sform;
                        int32_t e = S16(st, a) + 1;
                        if (e >= 100) {
                            e = 100;
                        }
                        S16(st, a) = (int16_t) e;
                    }
                } else {
                    U8(st, tf + 0x1D) = (uint8_t) total;
                }
            }
            if (tform != 0) {
                S16(st, 0x00BCBA26u + 0x90u * (uint32_t) tform) = 6;
                S16(st, 0x00BCBA28u + 0x90u * (uint32_t) tform) = (int16_t) sform;
            }
            GAME_CALL(st, 0x0047D350u, 0x18);
        }
        dead = 1;
    }
    f = FIGB_F(st);
    if (dead) {
        U8(st, f + 0x0E) = 2;
    }
    /* @bin 0x0049AB86 32 directions */
    int32_t d = S8(st, f + 0x11) - 4 * S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 0x20;
        S32(st, FIGB_DIR_REL) = d;
    }
    uint8_t stype = U8(st, sf + 0x0A);
    if (stype != 0x2A && stype != 0x3E) {
        U32(st, f + 0x04) = U32(st, 0x00F55CECu) + (uint32_t) d;
        U32(st, f + 0x180) = U32(st, 0x00F55CF4u) + (uint32_t) d;
        return;
    }
    U32(st, f + 0x04) = U32(st, 0x00F56F1Cu) + (uint32_t) d;
    U32(st, f + 0x180) = U32(st, 0x00F56F14u) + (uint32_t) d;
}

void figb_action_3d(game_state *st)
{
    /* @bin 0x0049AC00 a missile hitting figures found by 0x004B56D0 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x0C) = 1;
    int8_t age = (int8_t) (U8(st, f + 0x39) + 1);
    U8(st, f + 0x39) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + 0x0E) = 2;
    }
    int32_t landed = GAME_CALL(st, 0x004B3FF0u, id, 4);
    int32_t t = GAME_CALL(st, 0x004B56D0u, FIGB_CUR(st));
    if (t == 0) {
        if (landed != 0) {
            U8(st, FIGB_F(st) + 0x0E) = 2;
            GAME_CALL(st, 0x0047D350u, 0x0E);
        }
    } else {
        missile_damage(st, t);
        GAME_CALL(st, 0x0047D350u, 0x0F);
        U8(st, FIGB_F(st) + 0x0E) = 2;
    }
    f = FIGB_F(st);
    int32_t d = S8(st, f + 0x11) - 2 * S32(st, FIGB_VIEW_DIR);
    S32(st, FIGB_DIR_REL) = d;
    if (d < 0) {
        d += 0x10;
        S32(st, FIGB_DIR_REL) = d;
    }
    U32(st, f + 0x04) = U32(st, 0x00F56424u) + 0x20u + (uint32_t) d;
}
