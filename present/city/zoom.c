/* Imhotep - the city view zoomed (docs/enhanced.md, @imhotep enhanced zoom): not part of the original.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * At a zoom other than 100 % the view's geometry is that of a virtual view of (size * 100 / zoom)
 * pixels (core/newgame/reset.c view_geometry_fit). The original's city draw (0x00430D30) draws it into
 * a buffer of the platform's memory: the back buffer's globals point at it while it runs. The buffer is
 * then scaled into the view's rectangle of the screen (x 0 .. the sidebar, y 30 .. the bottom):
 * nearest pixel when zoomed in, the average of the pixels covered when zoomed out. */
#include "city.h"

#include <string.h>

#include "frame/display.h"
#include "present_layout.h"
#include "sim/platform.h"
#include "state/ported.h"

static uint32_t buf_addr, buf_size;
static int in_zoom;

static uint32_t zoom_buffer(game_state *st, uint32_t size)
{
    if (buf_addr && buf_size >= size) {
        return buf_addr;
    }
    if (buf_addr) {
        display_memory(st, buf_addr, 0);
    }
    buf_addr = display_memory(st, 0, size);
    buf_size = buf_addr ? size : 0;
    return buf_addr;
}

static void channels(uint16_t p, int rgb565, uint32_t c[3])
{
    if (rgb565) {
        c[0] = p >> 11, c[1] = (p >> 5) & 0x3F, c[2] = p & 0x1F;
    } else {
        c[0] = (p >> 10) & 0x1F, c[1] = (p >> 5) & 0x1F, c[2] = p & 0x1F;
    }
}

static uint16_t pack(const uint32_t c[3], int rgb565)
{
    return rgb565 ? (uint16_t) (c[0] << 11 | c[1] << 5 | c[2]) : (uint16_t) (c[0] << 10 | c[1] << 5 | c[2]);
}

/* the buffer (sw x sh, pitch spitch bytes) scaled into the screen's rectangle (0, 30) .. (right, h) */
static void zoom_scale(game_state *st, const uint8_t *src, int32_t spitch, int32_t sw, int32_t sh, int32_t z,
                       int32_t right, int32_t h)
{
    static int32_t x0[4096], x1[4096];
    int32_t pitch = S32(st, G_SCREEN_PITCH);
    uint8_t *dst = state_ptr(st, U32(st, G_BACK_BUFFER), (uint32_t) (pitch * h));
    int rgb565 = S32(st, G_PIXEL_FORMAT) == 2;
    if (!dst || right > 4096) {
        return;
    }
    for (int32_t x = 0; x < right; x++) {
        x0[x] = x * 100 / z;
        x1[x] = (x + 1) * 100 / z;
        if (x0[x] >= sw) {
            x0[x] = sw - 1;
        }
        if (x1[x] <= x0[x]) {
            x1[x] = x0[x] + 1;
        }
        if (x1[x] > sw) {
            x1[x] = sw;
        }
    }
    for (int32_t y = 0x1E; y < h; y++) {
        int32_t y0 = 0x1E + (y - 0x1E) * 100 / z, y1 = 0x1E + (y - 0x1E + 1) * 100 / z;
        if (y0 >= sh) {
            y0 = sh - 1;
        }
        if (y1 <= y0) {
            y1 = y0 + 1;
        }
        if (y1 > sh) {
            y1 = sh;
        }
        uint16_t *out = (uint16_t *) (dst + (size_t) y * (size_t) pitch);
        if (z >= 100) {
            const uint16_t *in = (const uint16_t *) (src + (size_t) y0 * (size_t) spitch);
            for (int32_t x = 0; x < right; x++) {
                out[x] = in[x0[x]];
            }
            continue;
        }
        for (int32_t x = 0; x < right; x++) {
            uint32_t sum[3] = {0, 0, 0}, c[3], n = 0;
            for (int32_t yy = y0; yy < y1; yy++) {
                const uint16_t *in = (const uint16_t *) (src + (size_t) yy * (size_t) spitch);
                for (int32_t xx = x0[x]; xx < x1[x]; xx++) {
                    channels(in[xx], rgb565, c);
                    sum[0] += c[0], sum[1] += c[1], sum[2] += c[2], n++;
                }
            }
            sum[0] = (sum[0] + n / 2) / n, sum[1] = (sum[1] + n / 2) / n, sum[2] = (sum[2] + n / 2) / n;
            out[x] = pack(sum, rgb565);
        }
    }
}

int city_view_draw_zoomed(game_state *st, int32_t camera_x, int32_t camera_y, int32_t map_start)
{
    /* 1 when drawn here (zoomed), 0 at 100 % or inside the zoomed draw itself (the original's draw) */
    int32_t z = platform_display_zoom(st);
    if (z == 100 || in_zoom || !display_has_memory(st)) {
        return 0;
    }
    int32_t right = S32(st, G_SIDEBAR_COLLAPSED) == 1 ? S32(st, G_SIDEBAR_X_COLLAPSED) : S32(st, G_SIDEBAR_X);
    int32_t h = S32(st, G_SCREEN_H);
    /* the virtual view: its pixels as the geometry has them (up to its clip), with a margin */
    int32_t sw = S32(st, G_CV_VIEW_RIGHT) + 2, sh = S32(st, G_CV_VIEW_BOTTOM) + 0x20;
    if (sw <= 0 || sh <= 0 || sw > 8192 || sh > 8192) {
        return 0;
    }
    uint32_t buf = zoom_buffer(st, (uint32_t) (sw * sh * 2));
    uint8_t *px = buf ? state_ptr(st, buf, (uint32_t) (sw * sh * 2)) : NULL;
    if (!px) {
        return 0;
    }
    memset(px, 0, (size_t) sw * (size_t) sh * 2);
    static const uint32_t saved_at[] = {G_BACK_BUFFER, G_SCREEN_PITCH, G_DRAW_W, G_DRAW_H, G_SCREEN_W, G_SCREEN_H};
    int32_t saved[6];
    for (int i = 0; i < 6; i++) {
        saved[i] = S32(st, saved_at[i]);
    }
    U32(st, G_BACK_BUFFER) = buf;
    S32(st, G_SCREEN_PITCH) = sw * 2;
    S32(st, G_DRAW_W) = sw;
    S32(st, G_DRAW_H) = sh;
    S32(st, G_SCREEN_W) = sw;
    S32(st, G_SCREEN_H) = sh;
    in_zoom = 1;
    city_view_draw(st, camera_x, camera_y, map_start);
    in_zoom = 0;
    for (int i = 0; i < 6; i++) {
        S32(st, saved_at[i]) = saved[i];
    }
    S32(st, G_CLIP_RIGHT) = S32(st, G_SCREEN_W);
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
    zoom_scale(st, px, sw * 2, sw, sh, z, right, h);
    return 1;
}
