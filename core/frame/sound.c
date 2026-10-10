/* Imhotep - the frame's sound logic: ambient slots, the city's ambient stream and random sounds, the
 * music, the stream fades (docs/presentation.md 6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The frame 0x004D6B60 calls these after the present: the ambient slots 0x0041D560 (step 17), then
 * (step 19) the music 0x00539820, a random city sound 0x0053ACF0, the ambient stream 0x0053B960 and
 * the fades 0x0053B6F0. They write state (the slots, chunk 92; the music object 0x00E39180; the
 * ambient choice 0x00F55B80; the CRT rand) and ask the sound driver for streams and samples through
 * the platform requests of core/sim/platform.h. The city view's ground pass 0x00430EF0 feeds them:
 * 0x0053B670 clears the ambient counters 0x00F55B50 and 0x0041D410 marks the slots of the visible
 * buildings. The frame clock is 0x00E92E0C (timeGetTime at the frame's start); the music's wait reads
 * GetTickCount (platform_clock_ms). */
#include "frame/sound.h"

#include "frame/view.h"
#include "figure/actions_c.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define STREAMS (G_SOUND + 0x1D8)
#define POPULATION(st) S32(st, CURRENT_CITY(st) + CITY_POPULATION)

/* ---- ambient slots (chunk 92: 70 slots of 0x80 bytes at 0x00702900) ----
 * slot +0 seen this frame, +4 frames seen, +8 frames needed, +0x0C .. +0x1C five counters (by the
 * ground pass's second argument), +0x20 the next sound, +0x24 sounds, +0x28 the sounds (8 at most),
 * +0x4C times played, +0x50 last played (frame clock), +0x54 interval, +0x58 ready */
/* a - b as the original's 32-bit sub */
#define SUB(a, b) ((int32_t) ((uint32_t) (a) - (uint32_t) (b)))
#define SLOT(k) (G_SOUND_SLOTS + 0x80u * (uint32_t) (k))

void ambient_building(game_state *st, int32_t building, int32_t counter)
{
    /* @bin 0x0041D41F a building in use (+0 == 1) whose type (+0x10) has a slot (table 0x0057C780) */
    uint32_t b = BUILDING_ADDR(building);
    if (U8(st, b + BLD_STATE) != 1) {
        return;
    }
    int16_t type = S16(st, b + BLD_TYPE);
    int32_t slot = S32(st, G_AMBIENT_SLOT_BY_TYPE + 4u * (uint32_t) (int32_t) type);
    if (slot <= 0) {
        return;
    }
    /* @bin 0x0041D445 types 0x1E, 0x1F, 0x20, 0x21, 0xE2: only while the city +0x4634 is <= 0 and
     * the building's +0x40 is > 0 */
    if (type == 0x1F || type == 0x1E || type == 0x21 || type == 0x20 || type == 0xE2) {
        if (S32(st, CURRENT_CITY(st) + CITY_U4634) > 0 || S16(st, b + BLD_WORKERS) <= 0) {
            return;
        }
    }
    /* @bin 0x0041D527 */
    S32(st, SLOT(slot)) = 1;
    S32(st, SLOT(slot) + 4) += 1;
    S32(st, SLOT(slot) + 0x0C + 4u * (uint32_t) counter) += 1;
}

