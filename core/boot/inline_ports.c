/* Imhotep - small functions whose work the ports of their callers do inline (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Their only callers are ported with the work written out in place (the boot constructors
 * core/boot/ctors.c, the monuments' constructors core/boot/monuments.c, the graphics slots
 * core/boot/graphics.c, the save loader core/io/save_load.c); registered here so that a call
 * through GAME_CALL reaches a port too. Objects outside the state (the original's stack) are not
 * handled: state_unported. */
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/vec.h"

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int in_state(game_state *st, uint32_t p, uint32_t n, uint32_t addr)
{
    if (p != 0 && state_ptr(st, p, n)) {
        return 1;
    }
    state_unported(st, addr);
    return 0;
}

static int32_t run_message_type(game_state *st, const int32_t *a)
{
    (void) st;
    /* @bin 0x004E1600 a message type of an old save: above 0x32 + 0xC7, else + 0x63 */
    return a[0] > 0x32 ? a[0] + 0xC7 : a[0] + 0x63;
}

static int32_t run_rect(game_state *st, const int32_t *a)
{
    /* @bin 0x004E5120 CRect(l, t, r, b): the four dwords; returns this */
    uint32_t r = THIS(a);
    if (!in_state(st, r, 16, FN_CRECT)) {
        return 0;
    }
    S32(st, r) = a[0];
    S32(st, r + 4) = a[1];
    S32(st, r + 8) = a[2];
    S32(st, r + 0xC) = a[3];
    return (int32_t) r;
}

static int32_t run_slot_by_name(game_state *st, const int32_t *a)
{
    /* @bin 0x0053DD80 (thiscall on the graphics object) 0x0053DD90(name, -1) */
    return GAME_CALLT(st, THIS(a), FN_GRAPHICS_LOAD_SLOT, a[0], -1);
}

static int32_t run_registry(game_state *st, const int32_t *a)
{
    /* @bin 0x0055DBC0 the instance counter 0x012663B0 += 0xC; +8, +4, +0 := 0; returns this */
    uint32_t r = THIS(a);
    if (!in_state(st, r, 12, FN_REGISTRY_CTOR)) {
        return 0;
    }
    S32(st, G_REGISTRY_INSTANCES) = S32(st, G_REGISTRY_INSTANCES) + 0xC;
    S32(st, r + 8) = 0;
    S32(st, r + 4) = 0;
    S32(st, r) = 0;
    return (int32_t) r;
}

static int32_t run_event_ctor(game_state *st, const int32_t *a)
{
    /* @bin 0x00448960 (thiscall, a 0x7C-byte event record) the words +6, +0xA, +8, +4, +0xE,
     * +0x12, +0x10, +0xC, +0x16, +0x1A, +0x18, +0x14, +0x1E, +0x22, +0x20, +0x1C, +0x5A, +0x5E,
     * +0x5C, +0x58 := -1, then 0x004489C0; returns this */
    static const uint8_t off[20] = {6, 0xA, 8, 4, 0xE, 0x12, 0x10, 0xC, 0x16, 0x1A,
                                    0x18, 0x14, 0x1E, 0x22, 0x20, 0x1C, 0x5A, 0x5E, 0x5C, 0x58};
    uint32_t r = THIS(a);
    if (!in_state(st, r, 0x7C, FN_EVENT_CTOR)) {
        return 0;
    }
    for (int i = 0; i < 20; i++) {
        P16(st, r + off[i]) = -1;
    }
    GAME_CALLT0(st, r, FN_EVENT_INIT);
    return (int32_t) r;
}

static int32_t run_queue_ctor(game_state *st, const int32_t *a)
{
    /* @bin 0x004F6690 (thiscall, n) +8, +0x0C, +0x14, +0x10 := 0, +0 := 0, +4 := 1, then
     * 0x004F6700(n); returns this */
    uint32_t r = THIS(a);
    if (!in_state(st, r, 0x18, FN_QUEUE_CTOR)) {
        return 0;
    }
    S32(st, r + 8) = 0;
    S32(st, r + 0xC) = 0;
    U32(st, r + 0x14) = 0;
    S32(st, r + 0x10) = 0;
    P8(st, r) = 0;
    S32(st, r + 4) = 1;
    vec_reserve(st, r, a[0]);
    return (int32_t) r;
}

const ported_function inline_ports[] = {
    {0x004E1600u, "message_type_remap", 1, 32, 0, run_message_type, 0},
    {0x004E5120u, "CRect", 4, 32, 0, run_rect, 0},
    {0x004E5140u, "monument part copy (in the monument constructors, monuments.c)", 1, 32, 0, NULL, 0},
    {0x0053DD80u, "graphics_slot_by_name", 1, 32, G_GRAPHICS_GFX, run_slot_by_name, 0},
    {0x0055DBC0u, "registry_ctor", 0, 32, 0, run_registry, 0},
    {0x00448960u, "event_ctor", 0, 32, 0, run_event_ctor, 0},
    {0x004F6690u, "queue_ctor", 1, 32, 0, run_queue_ctor, 0},
};
const int inline_ports_count = sizeof(inline_ports) / sizeof(inline_ports[0]);
