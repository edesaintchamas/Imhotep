/* Imhotep - named globals: figures (the record array: state/records.h, its fields: layout/figure.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_FIGURE_H
#define IMHOTEP_STATE_LAYOUT_G_FIGURE_H

/* @bin 0x00461B20 the name counters (chunks 54-74, save.md §4): i32 index of the next name in the
 * text group named after it (FIG_NAME_GROUP, FIG_NAME_INDEX), cycling at its limit */
#define G_NAME_COUNTER_10F 0x00D0569Cu
#define G_NAME_COUNTER_10A 0x00D056B0u /* groups 0x10A .. 0x10E: this one and the four below it */
#define G_NAME_COUNTER_109 0x00D056B4u
#define G_NAME_COUNTER_108 0x00D056B8u
#define G_NAME_COUNTER_107 0x00D056BCu
#define G_NAME_COUNTER_101 0x00D056D4u

/* @bin 0x004B5AF0 the other name counters (chunks 54-74, 101-104, 150-165, save.md section 3), by
 * text group: the groups the constructor's jump table passes with them (figure_create.c) */
#define G_NAME_COUNTER_12F 0x00D05640u
#define G_NAME_COUNTER_12E 0x00D05644u
#define G_NAME_COUNTER_12D 0x00D05648u
#define G_NAME_COUNTER_12C 0x00D0564Cu
#define G_NAME_COUNTER_122 0x00D05650u
#define G_NAME_COUNTER_121 0x00D05654u
#define G_NAME_COUNTER_120 0x00D05658u
#define G_NAME_COUNTER_11F 0x00D0565Cu
#define G_NAME_COUNTER_11E 0x00D05660u
#define G_NAME_COUNTER_11D 0x00D05664u
#define G_NAME_COUNTER_11C 0x00D05668u
#define G_NAME_COUNTER_11B 0x00D0566Cu
#define G_NAME_COUNTER_11A 0x00D05670u
#define G_NAME_COUNTER_119 0x00D05674u
#define G_NAME_COUNTER_118 0x00D05678u
#define G_NAME_COUNTER_117 0x00D0567Cu
#define G_NAME_COUNTER_116 0x00D05680u
#define G_NAME_COUNTER_115 0x00D05684u
#define G_NAME_COUNTER_114 0x00D05688u
#define G_NAME_COUNTER_113 0x00D0568Cu
#define G_NAME_COUNTER_112 0x00D05690u
#define G_NAME_COUNTER_111 0x00D05694u
#define G_NAME_COUNTER_110 0x00D05698u
#define G_NAME_COUNTER_106 0x00D056C0u
#define G_NAME_COUNTER_105 0x00D056C4u
#define G_NAME_COUNTER_104 0x00D056C8u
#define G_NAME_COUNTER_103 0x00D056CCu
#define G_NAME_COUNTER_102 0x00D056D0u
#define G_NAME_COUNTER_100 0x00D056D8u
#define G_NAME_COUNTER_FF 0x00D056DCu
#define G_NAME_COUNTER_FE 0x00D056E0u

/* @bin the figure property table G_PROPS (0x007090CC + type * 0x38, save.md section 5 "Figures"): the
 * fields of an entry, S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * type) (G_FIGURE_TYPES and G_TYPE_TABLE
 * below are the addresses of +4 and +0x18) */
#define FIGTYPE_U00 0x00u        /* i32: the fighting class 1 .. 7 (0x004B1210) */
#define FIGTYPE_MAX_DAMAGE 0x04u /* i32: maximum damage (save.md; 0x004B17D0) */
#define FIGTYPE_ATTACK 0x08u     /* i32: attack (save.md; 0x004B17D0) */
#define FIGTYPE_DEFENCE 0x0Cu    /* i32: defence (save.md; 0x004B17D0) */
#define FIGTYPE_U10 0x10u        /* i32: the defence of a missile's target (0x0049A5FD) */
#define FIGTYPE_U14 0x14u        /* i32: a missile's attack (0x0049A5FD) */
#define FIGTYPE_RANGE 0x18u      /* i32: a range (actions_a.c, the target searches) */
#define FIGTYPE_PERIOD 0x1Cu     /* i32: search period, ticks between shots (actions_a.c, actions_c2.c) */
#define FIGTYPE_SPEED 0x20u      /* i32: the speed passed to the movement (figure_move) */
#define FIGTYPE_U24 0x24u        /* i32: shares of an invasion's three groups (0x004AD556: types 0x2B..0x2D, 0x36..0x38, 0x63) */
#define FIGTYPE_U28 0x28u        /* i32: compared with FIG_U134 (0x004A61E2, 0x004AE34A) */
#define FIGTYPE_U2C 0x2Cu        /* i32: compared with FIG_U138 (0x004A5DA0 caller, actions_a3.c) */
#define FIGTYPE_U30 0x30u        /* @bin 0x00446A30 i32: an invader's FIG_FRAME_PERIOD (core/city/events_troops.c) */
#define FIGTYPE_U34 0x34u        /* @bin 0x00446A30 i32: an invader's FIG_FRAME_TICKS (core/city/events_troops.c) */
#define G_FIGURE_TYPE_STRIDE 0x38u /* @bin 0x00446A30 the stride of G_PROPS (one entry per figure type) */

/* @bin formations and the combat searches (formation.c, enemies.c, invaders.c, actions_d.c): scratch
 * globals and the counters of chunks 26, 27, 108-112 (save.md section 3) */
