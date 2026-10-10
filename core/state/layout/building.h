/* Imhotep - the building record's fields (chunk 32: BUILDING_BASE, 4000 records of 0x108 bytes).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Offsets within one record: S16(st, BUILDING_ADDR(id) + BLD_TYPE). The access keeps its width at
 * each use (S16, U8, ...). Evidence: docs/formats/save.md §5 "Buildings" (the code cited there) or
 * the function named after the field. A field whose meaning is not established is BLD_U<offset>.
 * Some offsets mean different things for different building types: each meaning has its own name.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_BUILDING_H
#define IMHOTEP_STATE_LAYOUT_BUILDING_H

#include "state/records.h"

/* @bin 0x0046A400 the constructor, 0x004D41E0 the loader (save.md §5) */
#define BLD_STATE 0x00u            /* u8: 0 free, 1 in use, 3 created; 2 abandoned (0x00457AF0), 4 collapse (0x00468B70) */
#define BLD_OWNER 0x01u            /* u8: player index */
#define BLD_U02 0x02u              /* u8: copied from the owner's city record +0x05 */
#define BLD_SIZE 0x03u             /* u8: footprint n x n */
#define BLD_HOUSE_MERGED 0x04u     /* u8: merged house (0x00458440) */
#define BLD_HOUSE_SIZE 0x05u       /* u8: house size class 1-4, 0 for other buildings */
#define BLD_X 0x06u                /* i16: corner tile, map-relative */
#define BLD_Y 0x08u                /* i16 */
#define BLD_GRID_OFFSET 0x0Cu      /* u32: grid offset of the corner tile */
#define BLD_TYPE 0x10u             /* i16: building type (names: text group 41) */
#define BLD_SUBTYPE 0x12u          /* i16: house level (type - 10) for houses, else a per-type value */
#define BLD_INPUT 0x12u            /* i16: workshops: the input good; non-zero, done at 400 (0x00455BF0); BLD_INPUT2 the second */
#define BLD_PART 0x12u             /* i16: monuments: the part's index (id - it: the first part, 0x004F30B0) */
#define BLD_ROAD_NETWORK 0x14u     /* u8: road network id ('roadnet', 0x00457AF0) */
#define BLD_SEQUENCE 0x16u         /* i16: creation sequence number (G_BUILDING_SEQUENCE) */
#define BLD_SERVICE 0x18u          /* i16: service / houses-covered counter (0x004BAA10) */
#define BLD_LABOUR_PERCENT 0x1Au   /* i16: share of its category's workers, percent (0x00459670); < 1: problem 0x1000 */
#define BLD_POPULATION 0x1Cu       /* i16: house population ('pop', 0x00458440) */
#define BLD_ROOM_LEFT 0x1Eu        /* i16: 'spare room' */
#define BLD_DISTANCE 0x20u         /* i16: road distance from the city entry, > 0 reachable (0x00457AF0) */
#define BLD_MAX_POPULATION 0x22u   /* i16: 'max pop' */
#define BLD_DAYS_CUT_OFF 0x24u     /* i16: days without a road to the entry (0x00457AF0; 'no contact with rome') */
#define BLD_ROAD_ACCESS_X 0x26u    /* i16: road tile used for access (0x00462180, 0x00457AF0) */
#define BLD_ROAD_ACCESS_Y 0x28u    /* i16 */
#define BLD_WALKER 0x2Au           /* i16: main walker figure ('agent') */
#define BLD_WALKER2 0x2Cu          /* i16: second walker ('SUB agent') */
#define BLD_IMMIGRANT 0x2Eu        /* i16: immigrant figure (0x00458D00) */
#define BLD_TOWER_WALKER 0x30u     /* i16 (0x00462180) */
#define BLD_SPAWN_DELAY 0x32u      /* u8 (0x00462180) */
#define BLD_ROAM_DIRECTION 0x34u   /* u8: random & 6 at creation */
#define BLD_WATER_SUPPLY 0x35u     /* u8: 'water sup' */
#define BLD_WATER_DEP 0x36u        /* u8: 'water dep' */
#define BLD_PREV_PART 0x38u        /* i16: previous part id, 0 for the main part (0x00475B50) */
#define BLD_NEXT_PART 0x3Au        /* i16: next part id (storage-yard spaces, 0x0045D510) */
#define BLD_STORED 0x3Cu           /* i16: stored amount ('goods' / 'raw materials', 0x00455C30) */
#define BLD_WELL_ACCESS 0x3Fu      /* u8: 'well sup' */
#define BLD_WORKERS 0x40u          /* i16: 'employees' (0x00459670) */
#define BLD_OUTPUT_RESOURCE 0x43u  /* u8 (0x00462180) */
#define BLD_COLLAPSE_RISK 0x46u    /* i16: 'damage' (0x00468B70) */
#define BLD_FIRE_RISK 0x48u        /* i16 (0x00468B70) */
#define BLD_BURNING 0x4Au          /* i16: burning duration (0x00468B20) */
#define BLD_U4C 0x4Cu              /* u8: building property table +4 at creation */
#define BLD_RANDOM 0x4Du           /* u8: random grid & 0x7F at creation */
#define BLD_TAX_COVERAGE 0x4Eu     /* u8: 'tax cvr' (0x00484A90) */
#define BLD_INDUSTRY_PROGRESS 0x54u /* i16: workshops and farms (0x00455C30) */
#define BLD_HOUSE_INVENTORY 0x54u  /* i16[12]: houses (0x00454190) */
#define BLD_MONUMENT_KIND 0x54u    /* u8: monuments: the kind indexing the part tables (0x004F30B0) */
#define BLD_U55 0x55u              /* u8: types 0xCF, 0xD0: 100 when finished (0x0046F200) */
#define BLD_INVENTORY 0x56u        /* i16[]: bazaar (shifted by one), granary per resource */
#define BLD_MONUMENT_STAGE 0x58u   /* i32: monuments: construction stage (0x0040EB90, the work-step tables) */
/* u8: set when the monument's finished message is posted (0x0046F200), by monument type */
#define BLD_MON_ANNOUNCED_60 0x60u /* type 0xE6 */
#define BLD_MON_ANNOUNCED_68 0x68u /* types 0xD5, 0xE1, 0xE3, 0xE4 */
#define BLD_MON_ANNOUNCED_74 0x74u /* type 0xDE */
#define BLD_MON_ANNOUNCED_80 0x80u /* types 0xB7, 0xCF, 0xD0 */
#define BLD_MON_ANNOUNCED_9D 0x9Du /* types 0xE5, 0xEA, 0xEB, 0xEC */
/* monuments: the parts of a monument are consecutive buildings, BLD_PART numbering them from the main
 * part (core/building/map_add_monument.c) */
