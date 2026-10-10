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
#define G_TYPE_FIRE_STEP 0x00705EBCu       /* @bin 0x00468B70 i16 (+4 of the per-type table): BLD_FIRE_RISK step (save.md §5) */
#define G_TYPE_DAMAGE_STEP 0x00705EC0u     /* @bin 0x00468B70 i16 (+8): BLD_COLLAPSE_RISK step */
#define G_TYPE_WALKER_COST 0x00705EC4u     /* @bin 0x004623D1 i32 (+0xC): the stock a walker costs (types 0x43, 0x44: a percentage, 0x004659FB) */
#define G_TYPE_COUNT 0x0070573Cu           /* @bin 0x00461950 i32 per type: the number of such buildings */
#define G_U705C10 0x00705C10u              /* @bin 0x004C4F05 i32: tested with G_U705DF4 by a small wrath of god 2 */
#define G_U705DF4 0x00705DF4u

/* house levels (exe data): 0x0070CDCC + level * 0x58 (save.md §5) */
#define G_HOUSE_LEVEL_STRIDE 0x58u
#define G_HOUSE_LEVEL_VARIANTS 0x005D034Cu  /* @bin 0x004539A0 i32 by level: image variants of small houses */
#define G_HOUSE_LEVEL_GROUP 0x005D03ECu     /* @bin 0x004539A0 u32 by level: the image group */
#define G_HOUSE_LEVEL_IMAGE_ADD 0x005D043Cu /* @bin 0x004539A0 i32 by level: added to the group's image */
#define G_HOUSE_LEVEL_U28 0x0070CDF4u /* @bin 0x004E8840 i32: 0 selects the other sentiment rule; 0x0045E4B0 divides by it */
#define G_HOUSE_LEVEL_GOODS 0x0070CDF8u /* @bin 0x00454B40 i32[4]: goods used a month: pottery, linen, luxury, beer (0x00455CC0) */
#define G_HOUSE_LEVEL_U3C 0x0070CE08u /* @bin 0x004E8840 i32: added to BLD_CRIME_RISK per pass */
#define G_HOUSE_LEVEL_CRIME_RISK 0x0070CE0Cu /* @bin 0x0046A400 i32: a new house's BLD_CRIME_RISK (save.md §5 +0xE4) */

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
#define MONOBJ_PART_X 0x4Cu     /* @bin 0x004015A0 i32: part 0's x, y; 0x34 bytes a part */
#define MONOBJ_PART_Y 0x50u
#define MONOBJ_PART_TABLE 0x54u /* @bin 0x0040EB90 i32[13] per part: [0] the part's size */
#define E3OBJ_FILES 0x220u      /* @bin 0x00417310 u32[]: type 0xE3's object: graphics files by stage */
#define E4OBJ_FILES 0xB4u       /* @bin 0x004E6A70 u32[]: type 0xE4's object: graphics files by stage */
#define TOMBOBJ_SLEDGE 0x4Cu    /* @bin 0x00547C10 u8: a tomb object's sledge is on its way (0x00545C30 clears it) */
#define TOMBOBJ_PART 0x50u      /* @bin 0x00546660 i32: the object's own part kind (terrain 0x80 for it) */
#define TOMBOBJ_PARTS_FREE 0x54u       /* @bin 0x00545C30 i32: := 1 when a tomb is laid out */
#define MONOBJ_PART_KIND 0x58u  /* @bin 0x004176A0 i32: the part's slot (0 free), as BLD_PART at creation (0x004E6840) */
#define MONOBJ_PART_ENTRIES 13

/* the monuments' work lists (exe data): 16-byte slots, each with the animation object that counts a
 * job's work (core/building/monument.c "work-animation lists", @bin 0x00420130, 0x004E5D10) */