#define G_PLACE_OFFSETS 0x00BCA360u /* i32 per figure of the formation: its layout slot's grid offset (0x004BA7E0) */
#define G_EMPTY_FLAGS 0x00BCA428u /* u8 per entry of G_EMPTY_EVENTS (formations_rebuild) */
#define G_EMPTY_EVENTS 0x00BCA45Cu /* i32 the +0x78 of the empty enemy formations (formations_rebuild) */
#define G_ENEMY_TILES 0x00BCA524u /* i32 tiles of the figures that can be attacked (0x0048E820) */
#define G_ENEMY_IDS 0x00BCA6B4u /* i32 ids of those figures (0x0048E820) */
#define G_ENEMY_COUNT 0x00BCA844u /* i32 their number (0x0048E820) */
#define G_OPPOSITE_DIRECTION 0x00BCB9A0u /* i32 the opposite direction (0x0048A6E3) */
#define G_PLACE_Y 0x00BCB9A4u /* i32 y of the place found by formation_find_place (0x004BA7E0) */
#define G_PLACE_X 0x00BCB9A8u /* i32 x of that place */
#define G_LEGIONS 0x00BCB9B4u /* i32 number of player legions (chunk 27) */
#define G_LEGION_LAST 0x00BCB9B8u /* i32 last player legion found by the recount (chunk 26) */
#define G_UBCD5E8 0x00BCD5E8u /* i32 := 1 by the roaming step before figure_move, 0 after the sub-steps (0x004B1EF0); also the end of the formation records */
#define G_YARDS_LOW 0x00D05628u /* i32 storage yards passed over for their worker ratio (0x0045D9E0) */
#define G_ENEMY_COUNTER 0x00D0562Cu /* i32 counter 0..4 (chunk 112, 0x004B8CF0) */
#define G_OWN_STRENGTH 0x00D05630u /* i32 own soldier strength (chunk 111) */
#define G_OWN_FORMATIONS 0x00D05634u /* i32 own formations (chunk 110) */
#define G_ENEMY_SOLDIERS 0x00D05638u /* i32 enemy soldiers (chunk 109) */
#define G_ENEMY_FORMATIONS 0x00D0563Cu /* i32 enemy formations (chunk 108) */

/* @bin tables and scratch globals of the figure actions; the function on each line shows the use
 * (core/figure); a name with U and the address until its meaning is established */
