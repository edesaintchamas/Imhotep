/* Imhotep - pending names of package p8c (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P8C_UI_ADVISORS_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P8C_UI_ADVISORS_H

/* ---- city record fields (owner: city.h, P3). Evidence: docs/formats/save.md §5 "City data" (the
 * finance names: "text group 60 in 0x0052A250", the page that shows them, present/ui/advisors/f_pages.c)
 * and the present/ui/advisors ports named on each line; CITY_U<offset> where the meaning is not
 * established. Notes for the merge: the advisor ports also read CITY_U25BC / CITY_U25C0 (p6) as the
 * population history's index and months (save.md, 0x004E9760), CITY_U4434.. as the rating
 * explanations, CITY_GIFT_COST + 4 / + 8 where p6 has CITY_U4578 / CITY_U457C. ---- */
#define CITY_POPULATION_HISTORY 0x3Cu     /* @bin 0x004E9760 i32[2400]: monthly population history (save.md) */
#define CITY_POPULATION_BY_HOUSE 0x268Cu  /* @bin 0x005278E0 i32[20]: population per house type - 10 (save.md; page 7's graph) */
#define CITY_FOOD_TYPES 0x2970u           /* @bin 0x00527BF0 i32: food types (save.md; page 8) */
#define CITY_U2B18 0x2B18u                /* @bin 0x00526705 i32: > 0 read by pages 7 and 13 and window 0x4F (the immigration line) */
#define CITY_U2B1C 0x2B1Cu                /* @bin 0x00526705 i32: counted on page 7's immigration line (string 8/6) */
#define CITY_UNEMPLOYED 0x2BF8u           /* @bin 0x00521235 i32: unemployed (save.md; page 1) */
#define CITY_WAGES_LAST_YEAR 0x2C1Cu      /* @bin 0x0052A250 i32: wages last year (save.md) */
#define CITY_U2C44 0x2C44u                /* @bin 0x0052A638 i32: summed with CITY_U2C48 on page 12 and by 0x00538050 */
#define CITY_U2C48 0x2C48u                /* @bin 0x0052A638 i32 */
#define CITY_TAXES_LAST_YEAR 0x2C50u      /* @bin 0x0052A250 i32: taxes last year (save.md) */
#define CITY_EXPORTS_THIS_YEAR 0x2C64u    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_EXPORTS_LAST_YEAR 0x2C68u    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_IMPORTS_THIS_YEAR 0x2C6Cu    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_IMPORTS_LAST_YEAR 0x2C70u    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_INTEREST_LAST_YEAR 0x2C78u   /* @bin 0x0052A250 i32 (save.md) */
#define CITY_THEFT_LAST_YEAR 0x2C80u      /* @bin 0x0052A250 i32 (save.md) */
#define CITY_THEFT_THIS_YEAR 0x2C84u      /* @bin 0x0052A250 i32 (save.md) */
#define CITY_CONSTRUCTION_LAST_YEAR 0x2C88u /* @bin 0x0052A250 i32 (save.md) */
#define CITY_SALARY_LAST_YEAR 0x2C90u     /* @bin 0x0052A250 i32 (save.md) */
#define CITY_INCOME_LAST_YEAR 0x2CA4u     /* @bin 0x0052A250 i32 (save.md) */
#define CITY_INCOME_THIS_YEAR 0x2CA8u     /* @bin 0x0052A250 i32 (save.md) */
#define CITY_EXPENSES_LAST_YEAR 0x2CACu   /* @bin 0x0052A250 i32 (save.md) */
#define CITY_EXPENSES_THIS_YEAR 0x2CB0u   /* @bin 0x0052A250 i32 (save.md) */
#define CITY_NET_LAST_YEAR 0x2CB4u        /* @bin 0x0052A250 i32 (save.md) */
#define CITY_NET_THIS_YEAR 0x2CB8u        /* @bin 0x0052A250 i32 (save.md) */
#define CITY_BALANCE_THIS_YEAR 0x2CC0u    /* @bin 0x0052A250 i32 (save.md; CITY_BALANCE_LAST_YEAR the other column) */
#define CITY_RATING_CULTURE 0x42CCu       /* @bin 0x004F6E51 i32: culture rating (save.md; 0x004269B0) */
#define CITY_RATING_PROSPERITY 0x42D0u    /* @bin 0x004F6E51 i32: prosperity rating (save.md; 0x0046E1E0) */
#define CITY_RATING_MONUMENT 0x42D4u      /* @bin 0x004F6E51 i32: monument rating (save.md; 0x0046EF50) */
#define CITY_U42E4 0x42E4u                /* @bin 0x004F7519 i32: + 0x14, a second prosperity explanation line */
#define CITY_U4318 0x4318u                /* @bin 0x0052886A i32: page 9's advice */
#define CITY_U431C 0x431Cu                /* @bin 0x0052886A i32: page 9's advice */
#define CITY_U4344 0x4344u                /* @bin 0x005282A5 i32: page 8's advice */
#define CITY_U4348 0x4348u                /* @bin 0x005282A5 i32: page 8's advice */
#define CITY_U434C 0x434Cu                /* @bin 0x005282A5 i32: page 8's advice */
#define CITY_U435C 0x435Cu                /* @bin 0x0052A18E i32: != 0 picks religion advice 10 / god's (house evolution counters, save.md) */
#define CITY_U4360 0x4360u                /* @bin 0x00528B90 i32: page 10's row 1 figure (entertainment coverage, save.md) */
#define CITY_U4368 0x4368u                /* @bin 0x00528B90 i32: row 2 */
#define CITY_U4370 0x4370u                /* @bin 0x00528B90 i32: row 3 */
#define CITY_U4378 0x4378u                /* @bin 0x00528B90 i32: row 4 */
#define CITY_U4380 0x4380u                /* @bin 0x00528E48 i32: page 10's advice, 0..3 */
#define CITY_U4384 0x4384u                /* @bin 0x00528E48 i32: page 10's advice */
#define CITY_U4408 0x4408u                /* @bin 0x0052A18E i32: a god (1..4) whose wrath byte picks page 11's advice */
#define CITY_RATING_SELECTED 0x4430u      /* @bin 0x004F7519 i32: the rating explained on page 5 (0 none, 1-4; := by 0x0040ACE0) */
#define CITY_PROSPERITY_EXPLANATION 0x4438u /* @bin 0x004F7519 i32: rating 2's explanation (save.md +0x4434..+0x4440) */
#define CITY_MONUMENT_EXPLANATION 0x443Cu /* @bin 0x004F7519 i32: rating 3's explanation */
#define CITY_KINGDOM_EXPLANATION 0x4440u  /* @bin 0x004F7519 i32: rating 4's explanation */
#define CITY_GIFTS_LAST_YEAR 0x4454u      /* @bin 0x0052A250 i32 (save.md) */
#define CITY_TRIBUTE_THIS_YEAR 0x44E0u    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_TRIBUTE_LAST_YEAR 0x44E4u    /* @bin 0x0052A250 i32 (save.md) */
#define CITY_U466C 0x466Cu                /* @bin 0x00521E9B u8: counted on page 2 */
#define CITY_U466D 0x466Du                /* @bin 0x0040AE10 u8: := 0 by a request row */
#define CITY_U466F 0x466Fu                /* @bin 0x00521E9B u8: counted on page 2 */
#define CITY_U4678 0x4678u                /* @bin 0x00538050 i32: >= 0 picks window 0x4F's military text */
#define CITY_U4748 0x4748u                /* @bin 0x005282A5 i32: page 8's advice (house evolution counters, save.md) */
#define CITY_U4750 0x4750u                /* @bin 0x00528153 i32: page 8's advice (save.md: houses at disease level 1000) */
#define CITY_REQUESTS_LAST_YEAR 0x4754u   /* @bin 0x0052A250 i32 (save.md) */
#define CITY_EXPLAIN_PAGE 0x4768u         /* @bin 0x00538050 i32: the advisor page window 0x4F offers */
#define CITY_GOLD_LAST_YEAR 0x47ACu       /* @bin 0x0052A250 i32: mined gold (save.md) */
#define CITY_GOLD_THIS_YEAR 0x47B0u       /* @bin 0x0052A250 i32 (save.md) */
#define CITY_TRADE_SCROLL 0x4834u         /* @bin 0x0055A0E0 i32: page 6's list scroll */
#define CITY_U4838 0x4838u                /* @bin 0x0055AD30 i32: := 1 by window 0x28 */

