/* Imhotep - monuments' footprints drawn again (package rest): the redraw entries of the monument
 * objects (thiscall, called by buildings_redraw 0x0046D840 on objects 0x006FD5D8, 0x006FD368,
 * 0x006DE650, 0x006DE548, 0x006F9320, 0x006F5648, 0x006DE9E0, 0x006E9188) and their image
 * functions (vtable +0x1C), which return an image and write flags into their caller's stack.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/rest_monument.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


#define G32(a) S32(st, (a))

/* @bin 0x0053DD90 (thiscall on 0x00F55BB0) a graphics file into an image slot */
static void load_images(game_state *st, uint32_t name, int32_t slot)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_GRAPHICS_LOAD_SLOT, (int32_t) name, slot);
}

static int32_t part_entry(game_state *st, uint32_t self, int32_t b, uint32_t off)
{
    /* the object's part table: 0x34 bytes a part from +0x54, by the building's part +0x12 */
    return (int32_t) P32(st, self + off + 0x34u * (uint32_t) (int32_t) S16(st, BUILDING_ADDR(b) + BLD_PART));
}

static int32_t half_orient(game_state *st)
{
    return G32(G_VIEW_ORIENTATION) / 2;
}

/* ---- image functions (vtable +0x1C) ------------------------------------------------------- */

int32_t monument_image_401800(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x00401800 (vtable 0x0056F438 +0x1C) by the part's kind (+0x58 of the part table) and
     * the view (0x00BCA2E0 / 2): an image of 0x00F57514..0x00F5757C */
    uint32_t r = BUILDING_ADDR(b);
    int32_t kind = part_entry(st, self, b, MONOBJ_PART_KIND);
    int32_t h = half_orient(st);
    int corner = h == 0 || h == 3;
    *out = 0;
    load_images(st, G_E6_FILE, 0x1A);
    /* @bin 0x00401A78.. per-kind tables by the view */
    static const uint32_t k0[4] = {G_IMAGE_GROUPS + 8 * 822, G_IMAGE_GROUPS + 8 * 824, G_IMAGE_GROUPS + 8 * 825, G_IMAGE_GROUPS + 8 * 823};
    static const uint32_t k1[4] = {G_IMAGE_GROUPS + 8 * 824, G_IMAGE_GROUPS + 8 * 825, G_IMAGE_GROUPS + 8 * 823, G_IMAGE_GROUPS + 8 * 822};
    static const uint32_t k2[4] = {G_IMAGE_GROUPS + 8 * 823, G_IMAGE_GROUPS + 8 * 822, G_IMAGE_GROUPS + 8 * 824, G_IMAGE_GROUPS + 8 * 825};
    static const uint32_t k3[4] = {G_IMAGE_GROUPS + 8 * 819, G_IMAGE_GROUPS + 8 * 818, G_IMAGE_GROUPS + 8 * 821, G_IMAGE_GROUPS + 8 * 820};
    static const uint32_t k4[4] = {G_IMAGE_GROUPS + 8 * 820, G_IMAGE_GROUPS + 8 * 819, G_IMAGE_GROUPS + 8 * 818, G_IMAGE_GROUPS + 8 * 821};
    static const uint32_t k5[4] = {G_IMAGE_GROUPS + 8 * 818, G_IMAGE_GROUPS + 8 * 821, G_IMAGE_GROUPS + 8 * 820, G_IMAGE_GROUPS + 8 * 819};
    static const uint32_t k6[4] = {G_IMAGE_GROUPS + 8 * 821, G_IMAGE_GROUPS + 8 * 820, G_IMAGE_GROUPS + 8 * 819, G_IMAGE_GROUPS + 8 * 818};
    static const uint32_t *const kt[7] = {k0, k1, k2, k3, k4, k5, k6};
    switch ((uint32_t) kind) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        if ((uint32_t) h > 3) {
            return 0;
        }
        return G32(kt[kind][h]);
    case 7:
        if (corner) {
            return G32(G_IMAGE_GROUPS + 8 * 815) + (h & 1) + S32(st, r + BLD_MONUMENT_STAGE) * 2;
        }
        return G32(G_IMAGE_GROUPS + 8 * 814) + (~h & 1);
    case 8:
        if (corner) {
            return G32(G_IMAGE_GROUPS + 8 * 816) + (h & 1) + S32(st, r + BLD_MONUMENT_STAGE) * 2;
        }
        return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 816) + 0xE;
    case 9: {
        int32_t v = S32(st, r + BLD_MONUMENT_STAGE);
        if (v > 0) {
            return G32(G_IMAGE_GROUPS + 8 * 817) + (h & 1) + v * 2;
        }
        return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 817);
    }
    case 10:
    case 11:
        if (corner) {
            return G32(G_IMAGE_GROUPS + 8 * 813) + (h & 1) + S32(st, r + BLD_MONUMENT_STAGE) * 2;
        }
        return G32(G_IMAGE_GROUPS + 8 * 812) + (~h & 1);
    default:
        return 0;
    }
}

int32_t monument_image_417310(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x00417310 (vtable 0x0056F4DC +0x1C) the part +0x12 (3 x 3 parts) through a table of
     * the view, plus 0x00F57394; loads the images named by the object's +0x220 table */
    static const int32_t order[36] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 2, 5, 8, 1, 4, 7, 0, 3, 6,
                                      8, 7, 6, 5, 4, 3, 2, 1, 0, 6, 3, 0, 7, 4, 1, 8, 5, 2};
    uint32_t r = BUILDING_ADDR(b);
    int32_t idx = half_orient(st) * 9 + S16(st, r + BLD_PART);
    if (idx < 0 || idx >= 36) {
        state_unported(st, FN_MONUMENT_IMAGE_417310); /* the original reads its stack outside the table */
        return 0;
    }
    int32_t v = order[idx];
    *out = 0;
    uint32_t main = BUILDING_ADDR(b - S16(st, r + BLD_PART));
    load_images(st, P32(st, self + E3OBJ_FILES + 4u * U32(st, main + BLD_MONUMENT_STAGE)), 0x12);
    return v + G32(G_IMAGE_GROUPS + 8 * 764);
}