#define BLD_MONUMENT_SLOT 0x54u    /* i32: monuments laid out from a table: the scenario slot (0x004F4F80; 0x004DF9D0, 0x005417B0) */
#define BLD_U54 0x54u              /* i32: type 0xE6: the work list key of the main part (0x00420A90); 0 at creation */
#define BLD_MONUMENT_WORKER 0x5Cu  /* i32: the figure working the part (0x004E0530, 0x00417590, 0x00541DE0); not pyramids */
#define BLD_MONUMENT_FLAGS 0x64u   /* u32: types 0xDE, 0xD5, main part: the work done, by bits (0x004E05F0, 0x00541DE0) */
#define BLD_U60 0x60u              /* i32: 1 on the main part of the laid-out monuments (0x004DF9D0, 0x005417B0) */
#define BLD_LAYOUT_X 0x68u         /* i32: type 0xDE: the layout's origin x, y and direction (0x004DF9D0, 0x004DED50) */
#define BLD_LAYOUT_Y 0x6Cu
#define BLD_LAYOUT_DIRECTION 0x70u
#define BLD_OBELISK_SLOT 0x12u     /* i16: types 0xCF, 0xD0: the scenario slot (0x004E57F0) */
#define BLD_STORED_GOOD 0x12u      /* i16: storage yard spaces: the good stored, amount BLD_STORED (0x0045B700) */
#define BLD_REC88_SLOT 0xCAu       /* u8: its record of G_FERRY_QUEUES (0xE0 bytes), 0 none (0x0046AC50) */
#define BLD_REC4A_SLOT 0xCBu       /* u8: its record of G_REC4A, 1..10, 0 none (0x0046A2E0) */
#define BLD_AREA_TILE 0xD0u        /* i32: types 0x1E, 0x1F, 0x21, 0xD1: the area's first tile (0x0043FD00) */
#define BLD_GATE_DIRECTION 0xD4u   /* i32: two-tile gates: 0 the other part (BLD_NEXT_PART) at y + 1, else at x + 1 (0x00440540) */
#define BLD_D7_PYRAMID 0x12u       /* i16: type 0xD7: the pyramid's main part (0x004F5F10) */
/* the pyramid (type 0xB7), each part (core/building/pyramid_terr.c: 0x004EE7A0, 0x004F31F0, 0x004EDF50) */
#define BLD_PYRAMID_LEVEL 0x55u    /* u8 */
#define BLD_PYRAMID_STAGE 0x56u    /* u8: within the layer; the level reached: stage + 6 * layer (0x004EF100) */
#define BLD_PYRAMID_LAYER 0x57u    /* u8 */
#define BLD_PYRAMID_LAYERS 0x58u   /* u8: the layer limit, from G_PYRAMID_PART_CORNER + 4 */
#define BLD_PYRAMID_RAMP_A 0x59u   /* u8: ramp marks */
#define BLD_PYRAMID_RAMP_B 0x5Au
#define BLD_PYRAMID_CORNERS 0x5Bu  /* u8: done corners, bits 0-3 (0x004F3690) */
#define BLD_PYRAMID_RAMP_A_POS 0x5Cu /* i16[2]: the ramp marks' offsets (0x004EDF50) */
#define BLD_PYRAMID_RAMP_B_POS 0x60u
#define BLD_PYRAMID_WORKER 0x64u   /* i16: the figure working the part (0x004EEA10) */
#define BLD_PYRAMID_RAMP_IMAGE 0x68u /* i32 (0x004EDF50) */
#define BLD_PYRAMID_WORK 0xC8u     /* i16: main part: the work left (0x004F31F0, 0x004EE780) */
/* royal burial tombs (types 0xE5, 0xEA, 0xEB, 0xEC), each part (@bin 0x00547870, 0x004223F0,
 * 0x004233F0, 0x00422A90; core/building/tomb_stones.c) */
