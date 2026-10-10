/* Imhotep - the .sg3 image index reader of the graphics object (package sg3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0053DFF0 loads one image file into a slot of the graphics object 0x00F55BB0 (called by
 * 0x0053DD90, core/graphics/last_graphics.c; docs/formats/graphics.md §10-11). It reads the whole
 * <dir><name>.sg3 into its own stack frame (0x004CEB60), copies the entries and the bitmap names
 * into the state, builds the group records {slot, first image id} from the file's group table
 * (simulation state: the image ids the saved grids hold), then has 0x0053F660 read the .555
 * pixels into the graphics heap and points the internal entries into them.
 *
 * Where the bytes come from: the file read is a platform request (platform_file_read,
 * core/sim/platform.h): core asks for "data\<name>.sg3" by the path the game builds, the platform
 * reads it from the game's directory (state_replay --map files=DIR in the oracle comparisons).
 *
 * What is not in the state: the group list nodes (operator new while no simulation allocator is
 * on the call stack: tools/oracle/game.py SIM_ALLOCATORS, they go to the graphics heap and are
 * freed before the return) are kept on the host; the pixel buffer and its conversions are the
 * graphics heap's. 0x0053F660 (the .555 read) is a platform request (core/sim/platform.c
 * platform_pixels_load): the platform loads the pixels and returns the handle kept at +0xCC24;
 * the comparisons replay the original's call, so the stored pointer is the original's. */
#include "graphics/sg3.h"

#include <stdlib.h>
#include <string.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

#define SG3_FRAME 0xD8928u  /* @bin 0x0053DFFE the file buffer: frame 0xD8A48 bytes, the file at +0x120 */
#define SG3_GROUPS 80u      /* group table: 300 int16 (frame +0x170) */
#define SG3_BITMAPS 680u    /* bitmap records, 200 bytes each (frame +0x3C8) */
#define SG3_ENTRIES 40680u  /* 64-byte entries (frame +0x9FE8 + 64) */

static int str_bytes(game_state *st, uint32_t address, uint32_t *len)
{
    /* length with the NUL of a string of the state */
    for (uint32_t k = 0;; k++) {
        const uint8_t *p = state_ptr(st, address + k, 1);
        if (!p) {
            return -1;
        }
        if (*p == 0) {
            *len = k + 1;
            return 0;
        }
    }
}

static void sg3_fail(game_state *st, uint32_t address, uint8_t *buf)
{
    free(buf);
    state_unported(st, address);
}