int32_t monument_image_4e6a70(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x004E6A70 (vtable 0x0056F6E8 +0x1C) kind 0: images of the object's +0xB4 table, image
     * 0x00F5739C; kind 1: image 0x00F55BB4 (out not written); else 0 */
    int32_t kind = part_entry(st, self, b, MONOBJ_PART_KIND);
    if (kind == 0) {
        *out = 0;
        load_images(st, P32(st, self + E4OBJ_FILES + 4u * U32(st, BUILDING_ADDR(b) + BLD_MONUMENT_STAGE)), 0x13);
        return G32(G_IMAGE_GROUPS + 8 * 765);
    }
    if (kind == 1) {
        return G32(G_IMAGE_GROUPS);
    }
    return 0;
}


int32_t monument_image_40de80(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x0040DE80 (vtable 0x0056F48C +0x1C, the library-like monument 0x006FD5D8): by the
     * part's kind (+0x58 of the part table, 0..0x18) and the stage +0x58 (the main part's when
     * above 2), a graphics file is loaded (0x0053DD90) and its image returned; flags: 1 plain
     * ground, 7 the stage-0 image 0x00F573BC */
    uint32_t r = BUILDING_ADDR(b);
    int32_t v = S32(st, r + BLD_MONUMENT_STAGE);
    if (v >= 3) {
        v = S32(st, BUILDING_ADDR(b - S16(st, r + BLD_PART)) + BLD_MONUMENT_STAGE);
    }
    int32_t kind = part_entry(st, self, b, MONOBJ_PART_KIND);
    int32_t h = half_orient(st);
    uint32_t name;
    *out = 0;
    switch ((uint32_t) kind) {
    case 0:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v > 4) {
            if (v < 8) {
                name = G_LIB_KIND0_1;
            } else if (v < 0xB) {
                name = G_LIB_KIND0_2;
            } else if (v < 0xE) {
                name = G_LIB_KIND0_3;
            } else if (v < 0x12) {
                name = G_LIB_KIND0_4;
            } else {
                name = G_LIB_KIND0_5;
            }
            load_images(st, name, 0x15);
            return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 767);
        }
        break;
    case 1:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 4) {
            break;
        }
        if (v < 6) {
            if (h > 1) {
                goto plain;
            }
            name = G_LIB_MAIN_A1;
            goto main_a;
        }
        if (v < 7) {
            if (h < 2) {
                goto lab_dfa5;
            }
        } else {
            if (v > 8) {
                if (v < 10) {
                    if (h < 2) {
                        goto lab_dfd9;
                    }
                } else {
                    if (v > 0xB) {
                        if (v < 0xD) {
                            if (h < 2) {
                                goto lab_e007;
                            }
                        } else {
                            if (v > 0xE) {
                                if (v > 0x10) {
                                    if (v < 0x13) {
                                        if (h > 1) {
                                            goto lab_e044;
                                        }
                                    } else if (h > 1) {
                                    lab_e181:
                                        name = G_LIB_MAIN_C5;
                                        goto main_c;
                                    }
                                lab_e059:
                                    name = G_LIB_MAIN_A5;
                                    goto main_a;
                                }
                                if (h < 2) {
                                    goto lab_e031;
                                }
                            lab_e044:
                                name = G_LIB_MAIN_C4;
                                goto main_c;
                            }
                            if (h < 2) {
                                name = G_LIB_MAIN_A4;
                                goto main_a;
                            }
                        }
                    lab_e141:
                        name = G_LIB_MAIN_C3;
                        goto main_c;
                    }
                    if (h < 2) {
                        name = G_LIB_MAIN_A3;
                        goto main_a;
                    }
                }
            lab_e113:
                name = G_LIB_MAIN_C2;
                goto main_c;
            }
            if (h < 2) {
                name = G_LIB_MAIN_A2;
                goto main_a;
            }
        }
    lab_e0df:
        name = G_LIB_MAIN_C1;
    main_c:
        load_images(st, name, 0x16);
        return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 768);
    main_a:
        load_images(st, name, 0x14);
        return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 766);
    case 2:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 4) {
            break;
        }
        if (v < 6) {
            if (h < 2) {
                *out = 1;
                return G32(G_PLAIN);
            }
        } else {
            if (v > 6) {
                if (v < 9) {
                    if (h < 2) {
                        goto lab_e0df;
                    }
                } else {
                    if (v > 9) {
                        if (v < 0xC) {
                            if (h < 2) {
                                goto lab_e113;
                            }
                        } else {
                            if (v > 0xC) {
                                if (v < 0xF) {
                                    if (h < 2) {
                                        goto lab_e141;
                                    }
                                } else {
                                    if (v > 0x10) {
                                        if (v < 0x13) {
                                            if (h < 2) {
                                                name = G_LIB_MAIN_C4;
                                                goto main_c;
                                            }
                                        } else if (h < 2) {
                                            goto lab_e181;
                                        }
                                        goto lab_e059;
                                    }
                                    if (h < 2) {
                                        name = G_LIB_MAIN_C4;
                                        goto main_c;
                                    }
                                }
                            lab_e031:
                                name = G_LIB_MAIN_A4;
                                goto main_a;
                            }
                            if (h < 2) {
                                name = G_LIB_MAIN_C3;
                                goto main_c;
                            }
                        }
                    lab_e007:
                        name = G_LIB_MAIN_A3;
                        goto main_a;
                    }
                    if (h < 2) {
                        name = G_LIB_MAIN_C2;
                        goto main_c;
                    }
                }
            lab_dfd9:
                name = G_LIB_MAIN_A2;
                goto main_a;
            }
            if (h < 2) {
                name = G_LIB_MAIN_C1;
                goto main_c;
            }
        }
    lab_dfa5:
        name = G_LIB_MAIN_A1;
        goto main_a;
    case 3:
    case 4: {
        uint32_t last = kind == 3 ? G_IMAGE_GROUPS + 8 * 783 : G_IMAGE_GROUPS + 8 * 782;
        if (v < 2) {
            *out = 7;
            return G32(G_IMAGE_GROUPS + 8 * 769);
        }
        if (v < 0x10) {
            break;
        }
        if (v < 0x14) {
            name = G_LIB_EXTRA;
        } else {
            name = G_LIB_EXTRA2;
            if (v > 0x14) {
                load_images(st, G_LIB_EXTRA2, 0x17);
                if (h < 2) {
                    return G32(G_IMAGE_GROUPS + 8 * 770);
                }
                return (h & 1) + G32(last);
            }
        }
        load_images(st, name, 0x17);
        if (h < 2) {
            *out = 1;
            return G32(G_PLAIN);
        }
        return (h & 1) + G32(last);
    }
    case 5:
    case 9:
    case 0xC: {
        uint32_t last = kind == 5 ? G_IMAGE_GROUPS + 8 * 779 : kind == 9 ? G_IMAGE_GROUPS + 8 * 777 : G_IMAGE_GROUPS + 8 * 776;
        uint32_t top = kind == 5 ? G_IMAGE_GROUPS + 8 * 771 : G_IMAGE_GROUPS + 8 * 774;
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 0x10) {
            *out = 1;
            return G32(G_PLAIN);
        }
        if (v < 0x14) {
            name = G_LIB_EXTRA;
        } else {
            name = G_LIB_EXTRA2;
            if (v > 0x14) {
                load_images(st, G_LIB_EXTRA2, 0x17);
                if (h < 2) {
                    return G32(top);
                }
                return (h & 1) + G32(last);
            }
        }
        load_images(st, name, 0x17);
        if (kind == 5) {
            if (h > 1) {
                return (h & 1) + G32(last);
            }
            break;
        }
        if (h < 2) {
            *out = 1;
            return G32(G_PLAIN);
        }
        return (h & 1) + G32(last);
    }
    case 6:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v > 0xF) {
            if (v < 0x14) {
                load_images(st, G_LIB_EXTRA, 0x17);
                if (h < 2) {
                    *out = 1;
                    return G32(G_PLAIN);
                }
            } else if (v < 0x15) {
                load_images(st, G_LIB_EXTRA2, 0x17);
                if (h < 2) {
                    *out = 1;
                    return G32(G_PLAIN);
                }
            } else {
                load_images(st, G_LIB_EXTRA2, 0x17);
                if (h < 2) {
                    return G32(G_IMAGE_GROUPS + 8 * 771);
                }
            }
            return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 778);
        }
        break;
    case 7:
    case 8: {
        uint32_t last = kind == 7 ? G_IMAGE_GROUPS + 8 * 785 : G_IMAGE_GROUPS + 8 * 784;
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 0x10) {
            *out = 1;
            return G32(G_PLAIN);
        }
        if (v > 0x13) {
            load_images(st, G_LIB_EXTRA2, 0x17);
            if (h < 2) {
                return G32(G_IMAGE_GROUPS + 8 * 773);
            }
            return (h & 1) + G32(last);
        }
        load_images(st, G_LIB_EXTRA, 0x17);
        if (h < 2) {
            *out = 1;
            return G32(G_PLAIN);
        }
        return (h & 1) + G32(last);
    }
    case 10:
    case 0xB:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 0x15) {
            *out = 1;
            return G32(G_PLAIN);
        }
        if (h > 1) {
            return G32(G_IMAGE_GROUPS + 8 * 773);
        }
        return G32(G_IMAGE_GROUPS + 8 * 775);
    case 0xD:
    case 0xE:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v > 0xF) {
            load_images(st, v < 0x14 ? G_LIB_EXTRA : G_LIB_EXTRA2, 0x17);
            if ((kind == 0xD) == (h > 1)) {
                return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 781);
            }
            return (h & 1) + G32(G_IMAGE_GROUPS + 8 * 780);
        }
        break;
    case 0xF:
    case 0x10:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16: {
        /* @bin 0x0040E6xx.. the image above stage 0x14 (view > 1) and the turned ones */
        uint32_t top, low;
        switch (kind) {
        case 0xF:
            top = G_IMAGE_GROUPS + 8 * 774, low = G_IMAGE_GROUPS + 8 * 776;
            break;
        case 0x10:
            top = G_IMAGE_GROUPS + 8 * 774, low = G_IMAGE_GROUPS + 8 * 777;
            break;
        case 0x13:
            top = G_IMAGE_GROUPS + 8 * 771, low = G_IMAGE_GROUPS + 8 * 778;
            break;
        case 0x14:
            top = G_IMAGE_GROUPS + 8 * 771, low = G_IMAGE_GROUPS + 8 * 779;
            break;
        case 0x15:
            top = G_IMAGE_GROUPS + 8 * 770, low = G_IMAGE_GROUPS + 8 * 782;
            break;
        default:
            top = G_IMAGE_GROUPS + 8 * 770, low = G_IMAGE_GROUPS + 8 * 783;
            break;
        }
        if (v < 2) {
            if (kind >= 0x15) {
                *out = 7;
                return G32(G_IMAGE_GROUPS + 8 * 769);
            }
            *out = 1;
            return 0;
        }
        if (v > 0xF) {
            if (v < 0x14) {
                name = G_LIB_EXTRA;
            } else {
                name = G_LIB_EXTRA2;
                if (v > 0x14) {
                    load_images(st, G_LIB_EXTRA2, 0x17);
                    if (h > 1) {
                        return G32(top);
                    }
                    return (h & 1) + G32(low);
                }
            }
            load_images(st, name, 0x17);
            if (h < 2) {
                return (h & 1) + G32(low);
            }
            goto plain;
        }
        break;
    }
    case 0x11:
    case 0x12:
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v < 0x15) {
            *out = 1;
            return G32(G_PLAIN);
        }
        if (h < 2) {
            return G32(G_IMAGE_GROUPS + 8 * 773);
        }
        return G32(G_IMAGE_GROUPS + 8 * 775);
    case 0x17:
    case 0x18: {
        uint32_t low = kind == 0x17 ? G_IMAGE_GROUPS + 8 * 784 : G_IMAGE_GROUPS + 8 * 785;
        if (v < 2) {
            *out = 1;
            return 0;
        }
        if (v > 0xF) {
            if (v > 0x13) {
                load_images(st, G_LIB_EXTRA2, 0x17);
                if (h > 1) {
                    return G32(G_IMAGE_GROUPS + 8 * 773);
                }
                return (h & 1) + G32(low);
            }
            load_images(st, G_LIB_EXTRA, 0x17);
            if (kind == 0x17 ? h < 2 : h <= 1) {
                return (h & 1) + G32(low);
            }
        }
        goto plain;
    }
    default:
        return 0;
    }
