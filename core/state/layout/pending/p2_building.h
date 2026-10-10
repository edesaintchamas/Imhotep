/* Imhotep - pending names of package p2 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P2_BUILDING_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P2_BUILDING_H

/* city record fields (layout/city.h, P3) */
#define CITY_U05 0x05u /* @bin 0x0046A400 u8: copied into a new building's BLD_U02 (save.md §5) */
#define CITY_U2790 0x2790u /* @bin 0x00463FB7 i32: a building whose workers let types 0x56, 0x57 send walker 7 */
#define CITY_U28C8 0x28C8u /* @bin 0x00462763 i16: 0, a type 0x66 building sends a second cart (good 9) */
#define CITY_U2826 0x2826u /* @bin 0x00457210 i16 per good: the city's trade setting, bazaars take goods set to 1 or 3 */
#define CITY_U2A88 0x2A88u /* @bin 0x004571AA i32 per good: non-zero, the good is not traded (core/building/market.c) */
#define CITY_U25C4 0x25C4u /* @bin 0x004E919A i16[100]: summed into CITY_POPULATION */
#define CITY_U26E4 0x26E4u /* @bin 0x004E9170 i32: += people removed */
#define CITY_U26F8 0x26F8u /* @bin 0x004E9250 i32: += people removed */
#define CITY_U4414 0x4414u /* @bin 0x00468B70 i32: := 0 by the day's risks */
#define CITY_U4418 0x4418u /* @bin 0x00468B70 i32: := 0 by the day's risks */
#define CITY_U3D04 0x3D04u /* @bin 0x0045BF00 i32 per workshop need index: > 0, the need counts */
#define CITY_U44AC 0x44ACu /* @bin 0x0045BF00 i32: a building whose stocks (goods 10, 0x1C) storage carts fill */
#define CITY_U4460 0x4460u /* @bin 0x004808A0 i16[10]: the city's docks */
#define CITY_U483C 0x483Cu /* @bin 0x0045E360 i32 per good: lowered by what a building's stock gives up */
#define CITY_U4860 0x4860u /* @bin 0x0045B700 i32 per good: raised by what a storage yard gives up */
#define CITY_U48F0 0x48F0u /* @bin 0x0045B700 i32 per good: lowered by it */
#define CITY_U459C 0x459Cu /* @bin 0x004661A6 i32: := 1 by type 0x50; > 0 lets type 0x59 send walker 0x3A */
#define CITY_U4774 0x4774u /* @bin 0x004651F0 i32: += 100 a cart of type 0x73 */
#define CITY_U4780 0x4780u /* @bin 0x004664FC i32: += 100 a cart of type 0x4C */
#define CITY_U4990 0x4990u /* @bin 0x004C3420 i32: 10 for a complex of level 4, else 0 */
#define CITY_U4730 0x4730u /* @bin 0x004F4F80 i32[3]: the scenario slots taken (read from player 1's record) */
#define CITY_U473C 0x473Cu /* @bin 0x004F4A60 i32[3]: the monument building of each scenario slot */
#define CITY_U469C 0x469Cu /* @bin 0x004E919A i32: the highest CITY_POPULATION */

/* city fields that 0x0046AC50 (building_release) resets when a building of a type goes */
#define CITY_U2788 0x2788u /* @bin 0x0046AC50 i16: types 0xBB-0xBD: x (tile CITY_U278C) */
#define CITY_U278A 0x278Au /* i16: y */
#define CITY_U278C 0x278Cu /* i32: the building's tile */
#define CITY_U4486 0x4486u /* i16: := 0 with them */
#define CITY_U448A 0x448Au /* @bin 0x0046AC50 i16: types 0x41-0x45: := 0 */
#define CITY_U44A4 0x44A4u /* @bin 0x0046AC50 i16: type 0x5F: x (tile CITY_U44A8) */
#define CITY_U44A6 0x44A6u /* i16: y */
#define CITY_U44A8 0x44A8u /* i32: the building's tile */
#define CITY_U44B0 0x44B0u /* i32: := 0 with them */
#define CITY_U45F4 0x45F4u /* @bin 0x0046AC50 i16: type 0x32: x (tile CITY_U45F8) */
#define CITY_U45F6 0x45F6u /* i16: y */
#define CITY_U45F8 0x45F8u /* i32: the building's tile */
#define CITY_U4600 0x4600u /* i32: := 0 with them */
#define CITY_U47A0 0x47A0u /* @bin 0x0046AC50 i32: type 0xD1: := -1 */
#define CITY_U47A4 0x47A4u /* i32: type 0xD1: := -1 */
#define CITY_U4814 0x4814u /* @bin 0x0046AC50, 0x0043FD00 i16: type 0xD1: := 0 */
#define CITY_U499C 0x499Cu /* @bin 0x0046AC50 i32: types 0x4D-0x4F: := 0 */
#define CITY_U49D0 0x49D0u /* @bin 0x0046AC50 i32: type 0x5E: := 0 */
#define CITY_U49D4 0x49D4u /* @bin 0x0046AC50 i32: type 0xD2: := CITY_U433A */