void ambient_step(game_state *st)
{
    int32_t now = S32(st, G_FRAME_CLOCK);
    /* @bin 0x0041D576 slots 1..69: ready when seen this frame for long enough (+4 >= +8) and the
     * interval +0x54 has passed since +0x50; a slot not seen is reset */
    for (int32_t k = 1; k < 70; k++) {
        uint32_t s = SLOT(k);
        S32(st, s + 0x58) = 0;
        if (S32(st, s) <= 0) {
            S32(st, s + 4) = 0;
            for (uint32_t i = 0; i < 5; i++) {
                S32(st, s + 0x0C + 4 * i) = 0;
            }
        } else {
            S32(st, s) = 0;
            if (S32(st, s + 4) >= S32(st, s + 8) && SUB(now, S32(st, s + 0x50)) >= S32(st, s + 0x54)) {
                S32(st, s + 0x58) = 1;
            }
        }
    }
    /* @bin 0x0041D5BE every 2000 ms (0x007028FC: the last time), the ready slot played longest ago */
    if (SUB(now, S32(st, G_SOUND_LAST_TIME)) < 2000) {
        return;
    }
    int32_t best = 0, oldest = 0;
    for (int32_t k = 1; k < 70; k++) {
        uint32_t s = SLOT(k);
        if (S32(st, s + 0x58) > 0 && SUB(now, S32(st, s + 0x50)) > oldest) {
            oldest = SUB(now, S32(st, s + 0x50));
            best = k;
        }
    }
    if (best <= 0) {
        return;
    }
    uint32_t s = SLOT(best);
    /* @bin 0x0041D610 its next sound (up to 8 tries while the player 0x0047D350 refuses: it is
     * `xor eax, eax; ret`, so 8), 0x007028F8 the sound */
    for (int32_t tries = 0; tries < 8; tries++) {
        S32(st, s + 0x20) += 1;
        if (S32(st, s + 0x20) >= S32(st, s + 0x24)) {
            S32(st, s + 0x20) = 0;
        }
        S32(st, G_AMBIENT_SOUND) = S32(st, s + 0x28 + 4u * (uint32_t) S32(st, s + 0x20));
        if (return_zero(st, S32(st, G_AMBIENT_SOUND)) != 0) {
            break;
        }
    }
    /* @bin 0x0041D656 the variant: counter +0x14 > 10: 2; +0x0C > 10: 0; else +0x1C <= 10: 2, else 4 */
    int32_t variant = S32(st, s + 0x14) > 10 ? 2 : S32(st, s + 0x0C) > 10 ? 0 : S32(st, s + 0x1C) <= 10 ? 2 : 4;
    (void) variant; /* the second argument of 0x0047D350, which plays nothing */
    return_zero(st, S32(st, G_AMBIENT_SOUND));
    /* @bin 0x0041D69A */
    now = S32(st, G_FRAME_CLOCK);
    S32(st, s + 0x50) = now;
    S32(st, s + 4) = 0;
    S32(st, s + 0x0C) = 0;
    S32(st, G_SOUND_LAST_TIME) = now;
    S32(st, s + 0x10) = 0;
    S32(st, s + 0x14) = 0;
    S32(st, s + 0x18) = 0;
    S32(st, s + 0x1C) = 0;
    S32(st, s + 0x4C) += 1;
}

void ambient_counters_clear(game_state *st)
{
    /* @bin 0x0053B670 the 12 terrain counters 0x00F55B50 and 0x00F55B84 */
    for (uint32_t i = 0; i < 12; i++) {
        S32(st, G_AMBIENT_COUNTERS + 4 * i) = 0;
    }
    S32(st, G_AMBIENT_TOTAL) = 0;
}

int32_t ambient_class(int32_t type)
{
    /* @bin 0x0053B830 jump table 0x0053B868 by the bytes at 0x0053B87C (types 6 .. 0xE7): 1 town,
     * 2, 3, 4 (assumed: the ambient classes of the ground pass's counters), 0 otherwise */
    if (type == 0x06 || type == 0x0A || (type >= 0x0B && type <= 0x2D) || type == 0x2F || type == 0x31 ||
        type == 0x33 || type == 0x35 || type == 0x37 || type == 0x3A || type == 0x3B || type == 0x46 ||
        type == 0x4D || type == 0x4E || type == 0x4F || type == 0x51 || type == 0x5A || type == 0x5C ||
        type == 0x5E || type == 0x5F || type == 0x87 || (type >= 0x8C && type <= 0x91) ||
        (type >= 0x97 && type <= 0xA0) || (type >= 0xA7 && type <= 0xAD) || type == 0xB8 || type == 0xCE ||
        type == 0xE2) {
        return 1;
    }
    if ((type >= 0x47 && type <= 0x4C) || (type >= 0x6A && type <= 0x73) || type == 0x88 || type == 0xA1 ||
        type == 0xA2 || type == 0xB1 || type == 0xB2 || type == 0xB3 || type == 0xB5 || type == 0xB6 ||
        type == 0xCC || type == 0xCD || type == 0xD9 || type == 0xDD || type == 0xE7) {
        return 2;
    }
    if (type == 0x56 || type == 0x57 || (type >= 0x65 && type <= 0x68) || type == 0xE0) {
        return 3;
    }
    if (type == 0xB7 || type == 0xD2) {
        return 4;
    }
    return 0;
}