#define WORKSLOT_KEY 0u          /* i32: the stage (or key) the slot serves */
#define WORKSLOT_FIGURE_TYPE 4u  /* i32: the figure type (FIG_TYPE) it takes */
#define WORKSLOT_ANIM 8u         /* u32: its animation object (core/city/day_anim.c, ANIMOBJ_) */
#define WORKSLOT_WORK 0xCu       /* i32: the work, copied into the object's ANIMOBJ_WORK (0x004E5B90) */
#define G_WORK_SLOTS_CF 0x00586B08u /* @bin 0x004E5C00 10 slots: type 0xCF, by its stage count (0x004E5A30) */
#define G_WORK_SLOTS_D0 0x0058FE48u /* @bin 0x004E5C00 18 slots: the other obelisk type (0xD0) */
#define G_WORK_SLOTS_D5 0x00596080u /* @bin 0x00420240 13 slots: the sun temple (0xD5), by BLD_MONUMENT_STAGE */
#define G_WORK_SLOTS_E1 0x0059EDE8u /* @bin 0x004207E0 27 slots: type 0xE1, by the main part's BLD_MONUMENT_STAGE */
#define G_WORK_SLOTS_E6 0x005A8140u /* @bin 0x00420B10 56 slots: type 0xE6, by the main part's BLD_U54 */
#define G_WORK_SLOTS_E3 0x005AD210u /* @bin 0x00420E20 27 slots: type 0xE3, by BLD_MONUMENT_STAGE */
#define G_WORK_SLOTS_E4 0x005B0058u /* @bin 0x00421120 15 slots: type 0xE4, by BLD_MONUMENT_STAGE */
#define G_WORK_SLOTS_D2 0x005C2748u /* @bin 0x00420130 22 slots: type 0xD2, by the city's CITY_U433A */
/* the tombs' stone slots: 25 tables of three work slots, 0x30 bytes a table, one per stone set
 * (BLD_TOMB_STONE_SET); the template instances' T (@bin 0x004233F0, 0x00422A90) */
#define G_STONE_SLOTS 0x005BE508u

/* graphics files (exe data, file name strings) loaded into an image slot by 0x0053DD90 */
#define G_E6_FILE 0x005760B0u     /* @bin 0x00401800 type 0xE6, slot 0x1A */
#define G_LIB_KIND0_5 0x00576308u /* @bin 0x0040DE80 type 0xE1, part kind 0, by stage: from 0x12 */
#define G_LIB_KIND0_4 0x00576314u /* stages 0xE .. 0x11 */
#define G_LIB_KIND0_3 0x00576320u /* stages 0xB .. 0xD */
#define G_LIB_KIND0_2 0x0057632Cu /* stages 8 .. 0xA */
#define G_LIB_KIND0_1 0x00576338u /* stages 5 .. 7 */

/* type 0xE6's steps (exe data, 4 bytes a step, by BLD_U54; @bin 0x00401BE0) */
#define G_E6_STEPS 0x0056F348u
#define E6STEP_KIND 0u  /* u8 */
#define E6STEP_FLAG 1u  /* u8: 0 a byte of the main part (BLD_U61 + value - 1), else a part's stage */
#define E6STEP_PART 2u  /* u8: the part (1..5) */
#define E6STEP_VALUE 3u /* u8 */
/* obelisks (types 0xCF, 0xD0): the stage files (exe data, i32 names; 0xCF: 4, the other: 6 from +0x10;
 * @bin 0x004E5840) */
#define G_OBELISK_FILES 0x005E4588u

/* a monument model (G_MONUMENT_MODELS, 16 bytes; @bin 0x004F44E0, 0x0053D930, 0x004F4B70) */
#define MONMODEL_CLASS 0u   /* i16: 1 pyramids, 2 mastabas and obelisks, ... */
#define MONMODEL_KIND 2u    /* i16 */
#define MONMODEL_VARIANT 4u /* i16 */

/* the pyramid's tables (exe data), by BLD_MONUMENT_KIND; "_KIND5": the tables used when G_PYRAMID_KIND
 * is 5 (core/building/monument.c, pyramid_terr.c, map_add_monument.c) */
