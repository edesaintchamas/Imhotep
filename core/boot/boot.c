/* Imhotep - the game's start-up: WinMain's initialisation (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "boot/boot.h"

#include <string.h>

#include "boot/boot_internal.h"
#include "command/models.h"
#include "newgame/flow.h"
#include "newgame/reset.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

/* ---- the steps, in WinMain's order (@bin 0x00413ED0) ---- */

typedef enum {
    STEP_CTORS,      /* the C++ static constructors (CRT _initterm over 0x00576000 .. 0x0057607C) */
    STEP_SETTINGS,   /* 0x004147F0 the settings: defaults, Pharaoh.ini, Pharaoh.inf */
    STEP_TEXT,       /* 0x004CEC40 Pharaoh_Text.eng and Pharaoh_MM.eng */
    STEP_CD,         /* 0x004268C0 the CD drive's letter */
    STEP_NAMES,      /* 0x004149D0 default names from the text */
    STEP_DISPLAY,    /* 0x004CF960 DirectDraw and the window (display boots only) */
    STEP_GRAPHICS,   /* 0x00414420 the graphics slots */
    STEP_MODEL_B,    /* 0x00540C90 the building model */
    STEP_MODEL_F,    /* 0x00540ED0 the figure model */
    STEP_MODEL_T,    /* 0x00541160 the tax model */
    STEP_CAMPAIGN,   /* 0x0041AE40 campaign.txt */
    STEP_541310,     /* 0x00541310 */
    STEP_44EFA0,     /* 0x0044EFA0 */
    STEP_SOUND,      /* 0x005394F0 the sound start-up (sound boots only) */
    STEP_51F780,     /* 0x0051F780 (returns at once) */
    STEP_GLOBALS,    /* the globals WinMain sets before 0x00540B50 */
    STEP_VIEW,       /* 0x00540B50 the view, the random generator, the text boxes */
    STEP_MON_0,      /* the monument objects' set-ups (8 steps) */
    STEP_MON_7 = STEP_MON_0 + 7,
    STEP_541D50,     /* 0x00541D50 */
    STEP_4DEE10,     /* 0x004DEE10 */
    STEP_GLOBALS2,   /* 0x006FDCB8 := 0 */
    STEP_SIDEBAR,    /* 0x0051E930 the sidebar's x */
    STEP_GLOBALS3,   /* the first window (0x44) and a redraw */
    STEP_4CF8E0,     /* 0x004CF8E0 */
    STEP_VIDEO,      /* the Bink player's object (no-display boots: 0x004CF960 makes it otherwise) */
    STEP_COUNT
} step_id;

static const char *const step_names[STEP_COUNT] = {
    "initializers", "0x004147F0", "0x004CEC40", "cd", "0x004149D0", "display", "0x00414420",
    "0x00540C90", "0x00540ED0", "0x00541160", "0x0041AE40", "0x00541310", "0x0044EFA0", "0x005394F0",
    "0x0051F780", "globals", "0x00540B50", "0x004013F0", "0x0040D7E0", "0x00416F60", "0x004E6590",
    "0x005456A0", "0x00558B40", "0x00555DE0", "0x0054F3D0", "0x00541D50", "0x004DEE10", "globals2",
    "0x0051E930", "globals3", "0x004CF8E0", "video",
};

/* the steps of a boot: the display step only with a display (and then no video step: the display
 * set-up makes the Bink object), the sound step only with sound (tools/oracle/game.py boot_steps) */
static int step_list(const boot_display *display, int sound, step_id *out)
{
    int with_display = display && display->width > 0;
    int n = 0;
    for (int s = 0; s < STEP_COUNT; s++) {
        if ((s == STEP_DISPLAY && !with_display) || (s == STEP_VIDEO && with_display) ||
            (s == STEP_SOUND && !sound)) {
            continue;
        }
        out[n++] = (step_id) s;
    }
    return n;
}

int boot_step_count(const boot_display *display, int sound)
{
    step_id list[STEP_COUNT];
    return step_list(display, sound, list);
}

const char *boot_step_name(const boot_display *display, int sound, int step)
{
    step_id list[STEP_COUNT];
    int n = step_list(display, sound, list);
    return step >= 0 && step < n ? step_names[list[step]] : NULL;
}

