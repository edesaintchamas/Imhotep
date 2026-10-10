/* Imhotep - the problems overlay 0x1F (package cityview): the building problem flags 0x00432920, the
 * house evolution reason 0x00454190, the overlay's column 0x00435E70.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Drawing the overlay 0x1F writes the building records (state): +0x100 the problem bits, +0xC3 the
 * "has a problem" byte, and for houses +0x85 the reason the house does not evolve (0x00454190, also
 * called by 0x004F7E50). The house level tables are 0x58 bytes per level at 0x0070CDCC. */
#include "city.h"

#include "building/building_update.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

int city_building_0x459ff0(game_state *st, int32_t b)
{
    /* @bin 0x00459FF0 building types 0x4D, 0x4E, 0x4F, 0x54 .. 0x57, 0xB8, 0xBB, 0xBC, 0xBD (movsx) */
    switch (S16(st, BUILDING_ADDR(b) + BLD_TYPE)) {
    case 0x4D: case 0x4E: case 0x4F: case 0x54: case 0x55: case 0x56: case 0x57:
    case 0xB8: case 0xBB: case 0xBC: case 0xBD:
        return 1;
    default:
        return 0;
    }
}

static int32_t quarter(game_state *st, uint32_t r, int32_t v)
{
    /* @bin 0x00454239 a size-1 house (+3 = 1) needs a quarter (rounded, signed) of a positive need,
     * at least 1 */
    if (U8(st, r + BLD_SIZE) == 1 && v > 0) {
        v = (v + 2) / 4;
        if (v < 1) {
            v = 1;
        }
    }
    return v;
}

