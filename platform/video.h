/* Imhotep - the Bink videos of build/imhotep (docs/presentation.md 7).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PLATFORM_VIDEO_H
#define IMHOTEP_PLATFORM_VIDEO_H

#include <SDL.h>

typedef struct {
    SDL_Renderer *renderer;
    int width, height;  /* the game's screen (logical size): the display mode the game set */
    int quit;           /* set when the window is closed during a video */
    int max_ms;         /* tests: stop after this long (0: to the end) */
    const char *shot;   /* tests: the window saved there (BMP) when it stops */
    int frames_shown;   /* out: video frames shown */
} video_target;

typedef struct video video;

/* @bin 0x004135DA BinkOpen: the Bink video at the host path opened (its frame size in *width,
 * *height), or NULL when it cannot be opened */
video *video_open_bink(const char *path, int *width, int *height);

/* @bin 0x00413690 the opened video played to its end, or until a key, an Alt key or a left click
 * (WM_KEYDOWN, WM_SYSKEYDOWN, WM_LBUTTONDOWN: 0x004137C4), the cursor hidden (ShowCursor), its frames
 * at x, y of the game's screen (the game centres it and, fullscreen, switches the display to 640 x 480
 * first: core/sim/platform.c); then closed (0x00413420 BinkClose) and freed. Returns 1. */
int video_play_opened(video *v, video_target *t, int x, int y);

#endif
