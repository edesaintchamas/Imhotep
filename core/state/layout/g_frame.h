/* Imhotep - named globals: the frame, its input, the display and the window, the frame's sound and the
 * autosave (core/frame; docs/presentation.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P7). */
#ifndef IMHOTEP_STATE_LAYOUT_G_FRAME_H
#define IMHOTEP_STATE_LAYOUT_G_FRAME_H

/* ---- the frame: counters, pacing, timers (docs/presentation.md 1) ---- */
#define G_FRAME_COUNTER 0x00E390ECu     /* @bin 0x004D6B60 u32: += 1 each frame */
#define G_TICK_COUNTER 0x00E94DBCu      /* @bin 0x004D6B60 i32: += 1 before each tick */
#define G_TICK_LAST_TIME 0x00E94DB8u    /* @bin 0x004DDCB0 u32: timeGetTime when the last tick was due */
#define G_WINDOW_STOPS_TIME 0x005E1C50u /* @bin 0x004DDCB0 u32 per window id: non-zero, no tick */
#define G_SPEED_DELAYS 0x005E1DB4u      /* @bin 0x004DDCB0 i32[11]: ms between ticks by speed step (+ 2) */
#define G_GAME_SPEED 0x00E38E6Cu        /* @bin 0x00403320 i32: the speed option, 10..100 */
#define G_SCROLLING 0x00E38DCCu         /* @bin 0x004DDCB0 i32: the map scrolls (0x004CF660): no tick */
#define G_ANIM_FLAGS 0x00605B38u        /* @bin 0x0040F270 u8[51]: timer k fired this frame */
#define G_LAYOUT_MODE 0x00E94D98u       /* @bin 0x004D6070 i32: non-zero, a full redraw each frame */
/* @bin 0x004D6CB7 the movie the frame plays instead of the present: never set (docs/provenance/frontier.md
 * 0x005605A0, QuickTime glue) */
#define G_MOVIE 0x006FDC40u             /* @bin 0x004D6CB7 i32: a movie open (0x00414CD0 too) */
#define G_U6FDC38 0x006FDC38u           /* @bin 0x004D6CB7 i32: the argument of 0x005605A0 */

/* ---- the present (docs/presentation.md 1.3) ---- */
#define G_PRESENT_NOW 0x00E38DC8u       /* @bin 0x004D0280 u8: present at the next present call */
#define G_PRESENT_ELAPSED 0x00E92E08u   /* @bin 0x004D0280 i32: ms since the last present (50: due) */
#define G_PRESENT_LAST_TIME 0x00E36C88u /* @bin 0x004D0040 u32: timeGetTime of the last call */

