/* Imhotep - the minimap (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0043E810 draws the map at two pixels per tile in a rectangle of the sidebar, over the
 * city view's view-to-tile table 0x0070E494 (0xE7 x 0x1C9 grid offsets, built by the city view),
 * centred on the camera; then the frame of the visible area. Pixels go through the original's
 * blitters (package blit): 0x0053F4A0 (a minimap image), 0x0053E590, 0x0040FC10 (two pixels). */
#include "sidebar.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define VIEW_TILE 0x0070E494u          /* i32 grid offset per (column, row) of the view, 0xE7 per row */
#define VIEW_COLUMNS 0xE7
#define VIEW_ROWS 0x1C9
#define MINIMAP_COLUMN 0x0070E480u     /* i32 current column of the walk */
#define MINIMAP_ROW 0x0070E47Cu        /* i32 current row */
#define MINIMAP_TILE 0x0070E488u       /* i32 current grid offset */
#define MINIMAP_FIRST_COLUMN 0x0070E3DCu /* i32 */
#define MINIMAP_FIRST_ROW 0x0070E3D8u  /* i32 */
#define GRID_TERRAIN 0x00AF598Cu       /* u32 per tile */
#define GRID_BUILDING 0x00B4E6FCu      /* i16 building id per tile */
#define GRID_FIGURE 0x00B350DCu        /* i16 first figure per tile */
#define GRID_EDGE 0x00B67D1Cu          /* u8 per tile: 0x40 the building's drawing tile */
#define GRID_RANDOM 0x00ACF85Cu        /* u8 per tile */
#define GRID_SPRITE 0x00ADC36Cu        /* u8 per tile (chunk 14) */
#define GRID_SIZE_FLAGS 0x00B285CCu    /* u8 per tile: low 4 bits the building's size */
#define TINT 0x0070E120u               /* u16 colour tint of the tinted blitters */

static void draw_minimap_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, IMAGES, 0x0053F4A0u, id, x, y);
}

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, IMAGES, 0x0053E590u, id, x, y);
}

static uint8_t figure_type(game_state *st, int32_t id)
{
    return U8(st, FIGURE_ADDR(id) + 0x0A);
}

static int32_t figure_next(game_state *st, int32_t id)
{
    return S16(st, FIGURE_ADDR(id) + 0x08);
}

void minimap_first_tile(game_state *st, int32_t w, int32_t h)
{
    /* @bin 0x0043E670 the view column and row of the minimap's top left: centred on the camera
     * (0x0070E478 column, 0x0070E474 row, view size 0x0070E464 x 0x0070E460) within the map's
     * bounds (columns 0x0070E42C..0x0070E424, rows 0x0070E428..0x0070E420), or the table's
     * centre when the map is narrower than the minimap; the row is even */
    int32_t rows_left = S32(st, 0x0070E420u) - S32(st, 0x0070E428u) - h + 1;
    S32(st, MINIMAP_FIRST_COLUMN) = (VIEW_COLUMNS - w) / 2;
    int32_t centre_row = (VIEW_ROWS - h) / 2;
    int32_t columns_left = S32(st, 0x0070E424u) - S32(st, 0x0070E42Cu) - w + 1;
    if (columns_left > 0) {
        int32_t c = S32(st, 0x0070E464u) / 2 - w / 2 + S32(st, 0x0070E478u);
        S32(st, MINIMAP_FIRST_COLUMN) = c;
        int32_t lo = S32(st, 0x0070E42Cu);
        if (c < lo) {
            S32(st, MINIMAP_FIRST_COLUMN) = lo;
        } else {
            int32_t hi = S32(st, 0x0070E424u) - w;
            if (c > hi) {
                S32(st, MINIMAP_FIRST_COLUMN) = hi;
            }
        }
    }
    if (rows_left <= 0) {
        S32(st, MINIMAP_FIRST_ROW) = (int32_t) ((uint32_t) centre_row & 0xFFFEu);
        return;
    }
    int32_t r = S32(st, 0x0070E460u) / 2 - h / 2 + S32(st, 0x0070E474u);
    int32_t lo = S32(st, 0x0070E428u);
    if (r < lo) {
        S32(st, MINIMAP_FIRST_ROW) = (int32_t) ((uint32_t) lo & 0xFFFEu);
        return;
    }
    int32_t hi = S32(st, 0x0070E420u) - h;
    if (r > hi) {
        S32(st, MINIMAP_FIRST_ROW) = (int32_t) ((uint32_t) hi & 0xFFFEu);
        return;
    }
    S32(st, MINIMAP_FIRST_ROW) = (int32_t) ((uint32_t) r & 0xFFFEu);
}

