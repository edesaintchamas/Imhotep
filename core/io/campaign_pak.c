/* Imhotep - campaign scenario archive (missionN.pak). Format: docs/formats/campaign.md.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "io/campaign_pak.h"

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

int campaign_pak_record(const uint8_t *data, size_t size, int index, size_t *start, size_t *len)
{
    /* @bin 0x004D58B0 the record offset is the u32 at index * 4; a value < 1 means no record */
    if (index < 0 || index >= CAMPAIGN_PAK_SLOTS || size < 4 * CAMPAIGN_PAK_SLOTS) {
        return -1;
    }
    uint32_t off = u32(data + 4 * index);
    if (off < 1 || off >= size) {
        return -1;
    }
    /* The game reads the record sequentially; its end is the next record's offset (or end of
     * file). @diff terrain_check (every record's chunk walk ends exactly there) */
    size_t end = size;
    for (int k = 0; k < CAMPAIGN_PAK_SLOTS; k++) {
        uint32_t o = u32(data + 4 * k);
        if (o > off && o < end) {
            end = o;
        }
    }
    *start = off;
    *len = end - off;
    return 0;
}