#define G_PYRAMID_BASE_IMAGES 0x0056F840u   /* @bin 0x004EDF50 i32: image offset by part (0-3) + 4 * view */
#define G_PYRAMID_CORNER_OFFSETS 0x0056F880u /* @bin 0x004EDF50 i32[4]: grid offsets of a part's corner tiles */
#define G_PYRAMID_PART_OFFSETS 0x0056F898u  /* @bin 0x004EE780 (i32 dx, dy) of part k + 1 */
#define G_PYRAMID_PART_OFFSETS_KIND5 0x0056FD18u
#define G_PYRAMID_ENTRY_COUNT 0x00570070u   /* @bin 0x004EF100 i32: the entries of G_PYRAMID_ENTRY_PART */
#define G_PYRAMID_D7_OFFSET 0x005700D4u     /* @bin 0x004F5E70, 0x004F5F10 i32: the type 0xD7 building's offset from the main part */
#define G_PYRAMID_D7_OFFSET_KIND5 0x005700E8u
#define G_PYRAMID_BESIDE_OFFSETS 0x005700F4u /* @bin 0x004F1EF2 i32[3] grid offsets by view, 12 bytes */
#define G_PYRAMID_RISE_TIME_A 0x005E4EE8u   /* @bin 0x004EEB60 i32: a rising part's work time: A + B */
#define G_PYRAMID_RISE_TIME_B 0x005E4EECu
#define G_PYRAMID_DONE_TIME 0x005E4EF0u     /* @bin 0x004EEB60 i32: a finished part's work time */
#define G_PYRAMID_PART_CORNER_KIND5 0x005E6578u /* @bin 0x004F30B0 the same as G_PYRAMID_PART_CORNER, 0x24 parts per kind */
#define G_PYRAMID_LEVEL_LIST 0x005F4B38u    /* @bin 0x004F25C0 (i32 part, level), 60 per kind (0x1E0 bytes) */
#define G_PYRAMID_RAMP_STEPS 0x005F5618u    /* @bin 0x004EE205 (i32 x, y) by ramp mark and shape */
#define G_PYRAMID_RAMP_STEPS3 0x005F57C8u   /* @bin 0x004EE2BF G_PYRAMID_KIND 3: 16 bytes by mark and shape */
#define G_PYRAMID_RAMP_BASE 0x005F5B28u     /* @bin 0x004EE205 (i32 x, y) by shape */
#define G_PYRAMID_WORK_CLIMATE2 0x005F5C40u /* @bin 0x004EE780 i32: BLD_PYRAMID_WORK at creation, G_PYRAMID_KIND 2 and 4 */
#define G_PYRAMID_WORK_CLIMATE3 0x005F5C54u /* G_PYRAMID_KIND 3 */
#define G_PYRAMID_WORK_CLIMATE1 0x005F5C68u /* G_PYRAMID_KIND 1 */
#define G_PYRAMID_WORK_CLIMATE5 0x005F5C7Cu /* G_PYRAMID_KIND 5 */
#define G_PYRAMID_RAMP_RANGES 0x005F5CA8u   /* @bin 0x004F2190 6 entries of 4 part numbers: two ranges */
#define G_PYRAMID_RAMP_START 0x005F5D08u    /* @bin 0x004F3C90 i32: the ramp's start, offset from the main part */
#define G_PYRAMID_PAIRS_KIND5 0x005F5D58u   /* @bin 0x004F27B0 (i32 a, b) pairs until a < 0, 0xA0 bytes per kind */
#define G_PYRAMID_PAIRS 0x005F5F38u         /* the same, 0x270 bytes per kind */
#define G_PYRAMID_BLOCK_CORNERS 0x005F6BDCu /* @bin 0x004F3690 i32[4]: the corner tiles' offsets */
#define G_PYRAMID_STAGE3_LISTS 0x005F6CD0u  /* @bin 0x004F4160, 0x004F3C90 (u32 list, i32 count) per kind: (part, offset) pairs */
#define G_PYRAMID_STAGE3_LISTS_KIND5 0x005F6D58u
/* an entry of G_PYRAMID_ENTRY_PART (0x14 bytes; @bin 0x004EF100, 0x004EF200) */
#define PYRENTRY_LEVEL_A 4u     /* i32: the first level (+0 the part) */
#define PYRENTRY_LEVEL_B 8u     /* i32: the second level */
#define PYRENTRY_PARTNER_A 0xCu /* i32 */
#define PYRENTRY_PARTNER_B 0x10u