/* ---- the mouse (docs/presentation.md 5.2) ---- */
#define G_CURSOR_POS_X 0x00E38DC4u      /* @bin 0x004CF210 i32: GetCursorPos */
#define G_CURSOR_POS_Y 0x00E38DC0u      /* @bin 0x004CF210 i32 */
#define G_MSG_MOUSE_X 0x00E38E0Au       /* @bin 0x00414CD0 i16: the last mouse message's client x */
#define G_MSG_MOUSE_Y 0x00E38E08u       /* @bin 0x00414CD0 i16: its y */
#define G_MOUSE_X 0x00E38E34u           /* @bin 0x004CF210 i32: the cursor scaled to the drawing size */
#define G_MOUSE_Y 0x00E38E30u           /* @bin 0x004CF210 i32 */
#define G_MOUSE_X_PREV 0x00E38E10u      /* @bin 0x004CF210 i32: G_MOUSE_X of the frame before */
#define G_MOUSE_Y_PREV 0x00E38E0Cu      /* @bin 0x004CF210 i32 */
#define G_DRAG_START_X 0x00E38E1Cu      /* @bin 0x004CF263 i32: a drag's start (G_DRAG_STATE 2 past 50 px) */
#define G_DRAG_START_Y 0x00E38E18u      /* @bin 0x004CF263 i32 */
#define G_MOUSE_MOVED 0x00E38E40u       /* @bin 0x004CF210 i32: moved or a button changed this frame */
#define G_BUTTON_CHANGED 0x00E38E38u    /* @bin 0x004CF210 i32: a button changed this frame */
#define G_MOUSE_STILL_TIME 0x00E38E3Cu  /* @bin 0x004CF3AA i32: ms since the last move */
#define G_MOUSE_MOVE_TIME 0x00E36C8Cu   /* @bin 0x004CF3AA i32: the frame clock at the last move */
#define G_BUTTONS 0x00E38E07u           /* @bin 0x00414CD0 u8: bit 0 left, bit 1 right */
#define G_LEFT_HELD 0x00E38E00u         /* @bin 0x004CF210 u8 */
#define G_LEFT_HELD_PREV 0x00E38DF9u    /* @bin 0x004CF210 u8: G_LEFT_HELD of the frame before */
#define G_LEFT_PRESSED 0x00E38E03u      /* @bin 0x004CF210 u8 */
#define G_LEFT_RELEASED 0x00E38E04u     /* @bin 0x004CF210 u8 */
#define G_LEFT_LATCH 0x00E38E02u        /* @bin 0x00414CD0 u8: WM_LBUTTONDOWN's click latch (0x004CF1F0 clears) */
#define G_RIGHT_HELD 0x00E38DF4u        /* @bin 0x004CF210 u8 */
#define G_RIGHT_HELD_PREV 0x00E38DEDu   /* @bin 0x004CF210 u8: G_RIGHT_HELD of the frame before */
#define G_RIGHT_PRESSED 0x00E38DF7u     /* @bin 0x004CF210 u8 */
#define G_RIGHT_RELEASED 0x00E38DF8u    /* @bin 0x004CF210 u8 */
#define G_UE38E06 0x00E38E06u           /* @bin 0x004CF210 u8 := 0 at the end */
#define G_UE38DF6 0x00E38DF6u           /* @bin 0x004CF1F0 u8: a one-frame flag cleared */
#define G_UE38DFC 0x00E38DFCu           /* @bin 0x004CF1F0 i32: a one-frame flag cleared */

/* ---- the window procedure: keys and window messages (docs/presentation.md 5.1, 5.3) ---- */
#define G_CTRL 0x00E92E20u              /* @bin 0x00414CD0 i32: Ctrl held (WM_KEYDOWN / WM_KEYUP) */
#define G_SHIFT 0x00E92E24u             /* @bin 0x00414CD0 i32: Shift held */
#define G_KEYDOWN_SEEN 0x00E38E4Eu      /* @bin 0x004158BA u8 := 1 by each WM_KEYDOWN */
#define G_DEBUG_OFF 0x00578C68u         /* @bin 0x004158BA u8: 1 in the exe, the debug keys are off */
#define G_ARROW_KEY_RIGHT 0x00E92E2Cu   /* @bin 0x004158BA i32 := 1 by the arrow key (present/ui/cityframe.c scrolls) */
#define G_ARROW_KEY_LEFT 0x00E92E30u    /* @bin 0x004158BA i32 */
#define G_ARROW_KEY_DOWN 0x00E92E34u    /* @bin 0x00415AB5 i32 */
#define G_ARROW_KEY_UP 0x00E92E38u      /* @bin 0x00415BBD i32 */
#define G_LEGION_KEY_LAST 0x00578C6Cu   /* @bin 0x00416A20 i32: the legion 'L' chose last */
#define G_MINIMAP_TOGGLE 0x00775680u    /* @bin 0x004158BA u8: toggled by F12 (present/sidebar/minimap.c) */
#define G_DEBUG_GRID 0x00E38E94u        /* @bin 0x004158BA i32: which grid the debug display shows */
#define G_QUIT_REQUEST 0x00D3B43Du      /* @bin 0x00414CD0 u8: 3 WM_CLOSE, 1 the quit box's yes */
#define G_HWND_CREATE 0x006FDC44u       /* @bin 0x00414CD0 u32: the window given by WM_CREATE */
#define G_MINIMISED 0x006FDC48u         /* @bin 0x00414CD0 i32: 1 after a minimising WM_ACTIVATE */
#define G_UE38DBC 0x00E38DBCu           /* @bin 0x00414CD0 i32: WM_DISPLAYCHANGE's wParam */
#define G_PAGE 0x00605A44u              /* @bin 0x00416990 i32: the advisor page shown in window 7 (1-14) */
#define G_OVERLAY 0x00E94E0Cu           /* @bin 0x004168E0 i32: the overlay shown (0: none) */
#define G_INFO_WARSHIP 0x00EADCC4u      /* @bin 0x004151A8 i32: the warship figure the order keys act on */
#define G_STR_SHOT1_BMP 0x0057C768u     /* @bin 0x0041D110 "shot1.bmp" */
#define G_STR_SHOT2_BMP 0x0057C774u     /* @bin 0x0041D110 "shot2.bmp" */