plain:
    *out = 1;
    return G32(G_PLAIN);
}

/* The four pyramid-like monuments (0x006F9320, 0x006F5648, 0x006DE9E0, 0x006E9188): the same
 * function with their own image table. Entries: [type] = {image when the object's +0x54 is set,
 * image otherwise (plus the stage offset)}; 0: no image. */
typedef struct {
    uint32_t addr;     /* the image function */
    uint32_t progress; /* its parts-progress helper (0x00546790 with its own index) */
    int same_part;     /* the stage offset is 0 for the part of the object's +0x50 */
    uint32_t pick[0x1E][2];
    uint32_t fixed11;  /* type 0x11: this image (plus the stage offset if add11) */
    int add11;
    uint32_t fixed12, fixed1b, fixed1c; /* types 0x12, 0x1B, 0x1C: image + rotation */
} pyramid_images;

#define P(a, b) {a, b}
static const pyramid_images pyramid_tables[4] = {
    {FN_MONUMENT_IMAGE_545760, FN_MONUMENT_PROGRESS_PARTS_9, 0,
     {[0] = P(G_IMAGE_GROUPS + 8 * 792, G_IMAGE_GROUPS + 8 * 791), [1] = P(G_IMAGE_GROUPS + 8 * 794, G_IMAGE_GROUPS + 8 * 793), [2] = P(G_IMAGE_GROUPS + 8 * 796, G_IMAGE_GROUPS + 8 * 795),
      [3] = P(G_IMAGE_GROUPS + 8 * 798, G_IMAGE_GROUPS + 8 * 797), [6] = P(G_IMAGE_GROUPS + 8 * 800, G_IMAGE_GROUPS + 8 * 799), [0xF] = P(G_IMAGE_GROUPS + 8 * 802, G_IMAGE_GROUPS + 8 * 801),
      [0x10] = P(G_IMAGE_GROUPS + 8 * 804, G_IMAGE_GROUPS + 8 * 803), [0x1D] = P(G_IMAGE_GROUPS + 8 * 811, G_IMAGE_GROUPS + 8 * 810)},
     G_IMAGE_GROUPS + 8 * 805, 1, G_IMAGE_GROUPS + 8 * 807, G_IMAGE_GROUPS + 8 * 808, G_IMAGE_GROUPS + 8 * 809},
    {FN_MONUMENT_IMAGE_558C00, FN_MONUMENT_PROGRESS_PARTS_10, 1,
     {[0] = P(G_IMAGE_GROUPS + 8 * 829, G_IMAGE_GROUPS + 8 * 828), [1] = P(G_IMAGE_GROUPS + 8 * 831, G_IMAGE_GROUPS + 8 * 830), [2] = P(G_IMAGE_GROUPS + 8 * 833, G_IMAGE_GROUPS + 8 * 832),
      [9] = P(G_IMAGE_GROUPS + 8 * 835, G_IMAGE_GROUPS + 8 * 834), [10] = P(G_IMAGE_GROUPS + 8 * 837, G_IMAGE_GROUPS + 8 * 836), [0x13] = P(G_IMAGE_GROUPS + 8 * 839, G_IMAGE_GROUPS + 8 * 838),
      [0x14] = P(G_IMAGE_GROUPS + 8 * 841, G_IMAGE_GROUPS + 8 * 840), [0x15] = P(G_IMAGE_GROUPS + 8 * 843, G_IMAGE_GROUPS + 8 * 842), [0x1D] = P(G_IMAGE_GROUPS + 8 * 850, G_IMAGE_GROUPS + 8 * 849)},
     G_IMAGE_GROUPS + 8 * 844, 0, G_IMAGE_GROUPS + 8 * 846, G_IMAGE_GROUPS + 8 * 847, G_IMAGE_GROUPS + 8 * 848},
    {FN_MONUMENT_IMAGE_555EA0, FN_MONUMENT_PROGRESS_PARTS_21, 1,
     {[0] = P(G_IMAGE_GROUPS + 8 * 852, G_IMAGE_GROUPS + 8 * 851), [1] = P(G_IMAGE_GROUPS + 8 * 854, G_IMAGE_GROUPS + 8 * 853), [2] = P(G_IMAGE_GROUPS + 8 * 856, G_IMAGE_GROUPS + 8 * 855),
      [4] = P(G_IMAGE_GROUPS + 8 * 858, G_IMAGE_GROUPS + 8 * 857), [5] = P(G_IMAGE_GROUPS + 8 * 860, G_IMAGE_GROUPS + 8 * 859), [6] = P(G_IMAGE_GROUPS + 8 * 862, G_IMAGE_GROUPS + 8 * 861),
      [7] = P(G_IMAGE_GROUPS + 8 * 864, G_IMAGE_GROUPS + 8 * 863), [8] = P(G_IMAGE_GROUPS + 8 * 866, G_IMAGE_GROUPS + 8 * 865), [0xC] = P(G_IMAGE_GROUPS + 8 * 868, G_IMAGE_GROUPS + 8 * 867),
      [0x16] = P(G_IMAGE_GROUPS + 8 * 870, G_IMAGE_GROUPS + 8 * 869), [0x17] = P(G_IMAGE_GROUPS + 8 * 872, G_IMAGE_GROUPS + 8 * 871), [0x1D] = P(G_IMAGE_GROUPS + 8 * 879, G_IMAGE_GROUPS + 8 * 878)},
     G_IMAGE_GROUPS + 8 * 873, 0, G_IMAGE_GROUPS + 8 * 875, G_IMAGE_GROUPS + 8 * 876, G_IMAGE_GROUPS + 8 * 877},
    {FN_MONUMENT_IMAGE_54F490, FN_MONUMENT_PROGRESS_PARTS_31, 1,
     {[0] = P(G_IMAGE_GROUPS + 8 * 881, G_IMAGE_GROUPS + 8 * 880), [1] = P(G_IMAGE_GROUPS + 8 * 883, G_IMAGE_GROUPS + 8 * 882), [2] = P(G_IMAGE_GROUPS + 8 * 885, G_IMAGE_GROUPS + 8 * 884),
      [4] = P(G_IMAGE_GROUPS + 8 * 887, G_IMAGE_GROUPS + 8 * 886), [6] = P(G_IMAGE_GROUPS + 8 * 889, G_IMAGE_GROUPS + 8 * 888), [7] = P(G_IMAGE_GROUPS + 8 * 891, G_IMAGE_GROUPS + 8 * 890),
      [8] = P(G_IMAGE_GROUPS + 8 * 893, G_IMAGE_GROUPS + 8 * 892), [0xB] = P(G_IMAGE_GROUPS + 8 * 895, G_IMAGE_GROUPS + 8 * 894), [0xC] = P(G_IMAGE_GROUPS + 8 * 897, G_IMAGE_GROUPS + 8 * 896),
      [0xD] = P(G_IMAGE_GROUPS + 8 * 899, G_IMAGE_GROUPS + 8 * 898), [0xE] = P(G_IMAGE_GROUPS + 8 * 901, G_IMAGE_GROUPS + 8 * 900), [0x18] = P(G_IMAGE_GROUPS + 8 * 903, G_IMAGE_GROUPS + 8 * 902),
      [0x19] = P(G_IMAGE_GROUPS + 8 * 905, G_IMAGE_GROUPS + 8 * 904), [0x1A] = P(G_IMAGE_GROUPS + 8 * 907, G_IMAGE_GROUPS + 8 * 906), [0x1D] = P(G_IMAGE_GROUPS + 8 * 914, G_IMAGE_GROUPS + 8 * 913)},
     G_IMAGE_GROUPS + 8 * 908, 0, G_IMAGE_GROUPS + 8 * 910, G_IMAGE_GROUPS + 8 * 911, G_IMAGE_GROUPS + 8 * 912},
};
#undef P

