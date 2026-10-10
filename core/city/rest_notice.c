/* Imhotep - the in-game notices (package rest): 0x004E2970 fills a free notice slot (0x78 bytes
 * at 0x00E94E28) with a text of the language file and its measures.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The text functions 0x004CC110 (find the text: pointer 0x00E20768 into the language data),
 * 0x004CBF30 (its width with the font of 0x00F56F8C: glyphs by table 0x005E047C, widths +0 of the
 * image records 0x010D6A0C, 0x40 bytes each, count 0x0126517C), 0x004CC000 (its length) and
 * 0x004CBEA0 (copy) read the language data, which the original keeps outside the simulation
 * state (heap 0x68xxxxxx, loaded by 0x00469D50): they stay calls of the original. */
#include "city/rest_notice.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

void notice_add_4e2970(game_state *st, int32_t kind)
{
    /* @bin 0x004E2970 with notices on (0x00E38EC1, 0x00E38DDA; kind 2 not when 0x00605A3C is 1):
     * the first free slot (+6 == 0) gets kind +0, +6 = 1, the width bucket +0xA (200, 300, 400,
     * 460 for text widths up to 100, 200, 300, above), the length +8, the time +0xC (0x00E92E0C)
     * and the text +0x12 (0x64 bytes); its 0x00F53F34 entry cleared, 0x00E94E20 set */
    if (U8(st, G_NOTICES_ON) == 0) {
        return;
    }
    if (kind == 2 && S32(st, G_EDITOR) == 1) {
        return;
    }
    if (U8(st, G_WINDOW) == 0) {
        return;
    }
    int32_t i;
    for (i = 0; i < G_NOTICE_COUNT; i++) {
        if (S16(st, G_NOTICES + NOTICE_SIZE * (uint32_t) i + NOTICE_ACTIVE) == 0) {
            break;
        }
    }
    if (i >= G_NOTICE_COUNT) {
        return;
    }
    uint32_t n = G_NOTICES + NOTICE_SIZE * (uint32_t) i;
    S32(st, G_UF53F34 + 4u * (uint32_t) i) = 0;
    S16(st, n + NOTICE_ACTIVE) = 1;
    if (kind == 1) {
        GAME_CALL(st, FN_TEXT_SEEK, 0x11, S32(st, G_VIEW_ORIENTATION));
    } else {
        GAME_CALL(st, FN_TEXT_SEEK, 0x13, kind - 2);
    }
    S32(st, G_NOTICE_POSTED) = 1;
    S16(st, n + NOTICE_KIND) = (int16_t) kind;
    S16(st, n + NOTICE_WIDTH) = (int16_t) GAME_CALL(st, FN_TEXT_WIDTH, S32(st, G_TEXT_CURSOR), S32(st, G_FONT_LARGE));
    S16(st, n + NOTICE_LENGTH) = (int16_t) GAME_CALL(st, FN_TEXT_LENGTH, S32(st, G_TEXT_CURSOR));
    int16_t w = S16(st, n + NOTICE_WIDTH);
    if (w <= 0x64) {
        S16(st, n + NOTICE_WIDTH) = 200;
    } else if (w <= 0xC8) {
        S16(st, n + NOTICE_WIDTH) = 300;
    } else {
        S16(st, n + NOTICE_WIDTH) = (int16_t) (w > 0x12C ? 0x1CC : 0x190);
    }
    S32(st, n + NOTICE_TIME) = S32(st, G_FRAME_CLOCK);
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) (n + NOTICE_TEXT), NOTICE_TEXT_SIZE, 0);
    GAME_CALL(st, FN_TEXT_COPY, S32(st, G_TEXT_CURSOR), (int32_t) (n + NOTICE_TEXT), S16(st, n + NOTICE_LENGTH));
}

static int32_t run_4e2970(game_state *st, const int32_t *a)
{
    notice_add_4e2970(st, a[0]);
    return 0;
}

const ported_function rest_notice_ports[] = {
    {0x004E2970u, "notice_add_4e2970", 1, 0, 0, run_4e2970, 0},
};
const int rest_notice_ports_count = sizeof(rest_notice_ports) / sizeof(rest_notice_ports[0]);
