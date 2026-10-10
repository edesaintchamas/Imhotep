/* Imhotep - the mouse cursors of build/imhotep (see cursors.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/cursors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "platform/game_files.h"

/* @bin 0x004CFB98..0x004CFC8F the slot of each cursor resource, in the order of the LoadCursorA calls */
static const struct {
    uint32_t slot;
    uint16_t id;
} loads[19] = {
    {0x00E390D8u, 0x66}, {0x00E390D4u, 0x88}, {0x00E390D0u, 0x82}, {0x00E390BCu, 0x93}, {0x00E390C4u, 0x90},
    {0x00E390C8u, 0x91}, {0x00E39094u, 0x92}, {0x00E390CCu, 0x8E}, {0x00E390B8u, 0x96}, {0x00E390B0u, 0x95},
    {0x00E390B4u, 0x94}, {0x00E390ACu, 0x99}, {0x00E390A8u, 0x98}, {0x00E390A4u, 0x9B}, {0x00E390A0u, 0x9A},
    {0x00E3909Cu, 0x9C}, {0x00E39098u, 0x9D}, {0x00E390C0u, 0x9F}, {0x00E39090u, 0xA0},
};

static SDL_Cursor *cursors[19];
static SDL_Cursor *arrow;
static uint32_t shown = 0xFFFFFFFFu;

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t) (p[0] | p[1] << 8);
}

typedef struct {
    const uint8_t *d;
    size_t len;
    uint32_t base, rva; /* file offset and RVA of .rsrc */
} rsrc;

/* the data entry of type / name (first language) in the resource tree; NULL when absent */
static const uint8_t *find(const rsrc *r, uint32_t type, uint32_t name, uint32_t *size)
{
    uint32_t off = 0;
    uint32_t want[2] = {type, name};
    for (int level = 0; level < 3; level++) {
        const uint8_t *dir = r->d + r->base + off;
        if (r->base + off + 16 > r->len) {
            return NULL;
        }
        int n = le16(dir + 12) + le16(dir + 14);
        int found = 0;
        for (int i = 0; i < n; i++) {
            const uint8_t *e = dir + 16 + 8 * i;
            uint32_t id = le32(e), to = le32(e + 4);
            if (level == 2 || id == want[level]) {
                off = to & 0x7FFFFFFFu;
                found = 1;
                if (level == 2 && (to & 0x80000000u) == 0) {
                    const uint8_t *de = r->d + r->base + off;
                    uint32_t data = le32(de);
                    *size = le32(de + 4);
                    if (data - r->rva + r->base + *size > r->len) {
                        return NULL;
                    }
                    return r->d + (data - r->rva + r->base);
                }
                break;
            }
        }
        if (!found) {
            return NULL;
        }
    }
    return NULL;
}

/* a cursor resource (hot spot, BITMAPINFOHEADER, 2-colour palette, XOR mask, AND mask; rows
 * bottom-up, 32-bit aligned) as an SDL colour cursor */
static SDL_Cursor *make_cursor(const uint8_t *c, uint32_t size, int scale)
{
    if (size < 4 + 40 + 8) {
        return NULL;
    }
    int hx = le16(c), hy = le16(c + 2);
    const uint8_t *bih = c + 4;
    int w = (int) le32(bih + 4), h2 = (int) le32(bih + 8), bpp = le16(bih + 14);
    int h = h2 / 2;
    if (bpp != 1 || w <= 0 || h <= 0 || w > 256) {
        return NULL;
    }
    uint32_t hdr = le32(bih);
    const uint8_t *pal = bih + hdr;
    int stride = ((w + 31) / 32) * 4;
    const uint8_t *xr = pal + 8, *an = xr + stride * h;
    if ((size_t) (an + stride * h - c) > size) {
        return NULL;
    }
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w * scale, h * scale, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) {
        return NULL;
    }
    for (int y = 0; y < h; y++) {
        int row = h - 1 - y;
        for (int x = 0; x < w; x++) {
            int xb = (xr[row * stride + x / 8] >> (7 - x % 8)) & 1;
            int ab = (an[row * stride + x / 8] >> (7 - x % 8)) & 1;
            uint32_t px;
            if (ab && !xb) {
                px = 0; /* transparent */
            } else if (ab && xb) {
                px = 0xFF000000u; /* inverted screen: shown black */
            } else {
                const uint8_t *q = pal + 4 * xb;
                px = 0xFF000000u | (uint32_t) q[2] << 16 | (uint32_t) q[1] << 8 | q[0];
            }
            for (int sy = 0; sy < scale; sy++) {
                uint32_t *line = (uint32_t *) ((uint8_t *) s->pixels + (size_t) (y * scale + sy) * s->pitch);
                for (int sx = 0; sx < scale; sx++) {
                    line[x * scale + sx] = px;
                }
            }
        }
    }
    SDL_Cursor *cur = SDL_CreateColorCursor(s, hx * scale, hy * scale);
    SDL_FreeSurface(s);
    return cur;
}

int cursors_load(const char *game, int scale)
{
    char path[1200];
    size_t len;
    if (find_file(game, "Pharaoh", ".exe", path, sizeof(path)) != 0) { /* the name in any case */
        snprintf(path, sizeof(path), "%s/Pharaoh.exe", game);
    }
    uint8_t *d = read_file(path, &len);
    if (!d || len < 0x40) {
        free(d);
        return -1;
    }
    rsrc r = {d, len, 0, 0};
    uint32_t pe = le32(d + 0x3C);
    unsigned nsec = le16(d + pe + 6), opt = le16(d + pe + 20);
    for (unsigned i = 0; i < nsec; i++) {
        const uint8_t *h = d + pe + 24 + opt + 40 * i;
        if (memcmp(h, ".rsrc", 6) == 0) {
            r.rva = le32(h + 12);
            r.base = le32(h + 20);
        }
    }
    int ok = 0;
    for (int i = 0; r.base && i < 19; i++) {
        uint32_t gs, cs;
        const uint8_t *g = find(&r, 12, loads[i].id, &gs); /* RT_GROUP_CURSOR */
        if (!g || gs < 6 + 14 || le16(g + 4) < 1) {
            continue;
        }
        const uint8_t *c = find(&r, 1, le16(g + 6 + 12), &cs); /* RT_CURSOR, the first image */
        if (c && (cursors[i] = make_cursor(c, cs, scale < 1 ? 1 : scale))) {
            ok++;
        }
    }
    free(d);
    arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    return ok == 19 ? 0 : -1;
}

void cursors_install(game_state *st)
{
    for (int i = 0; i < 19; i++) {
        U32(st, loads[i].slot) = CURSOR_HANDLE(loads[i].id);
    }
}

void cursors_show(uint32_t handle)
{
    if (handle == shown) {
        return;
    }
    shown = handle;
    for (int i = 0; i < 19; i++) {
        if (handle == CURSOR_HANDLE(loads[i].id) && cursors[i]) {
            SDL_SetCursor(cursors[i]);
            return;
        }
    }
    if (arrow) {
        SDL_SetCursor(arrow);
    }
}

void cursors_free(void)
{
    for (int i = 0; i < 19; i++) {
        if (cursors[i]) {
            SDL_FreeCursor(cursors[i]);
        }
        cursors[i] = NULL;
    }
    if (arrow) {
        SDL_FreeCursor(arrow);
    }
    arrow = NULL;
}
