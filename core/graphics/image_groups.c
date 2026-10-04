/* Imhotep - global image ids and image groups of the graphics families loaded at startup.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "graphics/image_groups.h"

#include <string.h>

/* @bin 0x00414420 startup: families loaded into slots 0..29 in this order (0x0053DD10/0x0053DD40
 * -> 0x0053DD90(name, -1)). Missions may reload a slot later at the same base (not handled). */
const char *const image_startup_slots[IMAGE_SLOT_COUNT] = {
    "Pharaoh_Unloaded", "SprMain", "Hyksos", "Pharaoh_General", "Pharaoh_Terrain",
    "SPHINX1A", "SPHINX1B", "SPHINX1C", "SPHINX1D", "SPHINX_EXTRA", "TEMPLE_NILE", "empire",
    "SprAmbient", "Pharaoh_Fonts", "Egyptian", "SprMain2", "expansion", "PyramidDummy",
    "CaesareumDummy", "LtHouseDummy", "LibMainDummy", "LibMainDummy", "LibMainDummy", "LibExtra",
    "MausCarryoverDummy", "TombThutDummy", "AbuSimbelDummy", "TombTutDummy", "TombSetiDummy",
    "TombRamDummy",
};

#define SYSTEM_IMAGES 200 /* @bin 0x0053DFF0 slots other than 0 skip the 200 system.bmp images */
#define GROUP_SLOTS 300   /* @bin 0x0053DFF0 loop over 300 group slots (u16 at offset 80) */

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

static int is_system_bmp(const uint8_t *name)
{
    static const char sys[] = "system.bmp";
    for (size_t i = 0; i < sizeof(sys); i++) {
        int c = name[i];
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        if (c != sys[i]) {
            return 0;
        }
    }
    return 1;
}

int image_groups_build(image_groups *g, const uint8_t *const headers[IMAGE_SLOT_COUNT],
                       const size_t lengths[IMAGE_SLOT_COUNT])
{
    memset(g, 0, sizeof(*g));
    /* @bin 0x0053DB60 the image and group running totals both start at 1 (id 0 = no image)
     * @diff terrain_check (every stored map id is reproduced only with this base) */
    int32_t base = 1, group = 1;
    for (int s = 0; s < IMAGE_SLOT_COUNT; s++) {
        const uint8_t *h = headers[s];
        if (!h || lengths[s] < SG3_HEADER_BYTES) {
            return -1;
        }
        /* @bin 0x0053DFF0 skip system.bmp in slots > 0 (first bitmap name at offset 680) */
        int32_t skip = s > 0 && is_system_bmp(h + 680) ? SYSTEM_IMAGES : 0;
        int32_t count = (int32_t) u32(h + 16) - skip; /* @bin 0x0053DFF0 copies last_index - skip entries */
        g->slot_base[s] = base;
        g->slot_count[s] = count;
        g->slot_skip[s] = skip;
        for (int k = 0; k < GROUP_SLOTS; k++) {
            int32_t v = (int32_t) (h[80 + 2 * k] | h[81 + 2 * k] << 8);
            /* @bin 0x0053DFF0 non-empty groups numbered in slot order; first id = base + v - skip - 1 */
            if (v - skip > 0) {
                if (group >= IMAGE_GROUPS_MAX) {
                    return -1;
                }
                g->group_first[group++] = base + v - skip - 1;
            }
        }
        base += count; /* @bin 0x0053DD90 running total of images */
    }
    g->group_count = group;
    return 0;
}
