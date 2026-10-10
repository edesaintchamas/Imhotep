/* Imhotep - figure actions, group c (action table 0x005DC28C, figure types 0x47 .. 0x68).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). Each action runs on the current figure
 * 0x00D056F8, which it re-reads where the original does. */
#include "figure/actions_c.h"

#include "figure/actions_c2.h"

#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CUR(st) S32(st, G_CURRENT_FIGURE)
#define FIG(id) FIGURE_ADDR(id)


/* @bin 0x004A2FF6, 0x004A3338 (and the same tail in the other actions) the figure's direction
 * +0x11 relative to the view: minus the orientation << shift, plus `wrap` when negative */
static int32_t view_direction(game_state *st, uint32_t f, int shift, int32_t wrap)
{
    int32_t d = (int32_t) ((uint32_t) (int32_t) S8(st, f + FIG_DIRECTION) - ((uint32_t) S32(st, G_VIEW_ORIENTATION) << shift));
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += wrap;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

/* @bin 0x007090EC a 56-byte record per figure type; its first dword is passed to the movement
 * function 0x004A5DA0 (a speed) */
static int32_t type_speed(game_state *st, uint8_t type)
{
    return S32(st, G_PROPS + FIGTYPE_SPEED + 56u * type);
}

/* @bin the image ids of the figure sprite groups (set when the graphics load) */
static int32_t image(game_state *st, uint32_t a)
{
    return S32(st, a);
}

static int32_t add32(int32_t a, int32_t b)
{
    return (int32_t) ((uint32_t) a + (uint32_t) b);
}

/* ---- type 0x49 ------------------------------------------------------------------------------ */

/* @bin 0x00F55BB4 image ids of the sprite groups, 8 bytes per group (the first dword: the id) */
static int32_t image_group(game_state *st, int32_t group)
{
    return S32(st, G_DAY2_REDRAW_GROUPS + 8u * (uint32_t) group);
}

/* @bin 0x004A2320 the nearest prey within range (0 when none) */
static int32_t hunter_find_prey(game_state *st, int32_t x, int32_t y, int32_t range)
{
    return GAME_CALL(st, FN_HUNTER_FIND_PREY, x, y, range);
}

/* @bin 0x004A2560, 0x004A2BB0 the hunter (current figure) goes for the prey: destination its tile,
 * +0x5E the prey; the prey (unless dying, action 0x0C) remembers the hunter's building (+0x138)
 * and the hunter chases it (action 9) */
static void hunter_chase(game_state *st, int32_t prey)
{
    int32_t id = CUR(st);
    uint32_t c = FIG(id), p = FIG(prey);
    U16(st, c + FIG_DEST_X) = U16(st, p + FIG_X);
    U16(st, c + FIG_DEST_Y) = U16(st, p + FIG_Y);
    U16(st, c + FIG_DEST_BUILDING) = (uint16_t) prey;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    if (U8(st, p + FIG_ACTION) != 0x0C) {
        c = FIG(CUR(st));
        S32(st, p + FIG_U138) = S16(st, c + FIG_HOME);
        U8(st, c + FIG_ACTION) = 9;
    }
}

/* the destination: the figure's building's tile (+0x26, +0x28) */
static void head_home(game_state *st, uint32_t c)
{
    uint32_t b = BUILDING_ADDR(S16(st, c + FIG_HOME));
    U16(st, c + FIG_DEST_X) = U16(st, b + BLD_ROAD_ACCESS_X);
    U16(st, c + FIG_DEST_Y) = U16(st, b + BLD_ROAD_ACCESS_Y);
}

void figure_action_49(game_state *st)
{
    /* @bin 0x004A2430 a hunter of a building (+0x5A): looks for prey (0x004A2320), chases it,
     * shoots (a missile of type 0x4A), collects the carcass and brings it home (+100 to the
     * building's +0x3C) */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    int32_t shot_frame = 0; /* [esp+0x18] */
    U8(st, c + FIG_TERRAIN_USAGE) = 3;
    U8(st, c + FIG_U0C) = 0;
    U8(st, c + FIG_INVISIBLE) = 0;
    if (U8(st, BUILDING_ADDR(S16(st, c + FIG_HOME)) + BLD_STATE) != 1) {
        U8(st, c + FIG_STATE) = 2;
        return;
    }
    int wrapped = 0;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
        wrapped = 1;
    }
    uint8_t action = U8(st, c + FIG_ACTION);
    uint8_t dir;
    int32_t prey;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 0x0E:
        U8(st, c + FIG_INVISIBLE) = 1;
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            U8(st, c + FIG_ACTION) = 8;
            U8(st, c + FIG_ANIM_FRAME) = 0;
        }
        break;
    case 8:
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
        if (S16(st, c + FIG_WAIT_TICKS) > 0) {
            break;
        }
        /* @bin 0x004A254D look for prey within 1000 */
        prey = hunter_find_prey(st, S16(st, c + FIG_X), S16(st, c + FIG_Y), 1000);
        if (prey != 0) {
            hunter_chase(st, prey);
            break;
        }
        c = FIG(CUR(st));
        U8(st, c + FIG_ACTION) = 0x0D;
        head_home(st, c);
        if (U16(st, c + FIG_DEST_X) == U16(st, c + FIG_X) && U16(st, c + FIG_DEST_Y) == U16(st, c + FIG_Y)) {
            U8(st, c + FIG_INVISIBLE) = 1;
            U8(st, c + FIG_STATE) = 2;
            return;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        c = FIG(CUR(st));
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0x14;
            break;
        }
        if (dir == 0x0A) {
            U8(st, c + FIG_STATE) = 2;
            break;
        }
        if (!wrapped && dir != 8) {
            break;
        }
        /* @bin 0x004A26E1 prey within 6: aim, and shoot when within 2 tiles */
        prey = hunter_find_prey(st, S16(st, c + FIG_X), S16(st, c + FIG_Y), 6);
        if (prey == 0) {
            c = FIG(CUR(st));
            if (U8(st, c + FIG_DIRECTION) == 8) {
                U8(st, c + FIG_ACTION) = 0x0D;
                head_home(st, c);
            }
            break;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
        {
            uint32_t p = FIG(prey);
            c = FIG(CUR(st));
            U16(st, c + FIG_DEST_X) = U16(st, p + FIG_X);
            U16(st, c + FIG_DEST_Y) = U16(st, p + FIG_Y);
            if (U8(st, p + FIG_ACTION) == 0x0C) {
                break;
            }
            S32(st, p + FIG_U138) = S16(st, c + FIG_HOME);
            int32_t px = S16(st, p + FIG_X), cy = S16(st, c + FIG_Y), py = S16(st, p + FIG_Y), cx = S16(st, c + FIG_X);
            /* @bin 0x004A2781 the distances, kept in 0x00D3B4FC, 0x00D3B4F8 */
            int32_t dx = cx > px ? cx - px : cx < px ? px - cx : 0;
            S32(st, G_DIST_X) = dx;
            int32_t dy = cy > py ? cy - py : cy < py ? py - cy : 0;
            S32(st, G_DIST_Y) = dy;
            if (dx < dy) {
                dx = dy;
            }
            if (dx > 2) {
                break;
            }
            U16(st, c + FIG_DEST_X) = U16(st, p + FIG_X);
            U16(st, c + FIG_DEST_Y) = U16(st, p + FIG_Y);
            int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, c + FIG_X), S16(st, c + FIG_Y), S16(st, p + FIG_X),
                                  S16(st, p + FIG_Y));
            U8(st, c + FIG_DIRECTION) = (uint8_t) d;
            U8(st, c + FIG_ACTION) = 0x0F;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        break;
    case 0x0F:
        /* @bin 0x004A2888 shooting: at frame 9 a sound (by the climate 0x00784CB0) and the missile
         * (type 0x4A) */
        shot_frame = S16(st, c + FIG_WAIT_TICKS);
        if (S16(st, c + FIG_WAIT_TICKS) == 9) {
            if (U8(st, G_U784CB0) == 0) {
                GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, CUR(st), 0x10, 2);
            }
            if (U8(st, G_U784CB0) == 1) {
                GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, CUR(st), 0x0E, 2);
            }
            if (U8(st, G_U784CB0) == 2) {
                GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, CUR(st), 0x0F, 2);
            }
            c = FIG(CUR(st));
            GAME_CALL(st, FN_MISSILE_CREATE, CUR(st), S16(st, c + FIG_X), S16(st, c + FIG_Y), S16(st, c + FIG_DEST_X),
                      S16(st, c + FIG_DEST_Y), 0x4A);
        }
        if (shot_frame >= 0x0C) {
            shot_frame = 0;
        }
        c = FIG(CUR(st));
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x12C) {
            U8(st, c + FIG_ACTION) = 8;
        }
        break;
    case 0x0B:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        c = FIG(CUR(st));
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            /* @bin 0x004A2991 at the carcass (+0x5E, dying on the same tile): take it (action 0x0A) */
            int32_t k = S16(st, c + FIG_DEST_BUILDING);
            uint32_t p = FIG(k);
            if (k != 0 && S32(st, p + FIG_GRID_OFFSET) == S32(st, c + FIG_GRID_OFFSET) && U8(st, p + FIG_STATE) != 0 &&
                U8(st, p + FIG_ACTION) == 0x0C) {
                U8(st, p + FIG_STATE) = 2;
                U8(st, c + FIG_ACTION) = 0x0A;
                U16(st, c + FIG_WAIT_TICKS) = 0;
            } else {
                U8(st, c + FIG_ACTION) = 8;
                U16(st, c + FIG_WAIT_TICKS) = 0x14;
            }
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        } else if (dir == 0x0A) {
            U8(st, c + FIG_STATE) = 2;
        }
        break;
    case 0x0A: {
        /* @bin 0x004A2A3D taking the carcass: 18 ticks (0 when the climate 0x00784CB0 is 0) */
        int32_t wait = U8(st, G_U784CB0) != 0 ? 0x12 : 0;
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > wait) {
            head_home(st, c);
            U8(st, c + FIG_ACTION) = 0x0C;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
        break;
    }
    case 0x0C:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        c = FIG(CUR(st));
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, c + FIG_STATE) = 2;
            U16(st, BUILDING_ADDR(S16(st, c + FIG_HOME)) + BLD_STORED) += 0x64;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
        } else if (dir == 0x0A) {
            U8(st, c + FIG_STATE) = 2;
        }
        break;
    case 0x0D:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        if (wrapped) {
            c = FIG(CUR(st));
            prey = hunter_find_prey(st, S16(st, c + FIG_X), S16(st, c + FIG_Y), 0x64);
            if (prey != 0) {
                hunter_chase(st, prey);
            }
        }
        c = FIG(CUR(st));
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
            c = FIG(CUR(st));
            U8(st, c + FIG_STATE) = 2;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
        } else if (dir == 0x0A) {
            U8(st, c + FIG_STATE) = 2;
        }
        break;
    case 7:
        U8(st, c + FIG_ANIM_FRAME) = 0;
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x64) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) figure_may_leave(st, id) == 0) {
                U8(st, FIG(CUR(st)) + FIG_ACTION) = 8;
            }
        }
        break;
    case 6:
        U8(st, c + FIG_INVISIBLE) = 1;
        break;
    default:
        U8(st, c + FIG_ACTION) = 8;
        break;
    }
    c = FIG(CUR(st));
    int32_t d = view_direction(st, c, 0, 8);
    /* @bin 0x004A2AC7 the sprite groups by the climate 0x00784CB0 */
    static const int16_t groups[3][7] = {
        /* walk, die, shoot, carry, take, a, b */
        {0xDF, 0xE0, 0xE1, 0xDF, 0xDF, 0xE1, 0xE1},
        {0x265, 0x266, 0x267, 0x26B, 0x26A, 0x268, 0x269},
        {0x54, 0x55, 0x56, 0x5A, 0x59, 0x57, 0x58},
    };
    uint8_t mode = U8(st, G_U784CB0);
    const int16_t *g = groups[mode == 0 ? 0 : mode == 1 ? 1 : 2];
    int32_t walk = g[0], die = g[1], shoot = g[2], carry = g[3], take = g[4], g14 = g[5], g10 = g[6];
    uint32_t fr8 = 8u * U8(st, c + FIG_ANIM_FRAME);
    switch (U8(st, c + FIG_ACTION)) {
    case 4: {
        int32_t k = S16(st, c + FIG_WAIT_TICKS) / 2;
        S32(st, c + FIG_U04) = add32(U8(st, (uint32_t) ((int32_t) G_DIE_FRAMES + k)), image_group(st, die));
        return;
    }
    case 5:
        S32(st, c + FIG_U04) = add32(add32(image_group(st, U8(st, c + FIG_ACTION_BEFORE_ATTACK) == 0x0C ? g10 : g14), (int32_t) fr8), d);
        return;
    case 6:
        return;
    case 7:
        S32(st, c + FIG_U04) = add32(image_group(st, U8(st, c + FIG_ACTION_BEFORE_ATTACK) == 0x0C ? carry : walk), d);
        return;
    case 0x0A:
        S32(st, c + FIG_U04) = add32(add32(image_group(st, take), (int32_t) (8u * (uint32_t) (int32_t) S16(st, c + FIG_WAIT_TICKS))), d);
        return;
    case 0x0C:
        S32(st, c + FIG_U04) = add32(add32(image_group(st, carry), (int32_t) fr8), d);
        return;
    case 0x0E:
        S32(st, c + FIG_U04) = 0;
        return;
    case 0x0F:
        S32(st, c + FIG_U04) = add32(add32(image_group(st, shoot), (int32_t) (8u * (uint32_t) shot_frame)), d);
        return;
    default:
        S32(st, c + FIG_U04) = add32(add32(image_group(st, walk), (int32_t) fr8), d);
        return;
    }
}

/* @bin 0x0047D350 returns 0 (xor eax, eax; ret) */
int32_t return_zero(game_state *st, int32_t a)
{
    (void) st;
    (void) a;
    return 0;
}

int32_t tile_figure_of_type(game_state *st, int32_t id, int32_t type)
{
    /* @bin 0x004B57E0 the first figure of the type in the list of the figure's tile (+0x20) */
    int32_t k = S16(st, G_FIGURE_GRID + 2u * U32(st, FIG(id) + FIG_GRID_OFFSET));
    while (k > 0) {
        uint32_t g = FIG(k);
        if ((int32_t) U8(st, g + FIG_TYPE) == type) {
            return k;
        }
        k = S16(st, g + FIG_NEXT_ON_TILE);
    }
    return 0;
}

int32_t tile_target(game_state *st, int32_t id)
{
    /* @bin 0x004B5770 the first figure of the tile's list that a missile hits: not dying, not of
     * the types 0, 6, 0x0E, 0x24, 0x25, nor 0x29 .. 0x4C, nor above 0x4E */
    int32_t k = S16(st, G_FIGURE_GRID + 2u * U32(st, FIG(id) + FIG_GRID_OFFSET));
    while (k > 0) {
        uint32_t g = FIG(k);
        uint8_t t = U8(st, g + FIG_TYPE);
        if (U8(st, g + FIG_ACTION) != 4 && t != 0 && t != 6 && t != 0x0E && t != 0x24 && t != 0x25 &&
            (t < 0x29 || t >= 0x4D) && t <= 0x4E) {
            return k;
        }
        k = S16(st, g + FIG_NEXT_ON_TILE);
    }
    return 0;
}

/* ---- type 0x4A ------------------------------------------------------------------------------ */

