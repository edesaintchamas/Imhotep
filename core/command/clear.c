/* Imhotep - clearing land (player command, docs/commands.md section 2.5): 0x004743F0 and
 * 0x00474700, the four confirmation callbacks, the popup request 0x004264D0, bridges.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "command/clear.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


#define T32(st, off) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (off))
#define G8(st, base, off) U8(st, (base) + (uint32_t) (off))
#define BTYPE(st, id) S16(st, BUILDING_ADDR(id) + BLD_TYPE)

int clear_is_monument_type(int32_t type)
{
    /* @bin 0x00474380 */
    switch (type) {
    case 0xB7: case 0xDE: case 0xD5: case 0xCF: case 0xD0: case 0xD2: case 0xE6:
    case 0xE1: case 0xE3: case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xE4:
        return 1;
    default:
        return 0;
    }
}

static int is_temple_complex(int32_t type)
{
    /* @bin 0x00474577 / 0x00474AE8 the types asked as "temple complex" (text 0x6A) */
    return (type >= 0x41 && type <= 0x45) || type == 0x61 || (type >= 0x9C && type <= 0xA0);
}

void popup_confirm_open(game_state *st, int32_t text_id, int32_t callback, int32_t kind)
{
    /* @bin 0x004264D0 the window to return to, the popup's place (0x00E38DA8 / 0x00E38DA4 + 0x50),
     * window 0x3F, answer cleared, callback / text / kind, redraw */
    U32(st, G_POPUP_BACK) = U8(st, G_WINDOW);
    U32(st, G_POPUP_X) = U32(st, G_LAYOUT_DX) + 0x50u;
    U32(st, G_POPUP_Y) = U32(st, G_LAYOUT_DY) + 0x50u;
    U8(st, G_WINDOW) = 0x3F;
    U8(st, G_CONFIRM_ANSWER) = 0;
    S32(st, G_POPUP_ACTION) = callback;
    S32(st, G_POPUP_TEXT) = text_id;
    S32(st, G_POPUP_BUTTONS) = kind;
    U8(st, G_REDRAW) = 1;
}

void clear_land(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x004743F0 */
    S32(st, G_OK_BRIDGE) = 0;
    S32(st, G_OK_FORT) = 0;
    S32(st, G_OK_MONUMENT) = 0;
    S32(st, G_OK_TEMPLE) = 0;
    if (preview != 0) {
        clear_land_apply(st, preview, x1, y1, x2, y2);
        return;
    }
    int32_t xmin = x1, xmax = x2, ymin = y1, ymax = y2;
    if (x1 > x2) {
        xmin = x2;
        xmax = x1;
    }
    if (y1 > y2) {
        ymin = y2;
        ymax = y1;
    }
    if (xmin < 0) {
        xmin = 0;
    }
    if (xmax >= S32(st, G_MAP_WIDTH)) {
        xmax = S32(st, G_MAP_WIDTH) - 1;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (ymax >= S32(st, G_MAP_HEIGHT)) {
        ymax = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t width = xmax - xmin + 1;
    int fort = 0, bridge = 0, monument = 0, temple = 0;
    S32(st, G_WALK_Y) = ymin;
    S32(st, G_TILE_CURSOR) = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (ymin * 228) + (uint32_t) xmin);
    while (S32(st, G_WALK_Y) <= ymax) {
        int32_t off = S32(st, G_TILE_CURSOR);
        S32(st, G_WALK_X) = xmin;
        if (xmin <= xmax) {
            do {
                int32_t bid = S16(st, G_GRID_BUILDING + 2u * (uint32_t) off);
                uint8_t sprite = G8(st, G_SPRITE, off);
                uint32_t bridge_bit = T32(st, off) & 4u;
                if (bid != 0) {
                    /* @bin 0x00474521 fort parts 0x39 / 0x36 (text 0x10) */
                    uint16_t t = U16(st, BUILDING_ADDR(bid) + BLD_TYPE);
                    if (t == 0x39 || t == 0x36) {
                        fort = 1;
                    }
                }
                /* @bin 0x00474535 a bridge: sprite overlay on a terrain-4 tile without building (text 0x14) */
                if (sprite != 0 && bridge_bit != 0 && bid == 0) {
                    bridge = 1;
                }
                if (clear_is_monument_type(BTYPE(st, bid))) {
                    monument = 1;
                }
                if (is_temple_complex(BTYPE(st, bid))) {
                    temple = 1;
                }
                S32(st, G_WALK_X) = S32(st, G_WALK_X) + 1;
                off = S32(st, G_TILE_CURSOR) + 1;
                S32(st, G_TILE_CURSOR) = off;
            } while (S32(st, G_WALK_X) <= xmax);
        }
        S32(st, G_WALK_Y) = S32(st, G_WALK_Y) + 1;
        S32(st, G_TILE_CURSOR) = off + (0xE4 - width);
    }
    S32(st, G_RECT_X1) = x1;
    S32(st, G_RECT_Y1) = y1;
    S32(st, G_RECT_X2) = x2;
    S32(st, G_RECT_Y2) = y2;
    /* @bin 0x00474628 one popup, by priority: monument 0x68, temple complex 0x6A, fort 0x10, bridge 0x14 */
    if (monument) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x68, FN_CLEAR_CONFIRM_MONUMENT, 2);
    } else if (temple) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x6A, FN_CLEAR_CONFIRM_TEMPLE, 2);
    } else if (fort) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x10, FN_CLEAR_CONFIRM_FORT, 2);
    } else if (bridge) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x14, FN_CLEAR_CONFIRM_BRIDGE, 2);
    } else {
        clear_land_apply(st, 0, x1, y1, x2, y2);
    }
}