static int32_t pyramid_image(game_state *st, const pyramid_images *t, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x00545760 / 0x00558C00 / 0x00555EA0 / 0x0054F490 */
    uint32_t r = BUILDING_ADDR(b);
    *out = 0;
    int32_t stage = S32(st, r + BLD_TOMB_STAGE);
    int32_t type = S32(st, r + BLD_TOMB_STONE_SET);
    if (stage == 0 && type != 0x12 && type != 0x1B && type != 0x1C) {
        *out = 1;
        return (int32_t) U8(st, r + BLD_U9C) + G32(G_IMAGE_GROUPS + 8 * 805);
    }
    int32_t rot = S32(st, r + BLD_TOMB_ROTATION) - half_orient(st);
    if (rot < 0) {
        rot += 4;
    }
    int32_t add = stage == 3 ? 4 : stage == 4 ? 8 : stage == 5 ? 0xC : 0;
    if (t->same_part && S16(st, r + BLD_PART) == (int32_t) P32(st, self + TOMBOBJ_PART)) {
        add = 0;
    }
    /* @bin 0x00545FB0 (a jump to 0x004EA930) */
    if (((uint8_t) GAME_CALLT0(st, self, FN_GOALS_COUNTERS_MET_545FB0) != 0 && U8(st, r + BLD_U94) != 0 &&
         GAME_CALLT(st, self, t->progress, b, 0) == 100) ||
        (S16(st, r + BLD_SEQUENCE) == -1 && U8(st, r + BLD_U94) != 0)) {
        add += 4;
    }
    type = S32(st, r + BLD_TOMB_STONE_SET);
    if ((uint32_t) type > 0x1D) {
        return 0;
    }
    switch (type) {
    case 0x11:
        return G32(t->fixed11) + (t->add11 ? add : 0);
    case 0x12:
        return G32(t->fixed12) + rot;
    case 0x1B:
        return G32(t->fixed1b) + rot;
    case 0x1C:
        return G32(t->fixed1c) + rot;
    default:
        break;
    }
    const uint32_t *e = t->pick[type];
    if (e[0] == 0) {
        return 0;
    }
    if (P32(st, self + TOMBOBJ_PARTS_FREE) != 0) {
        return G32(e[0]) + rot;
    }
    return G32(e[1]) + add + rot;
}