void figure_action_4a(game_state *st)
{
    /* @bin 0x004A2E90 a missile: flies 0x68 ticks at most, hits a figure of type 0x46, 0x44 or 0x45
     * on its way */
    int32_t id = CUR(st);
    uint32_t f = FIG(id);
    U8(st, f + FIG_U0C) = 1;
    int8_t age = (int8_t) (U8(st, f + FIG_PROGRESS_ON_TILE) + 1);
    U8(st, f + FIG_PROGRESS_ON_TILE) = (uint8_t) age;
    if (age > 0x68) { /* @bin 0x004A2EB0 signed */
        U8(st, f + FIG_STATE) = 2;
    }
    uint8_t frame = (uint8_t) (U8(st, f + FIG_ANIM_FRAME) + 1);
    U8(st, f + FIG_ANIM_FRAME) = frame;
    if (frame >= 5) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    int32_t arrived = GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 3);
    int32_t hit = tile_figure_of_type(st, CUR(st), 0x46);
    if (hit == 0) {
        hit = tile_figure_of_type(st, CUR(st), 0x44);
        if (hit == 0) {
            hit = tile_figure_of_type(st, CUR(st), 0x45);
        }
    }
    int32_t cur;
    uint8_t ha = hit ? U8(st, FIG(hit) + FIG_ACTION) : 0;
    if (hit != 0 && ha != 0x0C && ha != 4) {
        /* @bin 0x004A2F48 the prey dies (action 0x0C); the shooter (+0x5A) goes for it (action 0x0B,
         * destination its tile) */
        uint32_t h = FIG(hit);
        U8(st, h + FIG_ACTION) = 0x0C;
        U16(st, h + FIG_WAIT_TICKS) = 0;
        uint32_t s = FIG(S16(st, FIG(CUR(st)) + FIG_SHOOTER));
        U16(st, s + FIG_PREY) = (uint16_t) hit;
        U8(st, s + FIG_ACTION) = 0x0B;
        U16(st, s + FIG_DEST_X) = U16(st, h + FIG_X);
        U16(st, s + FIG_DEST_Y) = U16(st, h + FIG_Y);
        return_zero(st, 0x18);
        cur = CUR(st);
        U8(st, FIG(cur) + FIG_STATE) = 2;
    } else {
        cur = CUR(st);
        if (arrived != 0) {
            uint32_t c = FIG(cur);
            U8(st, FIG(S16(st, c + FIG_SHOOTER)) + FIG_ACTION) = 8;
            U8(st, c + FIG_STATE) = 2;
        }
    }
    f = FIG(cur);
    int32_t d = view_direction(st, f, 2, 0x20);
    if (U8(st, G_U784CB0) == 0) {
        S32(st, f + FIG_U04) = (int32_t) U8(st, f + FIG_ANIM_FRAME) + S32(st, G_IMAGE_GROUP_50);
        S32(st, f + FIG_U180) = 0;
    } else {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_39) + d;
        S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUP_40) + d;
    }
}

/* ---- type 0x47 ------------------------------------------------------------------------------ */

/* @bin 0x00BCB9CC formation records (0x90 bytes), indexed by a figure's +0x60 */
/* a formation record + FM_TYPE, as the original addresses it: a field is FORMATION(i) - FM_TYPE + FM_* */
#define FORMATION(i) ((uint32_t) ((int32_t) (G_FORMATION_BASE + FM_TYPE) + 0x90 * (int32_t) (i)))

/* @bin 0x004A1FE5 x / 20, truncated (imul 0x66666667, sar 3) */
static int32_t div20(int32_t x)
{
    return x / 20;
}

void figure_action_47(game_state *st)
{
    /* @bin 0x004A1EF0 a missile of a shooter (+0x5A): flies 0x78 ticks at most, damages the figure
     * it hits (0x004B5770) */
    int32_t id = CUR(st);
    uint32_t f = FIG(id);
    U8(st, f + FIG_U0C) = 1;
    int8_t age = (int8_t) (U8(st, f + FIG_PROGRESS_ON_TILE) + 1);
    U8(st, f + FIG_PROGRESS_ON_TILE) = (uint8_t) age;
    if (age > 0x78) { /* @bin 0x004A1F1C signed */
        U8(st, f + FIG_STATE) = 2;
    }
    int32_t arrived = GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 4);
    int32_t target = tile_target(st, CUR(st));
    uint32_t c = FIG(CUR(st));
    int32_t shooter = S16(st, c + FIG_HOME);
    int16_t shooter_formation = S16(st, FIG(shooter) + FIG_FORMATION);
    int32_t max_damage = 0, have_max = 0; /* [esp+0x10] */
    if (target == 0) {
        if (arrived != 0) {
            U8(st, c + FIG_STATE) = 2;
        }
    } else {
        uint32_t t = FIG(target);
        int32_t formation = S16(st, t + FIG_FORMATION);
        int32_t bonus = 0;
        uint8_t ttype = U8(st, t + FIG_TYPE);
        int from_record = 0;
        /* @bin 0x004A1FA3 the target is the current player's (+0x0F) */
        if ((uint32_t) U8(st, t + FIG_OWNER) == U32(st, CURRENT_PLAYER_ADDR)) {
            if (ttype >= 0x0B && ttype <= 0x0D) {
                /* @bin 0x004A1FCC (formation +0x76 + 10) / 20 */
                bonus = div20(S16(st, FORMATION(formation) - FM_TYPE + FM_EXPERIENCE) + 10);
            } else if (ttype == 0x4D) {
                max_damage = S32(st, t + FIG_HIT_POINTS_MAX);
                from_record = 1;
            } else if (ttype == 0x4E) {
                max_damage = S32(st, t + FIG_HIT_POINTS_MAX);
                from_record = 1;
                if (U8(st, t + FIG_ACTION) == 0x14) {
                    bonus = 2;
                }
            }
        }
        if (!from_record) {
            max_damage = S32(st, G_PROPS + FIGTYPE_MAX_DAMAGE + 56u * ttype);
        }
        have_max = 1;
        /* @bin 0x004A203A the target type's defence (+0x0C of the type record), at most 20 */
        int32_t defence = bonus + S32(st, G_PROPS + FIGTYPE_U10 + 56u * ttype);
        if (defence >= 0x14) {
            defence = 0x14;
        }
        int dead_shooter = 0;
        uint8_t attacker;
        if (ttype == 0x4E || ttype == 0x4D || ttype == 0x19) {
            uint32_t sf = FIG(shooter);
            if (U8(st, sf + FIG_STATE) == 2) {
                dead_shooter = 1;
            }
            attacker = U8(st, sf + FIG_TYPE);
        } else {
            attacker = U8(st, c + FIG_TYPE);
        }
        if (dead_shooter) {
            U8(st, c + FIG_STATE) = 2;
        } else {
            /* @bin 0x004A20A7 the attacker type's attack (+0x10 of the type record) */
            int32_t attack = S32(st, G_PROPS + FIGTYPE_U14 + 56u * attacker);
            uint32_t fm = FORMATION(formation);
            if (U16(st, fm - FM_TYPE + FM_U5A) != 0) {
                int32_t kind = S16(st, fm);
                if (kind >= 0x0B && kind <= 0x0D) {
                    int32_t r = GAME_CALL(st, FN_DIRECTION_NEAR, S8(st, t + FIG_ATTACK_DIRECTION), U8(st, fm - FM_TYPE + FM_FACING));
                    int32_t layout = S16(st, fm - FM_TYPE + FM_LAYOUT);
                    if (r == 0) {
                        defence -= 4;
                    } else if (kind <= 0x0C) {
                        if (layout >= 3 && layout <= 4) {
                            defence += 4;
                        } else if (layout == 6) {
                            defence -= 2;
                        }
                    } else if (layout == 6) {
                        defence -= 2;
                    }
                }
            }
            int32_t def = defence < 0 ? 0 : defence;
            if (def > 0x14) {
                def = 0x14;
            }
            int32_t damage = 0;
            if (attack != 0) {
                damage = (int32_t) ((uint32_t) div20(0x1E - def) * (uint32_t) attack);
                if (damage < 0) {
                    damage = 0;
                }
            }
            if (ttype == 0x4E || ttype == 0x4D || ttype == 0x19) {
                int32_t hp = S32(st, t + FIG_HIT_POINTS) - damage;
                if (hp > 0) {
                    S32(st, t + FIG_HIT_POINTS) = hp;
                } else {
                    S32(st, t + FIG_HIT_POINTS) = 0;
                    U8(st, t + FIG_ACTION) = 4;
                    U16(st, t + FIG_WAIT_TICKS) = 0;
                    GAME_CALL(st, FN_FIGURE_HIT_SOUND, target);
                }
            } else {
                int32_t dmg = add32(U8(st, t + FIG_DAMAGE), damage);
                if (dmg > max_damage) {
                    U8(st, t + FIG_DAMAGE) = (uint8_t) (max_damage + 1);
                    U8(st, t + FIG_ACTION) = 4;
                    U16(st, t + FIG_WAIT_TICKS) = 0;
                    GAME_CALL(st, FN_FIGURE_HIT_SOUND, target);
                    GAME_CALL(st, FN_FORMATION_MORALE_HIT, target);
                } else {
                    U8(st, t + FIG_DAMAGE) = (uint8_t) dmg;
                }
                /* @bin 0x004A21D5 the target formation is under attack (+0x5A = 6) by the
                 * shooter's formation (+0x5C) */
                U16(st, fm - FM_TYPE + FM_U5E) = 6;
                U16(st, fm - FM_TYPE + FM_U60) = (uint16_t) shooter_formation;
            }
            return_zero(st, 0x18);
            c = FIG(CUR(st));
            U8(st, c + FIG_STATE) = 2;
        }
    }
    int32_t d = view_direction(st, c, 2, 0x20);
    if (U8(st, c + FIG_TYPE) == 0x2B) {
        if (!have_max) {
            state_unported(st, FN_FIGURE_ACTION_47); /* the original reads an uninitialised local here */
            return;
        }
        if (U8(st, FORMATION(max_damage) - FM_TYPE + FM_KIND) == 4) {
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_621), d);
            S32(st, c + FIG_U180) = add32(image(st, G_IMAGE_GROUP_620), d);
            return;
        }
    }
    S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_39), d);
    S32(st, c + FIG_U180) = add32(image(st, G_IMAGE_GROUP_40), d);
}

/* ---- type 0x4B ------------------------------------------------------------------------------ */


void figure_action_4b(game_state *st)
{
    /* @bin 0x004A3070 a worker of a building (+0x5A) that walks to a tile, works there 300 ticks,
     * clears the tile's byte in grid 0x00A43EAC and comes back (+25 to the building's +0x3C) */
    int32_t id = CUR(st);
    uint32_t f = FIG(id);
    int32_t speed = type_speed(st, U8(st, f + FIG_TYPE));
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    U8(st, f + FIG_INVISIBLE) = 0;
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, b + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    uint8_t frame = (uint8_t) (U8(st, f + FIG_ANIM_FRAME) + 1);
    U8(st, f + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    uint8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        id = CUR(st);
        break;
    case 6:
        U8(st, f + FIG_INVISIBLE) = 1;
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S32(st, f + FIG_U04) = add32(image(st, G_IMAGE_GROUP_112), S32(st, G_SPRITE_DIRECTION));
        U16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            U16(st, f + FIG_WAIT_TICKS) = 0;
            uint8_t r = (uint8_t) figure_may_leave(st, id);
            id = CUR(st);
            if (r == 0) {
                U8(st, FIG(id) + FIG_ACTION) = 8;
            }
        }
        break;
    case 8:
        U16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            int32_t found = find_tree(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0xFF, 1);
            id = CUR(st);
            f = FIG(id);
            if (found != 0) {
                /* @bin 0x004A3156 the tile found (0x00D3B4E4, 0x00D3B4E0) */
                U8(st, f + FIG_ACTION) = 9;
                U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
                U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
            } else {
                U8(st, f + FIG_STATE) = 2;
            }
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        f = FIG(id);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            /* @bin 0x004A31CC arrived: work (action 0x0A) unless the tile is taken by another
             * figure in action 0x0A */
            int32_t g = S32(st, f + FIG_GRID_OFFSET);
            if (U8(st, (uint32_t) ((int32_t) G_GRID_VEGETATION + g)) >= 0xFF) {
                uint16_t o = U16(st, (uint32_t) ((int32_t) G_FIGURE_GRID + 2 * g));
                if (o == 0 || U8(st, FIG((int16_t) o) + FIG_ACTION) != 0x0A) {
                    U8(st, f + FIG_ACTION) = 0x0A;
                } else {
                    U8(st, f + FIG_ACTION) = 8;
                }
            } else {
                U8(st, f + FIG_ACTION) = 8;
            }
            U16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        } else if (dir == 0x0A) {
            U8(st, f + FIG_ACTION) = 4;
            U16(st, f + FIG_WAIT_TICKS) = 0;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        }
        break;
    case 0x0A:
        U16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x12C) {
            int32_t g = add32(S32(st, G_MAP_START),
                              add32(S16(st, f + FIG_X), (int32_t) ((uint32_t) (int32_t) S16(st, f + FIG_Y) * 228u)));
            U8(st, (uint32_t) ((int32_t) G_GRID_VEGETATION + g)) = 0;
            GAME_CALL(st, FN_TREE_IMAGE_UPDATE, g);
            id = CUR(st);
            f = FIG(id);
            U8(st, f + FIG_ACTION) = 0x0B;
            U16(st, f + FIG_DEST_X) = U16(st, b + BLD_ROAD_ACCESS_X);
            U16(st, f + FIG_DEST_Y) = U16(st, b + BLD_ROAD_ACCESS_Y);
        }
        break;
    case 0x0B:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        f = FIG(id);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_STATE) = 2;
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U16(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_STORED) += 0x19;
        } else if (dir == 0x0A) {
            U8(st, f + FIG_ACTION) = 4;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        U16(st, f + FIG_WAIT_TICKS) = 0x0A;
        break;
    }
    f = FIG(id);
    int32_t d = view_direction(st, f, 0, 8);
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 4) {
        /* @bin 0x004A3370 dying: frame table 0x005D048C by +0x36 / 2 */
        int32_t k = S16(st, f + FIG_WAIT_TICKS) / 2;
        S32(st, f + FIG_U04) = add32(U8(st, (uint32_t) ((int32_t) G_DIE_FRAMES + k)), image(st, G_IMAGE_GROUP_113));
    } else if (action == 0x0A) {
        S32(st, f + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_114), 8 * U8(st, f + FIG_ANIM_FRAME)), d);
    } else if (action == 8) {
        S32(st, f + FIG_U04) = add32(image(st, G_IMAGE_GROUP_112), d);
    } else if (action == 0x0B) {
        S32(st, f + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_115), 8 * U8(st, f + FIG_ANIM_FRAME)), d);
    } else {
        S32(st, f + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_112), 8 * U8(st, f + FIG_ANIM_FRAME)), d);
    }
}

/* ---- type 0x4C ------------------------------------------------------------------------------ */

/* @bin 0x00EAAF68 an object of 224 bytes per player (+0xCA of a building); 0x004F6630 (thiscall)
 * counts what waits there */
static int32_t ferry_waiting(game_state *st, uint8_t player)
{
    return GAME_CALLT0(st, G_FERRY_QUEUES + 224u * player, FN_QUEUE_LENGTH);
}

/* @bin 0x004A3DA0 at most 4 (the count is read again when below) */
static int32_t ferry_load(game_state *st, uint8_t player)
{
    if (ferry_waiting(st, player) >= 4) {
        return 4;
    }
    return ferry_waiting(st, player);
}

/* @bin 0x009784D8 a record of 44 bytes per ferry (+0x11C): a flag, then the passengers (dwords) */

int32_t ferry_passengers(game_state *st, int32_t id)
{
    /* @bin 0x004A34B0 the passenger slots in use (> 0) among the first 4 of the ferry's record */
    uint32_t p = (uint32_t) ((int32_t) G_FERRY_SLOTS + 4 + 44 * S32(st, FIG(id) + FIG_U11C));
    int32_t n = 0;
    for (int i = 0; i < 4; i++) {
        if (S32(st, p + 4u * (uint32_t) i) > 0) {
            n++;
        }
    }
    return n;
}

int32_t figure_may_leave(game_state *st, int32_t id)
{
    /* @bin 0x004A35E0 1 unless the figure waits for a ferry (action 7) and none of the 50 ferry
     * queues 0x00EAAF68 (224 bytes each, in use when the first dword is set) holds it
     * (0x004F6570, thiscall) */
    uint8_t a = U8(st, FIG(id) + FIG_ACTION);
    if (a != 7) { /* @bin 0x004A35F5 (6 is tested first, with the same result) */
        return 1;
    }
    for (uint32_t q = G_FERRY_QUEUES; q < G_FERRY_QUEUES + 50u * 0xE0u; q += 0xE0u) {
        if (U32(st, q) != 0 && GAME_CALLT(st, q, FN_QUEUE_CONTAINS, id) != 0) {
            return 1;
        }
    }
    return 0;
}