#define G_U579520 0x00579520u /* i32 compared with the mission byte for a tutorial check (0x0045FECD) */
#define G_RING_COUNT 0x005C9AFEu /* i16 per ring (1-based): tiles of the 1x1 ring (0x00481490; core/map/terrain_rings.c) */
#define G_RING_FIRST 0x005C9B52u /* i16 per ring: its first entry in G_RING_OFFSETS_1 (0x00481490) */
#define G_COMPANY_SHIFT_X 0x005CA506u /* i8 pairs by the figure's FIG_U00: x shift of its company point (0x0048E9E0) */
#define G_COMPANY_SHIFT_Y 0x005CA507u /* i8: y of the same pairs */
#define G_FORMATION_LAYOUT_X 0x005CD6F8u /* i32 x of the 16 slots (8 bytes each) of a formation layout, 0x80 bytes per layout (0x0048EB4D, 0x004BA7E0) */
#define G_FORMATION_LAYOUT_Y 0x005CD6FCu /* i32: y of the same slots */
#define G_RING16_DX 0x005CDB78u /* i32: dx of 16 offsets (8 bytes each) around a point (0x004EAB10) */
#define G_RING16_DY 0x005CDB7Cu /* i32: dy of the same offsets */
#define G_FORMATION_LAYOUT_0D_1 0x005CDD78u /* slots of layout 1 for figure type 0x0D (0x0048EB4D) */
#define G_FORMATION_LAYOUT_0D_2 0x005CDDF8u /* slots of layout 2 for figure type 0x0D (0x0048EB4D) */
#define G_U5D050C 0x005D050Cu /* u8 frame by FIG_U6B / 2 (the image of the figure of 0x00497450) */
#define G_ATTACK_FRAMES 0x005D058Cu /* u8 frame by the attack step / 2 (0x00490A54, 0x00497FC0) */
#define G_ROAD_WEAR_KIND 0x005DBF70u /* i32 per figure type: the 3-bit field of G_TILE_VISITS it wears (0x0047E340, 0x0047E250) */
#define G_U5DC128 0x005DC128u /* i32 grid offsets by even direction, 8 bytes each (0x0047E340) */
#define G_CARRY_OFFSET_X 0x005DC23Cu /* u8 pairs by direction: the carried resource's offset, x (0x0048A6E3, 0x0049A55C) */
#define G_CARRY_OFFSET_Y 0x005DC23Du /* u8: y of the same pairs */
#define G_RAM_DIVISOR 0x005DC24Cu /* u8 [8 x 8] by the two directions (0x004A6160, 0x004AE34A) */
#define G_U5DC5FC 0x005DC5FCu /* i32 by size: tile offset of the spawned figures (0x0048C0E0) */
#define G_U5DC614 0x005DC614u /* i32 by size: their offset in 1/15 tile (0x0048C0E0) */
#define G_U5DC62C 0x005DC62Cu /* i16 dx of the destination pairs, 8 bytes each (0x0048C0E0) */
#define G_U5DC630 0x005DC630u /* i16: dy of the same pairs */
#define G_U5DC6AC 0x005DC6ACu /* u8 per entry (4 bytes each), kept at +0x59 (0x0048C0E0) */
#define G_U5DC7EC 0x005DC7ECu /* u32 frame offset by FIG_ANIM_FRAME (0x00494800 caller, kind 0) */
#define G_U5DC81C 0x005DC81Cu /* u32 frame offset by FIG_ANIM_FRAME (kinds 1 and 2) */
#define G_U5DC87C 0x005DC87Cu /* i32 frame offset by FIG_ANIM_FRAME, -1: none (kind 3) */
#define G_ENEMY_LAYOUT 0x005DCB1Cu /* i32 (x, y) pairs: a figure's place in its group's layout (0x004B9213) */
#define G_ENEMY_TARGET_TYPES 0x005DEB9Cu /* building types preferred by formation kind, 200 bytes per kind (0x004BA3B0) */
#define G_ENEMY_TARGET_GENERAL 0x005DEEBCu /* the general target list (0x004BA3B0) */
#define G_STR_HORN1_WAV 0x005DF6B8u /* the leader's sound file name: horn 1 (0x004992EF) */
#define G_STR_HORN2_WAV 0x005DF6C8u /* horn 2 (0x004992EF) */
#define G_STR_DRUMS_WAV 0x005DF6D8u /* drums (0x004992EF) */
#define G_STR_DEBUG_5DF6E8 0x005DF6E8u /* sprintf format "Laborer at x,y: %d, %d" for the debugger output (0x004A9E51) */
#define G_STR_DEBUG_5DF700 0x005DF700u /* sprintf format for the debugger output (0x004B02D3) */
#define G_STR_DEBUG_5DF718 0x005DF718u /* sprintf format for the debugger output (0x004B014A) */
#define G_U5E4C3C 0x005E4C3Cu /* u8 at +0x1C of the 0x30-byte records of G_HERD_RECORDS by G_U784CB0 (0x004EAAC0) */
#define G_INVASION_SUB_RECORDS 0x005E4CB0u /* 0x30-byte records by G_INVASION_SUB + 2 * kind (invaders.c) */
#define G_U5E4CCC 0x005E4CCCu /* u8 at +0x1C of the G_INVASION_SUB_RECORDS (0x004EAAC0) */
#define G_U5E4CD0 0x005E4CD0u /* at +0x20 of the G_INVASION_SUB_RECORDS: +0 ticks between searches, +4 search radius */
#define G_U5E4DD0 0x005E4DD0u /* figure types searched by map_search_rings (0x004EABF0 caller) */
#define G_U5E4DF0 0x005E4DF0u /* figure types searched by figb_ring_search */
#define G_U5E4E0C 0x005E4E0Cu /* animal wander offsets (0x004EC4A0) */
#define G_U5E4E30 0x005E4E30u /* animal wander pairs (0x004EC4A0) */
#define G_U5E4E70 0x005E4E70u /* i32 next actions drawn at random (0x004EC7C0) */
#define G_U5E4E80 0x005E4E80u /* animal wander offsets (0x004ECA70) */
#define G_U5E4EA0 0x005E4EA0u /* animal wander pairs (0x004ECA70) */
#define G_U5E4EE0 0x005E4EE0u /* i32 next actions (0x004ECA70 caller) */
#define G_U5F6B68 0x005F6B68u /* i32 by level: the reference corner (0x004F3370) */
#define G_U5F6B94 0x005F6B94u /* i32 by pyramid kind: a part offset (0x004F3370) */
#define G_U5F6BBC 0x005F6BBCu /* i32 corner dx (0x004F3370) */
#define G_U5F6BCC 0x005F6BCCu /* i32 corner dy (0x004F3370) */
#define G_U7065F8 0x007065F8u /* i32 read by walker type 0x21 (0x004BAA10) */
#define G_U706760 0x00706760u /* i32 read by walker type 0x58 */
#define G_U706918 0x00706918u /* i32 read by the temple walker (god mood 1) */
#define G_U706968 0x00706968u /* i32 read by the temple walker (god mood 3) */
#define G_U706B70 0x00706B70u /* i32 amount for building field +0x46 (0x00482C10) */
#define G_U7078E0 0x007078E0u /* i32 amount for building field +0x48 (0x00483000) */
#define G_U707B88 0x00707B88u /* i32 read by walker type 0x59 */
#define G_U707EF8 0x00707EF8u /* i32 read by walker type 0x1C */
#define G_TRADE_OPEN_13 0x007756F0u /* i32 per good: trade_good_open_13 flags (0x00445670) */
#define G_TRADE_OPEN_24 0x00775780u /* i32 per good: trade_good_open_24 flags (0x00445AC0) */
#define G_U784B38 0x00784B38u /* u8 x by index (2 bytes each) for indices 0x16..0x19 (0x00494800) */
#define G_U784B40 0x00784B40u /* u8 y of the same */
#define G_U784B58 0x00784B58u /* u8 x for indices 0x0E..0x15 */
#define G_U784B5C 0x00784B5Cu /* u8 x for the other indices */
#define G_U784B68 0x00784B68u /* u8 y for indices 0x0E..0x15 */
#define G_U784B7C 0x00784B7Cu /* u8 y for the other indices */
#define G_U784D4C 0x00784D4Cu /* u8 x for indices 0x1E..0x20 (4 bytes each, 0x00494800) */
#define G_U784D58 0x00784D58u /* u8 y of the same */
#define G_RUIN_DISTANCE 0x0078D7F8u /* i32 distance of the burning ruin found by 0x00469A50 */
#define G_GOOD_CHOSEN 0x0078D99Cu /* i32 the good 0x0045E150 chose */
#define G_U972534 0x00972534u /* i32 building ids listed by 0x00492650 / 0x00492D20 (G_CANDIDATE_IDS + 4: the same list, 1-based) */
#define G_DOCK_BATTLES_U0C 0x009783B0u /* i32 at +0x0C of the dock battle records (28 bytes, by building +0xCB; 0x004069E0) */
#define G_SERVICE_TOP 0x009AB9DCu /* i32 the highest value met by the last pass (0x00482C10) */
#define G_SERVICE_TOP_46 0x009AB9E0u /* i32 the highest +0x46 of the last pass (0x00482C10) */
#define G_SERVICE_TOP_48 0x009AB9E4u /* i32 the highest +0x48 of the last pass (0x00483000) */
#define G_SERVICE_TOP_HOUSES 0x009AB9E8u /* i32 the largest value of the houses within 2 tiles (0x00484A90) */