/* ---- streams: fades, the ambient stream ---- */

void stream_fade_out(game_state *st, int32_t channel)
{
    /* @bin 0x0053B6F0 while the channel's stream plays its volume goes down by 5 a frame; below 0 it
     * is closed. Once it is over (or there is none) the fade flag of channel 0 (0x00F55B8C) or 1
     * (0x00F55B90) is cleared */
    if (S32(st, G_SOUND_ON) != 0 && S32(st, STREAMS + 4u * (uint32_t) channel) != 0 &&
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, channel) != 0) {
        int32_t v = GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME, channel) - 5;
        if (v >= 0) { /* jns */
            GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_SET, channel, v);
            return;
        }
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, channel);
    }
    if (channel == 0) {
        S32(st, G_STREAM_FADE0) = 0;
    } else if (channel == 1) {
        S32(st, G_STREAM_FADE1) = 0;
    }
}

void stream_fade_in(game_state *st, int32_t channel)
{
    /* @bin 0x0053B790 a playing stream's volume up by 5, to its option's volume at most (0: music
     * 0x00E38EC8 (the cap is applied, then set), 1: speech 0x00E38ECC, 2: city 0x00E38ED0) */
    if (S32(st, G_SOUND_ON) == 0 || S32(st, STREAMS + 4u * (uint32_t) channel) == 0 ||
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, channel) == 0) {
        return;
    }
    int32_t v = GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME, channel) + 5;
    if (channel == 0 || channel == 1 || channel == 2) {
        int32_t cap = channel == 0 ? S32(st, G_MUSIC_VOLUME) : channel == 1 ? S32(st, G_SPEECH_VOLUME) : S32(st, G_CITY_VOLUME);
        if (v > cap) {
            v = cap;
        }
    }
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_SET, channel, v);
}

void ambient_stream(game_state *st)
{
    /* @bin 0x0053B960 sound on, city sounds on */
    if (S32(st, G_SOUND_ON) == 0 || U8(st, G_CITY_SOUNDS_ON) == 0) {
        return;
    }
    /* @bin 0x0053B988 the largest of the 12 counters 0x00F55B50 (the first one on a tie; -1 when
     * all are <= 0) */
    int32_t most = 0, which = -1;
    for (int32_t i = 0; i < 12; i++) {
        int32_t n = S32(st, G_AMBIENT_COUNTERS + 4u * (uint32_t) i);
        if (n > most) {
            most = n;
            which = i;
        }
    }
    /* @bin 0x0053B9A3 the same as before (0x00F55B80) and still playing: fade it in below 100 */
    if (which == S32(st, G_AMBIENT_CHOICE) && GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 2) != 0) {
        if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME, 2) < 100) {
            stream_fade_in(st, 2);
        }
        return;
    }
    S32(st, G_AMBIENT_CHOICE) = which;
    /* @bin 0x0053BC48 the jump table by counter: the file (strings 0x005FAB58 .. 0x005FACD0) on
     * stream 2 played forever (loops 0), at volume 0 (it fades in) */
    uint32_t file;
    switch (which) {
    case 1: {
        /* @bin 0x0053BB05 housing by the population: < 500, < 2000, < 5000, < 8000, more */
        int32_t pop = POPULATION(st);
        file = pop < 500 ? G_STR_AMBIENT_HOUSING1 : pop < 2000 ? G_STR_AMBIENT_HOUSING2 : pop < 5000 ? G_STR_AMBIENT_HOUSING3 : pop < 8000 ? G_STR_AMBIENT_HOUSING4 : G_STR_AMBIENT_HOUSING5;
        break;
    }
    case 2:
        /* @bin 0x0053BAC3 industrial; the x87 test of 0x00F55B58 against 0.33 * 0x00F55B84 leads to the
         * same call on both sides */
        file = G_STR_AMBIENT_INDUSTRIAL;
        break;
    case 3:
    case 7:
        file = G_STR_AMBIENT_MEADOW; /* meadow */
        break;
    case 4:
        /* @bin 0x0053BBDD monument: 0x005F9058 >= 1000 monument_f, else monument_b */
        file = S32(st, G_U5F9058) >= 1000 ? G_STR_AMBIENT_MONUMENT_F : G_STR_AMBIENT_MONUMENT_B;
        break;
    case 6:
        file = G_STR_AMBIENT_DESERT; /* desert */
        break;
    case 8:
        file = G_STR_AMBIENT_FLOODPLAIN; /* floodplain */
        break;
    case 9:
        file = G_STR_AMBIENT_ROCK; /* rock */
        break;
    case 10:
        file = G_STR_AMBIENT_WATER; /* water */
        break;
    case 11:
        file = G_STR_AMBIENT_VEGETATION; /* vegetation */
        break;
    default:
        /* @bin 0x0053BC2B none (counter 5, or nothing counted): stream 2 closed */
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 2);
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_SET, 2, 0);
        return;
    }
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, (int32_t) file, 2, 0);
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_SET, 2, 0);
}