void city_house_evolve_reason(game_state *st, int32_t b)
{
    /* @bin 0x00454190 building +0x85 := why the house (level +0x12) does not evolve: the first need of
     * its level table (0x0070CDCC + 0x58 * level) it misses (1 .. 0x48), else of the next level's
     * (0x1E .. 0x4A), else 0x3D (0x40 with +0x7E = 1); 0 when +0xBE (desirability) is not above the
     * level's +0 */
    uint32_t r = BUILDING_ADDR(b);
    int32_t level = S16(st, r + BLD_SUBTYPE);
    uint32_t t = G_HOUSE_LEVELS + 0x58u * (uint32_t) level;
    int32_t des = S8(st, r + BLD_DESIRABILITY);
    int32_t evolve;
    if (des <= S32(st, t)) {
        evolve = -1;
    } else {
        evolve = des >= S32(st, t + 4);
    }
    int32_t water = S32(st, t + 0xC), food = S32(st, t + 8), ent = S32(st, t + 0x14), rel = S32(st, t + 0x10);
    int32_t edu = S32(st, t + 0x1C), health = S32(st, t + 0x20), bazaar = S32(st, t + 0x24), goods = S32(st, t + 0x28);
    int32_t q2c = quarter(st, r, S32(st, t + 0x2C)), q30 = quarter(st, r, S32(st, t + 0x30));
    int32_t q34 = quarter(st, r, S32(st, t + 0x34)), q38 = quarter(st, r, S32(st, t + 0x38));
    /* @bin 0x004542EA the foods held: the city's food types 0x00F53DDC[0x00F53DD8] mapped to the
     * house's i16 +0x54 slots (1..8 -> 0..7, 0xD -> 9, 0xF -> 8, 0x11 -> 10, 0x13 -> 11, else -1) */
    int32_t count = 0;
    int32_t n = S32(st, G_GOODS_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int32_t g = S32(st, G_GOODS_LIST + 4u * (uint32_t) i);
        if (g == 0) {
            continue;
        }
        int32_t k;
        switch (g) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: k = g - 1; break;
        case 0xD: k = 9; break;
        case 0xF: k = 8; break;
        case 0x11: k = 10; break;
        case 0x13: k = 11; break;
        default: k = -1; break;
        }
        if (S16(st, (uint32_t) ((int32_t) r + BLD_HOUSE_INVENTORY + 2 * k)) != 0) {
            count++;
        }
    }
    uint32_t reason = r + BLD_EVOLVE_STATUS;
    if (evolve == -1) {
        U8(st, reason) = 0;
        return;
    }
    /* the current level's needs */
    if (water == 1) {
        if (U8(st, r + BLD_WATER_SUPPLY) == 0 && U8(st, r + BLD_WELL_ACCESS) == 0) {
            U8(st, reason) = 1;
            return;
        }
    } else if (water == 2 && U8(st, r + BLD_WATER_SUPPLY) == 0) {
        U8(st, reason) = 2;
        return;
    }
    int32_t has_food = S8(st, r + BLD_ENTERTAINMENT);
    if (food > has_food) {
        U8(st, reason) = has_food == 0 ? 3 : food < 0xA ? 4 : food < 0x19 ? 5 : food < 0x32 ? 6 : food < 0x50 ? 7 : 8;
        return;
    }
    if (goods > count) {
        if (goods == 1) {
            U8(st, reason) = 9;
            return;
        }
        if (goods == 2) {
            U8(st, reason) = 0xA;
            return;
        }
        if (goods == 3) {
            U8(st, reason) = 0xB;
            return;
        }
    }
    if (ent > S8(st, r + BLD_EDUCATION)) {
        if (ent == 1) {
            U8(st, reason) = 0xE;
            return;
        }
        if (ent == 2) {
            if (U8(st, r + BLD_ACCESS_TEACHER) != 0) {
                U8(st, reason) = 0xF;
                return;
            }
            if (U8(st, r + BLD_ACCESS_LIBRARIAN) != 0) {
                U8(st, reason) = 0x10;
                return;
            }
        } else if (ent == 3) {
            U8(st, reason) = 0x11;
            return;
        }
    }
    int32_t has_health = U8(st, r + BLD_ACCESS_MAGISTRATE);
    if (health > has_health) {
        U8(st, reason) = 0x12;
        return;
    }
    if (q2c > S16(st, r + BLD_U66)) {
        U8(st, reason) = 0x13;
        return;
    }
    if (rel > S8(st, r + BLD_RELIGION)) {
        if (rel == 1) {
            U8(st, reason) = 0x14;
            return;
        }
        if (rel == 2) {
            U8(st, reason) = 0x15;
            return;
        }
        if (rel == 3) {
            U8(st, reason) = 0x16;
            return;
        }
    }
    int32_t has_edu = U8(st, r + BLD_ACCESS_DENTIST);
    if (edu > has_edu) {
        U8(st, reason) = 0x17;
        return;
    }
    if (bazaar > S8(st, r + BLD_HEALTH)) {
        U8(st, reason) = bazaar == 1 ? 0x18 : U8(st, r + BLD_ACCESS_PHYSICIAN) != 0 ? 0x19 : 0x1A;
        return;
    }
    if (q38 > S16(st, r + BLD_U64)) {
        U8(st, reason) = 0x1D;
        return;
    }
    if (q30 > S16(st, r + BLD_HOUSE_INVENTORY + 2u * 10u)) {
        U8(st, reason) = 0x1B;
        return;
    }
    if (q34 > S16(st, r + BLD_U6A)) {
        U8(st, reason) = 0x48;
        return;
    }
    uint32_t city = CURRENT_CITY(st) + CITY_U46A4;
    if (q34 > 1 && S32(st, city) < 2) {
        U8(st, reason) = 0x46;
        return;
    }
    if (level >= 0x13) {
        U8(st, reason) = 0x3C;
        return;
    }
    /* @bin 0x004546B7 the next level's needs */
    t += 0x58;
    food = S32(st, t + 8);
    ent = S32(st, t + 0x14);
    rel = S32(st, t + 0x10);
    water = S32(st, t + 0xC);
    goods = S32(st, t + 0x28);
    health = S32(st, t + 0x20);
    edu = S32(st, t + 0x1C);
    bazaar = S32(st, t + 0x24);
    q2c = quarter(st, r, S32(st, t + 0x2C));
    q30 = quarter(st, r, S32(st, t + 0x30));
    q34 = quarter(st, r, S32(st, t + 0x34));
    q38 = quarter(st, r, S32(st, t + 0x38));
    if (evolve == 0) {
        U8(st, reason) = S32(st, G_UEADCD4) != 0 ? 0x3E : 0x1E;
        return;
    }
    if (water == 1) {
        if (U8(st, r + BLD_WATER_SUPPLY) == 0 && U8(st, r + BLD_WELL_ACCESS) == 0) {
            U8(st, reason) = 0x1F;
            return;
        }
    } else if (water == 2 && U8(st, r + BLD_WATER_SUPPLY) == 0) {
        U8(st, reason) = 0x20;
        return;
    }
    if (food > has_food) {
        U8(st, reason) = U8(st, r + BLD_ENTERTAINMENT) == 0 ? 0x21 : food < 0xA ? 0x22 : food < 0x19 ? 0x23 : food < 0x32 ? 0x24
                       : food < 0x50 ? 0x25 : 0x26;
        return;
    }
    if (goods > count) {
        if (goods == 1) {
            U8(st, reason) = 0x27;
            return;
        }
        if (goods == 2) {
            U8(st, reason) = 0x28;
            return;
        }
        if (goods == 3) {
            U8(st, reason) = 0x29;
            return;
        }
    }
    if (ent > S8(st, r + BLD_EDUCATION)) {
        if (ent == 1) {
            U8(st, reason) = 0x2C;
            return;
        }
        if (ent == 2) {
            if (U8(st, r + BLD_ACCESS_TEACHER) != 0) {
                U8(st, reason) = 0x2D;
                return;
            }
            if (U8(st, r + BLD_ACCESS_LIBRARIAN) != 0) {
                U8(st, reason) = 0x2E;
                return;
            }
        }
    }
    if (health > has_health) {
        U8(st, reason) = 0x30;
        return;
    }
    if (q2c > S16(st, r + BLD_U66)) {
        U8(st, reason) = 0x31;
        return;
    }
    if (rel > S8(st, r + BLD_RELIGION)) {
        if (rel == 1) {
            U8(st, reason) = 0x32;
            return;
        }
        if (rel == 2) {
            U8(st, reason) = 0x33;
            return;
        }
        if (rel == 3) {
            U8(st, reason) = 0x34;
            return;
        }
    }
    if (edu > has_edu) {
        U8(st, reason) = 0x35;
        return;
    }
    if (bazaar > S8(st, r + BLD_HEALTH)) {
        U8(st, reason) = bazaar == 1 ? 0x36 : U8(st, r + BLD_ACCESS_PHYSICIAN) != 0 ? 0x37 : 0x38;
        return;
    }
    if (q38 > S16(st, r + BLD_U64)) {
        U8(st, reason) = 0x3B;
        return;
    }
    if (q30 > S16(st, r + BLD_HOUSE_INVENTORY + 2u * 10u)) {
        U8(st, reason) = 0x39;
        return;
    }
    if (q34 > S16(st, r + BLD_U6A)) {
        U8(st, reason) = 0x44;
        return;
    }
    city = CURRENT_CITY(st) + CITY_U46A4;
    if (q34 > 1 && S32(st, city) < 2) {
        U8(st, reason) = 0x4A;
        return;
    }
    U8(st, reason) = 0x3D;
    if (U8(st, r + BLD_U7E) == 1) {
        U8(st, reason) = 0x40;
    }
}

