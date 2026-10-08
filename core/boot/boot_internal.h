/* Imhotep - the boot's modules (package boot): what boot.c calls.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BOOT_BOOT_INTERNAL_H
#define IMHOTEP_BOOT_BOOT_INTERNAL_H

#include <stdint.h>

#include "boot/boot.h"
#include "state/state.h"

/* memory outside the state from the host (state_unported(site) without one) */
uint32_t boot_alloc(game_state *st, const boot_host *host, uint32_t size, uint32_t site);
void boot_release(game_state *st, const boot_host *host, uint32_t address);

/* the bytes of a whole game file (platform_file_read) into `dst` (an address of the state or of
 * mapped memory, at most `cap` bytes); returns the file's size, -1 when it cannot be read */
int32_t boot_file_into(game_state *st, const char *path, uint32_t dst, uint32_t cap);
/* @bin 0x00561C96 CRT atoi on a string of the state or mapped memory */
int32_t boot_atoi(game_state *st, uint32_t p);

/* settings.c */
int32_t boot_settings(game_state *st, const boot_host *host, int32_t screen_width);                /* @bin 0x004147F0 */
int32_t boot_ini_read(game_state *st, const boot_host *host);                  /* @bin 0x00414A90 */
int32_t boot_file_read(game_state *st, const char *path, uint32_t dst);         /* @bin 0x004CEB10 */
int32_t boot_text_load(game_state *st, const boot_host *host, const char *text, const char *mm); /* 0x004CEC40 */
void boot_names(game_state *st);                                                 /* @bin 0x004149D0 */

/* graphics.c */
int32_t boot_graphics(game_state *st, const boot_host *host); /* @bin 0x00414420 */

/* steps.c */
int32_t boot_541310(game_state *st, const boot_host *host); /* @bin 0x00541310 */
int32_t boot_44efa0(game_state *st, const boot_host *host); /* @bin 0x0044EFA0 */
void boot_541d50(game_state *st);                           /* @bin 0x00541D50 */
void boot_4dee10(game_state *st);                           /* @bin 0x004DEE10 */
void boot_4cf8e0(game_state *st);                           /* @bin 0x004CF8E0 */

/* ctors.c: one static constructor of the table 0x00576000 (its address) */
void boot_ctor(game_state *st, const boot_host *host, uint32_t fn);
/* monuments.c: the k-th monument object set-up of WinMain (0x004013F0 on 0x006DE650, ...) */
void boot_monument_setup(game_state *st, int k);

/* display.c */
int32_t boot_display_setup(game_state *st, const boot_host *host, const boot_display *display); /* 0x004CF960 */
int32_t boot_bink_object(game_state *st, const boot_host *host);

/* sound.c */
void boot_sound_start(game_state *st, const boot_host *host); /* @bin 0x005394F0 */
int32_t sound_register(game_state *st, uint32_t name);        /* @bin 0x00539C60 */

#endif