/* ---- globals of other areas ---- */
/* scenario goals (core/city/day_goals.c 0x004EA2C0, month_ratings.c; owner g_city.h / g_scenario.h) */
#define G_U784B60 0x00784B60u             /* @bin 0x004EA2C0 i16: != 0: no goals checked (page 5 hides them) */
#define G_GOAL_CULTURE 0x00784C24u        /* @bin 0x004EA2C0 i32: the culture rating goal (CITY_RATING_CULTURE) */
#define G_GOAL_PROSPERITY 0x00784C28u     /* @bin 0x004EA2C0 i32: the prosperity rating goal */
#define G_GOAL_KINGDOM 0x00784C30u        /* @bin 0x004EA2C0 i32: the kingdom rating goal */
#define G_GOAL_HOUSES 0x00784C34u         /* @bin 0x004EA2C0 i32: houses of level G_U784C38 at least this, 0: no goal */
#define G_GOAL_CULTURE_ON 0x00784C3Cu     /* @bin 0x004EA2C0 u8: the culture goal is set */
#define G_GOAL_PROSPERITY_ON 0x00784C3Du  /* @bin 0x004EA2C0 u8 */
#define G_GOAL_MONUMENT_ON 0x00784C3Eu    /* @bin 0x004EA2C0 u8 */
#define G_GOAL_KINGDOM_ON 0x00784C3Fu     /* @bin 0x004EA2C0 u8 */
#define G_GOAL_POPULATION_ON 0x00784C58u  /* @bin 0x004EA2C0 i32: the population goal is set */
#define G_GOAL_POPULATION 0x00784C5Cu     /* @bin 0x004EA2C0 i32: the population goal */
#define G_INTEREST_RATE 0x00784DDCu       /* @bin 0x0045A350 i32: the interest rate on a negative treasury (core/city/month.c); page 12 0x0052A250 */
#define G_U784F0C 0x00784F0Cu             /* @bin 0x00447840 i32: string of group 0x98 joined to the player name (0: the name alone) */
#define G_COVERAGE 0x0078D5F0u            /* @bin 0x00468170 i32[]: the advisors' coverage percentages (core/city/day_city.c) */
#define G_U7027C6 0x007027C6u             /* @bin 0x00409FA0 u8: 0 (with the mission G_U579638) shows tutorial step 0x0E */
#define G_U579638 0x00579638u             /* @bin 0x00409FA0 i32: a mission number (4) compared with G_MISSION */
#define G_FLOOD_MONTH 0x00D386D0u         /* @bin 0x0052D483 i32: the flood's month (present/ui/info/queries.c) */
/* the population graph's dates, written by population_history_dates 0x004E97C0 */
#define G_GRAPH_LAST_YEAR 0x00EA1800u     /* @bin 0x004E97C0 i32 */
#define G_GRAPH_LAST_MONTH 0x00EA1804u    /* @bin 0x004E97C0 i32 */
#define G_GRAPH_FIRST_YEAR 0x00EA1808u    /* @bin 0x004E97C0 i32 */
#define G_GRAPH_FIRST_MONTH 0x00EA180Cu   /* @bin 0x004E97C0 i32 */
/* images shared with core/figure, present/draw */
#define G_LEGION_BANNERS 0x00F56814u      /* @bin 0x00522565 i32: first image of the legions' banners (+ kind; core/figure/actions_b1.c, present/ui/info/military.c) */
#define G_WARSHIP_IMAGE 0x00F56694u       /* @bin 0x00522E10 i32: the warship rows' image (core/figure/actions_b1.c) */
#define G_INNER_PANEL_IMAGES 0x00F56844u  /* @bin 0x00536A30 i32: first image of the inner panel group (present/draw/panel.c; the strip 0x00536BA0) */

