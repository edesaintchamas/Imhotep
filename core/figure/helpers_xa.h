/* Imhotep - figure helpers under the figure actions (package figx, part a): the sources of a
 * buyer's goods, the delivery of goods to a stop, soldier targets.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_HELPERS_XA_H
#define IMHOTEP_FIGURE_HELPERS_XA_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00456760 the nearest source building (by walking route from building `b`) of each good
 * index, in 0x0089B6D8 (27 i32) */
void buyer_find_sources(game_state *st, int32_t fid, int32_t b);

/* @bin 0x00496C70 / 0x00496DE0 deliver good index `idx` of figure `fid` at building `dest` */
int32_t deliver_good_0_7(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t idx,
                         int32_t amount, int32_t slot);
int32_t deliver_good_8_11(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t idx,
                          int32_t amount, int32_t slot);

/* @bin 0x00496550 / 0x00496460 / 0x004963B0 the loads figure fid can take of `good` at `dest`
 * for building b's good index idx (amount already carried) */
int32_t deliver_count_storage(game_state *st, int32_t fid, int32_t good, int32_t b, int32_t dest,
                              int32_t amount, int32_t idx);
int32_t deliver_count_parts(game_state *st, int32_t fid, int32_t good, int32_t b, int32_t dest,
                            int32_t amount, int32_t idx);
int32_t deliver_count_8_11(game_state *st, int32_t good, int32_t fid, int32_t b, int32_t dest,
                           int32_t amount, int32_t idx);

/* @bin 0x00496640 fid (partial: the original never returns when its +0x68 is set) */
int32_t figure_chain_free(game_state *st, int32_t fid);

/* @bin 0x004968A0 / 0x00496660 n loads taken from dest, n followers created behind `lead` */
int32_t deliver_take(game_state *st, int32_t good, int32_t n, int32_t fid, int32_t b, int32_t dest,
                     int32_t lead);
int32_t deliver_take_8_11(game_state *st, int32_t good, int32_t n, int32_t fid, int32_t b, int32_t dest,
                          int32_t lead);

/* @bin 0x00495CF0 / 0x00496010 the same for the figure's slot `slot` (+0x9C, +0xBC) */
int32_t deliver_slot_0_7(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot);
int32_t deliver_slot_8_11(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot);

/* @bin 0x004998F0 the nearest figure of types 0x0B..0x0D to (x, y) */
int32_t soldier_nearest_0b_0d(game_state *st, int32_t x, int32_t y);

/* @bin 0x00499E50 the target of soldier `fid` within `range` (0: none) */
int32_t soldier_find_target(game_state *st, int32_t fid, int32_t range);

/* @bin 0x004B6FF0 a new formation (10..49) at (x, y); its id or 0 */
int32_t formation_create_0x4b6ff0(game_state *st, int32_t a, int32_t kind, int32_t sub, int32_t x, int32_t y);

/* @bin 0x004463C0 the message of event n (type 2) */
void event_message_0x4463c0(game_state *st, int32_t n, int32_t arg);

/* @bin 0x00469140 the building on tile (x, y) (grid offset `tile`) is destroyed */
void building_destroy_at_0x469140(game_state *st, int32_t x, int32_t y, int32_t tile);

#endif