int32_t minimap_figure_shown(game_state *st, int32_t id)
{
    /* @bin 0x0043E790 1 when a figure on the tile is drawn as a dot: one 0x004387D0 accepts, or
     * (no overlay) one of the types 0x16, 0x17, 0x18, 0x6A, 0x6B, 0x6D; never with overlay 0x33 */
    if (id == 0) {
        return 0;
    }
    for (;;) {
        if (S32(st, 0x00E94E0Cu) == 0x33) {
            return 0;
        }
        if (GAME_CALL(st, 0x004387D0u, id) != 0) {
            return 1;
        }
        if (S32(st, 0x00E94E0Cu) == 0) {
            uint8_t t = figure_type(st, id);
            if (t == 0x16 || t == 0x17 || t == 0x18 || t == 0x6B || t == 0x6A || t == 0x6D) {
                return 1;
            }
        }
        id = figure_next(st, id);
        if (id == 0) {
            return 0;
        }
    }
}

int32_t minimap_figure_soldier(game_state *st, int32_t id)
{
    /* @bin 0x0043EE10 over the tile's figures: types 0x0B..0x0D (soldiers) give 1 when their
     * formation (+0x60) is an enemy one (formation +0x52 < 0, records of 0x90 at 0x00BCB9F8), types
     * 0x2B..0x39, 0x63, 0x29 with +0x38 = 0x0B give 2, enemies (0x004EA9C0) 3; else 0 */
    while (id != 0) {
        uint8_t t = figure_type(st, id);
        if (t >= 0x0B && t <= 0x0D) {
            int32_t formation = S16(st, FIGURE_ADDR(id) + 0x60);
            return S16(st, 0x00BCBA4Au + 0x90u * (uint32_t) formation) < 0;
        }
        if ((t >= 0x2B && t <= 0x39) || t == 0x37 || t == 0x36 || t == 0x38 || t == 0x63 ||
            (t == 0x29 && U8(st, FIGURE_ADDR(id) + 0x38) == 0x0B)) {
            return 2;
        }
        if ((GAME_CALL(st, 0x004EA9C0u, t) & 0xFF) != 0) {
            return 3;
        }
        id = figure_next(st, id);
    }
    return 0;
}

int32_t minimap_figure_animal(game_state *st, int32_t id)
{
    /* @bin 0x0043EEB0 over the tile's figures: types 0x4E, 0x4D, 0x19, 0x4C, 0x14 give 1, types
     * 0x5D, 0x5C, 0x64, 0x65 give 2; else 0 */
    while (id != 0) {
        uint8_t t = figure_type(st, id);
        if (t == 0x4E || t == 0x4D || t == 0x19 || t == 0x4C || t == 0x14) {
            return 1;
        }
        if (t == 0x5D || t == 0x5C || t == 0x64 || t == 0x65) {
            return 2;
        }
        id = figure_next(st, id);
    }
    return 0;
}

int32_t minimap_building_colour(int32_t type, game_state *st)
{
    /* @bin 0x0043E770 the minimap colour index of a building type (table 0x005CE008, 0xED types,
     * stored + 1), -1 for none */
    if (type < 0 || type >= 0xED) {
        return -1;
    }
    int32_t c = S32(st, 0x005CE008u + 4u * (uint32_t) type);
    if (c <= 0) {
        return -1;
    }
    return c - 1;
}