/* ---- the city view's geometry and the cell under a point (core/frame/view.c) ---- */
#define G_TILE_H 0x0070E44Cu            /* @bin 0x00430790 i32: the row step in pixels */
#define G_U70E458 0x0070E458u           /* @bin 0x00430790 i32: (+ 2) * G_VIEW_COLS: the view's width */
#define G_VIEW_ROWS 0x0070E460u         /* @bin 0x00430790 i32: the view's rows */
#define G_VIEW_COLS 0x0070E464u         /* @bin 0x00430790 i32: the view's columns */
#define G_VIEW_FIRST_ROW 0x0070E474u    /* @bin 0x004308B7 i32: the view's first row in G_VIEW_TABLE */
#define G_VIEW_FIRST_COL 0x0070E478u    /* @bin 0x004308B7 i32: its first column */
#define G_VIEW_Y 0x0070E48Cu            /* @bin 0x00430790 i32: the screen y of the first row */
#define G_VIEW_X 0x0070E490u            /* @bin 0x00430790 i32: the screen x of the first column */
#define G_VIEW_CELL_ENTRY 0x0070E3F8u   /* @bin 0x00430790 i32: the G_VIEW_TABLE entry under the point */
#define G_VIEW_CELL_ROW_EDGE 0x0070E3FCu /* @bin 0x00430790 i32: 2 the first row, 1 past the last */
#define G_VIEW_CELL_COL_EDGE 0x0070E400u /* @bin 0x00430790 i32: odd rows: 2 the first column, 1 past the last */
#define G_VIEW_CELL_ROW 0x0070E404u     /* @bin 0x00430790 i32: the view row under the point */
#define G_VIEW_CELL_COL 0x0070E408u     /* @bin 0x00430790 i32: the view column under the point */

/* ---- tooltips the frame tests ---- */
#define G_TIP_KIND 0x00D3A6C0u          /* @bin 0x004D6B60 i32: 0x23 the hovered tile is updated */
#define G_TIP_WINDOW 0x00D3A6C4u        /* @bin 0x004DDD17 i32: 1 lets windows 0x12 / 0x23 tick */
#define G_TIP_ID 0x00D3A6E4u            /* @bin 0x004D6B60 i32: the tooltip's text (group 0x44), 0: none */

