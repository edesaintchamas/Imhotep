/* Imhotep - the sound driver of build/imhotep (see audio.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "platform/game_files.h"
#include "platform/media.h"

#define VOICES 5
#define STREAMS 4                 /* 0 music, 1 speech, 2 ambient, 3 video */
#define RING (MEDIA_RATE * 2)     /* stereo frames buffered per stream: 2 s */
#define MAX_SOUNDS 4096

typedef struct {
    int16_t *data; /* stereo frames */
    int frames;
    int loaded;    /* 1 loaded, -1 cannot be loaded */
} sample;

typedef struct {
    int active, sound;
    const int16_t *data;
    int frames, pos, loops, volume, pan;
} voice;

typedef struct {
    int open;       /* a stream is open (handle given) */
    int finished;   /* the decoder is at its end, nothing more to queue */
    media *m;
    int loops, volume;
    int16_t ring[RING * 2];
    int rd, count;  /* frames */
    int64_t played;
} stream;

static SDL_AudioDeviceID dev;
static sample samples[MAX_SOUNDS];
static voice voices[VOICES];
static stream streams[STREAMS];
static int32_t next_handle = 1;
static int peak;           /* the loudest output sample so far (a check that something is heard) */
static int64_t mixed;      /* output frames mixed */

int audio_peak(void)
{
    return peak;
}

int64_t audio_mixed(void)
{
    return mixed;
}

int audio_ok(void)
{
    return dev != 0;
}

static float gain(int volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 127) {
        volume = 127;
    }
    return (float) volume / 127.0f;
}

static void mix(void *ud, Uint8 *out8, int len)
{
    (void) ud;
    int16_t *out = (int16_t *) out8;
    int n = len / 4;
    for (int i = 0; i < n; i++) {
        float l = 0, r = 0;
        for (int v = 0; v < VOICES; v++) {
            voice *vc = &voices[v];
            if (!vc->active) {
                continue;
            }
            float g = gain(vc->volume);
            float pl = (127 - vc->pan) / 63.5f, pr = vc->pan / 63.5f;
            if (pl > 1) {
                pl = 1;
            }
            if (pr > 1) {
                pr = 1;
            }
            l += vc->data[2 * vc->pos] * g * pl;
            r += vc->data[2 * vc->pos + 1] * g * pr;
            if (++vc->pos >= vc->frames) {
                vc->pos = 0;
                if (vc->loops != 0 && --vc->loops == 0) {
                    vc->active = 0;
                }
            }
        }
        for (int s = 0; s < STREAMS; s++) {
            stream *st = &streams[s];
            if (!st->open || st->count == 0) {
                continue;
            }
            float g = s == AUDIO_VIDEO ? 1.0f : gain(st->volume);
            l += st->ring[2 * st->rd] * g;
            r += st->ring[2 * st->rd + 1] * g;
            st->rd = (st->rd + 1) % RING;
            st->count--;
            st->played++;
        }
        out[2 * i] = (int16_t) (l > 32767 ? 32767 : l < -32768 ? -32768 : l);
        out[2 * i + 1] = (int16_t) (r > 32767 ? 32767 : r < -32768 ? -32768 : r);
        int a = out[2 * i] < 0 ? -out[2 * i] : out[2 * i];
        if (a > peak) {
            peak = a;
        }
    }
    mixed += n;
}

int audio_init(void)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return -1;
    }
    SDL_AudioSpec want = {0}, have;
    want.freq = MEDIA_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    want.callback = mix;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev) {
        return -1;
    }
    SDL_PauseAudioDevice(dev, 0);
    return 0;
}

void audio_quit(void)
{
    if (dev) {
        SDL_CloseAudioDevice(dev);
        dev = 0;
    }
    for (int s = 0; s < STREAMS; s++) {
        media_close(streams[s].m);
        streams[s].m = NULL;
    }
    for (int i = 0; i < MAX_SOUNDS; i++) {
        free(samples[i].data);
    }
}