/* @bin 0x004A37BE the figure that leads `k` (by its type: 0x00492360, 0x0049B3C0, 0x004A3540),
 * or 0 for the other types */
static int32_t leader_of(game_state *st, int32_t k)
{
    switch (U8(st, FIG(k) + FIG_TYPE)) {
    case 0x15:
        return GAME_CALL(st, FN_FIGB_LEADER_OF, k);
    case 0x42:
        return GAME_CALL(st, FN_FIGB_CHAIN_HEAD_27, k);
    case 0x56:
    case 0x60:
        return GAME_CALL(st, FN_FIGURE_SLEDGE_LEADER_4A3540, k);
    default:
        return -1; /* not called */
    }
}

static int has_leader_function(game_state *st, int32_t k)
{
    uint8_t t = U8(st, FIG(k) + FIG_TYPE);
    return t == 0x15 || t == 0x42 || t == 0x56 || t == 0x60;
}

void ferry_board(game_state *st, int32_t id, int32_t building, int32_t n)
{
    /* @bin 0x004A36E0 up to n figures waiting in the queue of the building's player (+0xCA) board
     * the ferry: each one waiting (action 7) takes a passenger slot (action 6), leaves the map
     * (routes and tile lists), and so do the figures it leads */
    uint32_t q = G_FERRY_QUEUES + 224u * U8(st, BUILDING_ADDR(building) + BLD_UCA);
    int32_t slot = S32(st, FIG(id) + FIG_U11C);
    if (n >= GAME_CALLT0(st, q, FN_QUEUE_LENGTH)) {
        n = GAME_CALLT0(st, q, FN_QUEUE_LENGTH);
    }
    for (int32_t j = 0; j < n; j++) {
        int32_t pid = GAME_CALLT0(st, q, FN_QUEUE_POP);
        uint32_t pf = FIG(pid);
        if (U8(st, pf + FIG_STATE) != 1 || U8(st, pf + FIG_ACTION) != 7) {
            continue;
        }
        U8(st, pf + FIG_ACTION) = 6;
        S32(st, pf + FIG_U04) = 0;
        S32(st, (uint32_t) ((int32_t) G_FERRY_SLOTS + 4 + 44 * slot + 4 * j)) = pid;
        if (S32(st, pf + FIG_U180) != 0) {
            S32(st, pf + FIG_U180) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, pid);
        GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, pid);
        for (int32_t k = 1; k < FIGURE_COUNT; k++) {
            if (has_leader_function(st, k) && leader_of(st, k) == pid) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, k);
                GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, k);
            }
        }
    }
}

/* a figure put on the tile (x, y) (FOUND_X, FOUND_Y of 0x00480180): +0x14 .. +0x1A, +0x20 its
 * grid offset, +0x4A, +0x4C the position x 15 */
static void place_on_tile(game_state *st, uint32_t f, int32_t x, int32_t y)
{
    U16(st, f + FIG_PREV_X) = (uint16_t) x;
    U16(st, f + FIG_X) = (uint16_t) x;
    U16(st, f + FIG_PREV_Y) = (uint16_t) y;
    U16(st, f + FIG_Y) = (uint16_t) y;
    S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    U16(st, f + FIG_CC_X) = (uint16_t) (U16(st, f + FIG_X) * 15);
    U16(st, f + FIG_CC_Y) = (uint16_t) (U16(st, f + FIG_Y) * 15);
}

void ferry_unload(game_state *st, int32_t id, int32_t building)
{
    /* @bin 0x004A38A0 the passengers get off at the building's road tile (0x00480180), back to
     * their action before boarding (+0x10), with the figures they lead */
    int32_t slot = S32(st, FIG(id) + FIG_U11C);
    int32_t n = ferry_passengers(st, id);
    if (n <= 0) {
        return;
    }
    uint32_t p = (uint32_t) ((int32_t) G_FERRY_SLOTS + 4 + 44 * slot);
    for (; n > 0; n--, p += 4) {
        int32_t pid = S32(st, p);
        S32(st, p) = -1;
        uint32_t pf = FIG(pid);
        if (U8(st, pf + FIG_STATE) != 1 || U8(st, pf + FIG_ACTION) != 6) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(building);
        GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2);
        place_on_tile(st, pf, S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y));
        U8(st, pf + FIG_ACTION) = U8(st, pf + FIG_ACTION_BEFORE_ATTACK);
        U8(st, pf + FIG_PROGRESS_ON_TILE) = 0x0F;
        GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, pid);
        for (int32_t k = 1; k < FIGURE_COUNT; k++) {
            if (has_leader_function(st, k) && leader_of(st, k) == pid) {
                place_on_tile(st, FIG(k), S16(st, pf + FIG_X), S16(st, pf + FIG_Y));
                U8(st, pf + FIG_PROGRESS_ON_TILE) = 0x0F;
                GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, k);
            }
        }
    }
}

void ferry_move(game_state *st, int32_t id, int32_t slowness)
{
    /* @bin 0x004A3B00 one step (0x004B27E0) except every `slowness`-th tick of +0x36 (which
     * restarts after 100) */
    if (slowness > 1) {
        uint32_t f = FIG(id);
        U16(st, f + FIG_WAIT_TICKS) += 1;
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        if (t % slowness != 0) {
            GAME_CALL(st, FN_FIGURE_ADVANCE, id, 1);
            return;
        }
        if (t > 0x64) {
            U16(st, f + FIG_WAIT_TICKS) = 0;
        }
        return;
    }
    GAME_CALL(st, FN_FIGURE_ADVANCE, id, 1);
}

void figure_action_4c(game_state *st)
{
    /* @bin 0x004A3B70 a ferry between its building (+0x5A) and the building's partner (+0xC4):
     * loads what waits (0x004A36E0), crosses, unloads (0x004A38A0); sinks when either side is
     * gone or has no workers */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int waiting_partner = 0; /* [esp+0x13] */
    U8(st, c + FIG_BOAT_MODE) = 1;
    int32_t a_id = S16(st, c + FIG_HOME);
    uint32_t a = BUILDING_ADDR(a_id);
    int32_t b_id = S32(st, a + BLD_FERRY_PARTNER);
    uint8_t a_player = U8(st, a + BLD_UCA);
    uint32_t b = BUILDING_ADDR(b_id);
    int32_t partner = S16(st, b + BLD_WALKER);
    uint8_t b_player = U8(st, b + BLD_UCA);
    /* @bin 0x004A3BE6 the building type's worker count (table 0x00705EB8, 40 bytes per type) */
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, a + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 40u * (uint32_t) (int32_t) S16(st, a + BLD_TYPE)));
    int32_t slowness = (int32_t) (100u - (uint32_t) pct) / 10;
    id = CUR(st);
    c = FIG(id);
    int32_t slot = S32(st, c + FIG_U11C);
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    uint8_t action = U8(st, c + FIG_ACTION);
    int sink = 0;
    uint8_t dir;
    switch (action) {
    case 4:
        U8(st, c + FIG_STATE) = 2;
        break;
    case 8:
        if (ferry_passengers(st, id) > 0) {
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 9;
        } else {
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 0x0A;
        }
        if ((U8(st, a + BLD_STATE) == 2 && U8(st, b + BLD_STATE) == 2) || U16(st, a + BLD_WORKERS) == 0 || U16(st, b + BLD_WORKERS) == 0) {
            sink = 1;
        }
        break;
    case 9:
    case 0x0A:
        ferry_move(st, id, slowness);
        if (U8(st, a + BLD_STATE) == 2 && U8(st, b + BLD_STATE) == 2) {
            c = FIG(CUR(st));
            sink = 1;
            break;
        }
        c = FIG(CUR(st));
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, c + FIG_ACTION) = action == 9 ? 0x0E : 0x0C;
        } else if (dir == 9 || dir == 0x0A) {
            U8(st, c + FIG_ACTION) = 8;
        }
        break;
    case 0x0B:
        if (b_id <= 0 || U8(st, a + BLD_STATE) == 2 || U8(st, b + BLD_STATE) == 2 || U16(st, a + BLD_WORKERS) == 0 || U16(st, b + BLD_WORKERS) == 0) {
            sink = 1;
            break;
        }
        if (U8(st, FIG(partner) + FIG_ACTION) == 9) {
            waiting_partner = 1;
        }
        {
            /* @bin 0x004A3D58 leaves when something waits and (100 ticks have passed, 4 wait, or
             * the partner is crossing) */
            int go = 0;
            if (ferry_waiting(st, a_player) != 0) {
                uint32_t cc = FIG(CUR(st));
                U16(st, cc + FIG_WAIT_TICKS) += 1;
                if (S16(st, cc + FIG_WAIT_TICKS) > 0x64 || ferry_waiting(st, a_player) >= 4) {
                    go = 1;
                }
            }
            if (!go && waiting_partner) {
                go = 1;
            }
            if (!go) {
                break;
            }
            int32_t n = ferry_load(st, a_player);
            ferry_board(st, CUR(st), a_id, n);
            U16(st, FIG(CUR(st)) + FIG_DEST_BUILDING) = (uint16_t) b_id;
            GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
            c = FIG(CUR(st));
            U16(st, c + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
            U16(st, c + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
            U16(st, c + FIG_SOURCE_X) = U16(st, c + FIG_X);
            U16(st, c + FIG_SOURCE_Y) = U16(st, c + FIG_Y);
            U8(st, c + FIG_ACTION) = 9;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        break;
    case 0x0C:
        ferry_unload(st, id, a_id);
        U8(st, FIG(CUR(st)) + FIG_ACTION) = 0x0B;
        break;
    case 0x0D:
        if (ferry_waiting(st, b_player) != 0) {
            int32_t n = ferry_load(st, b_player);
            ferry_board(st, CUR(st), b_id, n);
        }
        c = FIG(CUR(st));
        U16(st, c + FIG_DEST_BUILDING) = U16(st, c + FIG_HOME);
        U8(st, c + FIG_ACTION) = 0x0A;
        U16(st, c + FIG_WAIT_TICKS) = 0;
        U16(st, c + FIG_DEST_X) = U16(st, c + FIG_SOURCE_X);
        U16(st, c + FIG_DEST_Y) = U16(st, c + FIG_SOURCE_Y);
        break;
    case 0x0E:
        ferry_unload(st, id, b_id);
        U8(st, FIG(CUR(st)) + FIG_ACTION) = 0x0D;
        break;
    default:
        break;
    }
    if (sink) {
        /* @bin 0x004A3FDB sinking (action 4); the ferry's record is cleared */
        if (U8(st, c + FIG_ACTION) != 4) {
            U16(st, c + FIG_WAIT_TICKS) = 0xFFFF;
        }
        U8(st, c + FIG_ACTION) = 4;
        U8(st, (uint32_t) ((int32_t) G_FERRY_SLOTS + 44 * slot)) = 0;
    }
    id = CUR(st);
    c = FIG(id);
    int32_t d = S8(st, c + FIG_DIRECTION) >= 8 ? S8(st, c + FIG_PREV_DIRECTION) : S8(st, c + FIG_DIRECTION);
    d = (int32_t) ((uint32_t) d - (uint32_t) S32(st, G_VIEW_ORIENTATION));
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (U8(st, c + FIG_ACTION) == 4) {
        /* @bin 0x004A408C sunk: the passengers (0x004A34B0 of them) go too */
        U8(st, c + FIG_STATE) = 2;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        int32_t n = ferry_passengers(st, CUR(st));
        if (n > 0) {
            uint32_t p = (uint32_t) ((int32_t) G_FERRY_SLOTS + 4 + 44 * slot);
            for (int32_t i = 0;;) {
                int32_t v = S32(st, p);
                if (v > 0) {
                    S32(st, p) = -1;
                    GAME_CALL(st, FN_FIGURE_DYING, v);
                }
                i++;
                p += 4;
                if (i >= ferry_passengers(st, CUR(st))) {
                    break;
                }
            }
        }
        id = CUR(st);
    }
    c = FIG(id);
    action = U8(st, c + FIG_ACTION);
    if (action == 4) {
        int16_t t = S16(st, c + FIG_WAIT_TICKS);
        if (t < 0x21) {
            uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
            c = FIG(CUR(st));
            int32_t k = S16(st, c + FIG_WAIT_TICKS) / 3;
            int32_t e = S32(st, G_SPRITE_DIRECTION) / 8;
            S32(st, c + FIG_U04) = add32(e, add32(image(st, r ? G_IMAGE_GROUP_706 : G_IMAGE_GROUP_178), k));
        } else {
            int32_t k = t - 0x21;
            if (k >= 0x17) {
                k = 0x16;
            }
            S32(st, c + FIG_U04) = add32(k, image(st, G_IMAGE_GROUP_588));
        }
    } else if (action <= 8 || action > 0x0A) {
        uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
        int32_t e = S32(st, G_SPRITE_DIRECTION);
        S32(st, FIG(CUR(st)) + FIG_U04) = add32(image(st, r ? G_IMAGE_GROUP_707 : G_IMAGE_GROUP_179), e);
    } else {
        uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
        c = FIG(CUR(st));
        int32_t k = U8(st, c + FIG_ANIM_FRAME) / 3;
        S32(st, c + FIG_U04) = add32(add32(image(st, r ? G_IMAGE_GROUP_705 : G_IMAGE_GROUP_177), 8 * k), S32(st, G_SPRITE_DIRECTION));
    }
}

/* ---- type 0x4D ------------------------------------------------------------------------------ */

void figure_action_4d(game_state *st)
{
    /* @bin 0x004A4FD0 a transport ship of a building (+0x5A); +0x128 its hit points (of +0x12C) */
    boat_unstick(st, G_UD05720, CUR(st));
    int32_t moving = 1; /* [esp+0x10]: the sprite moves (the states below clear it) */
    uint32_t c = FIG(CUR(st));
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    int32_t building = S16(st, c + FIG_HOME);
    if (building != 0) {
        /* @bin 0x004A5022 the building's +0xB4: this ship when it is at the building's tile */
        uint32_t b = BUILDING_ADDR(building);
        GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
        int32_t id = CUR(st);
        c = FIG(id);
        if (S32(st, G_ROAD_ACCESS_X) == S16(st, c + FIG_X) && S32(st, G_ROAD_ACCESS_Y) == S16(st, c + FIG_Y)) {
            U16(st, b + BLD_SECOND_WALKER) = (uint16_t) id;
        } else {
            U16(st, b + BLD_SECOND_WALKER) = 0;
        }
    }
    U8(st, c + FIG_INVISIBLE) = 0;
    U8(st, c + FIG_BOAT_MODE) = 1;
    S32(st, c + FIG_U180) = 0;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    if (U8(st, BUILDING_ADDR(building) + BLD_STATE) != 1) {
        U8(st, c + FIG_ACTION) = 4;
    }
    if (S32(st, c + FIG_HIT_POINTS) <= 0) {
        U8(st, c + FIG_ACTION) = 4;
    }
    int32_t health = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + FIG_HIT_POINTS), S32(st, c + FIG_HIT_POINTS_MAX));
    int32_t id = CUR(st);
    uint8_t a = U8(st, FIG(id) + FIG_ACTION);
    if (a != 0x0B && a != 0x0F && a != 4 && health <= 0x0F && health > 0) {
        /* @bin 0x004A5123 badly damaged: back to a dock */
        boat_seek_dock(st, id);
        id = CUR(st);
    }
    c = FIG(id);
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
        boat_sink(st, id);
        break;
    case 8:
        boat_moored(st, id, building, &moving);
        break;
    case 9:
        boat_go(st, id, &moving);
        break;
    case 0x0A:
        boat_flee(st, id, &moving);
        break;
    case 0x0B:
        boat_to_battle(st, id, &moving);
        break;
    case 0x0C:
        boat_to_formation(st, id, &moving);
        break;
    case 0x0D:
        boat_to_landing(st, id, &moving);
        break;
    case 0x0E:
    case 0x13:
        U8(st, c + FIG_INVISIBLE) = 0;
        U16(st, c + FIG_WAIT_TICKS) = 0;
        boat_to_landing(st, id, &moving);
        break;
    case 0x0F:
        moving = 0;
        break;
    case 0x11:
        /* @bin 0x004A5216 to the map's exit tile (0x00784C8C, 0x00784C8E) */
        U16(st, c + FIG_DEST_X) = U8(st, G_ENTRY_X);
        U16(st, c + FIG_DEST_Y) = U8(st, G_ENTRY_Y);
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        if (U8(st, c + FIG_DIRECTION) == 8) {
            U8(st, c + FIG_ACTION) = 0x12;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        } else if (U8(st, c + FIG_DIRECTION) == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        } else if (U8(st, c + FIG_DIRECTION) == 0x0A) {
            U8(st, c + FIG_ACTION) = 4;
        }
        break;
    case 0x12:
        U8(st, c + FIG_INVISIBLE) = 1;
        break;
    case 0x14:
        boat_go(st, id, &moving);
        boat_face_enemy(st, CUR(st));
        break;
    default:
        U8(st, c + FIG_ACTION) = 4;
        break;
    }
    id = CUR(st);
    c = FIG(id);
    int32_t d = S8(st, c + FIG_DIRECTION) >= 8 ? S8(st, c + FIG_PREV_DIRECTION) : S8(st, c + FIG_DIRECTION);
    d = (int32_t) ((uint32_t) d - (uint32_t) S32(st, G_VIEW_ORIENTATION));
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (U8(st, c + FIG_ACTION) == 4) {
        int16_t t = S16(st, c + FIG_WAIT_TICKS);
        if (t < 0x21) {
            uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
            id = CUR(st);
            c = FIG(id);
            int32_t k = S16(st, c + FIG_WAIT_TICKS) / 3;
            int32_t e = S32(st, G_SPRITE_DIRECTION) / 8;
            S32(st, c + FIG_U04) = add32(e, add32(image(st, r ? G_IMAGE_GROUP_703 : G_IMAGE_GROUP_74), k));
        } else {
            int32_t k = t - 0x21;
            if (k >= 0x17) {
                k = 0x16;
            }
            S32(st, c + FIG_U04) = add32(k, image(st, G_IMAGE_GROUP_588));
        }
    } else if (moving != 0) {
        uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
        id = CUR(st);
        c = FIG(id);
        S32(st, c + FIG_U04) = add32(add32(image(st, r ? G_IMAGE_GROUP_702 : G_IMAGE_GROUP_73), 8 * (U8(st, c + FIG_ANIM_FRAME) / 3)),
                                  S32(st, G_SPRITE_DIRECTION));
    } else {
        uint8_t r = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, c + FIG_X), S16(st, c + FIG_Y));
        id = CUR(st);
        c = FIG(id);
        S32(st, c + FIG_U04) = add32(image(st, r ? G_IMAGE_GROUP_704 : G_IMAGE_GROUP_75), S32(st, G_SPRITE_DIRECTION));
    }
    /* @bin 0x004A54CE the selected figure (0x00D05700, while 0x00D05708) gets an overlay */
    if (S32(st, G_SEL_TRANSPORT) == id && S32(st, G_TRANSPORT_MODE) != 0) {
        S32(st, c + FIG_U180) = add32(image(st, G_IMAGE_GROUP_495), (int32_t) (8u * U8(st, c + FIG_ANIM_FRAME)) / 12);
    }
}

