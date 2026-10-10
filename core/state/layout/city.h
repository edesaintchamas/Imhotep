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
#define CITY_ROAD_NETWORK_TILES 0x46B0u /* i32: an entry's tile count, the ten largest kept (0x00468860) */
#define CITY_U472C 0x472Cu           /* i32: a building, read by 0x004C38F0 */
#define CITY_U47B4 0x47B4u           /* i32: := 0xC by a wrath of god 1 (0x004C47A0) */
#define CITY_U4984 0x4984u           /* i16: := 0xC by a small blessing of god 1 */
#define CITY_U4986 0x4986u           /* i16: := 0xC by a wrath of god 1 */
#define CITY_U4988 0x4988u           /* i16: := 0xC by a small wrath of god 1 */
#define CITY_U498C 0x498Cu           /* i32: := 0xA by a small blessing of god 3 */
#define CITY_U4994 0x4994u           /* i32: := 1 by a blessing of god 0 */
#define CITY_U4998 0x4998u           /* i32: := 1 by a small wrath of god 0 */

/* Package P3 (phase 10.3). Names from the field map of save.md §5 "City data" (its addresses are
 * the evidence); CITY_U<offset> where the map gives no meaning, with what the ports show. */

/* population (save.md §5 "Population") */
#define CITY_U1C 0x1Cu               /* i32: := 0 by 0x00461950 */
#define CITY_U20 0x20u               /* i32: := 0 by 0x0045E420 */
#define CITY_YOUNG 0x2Cu             /* i32: the first 14 ages of CITY_AGES (0x004E94A0, 0x00468170) */
#define CITY_U30 0x30u               /* i32: 7 ages of CITY_AGES from age 14 (0x004E94D0) */
#define CITY_HOUSE_CAPACITY 0x34u    /* i32: house capacity (0x00458440) */
#define CITY_HOUSE_ROOM 0x38u        /* i32: room left in the houses (0x00458440) */
#define CITY_POPULATION_HISTORY 0x3Cu /* i32[2400]: monthly population ring (0x004E9700) */
#define CITY_POPULATION_HISTORY_INDEX 0x25BCu /* i32: next index of the ring */
#define CITY_POPULATION_HISTORY_MONTHS 0x25C0u /* i32: months recorded */
#define CITY_AGES 0x25C4u            /* i16[100]: population by age (0x004E8650) */
#define CITY_POPULATION_BY_HOUSE 0x268Cu /* i32[20]: population per house level (0x004E8840 pass) */
#define CITY_BIRTHS 0x26DCu          /* i32: births at the yearly ageing */
#define CITY_DEATHS 0x26E0u          /* i32: deaths at the yearly ageing (death % table 0x005E49D4) */
#define CITY_U26E4 0x26E4u           /* i32: := 0 by the population month (0x004E87A0) */
#define CITY_IMMIGRANTS_DUE 0x26E8u  /* i32: immigrants to process (0x004E8F80 -> 0x00458540) */
#define CITY_EMIGRANTS_DUE 0x26ECu   /* i32: emigrants to process */
#define CITY_U26F0 0x26F0u           /* i32: carried into the emigrants, reset at 4 (core/city/day_labour.c) */
#define CITY_U26F4 0x26F4u           /* i32: carried into the immigrants, reset at 4 */
#define CITY_U26F8 0x26F8u           /* i32: lowered by the people removed (core/city/last_city.c) */
#define CITY_U26FC 0x26FCu           /* i32: := the population change (core/city/month_mood.c) */
#define CITY_POPULATION_AVERAGE 0x2700u /* i32: average yearly population (0x004E87A0) */
#define CITY_POPULATION_SUM 0x2704u  /* i32: its sum */
#define CITY_U2708 0x2708u           /* i32: population of houses of BLD_SUBTYPE <= 3 (0x00458F80) */
#define CITY_POPULATION_YEARS 0x2710u /* i32: its years */
#define CITY_AGEING_FLAG 0x2714u     /* i32: yearly-ageing flag */
#define CITY_U2718 0x2718u           /* i32: a house cursor (core/city/day_labour.c) */
#define CITY_U271C 0x271Cu           /* i16: a house cursor (core/city/day_labour.c) */
#define CITY_IMMIGRATED 0x2720u      /* i32: immigrated this cycle */
#define CITY_EMIGRATED 0x2724u       /* i32: emigrated this cycle */
#define CITY_U2728 0x2728u           /* i32: += the immigrants not placed this cycle (core/city/day_labour.c) */
#define CITY_MIGRATION_PCT 0x272Cu   /* i32: migration % from sentiment, -50..+100 */
#define CITY_U2734 0x2734u           /* i32: countdown, := 2 with a positive migration rate (core/city/day_labour.c) */
#define CITY_U2738 0x2738u           /* i32: countdown, := 2 with a negative migration rate */
#define CITY_IMMIGRANTS_THIS_MONTH 0x273Cu /* i32 */
#define CITY_IMMIGRANTS_LAST_MONTH 0x475Cu /* i32 */
#define CITY_POPULATION_HIGHEST 0x469Cu /* i32: highest population */