static void clear_confirmed(game_state *st, uint32_t flag)
{
    /* @bin 0x004021D0 answer 1 -> 1, anything else -> -1; then 0x00474700(0, stored rectangle) */
    S32(st, flag) = U8(st, G_CONFIRM_ANSWER) == 1 ? 1 : -1;
    clear_land_apply(st, 0, S32(st, G_RECT_X1), S32(st, G_RECT_Y1), S32(st, G_RECT_X2), S32(st, G_RECT_Y2));
}

void clear_confirm_monument(game_state *st)
{
    clear_confirmed(st, G_OK_MONUMENT); /* @bin 0x004021D0 */
}

void clear_confirm_temple(game_state *st)
{
    clear_confirmed(st, G_OK_TEMPLE); /* @bin 0x00402210 */
}

void clear_confirm_fort(game_state *st)
{
    clear_confirmed(st, G_OK_FORT); /* @bin 0x00402250 */
}

void clear_confirm_bridge(game_state *st)
{
    clear_confirmed(st, G_OK_BRIDGE); /* @bin 0x00402290 */
}

/* @bin 0x0047F837 / 0x0047F9DA a bridge runs along x or y: the direction with more bridge
 * neighbours (terrain 4 and a sprite) on the y axis wins ties; returns the step */
static int32_t bridge_step(game_state *st, int32_t off)
{
    int32_t ny = 0, nx = 0;
    if ((T32(st, off - 228) & 4u) && G8(st, G_SPRITE, off - 228) != 0) {
        ny = 1;
    }
    if ((T32(st, off - 456) & 4u) && G8(st, G_SPRITE, off - 456) != 0) {
        ny++;
    }
    if ((T32(st, off + 228) & 4u) && G8(st, G_SPRITE, off + 228) != 0) {
        ny++;
    }
    if ((T32(st, off + 456) & 4u) && G8(st, G_SPRITE, off + 456) != 0) {
        ny++;
    }
    if ((T32(st, off + 1) & 4u) && G8(st, G_SPRITE, off + 1) != 0) {
        nx = 1;
    }
    if ((T32(st, off + 2) & 4u) && G8(st, G_SPRITE, off + 2) != 0) {
        nx++;
    }
    if ((T32(st, off - 1) & 4u) && G8(st, G_SPRITE, off - 1) != 0) {
        nx++;
    }
    if ((T32(st, off - 2) & 4u) && G8(st, G_SPRITE, off - 2) != 0) {
        nx++;
    }
    return ny >= nx ? 228 : 1;
}

/* @bin 0x0047F8F2 the first tile of the bridge: back while the tile behind is a bridge tile */
static int32_t bridge_start(game_state *st, int32_t off, int32_t step)
{
    int32_t back = -step;
    if (T32(st, off + back) & 4u) {
        while (G8(st, G_SPRITE, off + back) != 0) {
            off += back;
            if (!(T32(st, off + back) & 4u)) {
                break;
            }
        }
    }
    return off;
}

