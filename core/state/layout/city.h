/* Imhotep - the city record's fields (chunk 28: CITY_BASE, one record of 0x49D8 bytes per player;
 * the human player is record 1, CURRENT_CITY(st) the current one).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Offsets within one record: S32(st, CURRENT_CITY(st) + CITY_TREASURY). Evidence: docs/formats/save.md
 * §5 "City data" (field map and the code cited there) or the function named after the field. A field
 * whose meaning is not established is CITY_U<offset>. Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_CITY_H
#define IMHOTEP_STATE_LAYOUT_CITY_H

#include "state/records.h"

/* @bin 0x004DE820 the scenario start, 0x004E7D10 the init; the readers cited in save.md §5 */
#define CITY_ACTIVE 0x02u            /* u8: player active (0x00540A10, 0x004E9680) */
#define CITY_TAX_RATE 0x08u          /* i32: init 9 (0x004599D0) */
#define CITY_TREASURY 0x0Cu          /* i32 (0x004DE820) */
#define CITY_SENTIMENT 0x10u         /* i32: average house BLD_SENTIMENT, 60 without houses (0x004E8840) */
#define CITY_HEALTH_TARGET 0x14u     /* i32 (0x004E81D0) */
#define CITY_HEALTH 0x18u            /* i32 */
#define CITY_POPULATION 0x24u        /* i32: sum of the age array (0x00451C50) */
#define CITY_POPULATION_LAST_YEAR 0x28u /* i32 */
#define CITY_U270C 0x270Cu           /* i32: read by 0x004E8EC0 */
#define CITY_ENTRY_X 0x2778u         /* u16: entry point (0x004DE820) */
#define CITY_ENTRY_Y 0x277Au         /* u16 */
#define CITY_ENTRY_OFFSET 0x277Cu    /* u32: its grid offset */
#define CITY_EXIT_X 0x2780u          /* u16: exit point */
#define CITY_EXIT_Y 0x2782u          /* u16 */
#define CITY_EXIT_OFFSET 0x2784u     /* u32: its grid offset */
#define CITY_U28B6 0x28B6u           /* i16[36] per resource: != 0 tested by 0x00459240 (save.md: industry off, assumed) */
#define CITY_U2988 0x2988u           /* i32: set when the Sphinx (type 0xD2) message is posted (0x0046F200) */
/* labour categories (0x00458880, 0x00459100, 0x00459670): 10 records of CITY_LABOUR_STRIDE bytes */
#define CITY_LABOUR_NEEDED 0x2B2Cu   /* i32: workers needed (0x00459670) */
#define CITY_LABOUR_AVAILABLE 0x2B30u /* i32: workers available (0x00459670) */
#define CITY_LABOUR_STRIDE 0x14u
#define CITY_UNEMPLOYMENT 0x2BFCu    /* i32: unemployment percent */
#define CITY_WAGE_RATE 0x2C08u       /* i32: init 30 */
#define CITY_WAGE_KINGDOM 0x2C0Cu    /* i32: kingdom wage reference */
#define CITY_U2C38 0x2C38u           /* i32: indexes G_SENTIMENT_TAX_TABLE with CITY_TAX_RATE (0x004E8840) */
#define CITY_U433A 0x433Au           /* i16: read by 0x0046F200 for the Sphinx message (>= 0x10) */
/* the gods (0x004E9890, 0x004C47A0): one entry per god, 5 gods */
#define CITY_GOD_TARGET_MOOD 0x4390u /* u8[5] */
#define CITY_GOD_MOOD 0x439Au        /* u8[5] */
#define CITY_GOD_WRATH 0x43A4u       /* u8[5]: 0x32 wrath, >= 0x14 small wrath (0x004C47A0) */
#define CITY_GOD_MONTHS 0x43E0u      /* i32[5]: months since the god's festival */
#define CITY_GOD_BLESSING 0x4816u    /* u8[5]: 0x32 blessing, >= 0x14 small blessing (0x004C47A0) */
#define CITY_U4478 0x4478u           /* i16: read by 0x004C38F0 */
#define CITY_FESTIVAL_SIZE 0x44F8u   /* i32: planned festival size (0x004EA000) */
#define CITY_FESTIVAL_MONTHS 0x44FCu /* i32: months to the planned festival */
#define CITY_FESTIVAL_GOD 0x4500u    /* i32 */
#define CITY_U4528 0x4528u           /* i32: := 1 with a planned festival (0x004C47A0) */
#define CITY_U4534 0x4534u           /* i32: := CITY_UNEMPLOYMENT (0x004E8840) */
#define CITY_SENTIMENT_LAST 0x4538u  /* i32: CITY_SENTIMENT at the previous pass (0x004E8840) */
#define CITY_SENTIMENT_LOW 0x4540u   /* i32: the lowest factor below 0, 1-5 (0x004E8840) */
#define CITY_U45CC 0x45CCu           /* i32: := 0xC by a blessing of god 1 (0x004C47A0) */
#define CITY_U45D0 0x45D0u           /* i32: := 0xA by a blessing of god 3 */
#define CITY_U4638 0x4638u           /* i32: := CITY_WAGE_RATE (0x004E8840) */
#define CITY_U463C 0x463Cu           /* i32: read by 0x004E8EC0 */
#define CITY_U4640 0x4640u           /* i32: read by 0x004E8EC0 */
#define CITY_U4674 0x4674u           /* i32: the first building of type 0x48 or 0xC1 (0x00457AF0) */
#define CITY_U468B 0x468Bu           /* u8: toggled by every pass of 0x004E8840 */
#define CITY_ROAD_NETWORKS 0x46ACu   /* u32 id, 8 bytes per entry, 10 entries: ranked by 0x00481210 */
#define CITY_U472C 0x472Cu           /* i32: a building, read by 0x004C38F0 */
#define CITY_U47B4 0x47B4u           /* i32: := 0xC by a wrath of god 1 (0x004C47A0) */
#define CITY_U4984 0x4984u           /* i16: := 0xC by a small blessing of god 1 */
#define CITY_U4986 0x4986u           /* i16: := 0xC by a wrath of god 1 */
#define CITY_U4988 0x4988u           /* i16: := 0xC by a small wrath of god 1 */
#define CITY_U498C 0x498Cu           /* i32: := 0xA by a small blessing of god 3 */
#define CITY_U4994 0x4994u           /* i32: := 1 by a blessing of god 0 */
#define CITY_U4998 0x4998u           /* i32: := 1 by a small wrath of god 0 */

#endif