/* city counters of the houses (@bin 0x00454D70 clears them daily; 0x00453B40 counts the needs) */
#define CITY_U42A4 0x42A4u /* i32: cleared daily */
#define CITY_U42AC 0x42ACu /* i32: cleared daily */
#define CITY_U42B4 0x42B4u /* i32: cleared daily */
#define CITY_U42BC 0x42BCu /* i32: cleared daily */
#define CITY_U4304 0x4304u /* i32: BLD_WATER_SUPPLY 0 where the level needs 2 */
#define CITY_U4308 0x4308u /* i32: no water, no BLD_WELL_ACCESS where the level needs 1 */
#define CITY_U430C 0x430Cu /* i32: BLD_ENTERTAINMENT below the need */
#define CITY_U4310 0x4310u /* i32: BLD_EDUCATION below the need */
#define CITY_U4314 0x4314u /* i32: BLD_EDUCATION 0 below the need */
#define CITY_U4318 0x4318u /* i32: the level record +0xC is 1 or 2 */
#define CITY_U431C 0x431Cu /* i32: the level record +0xC is 2 */
#define CITY_U4320 0x4320u /* i32: cleared daily */
#define CITY_U4324 0x4324u /* i32: BLD_ACCESS_DENTIST below the need */
#define CITY_U4328 0x4328u /* i32: BLD_ACCESS_MAGISTRATE below the need */
#define CITY_U432C 0x432Cu /* i32: too few kinds of goods held */
#define CITY_U433C 0x433Cu /* i32: BLD_HEALTH below a need of 1 */
#define CITY_U4340 0x4340u /* i32: BLD_HEALTH below a need of 2 or more */
#define CITY_U4344 0x4344u /* i32: the BLD_ACCESS_DENTIST need is 1 */
#define CITY_U4348 0x4348u /* i32: the magistrate need is 1 or 2 */
#define CITY_U434C 0x434Cu /* i32: a BLD_HEALTH need met */
#define CITY_U4350 0x4350u /* i32: BLD_RELIGION below a need of 1 */
#define CITY_U4354 0x4354u /* i32: BLD_RELIGION below a need of 2 */
#define CITY_U4358 0x4358u /* i32: BLD_RELIGION below a need of 3 */
#define CITY_U435C 0x435Cu /* i32: a BLD_RELIGION need met */
#define CITY_U4388 0x4388u /* i32: BLD_ENTERTAINMENT 0 below the need */
#define CITY_U46A4 0x46A4u /* i32: below 2, a house needing 2 or more of the third good does not evolve */
#define CITY_U46FC 0x46FCu /* i32: += 1 then */
#define CITY_U4748 0x4748u /* i32: the magistrate need is 2 */
#define CITY_U474C 0x474Cu /* i32: BLD_ACCESS_MAGISTRATE 0 below the need */

/* globals of the city area (g_city.h, P3) */
#define G_RAMP_WALK_TILE 0x00EAAE90u /* @bin 0x004ED360 i32: the tile the walk found (core/city/events_actions.c) */
#define G_RAMP_WALK_TYPE 0x00EAAE8Cu /* @bin 0x004ED360 i32: the building type to place there (0xDA .. 0xDC) */
#define G_COVERAGE 0x0078D5F0u /* @bin 0x00468170 i32[5]: the advisors' coverage percentages (core/city/day_city.c); 0x00454FA0 averages them */

