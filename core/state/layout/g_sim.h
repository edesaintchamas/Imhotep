/* Imhotep - named globals: the tick, the save / load machinery and the platform calls of core/sim.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P5). */
#ifndef IMHOTEP_STATE_LAYOUT_G_SIM_H
#define IMHOTEP_STATE_LAYOUT_G_SIM_H

/* the calendar and the tick (docs/formats/tick.md, save.md chunks 34-38) */
#define G_TICK 0x0078D7CCu /* @bin 0x00451C50 i32: tick within the day, 0..50 (chunk 34) */
#define G_U78D784 0x0078D784u /* @bin 0x00452150 i32 := 0 at tick 9 (no reader known) */
#define G_UF53FD0 0x00F53FD0u /* @bin 0x00451C50 i32 := 1 at ticks 3 and 0x1E (cleared by the sidebar) */

/* @bin 0x004CDC70 ten cycling animation counters, i32, named by their period */
#define G_ANIM_COUNTER_768 0x00D3B40Cu
#define G_ANIM_COUNTER_384 0x00D3B410u
#define G_ANIM_COUNTER_256 0x00D3B414u
#define G_ANIM_COUNTER_128 0x00D3B418u
#define G_ANIM_COUNTER_64 0x00D3B41Cu
#define G_ANIM_COUNTER_32 0x00D3B420u
#define G_ANIM_COUNTER_24 0x00D3B424u
#define G_ANIM_COUNTER_16 0x00D3B428u
#define G_ANIM_COUNTER_8 0x00D3B42Cu
#define G_ANIM_COUNTER_4 0x00D3B430u

/* timers counted down by the tick (0x00451C50; save.md chunks 40, 46, 105-107, 131) */
#define G_LOCUST_TIMER 0x0078D79Cu /* @bin 0x004E6EB0 i32: locust plague (-1 armed) */
#define G_HAIL_TIMER 0x0078D7A0u   /* @bin 0x004E6ED0 i32: hailstorm (408 ticks; counted in window 1 only) */
#define G_BLOOD_RIVER_DURATION 0x0078D7ACu /* @bin 0x004E7200 i32: river of blood duration */
#define G_BLOOD_RIVER_LEFT 0x0078D7B0u /* @bin 0x0045203E i32: counts down from G_BLOOD_RIVER_DURATION, the
                                        * water redrawn column by column (chunk 131) */
#define G_GOD_CURSE_ON 0x00EAAF5Cu /* @bin 0x004D41E0 u8 := G_PYRAMID_CURSE_COUNTDOWN > 0 (not saved); 0x004F3890 runs
                                    * every tick while set */
#define G_UF55B94 0x00F55B94u      /* @bin 0x00451FCC i32 := 0 when a timer stops the sound stream of channel 2
                                    * (:= 1 when the ambient track starts: core/city/events_actions.c) */

/* the month's autosave (0x00452370) */
#define G_AUTOSAVE_OPTION 0x00E38EC2u /* @bin 0x00452370 u8: 1 = "last.sav" written each month */
#define G_STR_LAST_SAV 0x005DBB94u /* @bin 0x00452370 "last.sav" */

/* storage settings (chunk 113): G_STORAGE + 0xC4 * id, ids 1..199; the building's BLD_STORAGE_ID holds
 * the id (@bin 0x0046B100, 0x0046B320) */
#define G_STORAGE 0x007900B8u /* @bin 0x00456760 196-byte records (save.md chunk 113) */
#define G_STORAGE_HIGHEST 0x0078D788u /* @bin 0x0046B100 i32: highest storage id in use */
#define STORAGE_BUILDING 0x04u /* i16: the building */
#define STORAGE_IN_USE 0x08u   /* u8 */
#define STORAGE_ACCEPTED 0x0Au /* u8 per resource */
#define STORAGE_U34 0x34u      /* i16 per resource: 0xC80 for types 0x47 and 0x48 at creation */
#define STORAGE_U7C 0x7Cu      /* i16 per resource: same */

/* records of type-0x4A buildings: G_REC4A + 0x1C * id, ids 1..10, the building's BLD_UCB holds the id
 * (@bin 0x0046A280, 0x0046A320); lists of core/sim/queue.c, packed into chunk 140 on save */
#define G_REC4A 0x009783A4u
#define REC4A_BUILDING 0x18u /* i32: the building */

