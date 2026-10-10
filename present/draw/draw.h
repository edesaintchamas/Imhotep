/* Imhotep - drawing primitives: images, blitters, clip, fills, lines (package blit, roadmap 7.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of the original's drawing primitives (docs/presentation.md 2.3, docs/formats/graphics.md
 * 8, 12). They write the back buffer the state points to ([0x00E92E18], pitch [0x00E38D94]; 555 or
 * 565 as the pixel data was converted at load: the blitters copy 16-bit words as they are) and the
 * presentation globals of the state the original writes on the way (the blitters' parameters
 * 0x00D3B530..0x00D3B5E0, present/draw/mem.h). Other packages call these directly; the original's
 * callers reach them through GAME_CALL too (the table draw_ports, present/draw/ports.c). Text:
 * present/draw/text.h. */
#ifndef IMHOTEP_PRESENT_DRAW_DRAW_H
#define IMHOTEP_PRESENT_DRAW_DRAW_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* ---- images (thiscall on the graphics object; the _on forms take `this`) ---- */

/* @bin 0x0053E590 image `id` at (x, y), clipped to the clip rectangle: the drawn width (0 when
 * nothing is drawn); mirrored, RLE, plain, isometric, glyph, external images; tinted by
 * [0x0070E120] when set */
int32_t image_draw(game_state *st, int32_t id, int32_t x, int32_t y);
int32_t image_draw_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y);
/* @bin 0x0053FA40 the same, flipped horizontally */
int32_t image_draw_flipped_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y);
/* @bin 0x0053F250 the top (has_top) of isometric image `id` whose footprint is at (x, y): 1, 0 when
 * it has none */
int32_t image_draw_top(game_state *st, int32_t id, int32_t x, int32_t y);
int32_t image_draw_top_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y);
/* @bin 0x0053F4F0 the screen under image `id`'s opaque pixels ANDed with mask (a tint) */
int32_t image_draw_tinted(game_state *st, int32_t id, int32_t x, int32_t y, int32_t mask);
int32_t image_draw_tinted_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y, int32_t mask);
/* @bin 0x0053F4A0 the first two pixels of image `id` at (x, y), unclipped */
int32_t image_draw_pixel_pair_on(game_state *st, uint32_t gfx, int32_t id, int32_t x, int32_t y);
/* @bin 0x0053E530 the image's width and height: 1, or 0 (both 0) for an id out of range */
int32_t image_size_on(game_state *st, uint32_t gfx, int32_t id, int32_t *width, int32_t *height);

/* external images (the .555 files of graphics.md 7) */
/* @bin 0x0053F790 free the buffer held at state address `slot` unless it is the spare screen */
void external_buffer_free(game_state *st, uint32_t slot, int32_t forget);
/* @bin 0x0053F7D0 rows of a wide external picture read into buf (thiscall on gfx) */
int32_t external_image_read(game_state *st, uint32_t gfx, uint32_t name, uint32_t buf, int32_t row_bytes,
                            int32_t rows, int32_t offset);
/* @bin 0x004CE310 / 0x004CE370 a file name's extension replaced by the 3 characters at ext */
void path_set_extension(game_state *st, char *path, uint32_t ext);
void path_replace_extension(game_state *st, char *path);

/* ---- clip ---- */

void clip_horizontal(game_state *st);                    /* @bin 0x004CDAD0 */
void clip_vertical(game_state *st);                      /* @bin 0x004CDB80 */
int32_t clip_contains(game_state *st, int32_t x, int32_t y); /* @bin 0x004CDC30 */
void clip_reset(game_state *st);                         /* @bin 0x005370D0 the whole screen */

/* ---- blitters: parameters in the globals (present/draw/mem.h), src the pixel data ---- */

void blit_opaque(game_state *st, uint32_t src);                     /* @bin 0x0040FB60 */
void blit_plain(game_state *st, uint32_t src);                      /* @bin 0x0040FFD0 */
void blit_plain_clipped(game_state *st, uint32_t src);              /* @bin 0x00410090 */
void blit_plain_mirrored(game_state *st, uint32_t src);             /* @bin 0x0040FED0 */
void blit_plain_mirrored_clipped(game_state *st, uint32_t src);     /* @bin 0x0040FF50 */
void blit_plain_mask(game_state *st, uint32_t src, int32_t mask);   /* @bin 0x004100F0 */
void blit_plain_mask_clipped(game_state *st, uint32_t src, int32_t mask); /* @bin 0x004101C0 */
uint32_t rle_skip_rows(game_state *st, uint32_t src);               /* @bin 0x00410F20 */
void blit_rle(game_state *st, uint32_t src);                        /* @bin 0x00410230 */
void blit_rle_left(game_state *st, uint32_t src);                   /* @bin 0x004105C0 */
void blit_rle_right(game_state *st, uint32_t src);                  /* @bin 0x00410660 */
void blit_rle_mask(game_state *st, uint32_t src, int32_t mask);     /* @bin 0x00410700 */
void blit_rle_mask_left(game_state *st, uint32_t src, int32_t mask);  /* @bin 0x004109D0 */
void blit_rle_mask_right(game_state *st, uint32_t src, int32_t mask); /* @bin 0x00410A70 */
void blit_rle_mirrored(game_state *st, uint32_t src);               /* @bin 0x0040FC40 */
void blit_rle_mirrored_right(game_state *st, uint32_t src);         /* @bin 0x0040FCF0 */
void blit_rle_mirrored_left(game_state *st, uint32_t src);          /* @bin 0x0040FDD0 */
void blit_glyph(game_state *st, uint32_t src);                      /* @bin 0x00410B10 */
void blit_glyph_left(game_state *st, uint32_t src);                 /* @bin 0x00410DE0 */
void blit_glyph_right(game_state *st, uint32_t src);                /* @bin 0x00410E80 */
void blit_type2(game_state *st, uint32_t src);                      /* @bin 0x00410F70 */
void blit_16x16(game_state *st, uint32_t src);                      /* @bin 0x00410FB0 */
void blit_24x24(game_state *st, uint32_t src);                      /* @bin 0x004114A0 */
void blit_32x32(game_state *st, uint32_t src);                      /* @bin 0x00411FD0 */
void block_to_screen(game_state *st, uint32_t src, int32_t dst_step, int32_t src_step);   /* @bin 0x0040F9A0 */
void block_from_screen(game_state *st, uint32_t dst, int32_t screen_step, int32_t dst_step); /* @bin 0x0040F9F0 */