static int figure_unloading(game_state *st, int32_t f)
{
    /* @bin 0x00432AE6 the building's figure +0x2A: type 4 (+0xA), action 8 (+0x38), +0x66 set */
    if (f == 0) {
        return 0;
    }
    uint32_t a = FIGURE_ADDR(f);
    return U8(st, a + FIG_TYPE) == 4 && U8(st, a + FIG_ACTION) == 8 && U8(st, a + FIG_SERVICE_MAX) != 0;
}

void city_building_problems(game_state *st, int32_t b)
{
    /* @bin 0x00432920 building +0x100 := its problem bits (and +0xC3 := 1 when any), drawn by the
     * overlay 0x1F */
    uint32_t r = BUILDING_ADDR(b);
    if (S16(st, r + BLD_TYPE) == 0x5C) {
        U32(st, r + BLD_PROBLEMS) = 0;
        return;
    }
    if (b == 0 || S16(st, r + BLD_PREV_PART) != 0) {
        return;
    }
    U32(st, r + BLD_PROBLEMS) = 0;
    if (S16(st, r + BLD_COLLAPSE_RISK) > 0x320) {
        U32(st, r + BLD_PROBLEMS) = 1;
    }
    if (S16(st, r + BLD_FIRE_RISK) > 0x320) {
        U32(st, r + BLD_PROBLEMS) |= 2;
    }
    if (U8(st, r + BLD_HOUSE_SIZE) != 0) {
        /* @bin 0x004329A9 a house */
        if (U8(st, r + BLD_STATE) == 0) {
            goto done;
        }
        if (S16(st, r + BLD_POPULATION) == 0) {
            U32(st, r + BLD_PROBLEMS) |= 8;
        } else if (S32(st, r + BLD_UEC) != 0 && S32(st, r + BLD_UEC) < 3) {
            U32(st, r + BLD_PROBLEMS) |= 0x10;
        }
        uint8_t k = U8(st, r + BLD_NO_ROOM);
        if (k == 1) {
            U32(st, r + BLD_PROBLEMS) |= 0x20;
        } else if (k == 2) {
            U32(st, r + BLD_PROBLEMS) |= 0x2000;
        } else if (S32(st, r + BLD_DISEASE_RISK) > 0x320) {
            if (S16(st, r + BLD_POPULATION) == 0) {
                goto done;
            }
            U32(st, r + BLD_PROBLEMS) |= 4;
        }
        if (S16(st, r + BLD_POPULATION) == 0) {
            goto done;
        }
        if (S32(st, r + BLD_CRIME_RISK) > 0x320) {
            U32(st, r + BLD_PROBLEMS) |= 0x40;
        }
        city_house_evolve_reason(st, b);
        uint8_t why = U8(st, r + BLD_EVOLVE_STATUS);
        if (why <= 0x1D || why == 0x48 || why == 0x46) {
            U32(st, r + BLD_PROBLEMS) |= 0x80;
        }
        goto done;
    }
    int16_t type;
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, b) != 0 ||
        ((type = S16(st, r + BLD_TYPE)) >= 0x6A && type <= 0x6D)) {
        /* @bin 0x00432B7D */
        if (figure_unloading(st, S16(st, r + BLD_WALKER))) {
            U8(st, r + BLD_PROBLEM_OVERLAY) = 1;
        }
    } else {
        int listed = type >= 0x6E && type <= 0x72;
        if (!listed) {
            switch (type) {
            case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2: case 0xBB:
            case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2: case 0xE7: case 0xE8:
            case 0xE9:
                listed = 1;
                break;
            default:
                break;
            }
        }
        if (listed && city_building_0x459ff0(st, b) == 0) {
            /* @bin 0x00432AD5 */
            if (figure_unloading(st, S16(st, r + BLD_WALKER))) {
                U8(st, r + BLD_PROBLEM_OVERLAY) = 1;
            } else if (S16(st, r + BLD_STORED) <= 0) {
                int flag = 1;
                if (S16(st, r + BLD_TYPE) == 0x4A) {
                    uint32_t v = U16(st, r + BLD_SECOND_WALKER);
                    if ((v & 0xF000) == 0) {
                        int32_t out = b;
                        flag = building_missing_worker(st, &out) != 0;
                    } else {
                        flag = !((v & 0xF000) == 0x1000 && (v & 0xFFF) == 0x19);
                    }
                }
                if (flag) {
                    U32(st, r + BLD_PROBLEMS) |= 0x100;
                    U8(st, r + BLD_PROBLEM_OVERLAY) = 1;
                }
            }
        }
    }
    /* @bin 0x00432BB5 workers: the type's labour 0x00705EB8 (0x28 bytes per type) */
    int32_t need = S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, r + BLD_TYPE));
    if (need == 0) {
        return;
    }
    if (GAME_CALL(st, FN_BUILDING_RESOURCE_IN_STOCK, b) != 0) {
        U32(st, r + BLD_PROBLEMS) |= 0x800;
    } else if (S16(st, r + BLD_LABOUR_PERCENT) <= 0 && (uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, b) == 0) {
        U32(st, r + BLD_PROBLEMS) |= 0x1000;
    } else if (S16(st, r + BLD_WORKERS) == 0) {
        U32(st, r + BLD_PROBLEMS) |= 0x200;
    } else if (S16(st, r + BLD_WORKERS) < need) {
        U32(st, r + BLD_PROBLEMS) |= 0x400;
    }