/* the mastaba (type 0xDE): 116 parts of 13 i32 (0x34 bytes; @bin 0x004DED50: +0 x, +4 y, +8 size,
 * +0x10 offset) */
#define G_MASTABA_LAYOUT 0x005E2AD8u
#define G_MASTABA_PART_KIND 0x005E2AE4u /* @bin 0x004E05F0 u32: low byte the kind, 0xF00 the corner, 0xF000 the stone corner */
/* the sun temple (type 0xD5): 100 parts of 0x34 bytes (@bin 0x005417B0: x, y, size) */
#define G_SUN_TEMPLE_PARTS 0x00601DD8u
#define G_SUN_TEMPLE_PART_KIND 0x00601DE4u /* @bin 0x00541DE0 u32: the part's kind, 0..0xC */
#define G_SUN_TEMPLE_FILES 0x00603228u     /* @bin 0x005414B0 i32[3]: the main part's files by stage */

/* storage settings records, 0xC4 bytes, selected by BLD_STORAGE_ID (save.md §5 +0xC1) */
#define G_STORAGE_RECORDS 0x007900BCu /* @bin 0x00456FC0 (core/building/market.c, core/figure/helpers_xa.c) */
#define STORAGE_REC_BUILDING 0u       /* i16: the building using the record */
#define STORAGE_REC_IN_USE 4u         /* u8 */
#define STORAGE_REC_ORDERS 6u         /* u8 per good: 1 not taken (G_STORAGE_ORDERS) */

/* counts of buildings */
#define G_TOWER_SENTRY_REQUEST 0x00BCB9ACu /* @bin 0x004B7400 i32: tower sentry request (save.md chunk 116): while > 0, a figure 0x2A for a type 0xAD building; := 2, counts down */
#define G_TYPE_B6_COUNT 0x00BCB9B0u /* @bin 0x0046AC50 i32: the type 0xB6 buildings (map_add +1, release -1) */


/* the houses' day (day step 0x27, @bin 0x004528B0) */
#define G_POSITION_LOST 0x0078D5C0u     /* @bin 0x0046B740 i32: buildings removed, not found on the building grid */
#define G_POSITION_SEARCHES 0x0078D5C4u /* @bin 0x0046B740 i32: building grid searches */
#define G_HOUSES_EXPANDED 0x0078D588u   /* @bin 0x004528B0 i32: a house expanded: the routing grids are rebuilt */
#define G_HOUSE_EVOLVE_FNS 0x005DA3D4u  /* @bin 0x004528B0 u32 by type: the evolution functions (exe data) */
#define G_MERGE_CORNERS 0x005DA2C4u     /* @bin 0x0046B480 (i32 dx, dy) by anchor: the merged house's corner */
#define G_MERGE_ANCHORS 0x005DA2E4u     /* @bin 0x0046B480 i32[4]: grid offsets of the anchors */
#define G_MERGE_TILES 0x005DA2F4u       /* @bin 0x0046B9E0 i32: grid offsets of a larger house's tiles from its corner */

/* the daily update 0x00462180 */
#define G_FIGURE_28_SENT 0x0078D584u /* @bin 0x004626CF i32: one figure 0x28 a day at most (types 0x18 .. 0x1D) */

/* storage */
#define G_STORAGE_GOOD_PENDING 0x0078D58Cu /* @bin 0x0045BF00 i32: the good (of kind 2) a storage cart is still to fill */

/* farms */
#define G_HARVEST_DOUBLED 0x00BCA348u /* @bin 0x00488400 u8: := 1 when a floodplain harvest is doubled (CITY_U4994) */