/* the C++ static constructors: CRT _initterm over the table 0x00576000 .. 0x0057607C (@bin 0x005646C0
 * _cinit; the C table 0x00576080 .. holds CRT internals: the x87 precision, 53 bits (0x005640EC), which
 * core's integer arithmetic has no use for, and the stdio set-up, the CRT's own state: core reads files
 * through the platform). Each entry is a game function: the table is read from the state, as _initterm
 * reads it, and each function runs through sim_call (its port). */
static void step_ctors(game_state *st, const boot_host *host)
{
    for (uint32_t p = G_CTOR_TABLE; p < G_CTOR_TABLE_END; p += 4) {
        uint32_t fn = U32(st, p);
        if (fn) {
            boot_ctor(st, host, fn);
        }
    }
}

static int32_t screen_width(const boot_display *display)
{
    if (display && display->screen_width) {
        return display->screen_width;
    }
    return display && display->width > 0 ? display->width : 1024;
}

static int step_run(game_state *st, const boot_host *host, const boot_display *display, int sound, step_id s)
{
    (void) sound;
    uint32_t before = st->unported;
    int32_t r = 1;
    switch (s) {
    case STEP_CTORS:
        step_ctors(st, host);
        break;
    case STEP_SETTINGS:
        /* @bin 0x00413F09 WinMain clears 0x44 dwords at 0x006FDB38 and the byte 0x00E38DD6 first */
        memset(st->mem + (G_U6FDB38 - STATE_BASE), 0, 0x44 * 4);
        U8(st, G_UE38DD6) = 0;
        r = boot_settings(st, host, screen_width(display)); /* @bin 0x00413F25 */
        break;
    case STEP_TEXT:
        /* @bin 0x00413F52 "Pharaoh_Text.eng" (0x00579044), "Pharaoh_MM.eng" (0x00579058) */
        r = boot_text_load(st, host, "Pharaoh_Text.eng", "Pharaoh_MM.eng");
        break;
    case STEP_CD:
        /* @bin 0x00413EFE WinMain's drive letter starts as 'D'; the CD search 0x004268C0 changes it
         * only when it finds the game's disc (its five files with their sizes). The GOG build ignores
         * the search's result (dis 0x00413F9C: an unconditional jump to 0x00414000), which stores the
         * letter in 0x00E38E62. assumed: no disc (tools/oracle/game.py 'cd' does the same) */
        U8(st, G_DRIVE) = 0x44;
        break;
    case STEP_NAMES:
        boot_names(st); /* @bin 0x00414009 */
        break;
    case STEP_DISPLAY:
        r = boot_display_setup(st, host, display); /* @bin 0x0041401E */
        break;
    case STEP_GRAPHICS:
        /* @bin 0x00414066 0: loaded; 1, 2, 3, -2: WinMain logs the error and quits */
        r = boot_graphics(st, host);
        r = r == 1 || r == 2 || r == 3 || r == -2 ? 0 : 1;
        break;
    case STEP_MODEL_B:
        r = model_buildings_load(st); /* @bin 0x004140CD */
        break;
    case STEP_MODEL_F:
        r = model_figures_load(st); /* @bin 0x004140E2 */
        break;
    case STEP_MODEL_T:
        r = model_tax_load(st); /* @bin 0x004140F7 */
        break;
    case STEP_CAMPAIGN: {
        /* @bin 0x0041410F campaign_load("campaign.txt", 0x00578D98) on the table 0x006FDD08 (its low
         * byte tested); the path is a string of the state */
        r = campaign_load(st, G_CAMPAIGN_TABLE, G_STR_CAMPAIGN) & 0xFF;
        break;
    }
    case STEP_541310:
        r = boot_541310(st, host); /* @bin 0x00414134 */
        break;
    case STEP_44EFA0:
        r = boot_44efa0(st, host) & 0xFF; /* @bin 0x0041414C */
        break;
    case STEP_SOUND:
        boot_sound_start(st, host); /* @bin 0x00414167 */
        break;
    case STEP_51F780:
        GAME_CALL0(st, FN_NOTHING); /* @bin 0x0041416C */
        break;
    case STEP_GLOBALS:
        /* @bin 0x00414180 .. 0x004141C4 */
        S32(st, G_EDITOR) = 0;
        S32(st, G_U605A38) = 0;
        S32(st, G_U605A34) = 2;
        S32(st, G_EDITOR_TOOL) = 0;
        U8(st, G_NO_GROUND) = 0;
        U8(st, G_MONUMENT_FLAG) = 0;
        S32(st, G_FREE_BUILD) = 0;
        S32(st, G_DEBUG_EVENT) = 0;
        S32(st, G_DEBUG_EVENT_LEVEL) = 1;
        S32(st, G_DEBUG_EVENT_KIND) = 0;
        S32(st, G_NAME_PICK) = -1;
        break;
    case STEP_VIEW:
        newgame_view_reset(st); /* @bin 0x004141CE */
        break;
    case STEP_GLOBALS2:
        S32(st, G_CARRY) = 0; /* @bin 0x0041422D */
        break;
    case STEP_SIDEBAR:
        GAME_CALL0(st, FN_SIDEBAR_SET_X); /* @bin 0x00414233 (present/sidebar) */
        break;
    case STEP_GLOBALS3:
        U8(st, G_WINDOW) = 0x44; /* @bin 0x00414238 the first window */
        U8(st, G_REDRAW) = 1;    /* @bin 0x0041423F a redraw */
        break;
    case STEP_4CF8E0:
        boot_4cf8e0(st); /* @bin 0x00414246 */
        /* @bin 0x0041424B WinMain marks the start-up done (tools/oracle/game.py does not: docs) */
        U8(st, G_UE38DD6) = 1;
        break;
    case STEP_VIDEO:
        r = boot_bink_object(st, host);
        break;
    case STEP_541D50:
        boot_541d50(st); /* @bin 0x00414223 */
        break;
    case STEP_4DEE10:
        boot_4dee10(st); /* @bin 0x00414228 */
        break;
    default:
        if (s >= STEP_MON_0 && s <= STEP_MON_7) {
            boot_monument_setup(st, s - STEP_MON_0); /* @bin 0x004141D3 .. 0x0041421E */
        }
        break;
    case STEP_COUNT:
        return -1;
    }
    if (st->unported != before) {
        return -1;
    }
    return r ? 0 : -1;
}