/* ---- type 0x4E ------------------------------------------------------------------------------ */

/* @bin 0x004A747B dying: +0x36 restarts at -1 unless already dying */
static void start_dying(game_state *st, uint32_t c)
{
    if (U8(st, c + FIG_ACTION) != 4) {
        U16(st, c + FIG_WAIT_TICKS) = 0xFFFF;
    }
    U8(st, c + FIG_ACTION) = 4;
}

void figure_action_4e(game_state *st)
{
    /* @bin 0x004A7030 a warship of a building (+0x5A); +0x128 its hit points (of +0x12C);
     * +0x132 >= 0: leaving the map (action 0x11, then 0x12) */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    if (S16(st, c + FIG_U132) >= 0) {
        uint8_t a = U8(st, c + FIG_ACTION);
        if (a == 0x12) {
            U8(st, c + FIG_INVISIBLE) = 1;
            return;
        }
        if (a != 0x11) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            c = FIG(CUR(st));
            U16(st, c + FIG_ROAM_LENGTH) = 0;
            U8(st, c + FIG_PROGRESS_ON_TILE) = 0;
            U8(st, c + FIG_ACTION) = 0x11;
        }
    }
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    S32(st, CURRENT_CITY(st) + CITY_U4678) += 1; /* @bin 0x004A70C3 warships counted (0x00EA5EE8) */
    int32_t building = S16(st, c + FIG_HOME);
    int32_t moving = 1; /* [esp+0x10] */
    if (building != 0) {
        uint32_t b = BUILDING_ADDR(building);
        GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
        id = CUR(st);
        c = FIG(id);
        if (S32(st, G_ROAD_ACCESS_X) == S16(st, c + FIG_X) && S32(st, G_ROAD_ACCESS_Y) == S16(st, c + FIG_Y)) {
            U16(st, b + BLD_SECOND_WALKER) = (uint16_t) id;
        } else {
            U16(st, b + BLD_SECOND_WALKER) = 0;
        }
    }
    U8(st, c + FIG_INVISIBLE) = 0;
    U8(st, c + FIG_BOAT_MODE) = 1;
    S32(st, c + FIG_U180) = 0;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    if (U8(st, BUILDING_ADDR(building) + BLD_STATE) != 1) {
        start_dying(st, c);
    }
    if (S32(st, c + FIG_HIT_POINTS) <= 0) {
        start_dying(st, c);
    }
    int32_t health = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + FIG_HIT_POINTS), S32(st, c + FIG_HIT_POINTS_MAX));
    id = CUR(st);
    uint8_t a = U8(st, FIG(id) + FIG_ACTION);
    if (a != 0x0D && a != 0x0F && a != 4 && health <= 0x0F && health > 0) {
        boat_seek_dock(st, id);
        id = CUR(st);
    }
    c = FIG(id);
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_BOAT_DYING, id);
        break;
    case 8:
        warship_moored(st, id, &moving);
        warship_fire(st, CUR(st));
        break;
    case 9:
        warship_ram(st, id, &moving);
        break;
    case 0x0A:
    case 0x17:
        warship_attack(st, id, &moving);
        break;
    case 0x0B:
        warship_go(st, id, &moving);
        warship_fire(st, CUR(st));
        break;
    case 0x0C:
        warship_flee(st, id, &moving);
        warship_fire(st, CUR(st));
        break;
    case 0x0D:
        warship_to_battle(st, id, &moving);
        break;
    case 0x0F:
        moving = 0;
        break;
    case 0x11:
        U16(st, c + FIG_DEST_X) = U8(st, G_ENTRY_X);
        U16(st, c + FIG_DEST_Y) = U8(st, G_ENTRY_Y);
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        if (U8(st, c + FIG_DIRECTION) == 8) {
            U8(st, c + FIG_ACTION) = 0x12;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (U8(st, c + FIG_DIRECTION) == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (U8(st, c + FIG_DIRECTION) == 0x0A) {
            start_dying(st, c);
        }
        break;
    case 0x12:
        U8(st, c + FIG_INVISIBLE) = 1;
        break;
    case 0x13:
        U8(st, c + FIG_INVISIBLE) = 0;
        U16(st, c + FIG_WAIT_TICKS) = 0;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        if (U8(st, c + FIG_DIRECTION) == 8) {
            GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
            id = CUR(st);
            c = FIG(id);
            uint8_t dir = U8(st, c + FIG_DIRECTION);
            if (dir == 8) {
                U8(st, c + FIG_ACTION) = 8;
            } else if (dir == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                id = CUR(st);
            } else if (dir == 0x0A) {
                start_dying(st, c);
            }
        }
        break;
    case 0x14:
        warship_go(st, id, &moving);
        warship_fire(st, CUR(st));
        warship_face_enemy(st, CUR(st));
        break;
    case 0x18:
        warship_hunt(st, id, &moving);
        break;
    default:
        start_dying(st, c);
        break;
    }
    id = CUR(st);
    figure_next_direction(st, id); /* result unused */
    id = CUR(st);
    c = FIG(id);
    /* @bin 0x004A74A4 (0x00E94E00 is not written here) */
    int32_t d = S8(st, c + FIG_DIRECTION) >= 8 ? S8(st, c + FIG_PREV_DIRECTION) : S8(st, c + FIG_DIRECTION);
    d = (int32_t) ((uint32_t) d - (uint32_t) S32(st, G_VIEW_ORIENTATION));
    if (d < 0) {
        d += 8;
    }
    if (U8(st, c + FIG_ACTION) == 4) {
        int16_t t = S16(st, c + FIG_WAIT_TICKS);
        if (t < 0x21) {
            S32(st, c + FIG_U04) = add32(d / 8, add32(image(st, G_IMAGE_GROUP_181), t / 3));
        } else {
            int32_t k = t - 0x21;
            if (k >= 0x17) {
                k = 0x16;
            }
            S32(st, c + FIG_U04) = add32(k, image(st, G_IMAGE_GROUP_588));
        }
    } else if (moving != 0 && U8(st, c + FIG_U154) != 2) {
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_180), 8 * (U8(st, c + FIG_ANIM_FRAME) / 3)), d);
    } else {
        S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_182), d);
    }
    if (S32(st, G_SEL_WARSHIP) == id && S32(st, G_WARSHIP_MODE) != 0) {
        S32(st, c + FIG_U180) = add32(image(st, G_IMAGE_GROUP_495), (int32_t) (8u * U8(st, c + FIG_ANIM_FRAME)) / 12);
    }
}

/* ---- type 0x4F ------------------------------------------------------------------------------ */


/* @bin 0x004A7D75 done: the figure goes, its building (+0x5A) counts one worker out (+0xA6) */
static void worker_done(game_state *st, int32_t id)
{
    uint32_t c = FIG(id);
    U8(st, c + FIG_STATE) = 2;
    U8(st, BUILDING_ADDR(S16(st, c + FIG_HOME)) + BLD_SHOWS) -= 1;
}

void figure_action_4f(game_state *st)
{
    /* @bin 0x004A7600 a worker of a building of type 0xB1: waits (action 8) and takes jobs
     * (0x004BC140), walks to the building at its destination tile and works there (by its type) */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    U8(st, c + FIG_TERRAIN_USAGE) = 1;
    U8(st, c + FIG_U0C) = 0;
    U16(st, c + FIG_MAX_ROAM_LENGTH) = 0x200;
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    uint32_t b = BUILDING_ADDR(S16(st, c + FIG_HOME));
    if (U8(st, b + BLD_STATE) != 1 || U16(st, b + BLD_TYPE) != 0xB1) {
        U8(st, c + FIG_STATE) = 2;
        return;
    }
    int32_t t;
    uint8_t dir;
    uint16_t bt;
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        c = FIG(id);
        if (U8(st, c + FIG_STATE) == 2) {
            U8(st, BUILDING_ADDR(S16(st, c + FIG_HOME)) + BLD_SHOWS) -= 1;
        }
        break;
    case 5:
        U8(st, c + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        id = CUR(st);
        break;
    case 6:
        S32(st, c + FIG_U04) = 0;
        break;
    case 7:
        U8(st, c + FIG_ANIM_FRAME) = 0;
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x64) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
            uint8_t r = (uint8_t) figure_may_leave(st, id);
            id = CUR(st);
            if (r == 0) {
                U8(st, FIG(id) + FIG_ACTION) = 8;
            }
        }
        break;
    case 8:
        if (U8(st, c + FIG_INVISIBLE) != 0) {
            U16(st, c + FIG_WAIT_TICKS) += 1;
            if (S16(st, c + FIG_WAIT_TICKS) > 0x0A) {
                U8(st, c + FIG_INVISIBLE) = 0;
            }
            break;
        }
        /* @bin 0x004A7716 the random generator, inlined; a job after 30 + rand % 30 ticks */
        U16(st, c + FIG_WAIT_TICKS) += 1;
        random_next_state(st);
        if (S16(st, c + FIG_WAIT_TICKS) > (int32_t) (U32(st, G_R_1_15) % 30) + 30) {
            int32_t job = GAME_CALL(st, FN_MONUMENT_TAKE_WORKER, S16(st, c + FIG_X), S16(st, c + FIG_Y), id);
            if (job != 0) {
                U8(st, FIG(CUR(st)) + FIG_ACTION) = 9;
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
                U16(st, FIG(CUR(st)) + FIG_ROAM_LENGTH) = 0;
            }
            id = CUR(st);
            c = FIG(id);
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        if (U8(st, c + FIG_ACTION) != 8) {
            U8(st, c + FIG_ANIM_FRAME) = 0;
            break;
        }
        /* @bin 0x004A7872 after +0x42 ticks idle: back to the building's tile (0x0047FE60), or
         * gone when there is none */
        U16(st, c + FIG_ROAM_LENGTH) += 1;
        if (S16(st, c + FIG_ROAM_LENGTH) >= S16(st, c + FIG_MAX_ROAM_LENGTH)) {
            int32_t found = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
            if (found == 0) {
                id = CUR(st);
                U8(st, FIG(id) + FIG_ACTION) = 4;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                id = CUR(st);
                break;
            }
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 0x0D;
            U16(st, c + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
            U16(st, c + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
            id = CUR(st);
            U16(st, FIG(id) + FIG_ROAM_LENGTH) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        id = CUR(st);
        break;
    case 9: {
        U8(st, c + FIG_INVISIBLE) = 0;
        U8(st, c + FIG_TERRAIN_USAGE) = 3;
        t = S16(st, G_BUILDING_GRID + 2u * U32(st, c + FIG_U28));
        if (t != 0) {
            bt = U16(st, BUILDING_ADDR(t) + BLD_TYPE);
            if (bt == 0xD5 || bt == 0xE3 || bt == 0xE1 || bt == 0xE4 || bt == 0xDE) {
                U8(st, c + FIG_TERRAIN_USAGE) = 7;
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            if (t == 0) {
                U8(st, c + FIG_ACTION) = 0x0B;
                break;
            }
            bt = U16(st, BUILDING_ADDR(t) + BLD_TYPE);
            if (bt == 0xB7) {
                U8(st, c + FIG_ACTION) = 0x0C;
                worker_start_job(st, id);
                id = CUR(st);
            } else if (bt == 0xD2 || bt == 0xCF || bt == 0xD0 || bt == 0xD5 || bt == 0xE3 || bt == 0xE1 ||
                       bt == 0xE6 || bt == 0xE4 || bt == 0xDE) {
                U8(st, c + FIG_ACTION) = 0x0A;
                U16(st, c + FIG_WAIT_TICKS) = 0;
            } else {
                U8(st, c + FIG_ACTION) = 0x0B;
            }
        } else if (dir == 0x0A) {
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        }
        break;
    }
    case 0x0A: {
        /* @bin 0x004A7A85 working at the building on the destination tile, by its type */
        U8(st, c + FIG_INVISIBLE) = 0;
        t = S16(st, G_BUILDING_GRID + 2u * U32(st, c + FIG_U28));
        if (t == 0) {
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0;
            break;
        }
        uint32_t home = BUILDING_ADDR(S16(st, c + FIG_HOME));
        if (U8(st, home + BLD_STATE) != 1) {
            U8(st, c + FIG_STATE) = 2;
            break;
        }
        bt = U16(st, BUILDING_ADDR(t) + BLD_TYPE);
        switch (bt) {
        case 0xB7:
            worker_at_b7(st, t, id);
            break;
        case 0xD2:
            GAME_CALL(st, FN_CITY_MONUMENT_WORK, t, id, 0x0B);
            break;
        case 0xCF:
        case 0xD0:
            GAME_CALL(st, FN_MONUMENT_LIST_WORK, t, id, 0x0B);
            break;
        case 0xD5:
            GAME_CALL(st, FN_SUN_TEMPLE_WORK, t, id, 0x0B);
            break;
        case 0xE3:
            monument_work_417940(st, G_MONUMENT_E3, t, id, 0x0B);
            break;
        case 0xE4:
            monument_work_4e6e70(st, G_MONUMENT_E4, t, id, 0x0B);
            break;
        case 0xE1:
            GAME_CALLT(st, G_MONUMENT_E1, FN_OBELISK_BUILD, t, id, 0x0B);
            break;
        case 0xE6:
            monument_work_401f50(st, G_MONUMENT_E6, t, id, 0x0B);
            id = CUR(st);
            c = FIG(id);
            if (U8(st, c + FIG_ACTION) == 0x0B) {
                uint8_t n = (uint8_t) (U8(st, c + FIG_U154) + 1);
                U8(st, c + FIG_U154) = n;
                if (n >= 4) {
                    U8(st, c + FIG_U154) = 0;
                } else {
                    GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_TAKE_0XE6_401ED0, S16(st, c + FIG_X), S16(st, c + FIG_Y), id, 1);
                }
            }
            break;
        case 0xDE:
            GAME_CALL(st, FN_MASTABA_WORK, t, id, 8);
            id = CUR(st);
            c = FIG(id);
            if (U8(st, c + FIG_ACTION) == 8) {
                U16(st, c + FIG_WAIT_TICKS) = 0x0A;
            }
            break;
        default:
            U8(st, c + FIG_ACTION) = 0x0B;
            U16(st, c + FIG_DEST_X) = U16(st, home + BLD_ROAD_ACCESS_X);
            U16(st, c + FIG_DEST_Y) = U16(st, home + BLD_ROAD_ACCESS_Y);
            break;
        }
        id = CUR(st);
        break;
    }
    case 0x0B:
        U8(st, c + FIG_INVISIBLE) = 0;
        U8(st, c + FIG_TERRAIN_USAGE) = 7;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        dir = U8(st, FIG(id) + FIG_DIRECTION);
        if (dir == 8 || dir == 0x0A || dir == 9) {
            if (dir == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                id = CUR(st);
            }
            worker_done(st, id);
        }
        break;
    case 0x0C:
        U8(st, c + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_PYRAMID_WORK, id, 1, 0x0B, 0x0A);
        id = CUR(st);
        break;
    case 0x0D:
        U8(st, c + FIG_INVISIBLE) = 0;
        U8(st, c + FIG_TERRAIN_USAGE) = 7;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0;
            U8(st, c + FIG_INVISIBLE) = 1;
        } else if (dir == 0x0A || dir == 9) {
            if (dir == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                id = CUR(st);
            }
            worker_done(st, id);
        }
        break;
    default:
        break;
    }
    c = FIG(id);
    int32_t d = view_direction(st, c, 0, 8);
    uint8_t action = U8(st, c + FIG_ACTION);
    if (action == 4) {
        int32_t k = S16(st, c + FIG_WAIT_TICKS) / 2;
        S32(st, c + FIG_U04) = add32(U8(st, (uint32_t) ((int32_t) G_DIE_FRAMES + k)), image(st, G_IMAGE_GROUP_185));
    } else if (action == 0x0A) {
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_186), 8 * (U8(st, c + FIG_ANIM_FRAME) >> 1)), d);
    } else {
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_184), 8 * U8(st, c + FIG_ANIM_FRAME)), d);
    }
}

