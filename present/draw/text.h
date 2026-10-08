/* Imhotep - drawing primitives: text (package blit, roadmap 7.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Text is drawn glyph by glyph with the image draw (present/draw/draw.h). Strings are addresses
 * the state can reach (the data segments, the heap, or Pharaoh_Text.eng mapped at [0x00E36C70]).
 * Fonts are the first image ids of their groups: [0x00F56F84] small, [0x00F56FB4] large,
 * [0x00F56FBC] large 2, others (6 px spaces). Colours are 16-bit pixels of the screen's format. The
 * width of a string is text_width (0x004CBF30, core/city/text_lookup.h); the text cursor
 * 0x00E20768 is set by text_seek (0x004CC110, same header). */
#ifndef IMHOTEP_PRESENT_DRAW_TEXT_H
#define IMHOTEP_PRESENT_DRAW_TEXT_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004CCA60 one glyph (font, character byte - 0x20 in the low byte, x, y): its width */
int32_t text_glyph_draw(game_state *st, int32_t font, int32_t ch, int32_t x, int32_t y);
/* @bin 0x004CCB00 the next character of the edited string (al) */
int32_t text_edit_next(game_state *st);
/* @bin 0x004CC770 a string at (x, y); returns 0 */
int32_t text_draw(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t font, int32_t colour);
/* the same on a string of the port's own (where the original passes a buffer on its stack, which
 * the state does not hold): identical drawing and state writes */
int32_t text_draw_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t font, int32_t colour);
int32_t text_draw_centred_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t width, int32_t font,
                               int32_t colour);
/* @bin 0x004CBF30 the width of a host string (text_width, core/city/text_lookup.h, on the state's) */
int32_t text_width_host(game_state *st, const char *text, int32_t font);
/* @bin 0x004CC720 a string centred in [x, x + width) */
int32_t text_draw_centred(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t width, int32_t font,
                          int32_t colour);
/* @bin 0x004CD010 a text wrapped in lines narrower than width */
void text_draw_lines(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t width, int32_t font, int32_t colour);
/* the same on a host string (where the original passes a buffer of its stack) */
void text_draw_lines_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t width, int32_t font,
                          int32_t colour);
/* @bin 0x004CD120 the width of the next word (bytes taken in 0x00E20760) */
int32_t word_width(game_state *st, uint32_t text, int32_t font);
/* @bin 0x004CC020 a character's width + 1 */
int32_t glyph_width(game_state *st, int32_t ch, int32_t font);

/* strings of Pharaoh_Text.eng: string `index` of group `group` */
int32_t text_group_draw(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font,
                        int32_t colour);                                            /* @bin 0x004CCBB0 */
int32_t text_group_draw_centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                                int32_t font, int32_t colour);                      /* @bin 0x004CCB30 */
void text_group_draw_lines(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                           int32_t font, int32_t colour);                           /* @bin 0x004CCFD0 */
int32_t text_group_width(game_state *st, int32_t group, int32_t index, int32_t font); /* @bin 0x004CC6C0 */

/* numbers: written into the buffer 0x00E34238 */
int32_t number_format(game_state *st, int32_t value, int32_t pos, int32_t plus);    /* @bin 0x004CC5C0 */
/* @bin 0x004CCE00 prefix character (0: the text is empty), the number, the suffix string */
int32_t number_draw(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                    int32_t font, int32_t colour);
int32_t number_draw_centred(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                            int32_t width, int32_t font, int32_t colour);           /* @bin 0x004CCE90 */
/* @bin 0x004CCF20 "before value after" centred */
int32_t number_draw_between(game_state *st, int32_t value, uint32_t before, uint32_t after, int32_t x, int32_t y,
                            int32_t width, int32_t font, int32_t colour);
/* @bin 0x004CCC30 "count string" (string index + 1 for a plural count) */
void text_count_draw(game_state *st, int32_t group, int32_t index, int32_t count, int32_t x, int32_t y, int32_t font,
                     int32_t colour);
/* @bin 0x004CCCB0 an amount with the strings of group 20 */
void text_money_draw(game_state *st, int32_t kind, int32_t value, int32_t x, int32_t y, int32_t font, int32_t colour);
/* @bin 0x004CCD60 "first second" centred */
void text_pair_draw_centred(game_state *st, uint32_t first, uint32_t second, int32_t x, int32_t y, int32_t width,
                            int32_t font, int32_t colour);

/* in the 640 x 480 layout: rect is the address of i16 left, top, right, bottom; (dx, dy) added
 * with the layout's offset (0x00E38DA8, 0x00E38DA4) */
void layout_panel(game_state *st, int32_t a1, uint32_t rect, int32_t a3, int32_t dx, int32_t dy); /* @bin 0x004CD1A0 */
void layout_text_group(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t colour,
                       int32_t dx, int32_t dy);                                     /* @bin 0x004CD1F0 */
void layout_text_group_centred(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font,
                               int32_t colour, int32_t dx, int32_t dy);             /* @bin 0x004CD240 */
void layout_number(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, uint32_t rect, int32_t font,
                   int32_t colour, int32_t dx, int32_t dy);                          /* @bin 0x004CD290 */
void layout_number_centred(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, uint32_t rect,
                           int32_t font, int32_t colour, int32_t dx, int32_t dy);   /* @bin 0x004CD2E0 */
void layout_money(game_state *st, int32_t kind, int32_t value, uint32_t rect, int32_t font, int32_t colour,
                  int32_t dx, int32_t dy);                                           /* @bin 0x004CD340 */
void layout_text(game_state *st, uint32_t text, uint32_t rect, int32_t font, int32_t colour, int32_t dx,
                 int32_t dy);                                                        /* @bin 0x004CD390 */

/* the edit fields (present/draw/textedit.c): the current field [0x00E20990]'s string and caret */
void string_delete_at(game_state *st, uint32_t p, uint32_t end);   /* @bin 0x004CBEE0 */
void string_open_at(game_state *st, uint32_t p, uint32_t end);     /* @bin 0x004CBF10 */
void field_check(game_state *st);                                  /* @bin 0x004CC1B0 */
void field_type(game_state *st, int32_t ch);                       /* @bin 0x004CC250 a typed character */
void field_insert(game_state *st, int32_t ch);                     /* @bin 0x004CC400 */
void field_delete(game_state *st);                                 /* @bin 0x004CC4C0 (also 0x004CC350, Delete) */
void field_backspace(game_state *st);                              /* @bin 0x004CC330 */
void field_left(game_state *st);                                   /* @bin 0x004CC370 */
void field_right(game_state *st);                                  /* @bin 0x004CC390 */
void field_toggle_insert(game_state *st);                          /* @bin 0x004CC360 */
void field_done(game_state *st);                                   /* @bin 0x004CC320 */
void text_run_reset(game_state *st);                               /* @bin 0x004CC540 */

#endif