/* food, storage, trade (save.md §5 "Food, storage, trade") */
#define CITY_TRADE_STATUS 0x2826u    /* i16[36] per resource: trade status (0x004434E0) */
#define CITY_FOOD_TOTAL 0x296Cu      /* i32: total food */
#define CITY_FOOD_TYPES 0x2970u      /* i32: food types */
#define CITY_U2974 0x2974u          /* i32: 0 then 1 by 0x0045E420, tested in 0..1 by the prosperity rating */
#define CITY_U298C 0x298Cu           /* u8 per food 1..8: := 1 once in G_GOODS_LIST (0x005596D0) */
#define CITY_U2995 0x2995u           /* u8 per resource 1..35: := 1 once in G_GOODS_AVAILABLE (0x005596D0) */
#define CITY_U2B18 0x2B18u           /* i32: CITY_FOOD_TOTAL / CITY_U45AC, 12 without (core/city/day_buildings.c) */
#define CITY_U2B1C 0x2B1Cu           /* i32: a building count of the food pass (core/city/day_buildings.c) */
#define CITY_U3C64 0x3C64u           /* i32[27] per BLD_SUBTYPE slot: +0x3C used, summed (0x0045E6E0; save.md: stock, assumed) */
#define CITY_U3D04 0x3D04u           /* i32[27] per slot: capacity - used, summed (0x0045E6E0; save.md: capacity, assumed) */
#define CITY_U45AC 0x45ACu           /* i32: the divisor of CITY_U2B18 */
#define CITY_U45B0 0x45B0u           /* i32: a building count of the food pass */
#define CITY_U45B4 0x45B4u           /* i32: a building count of the food pass */
#define CITY_U45B8 0x45B8u           /* i32: a building count of the food pass */
#define CITY_FOOD_EATEN 0x45DCu      /* i32: food eaten this month */
#define CITY_U45E0 0x45E0u           /* i32: := CITY_U45E4 monthly */
#define CITY_U45E4 0x45E4u           /* i32: rolled into CITY_U45E0 monthly */
#define CITY_GRANARY_FOOD 0x483Cu    /* i32[9]: granary stock by food */
#define CITY_STORAGE_FREE 0x4860u    /* i32[36]: storage-yard free space per resource (0x0045AAE0) */
#define CITY_STORAGE_STOCK 0x48F0u   /* i32[36]: storage-yard stock per resource (0x0045AAE0) */

/* labour (save.md §5 "Labour"; the categories: CITY_LABOUR_NEEDED above) */
#define CITY_U2B20 0x2B20u           /* i32: calc_ratio_percent(low, high + low) of the labour pass (core/city/day_labour.c) */
#define CITY_WORKING_AGE 0x2B24u     /* i32: working-age population x 60 % */
#define CITY_LABOUR_FORCE 0x2B28u    /* i32: labour force */
#define CITY_LABOUR_WORKERS 0x2B34u  /* i32: the workers the category's buildings have (0x00459100), read by 0x004595A0 */
#define CITY_LABOUR_BUILDINGS 0x2B38u /* i32: the category's buildings (0x00459100) */
#define CITY_LABOUR_PRIORITY 0x2B3Cu /* i32: a category's priority (save.md: name assumed) */
#define CITY_EMPLOYED 0x2BF4u        /* i32: employed */
#define CITY_UNEMPLOYED 0x2BF8u      /* i32: unemployed */
#define CITY_U2C00 0x2C00u           /* i32: unemployment percent shown, moves 5 a step to 100 */
#define CITY_LABOUR_SHORTAGE 0x2C04u /* i32: shortage */