/* @bin 0x0053DFF0 image groups: the first image id of global image group n is at G_IMAGE_GROUPS
 * (0x00F55BB4) + 8 * n (docs/formats/graphics.md "What a load writes": this + 8 * group, record
 * {slot, first image id}, this = 0x00F55BB0). The groups the figure actions draw from */
#define G_IMAGE_GROUP_39 0x00F55CECu
#define G_IMAGE_GROUP_40 0x00F55CF4u
#define G_IMAGE_GROUP_41 0x00F55CFCu
#define G_IMAGE_GROUP_42 0x00F55D04u
#define G_IMAGE_GROUP_43 0x00F55D0Cu
#define G_IMAGE_GROUP_44 0x00F55D14u
#define G_IMAGE_GROUP_45 0x00F55D1Cu
#define G_IMAGE_GROUP_46 0x00F55D24u
#define G_IMAGE_GROUP_47 0x00F55D2Cu
#define G_IMAGE_GROUP_48 0x00F55D34u
#define G_IMAGE_GROUP_50 0x00F55D44u
#define G_IMAGE_GROUP_51 0x00F55D4Cu
#define G_IMAGE_GROUP_52 0x00F55D54u
#define G_IMAGE_GROUP_53 0x00F55D5Cu
#define G_IMAGE_GROUP_54 0x00F55D64u
#define G_IMAGE_GROUP_55 0x00F55D6Cu
#define G_IMAGE_GROUP_56 0x00F55D74u
#define G_IMAGE_GROUP_57 0x00F55D7Cu
#define G_IMAGE_GROUP_58 0x00F55D84u
#define G_IMAGE_GROUP_59 0x00F55D8Cu
#define G_IMAGE_GROUP_60 0x00F55D94u
#define G_IMAGE_GROUP_61 0x00F55D9Cu
#define G_IMAGE_GROUP_62 0x00F55DA4u
#define G_IMAGE_GROUP_63 0x00F55DACu
#define G_IMAGE_GROUP_64 0x00F55DB4u
#define G_IMAGE_GROUP_65 0x00F55DBCu
#define G_IMAGE_GROUP_66 0x00F55DC4u
#define G_IMAGE_GROUP_71 0x00F55DECu
#define G_IMAGE_GROUP_72 0x00F55DF4u
#define G_IMAGE_GROUP_73 0x00F55DFCu
#define G_IMAGE_GROUP_74 0x00F55E04u
#define G_IMAGE_GROUP_75 0x00F55E0Cu
#define G_IMAGE_GROUP_76 0x00F55E14u
#define G_IMAGE_GROUP_77 0x00F55E1Cu
#define G_IMAGE_GROUP_78 0x00F55E24u
#define G_IMAGE_GROUP_79 0x00F55E2Cu
#define G_IMAGE_GROUP_80 0x00F55E34u
#define G_IMAGE_GROUP_81 0x00F55E3Cu
#define G_IMAGE_GROUP_82 0x00F55E44u
#define G_IMAGE_GROUP_83 0x00F55E4Cu
#define G_IMAGE_GROUP_91 0x00F55E8Cu
#define G_IMAGE_GROUP_92 0x00F55E94u
#define G_IMAGE_GROUP_93 0x00F55E9Cu
#define G_IMAGE_GROUP_94 0x00F55EA4u
#define G_IMAGE_GROUP_95 0x00F55EACu
#define G_IMAGE_GROUP_96 0x00F55EB4u
#define G_IMAGE_GROUP_97 0x00F55EBCu
#define G_IMAGE_GROUP_98 0x00F55EC4u
#define G_IMAGE_GROUP_99 0x00F55ECCu
#define G_IMAGE_GROUP_100 0x00F55ED4u
#define G_IMAGE_GROUP_101 0x00F55EDCu
#define G_IMAGE_GROUP_102 0x00F55EE4u
#define G_IMAGE_GROUP_103 0x00F55EECu
#define G_IMAGE_GROUP_104 0x00F55EF4u
#define G_IMAGE_GROUP_105 0x00F55EFCu
#define G_IMAGE_GROUP_106 0x00F55F04u
#define G_IMAGE_GROUP_107 0x00F55F0Cu
#define G_IMAGE_GROUP_108 0x00F55F14u
#define G_IMAGE_GROUP_110 0x00F55F24u
#define G_IMAGE_GROUP_111 0x00F55F2Cu
#define G_IMAGE_GROUP_112 0x00F55F34u
#define G_IMAGE_GROUP_113 0x00F55F3Cu
#define G_IMAGE_GROUP_114 0x00F55F44u
#define G_IMAGE_GROUP_115 0x00F55F4Cu
#define G_IMAGE_GROUP_116 0x00F55F54u
#define G_IMAGE_GROUP_145 0x00F5603Cu
#define G_IMAGE_GROUP_148 0x00F56054u
#define G_IMAGE_GROUP_149 0x00F5605Cu
#define G_IMAGE_GROUP_150 0x00F56064u
#define G_IMAGE_GROUP_151 0x00F5606Cu
#define G_IMAGE_GROUP_152 0x00F56074u
#define G_IMAGE_GROUP_153 0x00F5607Cu
#define G_IMAGE_GROUP_154 0x00F56084u
#define G_IMAGE_GROUP_155 0x00F5608Cu
#define G_IMAGE_GROUP_156 0x00F56094u
#define G_IMAGE_GROUP_160 0x00F560B4u
#define G_IMAGE_GROUP_161 0x00F560BCu
#define G_IMAGE_GROUP_162 0x00F560C4u
#define G_IMAGE_GROUP_163 0x00F560CCu
#define G_IMAGE_GROUP_164 0x00F560D4u
#define G_IMAGE_GROUP_165 0x00F560DCu
#define G_IMAGE_GROUP_166 0x00F560E4u
#define G_IMAGE_GROUP_173 0x00F5611Cu
#define G_IMAGE_GROUP_174 0x00F56124u
#define G_IMAGE_GROUP_175 0x00F5612Cu
#define G_IMAGE_GROUP_176 0x00F56134u
#define G_IMAGE_GROUP_177 0x00F5613Cu
#define G_IMAGE_GROUP_178 0x00F56144u
#define G_IMAGE_GROUP_179 0x00F5614Cu
#define G_IMAGE_GROUP_180 0x00F56154u
#define G_IMAGE_GROUP_181 0x00F5615Cu
#define G_IMAGE_GROUP_182 0x00F56164u
#define G_IMAGE_GROUP_184 0x00F56174u
#define G_IMAGE_GROUP_185 0x00F5617Cu
#define G_IMAGE_GROUP_186 0x00F56184u
#define G_IMAGE_GROUP_189 0x00F5619Cu
#define G_IMAGE_GROUP_190 0x00F561A4u
#define G_IMAGE_GROUP_191 0x00F561ACu
#define G_IMAGE_GROUP_194 0x00F561C4u
#define G_IMAGE_GROUP_195 0x00F561CCu
#define G_IMAGE_GROUP_196 0x00F561D4u
#define G_IMAGE_GROUP_197 0x00F561DCu
#define G_IMAGE_GROUP_198 0x00F561E4u
#define G_IMAGE_GROUP_199 0x00F561ECu
#define G_IMAGE_GROUP_200 0x00F561F4u
#define G_IMAGE_GROUP_201 0x00F561FCu
#define G_IMAGE_GROUP_202 0x00F56204u
#define G_IMAGE_GROUP_203 0x00F5620Cu
#define G_IMAGE_GROUP_204 0x00F56214u
#define G_IMAGE_GROUP_219 0x00F5628Cu
#define G_IMAGE_GROUP_220 0x00F56294u
#define G_IMAGE_GROUP_221 0x00F5629Cu
#define G_IMAGE_GROUP_222 0x00F562A4u
#define G_IMAGE_GROUP_234 0x00F56304u
#define G_IMAGE_GROUP_235 0x00F5630Cu
#define G_IMAGE_GROUP_238 0x00F56324u
#define G_IMAGE_GROUP_239 0x00F5632Cu
#define G_IMAGE_GROUP_240 0x00F56334u
#define G_IMAGE_GROUP_241 0x00F5633Cu
#define G_IMAGE_GROUP_242 0x00F56344u
#define G_IMAGE_GROUP_244 0x00F56354u
#define G_IMAGE_GROUP_245 0x00F5635Cu
#define G_IMAGE_GROUP_246 0x00F56364u
#define G_IMAGE_GROUP_251 0x00F5638Cu
#define G_IMAGE_GROUP_252 0x00F56394u
#define G_IMAGE_GROUP_253 0x00F5639Cu
#define G_IMAGE_GROUP_254 0x00F563A4u
#define G_IMAGE_GROUP_255 0x00F563ACu
#define G_IMAGE_GROUP_257 0x00F563BCu
#define G_IMAGE_GROUP_258 0x00F563C4u
#define G_IMAGE_GROUP_259 0x00F563CCu
#define G_IMAGE_GROUP_260 0x00F563D4u
#define G_IMAGE_GROUP_261 0x00F563DCu
#define G_IMAGE_GROUP_262 0x00F563E4u
#define G_IMAGE_GROUP_263 0x00F563ECu
#define G_IMAGE_GROUP_264 0x00F563F4u
#define G_IMAGE_GROUP_265 0x00F563FCu
#define G_IMAGE_GROUP_266 0x00F56404u
#define G_IMAGE_GROUP_267 0x00F5640Cu
#define G_IMAGE_GROUP_268 0x00F56414u
#define G_IMAGE_GROUP_270 0x00F56424u
#define G_IMAGE_GROUP_408 0x00F56874u
#define G_IMAGE_GROUP_409 0x00F5687Cu
#define G_IMAGE_GROUP_422 0x00F568E4u
#define G_IMAGE_GROUP_423 0x00F568ECu
#define G_IMAGE_GROUP_424 0x00F568F4u
#define G_IMAGE_GROUP_425 0x00F568FCu
#define G_IMAGE_GROUP_432 0x00F56934u
#define G_IMAGE_GROUP_446 0x00F569A4u
#define G_IMAGE_GROUP_495 0x00F56B2Cu
#define G_IMAGE_GROUP_501 0x00F56B5Cu
#define G_IMAGE_GROUP_508 0x00F56B94u
#define G_IMAGE_GROUP_509 0x00F56B9Cu
#define G_IMAGE_GROUP_529 0x00F56C3Cu
#define G_IMAGE_GROUP_577 0x00F56DBCu
#define G_IMAGE_GROUP_585 0x00F56DFCu
#define G_IMAGE_GROUP_588 0x00F56E14u
#define G_IMAGE_GROUP_590 0x00F56E24u
#define G_IMAGE_GROUP_591 0x00F56E2Cu
#define G_IMAGE_GROUP_592 0x00F56E34u
#define G_IMAGE_GROUP_594 0x00F56E44u
#define G_IMAGE_GROUP_596 0x00F56E54u
#define G_IMAGE_GROUP_597 0x00F56E5Cu
#define G_IMAGE_GROUP_598 0x00F56E64u
#define G_IMAGE_GROUP_599 0x00F56E6Cu
#define G_IMAGE_GROUP_600 0x00F56E74u
#define G_IMAGE_GROUP_601 0x00F56E7Cu
#define G_IMAGE_GROUP_602 0x00F56E84u
#define G_IMAGE_GROUP_603 0x00F56E8Cu
#define G_IMAGE_GROUP_604 0x00F56E94u
#define G_IMAGE_GROUP_605 0x00F56E9Cu
#define G_IMAGE_GROUP_606 0x00F56EA4u
#define G_IMAGE_GROUP_607 0x00F56EACu
#define G_IMAGE_GROUP_608 0x00F56EB4u
#define G_IMAGE_GROUP_609 0x00F56EBCu
#define G_IMAGE_GROUP_610 0x00F56EC4u
#define G_IMAGE_GROUP_611 0x00F56ECCu
#define G_IMAGE_GROUP_612 0x00F56ED4u
#define G_IMAGE_GROUP_620 0x00F56F14u
#define G_IMAGE_GROUP_621 0x00F56F1Cu
#define G_IMAGE_GROUP_642 0x00F56FC4u
#define G_IMAGE_GROUP_643 0x00F56FCCu
#define G_IMAGE_GROUP_644 0x00F56FD4u
#define G_IMAGE_GROUP_646 0x00F56FE4u
#define G_IMAGE_GROUP_647 0x00F56FECu
#define G_IMAGE_GROUP_648 0x00F56FF4u
#define G_IMAGE_GROUP_649 0x00F56FFCu
#define G_IMAGE_GROUP_650 0x00F57004u
#define G_IMAGE_GROUP_651 0x00F5700Cu
#define G_IMAGE_GROUP_653 0x00F5701Cu
#define G_IMAGE_GROUP_654 0x00F57024u
#define G_IMAGE_GROUP_655 0x00F5702Cu
#define G_IMAGE_GROUP_656 0x00F57034u
#define G_IMAGE_GROUP_657 0x00F5703Cu
#define G_IMAGE_GROUP_658 0x00F57044u
#define G_IMAGE_GROUP_659 0x00F5704Cu
#define G_IMAGE_GROUP_660 0x00F57054u
#define G_IMAGE_GROUP_662 0x00F57064u
#define G_IMAGE_GROUP_663 0x00F5706Cu
#define G_IMAGE_GROUP_664 0x00F57074u
#define G_IMAGE_GROUP_665 0x00F5707Cu
#define G_IMAGE_GROUP_666 0x00F57084u
#define G_IMAGE_GROUP_667 0x00F5708Cu
#define G_IMAGE_GROUP_668 0x00F57094u
#define G_IMAGE_GROUP_669 0x00F5709Cu
#define G_IMAGE_GROUP_670 0x00F570A4u
#define G_IMAGE_GROUP_672 0x00F570B4u
#define G_IMAGE_GROUP_673 0x00F570BCu
#define G_IMAGE_GROUP_681 0x00F570FCu
#define G_IMAGE_GROUP_684 0x00F57114u
#define G_IMAGE_GROUP_687 0x00F5712Cu
#define G_IMAGE_GROUP_688 0x00F57134u
#define G_IMAGE_GROUP_689 0x00F5713Cu
#define G_IMAGE_GROUP_691 0x00F5714Cu
#define G_IMAGE_GROUP_694 0x00F57164u
#define G_IMAGE_GROUP_695 0x00F5716Cu
#define G_IMAGE_GROUP_697 0x00F5717Cu
#define G_IMAGE_GROUP_699 0x00F5718Cu
#define G_IMAGE_GROUP_700 0x00F57194u
#define G_IMAGE_GROUP_701 0x00F5719Cu
#define G_IMAGE_GROUP_702 0x00F571A4u
#define G_IMAGE_GROUP_703 0x00F571ACu
#define G_IMAGE_GROUP_704 0x00F571B4u
#define G_IMAGE_GROUP_705 0x00F571BCu
#define G_IMAGE_GROUP_706 0x00F571C4u
#define G_IMAGE_GROUP_707 0x00F571CCu
#define G_IMAGE_GROUP_708 0x00F571D4u
#define G_IMAGE_GROUP_709 0x00F571DCu
#define G_IMAGE_GROUP_710 0x00F571E4u
#define G_IMAGE_GROUP_721 0x00F5723Cu
#define G_IMAGE_GROUP_722 0x00F57244u

