/* Imhotep - the sound driver of build/imhotep: what Miles did for the original (SDL audio + a mixer).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * docs/presentation.md 6: 22050 Hz, 16 bit, stereo (0x0055E38B); 5 sample voices (0x0055DEA2(5)),
 * 3 streams (0 music, 1 speech, 2 ambient). The game decides what plays (core/frame/sound.c,
 * core/sim/platform.c); this is the driver's part:
 * - samples (0x0055F0E8): nothing when the same sound still plays on a voice; else the first voice
 *   that is not playing (none free: the request is dropped, as 0x0055F02D returns -1); the .wav (RIFF
 *   PCM) is loaded once and kept;
 * - streams (AIL_open_stream / AIL_start_stream ...): an MP3 decoded as it plays (media.c), `loops`
 *   times (0: forever), "playing" until its end;
 * - volumes 0..127 (Miles' scale; the game gives 0..100 as they are), pan 0 left .. 64 centre .. 127
 *   right.
 * A 4th stream carries a playing video's sound (video.c). */
#ifndef IMHOTEP_PLATFORM_AUDIO_H
#define IMHOTEP_PLATFORM_AUDIO_H

#include <stdint.h>

/* open the output device; 0, or -1 (the game then runs silent: the requests are accepted) */
int audio_init(void);
void audio_quit(void);
/* decode more of the streams (call every frame, and while a modal loop runs) */
void audio_update(void);

/* sim_platform callbacks: `path` is the host path (files_resolve) */
void audio_sample_play(int32_t sound, const char *path, int32_t volume, int32_t pan, int32_t loops);
int32_t audio_stream_play(const char *path, int32_t channel, int32_t loops, int32_t volume);
int audio_stream_playing(int32_t channel);
void audio_stream_stop(int32_t channel);
int32_t audio_stream_volume(int32_t channel);
void audio_stream_set_volume(int32_t channel, int32_t volume);

/* the video's sound (channel 3): frames queued as they are decoded; how many are still queued */
#define AUDIO_VIDEO 3
void audio_video_start(void);
void audio_video_push(const int16_t *frames, int n);
int audio_video_queued(void);
void audio_video_stop(void);
/* output frames played since audio_video_start (the video's clock) */
int64_t audio_video_played(void);
int audio_ok(void);
/* the loudest output sample so far, the output frames mixed (checks) */
int audio_peak(void);
int64_t audio_mixed(void);

#endif
