/* Imhotep - named globals: present/layout/g_ui_info.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8b). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_INFO_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_INFO_H

/* @bin 0x004F7E50 the info window's context, written by the opening click and read by every panel
 * (present/ui/info/info.h, docs/commands.md 1.4); i32 each */
#define G_INFO_SOUND 0x00EADD94u        /* 1 until the first draw (the building's sound plays once) */
#define G_INFO_W 0x00EADD88u            /* the panel's width in 16-pixel blocks */
#define G_INFO_TILE 0x00EADD80u         /* grid offset of the clicked tile (the building's for a building) */
#define G_INFO_TILE_X 0x00EADD7Cu
#define G_INFO_TILE_Y 0x00EADD78u
#define G_INFO_TERRAIN 0x00EADD68u      /* the tile's terrain image byte (0x00A9010C grid) */
#define G_INFO_TILE_FLAG 0x00EADD64u    /* the tile's 0x00AF598C bit 0x80 */
#define G_INFO_TILE_BYTE 0x00EADD60u    /* the tile's 0x00AE8E7C byte */
#define G_INFO_WORKERS 0x00EADD5Cu      /* the building's workers in percent of its need (0x004CDE60) */
#define G_INFO_ROAD 0x00EADD58u         /* 1: the building has road access */
#define G_INFO_HELP 0x00EADD54u         /* the help message the help button opens (0x00511C50) */
#define G_INFO_TERRAIN_KIND 0x00EADD4Cu /* the terrain kind of kind 1 (no figures): its text (0x004F810D) */
#define G_INFO_FIGURE_VIEW 0x00EADD48u  /* 1: the figure list is shown (kind 1 with figures) */
#define G_INFO_FIGURE_COUNT 0x00EADD40u /* figures listed in G_FIGURES (0..7) */
#define G_INFO_ADVISORS 0x00EADCE8u     /* @bin 0x00515130 i32[15] 1: the panel offers the advisor's button */
#define G_UEADCD0 0x00EADCD0u           /* @bin 0x004F8343 i32 the recruiter's (type 0x5F) legions ready (0x004B7590) + 0x00BCB9AC */
#define G_WORST_NEIGHBOUR_VALUE 0x00EADCD8u /* @bin 0x0050F0F0 i32 a house's worst neighbour: its desirability */
#define G_WORST_NEIGHBOUR 0x00EADCD4u   /* @bin 0x0050F0F0 i32 a house's worst neighbour: the building */
#define G_YARD_NOTICE 0x00EADCB8u       /* @bin 0x004F85D4 i32 1 / 2: the storage yard's notice (texts 0x63/0xD, 0x63/0xE; 0x0045D570) */
#define G_SPEECH_KIND 0x00EADCE0u       /* @bin 0x0053C100 i32 the speech table's entry for the figure's type, - 0xC9 */
#define G_SPEECH_PATH 0x00F55A20u       /* @bin 0x00539A70 char[] the speech file's path */

/* @bin 0x004F7FB7 the monuments' status records, computed by the open for the panel (by type), read
 * by the panels at record + offset (present/ui/info/monument.c) */
#define G_COURSE_SECOND 0x00EADB30u     /* @bin 0x00511BF8 i32 the resource of the secondary course */
#define G_COURSE_MAIN 0x00EADB34u       /* @bin 0x00511BF8 i32 the resource of the monument's main course */
#define G_MONUMENT_STATUS_E5 0x00EADB38u /* types 0xE5, 0xEA, 0xEB, 0xEC (0x00545740 on G_MONUMENT_E5, ...) */
#define G_MONUMENT_STATUS_DE 0x00EADB58u /* type 0xDE, the mausoleum (0x004DF040) */
#define G_MONUMENT_STATUS_E4 0x00EADB88u /* type 0xE4 (0x004E6710 on G_MONUMENT_E4) */
#define G_MONUMENT_STATUS_E3 0x00EADBA8u /* type 0xE3 (0x00417050 on G_MONUMENT_E3) */
#define G_MONUMENT_STATUS_E6 0x00EADBC8u /* type 0xE6 (0x00401540 on G_MONUMENT_E6) */
#define G_MONUMENT_STATUS_E1 0x00EADBD8u /* type 0xE1 (0x0040D940 on G_MONUMENT_E1) */
#define G_MONUMENT_STATUS_D5 0x00EADC10u /* type 0xD5, the sun temple (0x00542AD0) */
#define G_PYRAMID_STATUS 0x00EADC48u    /* type 0xB7: 0x1C i32 (0x004F1310) */