/* the ruins on fire (type 0x63; @bin 0x00469730) */
#define G_FIRE_LIST_INDEX 0x0078D7FCu /* i32: the next entry, capped at 499 (save.md chunk 96's count) */
#define G_FIRE_LIST_SIZE 0x0078D800u  /* i32: the list's size (chunk 85); := 0 by the pass */
#define G_FIRE_LIST_COUNT 0x0078D804u /* i32: burning ruins found by the last scan (chunk 84) */
#define G_WIND_DIRECTION 0x0078D808u  /* i32 0..7: the fire spreads that way */
#define G_FIRE_LIST 0x0078DD90u       /* i16[500]: the burning ruins (chunk 96) */

/* a ring queue of tiles (@bin 0x00468590 the groundwater; core/city/day_buildings.c) */
#define G_TILE_QUEUE_TAIL 0x0078C40Cu /* i32 */
#define G_TILE_QUEUE_HEAD 0x0078C410u /* i32 */
#define G_TILE_QUEUE 0x0078C414u      /* i32[1000] */

/* water lifts (type 0x5A; @bin 0x00487EC0, read by 0x00487D30 for the image) */
#define G_WATER_LIFT_FLOOD 0x00BCA32Cu /* u8: a floodplain object near (0x00487E30) */
#define G_WATER_LIFT_WATER 0x00BCA32Du /* u8: BLD_WATER_SUPPLY != 0 */

/* the day's fertility (@bin 0x004860D0, 0x004861BA, 0x0048643D) */
#define G_IRRIGATION_GROWTH_A 0x00978D8Cu /* i32: added (with _B) to an irrigated tile's fertility */
#define G_FLOODPLAIN_GROWTH_A 0x00978D90u /* i32 */
#define G_IRRIGATION_GROWTH_B 0x00978D94u /* i32 */
#define G_LIST_DONE_6C 0x00978D98u        /* u8: a type 0x6C building has worked the full irrigated tiles */
#define G_IRRIGATED_SEEN 0x00978D99u      /* u8: an irrigated tile was met */
#define G_LIST_DONE_C3 0x00978D9Au        /* u8: a type 0xC3 building has worked the full floodplain tiles */
#define G_FLOODPLAIN_SEEN 0x00978D9Bu     /* u8 */

/* tile tables (exe data) */
#define G_NEIGHBOUR4_OFFSETS 0x005CD678u /* @bin 0x00468590 i32[4]: the four neighbours' grid offsets */
#define G_AHEAD_OFFSETS_A 0x005CD6C8u    /* @bin 0x00487E60 i32[4] by BLD_ORIENTATION: tiles ahead */
#define G_AHEAD_OFFSETS_B 0x005CD6D8u

/* tile tables of the footprints (exe data) */
#define G_SQUARE3_OFFSETS 0x005CCCECu  /* @bin 0x0047D060 i32[9]: grid offsets of a 3 x 3 square */
#define G_FOOTPRINT_COORDS 0x005CCD10u /* @bin 0x0046D100, 0x004C04A0 (i32 dx, dy) of a footprint's tiles, in order */

/* the gatehouse's four parts (exe data; part records G_GATE_PARTS, 16 bytes: +0 u8 flag, +4 size, +8,
 * +0xC offsets by rotation; @bin 0x004BFB30, 0x004BFA50) */