void worker_start_job(game_state *st, int32_t id)
{
    /* @bin 0x004F1CB0 a worker at a monument site: +0x120 = 1, its building (+0x4A, +0x4E) the
     * one on the destination tile (grid 0x00B4E6FC), +0x52 = 1, +0x54 = 15 */
    uint32_t f = FIG(id);
    U16(st, f + FIG_FOLLOW) = 1;
    uint16_t b = U16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_U28));
    U16(st, f + FIG_CC_X) = b;
    U16(st, f + FIG_CC_Y) = 0;
    U16(st, f + FIG_WORK_BUILDING) = b;
    U16(st, f + FIG_CC_DEST_Y) = 0;
    U16(st, f + FIG_CC_DELTA_X) = 1;
    U16(st, f + FIG_CC_DELTA_Y) = 0x0F;
}

void worker_join(game_state *st, int32_t id)
{
    /* @bin 0x004F2410 the figure moves to the current figure's destination (+0x24, +0x26, +0x28)
     * (out of and back into the tile lists); +0x50 from its building (+0x4E) bytes +0x56, +0x57 */
    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
    uint32_t c = FIG(CUR(st)), f = FIG(id);
    U16(st, f + FIG_X) = U16(st, c + FIG_DEST_X);
    U16(st, f + FIG_Y) = U16(st, c + FIG_DEST_Y);
    S32(st, f + FIG_GRID_OFFSET) = S32(st, c + FIG_U28);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_WORK_BUILDING));
    U16(st, f + FIG_CC_DEST_Y) = (uint16_t) (U8(st, b + BLD_MON_U56) + 6 * U8(st, b + BLD_U57));
    U16(st, f + FIG_WAIT_TICKS) = 0;
}

void worker_at_b7(game_state *st, int32_t building, int32_t id)
{
    /* @bin 0x004EF3A0 a worker at a building of type 0xB7: done (action 0x0B) when 0x004EF100
     * says so, or when another worker holds the building (+0x64) and 0x004EEA10 agrees; else it
     * works 30 ticks, then 0x004EF200 and the building is free again */
    uint32_t f = FIG(id);
    if (GAME_CALL(st, FN_PYRAMID_PART_READY, building) == 0) {
        U8(st, f + FIG_ACTION) = 0x0B;
        U16(st, f + FIG_FOLLOW) = 0;
        return;
    }
    uint32_t b = BUILDING_ADDR(building);
    if (S16(st, b + BLD_U64) != id && (uint8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, building) != 0) {
        U8(st, f + FIG_ACTION) = 0x0B;
        U16(st, f + FIG_FOLLOW) = 0;
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x1E) {
        return;
    }
    GAME_CALL(st, FN_PYRAMID_PART_ADVANCE, building);
    U8(st, f + FIG_ACTION) = 0x0B;
    U16(st, b + BLD_U64) = 0;
    U16(st, f + FIG_FOLLOW) = 0;
}

/* @bin 0x00401F50, 0x00417940, 0x004E6E70 (thiscall on a monument object) */
int32_t monument_work_401f50(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c)
{
    (void) self;
    return GAME_CALL(st, FN_MONUMENT_E6_WORK, a, b, c);
}

/* @bin 0x00417940 unless the object's slot of the building's subtype (+0x12: this + 0x58 + 52 *
 * subtype) is set, 0x00420E20 (the result is then 13 * subtype, as the original leaves it) */
static int32_t monument_work_slot(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c, uint32_t fn)
{
    int32_t t = S16(st, BUILDING_ADDR(a) + BLD_PART);
    int32_t k = (int32_t) (13u * (uint32_t) t);
    if (S32(st, (uint32_t) ((int32_t) self + 0x58 + 4 * k)) != 0) {
        return k;
    }
    return GAME_CALL(st, fn, a, b, c);
}

int32_t monument_work_417940(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c)
{
    return monument_work_slot(st, self, a, b, c, FN_MONUMENT_E3_WORK);
}

int32_t monument_work_4e6e70(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c)
{
    /* @bin 0x004E6E70 as 0x00417940 with 0x00421120 */
    return monument_work_slot(st, self, a, b, c, FN_MONUMENT_E4_WORK);
}

/* ---- type 0x50 ------------------------------------------------------------------------------ */

void figure_action_50(game_state *st)
{
    /* @bin 0x004A7EA0 a worker of a building of type 0xB2: waits (action 8) and takes jobs
     * (0x004BC480), walks to the building at its destination tile and works there (by its type) */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int at_work = 0; /* [esp+0x13] */
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    if (S16(st, c + FIG_WAIT_TICKS) < 0) {
        U16(st, c + FIG_WAIT_TICKS) = 0;
    }
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    U8(st, c + FIG_TERRAIN_USAGE) = 1;
    U8(st, c + FIG_U0C) = 0;
    U16(st, c + FIG_MAX_ROAM_LENGTH) = 0x200;
    int32_t home_id = S16(st, c + FIG_HOME);
    uint32_t b = BUILDING_ADDR(home_id);
    if (U8(st, b + BLD_STATE) != 1 || U16(st, b + BLD_TYPE) != 0xB2) {
        U8(st, c + FIG_STATE) = 2;
        return;
    }
    int32_t t;
    uint8_t dir;
    uint16_t bt;
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        c = FIG(id);
        if (U8(st, c + FIG_STATE) == 2) {
            U8(st, BUILDING_ADDR(S16(st, c + FIG_HOME)) + BLD_SHOWS) -= 1;
        }
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        id = CUR(st);
        break;
    case 6:
        S32(st, c + FIG_U04) = 0;
        break;
    case 7:
        U8(st, c + FIG_ANIM_FRAME) = 0;
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x64) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
            uint8_t r = (uint8_t) figure_may_leave(st, id);
            id = CUR(st);
            if (r == 0) {
                U8(st, FIG(id) + FIG_ACTION) = 8;
            }
        }
        break;
    case 8:
        /* @bin 0x004A7FA2 the random generator, inlined; a job after 30 + rand % 30 ticks */
        U16(st, c + FIG_WAIT_TICKS) += 1;
        random_next_state(st);
        if (S16(st, c + FIG_WAIT_TICKS) > (int32_t) (U32(st, G_R_1_15) % 30) + 30) {
            int32_t job = GAME_CALL(st, FN_PYRAMID_ASSIGN_4EFE30, S16(st, c + FIG_X), S16(st, c + FIG_Y), id, 0); /* @bin 0x004BC480 -> 0x004F01C0 -> */
            if (job != 0) {
                U8(st, FIG(CUR(st)) + FIG_ACTION) = 0x0A;
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
                U16(st, FIG(CUR(st)) + FIG_ROAM_LENGTH) = 0;
            }
            id = CUR(st);
            c = FIG(id);
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        if (U8(st, c + FIG_ACTION) != 8) {
            U8(st, c + FIG_ANIM_FRAME) = 0;
            break;
        }
        U16(st, c + FIG_ROAM_LENGTH) += 1;
        if (S16(st, c + FIG_ROAM_LENGTH) >= S16(st, c + FIG_MAX_ROAM_LENGTH)) {
            int32_t found = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
            if (found == 0) {
                id = CUR(st);
                U8(st, FIG(id) + FIG_ACTION) = 4;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                id = CUR(st);
                break;
            }
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 9;
            U16(st, c + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
            U16(st, c + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
            id = CUR(st);
            U16(st, FIG(id) + FIG_ROAM_LENGTH) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        id = CUR(st);
        break;
    case 9:
        U8(st, c + FIG_TERRAIN_USAGE) = 3;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8 || dir == 0x0A) {
            worker_done(st, id);
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        }
        break;
    case 0x0A:
        U8(st, c + FIG_TERRAIN_USAGE) = 3;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 8) {
            t = S16(st, G_BUILDING_GRID + 2u * U32(st, c + FIG_U28));
            if (t != 0) {
                bt = U16(st, BUILDING_ADDR(t) + BLD_TYPE);
                if (bt == 0xB7) {
                    U8(st, c + FIG_ACTION) = 0x0D;
                    worker_start_job(st, id);
                    id = CUR(st);
                } else if (bt == 0xD2) {
                    U8(st, c + FIG_ACTION) = 0x0C;
                    U16(st, c + FIG_WAIT_TICKS) = 0;
                } else {
                    U8(st, c + FIG_ACTION) = 9;
                }
            } else if (U16(st, BUILDING_ADDR(S16(st, c + FIG_DEST_BUILDING)) + BLD_TYPE) == 0xB7 &&
                       (U32(st, G_GRID_TERRAIN + 4u * U32(st, c + FIG_U28)) & 0x10000000u)) {
                /* @bin 0x004A82DB no building on the tile but the target (+0x5E) is of type 0xB7
                 * and the tile has terrain bit 0x10000000 */
                U8(st, c + FIG_ACTION) = 0x0C;
                U16(st, c + FIG_WAIT_TICKS) = 0;
            } else {
                U8(st, c + FIG_ACTION) = 9;
            }
        } else if (dir == 0x0A || dir == 9) {
            if (dir == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                id = CUR(st);
                c = FIG(id);
            }
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        break;
    case 0x0B:
        if (U16(st, c + FIG_FOLLOW) == 0) {
            U8(st, c + FIG_ACTION) = 9;
            break;
        }
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) < 0) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
        }
        if (id != S16(st, BUILDING_ADDR(S16(st, c + FIG_WORK_BUILDING)) + BLD_U64)) {
            U8(st, c + FIG_ACTION) = 9;
            U16(st, c + FIG_FOLLOW) = 0;
        }
        break;
    case 0x0C:
        t = S16(st, G_BUILDING_GRID + 2u * U32(st, c + FIG_U28));
        if (t == 0) {
            int32_t g = S16(st, c + FIG_DEST_BUILDING);
            if (U16(st, BUILDING_ADDR(g) + BLD_TYPE) == 0xB7 && (U32(st, G_GRID_TERRAIN + 4u * U32(st, c + FIG_U28)) & 0x10000000u)) {
                GAME_CALL(st, FN_PYRAMID_RAMP_BUILD, g, id, 9);
                id = CUR(st);
            } else {
                U8(st, c + FIG_ACTION) = 8;
                U16(st, c + FIG_WAIT_TICKS) = 0;
            }
            break;
        }
        if (U8(st, b + BLD_STATE) != 1) {
            U8(st, c + FIG_STATE) = 2;
            break;
        }
        bt = U16(st, BUILDING_ADDR(t) + BLD_TYPE);
        if (bt == 0xB7) {
            if (U8(st, BUILDING_ADDR(t) + BLD_U55) == 0) {
                at_work = 1;
            }
            GAME_CALL(st, FN_FIGURE_PYRAMID_PART_WORK, t, id);
            id = CUR(st);
        } else if (bt == 0xD2) {
            GAME_CALL(st, FN_CITY_MONUMENT_WORK, t, id, 9);
            id = CUR(st);
        } else {
            U16(st, BUILDING_ADDR(t) + BLD_UC8) -= 1;
            U8(st, c + FIG_ACTION) = 9;
            U16(st, c + FIG_DEST_X) = U16(st, b + BLD_ROAD_ACCESS_X);
            U16(st, c + FIG_DEST_Y) = U16(st, b + BLD_ROAD_ACCESS_Y);
        }
        break;
    case 0x0D:
        GAME_CALL(st, FN_FIGURE_PYRAMID_WORK, id, 0, 9, 0x0E);
        id = CUR(st);
        break;
    case 0x0E:
        GAME_CALL(st, FN_FIGURE_PYRAMID_CHOOSE, id, 1, 9, 9, 0x0F);
        id = CUR(st);
        break;
    case 0x0F:
        if ((uint8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, 1) == 0) {
            id = CUR(st);
            break;
        }
        worker_join(st, CUR(st));
        id = CUR(st);
        c = FIG(id);
        if (U8(st, BUILDING_ADDR(S16(st, c + FIG_WORK_BUILDING)) + BLD_U55) == 0) {
            U8(st, c + FIG_ACTION) = 0x0B;
        } else {
            U8(st, c + FIG_ACTION) = 0x0C;
        }
        U16(st, c + FIG_WAIT_TICKS) = 0;
        break;
    default:
        break;
    }
    c = FIG(id);
    int32_t d = view_direction(st, c, 0, 8);
    uint8_t action = U8(st, c + FIG_ACTION);
    if (action == 4) {
        int32_t k = S16(st, c + FIG_WAIT_TICKS) / 2;
        S32(st, c + FIG_U04) = add32(U8(st, (uint32_t) ((int32_t) G_DIE_FRAMES + k)), image(st, G_IMAGE_GROUP_149));
    } else if (action == 0x0B) {
        /* @bin 0x004A868C waiting: the frame by (+0x36 / 2 + 13 * id) / 9 & 3 */
        int32_t w = (int32_t) ((uint32_t) (S16(st, c + FIG_WAIT_TICKS) >> 1) + 13u * (uint32_t) id);
        switch ((w / 9) & 3) {
        case 0:
            S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_151), 8 * ((w >> 2) & 1)), d);
            break;
        case 1:
            S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_151), 8 * (w % 9)), d);
            break;
        case 2:
            if ((w & 7) == 4) {
                /* @bin 0x004A86FB a random turn (CRT rand) */
                int32_t r = crt_rand(st) & 3;
                uint32_t f = FIG(CUR(st));
                if (r == 0) {
                    U8(st, f + FIG_DIRECTION) = (uint8_t) ((U8(st, f + FIG_DIRECTION) + 1) & 7);
                } else if (r == 1) {
                    U8(st, f + FIG_DIRECTION) = (uint8_t) ((U8(st, f + FIG_DIRECTION) - 1) & 7);
                }
                S32(st, f + FIG_U04) = add32(image(st, G_IMAGE_GROUP_151), S32(st, G_SPRITE_DIRECTION));
            }
            break;
        default:
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_151), d);
            break;
        }
    } else if (action == 0x0C) {
        if (at_work && S16(st, c + FIG_WAIT_TICKS) < S32(st, G_U5E4EE8)) {
            if (U8(st, c + FIG_ANIM_FRAME) >= 9) {
                U8(st, c + FIG_ANIM_FRAME) = 0;
            }
            S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_151), 8 * U8(st, c + FIG_ANIM_FRAME)), d);
        } else {
            S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_150), 8 * (S16(st, c + FIG_WAIT_TICKS) % 0x17)), d);
        }
    } else {
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_148), 8 * U8(st, c + FIG_ANIM_FRAME)), d);
    }
}

