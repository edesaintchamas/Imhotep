/* Imhotep - named globals: buildings (the record array itself: state/records.h, its fields:
 * layout/building.h) and the per-type tables of the exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_BUILDING_H
#define IMHOTEP_STATE_LAYOUT_G_BUILDING_H

/* @bin 0x0046A400 the constructor (save.md §5 "Buildings"; chunks 93, 117) */
#define G_BUILDING_HIGHEST 0x0078D790u  /* i32: highest building id in use (chunk 93) */
#define G_BUILDING_SEQUENCE 0x0078D7B4u /* i32: next creation sequence number, BLD_SEQUENCE (chunk 117) */
/* @bin 0x00488030, 0x00488050 thiscall on it */
#define G_BUILDING_MANAGER 0x00BCA350u

/* per-type tables (exe data): i32 entries, index the building type */
#define G_TYPE_LABOUR_CATEGORY 0x005D06ECu /* @bin 0x00459670 property table 0x005D06DC + 0x10, stride 0x14 */
#define G_TYPE_LABOUR_STRIDE 0x14u
#define G_TYPE_WORKERS 0x00705EB8u         /* @bin 0x00459670 workers needed (save.md: per-type table +0) */
#define G_TYPE_TABLE_STRIDE 0x28u          /* @bin 0x00468B70 save.md: 0x00705EB8 + type * 0x28 */
#define G_TYPE_COUNT 0x0070573Cu           /* @bin 0x00461950 i32 per type: the number of such buildings */
#define G_U705C10 0x00705C10u              /* @bin 0x004C4F05 i32: tested with G_U705DF4 by a small wrath of god 2 */
#define G_U705DF4 0x00705DF4u

/* house levels (exe data): 0x0070CDCC + level * 0x58 (save.md §5) */
#define G_HOUSE_LEVEL_STRIDE 0x58u
#define G_HOUSE_LEVEL_U28 0x0070CDF4u /* @bin 0x004E8840 i32: 0 selects the other sentiment rule; 0x0045E4B0 divides by it */
#define G_HOUSE_LEVEL_U3C 0x0070CE08u /* @bin 0x004E8840 i32: added to BLD_CRIME_RISK per pass */

/* the ring of tiles around a building, per size (exe data) */
#define G_RING_OFFSETS 0x005CC9E0u /* @bin 0x00481000, 0x00481210 i32 grid offsets, 0 ends; 0x60 bytes per size */
#define G_RING_OFFSETS_STRIDE 0x60u
#define G_RING_COORDS 0x005CCE98u  /* @bin 0x00481210 i32 (x, y) per ring tile; 0xC0 bytes per size */
#define G_RING_COORDS_STRIDE 0xC0u

/* the festival's performers (exe data, 0x00461950) */
#define G_FESTIVAL_SHARES 0x005DA544u /* @bin 0x00461950 i32[4]: percent of the performers per kind of venue */
#define G_FESTIVAL_VENUES 0x005DA554u /* @bin 0x00461950 four 0-terminated lists of building types, 12 i32 each */
#define G_FESTIVAL_VENUES_STRIDE 0x30u

/* monuments: the object of each monument type (thiscall `this` of its functions; WinMain's set-up,
 * core/boot/monuments.c) */
#define G_MONUMENT_E1 0x006FD5D8u /* @bin 0x0040EB90 type 0xE1 */
#define G_MONUMENT_E3 0x006FD368u /* @bin 0x00417490 type 0xE3 */
#define G_MONUMENT_E4 0x006DE548u /* @bin 0x0046F200 type 0xE4 (0x004E6710) */
#define G_MONUMENT_E5 0x006F9320u /* @bin 0x00457AF0 type 0xE5 (0x00545A30) */
#define G_MONUMENT_E6 0x006DE650u /* @bin 0x00401AF0 type 0xE6 */
#define G_MONUMENT_EA 0x006F5648u /* @bin 0x00457AF0 type 0xEA (0x00558F00) */
#define G_MONUMENT_EB 0x006DE9E0u /* @bin 0x00457AF0 type 0xEB (0x00556230) */
#define G_MONUMENT_EC 0x006E9188u /* @bin 0x00457AF0 type 0xEC (0x0054F8A0) */
/* the pyramid (type 0xB7) tables, by BLD_MONUMENT_KIND (exe data) */
#define G_PYRAMID_PARTS 0x0056F7D8u       /* @bin 0x0046F200 i32: number of parts, G_PYRAMID_KIND != 5 */
#define G_PYRAMID_PARTS_KIND5 0x0056F7ECu /* @bin 0x0046F200 i32: the same, G_PYRAMID_KIND 5 */
#define G_PYRAMID_ENTRY_PART 0x005F3878u  /* @bin 0x004F30B0 i32: the part holding the entry, 0x3C0 per kind */
#define G_PYRAMID_ENTRY_PART_STRIDE 0x3C0u
#define G_PYRAMID_ENTRY_PART_KIND5 0x005F5498u /* @bin 0x004F30B0 the same, G_PYRAMID_KIND 5, 0x80 per kind */
#define G_PYRAMID_ENTRY_PART_KIND5_STRIDE 0x80u
#define G_PYRAMID_PART_CORNER 0x005E4EF8u /* @bin 0x004F30B0 i32 corner code, 8 bytes per part, 0x90 parts per kind */
#define G_PYRAMID_PART_CORNER_STRIDE 8u
#define G_PYRAMID_PART_CORNER_PARTS 0x90u
/* a monument object's fields */
#define MONOBJ_PART_TABLE 0x54u /* @bin 0x0040EB90 i32[13] per part: [0] the part's size */
#define MONOBJ_PART_ENTRIES 13

#endif