int32_t bridge_figures(game_state *st, int32_t off)
{
    /* @bin 0x0047F9B0 */
    if (!(T32(st, off) & 4u) || G8(st, G_SPRITE, off) == 0) {
        return 0;
    }
    int32_t step = bridge_step(st, off);
    int32_t p = bridge_start(st, off, step);
    int32_t count = S16(st, G_FIGURE_GRID + 2u * (uint32_t) p) != 0;
    G8(st, G_GRID_BITFIELDS, p) &= 0xBF;
    if (T32(st, p + step) & 4u) {
        while (G8(st, G_SPRITE, p + step) != 0) {
            p += step;
            if (S16(st, G_FIGURE_GRID + 2u * (uint32_t) p) != 0) {
                count++;
            }
            G8(st, G_GRID_BITFIELDS, p) &= 0xBF;
            if (!(T32(st, p + step) & 4u)) {
                break;
            }
        }
    }
    return count;
}

static void bridge_remove_tile(game_state *st, int32_t p, int32_t preview)
{
    if (preview != 0) {
        G8(st, G_GRID_BITFIELDS, p) |= 0x40;
    } else {
        uint32_t t = T32(st, p);
        G8(st, G_SPRITE, p) = 0;
        T32(st, p) = t & 0xFFFFFFBFu;
    }
}

void bridge_remove(game_state *st, int32_t off, int32_t preview)
{
    /* @bin 0x0047F810 */
    if (!(T32(st, off) & 4u) || G8(st, G_SPRITE, off) == 0) {
        return;
    }
    int32_t step = bridge_step(st, off);
    int32_t p = bridge_start(st, off, step);
    bridge_remove_tile(st, p, preview);
    if (T32(st, p + step) & 4u) {
        while (G8(st, G_SPRITE, p + step) != 0) {
            p += step;
            bridge_remove_tile(st, p, preview);
            if (!(T32(st, p + step) & 4u)) {
                break;
            }
        }
    }
}

void clear_roads_square(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x0047B480 the square (x - 1, y - 1) of size `size`, clipped to the map */
    int32_t x0 = x - 1, y0 = y - 1, w = size, h = size;
    if (x0 < 0) {
        w = x0 + size;
        x0 = 0;
    } else if (x0 + size > S32(st, G_MAP_WIDTH)) {
        w = S32(st, G_MAP_WIDTH) - x0;
    }
    if (y0 < 0) {
        h = y0 + size;
        y0 = 0;
    } else if (y0 + size > S32(st, G_MAP_HEIGHT)) {
        h = S32(st, G_MAP_HEIGHT) - y0;
    }
    int32_t yend = h + y0;
    int32_t off = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (y0 * 228) + (uint32_t) x0);
    S32(st, G_WALK_Y) = y0;
    S32(st, G_TILE_CURSOR) = off;
    if (y0 >= yend) {
        return;
    }
    int32_t xend = x0 + w;
    do {
        S32(st, G_WALK_X) = x0;
        if (x0 < xend) {
            do {
                uint32_t t = T32(st, off);
                if ((t & 0x40u) && !(t & 0xCu)) {
                    GAME_CALL(st, FN_NEIGHBOURS_WITH_FLAGS, 0x08000040);
                    GAME_CALL0(st, FN_GATEHOUSE_FLAGS);
                    GAME_CALL0(st, FN_ROAD_TILE);
                    off = S32(st, G_TILE_CURSOR);
                }
                S32(st, G_WALK_X) = S32(st, G_WALK_X) + 1;
                off++;
                S32(st, G_TILE_CURSOR) = off;
            } while (S32(st, G_WALK_X) < xend);
        }
        off += 0xE4 - w;
        S32(st, G_WALK_Y) = S32(st, G_WALK_Y) + 1;
        S32(st, G_TILE_CURSOR) = off;
    } while (S32(st, G_WALK_Y) < yend);
}

