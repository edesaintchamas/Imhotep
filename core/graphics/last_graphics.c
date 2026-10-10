/* Imhotep - the image slot table of the graphics object 0x00F55BB0 (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The graphics object (this = 0x00F55BB0, docs/formats/graphics.md §10-11):
 *   this + 8 g            group records {slot, first image id} (the image ids the simulation uses,
 *                         0x00F56B84 = group 506 +4, ...)
 *   this + 0x1F40 + 0xCC3C s   slot records: file name, bitmap names (+0x104 each), +0xCC24
 *                         pixel buffer, +0xCC28 its size, +0xCC2C images, +0xCC30 groups,
 *                         +0xCC34 first image id, +0xCC38 first group
 *   this + 0x180E48 + 64 i     image entries (64 bytes, the .sg3 entry layout)
 *   this + 0x30F544 + 4 s      expected running total of groups per slot (read only here)
 *   this + 0x30F5C8 / CC / D0 / D4   groups, images, pixel bytes, slots filled so far
 * 0x0053DD90 loads a file into a slot unless the slot already holds it; the file itself is read
 * and parsed by 0x0053DFF0 (not ported: it reads the .sg3 into its own stack frame). */
#include "graphics/last_graphics.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

int32_t crt_strcmpi(game_state *st, uint32_t a, uint32_t b)
{
    /* @bin 0x00562370 _strcmpi, the "C" locale path (LC_CTYPE handle 0x0126648C == 0): bytes
     * A..Z folded to a..z; -1 / 1 at the first difference (unsigned), 0 when equal */
    if (S32(st, G_LC_CTYPE) != 0) {
        state_unported(st, FN_CRT_STRCMPI); /* the locale path (0x00566682): setlocale is not ported */
        return 0;
    }
    for (uint32_t k = 0;; k++) {
        uint8_t c1 = P8(st, a + k), c2 = P8(st, b + k);
        if (c1 == c2) {
            if (c1 == 0) {
                return 0;
            }
            continue;
        }
        uint8_t l2 = (uint8_t) (c2 - 0x41) < 0x1A ? (uint8_t) (c2 + 0x20) : c2;
        uint8_t l1 = (uint8_t) (c1 - 0x41) < 0x1A ? (uint8_t) (c1 + 0x20) : c1;
        if (l1 != l2) {
            return l1 < l2 ? -1 : 1;
        }
    }
}

int32_t graphics_load_slot(game_state *st, uint32_t self, uint32_t name, int32_t slot)
{
    /* @bin 0x0053DD90 (thiscall on the graphics object; file name, slot or -1 for the next free
     * slot 0x30F5D4): an empty slot (+0xCC2C 0) gets the file at the running totals of images
     * and groups, which grow by its counts when the load succeeds (and one more slot); a filled
     * slot holding another file (_strcmpi) is reloaded at its own first image / group, so
     * global ids never move; a slot holding this file is left alone (result 1). The pixel total
     * 0x30F5D0 loses the slot's old size and gains its new one. Returns the load's result. */
    if (name == 0) {
        return 0;
    }
    if (slot == -1) {
        slot = S32(st, self + GFX_SLOT_TOTAL);
    }
    uint32_t rec = self + GFX_SLOTS + GFX_SLOT_SIZE * (uint32_t) slot;
    int first = 0;
    int32_t image_base = 0, group_base = 0;
    uint8_t ok;
    if (S32(st, rec + GFXSLOT_IMAGES) == 0) {
        group_base = S32(st, self + GFX_GROUP_TOTAL);
        image_base = S32(st, self + GFX_IMAGE_COUNT);
        first = 1;
    } else {
        int same = crt_strcmpi(st, name, rec) == 0;
        if (!same) {
            image_base = S32(st, rec + GFXSLOT_FIRST_IMAGE);
            group_base = S32(st, rec + GFXSLOT_FIRST_GROUP);
        }
        U32(st, self + GFX_PIXEL_TOTAL) -= U32(st, rec + GFXSLOT_PIXEL_BYTES);
        if (same) {
            ok = 1;
            goto done;
        }
    }
    /* @bin 0x0053DE51 0x0053DFF0 (thiscall on the slot record) (name, slot, entries, first
     * image, group records, first group) */
    ok = (uint8_t) GAME_CALLT(st, rec, FN_SG3_LOAD, (int32_t) name, slot,
                              (int32_t) (self + GFX_ENTRIES + 64u * (uint32_t) image_base), image_base,
                              (int32_t) (self + 8u * (uint32_t) group_base), group_base);
    if (ok && first) {
        U32(st, self + GFX_IMAGE_COUNT) += U32(st, rec + GFXSLOT_IMAGES);
        U32(st, self + GFX_GROUP_TOTAL) += U32(st, rec + GFXSLOT_GROUPS);
        S32(st, self + GFX_SLOT_TOTAL) += 1;
    }
done:
    U32(st, self + GFX_PIXEL_TOTAL) += U32(st, rec + GFXSLOT_PIXEL_BYTES);
    {
        /* @bin 0x0053DEB3 the slot's group count against the expected running totals (+0x30F548
         * by slot, minus the previous slot's); a difference shows the "Graphic file out of sync"
         * message box (MFC strings, MessageBoxA): interface only. Not reached in play: every file
         * a slot is loaded with has the same group table as the slot's first file
         * (docs/formats/graphics.md §11); only a monument the scenario does not offer, forced into
         * a slot (obelisk5x5a, 1 group, into a slot of 66), reaches it */
        int32_t expected = S32(st, self + GFX_SLOT_GROUP_ENDS + 4u * (uint32_t) slot) - 1;
        int32_t before = slot > 0 ? S32(st, self + GFX_SLOT_GROUP_ENDS - 4u + 4u * (uint32_t) slot) - 1 : 0;
        if (expected - before != S32(st, rec + GFXSLOT_GROUPS)) {
            state_unported(st, 0x005609DAu); /* the message box */
        }
    }
    return ok;
}

static int32_t run_load_slot(game_state *st, const int32_t *a)
{
    uint32_t self = a[PORT_THIS_ARG] ? (uint32_t) a[PORT_THIS_ARG] : G_GRAPHICS_GFX;
    return graphics_load_slot(st, self, (uint32_t) a[0], a[1]);
}

static int32_t run_strcmpi(game_state *st, const int32_t *a)
{
    return crt_strcmpi(st, (uint32_t) a[0], (uint32_t) a[1]);
}

const ported_function last_graphics_ports[] = {
    {0x0053DD90u, "graphics_load_slot", 2, 8, G_GRAPHICS_GFX, run_load_slot, 0},
    {0x00562370u, "crt_strcmpi", 2, 32, 0, run_strcmpi, 0},
};
const int last_graphics_ports_count = sizeof(last_graphics_ports) / sizeof(last_graphics_ports[0]);