/* @bin 0x00439E54 (jump table of 0x004399C2): the overlay's value function per overlay id */
static int32_t overlay_value(game_state *st, int32_t overlay, int32_t building, int32_t tile, uint32_t terrain,
                             int *handled)
{
    *handled = 1;
    switch (overlay) {
    case 0x04: return GAME_CALL(st, 0x00434400u, building);
    case 0x08: return GAME_CALL(st, 0x00433C90u, building);
    case 0x09:
        if ((terrain & 0x4140u) != 0) {
            return GAME_CALL(st, 0x00433D30u, tile);
        }
        return GAME_CALL(st, 0x00433D50u, building);
    case 0x0A: return GAME_CALL(st, 0x00433E80u, building);
    case 0x0B: return GAME_CALL(st, 0x00433ED0u, building);
    case 0x0C: return GAME_CALL(st, 0x004340E0u, building);
    case 0x0D: return GAME_CALL(st, 0x00434150u, building);
    case 0x0E: return GAME_CALL(st, 0x004341D0u, building);
    case 0x0F:
    case 0x10: return GAME_CALL(st, 0x00434250u, building);
    case 0x11: return GAME_CALL(st, 0x00434060u, building);
    case 0x12: return GAME_CALL(st, 0x00434510u, building);
    case 0x13: return GAME_CALL(st, 0x00434570u, building);
    case 0x14: return GAME_CALL(st, 0x0047D350u, building);
    case 0x15: return GAME_CALL(st, 0x004345D0u, building);
    case 0x16: return GAME_CALL(st, 0x00434630u, building);
    case 0x17: return GAME_CALL(st, 0x00434690u, building);
    case 0x18: return GAME_CALL(st, 0x004346F0u, building);
    case 0x19: return GAME_CALL(st, 0x004347B0u, building);
    case 0x1A: return GAME_CALL(st, 0x004342C0u, building);
    case 0x1B: return GAME_CALL(st, 0x00433A60u, tile);
    case 0x1C: return GAME_CALL(st, 0x004339E0u, tile);
    case 0x1D: return GAME_CALL(st, 0x00434A10u, building);
    case 0x1E: return GAME_CALL(st, 0x00433C80u, building);
    case 0x1F: return GAME_CALL(st, 0x00434820u, building);
    case 0x21: return GAME_CALL(st, 0x00433AE0u, building, 1, 0);
    case 0x22: return GAME_CALL(st, 0x00433AE0u, building, 3, 2);
    case 0x23: return GAME_CALL(st, 0x00433AE0u, building, 6, 5);
    case 0x24: return GAME_CALL(st, 0x00433AE0u, building, 2, 1);
    case 0x25: return GAME_CALL(st, 0x00433AE0u, building, 4, 3);
    case 0x26: return GAME_CALL(st, 0x00433AE0u, building, 5, 4);
    case 0x27: return GAME_CALL(st, 0x00433AE0u, building, 0x0D, 9);
    case 0x28: return GAME_CALL(st, 0x00433AE0u, building, 0x13, 0x0B);
    case 0x29: return GAME_CALL(st, 0x00433AE0u, building, 0x11, 0x0A);
    case 0x2A: return GAME_CALL(st, 0x00433AE0u, building, 0x0F, 8);
    case 0x2B: return GAME_CALL(st, 0x00433E00u, building);
    case 0x2D: return GAME_CALL(st, 0x004343A0u, building);
    case 0x30: return GAME_CALL(st, 0x00433E40u, building);
    case 0x31: return GAME_CALL(st, 0x00434A60u, building);
    case 0x32: return GAME_CALL(st, 0x00434750u, building);
    default: *handled = 0; return 0;
    }
}

