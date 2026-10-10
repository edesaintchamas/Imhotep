/* Imhotep - drawing primitives: the port table (package blit).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "draw.h"
#include "text.h"

#include "present_layout.h"
#include "state/ported.h"

#define THIS(a) ((a)[PORT_THIS_ARG] ? (uint32_t) (a)[PORT_THIS_ARG] : G_GRAPHICS_GFX)
#define U(x) ((uint32_t) (x))

static int32_t run_clip_h(game_state *st, const int32_t *a) { (void) a; clip_horizontal(st); return 0; }
static int32_t run_clip_v(game_state *st, const int32_t *a) { (void) a; clip_vertical(st); return 0; }
static int32_t run_clip_contains(game_state *st, const int32_t *a) { return clip_contains(st, a[0], a[1]); }
static int32_t run_clip_reset(game_state *st, const int32_t *a) { (void) a; clip_reset(st); return 0; }

static int32_t run_pixel_put(game_state *st, const int32_t *a) { pixel_put(st, a[0], a[1], a[2]); return 0; }
static int32_t run_pixel_and(game_state *st, const int32_t *a) { pixel_and(st, a[0], a[1], a[2]); return 0; }
static int32_t run_pixel_get(game_state *st, const int32_t *a) { return pixel_get_masked(st, a[0], a[1], a[2]); }
static int32_t run_pixel_or(game_state *st, const int32_t *a) { pixel_or(st, a[0], a[1], a[2]); return 0; }
static int32_t run_pixel_checked(game_state *st, const int32_t *a) { pixel_put_checked(st, a[0], a[1]); return 0; }
static int32_t run_pixels2(game_state *st, const int32_t *a) { pixels_put2(st, U(a[0])); return 0; }
static int32_t run_pixels2v(game_state *st, const int32_t *a) { pixels_put2_value(st, U(a[0])); return 0; }
static int32_t run_block_to(game_state *st, const int32_t *a) { block_to_screen(st, U(a[0]), a[1], a[2]); return 0; }
static int32_t run_block_from(game_state *st, const int32_t *a) { block_from_screen(st, U(a[0]), a[1], a[2]); return 0; }

#define BLIT1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { fn(st, U(a[0])); return 0; }
#define BLIT2(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { fn(st, U(a[0]), a[1]); return 0; }
BLIT1(run_opaque, blit_opaque)
BLIT1(run_plain, blit_plain)
BLIT1(run_plain_c, blit_plain_clipped)
BLIT1(run_plain_m, blit_plain_mirrored)
BLIT1(run_plain_mc, blit_plain_mirrored_clipped)
BLIT2(run_plain_k, blit_plain_mask)
BLIT2(run_plain_kc, blit_plain_mask_clipped)
BLIT1(run_rle, blit_rle)
BLIT1(run_rle_l, blit_rle_left)
BLIT1(run_rle_r, blit_rle_right)
BLIT2(run_rle_k, blit_rle_mask)
BLIT2(run_rle_kl, blit_rle_mask_left)
BLIT2(run_rle_kr, blit_rle_mask_right)
BLIT1(run_rle_m, blit_rle_mirrored)
BLIT1(run_rle_mr, blit_rle_mirrored_right)
BLIT1(run_rle_ml, blit_rle_mirrored_left)
BLIT1(run_glyph, blit_glyph)
BLIT1(run_glyph_l, blit_glyph_left)
BLIT1(run_glyph_r, blit_glyph_right)
BLIT1(run_type2, blit_type2)
BLIT1(run_16, blit_16x16)
BLIT1(run_24, blit_24x24)
BLIT1(run_32, blit_32x32)
BLIT2(run_tile, footprint_tile)
BLIT2(run_tile_t, footprint_tile_tinted)
BLIT2(run_tile_l, footprint_tile_left)
BLIT2(run_tile_lt, footprint_tile_left_tinted)
BLIT2(run_tile_r, footprint_tile_right)
BLIT2(run_tile_rt, footprint_tile_right_tinted)

static int32_t run_rle_skip(game_state *st, const int32_t *a) { return (int32_t) rle_skip_rows(st, U(a[0])); }
static int32_t run_fp2(game_state *st, const int32_t *a) { (void) a; return footprint_draw(st, 2); }
static int32_t run_fp3(game_state *st, const int32_t *a) { (void) a; return footprint_draw(st, 3); }
static int32_t run_fp4(game_state *st, const int32_t *a) { (void) a; return footprint_draw(st, 4); }
static int32_t run_fp5(game_state *st, const int32_t *a) { (void) a; return footprint_draw(st, 5); }
static int32_t run_fp6(game_state *st, const int32_t *a) { (void) a; return footprint_draw(st, 6); }

static int32_t run_image(game_state *st, const int32_t *a) { return image_draw_on(st, THIS(a), a[0], a[1], a[2]); }
static int32_t run_flipped(game_state *st, const int32_t *a) { return image_draw_flipped_on(st, THIS(a), a[0], a[1], a[2]); }
static int32_t run_top(game_state *st, const int32_t *a) { return image_draw_top_on(st, THIS(a), a[0], a[1], a[2]); }
static int32_t run_tinted(game_state *st, const int32_t *a)
{
    return image_draw_tinted_on(st, THIS(a), a[0], a[1], a[2], a[3]);
}
static int32_t run_pair(game_state *st, const int32_t *a) { return image_draw_pixel_pair_on(st, THIS(a), a[0], a[1], a[2]); }

static int32_t run_size(game_state *st, const int32_t *a)
{
    int32_t w, h;
    state_s32 *pw = (state_s32 *) state_ptr(st, U(a[1]), 4);
    state_s32 *ph = (state_s32 *) state_ptr(st, U(a[2]), 4);
    if (!pw || !ph) {
        state_unported(st, FN_IMAGE_SIZE); /* outputs on the caller's stack */
        return 0;
    }
    int32_t r = image_size_on(st, THIS(a), a[0], &w, &h);
    *pw = w;
    *ph = h;
    return r;
}