/* @bin 0x004F96E0 the panels' button tables (.data, docs/commands.md 1.4) */
#define G_ORDER_CYCLE_BUTTONS 0x005C68C8u  /* the orders sub-panel's rows: cycle a resource's order (0x00406300) */
#define G_ORDERS_ALL_BUTTON 0x005C5BA8u    /* granaries and bazaars: all orders to state 1 (0x00406620) */
#define G_YARD_ORDERS_ALL_BUTTON 0x005C5BC8u /* storage yards' orders (0x00406680, p1 = 2) */
#define G_GRANARY_ORDERS_BUTTON 0x005C5A98u  /* @bin 0x004F9E99 the granary's orders button */
#define G_BAZAAR_ORDERS_BUTTON 0x005C5AB8u   /* @bin 0x004FA215 the bazaar's */
#define G_YARD_ORDERS_BUTTON 0x005C5AD8u     /* @bin 0x004FA0DD the storage yard's */
#define G_HOUSE_BUTTON 0x005C5A78u           /* @bin 0x004F9DE0 the houses' button */
#define G_PALACE_BUTTON 0x005C5188u          /* @bin 0x004F9FFF the palace's (0xD1) button */
#define G_MANSION_BUTTON 0x005C5448u         /* @bin 0x004F96E0 types 0x4D..0x4F (mansions, save.md) */
#define G_LEGION_LAYOUT_BUTTONS 0x005C66D8u  /* the legion's layout and return buttons (0x00406720, 0x00406FB0) */
#define G_LEGION_TOGGLE_BUTTON 0x005C6768u   /* the legion's formation +0x77 toggle (0x00407110) */
#define G_TRANSPORT_ORDER_BUTTONS 0x005C6788u /* 0x00406B40 */
#define G_WARSHIP_ORDER_BUTTONS 0x005C6818u  /* 0x00406C90 */
#define G_FIGURE_PORTRAIT_BUTTONS 0x005C5BE8u /* @bin 0x004F9C25 the figure list's portraits */
#define G_FIGURE_ORDER_BUTTON 0x005C5CB0u    /* the figure's order selection (0x0053D730) */
#define G_STORAGE_ORDER_BUTTONS 0x005C81F8u  /* present/ui/info/storage.c: the order rows' buttons, 0x48 bytes per row */
#define G_YARD_ORDERS_SCROLL_UP 0x005C5B48u  /* @bin 0x00502AB2 the storage yard orders' scroll buttons */
#define G_YARD_ORDERS_SCROLL_DOWN 0x005C5B60u

/* tables of the exe's data read by the panels */
#define G_TILE_NEIGHBOURS 0x005F76C4u   /* @bin 0x004F8655 i32[9] grid offsets: the tile and its 8 neighbours */
#define G_FIGURE_THUMB_OFFSETS 0x005F76E8u /* @bin 0x004FA3C0 i32[7] byte offsets of the thumbnails in the spare screen */
#define G_TRANSPORT_BUTTON_INDEX 0x005F78DCu /* @bin 0x0050F84F i32[5] {0, 1, 2, 4, 5}: the transport buttons' images */
#define G_SPEECH_NAMES 0x005FB16Cu      /* @bin 0x0053BFE0 24-byte speech file names, 20 per speech kind */
#define G_SPEECH_NAMES_6C 0x0060110Cu   /* @bin 0x0053BFE0 the same for kind 0x6C */
#define G_FIGURE_TEXT_GROUPS 0x006012ECu /* @bin 0x0053D280 i32 per figure type: its description's text group (< 0: by rule) */
#define G_FLOOD_MONTH 0x00D386D0u       /* @bin 0x00489240 i32 the flood's month */
#define G_UD38374 0x00D38374u           /* @bin 0x0053C8D7 i32 compared with 0x3C */
#define G_UD386DC 0x00D386DCu           /* @bin 0x0053C8D7 u8 tested with it */

/* image groups (i32: the group's first image) */
#define G_PORTRAIT_IMAGES 0x00F55C7Cu   /* @bin 0x004FA8C0 the figures' portraits, + type */
#define G_UF55C84 0x00F55C84u           /* @bin 0x004FA94D a portrait by climate (0x45) */
#define G_UF55C8C 0x00F55C8Cu           /* @bin 0x004FA8F3 portraits by the home's kind (+3, 2, 4, 1) */
#define G_UF571EC 0x00F571ECu           /* @bin 0x004FA8C0 portraits of types 0x18, 0x66..0x68 */
#define G_LEGION_BUTTON_IMAGES 0x00F55CC4u   /* @bin 0x0051089E the legion's buttons (types other than 0xB, 0xD) */
#define G_LEGION_BUTTON_IMAGES_B 0x00F55CCCu /* type 0xB */
#define G_LEGION_BUTTON_IMAGES_D 0x00F55CD4u /* type 0xD */
#define G_WARSHIP_BUTTON_IMAGES 0x00F55CDCu  /* @bin 0x0050FCA0 the warship's order buttons */
#define G_TRANSPORT_BUTTON_IMAGES 0x00F55CE4u /* @bin 0x0050F84F the transport ship's order buttons */
#define G_HEALTH_BAR_IMAGES 0x00F56B1Cu /* @bin 0x00510363 the legion's health bar, - (health + 3) / 5 + 20 */