int32_t minimap_draw_building(game_state *st, int32_t building, int32_t x, int32_t y)
{
    /* @bin 0x00439980 a building's tile on the minimap (the tile 0x0070E488 being its drawing
     * tile, edge bit 0x40, with terrain bit 8): its colour by type, or with an overlay the
     * overlay's level (images 0x00F568C4 + 5 min(level, 10)); granaries' and the like's tiles
     * (types 0x1E, 0x1F, 0x21, 0xD1, terrain bit 0x40) in 0x00F568B4; drawn by the building's
     * size (tile flags 0x00B285CC & 15: 1..4 a k x k image raised k, 5 four 3x3 images, more:
     * nothing). Returns 1 when drawn (0: not a building's tile, or a type with no colour) */
    int32_t tile = S32(st, MINIMAP_TILE);
    if ((U8(st, GRID_EDGE + (uint32_t) tile) & 0x40) == 0) {
        return 0;
    }
    uint32_t terrain = U32(st, GRID_TERRAIN + 4u * (uint32_t) tile);
    if ((terrain & 8) == 0) {
        return 0;
    }
    int32_t overlay = S32(st, 0x00E94E0Cu);
    int handled;
    int32_t image = overlay_value(st, overlay, building, tile, terrain, &handled);
    if (!handled) {
        uint32_t b = BUILDING_ADDR(building);
        if (U8(st, b + 5) != 0) {
            image = S32(st, 0x00F568BCu);
        } else if (U16(st, b + 0x10) == 0x5A) {
            image = S32(st, 0x00F568D4u) - 1;
        } else {
            int32_t c = minimap_building_colour(S16(st, b + 0x10), st);
            if (c < 0) {
                return 0;
            }
            image = S32(st, 0x00F56B14u) + c * 5;
        }
        /* @bin 0x00439C77 tint 0xF863 (red) for a building with +0x4A set */
        U16(st, TINT) = U16(st, b + 0x4A) != 0 ? 0xF863u : 0;
    }
    overlay = S32(st, 0x00E94E0Cu);
    if (overlay != 0 && overlay != 0x33) {
        if (image > 10) {
            image = 10;
        }
        image = S32(st, 0x00F568C4u) + image * 5;
    }
    int16_t type = S16(st, BUILDING_ADDR(building) + 0x10);
    if ((type == 0x1F || type == 0x1E || type == 0x21 || type == 0xD1) &&
        (U8(st, GRID_TERRAIN + 4u * (uint32_t) S32(st, MINIMAP_TILE)) & 0x40) != 0) {
        image = S32(st, 0x00F568B4u);
    }
    uint32_t size = U8(st, GRID_SIZE_FLAGS + (uint32_t) S32(st, MINIMAP_TILE)) & 0x0Fu;
    if (size == 0) {
        draw_minimap_image(st, image, x, y);
    } else if (size >= 1 && size <= 4) {
        draw_image(st, image + (int32_t) size, x, y - (int32_t) size);
    } else if (size == 5) {
        draw_image(st, image + 2, x + 3, y - 5);
        draw_image(st, image + 2, x + 3, y + 1);
        draw_image(st, image + 2, x, y - 2);
        draw_image(st, image + 2, x + 6, y - 2);
    }
    U16(st, TINT) = 0;
    return 1;
}