/* isometric footprints: src the tile's 1800 bytes, clip_y the vertical clip state */
void footprint_tile(game_state *st, uint32_t src, int32_t clip_y);             /* @bin 0x00427A20 */
void footprint_tile_tinted(game_state *st, uint32_t src, int32_t clip_y);      /* @bin 0x00428C00 */
void footprint_tile_left(game_state *st, uint32_t src, int32_t clip_y);        /* @bin 0x0042A160 */
void footprint_tile_left_tinted(game_state *st, uint32_t src, int32_t clip_y); /* @bin 0x0042AA00 */
void footprint_tile_right(game_state *st, uint32_t src, int32_t clip_y);       /* @bin 0x0042B440 */
void footprint_tile_right_tinted(game_state *st, uint32_t src, int32_t clip_y); /* @bin 0x0042BCF0 */
/* @bin 0x0042C740, 0x0042C9A0, 0x0042CEE0, 0x0042D810, 0x0042E680: an n x n footprint (n 2..6) of
 * the image set up by 0x0053E590 at (0x00D3B5E0, 0x00D3B5DC); returns 58 */
int32_t footprint_draw(game_state *st, int32_t n);

/* ---- pixels ---- */

void pixel_put(game_state *st, int32_t x, int32_t y, int32_t colour);           /* @bin 0x0040FA40 */
void pixel_and(game_state *st, int32_t x, int32_t y, int32_t mask);             /* @bin 0x0040FA70 */
int32_t pixel_get_masked(game_state *st, int32_t x, int32_t y, int32_t mask);   /* @bin 0x0040FAA0 */
void pixel_or(game_state *st, int32_t x, int32_t y, int32_t bits);              /* @bin 0x0040FAE0 */
void pixel_put_checked(game_state *st, int32_t x, int32_t y);                   /* @bin 0x0040FB10 */
void pixels_put2(game_state *st, uint32_t src);                                 /* @bin 0x0040FBE0 */
void pixels_put2_value(game_state *st, uint32_t value);                         /* @bin 0x0040FC10 */

/* ---- lines, rectangles, fills (colours are 16-bit pixels of the screen's format) ---- */

void line_setup(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);       /* @bin 0x004CD3E0 */
void line_error_step(game_state *st, int32_t along_y);                                  /* @bin 0x004CD490 */
void pixel_put_screen(game_state *st, int32_t x, int32_t y, int32_t colour);            /* @bin 0x004CD4E0 */
void line_draw(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t colour); /* @bin 0x004CD510 */
void rect_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t colour);  /* @bin 0x004CD680 */
void rect_draw_sunken(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);           /* @bin 0x004CD890 */
void rect_draw_raised(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);           /* @bin 0x004CD930 */
void rect_fill(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t colour);  /* @bin 0x004CD9D0 */
void rect_fill_and(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t mask); /* @bin 0x004CDA40 */
void rect_grey(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t shift);   /* @bin 0x004CF090 */
void rect_marching(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);              /* @bin 0x004CD710 */
void march_step(game_state *st);                                                        /* @bin 0x004CD850 */
void march_step_start(game_state *st);                                                  /* @bin 0x004CD810 */

/* ---- the windows' tiled panels (16 x 16 pieces) ---- */

/* (0x00536820, the window panel: panel_draw of package sidebar, present/sidebar/sidebar.h) */
void panel_draw_inner(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);  /* @bin 0x00536A30 w x h pieces */
/* @bin 0x00536C30 a frame around w x h pixels (lit: the highlighted pieces) */
void panel_draw_frame(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit);

/* ---- elements placed through the layout registry (present/draw/layout.c): (dx, dy) the window's
 * origin, the element moved by an override when the registry has one ---- */

int32_t layout_window_key(game_state *st, int32_t window);                                   /* @bin 0x004D5D70 */
uint32_t layout_find(game_state *st, int32_t kind, int32_t key, int32_t x, int32_t y);       /* @bin 0x004D5FE0 */
void layout_highlight(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t nx,
                      int32_t ny, int32_t dx, int32_t dy);                                   /* @bin 0x004D6090 */
void layout_mode_panel(game_state *st);                                                      /* @bin 0x004D6050 */
void layout_group_lines(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                        int32_t font, int32_t colour, int32_t dx, int32_t dy);                /* @bin 0x004D6160 */
void layout_group_centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                          int32_t font, int32_t colour, int32_t dx, int32_t dy);              /* @bin 0x004D6250 */
void layout_group(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font,
                  int32_t colour, int32_t dx, int32_t dy);                                   /* @bin 0x004D62F0 */
void layout_number_at(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                      int32_t font, int32_t colour, int32_t dx, int32_t dy);                  /* @bin 0x004D6860 */
void layout_frame(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit, int32_t dx,
                  int32_t dy);                                                               /* @bin 0x004D63D0 */
void layout_inner_panel(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t dx, int32_t dy); /* @bin 0x004D65B0 */

#endif