/* the load and save machinery (core/state) */
#define G_CHUNKS 0x005E09A0u         /* @bin 0x004D4E20 the save schema: 181 records of 20 bytes (+0: compressed) */
#define CHUNK_ADDRESS 0x04u          /* u32: the variable */
#define CHUNK_SIZE 0x08u             /* u32: the element size */
#define CHUNK_COUNT 0x0Cu            /* u32: the element count */
#define G_CAMPAIGN_ARCHIVES 0x005E1B98u /* @bin 0x004D58B0 char[20] per archive: mission1.pak .. (docs/formats/campaign.md) */
#define G_CITY_PLAYER_NAME 0x00EA184Cu /* @bin 0x004D5100 char[32]: player 1's name (chunk 30), := 0x00E38E74 */
#define G_SAVE_LOADED 0x00D3B1B8u    /* @bin 0x004D5100 u8: := 0 before a save file is read, 1 after */
#define G_PKWARE_DICTIONARY 0x00603650u /* @bin 0x0055BC50 i32: the compressor's dictionary size */
#define G_STR_MAPS 0x005E17CCu       /* @bin 0x004D4E20 "MAPS" */
/* written by the scenario start 0x004D52A0, meaning open */
#define G_U7027A8 0x007027A8u        /* @bin 0x00417E70 i32 := 1 */
#define G_UEAAC2C 0x00EAAC2Cu        /* @bin 0x004D52A0 u8 := 1 */
#define G_U78D828 0x0078D828u        /* @bin 0x004D52A0 i32 := 0 for a custom map (chunk 100: set by campaign missions
                                      * 0-4, blocks screen 0x14) */

/* the platform calls of core/sim/platform.c */
#define G_VIDEO_PLAYING 0x00F54054u  /* @bin 0x00539170 i32: != 0 while a video plays (set by the player's start) */
#define G_VIDEO_WINDOW 0x00F53FFCu   /* @bin 0x00539220 u8: the window id the video start saved */
#define G_PRESENT_WAIT 0x005E06B0u   /* @bin 0x00413690 i32: 1 while the display mode switches around a video */
#define G_SOUND_DRIVER 0x012663ACu   /* @bin 0x00413594 u32: the sound driver (0: none) */
#define G_BINK_PLAYER 0x006FDC54u        /* @bin 0x00413690 u32: the global object the modal video player uses */
#define G_SOUND_EFFECTS_ON 0x00E38E63u /* @bin 0x0053B090 u8: the sound effects option */
#define G_EFFECTS_VOLUME 0x00E38EC4u /* @bin 0x0053B090 i32: the effects volume */
#define G_REQUEST 0x00E92DA8u        /* @bin 0x0053B5A9 the sample request block passed to 0x0055F0E8 */
#define REQUEST_SOUND 0x04u          /* i32: the sample (the owner at +0: -1) */
#define REQUEST_VOLUME 0x08u         /* i32 */
#define REQUEST_PAN 0x0Cu            /* i32 */
#define REQUEST_LOOPS 0x10u          /* i32 */
#define G_MESSAGE_BOX_PRESSED 0x00F535C0u /* @bin 0x00538920 u8: a button was pressed */
#define G_MESSAGE_BOX_BUTTON 0x00F535C4u /* @bin 0x00538920 i32: the pressed button's index */
#define G_MESSAGE_BOX_COUNT 0x00F535C8u  /* @bin 0x00538940 i32: the message box's button count */
/* @bin 0x0053B4DF the sidebar's x: the city view's width (present/sidebar/sidebar.h) */
#define G_SIDEBAR_X_COLLAPSED 0x00F53F60u /* i32 x of the collapsed sidebar */
#define G_SIDEBAR_X 0x00F53F64u /* i32 x of the expanded sidebar (0x0051E930) */
#define G_SIDEBAR_COLLAPSED 0x00F53F78u /* i32 0 expanded, 1 collapsed */
#define G_UF535BC 0x00F535BCu        /* @bin 0x00516F90 i32: += 1 for each building flag cleared without a building
                                      * (also read by core/frame/display.c, present/ui/debug_panels.c) */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_IMGS_H 0x010D6A0Eu /* present/ui/messages/richtext.c */
/* --- end harvested --- */

#endif