#define G_GATE_GROUPS 0x005DF768u     /* @bin 0x004BFB30 i32[4]: the parts' image groups (0x1AE: by god) */
#define G_GATE_GOD_GROUPS 0x005DF778u /* @bin 0x004BFB30 i32: the group replacing 0x1AE, by god (0x004C53D0) */
#define G_GATE_PART_IDS 0x005DF78Cu   /* @bin 0x004BFA50 i32[4] part records by rotation, 16 bytes */
/* the wall patterns (exe data; @bin 0x0047AFD0) */
#define G_WALL_PATTERNS 0x005CB9D8u
#define G_WALL_PATTERNS_BUILDINGS 0x005CBCD8u /* next to buildings */
/* the image offsets of the areas drawn by 0x00440060 (exe data, i32 by view, n x n) */
#define G_AREA_IMAGES_1F 0x005D2078u /* type 0x1F, 2 x 2 */
#define G_AREA_IMAGES_1E 0x005D20B8u /* type 0x1E, 3 x 3 */
#define G_AREA_IMAGES_21 0x005D2148u /* type 0x21, 4 x 4 */
#define G_AREA_IMAGES_D1 0x005D2248u /* type 0xD1, 5 x 5 */
/* the 13 x 7 area of G_COMPLEX_TABLE and its building's parts (@bin 0x004C0BE0, 0x004C0D50) */
#define G_COMPLEX_PART_GROUPS 0x005DF8B0u /* i32: 3 parts' image groups by view and direction */
#define G_COMPLEX_ORIGIN 0x005E00CCu      /* (i32 dx, dy) by direction / 2 */
#define G_COMPLEX_VIEW 0x005E014Cu        /* i32: the image column by direction / 2 + 4 * view / 2 */
/* the mastaba's wall files (exe data, i32 names, 16 bytes by state and mission; @bin 0x004DF360) */
#define G_MASTABA_WALL_FILES 0x005E4268u

#define G_COMPLEX_PART_OFFSETS 0x005DF970u /* @bin 0x004C3420 (i32 dx, dy) by turn: the complex's parts apart */

/* forts: the soldiers waiting for a new fort, three records of 12 bytes (+0 u8 soldiers' kind, +4 home,
 * +8 u8 count) for fort types 0x2D, 0x2C, 0x28 (@bin 0x004714A0, 0x004B6E10) */
#define G_WAITING_SOLDIERS 0x006FDCE4u
/* docks on the shore (exe data; @bin 0x0047D060) */
#define G_DOCK_WATER_TILES 0x005DBE48u /* i32[9] by turn (36 bytes): non-zero, the tile of G_SQUARE3_OFFSETS is water */
#define G_DOCK_FRONT_TILES 0x005DBED8u /* i32[2] by turn: grid offsets of the two front tiles */
/* graphics files loaded into image slots (exe data, file names) */
#define G_GOD_FILES 0x005E018Cu         /* @bin 0x004710A0 i32 by god: temple complexes, slot 10 */
#define G_SPHINX_FILES 0x00601660u      /* @bin 0x0053D850 i32: 3 per CITY_U433A value, slots 5-7 */

/* the parts list (G_PLACES) is sorted around this corner (@bin 0x004F25C0, the comparator 0x004F24B0) */
#define G_PLACES_ORIGIN_X 0x00EAAC40u
#define G_PLACES_ORIGIN_Y 0x00EAAC3Cu

/* names collected in wave 0 (tools/naming/candidates.tsv), checked against their uses in core/building:
 * @bin the defining port and its comment on each line */