/* @bin wave 0 names checked against core/figure's uses (docs/naming.md "Harvested names"): the
 * evidence is the doc or chunk named, else the defining port and its comment */
#define G_DIE_FRAMES 0x005D048Cu /* core/figure/actions_b.h: dying animation frame by wait ticks / 2 */
#define G_PRICE_BUY 0x005D2C48u /* i32 per resource, 8-byte entries {buy, sell} (save.md chunk 53; present/ui/advisors/d_trade.c) */
#define G_PRICE_SELL 0x005D2C4Cu /* present/ui/advisors/d_trade.c */
#define G_ACTION_TABLE 0x005DC28Cu /* core/figure/figure.c: one action function per figure type */
#define G_DIRECTION_OFFSETS 0x005F80FCu /* core/map/route_path.c: i32 grid offset per direction 0..7 */
#define G_PROPS 0x007090CCu /* figure property table, 0x38 bytes per type, fields FIGTYPE_* (save.md section 5; core/figure/actions_b.h) */
#define G_FIGURE_TYPES 0x007090D0u /* G_PROPS + FIGTYPE_MAX_DAMAGE: the property table's +4 column (core/building/building_misc.c: records of 0x38 bytes per figure type) */
#define G_TYPE_TABLE 0x007090E4u /* G_PROPS + FIGTYPE_RANGE: the property table's +0x18 column (core/figure/actions_d4.c) */
#define G_HOUSE_TAX_VALUE 0x0070CE18u /* core/command/orders.c: i32 house model field per level, stride 0x58 */
#define G_DEFAULT_LIMIT 0x00775810u /* core/command/trade.c: i32 per resource, hundredths */
#define G_TRADE_DONE 0x0077592Cu /* i32 [route * 0x24 + resource]: traded this period (save.md chunk 115; present/ui/empire/empire.c) */
#define G_INVASION_SUB 0x00784B94u /* core/figure/invaders.c: i16 */
#define G_U784BBC 0x00784BBCu /* core/figure/actions_a2.c: non-zero: no goods are moved to the storage yards */
#define G_ENTRY_X 0x00784C8Cu /* core/figure/actions_d4.c: @bin the map's river entry / exit points (s16 each) */
#define G_ENTRY_Y 0x00784C8Eu /* core/building/daily.c: i16 */
#define G_EXIT_X 0x00784C90u /* core/figure/actions_d4.c */
#define G_EXIT_Y 0x00784C92u /* core/figure/actions_d4.c */
#define G_U784CB0 0x00784CB0u /* core/figure/invaders.c: low byte: the invasion kind (records of 0x30 bytes at 0x005E4C20) */
#define G_WORK_COUNT 0x0078D7F0u /* core/figure/actions_d2.c: @bin candidates found by 0x004BEEB0 */
#define G_WORK_LIST 0x0078D9A8u /* core/figure/actions_d2.c: @bin candidate building ids (s16, at most 500) */
#define G_STORAGE_GET 0x00790134u /* core/command/orders_storage.c: i16 fill limit per resource (other orders) */
#define G_TARGETS 0x0089B6D8u /* core/figure/helpers_xa.c: 27 i32: the chosen source per good index */
#define G_CAND_TILE 0x0089B744u /* core/figure/helpers_xa.c: 27 x 4000 i32: each candidate's road tile */
#define G_CAND_BUILDING 0x00904EC4u /* core/figure/helpers_xa.c: 27 x 4000 i32: each candidate's building */
#define G_CANDIDATES 0x0096E6B0u /* core/figure/actions_a2.c: count, then the candidates' map offsets (0x0096E6B4 ..) */
#define G_FULL_LIST 0x0096E6B4u /* core/building/day_water.c: i32[4000]: tiles at 255, floodplain from the start, others from the end */
#define G_CANDIDATE_IDS 0x00972530u /* core/figure/actions_a2.c: the candidates' building ids, 1-based like the offsets */
#define G_FERRY_SLOTS 0x009784D8u /* ferry-boat passenger lists, 50 x 44 (save.md chunk 138; figure +0x11C) (core/figure/actions_c.c) */
#define G_THREAT 0x00A835FCu /* core/figure/enemies.c: u8 per tile: threat of the player's soldiers */
#define G_MAP_YE 0x00BCA2F4u /* core/figure/actions_c2.c: last y */
#define G_PROF_STATS 0x00BCA850u /* core/figure/figure.c */
#define G_PROF_TYPE 0x00BCB610u /* core/figure/figure.c */
#define G_PROF_START 0x00BCB614u /* core/figure/figure.c */
#define G_PROF_END 0x00BCB618u /* core/figure/figure.c */
#define G_FORMATION_BASE 0x00BCB9C8u /* formations, 50 x 0x90, fields FM_* (save.md chunk 24; core/io/save_load.c) */
#define G_BEST_DISTANCE 0x00BCD5ECu /* core/figure/actions_c2.c: the target searches' best distance so far */
#define G_FIGURE_HIGHEST 0x00BCD5F0u /* core/figure/figure_create.c: highest figure id created */
#define G_FIGURE_PATHS 0x00C8AD38u /* 500 bytes per route slot (save.md chunk 23; core/map/route_path.c) */
#define G_ROUTE_OWNERS 0x00D04E58u /* route slot -> the figure using it, i16 (save.md chunk 22; core/figure/actions_a3.c) */
#define G_FIGURES_UPDATED 0x00D056F4u /* core/figure/figure.c: live figures counted by the last update */
#define G_CURRENT_FIGURE 0x00D056F8u /* core/figure/actions_b.h: the figure being updated */
#define G_FIGURE_SERIAL 0x00D056FCu /* creation counter, kept at +0x86 (save.md chunk 86; core/figure/figure_create.c) */
#define G_PROF_PRECISE 0x00D05710u /* core/figure/figure.c: QueryPerformanceCounter available */
#define G_DIST_Y 0x00D3B4F8u /* core/figure/actions_d2.c: @bin |dy| of the last distance */
#define G_DIST_X 0x00D3B4FCu /* core/figure/actions_a.c: the last distances computed (kept by the original in globals) */
#define G_SPRITE_DIRECTION 0x00E94E00u /* core/figure/actions_a.c: the current figure's direction relative to the view */
#define G_FERRY_QUEUES 0x00EAAF68u /* ferry-landing queues, 50 x 0xE0 (save.md chunk 137; building +0xCA) */
#define G_EBAB20 0x00EBAB20u /* core/map/routing.h */
#define G_EC7630 0x00EC7630u /* core/map/routing.h */
#define G_RESOURCE_ICONS 0x00F571FCu /* present/ui/messages/window.c: i32 first resource icon */