/* ---- type 0x66 ------------------------------------------------------------------------------ */

/* @bin 0x004A13C5 the random generator's 15-bit value (0x00D3B370) after one step */
static int32_t rand15(game_state *st)
{
    random_next_state(st);
    return (int32_t) U32(st, G_R_1_15);
}

int32_t ground_rule(game_state *st, int32_t flag)
{
    /* @bin 0x004EAAC0 a byte of the difficulty records 0x005E4C3C (by 0x00784CB0, with the flag)
     * or 0x005E4CCC (by 0x00784B94 and 0x00784CB0) */
    uint32_t mode = U32(st, G_U784CB0) & 0xFF;
    if ((uint8_t) flag != 0) {
        return U8(st, G_U5E4C3C + 48u * mode);
    }
    int32_t idx = (int32_t) ((uint32_t) (int32_t) S16(st, G_INVASION_SUB) + 2u * mode);
    return U8(st, (uint32_t) ((int32_t) G_U5E4CCC + 48 * idx));
}

int32_t type_is_boat(game_state *st, int32_t type)
{
    /* @bin 0x004EADA0 types 0x19, 0x4D, 0x4E, 0x5C, 0x5D, 0x64, 0x65 */
    (void) st;
    return type == 0x19 || type == 0x4D || type == 0x4E || type == 0x5C || type == 0x5D || type == 0x64 ||
           type == 0x65;
}

int32_t find_tree(game_state *st, int32_t a, int32_t b, int32_t c, int32_t d)
{
    /* @bin 0x004867E0 0x00486810(a, b, c, d, 0, 0, map width, map height) */
    return GAME_CALL(st, FN_FIND_GATHERABLE, a, b, c, d, 0, 0, S32(st, G_MAP_WIDTH), S32(st, G_MAP_HEIGHT));
}

int32_t herd_spot(game_state *st, int32_t cx, int32_t cy, int32_t k, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x004EAB10 from offset k on (table 0x005CDB78, 16 offsets around (cx, cy)), the first
     * free tile (no figure, terrain accepted by 0x004EC410) reachable from (x, y) (0x00519BE0 or
     * 0x00517F20 when 0x004EAAC0 accepts the flag, else 0x005181C0); its index, -1 when none */
    for (int i = 0; i < 16; i++) {
        int32_t ty = S32(st, G_RING16_DY + 8u * (uint32_t) k) + cy;
        int32_t tx = S32(st, G_RING16_DX + 8u * (uint32_t) k) + cx;
        uint32_t tile = U32(st, G_MAP_START) + 228u * (uint32_t) ty + (uint32_t) tx;
        int ok;
        if ((uint8_t) ground_rule(st, flag) != 0) {
            ok = GAME_CALL(st, FN_ROUTE_WATER_TO, x, y, tx, ty) != 0 || GAME_CALL(st, FN_ROUTE_ANY_FREE_TO, x, y, tx, ty) != 0;
        } else {
            ok = GAME_CALL(st, FN_ROUTE_WALK_DISTANCES, x, y, tx, ty) != 0;
        }
        if (ok && U16(st, G_FIGURE_GRID + 2u * tile) == 0 &&
            (uint8_t) GAME_CALL(st, FN_INVADER_TILE_OK, S32(st, G_GRID_TERRAIN + 4u * tile), flag) != 0) {
            return k;
        }
        k++;
        if (k >= 0x10) {
            k = 0;
        }
    }
    return -1;
}


/* @bin 0x004EAE20 (not in the Ghidra export; the ring search's callback for animals) a figure
 * on the tile, other than `id`, alive, not dying, of a type with a +0x00 record word (0x007090CC)
 * other than id's type, that 0x004EADA0 does not exclude */
static int32_t animal_prey_on_tile(game_state *st, int32_t tile, int32_t id)
{
    int32_t k = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile);
    while (k != 0) {
        uint32_t g = FIG(k);
        if (k != id && U8(st, g + FIG_STATE) != 0 && U8(st, g + FIG_ACTION) != 4) {
            uint8_t t = U8(st, g + FIG_TYPE);
            if (S32(st, G_PROPS + FIGTYPE_U00 + 56u * t) != 0 && U8(st, FIG(id) + FIG_TYPE) != t &&
                (uint8_t) type_is_boat(st, t) == 0) {
                return k;
            }
        }
        k = S16(st, g + FIG_NEXT_ON_TILE);
    }
    return 0;
}

int32_t ring_search(game_state *st, int32_t x, int32_t y, int32_t *px, int32_t *py, int32_t rings, int32_t id)
{
    /* @bin 0x004EC0F0 square rings of growing size around (x, y), each side walked in turn (right,
     * down, left, up); every tile inside the map (0x00BCA2D0, 0x00BCA2D4) without terrain bit
     * 0x80000 is offered to the callback (here 0x004EAE20, the only one passed); its first
     * non-zero result. *px, *py and SEARCH_TILE follow the walk. The callback argument of the
     * original is not kept: both callers pass 0x004EAE20. */
    uint32_t tile = U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x;
    int32_t side = 1;
    *px = x;
    *py = y;
    static const struct {
        int32_t dt, dx, dy;
    } dirs[4] = {{1, 1, 0}, {228, 0, 1}, {-1, -1, 0}, {-228, 0, -1}};
    for (int32_t r = 0;;) {
        S32(st, G_TILE_CURSOR) = (int32_t) tile;
        for (int d = 0; d < 4; d++) {
            for (int32_t i = 0; i < side; i++) {
                if (*px >= 0 && *py >= 0 && *px < S32(st, G_MAP_WIDTH) && *py < S32(st, G_MAP_HEIGHT) &&
                    (U32(st, G_GRID_TERRAIN + 4u * U32(st, G_TILE_CURSOR)) & 0x80000u) == 0) {
                    int32_t k = animal_prey_on_tile(st, S32(st, G_TILE_CURSOR), id);
                    if (k != 0) {
                        return k;
                    }
                }
                S32(st, G_TILE_CURSOR) += dirs[d].dt;
                *px += dirs[d].dx;
                *py += dirs[d].dy;
            }
            S32(st, G_TILE_CURSOR) -= dirs[d].dt;
            *px -= dirs[d].dx;
            *py -= dirs[d].dy;
        }
        if (rings == 0) {
            return 0;
        }
        r++;
        side += 2;
        tile -= 229u;
        *px -= 1;
        *py -= 1;
        rings--;
        if (r >= 0xE4) {
            return 0;
        }
    }
}

/* @bin 0x004EAF26 the attack settings by difficulty (0x00784CB0) and 0x00784B94: 48-byte records
 * at 0x005E4CD0 (+0 ticks between searches, +4 search radius) */
static uint32_t attack_settings(game_state *st)
{
    int32_t idx = (int32_t) ((uint32_t) (int32_t) S16(st, G_INVASION_SUB) + 2u * (U32(st, G_U784CB0) & 0xFF));
    return (uint32_t) ((int32_t) G_U5E4CD0 + 48 * idx);
}

int32_t animal_seek_prey(game_state *st, int32_t id)
{
    /* @bin 0x004EAEC0 a predator (0x004EA9C0) not fighting looks for prey every few ticks
     * (+0x6B): attack it (action 0x15, +0x5E, destination its tile) */
    uint32_t f = FIG(id);
    if ((uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, U8(st, f + FIG_TYPE)) == 0 || U8(st, f + FIG_STATE) == 0) {
        return 0;
    }
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 0x0C || a == 4 || a == 5 || a == 0x15) {
        return 0;
    }
    uint8_t n = (uint8_t) (U8(st, f + FIG_U6B) + 1);
    U8(st, f + FIG_U6B) = n;
    uint32_t rec = attack_settings(st);
    if ((int32_t) n < S32(st, rec)) {
        return 0;
    }
    int32_t rings = S32(st, rec + 4);
    U8(st, f + FIG_U6B) = 0;
    /* @bin 0x004EAF6B srand(timeGetTime()): the wall clock (core/sim/platform.h) */
    crt_srand(st, platform_clock_ms(st));
    int32_t px, py;
    int32_t k = ring_search(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), &px, &py, rings, id);
    if (k == 0) {
        return 0;
    }
    uint32_t g = FIG(k);
    U16(st, f + FIG_DEST_X) = U16(st, g + FIG_X);
    U16(st, f + FIG_DEST_Y) = U16(st, g + FIG_Y);
    U8(st, f + FIG_ACTION) = 0x15;
    U16(st, f + FIG_WAIT_TICKS) = (uint16_t) (rings * 18);
    U16(st, f + FIG_DEST_BUILDING) = (uint16_t) k;
    return 1;
}

void animal_pick_action(game_state *st, int32_t id)
{
    /* @bin 0x004EC7C0 a random next action (0x004EC4A0 with entry k of table 0x005E4E70), k drawn
     * with weights by the current action (rand % 100) */
    static const int32_t weights[4][4] = {
        {0x32, 0x32, 0, 0}, {0x4B, 5, 0x14, 0}, {0x5A, 1, 9, 0}, {0x1D, 0x46, 0, 1}};
    uint8_t a = U8(st, FIG(id) + FIG_ACTION);
    int sel = a == 8 || a == 0x19 ? 0 : a == 0x13 ? 1 : a == 0x1A ? 2 : 3;
    int32_t r = rand15(st) % 100;
    int32_t acc = 0, k;
    for (k = 0; k < 4; k++) {
        acc += weights[sel][k];
        if (r < acc) {
            break;
        }
    }
    GAME_CALL(st, FN_ANIMAL_SET_ACTION2, id, S32(st, G_U5E4E70 + 4u * (uint32_t) k));
}

void animal_seek_target(game_state *st, int32_t id)
{
    /* @bin 0x004EC990 a predator not fighting: a target within twice the search radius (its
     * tile as destination, +0x5E), else a random next action (0x004EC7C0) */
    uint32_t f = FIG(id);
    if ((uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, U8(st, f + FIG_TYPE)) == 0 || U8(st, f + FIG_STATE) == 0) {
        return;
    }
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 0x0C || a == 4) {
        return;
    }
    int32_t rings = (int32_t) ((uint32_t) S32(st, attack_settings(st) + 4) << 1);
    int32_t px, py;
    int32_t k = ring_search(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), &px, &py, rings, id);
    if (k != 0) {
        uint32_t g = FIG(k);
        U16(st, f + FIG_DEST_X) = U16(st, g + FIG_X);
        U16(st, f + FIG_DEST_Y) = U16(st, g + FIG_Y);
        U16(st, f + FIG_DEST_BUILDING) = (uint16_t) k;
        return;
    }
    animal_pick_action(st, id);
}

/* @bin 0x004A1549 a spot around the herd's centre (formation +0x36, +0x38; 0x004EAB10 with a
 * random index 0 .. 15): go there (action 9); 1 when one was found */
static int herd_move(game_state *st, uint32_t fm, int32_t id, int reset_frame_counter)
{
    uint32_t c = FIG(id);
    int32_t r = rand15(st) & 0x0F;
    int32_t k = herd_spot(st, S16(st, fm - FM_TYPE + FM_DEST_X), S16(st, fm - FM_TYPE + FM_DEST_Y), r, S16(st, c + FIG_X), S16(st, c + FIG_Y), 0);
    if (k < 0 || k >= 0x10) {
        return 0;
    }
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
    c = FIG(CUR(st));
    /* @bin 0x005CDB78 16 offsets (x at +0, y at +4) */
    U16(st, c + FIG_DEST_X) = (uint16_t) (U16(st, G_RING16_DX + 8u * (uint32_t) k) + U16(st, fm - FM_TYPE + FM_DEST_X));
    U16(st, c + FIG_DEST_Y) = (uint16_t) (U16(st, G_RING16_DY + 8u * (uint32_t) k) + U16(st, fm - FM_TYPE + FM_DEST_Y));
    U8(st, c + FIG_ACTION) = 9;
    if (reset_frame_counter) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
    }
    return 1;
}

/* What differs between the herd animals' actions (0x004A1130 and 0x0049DC50) */
typedef struct {
    int32_t sound;          /* act 5: 0x0053B4A0's second argument */
    int32_t graze_period;   /* act 8: +0x144 */
    int32_t graze_time;     /* act 8: +0x36 = rand % 100 + this */
    int moved_reset;        /* act 0x12, a spot found: +0x148 = 0 */
    int fixed_wait;         /* acts 0x12, 0x17, no spot: +0x36 = 100 (else rand % 100) */
    int32_t arrived_time;   /* acts 9, 0x0A arrived: +0x36 = rand % 100 + this */
    int arrived_reset;      /* acts 9, 0x0A arrived: +0x148 = 0 */
    int32_t idle_period;    /* other actions: +0x144 */
    uint32_t img_die, img_hurt, img_still, img_move, img_graze1, img_graze2, img_idle; /* 0: none */
    int still_frame;        /* act 8's sprite advances with +0x148 */
} herd_kind;