#define BLD_TOMB_STAGE 0x60u       /* i32: the part's stage, 0..4 */
#define BLD_TOMB_STONE_SET 0x68u   /* u32: the stone set, 0..0x1D (G_STONE_SLOTS tables) */
#define BLD_TOMB_LINKS 0x6Cu       /* i32[6]: neighbour parts relative to the first part, -1 none */
#define BLD_TOMB_STONES 0x80u      /* i32[]: by stage (1..3: +0x84 .. +0x8C), the stones still to lay */
#define BLD_TOMB_ROTATION 0x90u    /* i32: the part's rotation, less the view's (0x00545760, 0x005468D0) */
#define BLD_TOMB_STOCK 0x98u       /* i32: main part: the stone stock, taken by 100 (0x00547C10) */
/* u8: house access per service walker (0x004833C0 .. 0x00484780) */
#define BLD_ACCESS_JUGGLER 0x6Cu
#define BLD_ACCESS_MUSICIAN 0x6Du
#define BLD_ACCESS_PHYSICIAN 0x6Eu
#define BLD_ACCESS_DANCER 0x6Fu
#define BLD_ACCESS_MAGISTRATE 0x70u
#define BLD_ACCESS_SENET 0x71u
#define BLD_ACCESS_TEACHER 0x72u
#define BLD_ACCESS_LIBRARIAN 0x73u
#define BLD_U74 0x74u
#define BLD_ACCESS_DENTIST 0x75u
#define BLD_ACCESS_HERBALIST 0x76u
#define BLD_ACCESS_WATER 0x77u
#define BLD_ACCESS_EMBALMER 0x78u
#define BLD_ACCESS_PRIEST 0x79u    /* u8[5]: per god (0x00483FB0) */
#define BLD_U7F 0x7Fu              /* u8: houses: read by the sentiment pass (0x004E8840) */
#define BLD_ENTERTAINMENT 0x80u    /* i8: house levels (0x00454190) */
#define BLD_EDUCATION 0x81u
#define BLD_HEALTH 0x82u
#define BLD_RELIGION 0x83u
#define BLD_EVOLVE_STATUS 0x85u    /* u8 (0x00454190) */
#define BLD_ACCESS_ZOOKEEPER 0x86u /* u8 */
#define BLD_NO_ROOM 0x87u          /* u8: 1 problem 0x20, 2 problem 0x2000 (0x00458440) */
#define BLD_PERFORMERS 0x9Eu       /* i16[]: venues: performer figures (0x00462180) */
#define BLD_BAZAAR_DEMAND 0x9Eu    /* i16[]: bazaar: demand per good (0x00483910) */
#define BLD_SHOWS 0xA6u            /* u8[]: 'shows' flags; workers out for wood cutters / guilds */
#define BLD_ORIENTATION 0xAAu      /* u8 (0x004D4180, 0x0046A400, 0x0047E870) */
#define BLD_DOCKERS 0xAEu          /* i16[3]: docker figures (0x004B4980) */
#define BLD_REQUEST 0xB4u          /* u16: type 0x4A: a request, 0x1000 / 0x2000 and a figure type or id in the low 12 bits (0x004666CE) */
#define BLD_DOCK_BOAT 0xB4u        /* i16: docks (0x4C, 0xB5, 0xB6): the boat figure it is reserved for, 0 free (0x004808A0, 0x00480B80) */
#define BLD_SECOND_WALKER 0xB4u    /* i16: second trader / farm labourer (0x00462180) */
#define BLD_SECOND_CART 0xB6u      /* i16: a second figure: cart pusher (0x00462FCF), bazaar buyer (0x00464A17) (0x00488400) */
#define BLD_UB8 0xB8u              /* i32: value from 0x0045A140, partly lost on destruction */
#define BLD_UBC 0xBCu              /* u8: houses: counter 0..3 of the sentiment pass (0x004E8840) */
#define BLD_DESIRABILITY 0xBEu     /* i8 (0x00454190) */
#define BLD_NEAR_WATER 0xC0u       /* u8: a ring tile has water (0x00481000) */
#define BLD_STORAGE_ID 0xC1u       /* u8: storage settings index (0x00462180) */
#define BLD_SENTIMENT 0xC2u        /* i8: houses: averaged into CITY_SENTIMENT (0x004E8840); 50 at creation */
#define BLD_PROBLEM_OVERLAY 0xC3u  /* u8: 1 problem, 2 no coverage (0x00432920) */
#define BLD_FERRY_PARTNER 0xC4u    /* i32: Ferry Landing (0x88) partner building */
#define BLD_STOCK2 0xCCu           /* i16: second raw-material stock (0x00455C30) */
#define BLD_INPUT2 0xCEu           /* i16: second input code (0x004B4980) */
#define BLD_CRIME_RISK 0xE4u       /* i32: houses (0x00483260) */
#define BLD_MALARIA_RISK 0xE8u     /* i32: houses, 0-1000 (0x004E7F00) */
#define BLD_DIST_TERRAIN_4 0xF4u   /* i32: distance to terrain flag 4 (0x004E7F00) */
#define BLD_DIST_TERRAIN_40000 0xF8u /* i32: distance to terrain flag 0x40000 (0x004E7F00) */
#define BLD_DISEASE_RISK 0xFCu     /* i32: houses, 0-1000 (0x004E7F00) */
#define BLD_PROBLEMS 0x100u        /* u32: problem flags (0x00432920) */
#define BLD_INCOMING_CARTS 0x104u  /* i16: granary incoming carts; workshops input count (0x004BC7A0) */
#define BLD_INCOMING_AMOUNT 0x106u /* i16 */