void clear_pyramid_area(game_state *st, int32_t id)
{
    /* @bin 0x004F4930 a pyramid part (0xB7): its main building's kind (+0x54), when 0x004ECEA0
     * accepts it, has its ramp walked (0x004ED360) and the land redone from the kind's corner
     * (table 0x005F5D08 + the main tile +0x0C) to the tile 0x00EAAE9C + 2 */
    if (S16(st, BUILDING_ADDR(id) + BLD_TYPE) != 0xB7) {
        return;
    }
    int32_t main = id - S16(st, BUILDING_ADDR(id) + BLD_PART);
    int32_t kind = U8(st, BUILDING_ADDR(main) + BLD_MONUMENT_KIND);
    if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, kind) == 0) {
        return;
    }
    int32_t rel = (int32_t) (U32(st, G_PYRAMID_RAMP_START + 4u * (uint32_t) kind) + U32(st, BUILDING_ADDR(main) + BLD_GRID_OFFSET) -
                             U32(st, G_MAP_START));
    int32_t x = rel % 228, y = rel / 228;
    GAME_CALL(st, FN_RAMP_WALK, x, y, 5, 0);
    int32_t end = (int32_t) (U32(st, G_RAMP_LAST_TILE) - U32(st, G_MAP_START));
    int32_t ex = end % 228, ey = end / 228;
    int32_t w = ex - x + 2;
    int32_t x2 = ex + 2, y2 = ey + 2;
    GAME_CALL(st, FN_TERRAIN_LAND, x, y, x2, y2, 0, 0);
    GAME_CALL(st, FN_TERRAIN_LAND_AROUND, x, y, w);
    GAME_CALL(st, FN_MEADOW_RECT, x, y, x2, y2);
    GAME_CALL(st, FN_FLOODPLAIN_PASS, x, y, x2, y2);
    GAME_CALL(st, FN_WATER_PASS, x, y, x2, y2, 0);
    GAME_CALL(st, FN_FLAG_0X40000_RECT, x, y, x2, y2);
    GAME_CALL(st, FN_RUBBLE_RECT, x, y, x2, y2);
}

/* @bin 0x00474A1C a building on the tile: cleared with its linked parts unless kept; 1 if cleared */
static int clear_building(game_state *st, int32_t bid, int32_t preview)
{
    uint32_t b = BUILDING_ADDR(bid);
    uint16_t t = U16(st, b + BLD_TYPE);
    /* @bin 0x00474A35 never cleared: these types, and buildings already cleared (state 6) */
    if (t == 0x63 || t == 0x58 || t == 0x59 || t == 0x5D || t == 0xDA || t == 0xDB || t == 0xDC || t == 0xD7) {
        return 0;
    }
    if (U8(st, b + BLD_STATE) == 6) {
        return 0;
    }
    /* @bin 0x00474A96 forts, monuments and temple complexes only once confirmed (0x004743F0) */
    if ((t == 0x39 || t == 0x36) && preview == 0 && S32(st, G_OK_FORT) != 1) {
        return 0;
    }
    if (clear_is_monument_type((int16_t) t) && preview == 0 && S32(st, G_OK_MONUMENT) != 1) {
        return 0;
    }
    int16_t type = S16(st, b + BLD_TYPE);
    if (is_temple_complex(type) && preview == 0 && S32(st, G_OK_TEMPLE) != 1) {
        return 0;
    }
    if ((type == 0x39 || type == 0x36) && preview == 0 && S32(st, G_OK_FORT) == 1) {
        S32(st, G_UNDO_POSSIBLE) = 0; /* @bin 0x00474B51 a fort cleared: no undo */
    }
    if (type == 0xB7) {
        clear_pyramid_area(st, bid);
        GAME_CALL(st, FN_PYRAMID_CHAPEL_MARK, bid, 1);
    }
    /* @bin 0x00474B6E links freed: storage record +0xC1, table 0x009783A4 (+0xCB), 0x00EAAF68 (+0xCA) */
    uint8_t k = U8(st, b + BLD_STORAGE_ID);
    if (k != 0) {
        U8(st, G_STORAGE_SETTINGS_U08 + 0xC4u * k) = 0;
    }
    k = U8(st, b + BLD_UCB);
    if (k != 0) {
        U8(st, G_REC4A + 0x1Cu * k) = 0;
    }
    k = U8(st, b + BLD_UCA);
    if (k != 0) {
        U32(st, G_FERRY_QUEUES + 0xE0u * k) = 0;
    }
    /* @bin 0x00474BC5 a house's goods given back (0x00459020) */
    if (U8(st, b + BLD_HOUSE_SIZE) != 0 && S16(st, b + BLD_POPULATION) != 0 && preview == 0) {
        GAME_CALL(st, FN_SPAWN_FIGURE_3, S32(st, CURRENT_PLAYER_ADDR), S16(st, b + BLD_X), S16(st, b + BLD_Y),
                  S16(st, b + BLD_POPULATION));
        S16(st, b + BLD_POPULATION) = (int16_t) preview;
    }
    type = S16(st, b + BLD_TYPE);
    if (type == 0x6C || type == 0xC3 || type == 0x73) {
        U8(st, b + BLD_SHOWS) = 0;
    }
    if (U8(st, b + BLD_STATE) != 6) {
        S32(st, G_CONSTRUCTION_COUNT) = S32(st, G_CONSTRUCTION_COUNT) + 1;
        GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, bid);
    }
    U8(st, b + BLD_STATE) = 6;
    GAME_CALL(st, FN_UI_SOUND, 2);
    U8(st, b + BLD_UBF) = 1;
    /* @bin 0x00474C60 the parts linked by +0x38, then by +0x3A, at most 0x20D each */
    for (uint32_t link = 0x38; link <= 0x3A; link += 2) {
        int32_t id = bid;
        for (int32_t n = 0; n < 0x20D; n++) {
            id = S16(st, BUILDING_ADDR(id) + link);
            if (id <= 0) {
                break;
            }
            GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, id);
            U8(st, BUILDING_ADDR(id) + BLD_STATE) = 6;
            U8(st, BUILDING_ADDR(id) + BLD_UBF) = 1;
        }
    }
    return 1;
}