void minimap_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x0043E810 the minimap in (x, y, 2 w, h): clip to it, walk h + 8 rows and w + 4
     * columns of the view table from the first tile (0x0043E670) minus 4, a row's tiles 2 pixels
     * apart, odd rows shifted by one pixel. Per tile, in order: off the map (offset >= 0x0FFF0000,
     * terrain bit 0x80000, or outside the scenario's area 0x00E94DD0..0x00E94DCC x
     * 0x00E94DC8..0x00E94DC4 unless 0x00775680) the "outside" image 0x00F568DC; a figure shown
     * (0x0043E790, soldiers and animals) a 2-pixel dot (soldiers: the highlight colour, enemies
     * black); a building (0x00439980) or the terrain's image by its flags. Tiles out of the
     * rectangle are skipped, except the editor's overlay 0x31 marks. Then the frame of the city
     * view's area in 0xE70B, and the clip back to the screen */
    int32_t pair, colour;
    uint8_t mode = U8(st, 0x00784CB0u);
    if (S32(st, 0x00E92DFCu) == 2) {
        pair = (int32_t) 0xF000F000u;
        colour = mode == 1 ? 0x7800 : (mode != 0 ? (int32_t) (0xFFFFEFF0u + 0x181Fu) : 0x181F);
    } else {
        pair = 0x78007800;
        colour = mode == 0 ? 0x3C00 : ((uint8_t) (mode - 1) != 0 ? (int32_t) (0xFFFFF800u + 0xC1Fu) : 0xC1F);
    }
    int32_t highlight = (int32_t) (((uint32_t) colour << 16) + (uint32_t) colour);
    int32_t right = x + w * 2;
    int32_t odd = 0;
    S32(st, 0x00D3B55Cu) = x;
    S32(st, 0x00D3B558u) = right;
    S32(st, 0x00D3B554u) = y;
    S32(st, 0x00D3B550u) = h + y;
    minimap_first_tile(st, w, h);
    S32(st, 0x0070E3C8u) = h + y;
    S32(st, 0x0070E3CCu) = right;
    S32(st, 0x0070E3D4u) = x;
    S32(st, 0x0070E3D0u) = y;
    int32_t py = y - 4;
    S32(st, MINIMAP_ROW) = S32(st, MINIMAP_FIRST_ROW) - 4;
    if (h + 4 > -4) {
        for (int32_t row = py - y; row < h + 4; row++, py++) {
            int32_t px;
            if (odd == 0) {
                px = x - 8;
                odd = 1;
            } else {
                px = x - 9;
                odd = 0;
            }
            S32(st, MINIMAP_COLUMN) = S32(st, MINIMAP_FIRST_COLUMN) - 4;
            if (w > -4) {
                for (int32_t col = -4; col < w; col++, px += 2) {
                    int32_t tx = S32(st, MINIMAP_COLUMN);
                    int32_t ty = S32(st, MINIMAP_ROW);
                    if (tx >= 0 && tx < VIEW_COLUMNS && ty >= 0 && ty < VIEW_ROWS) {
                        int32_t out = 0;
                        if (row < 0 || row >= h) {
                            out = 1;
                        }
                        if (col < 0 || col >= w) {
                            out = 1;
                        }
                        int32_t tile = S32(st, VIEW_TILE + 4u * (uint32_t) (ty * VIEW_COLUMNS + tx));
                        S32(st, MINIMAP_TILE) = tile;
                        uint32_t terrain;
                        if (tile >= 0x0FFF0000) {
                            goto outside;
                        }
                        terrain = U32(st, GRID_TERRAIN + 4u * (uint32_t) tile);
                        if ((terrain & 0x80000u) != 0) {
                            goto outside;
                        }
                        if (U8(st, 0x00775680u) == 0 &&
                            !(tx >= S32(st, 0x00E94DD0u) && ty >= S32(st, 0x00E94DC8u) && tx <= S32(st, 0x00E94DCCu) &&
                              ty <= (int32_t) ((uint32_t) (S32(st, 0x00E94DC4u) - 1) & 0xFFFEu))) {
                            goto outside;
                        }
                        {
                            int32_t building = S16(st, GRID_BUILDING + 2u * (uint32_t) tile);
                            uint32_t has_building = terrain & 8;
                            int32_t figure = S16(st, GRID_FIGURE + 2u * (uint32_t) tile);
                            int32_t dot = pair;
                            int32_t soldier = minimap_figure_soldier(st, figure);
                            int32_t animal = minimap_figure_animal(st, figure);
                            if (soldier != 0) {
                                if (soldier == 2) {
                                    dot = highlight;
                                } else if (soldier == 3) {
                                    dot = 0;
                                }
                            } else if (animal != 0 && animal == 2) {
                                dot = highlight;
                            }
                            if (minimap_figure_shown(st, figure) != 0 || animal != 0 || soldier != 0) {
                                S32(st, 0x00D3B5E0u) = px;
                                S32(st, 0x00D3B5DCu) = py;
                                if (out == 0) {
                                    GAME_CALL(st, 0x0040FC10u, dot);
                                }
                                goto next;
                            }
                            if (has_building != 0 && building != 0) {
                                if ((U8(st, GRID_EDGE + (uint32_t) S32(st, MINIMAP_TILE)) & 0x40) == 0 || out != 0) {
                                    goto next;
                                }
                                if (minimap_draw_building(st, building, px, py) != 0) {
                                    goto next;
                                }
                                draw_minimap_image(st, S32(st, 0x00F56B14u) + ((terrain & 0x10000000u) ? 0x37 : 0x3C), px, py);
                                goto next;
                            }
                            if ((terrain & 0xC04000u) != 0 && S32(st, 0x00E94E0Cu) == 0x31) {
                                draw_minimap_image(st, S32(st, 0x00F568C4u) + 5, px, py);
                                goto next;
                            }
                            if (out != 0) {
                                goto next;
                            }
                            uint32_t v = U8(st, GRID_RANDOM + (uint32_t) S32(st, MINIMAP_TILE));
                            int32_t image;
                            if ((terrain & 0x40000044u) != 0 && U8(st, GRID_SPRITE + (uint32_t) S32(st, MINIMAP_TILE)) != 0) {
                                image = S32(st, 0x00F568B4u);
                            } else if (terrain & 0x10000000u) {
                                image = S32(st, 0x00F56B14u) + 0x37;
                            } else if (terrain & 0x20u) {
                                image = S32(st, 0x00F56B14u) + 0x32;
                            } else if (terrain & 4u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F5688Cu);
                            } else if (terrain & 0x10u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F56894u);
                            } else if (terrain & 1u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F56894u);
                            } else if (terrain & 2u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F568A4u);
                            } else if (terrain & 0x2000000u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F56AB4u);
                            } else if (terrain & 0x200u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F568A4u);
                            } else if (terrain & 0x40000040u) {
                                image = S32(st, 0x00F568B4u);
                            } else if (terrain & 0x100u) {
                                image = S32(st, 0x00F568D4u);
                            } else if (terrain & 0x4000u) {
                                image = S32(st, 0x00F56B14u) + 0x3C;
                            } else if (terrain & 0x800u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F568ACu);
                            } else if (terrain & 0x10000u) {
                                image = (int32_t) (v & 3) + S32(st, 0x00F568ACu);
                            } else if (terrain & 0x40000u) {
                                image = (int32_t) (v & 4) + S32(st, 0x00F56894u);
                            } else {
                                image = (int32_t) (v & 7) + S32(st, 0x00F56884u);
                            }
                            draw_minimap_image(st, image, px, py);
                            goto next;
                        }
                    outside:
                        if (out == 0) {
                            draw_minimap_image(st, S32(st, 0x00F568DCu), px, py);
                        }
                    }
                next:
                    S32(st, MINIMAP_COLUMN) = S32(st, MINIMAP_COLUMN) + 1;
                }
            }
            S32(st, MINIMAP_ROW) = S32(st, MINIMAP_ROW) + 1;
        }
    }
    int32_t fy = S32(st, 0x0070E474u) - S32(st, MINIMAP_FIRST_ROW) + y;
    int32_t fx = x + (S32(st, 0x0070E478u) - S32(st, MINIMAP_FIRST_COLUMN)) * 2;
    int32_t fw = S32(st, 0x0070E464u) * 2;
    int32_t fh = S32(st, 0x0070E460u);
    if (S32(st, 0x006FDC50u) != 0 && S32(st, EDITOR_MODE) == 1) {
        fw = S32(st, 0x0070E464u) * 2 - 4;
        fh -= 6;
    }
    if (fx < x) {
        fx = x;
    }
    GAME_CALL(st, 0x004CD680u, fx, fy, fw, fh, 0xE70B);
    S32(st, 0x00D3B558u) = S32(st, SCREEN_WIDTH);
    S32(st, 0x00D3B55Cu) = 0;
    S32(st, 0x00D3B554u) = 0;
    S32(st, 0x00D3B550u) = S32(st, SCREEN_HEIGHT);
}