/* @bin wave 0: the other offsets the ports use (tools/naming/rename.py scan); a name with U and
 * the offset until its meaning is established from the code that uses it */
#define BLD_U42                0x42u
#define BLD_U44                0x44u
#define BLD_U50                0x50u
#define BLD_FORMATION          0x50u /* i16: forts: their formation (0x004B76B0, 0x0046AC50 type 0x39) */
#define BLD_U57                0x57u
#define BLD_U59                0x59u
#define BLD_U5A                0x5Au
#define BLD_U5B                0x5Bu
#define BLD_U5C                0x5Cu
#define BLD_U5E                0x5Eu
#define BLD_U61                0x61u
#define BLD_U62                0x62u
#define BLD_U64                0x64u
#define BLD_U66                0x66u
#define BLD_U6A                0x6Au
#define BLD_U6C                0x6Cu
#define BLD_U7C                0x7Cu
#define BLD_U7E                0x7Eu
#define BLD_U84                0x84u
#define BLD_DEVOLVE_DAYS       0x84u /* u8: houses: days the house failed its needs, devolves after the third (0x00452980) */
#define BLD_U88                0x88u
#define BLD_U8C                0x8Cu
#define BLD_U90                0x90u
#define BLD_U94                0x94u
#define BLD_U98                0x98u
#define BLD_U9C                0x9Cu
#define BLD_UA0                0xA0u
#define BLD_UA2                0xA2u
#define BLD_UA4                0xA4u
#define BLD_UA7                0xA7u
#define BLD_UA8                0xA8u
#define BLD_UA9                0xA9u
#define BLD_UAC                0xACu
#define BLD_UAD                0xADu
#define BLD_UBD                0xBDu
#define BLD_UBF                0xBFu
#define BLD_UC8                0xC8u
#define BLD_UCA                0xCAu
#define BLD_UCB                0xCBu
#define BLD_UD0                0xD0u
#define BLD_UD4                0xD4u
#define BLD_UD8                0xD8u
#define BLD_UDC                0xDCu
#define BLD_UE0                0xE0u
#define BLD_UEC                0xECu
#define BLD_UF0                0xF0u

#endif