static void herd_animal(game_state *st, const herd_kind *hk)
{
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int frame_tick = 0; /* [esp+0x13] */
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    S32(st, c + FIG_FRAME_TICKS) += 1;
    if (S32(st, c + FIG_FRAME_TICKS) >= S32(st, c + FIG_FRAME_PERIOD)) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
        U8(st, c + FIG_ANIM_FRAME) += 1;
        frame_tick = 1;
    }
    U16(st, CURRENT_CITY(st) + CITY_U447A) += 1; /* @bin 0x004A11AE animals counted (0x00EA5CEA) */
    U8(st, c + FIG_TERRAIN_USAGE) = 5;
    U8(st, c + FIG_U0C) = 0;
    U8(st, c + FIG_INVISIBLE) = 0;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    uint32_t fm = FORMATION(S16(st, c + FIG_FORMATION));
    uint8_t dir;
    switch (U8(st, c + FIG_ACTION)) {
    case 5:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, hk->sound, 2);
        GAME_CALL(st, FN_FIGURE_FIGHT, CUR(st));
        id = CUR(st);
        break;
    case 4:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        break;
    case 8:
        /* @bin 0x004A1257 two random steps: graze (0x12 or 0x13) for 50 + rand % 100 ticks */
        S32(st, c + FIG_FRAME_PERIOD) = hk->graze_period;
        U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
        U8(st, c + FIG_ACTION) = (rand15(st) & 1) ? 0x12 : 0x13;
        U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (rand15(st) % 100 + hk->graze_time);
        break;
    case 0x18:
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            S32(st, c + FIG_FRAME_TICKS) = 0;
            S32(st, c + FIG_FRAME_PERIOD) = 8;
            U8(st, c + FIG_ACTION) = 8;
        }
        S32(st, c + FIG_U04) = 0;
        return;
    case 0x13:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            U8(st, c + FIG_ACTION) = 8;
        }
        break;
    case 0x12:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            if (herd_move(st, fm, id, hk->moved_reset)) {
                animal_seek_prey(st, CUR(st));
                id = CUR(st);
                break;
            }
            int32_t r = hk->fixed_wait ? 0x64 : rand15(st) % 100;
            id = CUR(st);
            U16(st, FIG(id) + FIG_WAIT_TICKS) = (uint16_t) r;
        }
        animal_seek_prey(st, CUR(st));
        id = CUR(st);
        break;
    case 0x17:
        S32(st, c + FIG_FRAME_PERIOD) = 1;
        if (!herd_move(st, fm, id, 0)) {
            id = CUR(st);
            c = FIG(id);
            U8(st, c + FIG_ACTION) = 0x12;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (hk->fixed_wait ? 0x64 : rand15(st) % 100);
        }
        id = CUR(st);
        break;
    case 0x14:
        S32(st, c + FIG_FRAME_PERIOD) = 1;
        animal_seek_prey(st, id);
        id = CUR(st);
        break;
    case 9:
    case 0x0A:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0C;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        } else if (dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = (rand15(st) & 1) ? 0x12 : 0x13;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (rand15(st) % 100 + hk->arrived_time);
            if (hk->arrived_reset) {
                S32(st, c + FIG_FRAME_TICKS) = 0;
            }
        }
        break;
    case 0x15: {
        S32(st, c + FIG_FRAME_PERIOD) = 0x0C;
        int32_t t = S16(st, c + FIG_DEST_BUILDING);
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0 && frame_tick) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
            U8(st, c + FIG_U6B) = 0;
            U8(st, c + FIG_ACTION) = 0x17;
            break;
        }
        if (t != 0) {
            uint32_t g = FIG(t);
            if ((U16(st, g + FIG_X) != U16(st, c + FIG_DEST_X) || U16(st, g + FIG_Y) != U16(st, c + FIG_DEST_Y)) && frame_tick) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                uint32_t cc = FIG(CUR(st));
                U16(st, cc + FIG_DEST_X) = U16(st, g + FIG_X);
                U16(st, cc + FIG_DEST_Y) = U16(st, g + FIG_Y);
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE, CUR(st), speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 0x17;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        }
        break;
    }
    case 0x0D:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0C;
        GAME_CALL(st, FN_FIGURE_MOVE, id, (int32_t) ((uint32_t) speed * 2u));
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 0x17;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        }
        if (frame_tick) {
            animal_seek_prey(st, id);
            id = CUR(st);
        }
        break;
    case 0x0C:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        if (U16(st, c + FIG_WAIT_TICKS) == 0) {
            GAME_CALL(st, FN_FIGURE_HIT_SOUND, id);
            id = CUR(st);
        }
        c = FIG(id);
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x1F4) {
            U8(st, c + FIG_ACTION) = 4;
        }
        break;
    default:
        S32(st, c + FIG_FRAME_PERIOD) = hk->idle_period;
        U8(st, c + FIG_ACTION) = 8;
        break;
    }
    c = FIG(id);
    int32_t n = S32(st, c + FIG_FRAME_TICKS);
    if (n >= S32(st, c + FIG_FRAME_PERIOD) || n < 0) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
    }
    int32_t d = view_direction(st, c, 0, 8);
    int32_t k8 = (int32_t) (8u * U32(st, c + FIG_FRAME_TICKS));
    uint32_t img = 0;
    int with_frame = 1;
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
    case 0x0C: {
        int16_t t = S16(st, c + FIG_WAIT_TICKS);
        S32(st, c + FIG_U04) = add32(image(st, hk->img_die), t >= 6 ? 5 : t);
        return;
    }
    case 5:
        img = hk->img_hurt;
        break;
    case 8:
        img = hk->img_still, with_frame = hk->still_frame;
        break;
    case 9:
    case 0x0A:
    case 0x0D:
    case 0x15:
        img = hk->img_move;
        break;
    case 0x12:
        img = hk->img_graze1;
        break;
    case 0x13:
        img = hk->img_graze2;
        break;
    case 0x14:
        img = hk->img_idle, with_frame = 0;
        break;
    default:
        break;
    }
    if (img != 0) {
        S32(st, c + FIG_U04) = add32(add32(image(st, img), with_frame ? k8 : 0), d);
    }
}

void figure_action_66(game_state *st)
{
    /* @bin 0x004A1130 a herd animal (formation +0x60): grazes (0x12, 0x13), moves around the herd
     * (9), flees (0x0D), follows a target (0x15); +0x148 counts ticks per frame up to +0x144 */
    static const herd_kind k = {3, 0, 0x32, 1, 0, 0, 1, 0, G_IMAGE_GROUP_662, G_IMAGE_GROUP_660, 0, G_IMAGE_GROUP_659,
                                G_IMAGE_GROUP_663, G_IMAGE_GROUP_663, G_IMAGE_GROUP_663, 0};
    herd_animal(st, &k);
}

void figure_action_53(game_state *st)
{
    /* @bin 0x0049DC50 as 0x004A1130 with other timings and sprites */
    static const herd_kind k = {0, 1, 0x96, 0, 1, 0x96, 0, 1, G_IMAGE_GROUP_202, G_IMAGE_GROUP_201, G_IMAGE_GROUP_200, G_IMAGE_GROUP_200,
                                G_IMAGE_GROUP_203, G_IMAGE_GROUP_204, G_IMAGE_GROUP_204, 0};
    herd_animal(st, &k);
}

void figure_action_68(game_state *st)
{
    /* @bin 0x0049F710 as 0x004A1130 with other timings and sprites */
    static const herd_kind k = {2, 0, 0, 1, 0, 0, 1, 0, G_IMAGE_GROUP_672, G_IMAGE_GROUP_670, G_IMAGE_GROUP_669, G_IMAGE_GROUP_669,
                                G_IMAGE_GROUP_673, G_IMAGE_GROUP_673, G_IMAGE_GROUP_673, 1};
    herd_animal(st, &k);
}

void figure_action_67(game_state *st)
{
    /* @bin 0x004A04D0 a herd animal like 0x004A1130, with its own timings (+0x144 per action),
     * waits of 150 + rand % 200 and sprites */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int frame_tick = 0; /* [esp+0x13] */
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    S32(st, c + FIG_FRAME_TICKS) += 1;
    if (S32(st, c + FIG_FRAME_TICKS) >= S32(st, c + FIG_FRAME_PERIOD)) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
        U8(st, c + FIG_ANIM_FRAME) += 1;
        frame_tick = 1;
    }
    U16(st, CURRENT_CITY(st) + CITY_U447A) += 1;
    U8(st, c + FIG_TERRAIN_USAGE) = 5;
    U8(st, c + FIG_U0C) = 0;
    U8(st, c + FIG_INVISIBLE) = 0;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    uint32_t fm = FORMATION(S16(st, c + FIG_FORMATION));
    uint8_t dir;
    switch (U8(st, c + FIG_ACTION)) {
    case 5:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0B;
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 1, 2);
        GAME_CALL(st, FN_FIGURE_FIGHT, CUR(st));
        id = CUR(st);
        break;
    case 4:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        break;
    case 8:
        S32(st, c + FIG_FRAME_PERIOD) = 0;
        U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
        U8(st, c + FIG_ACTION) = (rand15(st) & 1) ? 0x12 : 0x13;
        U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (rand15(st) % 200 + 0x96);
        break;
    case 0x18:
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            S32(st, c + FIG_FRAME_TICKS) = 0;
            S32(st, c + FIG_FRAME_PERIOD) = 8;
            U8(st, c + FIG_ACTION) = 8;
        }
        S32(st, c + FIG_U04) = 0;
        return;
    case 0x13:
        S32(st, c + FIG_FRAME_PERIOD) = 8;
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            U8(st, c + FIG_ACTION) = 8;
        }
        break;
    case 0x12:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0A;
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            if (herd_move(st, fm, id, 0)) {
                animal_seek_prey(st, CUR(st));
                id = CUR(st);
                break;
            }
            U16(st, FIG(CUR(st)) + FIG_WAIT_TICKS) = 0xC8;
        }
        animal_seek_prey(st, CUR(st));
        id = CUR(st);
        break;
    case 0x17:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0B;
        if (!herd_move(st, fm, id, 0)) {
            c = FIG(CUR(st));
            U8(st, c + FIG_ACTION) = 0x12;
            U16(st, c + FIG_WAIT_TICKS) = 0xC8;
        }
        id = CUR(st);
        break;
    case 0x14:
        S32(st, c + FIG_FRAME_PERIOD) = 8;
        animal_seek_prey(st, id);
        id = CUR(st);
        break;
    case 9:
    case 0x0A:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 8;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        } else if (dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            if (rand15(st) & 1) {
                U8(st, c + FIG_ACTION) = 0x12;
                S32(st, c + FIG_FRAME_PERIOD) = 0x0A;
            } else {
                U8(st, c + FIG_ACTION) = 0x13;
                S32(st, c + FIG_FRAME_PERIOD) = 4;
            }
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (rand15(st) % 200 + 0x96);
        }
        break;
    case 0x15: {
        S32(st, c + FIG_FRAME_PERIOD) = 0x0B;
        int32_t t = S16(st, c + FIG_DEST_BUILDING);
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0 && frame_tick) {
            U16(st, c + FIG_WAIT_TICKS) = 0;
            U8(st, c + FIG_U6B) = 0;
            U8(st, c + FIG_ACTION) = 0x17;
            break;
        }
        if (t != 0) {
            uint32_t g = FIG(t);
            if ((U16(st, g + FIG_X) != U16(st, c + FIG_DEST_X) || U16(st, g + FIG_Y) != U16(st, c + FIG_DEST_Y)) && frame_tick) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                uint32_t cc = FIG(CUR(st));
                U16(st, cc + FIG_DEST_X) = U16(st, g + FIG_X);
                U16(st, cc + FIG_DEST_Y) = U16(st, g + FIG_Y);
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE, CUR(st), speed);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 0x17;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        }
        break;
    }
    case 0x0D:
        S32(st, c + FIG_FRAME_PERIOD) = 0x0B;
        GAME_CALL(st, FN_FIGURE_MOVE, id, (int32_t) ((uint32_t) speed * 2u));
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_ACTION) = 0x17;
            U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
        }
        if (frame_tick) {
            animal_seek_prey(st, id);
            id = CUR(st);
        }
        break;
    case 0x0C:
        S32(st, c + FIG_FRAME_PERIOD) = 6;
        if (U16(st, c + FIG_WAIT_TICKS) == 0) {
            GAME_CALL(st, FN_FIGURE_HIT_SOUND, id);
            id = CUR(st);
        }
        c = FIG(id);
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x1F4) {
            U8(st, c + FIG_ACTION) = 4;
        }
        break;
    default:
        S32(st, c + FIG_FRAME_PERIOD) = 1;
        U8(st, c + FIG_ACTION) = 8;
        break;
    }
    c = FIG(id);
    int32_t n = S32(st, c + FIG_FRAME_TICKS);
    if (n >= S32(st, c + FIG_FRAME_PERIOD) || n < 0) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
    }
    int32_t d = view_direction(st, c, 0, 8);
    int32_t k8 = (int32_t) (8u * U32(st, c + FIG_FRAME_TICKS));
    switch (U8(st, c + FIG_ACTION)) {
    case 4:
    case 0x0C: {
        int16_t t = S16(st, c + FIG_WAIT_TICKS);
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_666), 8 * (t >= 6 ? 5 : t)), d);
        break;
    }
    case 5:
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_665), k8), d);
        break;
    case 8:
        S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_664), d);
        break;
    case 9:
    case 0x0A:
    case 0x0D:
    case 0x15:
    case 0x17:
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_664), k8), d);
        break;
    case 0x12:
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_667), k8), d);
        break;
    case 0x13:
    case 0x14:
        S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_668), 8 * (S32(st, c + FIG_FRAME_TICKS) / 2)), d);
        break;
    default:
        break;
    }
}

/* ---- type 0x54 ------------------------------------------------------------------------------ */

/* @bin 0x0049EC5B a spot around (cx, cy) (0x004EAB10 with a random index 0 .. 15, offsets table
 * 0x005CD978): go there (action 9); 1 when one was found */
static int herd_move_54(game_state *st, int32_t cx, int32_t cy, uint32_t c)
{
    int32_t r = rand15(st) & 0x0F;
    int32_t k = herd_spot(st, cx, cy, r, S16(st, c + FIG_X), S16(st, c + FIG_Y), 0);
    if (k < 0 || k >= 0x10) {
        return 0;
    }
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
    c = FIG(CUR(st));
    U16(st, c + FIG_DEST_X) = (uint16_t) (U16(st, G_DIRECTIONS + 8u * (uint32_t) k) + (uint16_t) cx);
    U16(st, c + FIG_DEST_Y) = (uint16_t) (U16(st, G_DIRECTIONS_DY + 8u * (uint32_t) k) + (uint16_t) cy);
    U8(st, c + FIG_ACTION) = 9;
    return 1;
}

/* the target +0x5E alive and not dying */
static int target_alive(game_state *st, int32_t t)
{
    uint32_t g = FIG(t);
    return t != 0 && U8(st, g + FIG_ACTION) != 4 && U8(st, g + FIG_STATE) == 1;
}

/* the destination differs from the target's tile */
static int target_moved(game_state *st, uint32_t c, int32_t t)
{
    uint32_t g = FIG(t);
    return U16(st, g + FIG_X) != U16(st, c + FIG_DEST_X) || U16(st, g + FIG_Y) != U16(st, c + FIG_DEST_Y);
}

static void follow_target(game_state *st, int32_t t)
{
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, CUR(st));
    uint32_t c = FIG(CUR(st)), g = FIG(t);
    U16(st, c + FIG_DEST_X) = U16(st, g + FIG_X);
    U16(st, c + FIG_DEST_Y) = U16(st, g + FIG_Y);
}

static void attack_again(uint32_t c, game_state *st)
{
    U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
    U8(st, c + FIG_ACTION) = 0x17;
    U16(st, c + FIG_WAIT_TICKS) = (uint16_t) (U8(st, c + FIG_U62) * 5);
}

