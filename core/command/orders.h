/* Imhotep - player settings and orders (roadmap phase 5.2, docs/commands.md sections 3 and 4).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Button callbacks of the original UI, ported at the handler boundary: no arguments, their inputs
 * are globals of the state that the platform sets before the call (callback parameters
 * 0x00D3B198 / 0x00D3B194, confirmation answer 0x00D3B439, window context 0x00EADD6C, selected
 * resource 0x0060598C, ...). They also keep the original's UI stores (window id 0x00E38DDA, redraw
 * flag 0x00E38DCA): those globals are part of the state. */
#ifndef IMHOTEP_COMMAND_ORDERS_H
#define IMHOTEP_COMMAND_ORDERS_H

#include <stdint.h>

#include "state/state.h"

#define CMD_PARAM1 0x00D3B198u   /* i32 callback parameter 1 */
#define CMD_PARAM2 0x00D3B194u   /* i32 callback parameter 2 */
#define CMD_ANSWER 0x00D3B439u   /* u8 confirmation popup answer (1 = yes) */
#define UI_WINDOW 0x00E38DDAu    /* u8 current window id */
#define UI_REDRAW 0x00E38DCAu    /* u8 redraw request */

/* orders.c: finance, labour, festivals, game settings */
void cmd_tax_rate(game_state *st);            /* @bin 0x0040A260 */
void cmd_wages(game_state *st);               /* @bin 0x0040A1D0 */
void cmd_salary(game_state *st);              /* @bin 0x0040AF30 */
void cmd_donation_amount(game_state *st);     /* @bin 0x0040AFF0 */
void cmd_donation_step(game_state *st);       /* @bin 0x0040B0B0 */
void cmd_donate(game_state *st);              /* @bin 0x0040AF90 */
void cmd_gift_size(game_state *st);           /* @bin 0x0040B140 */
void cmd_gift_send(game_state *st);           /* @bin 0x0040B1C0 */
void gift_send(game_state *st);               /* @bin 0x0046EDD0 */
void tax_income_estimate(game_state *st);     /* @bin 0x00459D20 */
void wages_estimate(game_state *st);          /* @bin 0x0045A8C0 */
void salary_estimate(game_state *st);         /* @bin 0x0045A970 */
void cmd_labour_priority(game_state *st);     /* @bin 0x0040A730 */
void cmd_festival_god(game_state *st);        /* @bin 0x0040AC50 */
void cmd_festival_size(game_state *st);       /* @bin 0x0040AC80 */
void cmd_festival_hold(game_state *st);       /* @bin 0x0040AB00 */
void cmd_difficulty(game_state *st);          /* @bin 0x00402E50 */
void cmd_difficulty_lower(game_state *st);    /* @bin 0x004EA840 */
void cmd_keep_governing(game_state *st);      /* @bin 0x004EA770 */
void cmd_stop_governing(game_state *st);      /* @bin 0x004EA7F0 */
void cmd_message_options(game_state *st);     /* @bin 0x00402CC0 */
void cmd_rotate_map(game_state *st);          /* @bin 0x00409EB0 */

/* orders_storage.c: storage yards, granaries, bazaars (building 0x00EADD6C) */
void cmd_storage_cycle(game_state *st);       /* @bin 0x00406300 */
void cmd_storage_limit(game_state *st);       /* @bin 0x00406460 */
void cmd_storage_all(game_state *st);         /* @bin 0x00406620 */
void cmd_storage_special(game_state *st);     /* @bin 0x00406680 */
void cmd_storage_toggle_dc(game_state *st);   /* @bin 0x0040C630 */
int32_t storage_rows_visible(game_state *st); /* @bin 0x00502780 */

/* orders_messages.c: message list and event replies */
void cmd_message_delete(game_state *st);      /* @bin 0x004E2300 */
void cmd_messages_delete_read(game_state *st); /* @bin 0x004E2290 */
void cmd_message_open(game_state *st);        /* @bin 0x004E21D0 */
void cmd_event_reply_1(game_state *st);       /* @bin 0x0044EEA0 */
void cmd_event_reply_2(game_state *st);       /* @bin 0x0044EEF0 */
void cmd_event_reply_4(game_state *st);       /* @bin 0x0044EE30 */
uint32_t event_of_message(game_state *st, uint32_t message); /* @bin 0x0044EDD0 (this 0x00785D70) */
void message_window_close(game_state *st);    /* @bin 0x004C7F30 */
void message_window_open(game_state *st, int32_t type, int32_t param, uint8_t replies); /* @bin 0x004C5BF0 */

/* trade.c */
void cmd_trade_open_route(game_state *st);    /* @bin 0x004022D0 */
void cmd_trade_import(game_state *st);        /* @bin 0x00559A60 */
void cmd_trade_export(game_state *st);        /* @bin 0x00559B50 */
void cmd_trade_status(game_state *st);        /* @bin 0x005598F0 */
void cmd_trade_limit(game_state *st);         /* @bin 0x00559CD0 */
void cmd_trade_stockpile(game_state *st);     /* @bin 0x00559D70 */
void cmd_trade_industry(game_state *st);      /* @bin 0x005598A0 */
int32_t empire_buyer_open(game_state *st, int32_t resource); /* @bin 0x00443A20 */
void text_seek2(game_state *st, int32_t group, int32_t index); /* @bin 0x004CC160 */
void build_buttons_update(game_state *st);    /* @bin 0x004051A0 */

/* kingdom.c: requests, grave goods */
void cmd_request_confirmed(game_state *st);   /* @bin 0x00402340 */
void request_dispatch(game_state *st, int32_t event); /* @bin 0x0044C1A0 */
void request_send_troops(game_state *st, int32_t event); /* @bin 0x004B84B0 */
void cmd_grave_goods_choose(game_state *st);  /* @bin 0x0040CD70 */
void cmd_grave_goods_max(game_state *st);     /* @bin 0x0040B3B0 */
void cmd_grave_goods_more(game_state *st);    /* @bin 0x0040B420 */
void cmd_grave_goods_less(game_state *st);    /* @bin 0x0040B4B0 */
void cmd_grave_goods_send(game_state *st);    /* @bin 0x0040B1F0 */
void cmd_grave_goods_cancel(game_state *st);  /* @bin 0x0040B380 */
int32_t grave_goods_count(game_state *st, int32_t skip); /* @bin 0x0040CC40 */
int32_t grave_goods_row(game_state *st, int32_t row);    /* @bin 0x0040CD30 */

/* rotate.c: the view after a rotation */
int32_t view_width(game_state *st);           /* @bin 0x00430CB0 */
int32_t view_height(game_state *st);          /* @bin 0x00430CA0 */
void view_table_build(game_state *st, int32_t orient); /* @bin 0x00430530 */
void view_find(game_state *st, uint32_t value); /* @bin 0x004304C0 */
void rotate_view(game_state *st, int dir);    /* @bin 0x00430B20 (+1), 0x00430BE0 (-1) */
void camera_clamp(game_state *st);            /* @bin 0x004DE080 */
void map_rotated(game_state *st);             /* @bin 0x0047D6E0 */

/* cheats.c */
int32_t cheat_run(game_state *st, int32_t index); /* @bin 0x0041D010 */

#endif