/* ---- the display (core/frame/display.c, docs/presentation.md 2.1) ---- */
#define G_SCREEN_W 0x00E38DB0u          /* @bin 0x004CFD60 i32: the mode's width (G_SCREEN_H) */
#define G_SCREEN_PITCH 0x00E38D94u      /* @bin 0x004CFDC0 i32: the back buffer's pitch in bytes */
#define G_DRAW_H 0x00E38D98u            /* @bin 0x004CFDC0 i32: the drawing height (width G_UE38D9C) */
#define G_FULLSCREEN 0x00E38E5Cu        /* @bin 0x00426B40 i32: the option, fullscreen (1) or windowed */
#define G_CLIENT_RECT 0x00E39130u       /* @bin 0x00414E8A i32[4]: left, top, right, bottom of the client area */
#define G_DDRAW 0x00E39148u             /* @bin 0x00426EE0 the DirectDraw object */
#define G_DD_BACK 0x00E39174u           /* @bin 0x00426EE0 the back buffer surface */
#define G_DD_PRIMARY 0x00E39178u        /* @bin 0x00426EE0 the primary surface */
#define G_PIXEL_FORMAT 0x00E92DFCu      /* @bin 0x00426FF0 0: RGB 555, 2: RGB 565 */
#define G_BACK_BUFFER 0x00E92E18u       /* @bin 0x00426FF0 u32: the back buffer's pixels (16 bits) */
#define G_FRAMES_STOPPED 0x00E39128u    /* @bin 0x004CFF90 i32: 1 paused (minimised), 0 runs (0x004CFFC0) */
#define G_RESOLUTION_ASK_TIME 0x006057E8u /* @bin 0x00402DA0 u32: timeGetTime when window 0x3E opened */
#define G_EXTERNAL_IMAGES 0x01265170u   /* @bin 0x00414BD0 the external image buffer (0x0053F790 releases) */
#define G_UE36C80 0x00E36C80u           /* @bin 0x00414750 u32: a block freed at the end (present/draw/footprint.c adds it) */
#define G_TEXT_BUFFER 0x00E36C70u       /* @bin 0x00414750 u32: a text block deleted at the end (0x004CEC40) */
#define G_UEA1368 0x00EA1368u           /* @bin 0x004E63B0 an object deleted at the end (0x0055D960) */
#define G_DEBUG_ANIM1 0x00EA136Cu       /* @bin 0x004E63B0 debug animation object, never set (frontier.md 0x0055D290) */
#define G_DEBUG_ANIM2 0x00EA1370u       /* @bin 0x004E63B0 the second one */
#define G_SMACK_HANDLE 0x00F54114u      /* @bin 0x005391E0 the Smacker video (SmackClose) */
#define G_UF5400C 0x00F5400Cu           /* @bin 0x005391E0 the argument of 0x004CE100 (the CD path restored) */
#define G_UF5404C 0x00F5404Cu           /* @bin 0x005391E0 u32: non-zero, the CD path is not restored */
/* the platform's DirectDraw objects: handles, not memory (core/frame/display.c) */
#define G_HANDLE_DDRAW 0x00DD0001u      /* @bin 0x00426B40 the DirectDraw object's handle */
#define G_HANDLE_PRIMARY 0x00DD0002u    /* @bin 0x00426FF0 */
#define G_HANDLE_CLIPPER 0x00DD0003u    /* @bin 0x00426FF0 */
/* the logs */
#define G_STR_REMOVED_MANMADES 0x0057925Cu /* @bin 0x00414750 "OK :removed manmades=" */
#define G_STR_DD_REINIT 0x005792B0u     /* @bin 0x00414F86 "ERR:Re-initializing direct draw.  " */
#define G_STR_NOT_ACTIVE 0x005792D4u    /* @bin 0x00414F86 "OK :Not active.  " */
#define G_STR_ACTIVE 0x005792E8u        /* @bin 0x00414F86 "OK :Active.  " */
#define G_STR_DD_256_COLS 0x005D1CD4u   /* @bin 0x00426B40 "OK :DD 256 cols switching to full screen." */
#define G_STR_DD_RELEASED 0x005D1D00u   /* @bin 0x00426EE0 "OK :DD Released." */
#define G_STR_DD_FORMAT_CHANGED 0x005D1D34u /* @bin 0x004273EF "ERR: 16-bit surface format changed from %d to %d" */
#define G_STR_DD_EXTRA_SURFACE_FAILED 0x005D1FDCu /* @bin 0x00426FF0 "ERR:DD ExtraSurface failed." */
#define G_STR_EXITED_NORMALLY 0x005E074Cu /* @bin 0x004CFF50 "OK :Exited Normally  " */
#define G_STR_PAUSED 0x005E0764u        /* @bin 0x004CFF90 "OK :Paused.  " */
#define G_STR_NO_RESTORE_FRONT 0x005E0774u /* @bin 0x004CFFE0 "ERR:Can't restore front buffer.  " */
#define G_STR_NO_RESTORE_BACK 0x005E0798u /* @bin 0x004CFFE0 "ERR:Can't restore back buffer.  " */
#define G_STR_OLD_RES 0x005E0804u       /* @bin 0x004D00F8 "Old res=%d,%d; Color mode=%d" */
#define G_STR_DEFAULT_MODE 0x005E0824u  /* @bin 0x004D0060 "ERR:Trying default display mode" */
#define G_STR_GFX_NO_FLIPPING 0x005E0844u /* @bin 0x004D0060 "ERR:GFX couldn't create flipping surface." */