/* finance (save.md §5 "Finance": this year / last year, the year end 0x0045A5D0, the totals 0x0045A990) */
#define CITY_WAGES_ACCUMULATOR 0x2C14u /* i32 */
#define CITY_WAGES_THIS_YEAR 0x2C18u
#define CITY_WAGES_LAST_YEAR 0x2C1Cu
#define CITY_U2C20 0x2C20u           /* i32: taxed / untaxed population and tax due by class (save.md), cleared by 0x004E8840 */
#define CITY_U2C24 0x2C24u
#define CITY_U2C28 0x2C28u
#define CITY_U2C2C 0x2C2Cu
#define CITY_U2C30 0x2C30u           /* i32: % registered (save.md, 60:5) := ratio of CITY_U2C20 */
#define CITY_U2C34 0x2C34u           /* i32: the same of CITY_U2C24 */
#define CITY_U2C3C 0x2C3Cu           /* i32: tax accumulator, into CITY_TAXES_LAST_YEAR (0x00459E50) */
#define CITY_U2C40 0x2C40u           /* i32: tax accumulator, into CITY_TAXES_LAST_YEAR */
#define CITY_U2C44 0x2C44u           /* i32: tax accumulator */
#define CITY_U2C48 0x2C48u           /* i32: tax accumulator */
#define CITY_TAXES_THIS_YEAR 0x2C4Cu
#define CITY_TAXES_LAST_YEAR 0x2C50u
#define CITY_U2C54 0x2C54u           /* i32: tax due by class (save.md) */
#define CITY_U2C58 0x2C58u
#define CITY_U2C5C 0x2C5Cu
#define CITY_U2C60 0x2C60u
#define CITY_EXPORTS_THIS_YEAR 0x2C64u
#define CITY_EXPORTS_LAST_YEAR 0x2C68u
#define CITY_IMPORTS_THIS_YEAR 0x2C6Cu
#define CITY_IMPORTS_LAST_YEAR 0x2C70u
#define CITY_U2C74 0x2C74u           /* i32: interest accumulator, into CITY_INTEREST_LAST_YEAR (0x0045A500) */
#define CITY_INTEREST_LAST_YEAR 0x2C78u
#define CITY_INTEREST_THIS_YEAR 0x2C7Cu
#define CITY_THEFT_LAST_YEAR 0x2C80u
#define CITY_THEFT_THIS_YEAR 0x2C84u
#define CITY_CONSTRUCTION_LAST_YEAR 0x2C88u
#define CITY_CONSTRUCTION_THIS_YEAR 0x2C8Cu
#define CITY_SALARY_LAST_YEAR 0x2C90u
#define CITY_SALARY_THIS_YEAR 0x2C94u
#define CITY_SALARY 0x2C98u          /* i32: salary */
#define CITY_SALARY_RANK 0x2C9Cu     /* i32: salary rank */
#define CITY_U2CA0 0x2CA0u           /* i32: += CITY_SALARY monthly, into CITY_SALARY_LAST_YEAR (0x0045A530) */
#define CITY_INCOME_LAST_YEAR 0x2CA4u
#define CITY_INCOME_THIS_YEAR 0x2CA8u
#define CITY_EXPENSES_LAST_YEAR 0x2CACu
#define CITY_EXPENSES_THIS_YEAR 0x2CB0u
#define CITY_NET_LAST_YEAR 0x2CB4u
#define CITY_NET_THIS_YEAR 0x2CB8u
#define CITY_BALANCE_LAST_YEAR 0x2CBCu /* i32: := treasury at year end (0x0045A5D0) */
#define CITY_BALANCE_THIS_YEAR 0x2CC0u
#define CITY_PLAYER_RANK 0x444Cu     /* i32: player rank */
#define CITY_SAVINGS 0x4450u         /* i32: personal savings */
#define CITY_GIFTS_LAST_YEAR 0x4454u
#define CITY_GIFTS_THIS_YEAR 0x4458u
#define CITY_U4498 0x4498u           /* i32: := 1 once the debt loan is given (0x004460B0) */
#define CITY_U449C 0x449Cu           /* i32: := 1 once message 0x74 is posted (0x004460B0) */
#define CITY_U44DC 0x44DCu           /* i32: += CITY_WAGE_RATE monthly, into CITY_U45A8 */
#define CITY_TRIBUTE_THIS_YEAR 0x44E0u
#define CITY_TRIBUTE_LAST_YEAR 0x44E4u
#define CITY_U44E8 0x44E8u           /* i32: := 1 at the year end with no treasury (0x0045A5D0), tested by the kingdom rating */
#define CITY_U44EC 0x44ECu           /* i32: years in a row with CITY_U44E8 (0x0045A5D0) */
#define CITY_U45A8 0x45A8u           /* i32: := CITY_U44DC at year end */
#define CITY_REQUESTS_LAST_YEAR 0x4754u
#define CITY_REQUESTS_THIS_YEAR 0x4758u
#define CITY_GOLD_LAST_YEAR 0x47ACu  /* i32: mined gold */
#define CITY_GOLD_THIS_YEAR 0x47B0u
#define CITY_U482C 0x482Cu           /* i32: months in a row with a negative treasury (0x00452160) */
#define CITY_U4830 0x4830u           /* i32: its years */
#define CITY_MANSION_BUILT 0x499Cu   /* i32: mansion built */