#define G_LIB_EXTRA2 0x00576278u /* type 0xE1 (0x0040DE80): graphics file names (exe data), loaded by stage (core/building/rest_monument.c) */
#define G_LIB_EXTRA 0x00576284u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C5 0x00576290u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C3 0x0057629Cu /* core/building/rest_monument.c */
#define G_LIB_MAIN_C2 0x005762A8u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C1 0x005762B4u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A5 0x005762C0u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C4 0x005762CCu /* core/building/rest_monument.c */
#define G_LIB_MAIN_A4 0x005762D8u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A3 0x005762E4u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A2 0x005762F0u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A1 0x005762FCu /* core/building/rest_monument.c */
#define G_FOOTPRINT_OFFSETS 0x005CC8C0u /* core/map/map_building.c: rows of 6 x (i32 grid offset, u8 edge bits, 3 pad), 0x30 bytes */
#define G_MONUMENT_MODELS 0x005D19C8u /* core/command/menu.c: 16-byte records by monument code */
#define G_COMPLEX_TABLE 0x005DF9B0u /* present/city/ghost_gate.c: 7 rows x 13 records of 0x14 bytes: +0 group, +4..+0x10 image by turn */
#define G_PYR_SKIP 0x005E68D8u /* core/city/monument_rating.c: bytes by size * 6 + level: 1 = a level of 3, else 6 */
#define G_HOUSE_LEVELS 0x0070CDCCu /* core/building/day_houses.c: 0x58 bytes per house level */
#define G_FESTIVAL_TENTH 0x00775678u /* core/command/orders.c */
#define G_MONUMENT_SLOTS 0x00784DE0u /* core/command/menu.c: i16[3]: the scenario's monument codes */
#define G_GOODS_SUM 0x0078D590u /* core/building/day_houses.c: i32[12]: goods of merged houses */
#define G_MERGE_Y 0x0078D76Cu /* core/building/day_houses.c */
#define G_MERGE_X 0x0078D770u /* core/building/day_houses.c */
#define G_TICK_DAY 0x0078D7C4u /* core/sim/tick.c: day within the month, 0..15 (chunk 35) [generic] */
#define G_HOUSE_RESULT 0x0078D7E4u /* core/building/day_houses.c: i32: the evolution check's result (-1, 0, 1) */
#define G_HOUSE_ID 0x0078D838u /* core/building/day_houses.c: the house being evolved (global loop counter) */
#define G_STORAGE_ORDERS 0x007900C2u /* core/figure/helpers_xa.c: u8 per good of the record selected by building +0xC1 */
#define G_COUNTS 0x0096E644u /* core/figure/helpers_xa.c: 27 i32: candidates per good index */
#define G_GRID_DESIRABILITY 0x00AC2D4Cu /* i8 per tile (save.md chunk 16, desirability.md) */
#define G_GRID_RANDOM 0x00ACF85Cu       /* u8 per tile (chunk 15); 0x0046A400 BLD_RANDOM */
#define G_GRID_BITFIELDS 0x00B285CCu    /* u8 per tile: the bitfields grid (chunk 13) */
#define G_GRID_BUILDING 0x00B4E6FCu     /* i16 per tile: the building id (chunk 9) */
#define G_GRID_EDGE 0x00B67D1Cu         /* u8 per tile: the edge grid (chunk 8) */
#define G_GRID_IMAGE 0x00B7482Cu        /* i32 per tile: the image grid (chunk 7) */
#define G_UNDO_TYPE 0x00BC9690u /* core/command/undo.c */
#define G_OUT_OF_MAP 0x00BC9734u /* core/building/day2_destroy.c */
#define G_FOOTPRINT_ROWS 0x00BC9738u /* core/command/construction_checks.c */
#define G_FOOTPRINT_COLS 0x00BC973Cu /* core/command/construction_checks.c */
#define G_PLACES 0x00EAAC44u /* i32[]: the pyramid parts in progress (0x004EF450), sorted by 0x004F25C0 (core/figure/builders.c) */
#define G_PLACE_COUNT 0x00EAAF58u /* i32: their count */
#define G_GOODS_COUNT 0x00F53DD8u /* core/command/orders_storage.c: i32 count, then i32 resources (granaries, bazaars) */
#define G_GOODS_LIST 0x00F53DDCu /* core/building/market.c: i32 per entry: a good (1..0x13), 0: none */
#define G_GRAPHICS_GFX 0x00F55BB0u /* the graphics object: `this` of 0x0053DD90, which loads a file into an image slot (core/city/day2_farm.c) */
#define G_IMAGE_GROUPS 0x00F55BB4u /* i32 per sprite group, 8 bytes a group: its first image (core/map/terrain.c writes them; core/figure/actions_a.c) */
#define G_FORT_IMAGE 0x00F5662Cu /* present/city/ghost_big.c: i32 the fort's first image */
#define G_PLAIN 0x00F56B54u /* core/building/rest_monument.c: the plain ground image */
#define G_IMAGE_FLAT_TOP 0x00F572ECu /* present/city/overlay.c: group base: images + 0x30 .. + 0x47 are drawn 0x96 higher */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
