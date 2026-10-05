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
#include "state/ported.h"

#define NOTICE_BASE 0x00E94E28u /* @bin 0x00E94E2E is +6 of the first slot */
#define NOTICE_SIZE 0x78u
#define NOTICE_COUNT 5 /* @bin 0x00E94E2E .. 0x00E95086 */
#define TEXT_PTR 0x00E20768u

void notice_add_4e2970(game_state *st, int32_t kind)
{
    /* @bin 0x004E2970 with notices on (0x00E38EC1, 0x00E38DDA; kind 2 not when 0x00605A3C is 1):
     * the first free slot (+6 == 0) gets kind +0, +6 = 1, the width bucket +0xA (200, 300, 400,
     * 460 for text widths up to 100, 200, 300, above), the length +8, the time +0xC (0x00E92E0C)
     * and the text +0x12 (0x64 bytes); its 0x00F53F34 entry cleared, 0x00E94E20 set */
    if (U8(st, 0x00E38EC1u) == 0) {
        return;
    }
    if (kind == 2 && S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    if (U8(st, 0x00E38DDAu) == 0) {
        return;
    }
    int32_t i;
    for (i = 0; i < NOTICE_COUNT; i++) {
        if (S16(st, NOTICE_BASE + NOTICE_SIZE * (uint32_t) i + 6) == 0) {
            break;
        }
    }
    if (i >= NOTICE_COUNT) {
        return;
    }
    uint32_t n = NOTICE_BASE + NOTICE_SIZE * (uint32_t) i;
    S32(st, 0x00F53F34u + 4u * (uint32_t) i) = 0;
    S16(st, n + 6) = 1;
    if (kind == 1) {
        GAME_CALL(st, 0x004CC110u, 0x11, S32(st, 0x00BCA2E0u));
    } else {
        GAME_CALL(st, 0x004CC110u, 0x13, kind - 2);
    }
    S32(st, 0x00E94E20u) = 1;
    S16(st, n) = (int16_t) kind;
    S16(st, n + 0xA) = (int16_t) GAME_CALL(st, 0x004CBF30u, S32(st, TEXT_PTR), S32(st, 0x00F56F8Cu));
    S16(st, n + 8) = (int16_t) GAME_CALL(st, 0x004CC000u, S32(st, TEXT_PTR));
    int16_t w = S16(st, n + 0xA);
    if (w <= 0x64) {
        S16(st, n + 0xA) = 200;
    } else if (w <= 0xC8) {
        S16(st, n + 0xA) = 300;
    } else {
        S16(st, n + 0xA) = (int16_t) (w > 0x12C ? 0x1CC : 0x190);
    }
    S32(st, n + 0xC) = S32(st, 0x00E92E0Cu);
    GAME_CALL(st, 0x0040F8F0u, (int32_t) (n + 0x12), 0x64, 0);
    GAME_CALL(st, 0x004CBEA0u, S32(st, TEXT_PTR), (int32_t) (n + 0x12), S16(st, n + 8));
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