/* ---- random city sounds ---- */

/* (rand() * n) / 32768, rounded toward zero (cdq; and edx, 0x7FFF; add; sar 15) */
static int32_t rand_below(game_state *st, int32_t n)
{
    int32_t v = (int32_t) ((uint32_t) crt_rand(st) * (uint32_t) n);
    return (v + (v < 0 ? 0x7FFF : 0)) >> 15;
}

void city_random_sound(game_state *st)
{
    /* @bin 0x0053ACF0 sound on, city sounds on */
    if (S32(st, G_SOUND_ON) == 0 || U8(st, G_CITY_SOUNDS_ON) == 0) {
        return;
    }
    /* @bin 0x0053AD0C a random point of the city view (width: 0x00F53F60 when 0x00F53F78, else
     * 0x00F53F64; height 0x00E38D98) */
    int32_t x = rand_below(st, S32(st, G_SIDEBAR_COLLAPSED) != 0 ? S32(st, G_SIDEBAR_X_COLLAPSED) : S32(st, G_SIDEBAR_X));
    int32_t y = rand_below(st, S32(st, G_DRAW_H));
    int32_t tile = view_screen_to_tile(st, x, y);
    if (tile >= 0x0FFF0000) {
        return;
    }
    int32_t building = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    int32_t id;
    if (building != 0) {
        uint32_t b = BUILDING_ADDR(building);
        if (S16(st, b + BLD_TYPE) == 0x39) {
            /* @bin 0x0053AD9B type 0x39 by its +0x12: 0x0D, 0x0B, others (tables 0x00F54990,
             * 0x00F549E0, 0x00F549F4 with their counts after 4 entries) */
            int16_t sub = S16(st, b + BLD_SUBTYPE);
            uint32_t table = sub == 0x0D ? G_SOUNDS_39_SUB0D : sub == 0x0B ? G_SOUNDS_39_SUB0B : G_SOUNDS_39_OTHER;
            id = S32(st, table + 4u * (uint32_t) rand_below(st, S32(st, table + 0x10)));
        } else {
            /* @bin 0x0053AE11 the building type's sounds: 0x00F54670 + 20 * type, count at +0x10 */
            int32_t type = S16(st, b + BLD_TYPE);
            uint32_t table = G_BUILDING_SOUND_TABLE + 20u * (uint32_t) type;
            id = S32(st, table + 4u * (uint32_t) rand_below(st, S32(st, table + 0x10)));
        }
    } else {
        /* @bin 0x0053AE87 the terrain's class: bit 0: 5, bit 1: 2, bit 2 without bit 19: 1, bit 11:
         * 3, bit 16: 4, else none */
        int32_t k;
        if (terrain == 0) {
            return;
        }
        if (terrain & 1) {
            k = 5;
        } else if (terrain & 2) {
            k = 2;
        } else if ((terrain & 4) && !(terrain & 0x80000)) {
            k = 1;
        } else if (terrain & 0x800) {
            k = 3;
        } else if (terrain & 0x10000) {
            k = 4;
        } else {
            return;
        }
        /* @bin 0x0053AED3 the class's sounds: 0x00F54550 + 48 * class, count at +0x2C */
        uint32_t table = G_AMBIENT_SOUNDS + 48u * (uint32_t) k;
        id = S32(st, table + 4u * (uint32_t) rand_below(st, S32(st, table + 0x2C)));
    }
    if (id <= -1) {
        return;
    }
    /* @bin 0x0053AE49 pan from x (as a figure's), volume: city sounds 0x00E38ED0, once */
    S32(st, G_REQUEST + 12) = platform_view_pan(st, x);
    S32(st, G_REQUEST + 8) = S32(st, G_CITY_VOLUME);
    S32(st, G_REQUEST + 4) = id;
    S32(st, G_REQUEST) = -1;
    S32(st, G_REQUEST + 16) = 1;
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_SAMPLE_PLAY, (int32_t) G_REQUEST);
}