/* ---- names merged from the packages' pending fragments (phase 10.4): each group keeps its
 * package's heading (the package and its file in brackets) ---- */
/* @bin globals of other areas used by core/figure; the function or chunk on each line [p1_figure] */
#define G_TRADER_NEXT 0x0077AEFCu /* i32 next trader record index (chunk 95) */
#define G_U784DC4 0x00784DC4u /* i32 x of three points (-1: none; 0x004AEAB0) */
#define G_U784DD0 0x00784DD0u /* i32 y of the three points */
#define G_STORAGE_SETTINGS_U08 0x007900C0u /* storage settings records of 0xC4 bytes by building +0xC1 (chunk 113 at 0x007900B8, +8) */
#define G_FLOOD_STATE 0x00D386D4u /* i32 flood state 1..3 (0x004894A0; tick.md) */
/* @bin 0x0053DFF0 image groups (G_IMAGE_GROUPS + 8 * n: the group's first image id,
 * docs/formats/graphics.md) also drawn outside core/figure [p1_figure] */
#define G_IMAGE_GROUP_157 0x00F5609Cu
#define G_IMAGE_GROUP_323 0x00F565CCu
#define G_IMAGE_GROUP_348 0x00F56694u /* the warship picture (present/ui/advisors/a_military.c), also a map marker (core/figure/actions_b1.c) */
#define G_IMAGE_GROUP_353 0x00F566BCu
#define G_IMAGE_GROUP_395 0x00F5680Cu
#define G_IMAGE_GROUP_396 0x00F56814u /* the legion banners, + the legion (present/ui/info/military.c, a_military.c), also map markers (core/figure/actions_b1.c) */
#define G_IMAGE_GROUP_714 0x00F57204u
/* the military (g_figure.h, P1) [p2_building] */
#define G_FORMATION_HIGHEST 0x00BCB9BCu /* @bin 0x004B6E10 i32: the highest formation id in use (save.md chunk 25) */
/* ferry slots (G_FERRY_SLOTS, chunk 138, 0x2C-byte records, +0 in use; g_figure.h) [p5_map] */
#define FERRY_SLOT_FIGURES 0x04u /* @bin 0x004A3560 i32[4]: the figures listed */
/* ferry-landing queues (chunk 137): G_FERRY_QUEUES + 0xE0 * id, ids 1..49; the building's BLD_UCA holds the id [p5_map] */
#define REC88_BUILDING 0xDCu /* @bin 0x0046EFD0 i32: the building */
/* @bin 0x004B5AF0 the figure name counters (save.md §3 chunks 54-74, 101-104, 150-165; seeded by
 * 0x004B58D0), named after their text group as G_NAME_COUNTER_10F [g_figure.h] [p6_scenario] */