static void lock(void)
{
    if (dev) {
        SDL_LockAudioDevice(dev);
    }
}

static void unlock(void)
{
    if (dev) {
        SDL_UnlockAudioDevice(dev);
    }
}

/* ---- samples ---- */

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

/* a RIFF WAVE file, PCM 8 or 16 bit, mono or stereo, as stereo 16-bit frames at MEDIA_RATE (the
 * game's files are 22050 Hz 16 bit: docs/presentation.md 6.4; other rates by nearest sample) */
static int load_wav(const char *path, sample *s)
{
    size_t len;
    uint8_t *d = read_file(path, &len);
    if (!d || len < 12 || memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) {
        free(d);
        return -1;
    }
    int fmt = 0, ch = 0, rate = 0, bits = 0;
    const uint8_t *pcm = NULL;
    uint32_t pcm_len = 0;
    for (size_t p = 12; p + 8 <= len;) {
        uint32_t n = le32(d + p + 4);
        if (memcmp(d + p, "fmt ", 4) == 0 && n >= 16 && p + 8 + 16 <= len) {
            fmt = d[p + 8] | d[p + 9] << 8;
            ch = d[p + 10] | d[p + 11] << 8;
            rate = (int) le32(d + p + 12);
            bits = d[p + 22] | d[p + 23] << 8;
        } else if (memcmp(d + p, "data", 4) == 0) {
            pcm = d + p + 8;
            pcm_len = (uint32_t) (len - p - 8) < n ? (uint32_t) (len - p - 8) : n;
        }
        p += 8 + n + (n & 1);
    }
    if (fmt != 1 || (ch != 1 && ch != 2) || (bits != 8 && bits != 16) || !pcm || rate <= 0) {
        free(d);
        return -1;
    }
    int in_frames = (int) (pcm_len / (uint32_t) (ch * bits / 8));
    int64_t out_frames = (int64_t) in_frames * MEDIA_RATE / rate;
    s->data = malloc((size_t) (out_frames > 0 ? out_frames : 1) * 4);
    if (!s->data) {
        free(d);
        return -1;
    }
    for (int64_t i = 0; i < out_frames; i++) {
        int k = (int) (i * rate / MEDIA_RATE);
        int16_t l, r;
        if (bits == 16) {
            const uint8_t *q = pcm + (size_t) k * ch * 2;
            l = (int16_t) (q[0] | q[1] << 8);
            r = ch == 2 ? (int16_t) (q[2] | q[3] << 8) : l;
        } else {
            const uint8_t *q = pcm + (size_t) k * ch;
            l = (int16_t) ((q[0] - 128) << 8);
            r = ch == 2 ? (int16_t) ((q[1] - 128) << 8) : l;
        }
        s->data[2 * i] = l;
        s->data[2 * i + 1] = r;
    }
    s->frames = (int) out_frames;
    free(d);
    return s->frames > 0 ? 0 : -1;
}

void audio_sample_play(int32_t sound, const char *path, int32_t volume, int32_t pan, int32_t loops)
{
    if (sound < 0 || sound >= MAX_SOUNDS || !path) {
        return;
    }
    lock();
    for (int v = 0; v < VOICES; v++) {
        if (voices[v].active && voices[v].sound == sound) {
            unlock(); /* @bin 0x0055F136 the same sound still plays: nothing */
            return;
        }
    }
    int free_voice = -1;
    for (int v = 0; v < VOICES && free_voice < 0; v++) {
        if (!voices[v].active) {
            free_voice = v; /* @bin 0x0055F02D the first voice not playing */
        }
    }
    unlock();
    if (free_voice < 0) {
        return;
    }
    sample *s = &samples[sound];
    if (s->loaded == 0) {
        s->loaded = load_wav(path, s) == 0 ? 1 : -1;
        if (s->loaded < 0) {
            fprintf(stderr, "imhotep: sound %d: cannot load %s\n", sound, path);
        }
    }
    if (s->loaded < 0) {
        return;
    }
    lock();
    voices[free_voice] = (voice){1, sound, s->data, s->frames, 0, loops, volume, pan};
    unlock();
}