/* strings of the exe's data */
#define G_STR_COMMA 0x005E0400u         /* @bin 0x0050EBBD "," */
#define G_STR_OR 0x005F7E74u            /* @bin 0x0050EBBD "or" */
#define G_STR_CLOSE_PAREN 0x005F7E78u   /* @bin 0x0050EA73 ")" */
#define G_STR_VOICE_WALKER 0x005F90A4u  /* @bin 0x00539A70 the speech directory "audio\voice\walker\"  */
#define G_PERCENT_STRING 0x005E03FCu    /* "%" (present/ui/frontend/options.c: the suffix of the volumes) */
/* @bin the panels' sounds (their open, present/ui/info/) */
#define G_WAV_EMPTY_LAND 0x005F792Cu /* "wavs\empty_land.wav" */
#define G_WAV_AMPITHEATRE 0x005F7940u /* "wavs\ampitheatre.wav" */
#define G_WAV_THEATRE 0x005F7958u /* "wavs\theatre.wav" */
#define G_WAV_HIPPODROME 0x005F796Cu /* "wavs\hippodrome.wav" */
#define G_WAV_COLLOSEUM 0x005F7980u /* "wavs\colloseum.wav" */
#define G_WAV_GLAD_PIT 0x005F7994u /* "wavs\glad_pit.wav" */
#define G_WAV_LION_PIT 0x005F79A8u /* "wavs\lion_pit.wav" */
#define G_WAV_ART_PIT 0x005F79BCu /* "wavs\art_pit.wav" */
#define G_WAV_CHAR_PIT 0x005F79D0u /* "wavs\char_pit.wav" */
#define G_WAV_PARK 0x005F79E4u /* "wavs\park.wav" */
#define G_WAV_WALL 0x005F79F4u /* "wavs\wall.wav" */
#define G_WAV_AQUADUCT 0x005F7A04u /* "wavs\aquaduct.wav" */
#define G_WAV_PLAZA 0x005F7A18u /* "wavs\plaza.wav" */
#define G_WAV_STATUE 0x005F7A28u /* "wavs\statue.wav" */
#define G_WAV_CLINIC 0x005F7A38u /* "wavs\clinic.wav" */
#define G_WAV_HOSPITAL 0x005F7A48u /* "wavs\hospital.wav" */
#define G_WAV_BATHS 0x005F7A5Cu /* "wavs\baths.wav" */
#define G_WAV_BARBER 0x005F7A6Cu /* "wavs\barber.wav" */
#define G_WAV_SCHOOL 0x005F7A7Cu /* "wavs\school.wav" */
#define G_WAV_LIBRARY 0x005F7A8Cu /* "wavs\library.wav" */
#define G_WAV_PREFECTURE 0x005F7AA0u /* "wavs\prefecture.wav" */
#define G_WAV_FORT 0x005F7AB4u /* "wavs\fort.wav" */
#define G_WAV_GATEHOUSE 0x005F7AC4u /* "wavs\gatehouse.wav" */
#define G_WAV_NOACCESS 0x005F7AD8u /* "wavs\noaccess.wav" */
#define G_WAV_TOWER 0x005F7AECu /* "wavs\tower.wav" */
#define G_WAV_MIL_ACAD 0x005F7AFCu /* "wavs\mil_acad.wav" */
#define G_WAV_BARRACKS 0x005F7B10u /* "wavs\barracks.wav" */
#define G_WAV_MARKET 0x005F7B24u /* "wavs\market.wav" */
#define G_WAV_GRANARY 0x005F7B34u /* "wavs\granary.wav" */
#define G_WAV_WAREHOUSE 0x005F7B48u /* "wavs\warehouse.wav" */
#define G_WAV_SHIPYARD 0x005F7B5Cu /* "wavs\shipyard.wav" */
#define G_WAV_DOCK 0x005F7B70u /* "wavs\dock.wav" */
#define G_WAV_WHARF 0x005F7B80u /* "wavs\wharf.wav" */
#define G_WAV_GOV_PALACE 0x005F7B90u /* "wavs\gov_palace.wav" */
#define G_WAV_ENG_POST 0x005F7BA4u /* "wavs\eng_post.wav" */
#define G_WAV_MISSION 0x005F7BB8u /* "wavs\mission.wav" */
#define G_WAV_SENATE 0x005F7BCCu /* "wavs\senate.wav" */
#define G_WAV_FORUM 0x005F7BDCu /* "wavs\forum.wav" */
#define G_WAV_WATERWHEEL 0x005F7BECu /* "wavs\waterwheel.wav" */
#define G_WAV_FOUNTAIN 0x005F7C00u /* "wavs\fountain.wav" */
#define G_WAV_WELL 0x005F7C14u /* "wavs\well.wav" */
#define G_WAV_ORACLE 0x005F7C24u /* "wavs\oracle.wav" */
#define G_WAV_TEMPLE_LOVE 0x005F7C34u /* "wavs\temple_love.wav" */
#define G_WAV_TEMPLE_WAR 0x005F7C4Cu /* "wavs\temple_war.wav" */
#define G_WAV_TEMPLE_COMM 0x005F7C60u /* "wavs\temple_comm.wav" */
#define G_WAV_TEMPLE_SHIP 0x005F7C78u /* "wavs\temple_ship.wav" */
#define G_WAV_TEMPLE_FARM 0x005F7C90u /* "wavs\temple_farm.wav" */
#define G_WAV_RUIN 0x005F7CA8u /* "wavs\ruin.wav" */
#define G_WAV_WHEAT_FARM 0x005F7CB8u /* "wavs\wheat_farm.wav" */
#define G_WAV_VEG_FARM 0x005F7CCCu /* "wavs\veg_farm.wav" */
#define G_WAV_FIGS_FARM 0x005F7CE0u /* "wavs\figs_farm.wav" */
#define G_WAV_OLIVES_FARM 0x005F7CF4u /* "wavs\olives_farm.wav" */
#define G_WAV_VINES_FARM 0x005F7D0Cu /* "wavs\vines_farm.wav" */
#define G_WAV_MEAT_FARM 0x005F7D20u /* "wavs\meat_farm.wav" */
#define G_WAV_QUARRY 0x005F7D34u /* "wavs\quarry.wav" */
#define G_WAV_MINE 0x005F7D44u /* "wavs\mine.wav" */
#define G_WAV_TIMBER 0x005F7D54u /* "wavs\timber.wav" */
#define G_WAV_CLAY 0x005F7D64u /* "wavs\clay.wav" */
#define G_WAV_WINE_WORKSHOP 0x005F7D74u /* "wavs\wine_workshop.wav" */
#define G_WAV_OIL_WORKSHOP 0x005F7D8Cu /* "wavs\oil_workshop.wav" */
#define G_WAV_LAMP_MAKER 0x005F7DA4u /* "wavs\lamp_maker.wav" */
#define G_WAV_PAINT_MAKER 0x005F7DB8u /* "wavs\paint_maker.wav" */
#define G_WAV_WEAPONS_WORKSHOP 0x005F7DD0u /* "wavs\weapons_workshop.wav" */
#define G_WAV_FURNITURE_WORKSHOP 0x005F7DECu /* "wavs\furniture_workshop.wav" */
#define G_WAV_POTTERY_WORKSHOP 0x005F7E08u /* "wavs\pottery_workshop.wav" */
#define G_WAV_CARPENTER_LODGE 0x005F7E24u /* "wavs\carpenter_lodge.wav" */
#define G_WAV_BRICKLAYER_LODGE 0x005F7E40u /* "wavs\bricklayer_lodge.wav" */
#define G_WAV_ARTISANS_GUILD 0x005F7E5Cu /* "wavs\artisans_guild.wav" */
#define G_WAV_HOUSING 0x005F7E7Cu /* "wavs\housing.wav" */
#define G_WAV_PYRAMID 0x005F7E90u /* "wavs\pyramid.wav" */
#define G_WAV_SPHINX 0x005F7EA4u /* "wavs\sphinx.wav" */
#define G_WAV_SUNTEMPLE 0x005F7EB4u /* "wavs\suntemple.wav" */
#define G_WAV_MAUSOLEUM 0x005F7EC8u /* "wavs\mausoleum.wav" */

/* named by other ports (harvested, checked against the uses here) */
#define G_SELECTED_Y 0x0070E3E0u /* present/city/figures.c */
#define G_SELECTED_X 0x0070E3E4u /* present/city/figures.c: i32 the screen position of the figure drawn by number */
#define G_CV_MAP_START 0x0070E484u /* present/city/city.h */
#define G_BLIT_Y 0x00D3B5DCu /* present/ui/info/figure.c: i32 the copy's destination y (military.c: the image draw's y) */
#define G_BLIT_X 0x00D3B5E0u /* present/ui/info/figure.c: i32 the copy's destination x */
#define G_TEXT_Y 0x00E20774u /* present/ui/messages/richtext.c: i32 the height laid out (+16 per line) */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