/* ---- port table ---- */

static int32_t run_draw(game_state *st, const int32_t *a) { minimap_draw(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_first(game_state *st, const int32_t *a) { minimap_first_tile(st, a[0], a[1]); return 0; }
static int32_t run_shown(game_state *st, const int32_t *a) { return minimap_figure_shown(st, a[0]); }
static int32_t run_soldier(game_state *st, const int32_t *a) { return minimap_figure_soldier(st, a[0]); }
static int32_t run_animal(game_state *st, const int32_t *a) { return minimap_figure_animal(st, a[0]); }
static int32_t run_colour(game_state *st, const int32_t *a) { return minimap_building_colour(a[0], st); }
static int32_t run_building(game_state *st, const int32_t *a) { return minimap_draw_building(st, a[0], a[1], a[2]); }

const ported_function minimap_ports[] = {
    {0x0043E810u, "minimap_draw", 4, 0, 0, run_draw, 0},
    {0x0043E670u, "minimap_first_tile", 2, 0, 0, run_first, 0},
    {0x0043E790u, "minimap_figure_shown", 1, 32, 0, run_shown, 0},
    {0x0043EE10u, "minimap_figure_soldier", 1, 32, 0, run_soldier, 0},
    {0x0043EEB0u, "minimap_figure_animal", 1, 32, 0, run_animal, 0},
    {0x0043E770u, "minimap_building_colour", 1, 32, 0, run_colour, 0},
    {0x00439980u, "minimap_draw_building", 3, 32, 0, run_building, 0},
};
const int minimap_ports_count = sizeof(minimap_ports) / sizeof(minimap_ports[0]);
