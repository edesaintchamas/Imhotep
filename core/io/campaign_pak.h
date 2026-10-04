/* Imhotep - campaign scenario archive (missionN.pak). Format: docs/formats/campaign.md.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_IO_CAMPAIGN_PAK_H
#define IMHOTEP_IO_CAMPAIGN_PAK_H

#include <stddef.h>
#include <stdint.h>

#define CAMPAIGN_PAK_SLOTS 60 /* offset table of 60 u32 (240 bytes) @diff terrain_check (first offset is 240 in mission1.pak) */

/* Locate record `index`. Returns 0 and sets *start / *len (up to the next record or end of
 * file), or -1 if the slot is empty or out of range. */
int campaign_pak_record(const uint8_t *data, size_t size, int index, size_t *start, size_t *len);

#endif