int32_t monument_image(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* the object's vtable +0x1C */
    uint32_t fn = P32(st, P32(st, self) + 0x1C);
    switch (fn) {
    case FN_MONUMENT_IMAGE_401800:
        return monument_image_401800(st, self, b, out);
    case FN_MONUMENT_IMAGE_417310:
        return monument_image_417310(st, self, b, out);
    case FN_MONUMENT_IMAGE_4E6A70:
        return monument_image_4e6a70(st, self, b, out);
    case FN_MONUMENT_IMAGE_40DE80:
        return monument_image_40de80(st, self, b, out);
    default:
        for (int i = 0; i < 4; i++) {
            if (pyramid_tables[i].addr == fn) {
                return pyramid_image(st, &pyramid_tables[i], self, b, out);
            }
        }
        state_unported(st, fn); /* an image function writing its caller's stack */
        return 0;
    }
}

/* ---- the redraw entries ------------------------------------------------------------------- */

static void footprint(game_state *st, int32_t b, int32_t size, int32_t img, int32_t flags)
{
    uint32_t r = BUILDING_ADDR(b);
    GAME_CALL(st, FN_MAP_BUILDING_TILES, b, S16(st, r + BLD_X), S16(st, r + BLD_Y), size, size, img, flags);
}

void monument_redraw_4017a0(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x004017A0 (thiscall) the part's footprint (size: the part table's +0x54) */
    int32_t flags = b;
    int32_t img = monument_image(st, self, b, &flags);
    footprint(st, b, part_entry(st, self, b, MONOBJ_PART_TABLE), img, 8);
}