/* ratings (save.md §5 "Ratings": the win test 0x004EA2C0) */
#define CITY_RATING_CULTURE 0x42CCu  /* i32 (0x004269B0) */
#define CITY_RATING_PROSPERITY 0x42D0u /* i32 (0x0046E1E0) */
#define CITY_RATING_MONUMENT 0x42D4u /* i32 (0x0046EF50) */
#define CITY_RATING_KINGDOM 0x42D8u  /* i32 (0x0046E970) */
#define CITY_U42DC 0x42DCu           /* i32: the value 0x0046E880 grades */
#define CITY_U42E4 0x42E4u           /* i32: 0..2, set with the prosperity rating */
#define CITY_U42FC 0x42FCu           /* i32: := 0 by the culture rating */
#define CITY_U4300 0x4300u           /* i32: := 0 by the culture rating */
#define CITY_U442C 0x442Cu           /* i32: tested by 0x0046E880 */
#define CITY_RATING_CULTURE_REASON 0x4434u /* i32: explanation id (save.md: +0x4434..+0x4440), set with CITY_RATING_CULTURE */
#define CITY_RATING_PROSPERITY_REASON 0x4438u /* i32: set by the prosperity rating */
#define CITY_RATING_MONUMENT_REASON 0x443Cu /* i32: := 0 by 0x0046EF50 */
#define CITY_RATING_KINGDOM_REASON 0x4440u /* i32: set by 0x0046EC70 */
#define CITY_U4444 0x4444u           /* i32: set by 0x0046E880 */
#define CITY_U4548 0x4548u           /* i32: := 0 by the culture rating */
#define CITY_U454C 0x454Cu
#define CITY_U4550 0x4550u
#define CITY_U4554 0x4554u
#define CITY_U455C 0x455Cu           /* i32: months, wraps at 12 (the kingdom rating) */
#define CITY_U4560 0x4560u           /* i32: := 0 when CITY_U455C wraps */
#define CITY_U4580 0x4580u           /* i32: CITY_SALARY_RANK - CITY_PLAYER_RANK (0x0046EC70) */
#define CITY_U4584 0x4584u           /* i32: tested by 0x0046EC70 */
#define CITY_U4588 0x4588u           /* i32: tested by 0x0046EC70, set by events */
#define CITY_U458C 0x458Cu           /* i32: CITY_RATING_KINGDOM at the previous pass */
#define CITY_U4590 0x4590u           /* i32: 0..2 by the kingdom rating's change */
#define CITY_U4634 0x4634u           /* i32: tested by 0x0046E880 and the goals */
#define CITY_U4644 0x4644u           /* i32: tested by 0x0046E880 and the goals */
#define CITY_U4658 0x4658u           /* i32: := 0 by the culture rating */
#define CITY_U4678 0x4678u           /* i32: compared by the goals (0x004EA2C0) */
#define CITY_PROSPERITY_CAP 0x46A8u  /* i32: prosperity cap */