/* ---- functions (owner: functions.h, generated). tools/gen/gen_functions.py reads the port tables of
 * .c files only; these ports are rows of present/ui/advisors/X_ports.inc. Named as gen_functions.py
 * names them: drop when the generator reads .inc tables. ---- */
#define FN_ADVISOR_RATINGS_DRAW 0x004F6AF0u   /* b_ports.inc */
#define FN_ADVISOR_LABOUR_DRAW 0x00521080u    /* a_ports.inc */
#define FN_ADVISOR_MILITARY_DRAW 0x00521B50u  /* a_ports.inc */
#define FN_ADVISOR_FLEET_DRAW 0x00522E10u     /* a_ports.inc */
#define FN_ADVISOR_KINGDOM_DRAW 0x00523EA0u   /* e_ports.inc */
#define FN_ADVISOR_POPULATION_DRAW 0x00525CB0u /* b_ports.inc */
#define FN_ADVISOR_DRAW_PAGE8 0x00527BF0u     /* c_ports.inc */
#define FN_ADVISOR_DRAW_PAGE9 0x00528360u     /* c_ports.inc */
#define FN_ADVISOR_DRAW_PAGE10 0x00528950u    /* c_ports.inc */
#define FN_ADVISOR_DRAW_FESTIVAL_PANEL 0x00528F10u /* c_ports.inc */
#define FN_ADVISOR_RELIGION_DRAW 0x005298E0u  /* e_ports.inc */
#define FN_ADVISORS_DRAW_PAGE12 0x0052A250u   /* f_ports.inc */
#define FN_ADVISOR_MONUMENTS_DRAW 0x0052B520u /* e_ports.inc */
#define FN_ADVISORS_DRAW_PAGE13 0x0052C570u   /* f_ports.inc */
#define FN_TRADE_ADVISOR_DRAW 0x00559E10u     /* d_ports.inc */

#endif