int boot_step(game_state *st, const boot_host *host, const boot_display *display, int sound, int step)
{
    step_id list[STEP_COUNT];
    int n = step_list(display, sound, list);
    if (step < 0 || step >= n) {
        return -1;
    }
    return step_run(st, host, display, sound, list[step]);
}

int game_boot(game_state *st, const boot_host *host, const boot_display *display, int sound)
{
    int n = boot_step_count(display, sound);
    for (int k = 0; k < n; k++) {
        if (boot_step(st, host, display, sound, k) != 0) {
            return -1;
        }
    }
    return 0;
}

/* ---- helpers shared by the boot's modules ---- */

uint32_t boot_alloc(game_state *st, const boot_host *host, uint32_t size, uint32_t site)
{
    if (!host || !host->alloc) {
        state_unported(st, site); /* no memory outside the state without a host */
        return 0;
    }
    return host->alloc(host->ctx, st, size, site);
}

void boot_release(game_state *st, const boot_host *host, uint32_t address)
{
    if (host && host->release && address) {
        host->release(host->ctx, st, address);
    }
}

/* ---- boot_arena ---- */

int boot_arena_map(boot_arena *arena, game_state *st)
{
    return state_map_external(st, arena->base, arena->mem, arena->cap);
}

uint32_t boot_arena_alloc(boot_arena *arena, uint32_t size)
{
    /* @bin like tools/oracle/emu.py alloc: 16-byte steps, at least one, nothing reused */
    uint32_t step = ((size ? size : 1) + 15u) & ~15u;
    if (step < size || arena->used + step > arena->cap || arena->used + step < arena->used) {
        return 0;
    }
    uint32_t p = arena->base + arena->used;
    arena->used += step;
    return p;
}