void monument_redraw_4e69c0(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x004E69C0 (thiscall on 0x006DE548) parts of kind 1 only mark their tile (building
     * grid, terrain bit 0x10000000); the others get their footprint */
    uint32_t r = BUILDING_ADDR(b);
    int32_t flags = b;
    int32_t img = monument_image(st, self, b, &flags);
    int32_t kind = part_entry(st, self, b, MONOBJ_PART_KIND);
    int32_t size = part_entry(st, self, b, MONOBJ_PART_TABLE);
    if (kind == 1) {
        uint32_t t = (uint32_t) (G32(G_MAP_START) + S16(st, r + BLD_X) + S16(st, r + BLD_Y) * 228);
        S16(st, G_GRID_BUILDING + 2u * t) = (int16_t) b;
        U32(st, G_GRID_TERRAIN + 4u * t) |= 0x10000000u;
        return;
    }
    footprint(st, b, size, img, 8);
}

void monument_redraw_40dca0(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x0040DCA0 (thiscall on 0x006FD5D8) flag 1: the tiles are set one by one (terrain
     * bit 0x10000000 below stage 3, else 8; images but for the corners of parts 0xD..0x10 with
     * flag 2 and image 0x00F573BC); else the footprint */
    uint32_t r = BUILDING_ADDR(b);
    int32_t flags = 0;
    int32_t img = monument_image(st, self, b, &flags);
    int32_t part = S16(st, r + BLD_PART);
    int32_t main = b - part;
    int32_t size = part_entry(st, self, b, MONOBJ_PART_TABLE);
    if ((flags & 1) == 0) {
        footprint(st, b, size, img, 8);
        return;
    }
    int32_t base = G32(G_MAP_START) + S16(st, r + BLD_X) + S16(st, r + BLD_Y) * 228;
    S32(st, G_TILE_CURSOR) = base;
    if (size <= 0) {
        return;
    }
    uint32_t row = G_FOOTPRINT_OFFSETS;
    for (int32_t rows = size; rows != 0; rows--, row += 0x30) {
        int32_t corners = flags & 2;
        int32_t stage = S32(st, BUILDING_ADDR(main) + BLD_MONUMENT_STAGE);
        uint32_t e = row;
        for (int32_t col = 0; col < size; col++, e += 8) {
            uint32_t t = (uint32_t) (S32(st, e) + S32(st, G_TILE_CURSOR));
            if (stage < 3) {
                U8(st, G_GRID_TERRAIN + 4u * t + 3) |= 0x10;
            } else {
                U32(st, G_GRID_TERRAIN + 4u * t) = 8;
            }
            U8(st, G_GRID_BITFIELDS + t) &= 0xE0;
            U8(st, G_GRID_EDGE + t) |= 0x40;
            S16(st, G_GRID_BUILDING + 2u * t) = (int16_t) b;
            if (corners != 0 && img == G32(G_IMAGE_GROUPS + 8 * 769)) {
                int skip;
                if (part == 0xD) {
                    skip = col > 0 || row < G_FOOTPRINT_OFFSETS + 0x60;
                } else if (part == 0xE) {
                    skip = col < 2 || row < G_FOOTPRINT_OFFSETS + 0x60;
                } else if (part == 0xF) {
                    skip = col > 0 || row > G_FOOTPRINT_OFFSETS;
                } else if (part == 0x10) {
                    skip = col < 2 || row > G_FOOTPRINT_OFFSETS;
                } else {
                    skip = 0;
                }
                if (skip) {
                    continue;
                }
            }
            if (img != 0) {
                U32(st, G_GRID_IMAGE + 4u * t) = img == G32(G_PLAIN) ? (uint32_t) ((U8(st, G_GRID_RANDOM + t) & 7) + img) : (uint32_t) img;
            }
        }
    }
}

int32_t monument_parts_free_547ba0(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x00547BA0 (thiscall) 0 when the main part's +0x98 is above 0 or one of the
     * object's parts (vtable +0x38 count, +0x24 test from b) is taken; else 1 */
    uint32_t r = BUILDING_ADDR(b);
    if (S32(st, BUILDING_ADDR(b - S16(st, r + BLD_PART)) + BLD_TOMB_STOCK) > 0) {
        return 0;
    }
    uint32_t vt = P32(st, self);
    int32_t n = GAME_CALLT0(st, self, P32(st, vt + 0x38));
    for (int32_t i = 0; i < n; i++) {
        if ((uint8_t) GAME_CALLT(st, self, P32(st, vt + 0x24), i + b) != 0) {
            return 0;
        }
    }
    return 1;
}

