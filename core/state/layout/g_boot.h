/* Imhotep - named globals: core/state/layout/g_boot.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P6). */
#ifndef IMHOTEP_STATE_LAYOUT_G_BOOT_H
#define IMHOTEP_STATE_LAYOUT_G_BOOT_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_STR_CAMPAIGN 0x00578D98u /* core/newgame/flow2.c: "campaign.txt" */
#define G_FILES_LIST 0x006057C8u /* present/ui/frontend/files.c: the dialog's file list object [generic] */
#define G_MONUMENT_FLAG 0x006DE514u /* present/city/ghost.c: u8 1: the type is a monument of 0xE5, 0xEA, 0xEB, 0xEC */
#define G_STRINGS_THIS 0x00784F20u /* present/ui/messages/expand.c: @bin 0x00448344 `this` of 0x004CBDB0: +4 entries, +0x0C count */
#define G_KEYWORDS_THIS 0x00784F34u /* present/ui/messages/expand.c: @bin 0x00448358 `this` of 0x004CB980: [this] = the keyword table */
#define G_TEXT_OUTLINE 0x00D3B510u /* present/draw/text.c: i32 != 0: plain (the other styles off) */
#define G_TEXT_SHADOW_V 0x00D3B518u /* present/draw/text.c: i32 != 0: black copies one pixel above and below */
#define G_CLIP_BOTTOM 0x00D3B550u /* present/ui/advisors/b_population.c: i32 the clip rectangle's bottom */
#define G_CLIP_TOP 0x00D3B554u /* present/ui/messages/window.c */
#define G_CLIP_RIGHT 0x00D3B558u /* present/ui/messages/window.c */
#define G_CLIP_LEFT 0x00D3B55Cu /* present/ui/messages/window.c */
#define G_LAST_NAME 0x00E370BCu /* present/ui/frontend/files.c: char[] the last saved game's name */
#define G_FILES_NAME 0x00E37100u /* present/ui/frontend/files.c: char[0x40] the typed saved game's name (field 2) [generic] */
#define G_UE38D9C 0x00E38D9Cu /* present/ui/empire/empire.c: i32 the screen width (set-up copy) [unresolved] */
#define G_SCREEN_H 0x00E38DACu /* present/sidebar/sidebar.h: i32 */
#define G_DESKTOP_H 0x00E38DB4u /* core/frame/display.c */
#define G_DESKTOP_W 0x00E38DB8u /* core/frame/display.c: the screen's size (GetSystemMetrics, WM_DISPLAYCHANGE) */
#define G_DRIVE 0x00E38E62u /* core/boot/sound.c: the CD drive's letter */
#define G_SPEECH_ON 0x00E38E65u /* present/ui/frontend/options.c: u8 */
#define G_RESOLUTION 0x00E38E68u /* core/frame/display.c: i32 1: 640 x 480, 2: 800 x 600, 3: 1024 x 768 */
#define G_SCROLL_SPEED 0x00E38E70u /* present/ui/frontend/options.c: i32 */
#define G_DD_READY 0x00E390E0u /* core/frame/display.c: the set-up succeeded */
#define G_HWND 0x00E39140u /* core/boot/display.c */
#define G_UE3914C 0x00E3914Cu /* core/frame/display.c: 0x555, 0x565, 0x556, 0x655 [unresolved] */
#define G_DD_CLIPPER 0x00E39170u /* core/boot/display.c */
#define G_MUSIC 0x00E39180u /* core/boot/sound.c: the music tables */
#define G_SPARE_SCREEN 0x00E92E1Cu /* present/ui/info/figure.c: i32 the spare screen's address */
#define G_FAMILY_LIST 0x00EADFA8u /* core/newgame/flow2.h */
#define G_BUILDING_SOUNDS 0x00F5467Cu /* present/ui/info/info.c: i32 per type (stride 0x14): the sound of the type's panel, -1: none */
/* --- end harvested --- */

#endif
