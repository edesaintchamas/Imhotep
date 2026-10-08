/* Imhotep - the Bink videos of build/imhotep (see video.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/video.h"

#include <stdlib.h>
#include <string.h>

#include "platform/audio.h"
#include "platform/media.h"

struct video {
    media *m;
};

video *video_open_bink(const char *path, int *width, int *height)
{
    media *m = path ? media_open(path, 1) : NULL;
    video *v = m ? malloc(sizeof(video)) : NULL;
    if (!v) {
        if (m) {
            media_close(m);
        }
        return NULL;
    }
    v->m = m;
    *width = media_width(m);
    *height = media_height(m);
    return v;
}

int video_play_opened(video *v, video_target *t, int x, int y)
{
    media *m = v->m;
    free(v);
    int vw = media_width(m), vh = media_height(m);
    SDL_Texture *tex = SDL_CreateTexture(t->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, vw, vh);
    uint8_t *next = malloc((size_t) vw * vh * 4);
    if (!tex || !next) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
        free(next);
        media_close(m);
        return 1;
    }
    int lw, lh;
    SDL_RenderGetLogicalSize(t->renderer, &lw, &lh);
    SDL_RenderSetLogicalSize(t->renderer, t->width, t->height);
    SDL_Rect dst = {x, y, vw, vh};
    SDL_ShowCursor(SDL_DISABLE);
    audio_video_start();
    Uint32 start = SDL_GetTicks();
    int shown = 0, pending = 0, done = 0, ended = 0;
    int64_t pending_ms = 0;
    int16_t pcm[4096 * 2];
    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                t->quit = 1;
                done = 1;
            } else if ((e.type == SDL_KEYDOWN && !e.key.repeat) ||
                       (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)) {
                done = 1; /* @bin 0x004137C4 WM_KEYDOWN, WM_SYSKEYDOWN, WM_LBUTTONDOWN end the video */
            }
        }
        int64_t now = (int64_t) (SDL_GetTicks() - start);
        /* decode ahead: the next frame, and a quarter of a second of sound */
        while (!ended && (!pending || audio_video_queued() < MEDIA_RATE / 4)) {
            int r = media_step(m);
            if (r <= 0) {
                ended = 1;
                break;
            }
            int n;
            while ((n = media_take(m, pcm, 4096)) > 0) {
                audio_video_push(pcm, n);
            }
            if (r == 2) {
                memcpy(next, media_frame(m), (size_t) vw * vh * 4); /* a frame not shown yet is dropped */
                pending = 1;
                pending_ms = media_frame_ms(m);
            }
        }
        if (pending && pending_ms <= now) {
            SDL_UpdateTexture(tex, NULL, next, vw * 4);
            pending = 0;
            shown = 1;
            t->frames_shown++;
        }
        int stop = t->max_ms > 0 && now >= t->max_ms;
        SDL_SetRenderDrawColor(t->renderer, 0, 0, 0, 255);
        SDL_RenderClear(t->renderer);
        if (shown) {
            SDL_RenderCopy(t->renderer, tex, NULL, &dst);
        }
        if (stop && t->shot) {
            int ow, oh;
            SDL_GetRendererOutputSize(t->renderer, &ow, &oh);
            SDL_Surface *sf = SDL_CreateRGBSurfaceWithFormat(0, ow, oh, 32, SDL_PIXELFORMAT_ARGB8888);
            if (sf && SDL_RenderReadPixels(t->renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sf->pixels, sf->pitch) == 0) {
                SDL_Surface *c = SDL_ConvertSurfaceFormat(sf, SDL_PIXELFORMAT_RGB24, 0);
                if (c) {
                    SDL_SaveBMP(c, t->shot);
                    SDL_FreeSurface(c);
                }
            }
            if (sf) {
                SDL_FreeSurface(sf);
            }
        }
        SDL_RenderPresent(t->renderer);
        if (stop) {
            done = 1;
        }
        if (ended && !pending && (!audio_ok() || audio_video_queued() == 0)) {
            done = 1;
        }
        audio_update();
        SDL_Delay(1);
    }
    audio_video_stop();
    SDL_ShowCursor(SDL_ENABLE);
    SDL_RenderSetLogicalSize(t->renderer, lw, lh);
    SDL_DestroyTexture(tex);
    free(next);
    media_close(m);
    return 1;
}