done:
    if (U32(st, r + BLD_PROBLEMS) != 0 && U8(st, r + BLD_PROBLEM_OVERLAY) == 0) {
        U8(st, r + BLD_PROBLEM_OVERLAY) = 1;
    }
}

void city_overlay_problems_column(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00435E70 (overlay 0x1F) a building whose main part has problems: its top (0x0053F250,
     * from the corner; past 5: none), then one icon 0x00F5644C + bit (bit 12: + 9) per problem bit
     * 0 .. 13, 14 pixels apart, above the footprint (fields: the main part for 0x41 .. 0x45) */
    int32_t main = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
    if (U32(st, BUILDING_ADDR(main) + BLD_PROBLEMS) == 0) {
        return;
    }
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t corner = U8(st, G_DAY2_DESTROY_BITS + (uint32_t) tile) & 0xF;
    int32_t img = S32(st, G_IMAGE + 4u * (uint32_t) tile);
    S32(st, G_GLYPH_LAST) = img;
    int32_t px, py;
    if (cv_overlay_corner_pos(st, corner, x, y, &px, &py) >= 0) {
        cv_image_draw_top(st, img, px, py);
    }
    int32_t who = b;
    uint32_t r = BUILDING_ADDR(b);
    if (U8(st, r + BLD_HOUSE_SIZE) == 0) {
        if (S16(st, r + BLD_TYPE) == 0x47) {
            GAME_CALL(st, FN_STORAGE_DRAW, b, S32(st, G_VIEW_TILE), x, y);
        } else if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, b) != 0) {
            if (U8(st, G_EDGE + (uint32_t) S32(st, G_VIEW_TILE)) & 0x40) {
                GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_DRAW_488C20, b, S32(st, G_U70E458) + x + 2, y - S32(st, G_U70E448), 1);
            }
        } else {
            int16_t mt = S16(st, BUILDING_ADDR(main) + BLD_TYPE);
            if (mt >= 0x41 && mt <= 0x45) {
                if (corner == 0 || (U8(st, G_EDGE + (uint32_t) S32(st, G_VIEW_TILE)) & 0x40) == 0) {
                    return;
                }
                who = main;
            }
        }
    }
    int32_t ix = S32(st, G_TILE_W) * (int32_t) corner + x + 0x12;
    int32_t iy = S32(st, G_TILE_H) * (int32_t) corner + y - 2;
    uint32_t bits = BUILDING_ADDR(who) + BLD_PROBLEMS;
    for (int32_t i = 0; i < 0xE; i++) {
        int32_t k = i == 0xC ? 9 : i;
        if (U32(st, bits) & (1u << i)) {
            cv_image_draw(st, S32(st, G_IMAGE_GROUP(275)) + k, ix, iy);
            iy -= 0xE;
        }
    }
}

static int32_t run_459ff0(game_state *st, const int32_t *a) { return city_building_0x459ff0(st, a[0]); }
static int32_t run_reason(game_state *st, const int32_t *a)
{
    city_house_evolve_reason(st, a[0]);
    return 0;
}
static int32_t run_problems(game_state *st, const int32_t *a)
{
    city_building_problems(st, a[0]);
    return 0;
}
static int32_t run_column(game_state *st, const int32_t *a)
{
    city_overlay_problems_column(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function cityview_overlay_problem_ports[] = {
    {0x00459FF0u, "city_building_0x459ff0", 1, 32, 0, run_459ff0, 0},
    {0x00454190u, "city_house_evolve_reason", 1, 0, 0, run_reason, 0},
    {0x00432920u, "city_building_problems", 1, 0, 0, run_problems, 0},
    {0x00435E70u, "city_overlay_problems_column", 3, 0, 0, run_column, 0},
};
const int cityview_overlay_problem_ports_count =
    sizeof(cityview_overlay_problem_ports) / sizeof(cityview_overlay_problem_ports[0]);