/* ---- music (the object 0x00E39180, music.txt: 0x00539250) ----
 * per mission (s8 0x00E38EA8) a block of 0x1B10 bytes: 50 "M" tunes then 10 "A" tunes of 0x70 bytes
 * (name, +0x64 seconds to wait after it, +0x68 / +0x6C population window), +0x1A40 + 4 * i: "M" tune i
 * is followed by an "A" tune (1) or not, +0x1B08 / +0x1B0C the counts. Object +0x59A54 / +0x59A58 the
 * "M" tunes of the population, +0x59A5C the first one last chosen, +0x59A60 the next tune, +0x59A64 the
 * step (0 tune, 1 "A" tune, 2 start waiting, 3 wait), +0x59A68 the wait's start, +0x59A6C set after a
 * wait; 0x00F55B98 the wait in ms */
#define MUSIC_BLOCK(st, self) ((self) + 0x1B10u * (uint32_t) (int32_t) S8(st, G_MISSION))
#define TUNE(st, self, i) (MUSIC_BLOCK(st, self) + 0x70u * (uint32_t) (i))

void music_set_window(game_state *st, uint32_t self, int32_t first, int32_t last)
{
    /* @bin 0x00539A50 */
    S32(st, self + 0x59A54) = first;
    S32(st, self + 0x59A58) = last;
}

void music_window(game_state *st, uint32_t self, int32_t force)
{
    /* @bin 0x00539730 when the population left the current tune's window (or force): the "M" tunes
     * whose window holds it, from the first that does to the last before one whose minimum is above */
    int32_t pop = POPULATION(st);
    uint32_t block = MUSIC_BLOCK(st, self);
    uint32_t cur = TUNE(st, self, S32(st, self + 0x59A60));
    if (pop > S32(st, cur + 0x6C) || pop < S32(st, cur + 0x68) || force != 0) {
        int32_t n = S32(st, block + 0x1B08);
        int32_t i = 0;
        while (i < n && !(pop >= S32(st, block + 0x70u * (uint32_t) i + 0x68) &&
                          pop <= S32(st, block + 0x70u * (uint32_t) i + 0x6C))) {
            i++;
        }
        int32_t first = i;
        while (i < n && S32(st, block + 0x70u * (uint32_t) i + 0x68) <= pop) {
            i++;
        }
        music_set_window(st, self, first, i - 1);
    }
    /* @bin 0x005397DA the next tune back to the first when outside; a new first tune starts the
     * music over (step 1) unless a wait just ended */
    int32_t first = S32(st, self + 0x59A54);
    if (S32(st, self + 0x59A60) < first || S32(st, self + 0x59A60) > S32(st, self + 0x59A58)) {
        S32(st, self + 0x59A60) = first;
    }
    if (first != S32(st, self + 0x59A5C)) {
        S32(st, self + 0x59A5C) = first;
        if (S32(st, self + 0x59A6C) == 0) {
            S32(st, self + 0x59A64) = 1;
        }
    }
}