void clear_land_apply(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00474700 */
    int32_t radius = 0;
    S32(st, G_CONSTRUCTION_COUNT) = 0;
    GAME_CALL0(st, FN_UNDO_FORGET);
    GAME_CALL(st, FN_COPY_BYTES, G_TERRAIN_BACKUP, (int32_t) G_GRID_TERRAIN, 0x32C40);
    GAME_CALL(st, FN_COPY_BYTES, G_CANAL_BACKUP, (int32_t) G_CANAL, 0xCB10);
    GAME_CALL(st, FN_UNDO_RESTORE_IMAGES, 0);
    int32_t xmin = x1, xmax = x2, ymin = y1, ymax = y2;
    if (x1 > x2) {
        xmin = x2;
        xmax = x1;
    }
    if (y1 > y2) {
        ymin = y2;
        ymax = y1;
    }
    if (xmin < 0) {
        xmin = 0;
    }
    if (xmax >= S32(st, G_MAP_WIDTH)) {
        xmax = S32(st, G_MAP_WIDTH) - 1;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (ymax >= S32(st, G_MAP_HEIGHT)) {
        ymax = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t width = xmax - xmin + 1, height = ymax - ymin + 1;
    int32_t off = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (ymin * 228) + (uint32_t) xmin);
    S32(st, G_TILE_CURSOR) = off;
    S32(st, G_WALK_Y) = ymin;
    while (S32(st, G_WALK_Y) <= ymax) {
        S32(st, G_WALK_X) = xmin;
        if (xmin <= xmax) {
            do {
                uint32_t full = T32(st, off);
                uint32_t t = full & 0xEEFED77Fu;
                if (t & 0x02040202u) {
                    goto next;
                }
                int32_t bid = S16(st, G_GRID_BUILDING + 2u * (uint32_t) off);
                /* @bin 0x00474845 buildings: terrain 8, 0x40000000, or 0x10000000 under 0xE1 / 0xD5 */
                int on_building = (t & 8u) || (t & 0x40000000u);
                if (!on_building && bid != 0 && (full & 0x10000000u)) {
                    uint16_t bt = U16(st, BUILDING_ADDR(bid) + BLD_TYPE);
                    on_building = bt == 0xE1 || bt == 0xD5;
                }
                if (on_building) {
                    if (bid == 0) {
                        goto tail;
                    }
                    int cleared = clear_building(st, bid, preview);
                    off = S32(st, G_TILE_CURSOR);
                    if (!cleared) {
                        goto next;
                    }
                } else if (t & 0x100u) {
                    /* @bin 0x00474894 a canal: removed, the neighbours' canal shapes reopened */
                    T32(st, off) = full & 0x03052E80u;
                    G8(st, G_CANAL, off) = 0;
                    S32(st, G_CONSTRUCTION_COUNT) = S32(st, G_CONSTRUCTION_COUNT) + 1;
                    if (G8(st, G_CANAL, off - 228) == 5) {
                        G8(st, G_CANAL, off - 228) = 1;
                    }
                    if (G8(st, G_CANAL, off + 1) == 6) {
                        G8(st, G_CANAL, off + 1) = 2;
                    }
                    if (G8(st, G_CANAL, off + 228) == 5) {
                        G8(st, G_CANAL, off + 228) = 3;
                    }
                    if (G8(st, G_CANAL, off - 1) == 6) {
                        G8(st, G_CANAL, off - 1) = 4;
                    }
                } else if (t & 4u) {
                    /* @bin 0x00474904 a bridge: refused with notice 0x37 if figures are on it */
                    if (preview == 0) {
                        int32_t n = GAME_CALL(st, FN_BRIDGE_FIGURES, off);
                        off = S32(st, G_TILE_CURSOR);
                        if (n >= 1) {
                            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x37, 0, 0);
                            off = S32(st, G_TILE_CURSOR);
                            goto next;
                        }
                    }
                    if (S32(st, G_OK_BRIDGE) == 1) {
                        GAME_CALL(st, FN_BRIDGE_REMOVE, off, preview);
                        S32(st, G_CONSTRUCTION_COUNT) = S32(st, G_CONSTRUCTION_COUNT) + 1;
                        off = S32(st, G_TILE_CURSOR);
                    }
                } else if (t != 0) {
                    /* @bin 0x004749D0 trees, rubble, ... removed */
                    uint8_t e = G8(st, G_GRID_EDGE, off);
                    T32(st, off) = full & 0x03052E80u;
                    uint8_t bits = G8(st, G_GRID_BITFIELDS, off) & 0xF0;
                    e |= 0x40;
                    G8(st, G_GRID_BITFIELDS, off) = bits;
                    G8(st, G_GRID_EDGE, off) = e;
                    if (preview == 0) {
                        G8(st, G_GRID_BITFIELDS, off) = bits & 0x70;
                        G8(st, G_GRID_EDGE, off) = e | 0x40;
                    }
                    S32(st, G_CONSTRUCTION_COUNT) = S32(st, G_CONSTRUCTION_COUNT) + 1;
                }
tail:
                /* @bin 0x00474CEF terrain 0x148 cleared: the land images are redone 5 tiles around */
                if (preview == 0 && (t & 0x148u)) {
                    radius = 5;
                }
next:
                S32(st, G_WALK_X) = S32(st, G_WALK_X) + 1;
                off++;
                S32(st, G_TILE_CURSOR) = off;
            } while (S32(st, G_WALK_X) <= xmax);
        }
        S32(st, G_WALK_Y) = S32(st, G_WALK_Y) + 1;
        off += 0xE4 - width;
        S32(st, G_TILE_CURSOR) = off;
    }
    int32_t size = (width > height ? width : height) + 2;
    GAME_CALL(st, FN_TERRAIN_LAND, xmin, ymin, xmax, ymax, 0, 1);
    if (radius != 0) {
        GAME_CALL(st, FN_TERRAIN_LAND, xmin - radius, ymin - radius, xmax + radius, ymax + radius, 1, 0);
    }
    GAME_CALL(st, FN_MEADOW_RECT, xmin, ymin, xmax, ymax);
    GAME_CALL(st, FN_FLAG_0X40000_RECT, xmin, ymin, xmax, ymax);
    GAME_CALL(st, FN_FLOODPLAIN_PASS, xmin, ymin, xmax, ymax);
    GAME_CALL(st, FN_CANALS_RECT, xmin, ymin, xmax, ymax, 0);
    clear_roads_square(st, xmin, ymin, size);
    GAME_CALL(st, FN_RUBBLE_RECT, xmin, ymin, xmax, ymax);
    GAME_CALL0(st, FN_FLAG_0X20_ALL);
    GAME_CALL(st, FN_PLAZA_RECT, 0, 0, S32(st, G_MAP_WIDTH) - 1, S32(st, G_MAP_HEIGHT) - 1, 1);
    /* @bin 0x00474DC6 entertainment venues (0x1E, 0x1F, 0x21, 0xD1) redrawn over their parts */
    for (int32_t id = 1; id < BUILDING_COUNT; id++) {
        uint32_t b = BUILDING_ADDR(id);
        uint8_t state = U8(st, b + BLD_STATE);
        if (state == 0 || state == 6) {
            continue;
        }
        int16_t bt = S16(st, b + BLD_TYPE);
        if (bt == 0x1F || bt == 0x1E || bt == 0x21 || bt == 0xD1) {
            if (bt == 0x1E && S16(st, BUILDING_ADDR(S16(st, b + BLD_NEXT_PART)) + BLD_TYPE) == 0x1E) {
                GAME_CALL(st, FN_BUILDING_REDRAW_440540, id, S16(st, b + BLD_X), S16(st, b + BLD_Y));
            }
            if (GAME_CALL(st, FN_BUILDING_MAIN_PART, id) == id && S32(st, b + BLD_UD8) != 0) {
                GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, S32(st, b + BLD_UD0), bt, id);
            }
        }
    }
    GAME_CALL(st, FN_WALLS_RECT_47AFD0, xmin, ymin, size);
    GAME_CALL(st, FN_WATER_PASS, xmin, ymin, xmax, ymax, 0);
    if (preview == 0) {
        GAME_CALL(st, FN_CANALS_RECT_REDRAW, xmin - 1, ymin - 1, xmax + 1, ymax + 1);
        GAME_CALL(st, FN_ROADS_RECT, xmin - 1, ymin - 1, xmax + 1, ymax + 1);
        GAME_CALL0(st, FN_ROUTING_REBUILD_ED4140);
        GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
        GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
        GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
    }
}

