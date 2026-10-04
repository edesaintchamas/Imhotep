/* Imhotep - scenario map (.map) container reader. Format: docs/formats/map.md.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_IO_MAP_FILE_H
#define IMHOTEP_IO_MAP_FILE_H

#include <stddef.h>
#include <stdint.h>

#define MAP_FILE_MAX_CHUNKS 300 /* @bin 0x004D0E10 the schema holds up to 300 records */

/* Chunk indices of the map schema. @bin 0x005E17D8 (static schema table: each index is tied to
 * the variable Pharaoh.exe reads it into, e.g. 4 -> image grid 0x00B7482C). */
enum {
    MAP_CHUNK_IMAGE = 4,        /* u32 per tile, 0x00B7482C */
    MAP_CHUNK_EDGE = 5,         /* u8, 0x00B67D1C */
    MAP_CHUNK_TERRAIN = 6,      /* u32, 0x00AF598C */
    MAP_CHUNK_BITFIELDS = 7,    /* u8, 0x00B285CC */
    MAP_CHUNK_RANDOM = 8,       /* u8, 0x00ACF85C */
    MAP_CHUNK_SCENARIO = 14,    /* 1592 bytes, 0x007848D8 */
    MAP_CHUNK_FERTILITY = 15,   /* u8, 0x00A5D4CC */
    MAP_CHUNK_VEGETATION = 20,  /* u8, 0x00A43EAC */
    MAP_CHUNK_MOISTURE = 23,    /* u8, 0x00A509BC (record filled at run time; address: map.md §6) */
};

/* Chunk indices of the campaign scenario records in mission1.pak (saved-game layout, 175 or 181
 * chunks). Each record's schema addresses map onto these Pharaoh.exe variables with a single
 * constant delta per record (docs/formats/campaign.md). */
enum {
    CAMPAIGN_CHUNK_IMAGE = 7,         /* 0x00B7482C */
    CAMPAIGN_CHUNK_EDGE = 8,          /* 0x00B67D1C */
    CAMPAIGN_CHUNK_BUILDING = 9,      /* 0x00B4E6FC, u16 per tile */
    CAMPAIGN_CHUNK_TERRAIN = 10,      /* 0x00AF598C */
    CAMPAIGN_CHUNK_AQUEDUCT = 11,     /* 0x00AE8E7C */
    CAMPAIGN_CHUNK_BITFIELDS = 13,    /* 0x00B285CC */
    CAMPAIGN_CHUNK_RANDOM = 15,       /* 0x00ACF85C */
    CAMPAIGN_CHUNK_DESIRABILITY = 16, /* 0x00AC2D4C */
    CAMPAIGN_CHUNK_SCENARIO = 75,     /* 0x007848D8 */
    CAMPAIGN_CHUNK_FERTILITY = 135,   /* 0x00A5D4CC */
    CAMPAIGN_CHUNK_VEGETATION = 144,  /* 0x00A43EAC */
    CAMPAIGN_CHUNK_MOISTURE = 170,    /* 0x00A509BC */
};

typedef struct {
    uint32_t version;
    uint32_t count;
    uint8_t *chunk[MAP_FILE_MAX_CHUNKS];
    uint32_t size[MAP_FILE_MAX_CHUNKS];
} map_file;

/* Parse and decompress every chunk. Returns 0 on success, -1 on any inconsistency (bad magic,
 * schema, decompression, or a chunk walk that does not end exactly at end of file). */
int map_file_read(const uint8_t *data, size_t len, map_file *m);

/* Same container without the "MAPS" magic (saved games, mission1.pak records). */
int chunk_file_read(const uint8_t *data, size_t len, map_file *m);
void map_file_free(map_file *m);

#endif