static int32_t run_free(game_state *st, const int32_t *a) { external_buffer_free(st, U(a[0]), a[1]); return 0; }
static int32_t run_ext_read(game_state *st, const int32_t *a)
{
    return external_image_read(st, THIS(a), U(a[0]), U(a[1]), a[2], a[3], a[4]);
}

static int32_t run_set_ext(game_state *st, const int32_t *a)
{
    /* the path is a string of the state (a buffer on the caller's stack cannot be reached) */
    char path[0x200];
    uint32_t p = U(a[0]);
    uint32_t i = 0;
    for (; i < sizeof(path) - 1; i++) {
        const uint8_t *c = state_ptr(st, p + i, 1);
        if (!c) {
            state_unported(st, FN_PATH_SET_EXTENSION);
            return 0;
        }
        path[i] = (char) *c;
        if (*c == 0) {
            break;
        }
    }
    path[i] = 0;
    path_set_extension(st, path, U(a[1]));
    for (uint32_t k = 0; k <= i + 4 && k < sizeof(path); k++) {
        uint8_t *c = state_ptr(st, p + k, 1);
        if (c) {
            *c = (uint8_t) path[k];
        }
        if (path[k] == 0 && k >= i) {
            break;
        }
    }
    return 0;
}

static int32_t run_replace_ext(game_state *st, const int32_t *a)
{
    /* @bin 0x004CE370 in place, on a string of the state */
    uint32_t p = U(a[0]);
    for (;; p++) {
        const uint8_t *c = state_ptr(st, p, 1);
        if (!c) {
            state_unported(st, FN_PATH_REPLACE_EXTENSION);
            return 0;
        }
        if (*c == '.') {
            for (uint32_t k = 0; k < 3; k++) {
                uint8_t *d = state_ptr(st, p + 1 + k, 1);
                if (d) {
                    *d = U8(st, G_FILE_EXT + k);
                }
            }
            uint8_t *z = state_ptr(st, p + 4, 1);
            if (z) {
                *z = 0;
            }
            return 0;
        }
        if (*c == 0) {
            return 0;
        }
    }
}