void music_step(game_state *st, uint32_t self)
{
    /* @bin 0x00539820 sound on, music on */
    if (S32(st, G_SOUND_ON) == 0 || U8(st, G_MUSIC_ON) == 0) {
        return;
    }
    if (S32(st, self + 0x59A64) == 0) {
        music_window(st, self, 0);
    }
    /* @bin 0x0053984F the music volume option to stream 0 */
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_VOLUME_MUSIC, S32(st, G_MUSIC_VOLUME));
    uint32_t block = MUSIC_BLOCK(st, self);
    switch (S32(st, self + 0x59A64)) { /* @bin 0x00539A28 */
    case 0:
        /* @bin 0x00539875 the next "M" tune, once; its wait; then an "A" tune or the wait */
        if (S32(st, block + 0x1B08) <= 0) {
            return;
        }
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, (int32_t) TUNE(st, self, S32(st, self + 0x59A60)), 0, 1);
        S32(st, G_MUSIC_WAIT) = S32(st, TUNE(st, self, S32(st, self + 0x59A60)) + 0x64) * 1000;
        S32(st, self + 0x59A6C) = 0;
        S32(st, self + 0x59A60) += 1;
        S32(st, self + 0x59A64) =
            (S32(st, block + 0x1A40 + 4u * (uint32_t) S32(st, self + 0x59A60)) != 1 ? 1 : 0) + 1;
        return;
    case 1:
        /* @bin 0x00539946 a random "A" tune, once */
        if (S32(st, block + 0x1B0C) > 0) {
            int32_t a = rand_below(st, S32(st, block + 0x1B0C));
            GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, (int32_t) TUNE(st, self, a + 50), 0, 1);
        }
        S32(st, self + 0x59A64) = 2;
        return;
    case 2:
        /* @bin 0x005399C1 the wait starts (GetTickCount) */
        S32(st, self + 0x59A64) = 3;
        S32(st, self + 0x59A68) = (int32_t) platform_clock_ms(st);
        return;
    case 3:
        /* @bin 0x005399DA after the wait (unsigned compare): back to the first tune past the last */
        if (platform_clock_ms(st) > (uint32_t) S32(st, self + 0x59A68) + (uint32_t) S32(st, G_MUSIC_WAIT)) {
            if (S32(st, self + 0x59A60) > S32(st, self + 0x59A58)) {
                S32(st, self + 0x59A60) = S32(st, self + 0x59A54);
            }
            S32(st, self + 0x59A64) = 0;
            S32(st, self + 0x59A6C) = 1;
        }
        return;
    default:
        return;
    }
}

/* ---- the table ---- */

static int32_t run_ambient_building(game_state *st, const int32_t *a)
{
    ambient_building(st, a[0], a[1]);
    return 0;
}

static int32_t run_ambient_step(game_state *st, const int32_t *a)
{
    (void) a;
    ambient_step(st);
    return 0;
}

static int32_t run_counters_clear(game_state *st, const int32_t *a)
{
    (void) a;
    ambient_counters_clear(st);
    return 0;
}

static int32_t run_ambient_class(game_state *st, const int32_t *a)
{
    (void) st;
    return ambient_class(a[0]);
}

static int32_t run_fade_out(game_state *st, const int32_t *a)
{
    stream_fade_out(st, a[0]);
    return 0;
}

static int32_t run_fade_in(game_state *st, const int32_t *a)
{
    stream_fade_in(st, a[0]);
    return 0;
}

static int32_t run_ambient_stream(game_state *st, const int32_t *a)
{
    (void) a;
    ambient_stream(st);
    return 0;
}

static int32_t run_city_random_sound(game_state *st, const int32_t *a)
{
    (void) a;
    city_random_sound(st);
    return 0;
}

static int32_t run_music_set_window(game_state *st, const int32_t *a)
{
    music_set_window(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1]);
    return 0;
}

static int32_t run_music_window(game_state *st, const int32_t *a)
{
    music_window(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

static int32_t run_music_step(game_state *st, const int32_t *a)
{
    music_step(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

const ported_function frame_sound_ports[] = {
    {0x0041D410u, "ambient_building", 2, 0, 0, run_ambient_building, 0},
    {0x0041D560u, "ambient_step", 0, 0, 0, run_ambient_step, 0},
    {0x0053B670u, "ambient_counters_clear", 0, 0, 0, run_counters_clear, 0},
    {0x0053B830u, "ambient_class", 1, 32, 0, run_ambient_class, 0},
    {0x0053B6F0u, "stream_fade_out", 1, 0, 0, run_fade_out, 0},
    {0x0053B790u, "stream_fade_in", 1, 0, 0, run_fade_in, 0},
    {0x0053B960u, "ambient_stream", 0, 0, 0, run_ambient_stream, 0},
    {0x0053ACF0u, "city_random_sound", 0, 0, 0, run_city_random_sound, 0},
    {0x00539A50u, "music_set_window", 2, 0, G_MUSIC, run_music_set_window, 0},
    {0x00539730u, "music_window", 1, 0, G_MUSIC, run_music_window, 0},
    {0x00539820u, "music_step", 0, 0, G_MUSIC, run_music_step, 0},
};
const int frame_sound_ports_count = sizeof(frame_sound_ports) / sizeof(frame_sound_ports[0]);