/* ---- streams ---- */

static void stream_close(int channel)
{
    stream *s = &streams[channel];
    lock();
    media *m = s->m;
    s->m = NULL;
    s->open = 0;
    s->count = s->rd = 0;
    unlock();
    media_close(m);
}

int32_t audio_stream_play(const char *path, int32_t channel, int32_t loops, int32_t volume)
{
    if (channel < 0 || channel > 2 || !path) {
        return 0;
    }
    stream_close(channel);
    media *m = media_open(path, 0);
    if (!m) {
        fprintf(stderr, "imhotep: stream %d: cannot open %s\n", channel, path);
        return 0;
    }
    stream *s = &streams[channel];
    lock();
    s->m = m;
    s->finished = 0;
    s->loops = loops;
    s->volume = volume;
    s->rd = s->count = 0;
    s->played = 0;
    s->open = 1;
    unlock();
    audio_update();
    return next_handle++;
}

int audio_stream_playing(int32_t channel)
{
    if (channel < 0 || channel >= STREAMS) {
        return 0;
    }
    stream *s = &streams[channel];
    lock();
    int playing = s->open && !(s->finished && s->count == 0);
    unlock();
    return playing;
}

void audio_stream_stop(int32_t channel)
{
    if (channel >= 0 && channel <= 2) {
        stream_close(channel);
    }
}

int32_t audio_stream_volume(int32_t channel)
{
    return channel >= 0 && channel < STREAMS && streams[channel].open ? streams[channel].volume : 0;
}

void audio_stream_set_volume(int32_t channel, int32_t volume)
{
    if (channel >= 0 && channel < STREAMS) {
        lock();
        streams[channel].volume = volume;
        unlock();
    }
}

static void push(stream *s, const int16_t *frames, int n)
{
    lock();
    for (int i = 0; i < n && s->count < RING; i++) {
        int w = (s->rd + s->count) % RING;
        s->ring[2 * w] = frames[2 * i];
        s->ring[2 * w + 1] = frames[2 * i + 1];
        s->count++;
    }
    unlock();
}

void audio_update(void)
{
    int16_t buf[4096 * 2];
    for (int c = 0; c <= 2; c++) {
        stream *s = &streams[c];
        if (!s->open || s->finished || !s->m) {
            continue;
        }
        for (;;) {
            lock();
            int room = RING - s->count;
            unlock();
            if (room < RING / 2) {
                break;
            }
            int pending = media_pending(s->m);
            if (pending > 0) {
                int n = media_take(s->m, buf, pending < 4096 ? pending : 4096);
                n = n < room ? n : room;
                push(s, buf, n);
                continue;
            }
            int r = media_step(s->m);
            if (r == 0 || r < 0) {
                /* the end: again while loops remain (0: forever) */
                if (r == 0 && (s->loops == 0 || --s->loops > 0) && media_rewind(s->m) == 0) {
                    continue;
                }
                s->finished = 1;
                break;
            }
        }
    }
}

/* ---- the video's sound ---- */

void audio_video_start(void)
{
    stream *s = &streams[AUDIO_VIDEO];
    lock();
    s->rd = s->count = 0;
    s->played = 0;
    s->open = 1;
    unlock();
}

void audio_video_push(const int16_t *frames, int n)
{
    push(&streams[AUDIO_VIDEO], frames, n);
}

int audio_video_queued(void)
{
    lock();
    int n = streams[AUDIO_VIDEO].count;
    unlock();
    return n;
}

int64_t audio_video_played(void)
{
    lock();
    int64_t n = streams[AUDIO_VIDEO].played;
    unlock();
    return n;
}

void audio_video_stop(void)
{
    lock();
    streams[AUDIO_VIDEO].open = 0;
    streams[AUDIO_VIDEO].count = 0;
    unlock();
}