/* ---- the frame's sound (core/frame/sound.c, docs/presentation.md 6) ---- */
#define G_SOUND 0x00E92BF8u             /* @bin 0x0053B6F0 the sound object (thiscall) */
#define G_SOUND_ON 0x00F55B88u          /* @bin 0x0053B6F0 i32: sound system initialised (0x005394F0) */
#define G_CITY_SOUNDS_ON 0x00E38EC0u    /* @bin 0x0053B960 u8: option, city sounds */
#define G_MUSIC_VOLUME 0x00E38EC8u      /* @bin 0x0053B790 i32: option, music volume */
#define G_CITY_VOLUME 0x00E38ED0u       /* @bin 0x0053B790 i32: option, city sounds volume */
#define G_AMBIENT_COUNTERS 0x00F55B50u  /* @bin 0x0053B670 i32[12]: the ground pass's ambient counters */
#define G_AMBIENT_TOTAL 0x00F55B84u     /* @bin 0x0053B670 i32: cleared with them (0x0053BAC3 compares) */
#define G_AMBIENT_SLOT_BY_TYPE 0x0057C780u /* @bin 0x0041D41F i32 per building type: its ambient slot */
#define G_AMBIENT_SOUND 0x007028F8u     /* @bin 0x0041D610 i32: the slot's sound chosen */
#define G_STREAM_FADE0 0x00F55B8Cu      /* @bin 0x0053B6F0 i32: stream 0 (music) fades out */
#define G_STREAM_FADE1 0x00F55B90u      /* @bin 0x0053B6F0 i32: stream 1 fades out */
#define G_MUSIC_WAIT 0x00F55B98u        /* @bin 0x00539875 i32: the wait after a tune in ms */
#define G_BATTLE_MUSIC 0x00E92BF0u      /* @bin 0x004D6ECF i32: 1 while the battle music plays */
#define G_U5F9058 0x005F9058u           /* @bin 0x0053BBDD i32: >= 1000 monument_f, else monument_b */
#define G_SOUNDS_39_SUB0D 0x00F54990u   /* @bin 0x0053AD9B i32[4] + count: sounds of type 0x39 with +0x12 0x0D */
#define G_SOUNDS_39_SUB0B 0x00F549E0u   /* @bin 0x0053AD9B the same for +0x12 0x0B */
#define G_SOUNDS_39_OTHER 0x00F549F4u   /* @bin 0x0053AD9B the same for the others */
#define G_STR_BATTLE_MP3 0x005E1DE0u    /* @bin 0x004D6ECF "audio\music\battle.mp3" */
/* @bin 0x0053BC48 the ambient streams' files */
#define G_STR_AMBIENT_MONUMENT_F 0x005FAB58u  /* "audio\ambient\monument_f.mp3" */
#define G_STR_AMBIENT_MONUMENT_B 0x005FAB78u  /* "audio\ambient\monument_b.mp3" */
#define G_STR_AMBIENT_HOUSING5 0x005FAB98u    /* "audio\ambient\housing5.mp3" */
#define G_STR_AMBIENT_HOUSING4 0x005FABB4u    /* "audio\ambient\housing4.mp3" */
#define G_STR_AMBIENT_HOUSING3 0x005FABD0u    /* "audio\ambient\housing3.mp3" */
#define G_STR_AMBIENT_HOUSING2 0x005FABECu    /* "audio\ambient\housing2.mp3" */
#define G_STR_AMBIENT_HOUSING1 0x005FAC08u    /* "audio\ambient\housing1.mp3" */
#define G_STR_AMBIENT_INDUSTRIAL 0x005FAC24u  /* "audio\ambient\industrial.mp3" */
#define G_STR_AMBIENT_WATER 0x005FAC44u       /* "audio\ambient\water.mp3" */
#define G_STR_AMBIENT_VEGETATION 0x005FAC5Cu  /* "audio\ambient\vegetation.mp3" */
#define G_STR_AMBIENT_ROCK 0x005FAC7Cu        /* "audio\ambient\rock.mp3" */
#define G_STR_AMBIENT_FLOODPLAIN 0x005FAC94u  /* "audio\ambient\floodplain.mp3" */
#define G_STR_AMBIENT_MEADOW 0x005FACB4u      /* "audio\ambient\meadow.mp3" */
#define G_STR_AMBIENT_DESERT 0x005FACD0u      /* "audio\ambient\desert.mp3" */

/* ---- the autosave and the saver (core/frame/autosave.c) ---- */
#define G_FILE_SCHEMA 0x00E92E50u       /* @bin 0x004D4E20 chunk 6: 0x5DC dwords */
#define G_WORKSPACE 0x01265188u         /* @bin 0x0055BC50 the compressor's globals: output pointer, size, input size, crc */
#define G_STR_AUTOSAVE_HISTORY 0x005E1BFCu /* @bin 0x004D5C79 "autosave_history.sav" */
#define G_STR_AUTOSAVE_SAV 0x005E1C14u  /* @bin 0x004D5C79 "autosave.sav" */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