static uint32_t arena_alloc(void *ctx, game_state *st, uint32_t size, uint32_t site)
{
    (void) st;
    (void) site;
    return boot_arena_alloc(ctx, size);
}

static uint32_t arena_surface(void *ctx, game_state *st, int32_t w, int32_t h, int32_t primary)
{
    (void) st;
    (void) primary;
    return w > 0 && h > 0 ? boot_arena_alloc(ctx, (uint32_t) w * 2u * (uint32_t) h) : 0;
}

boot_host boot_arena_host(boot_arena *arena)
{
    boot_host h = {arena, arena_alloc, NULL, arena_surface, NULL, NULL, NULL};
    return h;
}

/* ---- the boot's functions in the port registry ----
 * Run by game_boot with its host (memory outside the state, the platform's handles): registered with no
 * wrapper of their own (the oracle's recorder treats them as ported; tests/differential/
 * oracle_boot_check.py checks them through game_boot). The DirectDraw set-up's callees (0x004D0220,
 * 0x004D0060, 0x00426B40, 0x00426FF0, 0x00427480, 0x004D0480, 0x00426F60, 0x004D01E0) are ported for the
 * boot's path in core/boot/display.c but not registered: package "play" claims them (the display mode
 * changes). */
const ported_function boot_ports[] = {
    {0x00413ED0u, "WinMain initialisation (game_boot)", 4, 32, 0, NULL, 0},
    {0x00414420u, "boot_graphics", 0, 32, 0, NULL, 0},
    {0x0053DD10u, "boot_graphics load_next", 1, 8, G_GRAPHICS_GFX, NULL, 0},
    {0x0053DD40u, "boot_graphics load_rest", 1, 8, G_GRAPHICS_GFX, NULL, 0},
    {0x004CF7B0u, "boot_graphics loading_text", 1, 0, 0, NULL, 0},
    {0x004CF960u, "boot_display_setup display_create", 1, 32, 0, NULL, 0},
    {0x004133D0u, "boot_bink_object (Bink object constructor)", 1, 32, 0, NULL, 0},
    {0x005394F0u, "boot_sound_start", 0, 0, 0, NULL, 0},
    {0x0055E6C4u, "boot_sound_start driver_open", 0, 32, G_SOUND, NULL, 0},
    {0x0055DEA2u, "boot_sound_start voices", 1, 32, G_SOUND, NULL, 0},
    {0x0055E5D2u, "boot_sound_start voice_reset", 1, 0, G_SOUND, NULL, 0},
    {0x0055DC63u, "boot_sound_start registry_create", 1, 32, G_SOUND, NULL, 0},
    {0x0055DC18u, "boot_sound_start registry_release", 0, 0, G_SOUND, NULL, 0},
    {0x00560280u, "boot_sound_start registry_entry", 0, 32, 0, NULL, 0},
    {0x005602B0u, "boot_sound_start registry_entry_release (frees a name only the CD path strdups)", 0, 0, 0, NULL, 0},
    {0x0055DD3Fu, "boot_sound_start registry_add", 2, 32, G_SOUND, NULL, 0},
    {0x00539A40u, "boot_sound_start music_mode", 1, 0, G_MUSIC, NULL, 0},
    {0x00539250u, "music_read", 0, 0, G_MUSIC, NULL, 0},
    {0x005395E0u, "tune_add", 6, 32, G_MUSIC, NULL, 0},
    {0x00539CF0u, "building_sound", 3, 0, 0, NULL, 0},
    {0x00539D60u, "building_sounds", 0, 0, 0, NULL, 0},
    {0x0053BC80u, "ambient_sound", 3, 0, 0, NULL, 0},
    {0x0053BCF0u, "ambient_sounds", 0, 0, 0, NULL, 0},
    {0x0053AF70u, "interface_sound", 2, 0, 0, NULL, 0},
    {0x0053AF90u, "interface_sounds", 0, 0, 0, NULL, 0},
    {0x0053B100u, "figure_sound", 3, 0, 0, NULL, 0},
    {0x0053B130u, "figure_sounds", 0, 0, 0, NULL, 0},
};
const int boot_ports_count = sizeof(boot_ports) / sizeof(boot_ports[0]);