uint8_t sg3_load(game_state *st, uint32_t rec, uint32_t name, int32_t slot, uint32_t entries, int32_t first_image,
                 uint32_t groups, int32_t first_group)
{
    /* @bin 0x0053E015 the slot's image count before the load; the old pixel buffer is deleted */
    int32_t old_count = (int32_t) P32(st, rec + GFXSLOT_IMAGES);
    uint32_t pixels = P32(st, rec + GFXSLOT_PIXELS);
    if (pixels != 0) {
        state_dealloc(st, pixels); /* graphics heap: nothing of the state (ignored by state_dealloc) */
        P32(st, rec + GFXSLOT_PIXELS) = 0;
    }
    /* @bin 0x0053E048 file name := name; size, images, groups := 0 */
    P8(st, rec) = 0;
    P32(st, rec + GFXSLOT_PIXEL_BYTES) = 0;
    P32(st, rec + GFXSLOT_IMAGES) = 0;
    P32(st, rec + GFXSLOT_GROUPS) = 0;
    uint32_t n;
    if (str_bytes(st, name, &n) != 0) {
        state_unported(st, FN_SG3_LOAD);
        return 0;
    }
    for (uint32_t k = 0; k < n; k++) {
        P8(st, rec + k) = P8(st, name + k);
    }
    /* @bin 0x0053E078 path := "data\" (0x006018F8) + name + ".sg3" (0x006018F0), on the stack */
    char path[0x104];
    uint32_t len = 0;
    const uint32_t parts[3] = {G_STR_DATA_DIR, name, G_STR_SG3_EXT};
    for (int i = 0; i < 3; i++) {
        uint32_t m;
        if (str_bytes(st, parts[i], &m) != 0 || len + m > sizeof(path)) {
            state_unported(st, FN_SG3_LOAD);
            return 0;
        }
        for (uint32_t k = 0; k < m; k++) {
            path[len + k] = (char) P8(st, parts[i] + k);
        }
        len += m - 1;
    }
    /* @bin 0x0053E10B first image / group */
    P32(st, rec + GFXSLOT_FIRST_IMAGE) = (uint32_t) first_image;
    P32(st, rec + GFXSLOT_FIRST_GROUP) = (uint32_t) first_group;
    /* @bin 0x0053E132 0x004CEB60(path, frame + 0x120, 0): the whole file into the frame */
    uint8_t *buf = calloc(1, SG3_FRAME);
    if (!buf) {
        state_unported(st, FN_SG3_LOAD);
        return 0;
    }
    int32_t size = platform_file_read(st, path, 0, buf, SG3_FRAME);
    if (size < 0 || (uint32_t) size > SG3_FRAME) {
        /* no such file: the original retries on the CD drive and asks for the CD (0x004CE9E0), then
         * reads its bitmap count from the uninitialised frame; a file larger than the frame
         * overflows the stack. Neither is reproducible */
        sg3_fail(st, FN_FILE_READ_AT, buf);
        return 0;
    }
    int32_t skip = 0;
    uint32_t skip_bytes = 0;
    if (slot != 0) {
        /* @bin 0x0053E152 first bitmap name == "system.bmp" (0x006018E4, case-sensitive): the
         * first 200 images and 0x992E bytes of the .555 are skipped */
        uint32_t m;
        if (str_bytes(st, G_STR_SYSTEM_BMP, &m) != 0) {
            sg3_fail(st, FN_SG3_LOAD, buf);
            return 0;
        }
        int same = 1;
        for (uint32_t k = 0; k < m; k++) {
            if (buf[SG3_BITMAPS + k] != P8(st, G_STR_SYSTEM_BMP + k)) {
                same = 0;
                break;
            }
        }
        if (same) {
            skip = 200;
            skip_bytes = 0x992E;
        }
    }
    /* @bin 0x0053E19F images := last_index (+16) - skip; entries 1 + skip ... copied */
    int32_t last_index;
    memcpy(&last_index, buf + 16, 4);
    int32_t count = (int32_t) ((uint32_t) last_index - (uint32_t) skip);
    P32(st, rec + GFXSLOT_IMAGES) = (uint32_t) count;
    uint32_t bytes = (uint32_t) count << 6;
    uint32_t from = SG3_ENTRIES + 64u * (uint32_t) (1 + skip);
    if (bytes > SG3_FRAME - from || (bytes && !state_ptr(st, entries, bytes))) {
        sg3_fail(st, FN_SG3_LOAD, buf);
        return 0;
    }
    memcpy(state_ptr(st, entries, bytes ? bytes : 1), buf + from, bytes);

    /* @bin 0x0053E1F0 the group table: for each group g < 300 whose first image (int16) - skip is
     * > 0, a node {key g, value {image, back link}} (operator new 8 and 16: graphics heap) goes
     * into a sorted list (0x00540540 push front, 0x005404B0 push back, 0x00540570 find,
     * 0x00540590 insert before), and the group count +0xCC30 grows */
    struct {
        int32_t next, prev, key, value;
    } node[301];
    int32_t image[301];
    int32_t head = 0, tail = 0, nn = 0;
    for (int32_t g = 0; g < 300; g++) {
        int16_t v;
        memcpy(&v, buf + SG3_GROUPS + 2u * (uint32_t) g, 2);
        int32_t first = (int32_t) v - skip;
        if (first <= 0) {
            continue;
        }
        int32_t a = ++nn, b = nn;
        image[a] = first;
        node[b].next = node[b].prev = 0;
        node[b].key = g;
        node[b].value = a;
        if (head == 0 || node[head].key >= g) {
            /* @bin 0x00540540 push front */
            node[b].next = head;
            if (head) {
                node[head].prev = b;
            }
            node[b].prev = 0;
            head = b;
            if (tail == 0) {
                tail = b;
            }
        } else if (node[tail].key <= g) {
            /* @bin 0x005404B0 push back */
            node[b].prev = tail;
            if (tail) {
                node[tail].next = b;
            }
            node[b].next = 0;
            int32_t h = head;
            tail = b;
            if (h == 0) {
                head = b;
            }
        } else {
            /* @bin 0x00540570 the first node whose key >= g; @bin 0x00540590 insert before it */
            int32_t pos = head;
            while (node[pos].key < g) {
                pos = node[pos].next;
            }
            node[b].next = pos;
            node[b].prev = node[pos].prev;
            node[node[pos].prev].next = b;
            node[pos].prev = b;
        }
        P32(st, rec + GFXSLOT_GROUPS) += 1;
    }
    /* @bin 0x0053E2B4 in list order: group record {slot, first image id + image - 1}; the group's
     * first entry +28 := its group number (first group + k); the node is unlinked and deleted */
    {
        int32_t it = head, a = 0;
        if (head) {
            a = node[head].value;
            it = node[head].next;
        }
        uint32_t out = groups;
        for (int16_t k = 0; a != 0; k++) {
            int32_t next_a = 0;
            if (it) {
                next_a = node[it].value;
                it = node[it].next;
            }
            P32(st, out) = (uint32_t) slot;
            P32(st, out + 4) = (uint32_t) (image[a] + first_image - 1);
            out += 8;
            P16(st, entries + ((uint32_t) image[a] << 6) - 0x24) = (int16_t) (first_group + k);
            int32_t b = a; /* the back link: node b holds a */
            if (node[b].prev) {
                node[node[b].prev].next = node[b].next;
            } else {
                head = node[b].next;
            }
            if (node[b].next) {
                node[node[b].next].prev = node[b].prev;
            } else {
                tail = node[b].prev;
            }
            a = next_a;
        }
    }
    /* @bin 0x0053E394 bitmap names: record i (200 bytes) -> rec + 0x104 (i + 1) */
    int32_t nbitmaps;
    memcpy(&nbitmaps, buf + 20, 4);
    for (int32_t i = 0; i < nbitmaps; i++) {
        uint32_t src = SG3_BITMAPS + 200u * (uint32_t) i, k = 0;
        for (;; k++) {
            if (src + k >= SG3_FRAME) {
                sg3_fail(st, FN_SG3_LOAD, buf);
                return 0;
            }
            P8(st, rec + 0x104u * (uint32_t) (i + 1) + k) = buf[src + k];
            if (buf[src + k] == 0) {
                break;
            }
        }
    }
    free(buf);
    /* @bin 0x0053E402 0x0053F660(name, &pixels, 0, 0, 0, 0, skipped bytes): the .555 into the
     * graphics heap; its size */
    int32_t size555 = GAME_CALL(st, FN_PLATFORM_PIXELS_LOAD, (int32_t) name, (int32_t) (rec + GFXSLOT_PIXELS), 0, 0, 0, 0,
                                (int32_t) skip_bytes);
    P32(st, rec + GFXSLOT_PIXEL_BYTES) = (uint32_t) size555;
    uint8_t ok = size555 != 0;
    if (ok) {
        /* @bin 0x0053E41E each entry: +12 := slot; external (+52): offset 1 when 0; internal: offset
         * := running pointer into the pixels (0x00D3B5F4, from the buffer + 4), which advances by
         * the length; pixels converted when the length is > 0: type 30 (+50) footprint plain and
         * top (+53) RLE, other types plain or RLE by +51 with +53 := 0 */
        P32(st, G_SG3_PIXEL_CURSOR) = P32(st, rec + GFXSLOT_PIXELS) + 4;
        for (int32_t i = 0; i < (int32_t) P32(st, rec + GFXSLOT_IMAGES); i++) {
            uint32_t e = entries + 64u * (uint32_t) i;
            P32(st, e + IMGENTRY_SLOT) = (uint32_t) slot;
            if (P8(st, e + IMGENTRY_EXTERNAL) != 0) {
                if (P32(st, e) == 0) {
                    P32(st, e) = 1;
                }
                continue;
            }
            P32(st, e) = U32(st, G_SG3_PIXEL_CURSOR);
            U32(st, G_SG3_PIXEL_CURSOR) = U32(st, G_SG3_PIXEL_CURSOR) + P32(st, e + IMGENTRY_LENGTH);
            int32_t length = (int32_t) P32(st, e + IMGENTRY_LENGTH);
            if (length <= 0) {
                continue;
            }
            if (P8(st, e + IMGENTRY_TYPE) == 0x1E) {
                if (P8(st, e + IMGENTRY_HAS_TOP) != 0) {
                    sg3_pixels_555_to_565(st, P32(st, e), (int32_t) P32(st, e + IMGENTRY_FULL_LENGTH));
                    uint32_t top = P32(st, e + IMGENTRY_FULL_LENGTH);
                    sg3_rle_555_to_565(st, P32(st, e) + top, (int32_t) (P32(st, e + IMGENTRY_LENGTH) - top));
                } else {
                    sg3_pixels_555_to_565(st, P32(st, e), length);
                }
            } else {
                P8(st, e + IMGENTRY_HAS_TOP) = 0;
                if (P8(st, e + IMGENTRY_COMPRESSED) != 0) {
                    sg3_rle_555_to_565(st, P32(st, e), length);
                } else {
                    sg3_pixels_555_to_565(st, P32(st, e), length);
                }
            }
        }
    }
    /* @bin 0x0053E4F4 images := max(new, old) */
    if ((int32_t) P32(st, rec + GFXSLOT_IMAGES) <= old_count) {
        P32(st, rec + GFXSLOT_IMAGES) = (uint32_t) old_count;
    }
    return ok;
}