/* a formation's fields (FORMATION_ADDR, core/figure/formation.h; 0x90 bytes) */
#define FORM_IN_USE 0x00u      /* @bin 0x004B82E0 u8 */
#define FORM_OWNER 0x01u       /* @bin 0x004B6E10 u8: := G_PLAYER */
#define FORM_KIND 0x02u        /* @bin 0x004B6E10 u8: 0..7, the first not in use */
#define FORM_U03 0x03u         /* @bin 0x004B76B0 u8: 0, no change of size; 0x004B6E10 := 1 */
#define FORM_FIGURE_TYPE 0x04u /* @bin 0x004B70C0 i16: its recruits' figure type */
#define FORM_FORT 0x06u        /* @bin 0x004B70C0 i16: its fort building */
#define FORM_FIGURES 0x08u     /* @bin 0x004B76B0 i16[16]: its figures */
#define FORM_COUNT 0x28u       /* @bin 0x004B76B0 u8: the figures it has (0x004B7353: the training average) */
#define FORM_MAX 0x29u         /* @bin 0x004B76B0 u8: the figures it may have */
#define FORM_U2A 0x2Au         /* @bin 0x004B6E10 i16: := 1 */
#define FORM_U2C 0x2Cu         /* @bin 0x004B6E10 i16: := 0x32 */
#define FORM_X 0x2Eu           /* @bin 0x004B6E10 i16: the fort's x + 3 (0x00472790: the formation's tile) */
#define FORM_Y 0x30u           /* i16: its y - 1 */
#define FORM_U32 0x32u         /* @bin 0x004B6E10 i16: := FORM_X */
#define FORM_U34 0x34u         /* i16: := FORM_Y */
#define FORM_U36 0x36u         /* i16: := FORM_X */
#define FORM_U38 0x38u         /* i16: := FORM_Y */
#define FORM_FIGURE 0x40u      /* @bin 0x004B82E0 i16: deleted with the formation */
#define FORM_PLAYER 0x42u      /* @bin 0x004B70C0 u8: the player's (FM_PLAYER, core/figure/formation.c) */
#define FORM_WANT 0x46u        /* @bin 0x004B70C0 i16: how much it wants a recruit (3, 2: the stock needed) */
#define FORM_U77 0x77u         /* @bin 0x004B6E10 u8: := 0 */
#define FORM_TRAINING 0x7Au    /* @bin 0x004B7353 i16: training, averaged with each recruit's, at most 100 */
#define FORM_U64 0x64u         /* @bin 0x004B76B0 i16: non-zero, no change of size */
#define FORM_U7E 0x7Eu         /* @bin 0x004B6E10 i16: := 0 */
#define FORM_U80 0x80u         /* @bin 0x004B6E10 i16: := 0 */
#define FORM_U82 0x82u         /* @bin 0x004B70C0 i16: >= 0, no recruit */
#define FORM_U84 0x84u         /* @bin 0x004B70C0 i32: non-zero, no recruit */
/* the military (g_figure.h, P1) */
#define G_FORMATION_HIGHEST 0x00BCB9BCu /* @bin 0x004B6E10 i32: the highest formation id in use (save.md chunk 25) */

/* an animation object's fields (core/city/day_anim.c, P3) */
#define ANIMOBJ_WORK 0x0Cu   /* @bin 0x004201FF i32: the work left (core/building/monument.c) */
#define ANIMOBJ_MODE 0x10u   /* @bin 0x0041FF60 i16: 2, 3 stopped; 1 clamp at the ends */
#define ANIMOBJ_FIGURE 0x12u /* @bin 0x0041FF60 i16: the figure it follows; 0x004233F0 tests it */
#define ANIMOBJ_FIGURE_ACTION 0x14u /* @bin 0x0041FD80 u8: that figure's action */

/* messages (g_message.h, P3): a collapse (message 0x70) and a fire (0x6F) of 0x00468B70 */
#define G_COLLAPSE_MESSAGE_TIME 0x00E950ECu /* @bin 0x00468D75 i32: G_FRAME_CLOCK at the last one (15000 apart) */
#define G_FIRE_MESSAGE_TIME 0x00E950F0u     /* @bin 0x00468FB5 i32 */
#define G_MESSAGE_DELAYS 0x00E95124u        /* i32[20]: per-category message delays (save.md chunk 83); [1] fires, [2] collapses: a pop-up when <= 0, then := 12 */
#define G_MESSAGE_COUNTERS 0x00E95174u      /* i32[20]: per-category message counters (chunk 82); [1] fires, [2] collapses */

/* the flood (P3) */
#define G_FLOOD_STATE 0x00D386D4u /* @bin 0x00488400 i32: 1, floodplain farms harvest */

/* map grids (g_map.h, P5): save.md §3 */
#define G_GRID_DAMAGE 0x00A9CC1Cu      /* @bin 0x004690F0 i16 per tile: damage counter (chunk 18) */

/* globals of the construction (g_command.h, P4) */
#define G_UBC972C 0x00BC972Cu /* @bin 0x00477A40 i32: := 1 when a tile's terrain bits change */
#define G_UBC9730 0x00BC9730u /* @bin 0x00477A40 i32: := 1 when they already were the mask */

/* named in present/layout/g_city_view.h (P9), used by core: the same definition, to move to a core header */
#define G_GATE_PARTS 0x005DF810u /* @bin 0x004BFA50 the gatehouse's part records, 16 bytes: +0 u8 flag, +4 size, +8, +0xC offsets */

/* the construction (g_command.h, P4) */
#define G_BUILD_TURN 0x00BC96D8u      /* @bin 0x0047D060 i32: a dock's turn */
#define G_BUILD_TURN_VIEW 0x00BC96DCu /* @bin 0x0047D060 i32: the turn against the view; 0x004555D0 sets it */

/* globals whose area is not established */
#define G_U78D828 0x0078D828u /* @bin 0x00465B86 i32: := 0 before tutorial step 0xC; save.md chunk 100 */
#define G_U77567C 0x0077567Cu /* @bin 0x0043FD00 i32: rubble tiles redrawn (0x0047A120) while set; := 0 after */
#define G_MAP_REFRESH_PENDING 0x00BCA30Cu /* @bin 0x00486E70 u8: the map is refreshed when set (type 0xB7 removed, 0x004525F0) */
#define G_U978D70 0x00978D70u /* @bin 0x00468B70 i32: 1 no risks; 0x004539A0: houses only redrawn */

#endif
