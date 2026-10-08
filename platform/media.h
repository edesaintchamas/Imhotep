/* Imhotep - MP3 and Bink decoding for build/imhotep, through FFmpeg (libavformat, libavcodec,
 * libswresample, libswscale; dynamically linked).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original plays MP3 streams through Miles (MP3DEC.ASI) and Bink videos through BINKW32.DLL;
 * neither is part of the game's logic (docs/presentation.md 6, 7). Audio comes out as signed 16-bit
 * stereo at MEDIA_RATE (the original's mixer: 22050 Hz, 16 bit, stereo, 0x0055E38B); video frames as
 * 32-bit BGRA (SDL_PIXELFORMAT_ARGB8888 on a little-endian host). */
#ifndef IMHOTEP_PLATFORM_MEDIA_H
#define IMHOTEP_PLATFORM_MEDIA_H

#include <stdint.h>

#define MEDIA_RATE 22050

typedef struct media media;

/* open a file (host path); video: also decode its video stream. NULL when it cannot be opened or
 * has no audio (and, with video, no video) */
media *media_open(const char *path, int video);
void media_close(media *m);
/* decode the next packet: 1 audio was added, 2 a video frame is ready, 3 something else, 0 end of
 * file, -1 error */
int media_step(media *m);
/* take up to max stereo frames of decoded audio; returns the count */
int media_take(media *m, int16_t *out, int max);
/* stereo frames of decoded audio not taken yet */
int media_pending(const media *m);
/* back to the start (for looping streams); 0 or -1 */
int media_rewind(media *m);
/* the video: size, the last frame (BGRA rows of width * 4 bytes) and its time in ms */
int media_width(const media *m);
int media_height(const media *m);
const uint8_t *media_frame(const media *m);
int64_t media_frame_ms(const media *m);

#endif