#define G_NAME_COUNTER_10B 0x00D056ACu /* [g_figure.h] */
#define G_NAME_COUNTER_10C 0x00D056A8u /* [g_figure.h] */
#define G_NAME_COUNTER_10D 0x00D056A4u /* [g_figure.h] */
#define G_NAME_COUNTER_10E 0x00D056A0u /* [g_figure.h] */
/* ---- globals of other areas (core or another present package uses them) ---- [p8a_ui_main] */
#define G_FORMATION_X 0x00BCB9F6u /* @bin 0x004B80B0 i16 +0x2E of formation 0 (G_FORMATION_BASE, stride 0x90): x */
#define G_FORMATION_Y 0x00BCB9F8u /* @bin 0x004B80B0 i16 +0x30 of formation 0: y (present/sidebar/minimap.c) */
#define G_FORMATION_U42 0x00BCBA0Au /* @bin 0x004B80B0 u8 +0x42 of formation 0: 0 when not dying */
#define G_FORMATION_IN_FIELD 0x00BCBA2Fu /* @bin 0x00522A60 u8 +0x67 of formation 0: set makes the legions page's "any" */
#define G_FORMATION_U82 0x00BCBA4Au /* @bin 0x00522A60 i16 +0x82 of formation 0: < 0 still to place */
#define G_IMG_OVERLAY_LEVELS 0x00F56CA4u /* @bin 0x00434AA0 i32 the first overlay level image (present/city/overlay_tiles.c) */

#endif