/* @bin 0x005468D0 the terrain tiles set to 8 by building type (+0x68) and rotation (+0x90):
 * offsets into FOOTPRINT (byte offsets / 8 = entries) */
static const uint8_t *pyramid_marks(int32_t type, int32_t rot, int *n)
{
    static const uint8_t t0[4][2] = {{0x08, 0x38}, {0x30, 0x38}, {0x00, 0x30}, {0x00, 0x08}};
    static const uint8_t t1[4][6] = {{0x00, 0x08, 0x10, 0x30, 0x38, 0x40},
                                     {0x08, 0x10, 0x38, 0x40, 0x68, 0x70},
                                     {0x30, 0x38, 0x40, 0x60, 0x68, 0x70},
                                     {0x00, 0x08, 0x30, 0x38, 0x60, 0x68}};
    static const uint8_t t2[4][3] = {{0x00, 0x08, 0x10}, {0x10, 0x40, 0x70}, {0x60, 0x68, 0x70}, {0x00, 0x30, 0x60}};
    static const uint8_t t3[4][3] = {{0x10, 0x40, 0x70}, {0x60, 0x68, 0x70}, {0x00, 0x30, 0x60}, {0x00, 0x08, 0x10}};
    static const uint8_t t4[4][10] = {{0x18, 0x20, 0x48, 0x50, 0x78, 0x80, 0xA8, 0xB0, 0xD8, 0xE0},
                                      {0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0},
                                      {0x00, 0x30, 0x60, 0x90, 0xC0, 0x08, 0x38, 0x68, 0x98, 0xC8},
                                      {0x00, 0x08, 0x10, 0x18, 0x20, 0x30, 0x38, 0x40, 0x48, 0x50}};
    static const uint8_t t5[4][15] = {
        {0x00, 0x08, 0x10, 0x18, 0x20, 0x30, 0x38, 0x40, 0x48, 0x50, 0x60, 0x68, 0x70, 0x78, 0x80},
        {0x10, 0x40, 0x70, 0xA0, 0xD0, 0x18, 0x48, 0x78, 0xA8, 0xD8, 0x20, 0x50, 0x80, 0xB0, 0xE0},
        {0x60, 0x68, 0x70, 0x78, 0x80, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0},
        {0x00, 0x30, 0x60, 0x90, 0xC0, 0x08, 0x38, 0x68, 0x98, 0xC8, 0x10, 0x40, 0x70, 0xA0, 0xD0}};
    static const uint8_t t6[4][2] = {{0x00, 0x70}, {0x60, 0x70}, {0x10, 0x70}, {0x00, 0x10}};
    static const uint8_t t7[4][10] = {{0x00, 0x08, 0x10, 0x18, 0x20, 0x30, 0x38, 0x40, 0x48, 0x50},
                                      {0x18, 0x48, 0x78, 0xA8, 0xD8, 0x20, 0x50, 0x80, 0xB0, 0xE0},
                                      {0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0},
                                      {0x00, 0x30, 0x60, 0x90, 0xC0, 0x08, 0x38, 0x68, 0x98, 0xC8}};
    static const uint8_t t8[1] = {0x00};
    *n = 0;
    if (type == 0x1B || type == 0x1C) {
        *n = 1;
        return t8;
    }
    if ((uint32_t) rot > 3) {
        return NULL;
    }
    switch (type) {
    case 1:
    case 0x1D:
        *n = 2;
        return t0[rot];
    case 2:
        *n = 6;
        return t1[rot];
    case 9:
    case 10:
    case 0xD:
    case 0xE:
        *n = 3;
        return t2[rot];
    case 0xB:
        *n = 3;
        return t3[rot];
    case 0x13:
        *n = 10;
        return t4[rot];
    case 0x16:
        *n = 15;
        return t5[rot];
    case 0x18:
        *n = 2;
        return t6[rot];
    case 0x1A:
        *n = 10;
        return t7[rot];
    default:
        return NULL;
    }
}