/* services, religion, festivals (save.md §5 "Services", "Religion and festivals") */
#define CITY_U4360 0x4360u           /* i32, 8 bytes apart (3): += 1 per venue of type 0x1F, 0x1E, 0x21 with part k (+0xA7) (0x004550E0; save.md: entertainment coverage) */
#define CITY_U4364 0x4364u           /* i32, 8 bytes apart (3): += the venue's part count when part k is missing */
#define CITY_U4378 0x4378u           /* i32: += 1 per venue of type 0x20 with +0xA7 */
#define CITY_U437C 0x437Cu           /* i32: += 100 per venue of type 0x20 without +0xA7 */
#define CITY_U4380 0x4380u           /* i32: 0..5, which of the counts above is largest (0x004550E0) */
#define CITY_U4384 0x4384u           /* i32: average house +0x80 (0x004550E0) */
#define CITY_U438C 0x438Cu           /* i32: := 1 by core/city/month.c */
#define CITY_U4408 0x4408u           /* i32: a god (core/city/day_city.c) */
#define CITY_U44C8 0x44C8u           /* i32: += 1 per venue of type 0xE2 with +0xA7 (0x004550E0) */
#define CITY_U44CC 0x44CCu           /* i32: += 100 per venue of type 0xE2 without +0xA7 */
#define CITY_U44F4 0x44F4u           /* i32: := 2 from 3 when CITY_U4514 is set */
#define CITY_FESTIVAL_COST_1 0x4504u /* i32: festival cost (save.md: +0x4504..+0x450C), population / 20 + 10 */
#define CITY_FESTIVAL_COST_2 0x4508u /* i32: population / 10 + 20 */
#define CITY_FESTIVAL_COST_3 0x450Cu /* i32: population / 5 + 40 */
#define CITY_U4510 0x4510u           /* i32: (population / 500 + 1) * 100 */
#define CITY_U4514 0x4514u           /* i32: 1 when CITY_STORAGE_STOCK of resource 15 is below CITY_U4510 */
#define CITY_U4518 0x4518u           /* i32: average house +0x83 (0x004550E0) */
#define CITY_U451C 0x451Cu           /* i32: average house +0x81 */
#define CITY_U4520 0x4520u           /* i32: average house +0x82 */
#define CITY_U452C 0x452Cu           /* i32: countdown (core/city/month.c) */
#define CITY_U45D8 0x45D8u           /* (core/city/day_city.c) */
#define CITY_HOUSES_DISEASED 0x4750u /* i32: houses at disease level 1000 (0x004E81D0) */

/* the city's buildings and the rest */
#define CITY_U4460 0x4460u           /* i16 per building found (core/city/day_buildings.c) */
#define CITY_U447C 0x447Cu           /* i16: += 1 per land trading city (0x00444C20) */
#define CITY_U447E 0x447Eu           /* i16: += 1 per sea trading city */
#define CITY_U4480 0x4480u           /* i16: countdown, := 0x30 by events */
#define CITY_U4482 0x4482u           /* i16: countdown, := 0x30 by events */
#define CITY_U4488 0x4488u           /* i16: a building count (core/city/day_buildings.c) */
#define CITY_U448C 0x448Cu           /* i16: rolled into CITY_U448E monthly (0x0045A560) */
#define CITY_U448E 0x448Eu           /* i16 */
#define CITY_RECRUITER 0x44ACu       /* i32: recruiter building (save.md: +0x44A4..+0x44AC), type 0x5F */
#define CITY_U44D0 0x44D0u           /* i32: tested with CITY_U44D4 by the goals */
#define CITY_U44D4 0x44D4u           /* i32: countdown */
#define CITY_U4630 0x4630u           /* i32: a building count (core/city/day_buildings.c) */
#define CITY_U46A4 0x46A4u           /* i32: the kinds counted by 0x00559FC0 (0x00444C20) */
#define CITY_U4714 0x4714u           /* i32: := 1 once by the goals (0x004EA2C0) */
#define CITY_U4718 0x4718u           /* i32: := 1 once by the won path of 0x004EA2C0 */
#define CITY_MONUMENT_TYPES 0x4730u  /* i32[3]: the city's monuments' types (0x004F78A0) */
#define CITY_MONUMENTS 0x473Cu       /* i32[3]: the city's monument buildings (0x004F78A0, 0x004E59C0) */
#define CITY_U476C 0x476Cu           /* i32 (0x004474E0) */
#define CITY_U4770 0x4770u           /* i32 (0x004474E0) */
#define CITY_U4774 0x4774u           /* i32 (0x004474E0) */
#define CITY_U4778 0x4778u           /* i32 (0x004474E0) */
#define CITY_U477C 0x477Cu           /* i32 (0x004474E0) */
#define CITY_U4780 0x4780u           /* i32 (0x004474E0) */
#define CITY_U4784 0x4784u           /* i32: := 0 by 0x0045A560 */
#define CITY_U47A8 0x47A8u           /* i32: extends the goals' year limit, 0: 7 years, 1: 2 (0x004EA2C0; save.md: difficulty copy) */
#define CITY_U4980 0x4980u           /* i32: += 1 (core/city/month_mood.c) */
#define CITY_U49A0 0x49A0u           /* i32: += 1 (core/city/events_troops.c) */

#endif
