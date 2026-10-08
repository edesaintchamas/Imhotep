/* Imhotep - the boot: the graphics slots and the loading screen (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * WinMain's 0x00414420 loads the graphics object's slots (0x00F55BB0: core/graphics/sg3.c for the
 * .sg3 indexes, the platform for the .555 pixels) from a table of 30 names, in five groups, showing a
 * loading picture (present/boot/loading.c) and a line of text between them. */
#include "boot/boot_internal.h"
#include "city/text_lookup.h"
#include "sim/call.h"
#include "sim/platform.h"

#define GFX 0x00F55BB0u
#define SLOTS_LOADED (GFX + 0x30F5D4u)

/* @bin 0x0041448A .. 0x00414578 the slot names (strings of .rdata), in their order on the stack */
static const uint32_t SLOT_NAMES[30] = {
    0x00579218u, 0x00579210u, 0x00579208u, 0x005791F8u, 0x005791E8u, 0x005791DCu, 0x005791D0u, 0x005791C4u,
    0x005791B8u, 0x005791A8u, 0x0057919Cu, 0x00579194u, 0x00579188u, 0x00579178u, 0x0057916Cu, 0x00579160u,
    0x00579154u, 0x00579144u, 0x00579134u, 0x00579124u, 0x0057922Cu, 0x0057922Cu, 0x0057922Cu, 0x00576284u,
    0x00579110u, 0x00579100u, 0x005790F0u, 0x005790E0u, 0x005790D0u, 0x005790C0u,
};

/* @bin 0x0053DD10 (thiscall on the graphics object, names) the next slot (the number loaded so far,
 * below 30) from its name, when it has one: 0x0053DD80 = 0x0053DD90(name, -1); 0 otherwise */
static int32_t load_next(game_state *st)
{
    int32_t n = S32(st, SLOTS_LOADED);
    if (n >= 0x1E) {
        return 0;
    }
    uint32_t name = SLOT_NAMES[n];
    if (!name || U8(st, name) == 0) {
        return 0;
    }
    return GAME_CALLT(st, GFX, 0x0053DD90u, (int32_t) name, -1) & 0xFF;
}

/* @bin 0x0053DD40 the following slots while each one loads (and while names remain) */
static void load_rest(game_state *st)
{
    int32_t ok = 1;
    for (;;) {
        int32_t n = S32(st, SLOTS_LOADED);
        if (n >= 0x1E) {
            return;
        }
        uint32_t name = SLOT_NAMES[n];
        if (name && U8(st, name) != 0) {
            ok = GAME_CALLT(st, GFX, 0x0053DD90u, (int32_t) name, -1) & 0xFF;
        }
        if (!ok) {
            return;
        }
    }
}

/* @bin 0x004CF7B0 the loading screen's line of text (`text`, a string of the game's text): a framed
 * box and the text drawn with GDI on the window. Without a display it draws nothing (the oracle's
 * boot without a display makes it a no-op: tools/oracle/game.py PRESENTATION) */
static void loading_text(game_state *st, const boot_host *host, uint32_t text)
{
    if (U32(st, 0x00E92E18u) == 0) {
        return; /* no back buffer: no display */
    }
    int32_t n = text_length(st, text); /* 0x004CC000 */
    if (n <= 0) {
        return;
    }
    /* @bin 0x004CF7D0 GetDC(NULL) (the platform's), GetTextExtentPointA */
    char buf[256];
    int32_t k = 0;
    for (; k < n && k < (int32_t) sizeof(buf) - 1; k++) {
        const uint8_t *c = state_ptr(st, text + (uint32_t) k, 1);
        buf[k] = c ? (char) *c : 0;
    }
    buf[k] = 0;
    int32_t cx = 8 * n, cy = 16;
    if (host && host->text_extent && !host->text_extent(host->ctx, buf, n, &cx, &cy)) {
        return;
    }
    /* @bin 0x004CF7F4 the box under the text, centred on the screen and a quarter down (signed halves) */
    int32_t bx = (S32(st, 0x00E38DB0u) - cx) / 2;
    int32_t by = (S32(st, 0x00E38DACu) - cy) / 2;
    by += by * 50 / 100;
    if (S32(st, 0x00E38DB4u) != S32(st, 0x00E38DACu)) {
        by -= 0x18;
    }
    GAME_CALL(st, 0x004CD9D0u, bx - 0x1A, by - 0xB, cx + 0x34, cy + 0x16, 0);
    GAME_CALL(st, 0x004CD9D0u, bx - 0x19, by - 0xA, cx + 0x32, cy + 0x14, 0xFFFF);
    /* @bin 0x004CF8B3 SetBkColor, TextOutA at the screen's centre, ReleaseDC: the window's text, the
     * platform's (it is not in the back buffer) */
}

/* the picture k, its text (group 0xB, `index`), a present between two draws of the text */
static void loading_step(game_state *st, const boot_host *host, int32_t picture, int32_t index)
{
    GAME_CALL(st, 0x00520B20u, picture);
    text_seek(st, 0xB, index);
    loading_text(st, host, U32(st, TEXT_CURSOR));
    GAME_CALL(st, 0x004D0280u, 1);
    loading_text(st, host, U32(st, TEXT_CURSOR));
}

int32_t boot_graphics(game_state *st, const boot_host *host)
{
    /* @bin 0x00414430 Pharaohmap.inf (0x0057924C) into 0x00BCA2B8 (file_read_at) */
    GAME_CALL(st, 0x004CEB60u, (int32_t) 0x0057924Cu, (int32_t) 0x00BCA2B8u, 0);
    S32(st, 0x00E38E94u) = 0x10;
    S32(st, 0x005E06D0u) = 1;
    /* @bin 0x00414459 CreateDirectoryA("Save", "Maps"): the player's directories, the platform's */
    /* @bin 0x00414464 the spare screen: malloc(height * width * 2), at least 1,200,000 bytes */
    int32_t size = (int32_t) ((uint32_t) S32(st, 0x00E38DACu) * (uint32_t) S32(st, 0x00E38DB0u) * 2u);
    if (size <= 0x124F80) {
        size = 0x124F80;
    }
    U32(st, 0x00E92E1Cu) = boot_alloc(st, host, (uint32_t) size, 0x00414485u);
    load_next(st);
    loading_step(st, host, 0, 7);
    if (U32(st, 0x00E92E1Cu) == 0) {
        return 1;
    }
    load_next(st);
    loading_step(st, host, 1, 8);
    load_next(st);
    loading_step(st, host, 2, 9);
    for (int i = 0; i < 5; i++) {
        load_next(st);
    }
    loading_step(st, host, 3, 10);
    load_rest(st);
    loading_step(st, host, 3, 11);
    GAME_CALL0(st, 0x00520AF0u);
    GAME_CALL(st, 0x004D0280u, 1);
    platform_log(st, 0x005790A4u, 0, 0); /* "OK :Setup loaded DAT files." */
    return 0;
}