void figure_action_54(game_state *st)
{
    /* @bin 0x0049E8B0 an animal of a herd (formation +0x60) that also swims: slower in deep water
     * (the tile and its 8 neighbours water), other sprites there */
    int32_t id = CUR(st);
    uint32_t c = FIG(id);
    int frame_tick = 0; /* [esp+0x13] */
    int32_t speed = type_speed(st, U8(st, c + FIG_TYPE));
    S32(st, c + FIG_FRAME_TICKS) += 1;
    if (S32(st, c + FIG_FRAME_TICKS) >= S32(st, c + FIG_FRAME_PERIOD)) {
        S32(st, c + FIG_FRAME_TICKS) = 0;
        U8(st, c + FIG_ANIM_FRAME) += 1;
        frame_tick = 1;
    }
    U8(st, c + FIG_TERRAIN_USAGE) = 6;
    U8(st, c + FIG_U0C) = 0;
    U8(st, c + FIG_INVISIBLE) = 0;
    uint32_t g = U32(st, c + FIG_GRID_OFFSET);
    uint32_t fm = FORMATION(S16(st, c + FIG_FORMATION));
    /* @bin 0x0049E93D in deep water ([esp+0x12]) */
    static const int32_t around[9] = {0, -228, 1, 228, -1, -227, -229, 229, 227};
    int water = 1;
    for (int i = 0; i < 9; i++) {
        if ((U8(st, G_GRID_TERRAIN + 4u * (g + (uint32_t) around[i])) & 4) == 0) {
            water = 0;
            break;
        }
    }
    U16(st, CURRENT_CITY(st) + CITY_U447A) += 1;
    uint8_t frame = (uint8_t) (U8(st, c + FIG_ANIM_FRAME) + 1);
    U8(st, c + FIG_ANIM_FRAME) = frame;
    if (frame >= 12) {
        U8(st, c + FIG_ANIM_FRAME) = 0;
    }
    int32_t half = water ? speed >> 1 : speed;
    int32_t t;
    uint8_t dir;
    switch (U8(st, c + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 6, 2);
        GAME_CALL(st, FN_FIGURE_FIGHT, CUR(st));
        id = CUR(st);
        break;
    case 4:
        if (U16(st, c + FIG_WAIT_TICKS) == 0) {
            GAME_CALL(st, FN_FIGURE_HIT_SOUND, id);
            id = CUR(st);
        }
        S32(st, FIG(id) + FIG_FRAME_PERIOD) = 8;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        id = CUR(st);
        break;
    case 0x18:
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            S32(st, c + FIG_FRAME_TICKS) = 0;
            S32(st, c + FIG_FRAME_PERIOD) = 8;
            U8(st, c + FIG_ACTION) = 8;
        }
        S32(st, c + FIG_U04) = 0;
        return;
    case 0x1A: {
        int32_t h = S32(st, c + FIG_FRAME_PERIOD) >> 1;
        if (S32(st, c + FIG_FRAME_TICKS) == h) {
            U16(st, c + FIG_WAIT_TICKS) -= 1;
            if (S16(st, c + FIG_WAIT_TICKS) > 0) {
                S32(st, c + FIG_FRAME_TICKS) = (S32(st, c + FIG_FRAME_PERIOD) >> 1) - 1;
            }
        } else if (frame_tick) {
            animal_pick_action(st, id);
            id = CUR(st);
        }
        break;
    }
    case 0x13:
        GAME_CALL(st, FN_FIGURE_MOVE, id, half);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            animal_pick_action(st, id);
            id = CUR(st);
        }
        break;
    case 0x12:
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            if (herd_move_54(st, S16(st, fm - FM_TYPE + FM_DEST_X), S16(st, fm - FM_TYPE + FM_DEST_Y), c)) {
                animal_seek_prey(st, CUR(st));
                id = CUR(st);
                break;
            }
            U16(st, FIG(CUR(st)) + FIG_WAIT_TICKS) = 0x64;
        }
        animal_seek_prey(st, CUR(st));
        id = CUR(st);
        break;
    case 0x14:
        animal_seek_prey(st, id);
        id = CUR(st);
        break;
    case 0x17:
        if (!herd_move_54(st, S16(st, fm - FM_TYPE + FM_DEST_X), S16(st, fm - FM_TYPE + FM_DEST_Y), c) &&
            !herd_move_54(st, S16(st, fm - FM_TYPE + FM_U36), S16(st, fm - FM_TYPE + FM_U38), FIG(CUR(st)))) {
            /* @bin 0x0049EFC5 nowhere to go: gone */
            U8(st, FIG(CUR(st)) + FIG_STATE) = 2;
            return;
        }
        id = CUR(st);
        break;
    case 8:
        animal_pick_action(st, id);
        id = CUR(st);
        break;
    case 9:
    case 0x0A:
        GAME_CALL(st, FN_FIGURE_MOVE, id, half);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            animal_pick_action(st, id);
            id = CUR(st);
        }
        break;
    case 0x19:
        t = S16(st, c + FIG_DEST_BUILDING);
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            U8(st, c + FIG_U6B) = 0;
            U8(st, c + FIG_ACTION) = 0x17;
            break;
        }
        if (target_alive(st, t)) {
            if (frame_tick && target_moved(st, c, t)) {
                follow_target(st, t);
            }
        } else {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            animal_seek_target(st, id);
        }
        GAME_CALL(st, FN_FIGURE_MOVE, CUR(st), water ? speed : (int32_t) ((uint32_t) speed * 2u));
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A) {
            U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
            U8(st, c + FIG_U6B) = 0;
            U8(st, c + FIG_ACTION) = 0x17;
        } else if (dir == 8) {
            if (t != 0 && target_moved(st, c, t) && U8(st, FIG(t) + FIG_ACTION) != 4 && U8(st, FIG(t) + FIG_STATE) == 1) {
                follow_target(st, t);
            } else {
                U8(st, c + FIG_DIRECTION) = U8(st, c + FIG_PREV_DIRECTION);
                animal_seek_target(st, id);
            }
            id = CUR(st);
        }
        break;
    case 0x15:
        t = S16(st, c + FIG_DEST_BUILDING);
        U16(st, c + FIG_WAIT_TICKS) -= 1;
        if (S16(st, c + FIG_WAIT_TICKS) <= 0) {
            U8(st, c + FIG_U6B) = 0;
            U8(st, c + FIG_ACTION) = 0x17;
            break;
        }
        if (frame_tick && t != 0 && target_moved(st, c, t)) {
            follow_target(st, t);
        }
        GAME_CALL(st, FN_FIGURE_MOVE, CUR(st), half);
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || (dir == 8 && (t == 0 || !target_moved(st, c, t)))) {
            attack_again(c, st);
        } else if (dir == 8) {
            follow_target(st, t);
            id = CUR(st);
        }
        break;
    case 0x0D:
        GAME_CALL(st, FN_FIGURE_MOVE, id, half);
        animal_seek_prey(st, CUR(st));
        id = CUR(st);
        c = FIG(id);
        dir = U8(st, c + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = CUR(st);
        } else if (dir == 0x0A || dir == 8) {
            attack_again(c, st);
        }
        break;
    case 0x0C:
        if (U16(st, c + FIG_WAIT_TICKS) == 0) {
            GAME_CALL(st, FN_FIGURE_HIT_SOUND, id);
            id = CUR(st);
        }
        c = FIG(id);
        U16(st, c + FIG_WAIT_TICKS) += 1;
        if (S16(st, c + FIG_WAIT_TICKS) > 0x1F4) {
            U8(st, c + FIG_ACTION) = 4;
        }
        break;
    default:
        U8(st, c + FIG_ACTION) = 8;
        break;
    }
    id = CUR(st);
    c = FIG(id);
    uint8_t action = U8(st, c + FIG_ACTION);
    if (U8(st, G_SIDE_SHOW) != 0) {
        /* @bin 0x0049F418 the other sprite set (0x007027E1) */
        if (action == 4 || action == 0x0C) {
            S32(st, c + FIG_FRAME_PERIOD) = 0x12;
            int32_t k = S16(st, c + FIG_WAIT_TICKS);
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_606), k >= 0x12 ? 0x24 : k + 0x13);
        } else {
            S32(st, c + FIG_FRAME_PERIOD) = 0x0E;
            S32(st, c + FIG_U04) = add32(add32(S32(st, c + FIG_FRAME_TICKS), image(st, G_IMAGE_GROUP_606)), 5);
        }
        return;
    }
    int32_t d = view_direction(st, c, 0, 8);
    int32_t k8 = (int32_t) (8u * U32(st, c + FIG_FRAME_TICKS));
    switch (action) {
    case 4:
    case 0x0C: {
        int16_t k = S16(st, c + FIG_WAIT_TICKS);
        if (water) {
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_588), k >= 0x17 ? 0x16 : k);
        } else {
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_600), k >= 8 ? 7 : k);
        }
        break;
    }
    case 8:
    case 0x17:
        S32(st, c + FIG_U04) = add32(image(st, water ? G_IMAGE_GROUP_604 : G_IMAGE_GROUP_599), d);
        break;
    case 9:
    case 0x0A:
    case 0x13:
    case 0x15:
        S32(st, c + FIG_U04) = add32(add32(image(st, water ? G_IMAGE_GROUP_602 : G_IMAGE_GROUP_599), k8), d);
        break;
    case 0x19:
        S32(st, c + FIG_U04) = add32(add32(image(st, water ? G_IMAGE_GROUP_602 : G_IMAGE_GROUP_601), k8), d);
        break;
    case 0x1A:
        if (water) {
            S32(st, c + FIG_U04) = add32(add32(image(st, G_IMAGE_GROUP_603), k8), d);
        } else {
            S32(st, c + FIG_U04) = add32(image(st, G_IMAGE_GROUP_599), d);
        }
        break;
    default:
        S32(st, c + FIG_U04) = add32(add32(image(st, water ? G_IMAGE_GROUP_604 : G_IMAGE_GROUP_605), k8), d);
        break;
    }
}

/* ---- table ---------------------------------------------------------------------------------- */

#define ACTION(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
ACTION(run_action_47, figure_action_47)

static int32_t run_ferry_passengers(game_state *st, const int32_t *a)
{
    return ferry_passengers(st, a[0]);
}

static int32_t run_figure_may_leave(game_state *st, const int32_t *a)
{
    return figure_may_leave(st, a[0]);
}

static int32_t run_ferry_board(game_state *st, const int32_t *a)
{
    ferry_board(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_ferry_unload(game_state *st, const int32_t *a)
{
    ferry_unload(st, a[0], a[1]);
    return 0;
}

static int32_t run_ferry_move(game_state *st, const int32_t *a)
{
    ferry_move(st, a[0], a[1]);
    return 0;
}

static int32_t run_tile_figure_of_type(game_state *st, const int32_t *a)
{
    return tile_figure_of_type(st, a[0], a[1]);
}

static int32_t run_tile_target(game_state *st, const int32_t *a)
{
    return tile_target(st, a[0]);
}

static int32_t run_herd_spot(game_state *st, const int32_t *a)
{
    return herd_spot(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_animal_pick_action(game_state *st, const int32_t *a)
{
    animal_pick_action(st, a[0]);
    return 0;
}

static int32_t run_animal_seek_prey(game_state *st, const int32_t *a)
{
    return animal_seek_prey(st, a[0]);
}

static int32_t run_animal_seek_target(game_state *st, const int32_t *a)
{
    animal_seek_target(st, a[0]);
    return 0;
}

static int32_t run_worker_start_job(game_state *st, const int32_t *a)
{
    worker_start_job(st, a[0]);
    return 0;
}

static int32_t run_worker_join(game_state *st, const int32_t *a)
{
    worker_join(st, a[0]);
    return 0;
}

static int32_t run_worker_at_b7(game_state *st, const int32_t *a)
{
    worker_at_b7(st, a[0], a[1]);
    return 0;
}

static int32_t run_monument_work_401f50(game_state *st, const int32_t *a)
{
    return monument_work_401f50(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
}

static int32_t run_monument_work_417940(game_state *st, const int32_t *a)
{
    return monument_work_417940(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
}

static int32_t run_monument_work_4e6e70(game_state *st, const int32_t *a)
{
    return monument_work_4e6e70(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
}

static int32_t run_ground_rule(game_state *st, const int32_t *a)
{
    return ground_rule(st, a[0]);
}

static int32_t run_type_is_boat(game_state *st, const int32_t *a)
{
    return type_is_boat(st, a[0]);
}

static int32_t run_find_tree(game_state *st, const int32_t *a)
{
    return find_tree(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_job_finder_bc480(game_state *st, const int32_t *a)
{
    /* @bin 0x004BC480 0x004F01C0 with the same arguments, which is 0x004EFE30(a, b, c, 0) */
    return GAME_CALL(st, FN_PYRAMID_ASSIGN_4EFE30, a[0], a[1], a[2], 0);
}

static int32_t run_return_zero(game_state *st, const int32_t *a)
{
    return return_zero(st, a[0]);
}
ACTION(run_action_49, figure_action_49)
ACTION(run_action_4a, figure_action_4a)
ACTION(run_action_4b, figure_action_4b)
ACTION(run_action_4c, figure_action_4c)
ACTION(run_action_4d, figure_action_4d)
ACTION(run_action_4e, figure_action_4e)
ACTION(run_action_4f, figure_action_4f)
ACTION(run_action_50, figure_action_50)
ACTION(run_action_66, figure_action_66)
ACTION(run_action_53, figure_action_53)
ACTION(run_action_54, figure_action_54)
ACTION(run_action_67, figure_action_67)
ACTION(run_action_68, figure_action_68)

const ported_function figc_ports[] = {
    {0x004A34B0u, "ferry_passengers", 1, 32, 0, run_ferry_passengers, 0},
    {0x004A35E0u, "figure_may_leave", 1, 8, 0, run_figure_may_leave, 0},
    {0x004A3B00u, "ferry_move", 2, 0, 0, run_ferry_move, 0},
    {0x004A36E0u, "ferry_board", 3, 0, 0, run_ferry_board, 0},
    {0x004A38A0u, "ferry_unload", 2, 0, 0, run_ferry_unload, 0},
    {0x0047D350u, "return_zero", 1, 32, 0, run_return_zero, 0},
    {0x004B57E0u, "tile_figure_of_type", 2, 32, 0, run_tile_figure_of_type, 0},
    {0x004B5770u, "tile_target", 1, 32, 0, run_tile_target, 0},
    {0x004EAB10u, "herd_spot", 6, 32, 0, run_herd_spot, 0},
    {0x004EC7C0u, "animal_pick_action", 1, 0, 0, run_animal_pick_action, 0},
    {0x004BC480u, "job_finder_bc480", 3, 32, 0, run_job_finder_bc480, 0},
    {0x004F01C0u, "job_finder_4f01c0", 3, 32, 0, run_job_finder_bc480, 0},
    {0x004EAAC0u, "ground_rule", 1, 8, 0, run_ground_rule, 0},
    {0x004F1CB0u, "worker_start_job", 1, 0, 0, run_worker_start_job, 0},
    {0x004F2410u, "worker_join", 1, 0, 0, run_worker_join, 0},
    {0x004EF3A0u, "worker_at_b7", 2, 0, 0, run_worker_at_b7, 0},
    {0x00401F50u, "monument_work_401f50", 3, 32, 0, run_monument_work_401f50, 0},
    {0x00417940u, "monument_work_417940", 3, 32, 0, run_monument_work_417940, 0},
    {0x004E6E70u, "monument_work_4e6e70", 3, 32, 0, run_monument_work_4e6e70, 0},
    {0x004EADA0u, "type_is_boat", 1, 8, 0, run_type_is_boat, 0},
    {0x004867E0u, "find_tree", 4, 32, 0, run_find_tree, 0},
    {0x004EAEC0u, "animal_seek_prey", 1, 8, 0, run_animal_seek_prey, 0},
    {0x004EC990u, "animal_seek_target", 1, 0, 0, run_animal_seek_target, 0},
    /* the ring search runs only with the animals' callback 0x004EAE20 (both callers) */
    {0x004EC0F0u, "ring_search (in actions_c.c, callback 0x004EAE20)", 7, 32, 0, NULL, 0},
    {0x004EAE20u, "animal_prey_on_tile (in actions_c.c)", 2, 32, 0, NULL, 0},
    {0x004A1EF0u, "figure_action_47", 0, 0, 0, run_action_47, 0},
    {0x004A2430u, "figure_action_49", 0, 0, 0, run_action_49, 0},
    {0x004A2E90u, "figure_action_4a", 0, 0, 0, run_action_4a, 0},
    {0x004A3070u, "figure_action_4b", 0, 0, 0, run_action_4b, 0},
    {0x004A3B70u, "figure_action_4c", 0, 0, 0, run_action_4c, 0},
    {0x004A4FD0u, "figure_action_4d", 0, 0, 0, run_action_4d, 0},
    {0x004A7030u, "figure_action_4e", 0, 0, 0, run_action_4e, 0},
    {0x004A7600u, "figure_action_4f", 0, 0, 0, run_action_4f, 0},
    {0x004A7EA0u, "figure_action_50", 0, 0, 0, run_action_50, 0},
    {0x004A1130u, "figure_action_66", 0, 0, 0, run_action_66, 0},
    {0x0049DC50u, "figure_action_53", 0, 0, 0, run_action_53, 0},
    {0x0049E8B0u, "figure_action_54", 0, 0, 0, run_action_54, 0},
    {0x004A04D0u, "figure_action_67", 0, 0, 0, run_action_67, 0},
    {0x0049F710u, "figure_action_68", 0, 0, 0, run_action_68, 0},
};
const int figc_ports_count = sizeof(figc_ports) / sizeof(figc_ports[0]);