static int32_t run_line_setup(game_state *st, const int32_t *a) { line_setup(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_line_err(game_state *st, const int32_t *a) { line_error_step(st, a[0]); return 0; }
static int32_t run_put_screen(game_state *st, const int32_t *a) { pixel_put_screen(st, a[0], a[1], a[2]); return 0; }
static int32_t run_line(game_state *st, const int32_t *a) { line_draw(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_rect(game_state *st, const int32_t *a) { rect_draw(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_sunken(game_state *st, const int32_t *a) { rect_draw_sunken(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_raised(game_state *st, const int32_t *a) { rect_draw_raised(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_fill(game_state *st, const int32_t *a) { rect_fill(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_fill_and(game_state *st, const int32_t *a) { rect_fill_and(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_grey(game_state *st, const int32_t *a) { rect_grey(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_marching(game_state *st, const int32_t *a) { rect_marching(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_march(game_state *st, const int32_t *a) { (void) a; march_step(st); return 0; }
static int32_t run_march_start(game_state *st, const int32_t *a) { (void) a; march_step_start(st); return 0; }

#define A(i) (a[i])
static int32_t run_glyph_draw(game_state *st, const int32_t *a) { return text_glyph_draw(st, A(0), A(1), A(2), A(3)); }
static int32_t run_edit_next(game_state *st, const int32_t *a) { (void) a; return text_edit_next(st); }
static int32_t run_text(game_state *st, const int32_t *a) { return text_draw(st, U(A(0)), A(1), A(2), A(3), A(4)); }
static int32_t run_text_c(game_state *st, const int32_t *a) { return text_draw_centred(st, U(A(0)), A(1), A(2), A(3), A(4), A(5)); }
static int32_t run_lines(game_state *st, const int32_t *a) { text_draw_lines(st, U(A(0)), A(1), A(2), A(3), A(4), A(5)); return 0; }
static int32_t run_word(game_state *st, const int32_t *a) { return word_width(st, U(A(0)), A(1)); }
static int32_t run_gwidth(game_state *st, const int32_t *a) { return glyph_width(st, A(0), A(1)); }
static int32_t run_tg(game_state *st, const int32_t *a) { return text_group_draw(st, A(0), A(1), A(2), A(3), A(4), A(5)); }
static int32_t run_tgc(game_state *st, const int32_t *a) { return text_group_draw_centred(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6)); }
static int32_t run_tgl(game_state *st, const int32_t *a) { text_group_draw_lines(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_tgw(game_state *st, const int32_t *a) { return text_group_width(st, A(0), A(1), A(2)); }
static int32_t run_nfmt(game_state *st, const int32_t *a) { return number_format(st, A(0), A(1), A(2)); }
static int32_t run_num(game_state *st, const int32_t *a) { return number_draw(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6)); }
static int32_t run_numc(game_state *st, const int32_t *a) { return number_draw_centred(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6), A(7)); }
static int32_t run_numb(game_state *st, const int32_t *a) { return number_draw_between(st, A(0), U(A(1)), U(A(2)), A(3), A(4), A(5), A(6), A(7)); }
static int32_t run_count(game_state *st, const int32_t *a) { text_count_draw(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_money(game_state *st, const int32_t *a) { text_money_draw(st, A(0), A(1), A(2), A(3), A(4), A(5)); return 0; }
static int32_t run_tpair(game_state *st, const int32_t *a) { text_pair_draw_centred(st, U(A(0)), U(A(1)), A(2), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_lpanel(game_state *st, const int32_t *a) { layout_panel(st, A(0), U(A(1)), A(2), A(3), A(4)); return 0; }
static int32_t run_ltg(game_state *st, const int32_t *a) { layout_text_group(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_ltgc(game_state *st, const int32_t *a) { layout_text_group_centred(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_lnum(game_state *st, const int32_t *a) { layout_number(st, A(0), A(1), U(A(2)), U(A(3)), A(4), A(5), A(6), A(7)); return 0; }
static int32_t run_lnumc(game_state *st, const int32_t *a) { layout_number_centred(st, A(0), A(1), U(A(2)), U(A(3)), A(4), A(5), A(6), A(7)); return 0; }
static int32_t run_lmoney(game_state *st, const int32_t *a) { layout_money(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6)); return 0; }
static int32_t run_ltext(game_state *st, const int32_t *a) { layout_text(st, U(A(0)), U(A(1)), A(2), A(3), A(4), A(5)); return 0; }

static int32_t run_sdel(game_state *st, const int32_t *a) { string_delete_at(st, U(A(0)), U(A(1))); return 0; }
static int32_t run_sopen(game_state *st, const int32_t *a) { string_open_at(st, U(A(0)), U(A(1))); return 0; }
static int32_t run_fcheck(game_state *st, const int32_t *a) { (void) a; field_check(st); return 0; }
static int32_t run_ftype(game_state *st, const int32_t *a) { field_type(st, A(0)); return 0; }
static int32_t run_finsert(game_state *st, const int32_t *a) { field_insert(st, A(0)); return 0; }
static int32_t run_fdelete(game_state *st, const int32_t *a) { (void) a; field_delete(st); return 0; }
static int32_t run_fback(game_state *st, const int32_t *a) { (void) a; field_backspace(st); return 0; }
static int32_t run_fleft(game_state *st, const int32_t *a) { (void) a; field_left(st); return 0; }
static int32_t run_fright(game_state *st, const int32_t *a) { (void) a; field_right(st); return 0; }
static int32_t run_ftoggle(game_state *st, const int32_t *a) { (void) a; field_toggle_insert(st); return 0; }
static int32_t run_fdone(game_state *st, const int32_t *a) { (void) a; field_done(st); return 0; }
static int32_t run_rreset(game_state *st, const int32_t *a) { (void) a; text_run_reset(st); return 0; }

static int32_t run_panel_in(game_state *st, const int32_t *a) { panel_draw_inner(st, A(0), A(1), A(2), A(3)); return 0; }
static int32_t run_panel_fr(game_state *st, const int32_t *a) { panel_draw_frame(st, A(0), A(1), A(2), A(3), A(4), A(5)); return 0; }

static int32_t run_lkey(game_state *st, const int32_t *a) { return layout_window_key(st, A(0)); }
static int32_t run_lfind(game_state *st, const int32_t *a) { return (int32_t) layout_find(st, A(0), A(1), A(2), A(3)); }
static int32_t run_lhl(game_state *st, const int32_t *a) { layout_highlight(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8)); return 0; }
static int32_t run_lmp(game_state *st, const int32_t *a) { (void) a; layout_mode_panel(st); return 0; }
static int32_t run_lgl(game_state *st, const int32_t *a) { layout_group_lines(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8)); return 0; }
static int32_t run_lgc(game_state *st, const int32_t *a) { layout_group_centred(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8)); return 0; }
static int32_t run_lg(game_state *st, const int32_t *a) { layout_group(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7)); return 0; }
static int32_t run_lna(game_state *st, const int32_t *a) { layout_number_at(st, A(0), A(1), U(A(2)), A(3), A(4), A(5), A(6), A(7), A(8)); return 0; }
static int32_t run_lfr(game_state *st, const int32_t *a) { layout_frame(st, A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7)); return 0; }
static int32_t run_lip(game_state *st, const int32_t *a) { layout_inner_panel(st, A(0), A(1), A(2), A(3), A(4), A(5)); return 0; }

const ported_function draw_ports[] = {
    {0x004D5D70u, "layout_window_key", 1, 32, 0, run_lkey, 0},
    {0x004D5FE0u, "layout_find", 4, 32, 0, run_lfind, 0},
    {0x004D6090u, "layout_highlight", 9, 0, 0, run_lhl, 0},
    {0x004D6050u, "layout_mode_panel", 0, 0, 0, run_lmp, 0},
    {0x004D6160u, "layout_group_lines", 9, 0, 0, run_lgl, 0},
    {0x004D6250u, "layout_group_centred", 9, 0, 0, run_lgc, 0},
    {0x004D62F0u, "layout_group", 8, 0, 0, run_lg, 0},
    {0x004D6860u, "layout_number_at", 9, 0, 0, run_lna, 0},
    {0x004D63D0u, "layout_frame", 8, 0, 0, run_lfr, 0},
    {0x004D65B0u, "layout_inner_panel", 6, 0, 0, run_lip, 0},
    {0x00536A30u, "panel_draw_inner", 4, 0, 0, run_panel_in, 0},
    {0x00536C30u, "panel_draw_frame", 6, 0, 0, run_panel_fr, 0},
    {0x004CBEE0u, "string_delete_at", 2, 0, 0, run_sdel, 0},
    {0x004CBF10u, "string_open_at", 2, 0, 0, run_sopen, 0},
    {0x004CC1B0u, "field_check", 0, 0, 0, run_fcheck, 0},
    {0x004CC250u, "field_type", 1, 0, 0, run_ftype, 0},
    {0x004CC400u, "field_insert", 1, 0, 0, run_finsert, 0},
    {0x004CC4C0u, "field_delete", 0, 0, 0, run_fdelete, 0},
    {0x004CC350u, "field_delete_key", 0, 0, 0, run_fdelete, 0},
    {0x004CC330u, "field_backspace", 0, 0, 0, run_fback, 0},
    {0x004CC370u, "field_left", 0, 0, 0, run_fleft, 0},
    {0x004CC390u, "field_right", 0, 0, 0, run_fright, 0},
    {0x004CC360u, "field_toggle_insert", 0, 0, 0, run_ftoggle, 0},
    {0x004CC320u, "field_done", 0, 0, 0, run_fdone, 0},
    {0x004CC540u, "text_run_reset", 0, 0, 0, run_rreset, 0},
    {0x004CCA60u, "text_glyph_draw", 4, 32, 0, run_glyph_draw, 0},
    {0x004CCB00u, "text_edit_next", 0, 8, 0, run_edit_next, 0},
    {0x004CC770u, "text_draw", 5, 32, 0, run_text, 0},
    {0x004CC720u, "text_draw_centred", 6, 32, 0, run_text_c, 0},
    {0x004CD010u, "text_draw_lines", 6, 0, 0, run_lines, 0},
    {0x004CD120u, "word_width", 2, 32, 0, run_word, 0},
    {0x004CC020u, "glyph_width", 2, 32, 0, run_gwidth, 0},
    {0x004CCBB0u, "text_group_draw", 6, 32, 0, run_tg, 0},
    {0x004CCB30u, "text_group_draw_centred", 7, 32, 0, run_tgc, 0},
    {0x004CCFD0u, "text_group_draw_lines", 7, 0, 0, run_tgl, 0},
    {0x004CC6C0u, "text_group_width", 3, 32, 0, run_tgw, 0},
    {0x004CC5C0u, "number_format", 3, 32, 0, run_nfmt, 0},
    {0x004CCE00u, "number_draw", 7, 32, 0, run_num, 0},
    {0x004CCE90u, "number_draw_centred", 8, 32, 0, run_numc, 0},
    {0x004CCF20u, "number_draw_between", 8, 32, 0, run_numb, 0},
    {0x004CCC30u, "text_count_draw", 7, 0, 0, run_count, 0},
    {0x004CCCB0u, "text_money_draw", 6, 0, 0, run_money, 0},
    {0x004CCD60u, "text_pair_draw_centred", 7, 0, 0, run_tpair, 0},
    {0x004CD1A0u, "layout_panel", 5, 0, 0, run_lpanel, 0},
    {0x004CD1F0u, "layout_text_group", 7, 0, 0, run_ltg, 0},
    {0x004CD240u, "layout_text_group_centred", 7, 0, 0, run_ltgc, 0},
    {0x004CD290u, "layout_number", 8, 0, 0, run_lnum, 0},
    {0x004CD2E0u, "layout_number_centred", 8, 0, 0, run_lnumc, 0},
    {0x004CD340u, "layout_money", 7, 0, 0, run_lmoney, 0},
    {0x004CD390u, "layout_text", 6, 0, 0, run_ltext, 0},
    {0x004CD3E0u, "line_setup", 4, 0, 0, run_line_setup, 0},
    {0x004CD490u, "line_error_step", 1, 0, 0, run_line_err, 0},
    {0x004CD4E0u, "pixel_put_screen", 3, 0, 0, run_put_screen, 0},
    {0x004CD510u, "line_draw", 5, 0, 0, run_line, 0},
    {0x004CD680u, "rect_draw", 5, 0, 0, run_rect, 0},
    {0x004CD890u, "rect_draw_sunken", 4, 0, 0, run_sunken, 0},
    {0x004CD930u, "rect_draw_raised", 4, 0, 0, run_raised, 0},
    {0x004CD9D0u, "rect_fill", 5, 0, 0, run_fill, 0},
    {0x004CDA40u, "rect_fill_and", 5, 0, 0, run_fill_and, 0},
    {0x004CF090u, "rect_grey", 5, 0, 0, run_grey, 0},
    {0x004CD710u, "rect_marching", 4, 0, 0, run_marching, 0},
    {0x004CD850u, "march_step", 0, 0, 0, run_march, 0},
    {0x004CD810u, "march_step_start", 0, 0, 0, run_march_start, 0},
    {0x004CDAD0u, "clip_horizontal", 0, 0, 0, run_clip_h, 0},
    {0x004CDB80u, "clip_vertical", 0, 0, 0, run_clip_v, 0},
    {0x004CDC30u, "clip_contains", 2, 8, 0, run_clip_contains, 0},
    {0x005370D0u, "clip_reset", 0, 0, 0, run_clip_reset, 0},
    {0x0040FA40u, "pixel_put", 3, 0, 0, run_pixel_put, 0},
    {0x0040FA70u, "pixel_and", 3, 0, 0, run_pixel_and, 0},
    {0x0040FAA0u, "pixel_get_masked", 3, 32, 0, run_pixel_get, 0},
    {0x0040FAE0u, "pixel_or", 3, 0, 0, run_pixel_or, 0},
    {0x0040FB10u, "pixel_put_checked", 2, 0, 0, run_pixel_checked, 0},
    {0x0040FBE0u, "pixels_put2", 1, 0, 0, run_pixels2, 0},
    {0x0040FC10u, "pixels_put2_value", 1, 0, 0, run_pixels2v, 0},
    {0x0040F9A0u, "block_to_screen", 3, 0, 0, run_block_to, 0},
    {0x0040F9F0u, "block_from_screen", 3, 0, 0, run_block_from, 0},
    {0x0040FB60u, "blit_opaque", 1, 0, 0, run_opaque, 0},
    {0x0040FFD0u, "blit_plain", 1, 0, 0, run_plain, 0},
    {0x00410090u, "blit_plain_clipped", 1, 0, 0, run_plain_c, 0},
    {0x0040FED0u, "blit_plain_mirrored", 1, 0, 0, run_plain_m, 0},
    {0x0040FF50u, "blit_plain_mirrored_clipped", 1, 0, 0, run_plain_mc, 0},
    {0x004100F0u, "blit_plain_mask", 2, 0, 0, run_plain_k, 0},
    {0x004101C0u, "blit_plain_mask_clipped", 2, 0, 0, run_plain_kc, 0},
    {0x00410F20u, "rle_skip_rows", 1, 32, 0, run_rle_skip, 0},
    {0x00410230u, "blit_rle", 1, 0, 0, run_rle, 0},
    {0x004105C0u, "blit_rle_left", 1, 0, 0, run_rle_l, 0},
    {0x00410660u, "blit_rle_right", 1, 0, 0, run_rle_r, 0},
    {0x00410700u, "blit_rle_mask", 2, 0, 0, run_rle_k, 0},
    {0x004109D0u, "blit_rle_mask_left", 2, 0, 0, run_rle_kl, 0},
    {0x00410A70u, "blit_rle_mask_right", 2, 0, 0, run_rle_kr, 0},
    {0x0040FC40u, "blit_rle_mirrored", 1, 0, 0, run_rle_m, 0},
    {0x0040FCF0u, "blit_rle_mirrored_right", 1, 0, 0, run_rle_mr, 0},
    {0x0040FDD0u, "blit_rle_mirrored_left", 1, 0, 0, run_rle_ml, 0},
    {0x00410B10u, "blit_glyph", 1, 0, 0, run_glyph, 0},
    {0x00410DE0u, "blit_glyph_left", 1, 0, 0, run_glyph_l, 0},
    {0x00410E80u, "blit_glyph_right", 1, 0, 0, run_glyph_r, 0},
    {0x00410F70u, "blit_type2", 1, 0, 0, run_type2, 0},
    {0x00410FB0u, "blit_16x16", 1, 0, 0, run_16, 0},
    {0x004114A0u, "blit_24x24", 1, 0, 0, run_24, 0},
    {0x00411FD0u, "blit_32x32", 1, 0, 0, run_32, 0},
    {0x00427A20u, "footprint_tile", 2, 0, 0, run_tile, 0},
    {0x00428C00u, "footprint_tile_tinted", 2, 0, 0, run_tile_t, 0},
    {0x0042A160u, "footprint_tile_left", 2, 0, 0, run_tile_l, 0},
    {0x0042AA00u, "footprint_tile_left_tinted", 2, 0, 0, run_tile_lt, 0},
    {0x0042B440u, "footprint_tile_right", 2, 0, 0, run_tile_r, 0},
    {0x0042BCF0u, "footprint_tile_right_tinted", 2, 0, 0, run_tile_rt, 0},
    {0x0042C740u, "footprint_2x2", 0, 32, 0, run_fp2, 0},
    {0x0042C9A0u, "footprint_3x3", 0, 32, 0, run_fp3, 0},
    {0x0042CEE0u, "footprint_4x4", 0, 32, 0, run_fp4, 0},
    {0x0042D810u, "footprint_5x5", 0, 32, 0, run_fp5, 0},
    {0x0042E680u, "footprint_6x6", 0, 32, 0, run_fp6, 0},
    {0x0053E590u, "image_draw", 3, 32, G_GRAPHICS_GFX, run_image, 0},
    {0x0053FA40u, "image_draw_flipped", 3, 32, G_GRAPHICS_GFX, run_flipped, 0},
    {0x0053F250u, "image_draw_top", 3, 32, G_GRAPHICS_GFX, run_top, 0},
    {0x0053F4F0u, "image_draw_tinted", 4, 32, G_GRAPHICS_GFX, run_tinted, 0},
    {0x0053F4A0u, "image_draw_pixel_pair", 3, 32, G_GRAPHICS_GFX, run_pair, 0},
    {0x0053E530u, "image_size", 3, 8, G_GRAPHICS_GFX, run_size, 0},
    {0x0053F790u, "external_buffer_free", 2, 0, 0, run_free, 0},
    {0x0053F7D0u, "external_image_read", 5, 32, G_GRAPHICS_GFX, run_ext_read, 0},
    {0x004CE310u, "path_set_extension", 2, 0, 0, run_set_ext, 0},
    {0x004CE370u, "path_replace_extension", 1, 0, 0, run_replace_ext, 0},
};
const int draw_ports_count = sizeof(draw_ports) / sizeof(draw_ports[0]);