void sg3_pixels_555_to_565(game_state *st, uint32_t pixels, int32_t length)
{
    /* @bin 0x004CDF20 when the screen is 565 (0x00E92DFC == 2): each 16-bit pixel except the key
     * 0xF81F becomes (p & 0xFFE0) << 1 | (p & 0x1F), for `length` bytes. The buffers are the
     * graphics heap's (outside the state): converting them is the platform's, nothing of the
     * state changes; a buffer inside the state is converted as the original does */
    if (S32(st, G_PIXEL_FORMAT) != 2) {
        return;
    }
    for (int32_t n = length; n > 0; n -= 2, pixels += 2) {
        uint8_t *p = state_ptr(st, pixels, 2);
        if (!p) {
            return;
        }
        uint16_t v = (uint16_t) (p[0] | p[1] << 8);
        if (v != 0xF81F) {
            v = (uint16_t) ((uint16_t) ((v & 0xFFE0) << 1) | (v & 0x1F));
            p[0] = (uint8_t) v;
            p[1] = (uint8_t) (v >> 8);
        }
    }
}

void sg3_rle_555_to_565(game_state *st, uint32_t data, int32_t length)
{
    /* @bin 0x004CDEB0 the RLE stream (docs/formats/graphics.md §8.2) when the screen is 565: a
     * byte 0xFF is followed by a skip byte; a byte c in 1..127 by c pixels, each becoming
     * (p & 0x7FE0) << 1 | (p & 0x1F); bytes 0 and 128..254 (signed <= 0) are passed over. Ends
     * when the byte count reaches 0 or goes below. Same scope as sg3_pixels_555_to_565 */
    if (S32(st, G_PIXEL_FORMAT) != 2) {
        return;
    }
    int32_t n = length;
    while (n > 0) {
        uint8_t *q = state_ptr(st, data, 1);
        if (!q) {
            return;
        }
        uint8_t c = *q;
        if (c == 0xFF) {
            data += 2;
            n -= 2;
            continue;
        }
        data += 1;
        n -= 1;
        for (int8_t k = (int8_t) c; k > 0; k--) {
            uint8_t *p = state_ptr(st, data, 2);
            if (!p) {
                return;
            }
            uint16_t v = (uint16_t) (p[0] | p[1] << 8);
            v = (uint16_t) ((uint16_t) ((v & 0x7FE0) << 1) | (v & 0x1F));
            p[0] = (uint8_t) v;
            p[1] = (uint8_t) (v >> 8);
            data += 2;
            n -= 2;
        }
    }
}