static int32_t run_monument_type(game_state *st, const int32_t *a)
{
    (void) st;
    return clear_is_monument_type(a[0]);
}

static int32_t run_clear_land(game_state *st, const int32_t *a)
{
    clear_land(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_clear_apply(game_state *st, const int32_t *a)
{
    clear_land_apply(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_confirm_monument(game_state *st, const int32_t *a)
{
    (void) a;
    clear_confirm_monument(st);
    return 0;
}

static int32_t run_confirm_temple(game_state *st, const int32_t *a)
{
    (void) a;
    clear_confirm_temple(st);
    return 0;
}

static int32_t run_confirm_fort(game_state *st, const int32_t *a)
{
    (void) a;
    clear_confirm_fort(st);
    return 0;
}

static int32_t run_confirm_bridge(game_state *st, const int32_t *a)
{
    (void) a;
    clear_confirm_bridge(st);
    return 0;
}

static int32_t run_popup(game_state *st, const int32_t *a)
{
    popup_confirm_open(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_bridge_figures(game_state *st, const int32_t *a)
{
    return bridge_figures(st, a[0]);
}

static int32_t run_bridge_remove(game_state *st, const int32_t *a)
{
    bridge_remove(st, a[0], a[1]);
    return 0;
}

static int32_t run_pyramid(game_state *st, const int32_t *a)
{
    clear_pyramid_area(st, a[0]);
    return 0;
}

static int32_t run_roads_square(game_state *st, const int32_t *a)
{
    clear_roads_square(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function clear_ports[] = {
    {0x00474380u, "clear_is_monument_type", 1, 8, 0, run_monument_type, 0},
    {0x004743F0u, "clear_land", 5, 0, 0, run_clear_land, 0},
    {0x00474700u, "clear_land_apply", 5, 0, 0, run_clear_apply, 0},
    {0x004021D0u, "clear_confirm_monument", 0, 0, 0, run_confirm_monument, 0},
    {0x00402210u, "clear_confirm_temple", 0, 0, 0, run_confirm_temple, 0},
    {0x00402250u, "clear_confirm_fort", 0, 0, 0, run_confirm_fort, 0},
    {0x00402290u, "clear_confirm_bridge", 0, 0, 0, run_confirm_bridge, 0},
    {0x004264D0u, "popup_confirm_open", 3, 0, 0, run_popup, 0},
    {0x0047F9B0u, "bridge_figures", 1, 32, 0, run_bridge_figures, 0},
    {0x0047F810u, "bridge_remove", 2, 0, 0, run_bridge_remove, 0},
    {0x0047B480u, "clear_roads_square", 3, 0, 0, run_roads_square, 0},
    {0x004F4930u, "clear_pyramid_area", 1, 0, 0, run_pyramid, 0},
};
const int clear_ports_count = sizeof(clear_ports) / sizeof(clear_ports[0]);