void monument_redraw_5468d0(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x005468D0 (thiscall) the pyramid-like monuments' parts */
    uint32_t r = BUILDING_ADDR(b);
    int16_t part = S16(st, r + BLD_PART);
    int32_t main = b - part;
    if (part == 0) {
        P32(st, self + TOMBOBJ_PARTS_FREE) = (uint32_t) monument_parts_free_547ba0(st, self, b) & 0xFF;
    }
    int32_t n = U8(st, r + BLD_SIZE);
    if (S32(st, BUILDING_ADDR(main) + BLD_MONUMENT_STAGE) == 1 && S32(st, G_OVERLAY) != 0x33) {
        int32_t base = G32(G_MAP_START) + S16(st, r + BLD_X) + S16(st, r + BLD_Y) * 228;
        if (S16(st, r + BLD_PART) == (int32_t) P32(st, self + TOMBOBJ_PART)) {
            uint32_t t = (uint32_t) (base + G32(G_FOOTPRINT_OFFSETS));
            U8(st, G_GRID_BITFIELDS + t) &= 0xE0;
            U8(st, G_GRID_EDGE + t) |= 0x40;
            U32(st, G_GRID_IMAGE + 4u * t) = (uint32_t) ((U8(st, G_GRID_RANDOM + t) & 7) + G32(G_PLAIN));
            return;
        }
        if (n <= 0) {
            return;
        }
        uint32_t row = G_FOOTPRINT_OFFSETS;
        for (int32_t i = n; i != 0; i--, row += 0x30) {
            uint32_t e = row;
            for (int32_t j = n; j != 0; j--, e += 8) {
                uint32_t t = (uint32_t) (S32(st, e) + base);
                int32_t v = GAME_CALL(st, FN_TERRAIN_CLIFF_INDEX, (int32_t) t);
                U8(st, G_GRID_BITFIELDS + t) &= 0xE0;
                U8(st, G_GRID_EDGE + t) |= 0x40;
                U32(st, G_GRID_IMAGE + 4u * t) = (uint32_t) (G32(G_IMAGE_FLAT_TOP) + v);
            }
        }
        return;
    }
    int32_t flags = 0;
    int32_t img = monument_image(st, self, b, &flags);
    if ((flags & 1) != 0) {
        int32_t base = G32(G_MAP_START) + S16(st, r + BLD_X) + S16(st, r + BLD_Y) * 228;
        if (n <= 0) {
            return;
        }
        uint32_t row = G_FOOTPRINT_OFFSETS;
        for (int32_t i = n; i != 0; i--, row += 0x30) {
            uint32_t e = row;
            for (int32_t j = n; j != 0; j--, e += 8) {
                uint32_t t = (uint32_t) (base + S32(st, e));
                U32(st, G_GRID_TERRAIN + 4u * t) = 8;
                S16(st, G_GRID_BUILDING + 2u * t) = (int16_t) b;
                U8(st, G_GRID_BITFIELDS + t) &= 0xE0;
                U8(st, G_GRID_EDGE + t) |= 0x40;
                if (img != 0) {
                    U32(st, G_GRID_IMAGE + 4u * t) =
                        img == G32(G_PLAIN) ? (uint32_t) ((U8(st, G_GRID_RANDOM + t) & 7) + img) : (uint32_t) img;
                }
            }
        }
        return;
    }
    footprint(st, b, n, img, 0x40000000);
    int32_t base = G32(G_MAP_START) + S16(st, r + BLD_X) + S16(st, r + BLD_Y) * 228;
    int count;
    const uint8_t *marks = pyramid_marks(S32(st, r + BLD_TOMB_STONE_SET), S32(st, r + BLD_TOMB_ROTATION), &count);
    for (int k = 0; k < count; k++) {
        U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (base + G32(G_FOOTPRINT_OFFSETS + marks[k]))) = 8;
    }
}

/* ---- the table ---------------------------------------------------------------------------- */

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int32_t image_out(game_state *st, const int32_t *a, int32_t img, int32_t out)
{
    uint8_t *p = state_ptr(st, (uint32_t) a[1], 4);
    if (p != NULL) {
        U32(st, (uint32_t) a[1]) = (uint32_t) out;
    }
    return img;
}

#define IMAGE_RUN(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        int32_t out = (int32_t) (state_ptr(st, (uint32_t) a[1], 4) ? U32(st, (uint32_t) a[1]) : 0); \
        int32_t img = fn(st, THIS(a), a[0], &out); \
        return image_out(st, a, img, out); \
    }

IMAGE_RUN(run_401800, monument_image_401800)
IMAGE_RUN(run_417310, monument_image_417310)
IMAGE_RUN(run_4e6a70, monument_image_4e6a70)
IMAGE_RUN(run_40de80, monument_image_40de80)
IMAGE_RUN(run_pyramid, monument_image)

static int32_t run_4017a0(game_state *st, const int32_t *a)
{
    monument_redraw_4017a0(st, THIS(a), a[0]);
    return 0;
}

static int32_t run_4e69c0(game_state *st, const int32_t *a)
{
    monument_redraw_4e69c0(st, THIS(a), a[0]);
    return 0;
}

static int32_t run_40dca0(game_state *st, const int32_t *a)
{
    monument_redraw_40dca0(st, THIS(a), a[0]);
    return 0;
}

static int32_t run_5468d0(game_state *st, const int32_t *a)
{
    monument_redraw_5468d0(st, THIS(a), a[0]);
    return 0;
}

static int32_t run_547ba0(game_state *st, const int32_t *a)
{
    return monument_parts_free_547ba0(st, THIS(a), a[0]);
}

static int32_t run_545fb0(game_state *st, const int32_t *a)
{
    (void) a;
    return GAME_CALL0(st, FN_GOALS_COUNTERS_MET_4EA930); /* @bin 0x00545FB0 jmp 0x004EA930 */
}

const ported_function rest_monument_ports[] = {
    {0x00401800u, "monument_image_401800", 2, 32, 0, run_401800, 0},
    {0x00417310u, "monument_image_417310", 2, 32, 0, run_417310, 0},
    {0x004E6A70u, "monument_image_4e6a70", 2, 32, 0, run_4e6a70, 0},
    {0x0040DE80u, "monument_image_40de80", 2, 32, 0, run_40de80, 0},
    {0x00545760u, "monument_image_545760", 2, 32, 0, run_pyramid, 0},
    {0x00558C00u, "monument_image_558c00", 2, 32, 0, run_pyramid, 0},
    {0x00555EA0u, "monument_image_555ea0", 2, 32, 0, run_pyramid, 0},
    {0x0054F490u, "monument_image_54f490", 2, 32, 0, run_pyramid, 0},
    {0x004017A0u, "monument_redraw_4017a0", 1, 0, 0, run_4017a0, 0},
    {0x004E69C0u, "monument_redraw_4e69c0", 1, 0, 0, run_4e69c0, 0},
    {0x0040DCA0u, "monument_redraw_40dca0", 1, 0, 0, run_40dca0, 0},
    {0x005468D0u, "monument_redraw_5468d0", 1, 0, 0, run_5468d0, 0},
    {0x00547BA0u, "monument_parts_free_547ba0", 1, 8, 0, run_547ba0, 0},
    {0x00545FB0u, "goals_counters_met (thunk)", 0, 8, 0, run_545fb0, 0},
};
const int rest_monument_ports_count = sizeof(rest_monument_ports) / sizeof(rest_monument_ports[0]);