static int32_t run_file_read_at(game_state *st, const int32_t *a)
{
    /* @bin 0x004CEB60 file_read_at(path, buffer, offset): the rest of the file from `offset` into
     * the buffer; 1, or 0 when the file cannot be opened. A buffer outside the state (a caller's
     * stack frame) needs the caller ported: it calls platform_file_read itself */
    uint32_t n;
    if (str_bytes(st, (uint32_t) a[0], &n) != 0 || n > 0x400) {
        state_unported(st, FN_FILE_READ_AT);
        return 0;
    }
    char path[0x400];
    for (uint32_t k = 0; k < n; k++) {
        path[k] = (char) P8(st, (uint32_t) a[0] + k);
    }
    uint32_t offset = (uint32_t) a[2];
    int32_t size = platform_file_read(st, path, offset, NULL, 0);
    if (size < 0) {
        return 0;
    }
    uint32_t rest = (uint32_t) size > offset ? (uint32_t) size - offset : 0;
    uint8_t *dst = rest ? state_ptr(st, (uint32_t) a[1], rest) : NULL;
    if (rest && !dst) {
        state_unported(st, FN_FILE_READ_AT);
        return 0;
    }
    if (rest) {
        platform_file_read(st, path, offset, dst, rest);
    }
    return 1;
}

static int32_t run_load(game_state *st, const int32_t *a)
{
    return sg3_load(st, (uint32_t) a[PORT_THIS_ARG], (uint32_t) a[0], a[1], (uint32_t) a[2], a[3], (uint32_t) a[4],
                    a[5]);
}

static int32_t run_pixels(game_state *st, const int32_t *a)
{
    sg3_pixels_555_to_565(st, (uint32_t) a[0], a[1]);
    return 0;
}

static int32_t run_rle(game_state *st, const int32_t *a)
{
    sg3_rle_555_to_565(st, (uint32_t) a[0], a[1]);
    return 0;
}

const ported_function sg3_ports[] = {
    {0x0053DFF0u, "sg3_load", 6, 8, 0, run_load, 0},
    {0x004CEB60u, "file_read_at (platform_file_read)", 3, 32, 0, run_file_read_at, 0},
    {0x004CDF20u, "pixels_555_to_565", 2, 0, 0, run_pixels, 0},
    {0x004CDEB0u, "rle_555_to_565", 2, 0, 0, run_rle, 0},
    {0x005404B0u, "sg3 list push back", 1, 0, 0, NULL, 0},
    {0x00540540u, "sg3 list push front", 1, 0, 0, NULL, 0},
    {0x00540570u, "sg3 list find", 3, 32, 0, NULL, 0},
    {0x00540590u, "sg3 list insert before", 2, 0, 0, NULL, 0},
};
const int sg3_ports_count = sizeof(sg3_ports) / sizeof(sg3_ports[0]);
