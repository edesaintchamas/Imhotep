/* Imhotep - the mission screens: start / victory picture (window 4), mission choice (0x34),
 * mission briefing (0x35, 0x3B), full-screen pictures (0x37-0x3A) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Full redraws from 0x004D71C0, per frame from 0x004D75D0, input from 0x004D8310 (package
 * wincore). The campaign table (0x006FDD08, 64-byte steps; count 0x006FF608) and the campaign
 * globals are core/newgame's (flow.c, flow2.c); the period choice window 0x17 is package
 * frontend's. */
#include "empire.h"

#include "draw/mem.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define GFX 0x00F55BB0u
#define LAYOUT_DX 0x00E38DA8u
#define LAYOUT_DY 0x00E38DA4u
#define SCR_W 0x00E38DB0u
#define SCR_H 0x00E38DACu
#define WINDOW_ID 0x00E38DDAu
#define REDRAW_ALL 0x00E38DCAu
#define REDRAW_SIDEBAR 0x00E38DC9u
#define TEXT_X 0x00E20778u
#define FONT_HEAD 0x00F56F8Cu
#define FONT_TEXT 0x00F56F9Cu
#define FONT_LIT 0x00F56FA4u
#define FONT_TITLE 0x00F56FB4u
#define CLIP_BOTTOM 0x00D3B550u
#define CLIP_TOP 0x00D3B554u
#define CLIP_RIGHT 0x00D3B558u
#define CLIP_LEFT 0x00D3B55Cu
#define PIC_X 0x00D3B56Cu          /* i32 the source offset of a full-screen picture (1024 x 768) */
#define PIC_Y 0x00D3B568u
#define PIC_W 0x00D3B574u
#define PIC_H 0x00D3B570u
#define CAMPAIGN 0x006FDD08u       /* campaign steps of 0x40 bytes */
#define CAMPAIGN_COUNT 0x006FF608u
#define STEP 0x00E38EABu           /* i8 the current campaign step */
#define STEP_PREV 0x00E38EAAu      /* i8 the previous step */
#define MISSION 0x00E38EA8u        /* i8 the mission (0x7F: none) */
#define CUSTOM 0x00E38EBCu         /* i32 != 0: a custom map */
#define CHOICE 0x007027A4u         /* u8 the mission chosen (1-based, 0: none) */
#define SCREEN_X 0x006FDCA0u       /* i32 the screens' origin */
#define SCREEN_Y 0x006FDC9Cu
#define PICTURE_KIND 0x006FDCB0u   /* i32 window 4: 0 mission start, 1 plain, 2 victory */
#define MOUSE_LEFT_RELEASED 0x00E38E04u
#define RIGHT_RELEASED 0x00E38DF8u
#define MW_TYPE 0x00D3B124u

#define MESSAGE_TYPES_ADDR(st) (0x00E20970u + 0x50u * (uint32_t) S32(st, MW_TYPE)) /* the message type */

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, GFX, 0x0053E590u, id, x, y);
}

static void picture_origin(game_state *st)
{
    /* @bin 0x0041A850 the part of a 1024 x 768 picture the screen shows (centred) */
    S32(st, PIC_W) = S32(st, SCR_W);
    S32(st, PIC_X) = (0x400 - S32(st, SCR_W)) / 2;
    S32(st, PIC_H) = S32(st, SCR_H);
    S32(st, PIC_Y) = (0x300 - S32(st, SCR_H)) / 2;
}

int32_t mission_speech_play(game_state *st, uint32_t name)
{
    /* @bin 0x00539B20 the mission's speech "audio\voice\mission\" (0x005F90B8) + name, from the
     * game directory or the CD ("%c:\%s", drive 0x00E38E62), on stream 1 at the speech volume;
     * only with sound (0x00F55B88) and speech on (0x00E38E65). The path is kept in 0x00F558F4. */
    if (S32(st, 0x00F55B88u) == 0) {
        return 0;
    }
    if (U8(st, 0x00E38E65u) == 0) {
        return 0;
    }
    char path[300];
    uint32_t n = 0;
    for (uint32_t p = 0x005F90B8u; n < sizeof(path) - 1 && mem_rd8(st, p) != 0; p++) {
        path[n++] = (char) mem_rd8(st, p);
    }
    for (uint32_t p = name; n < sizeof(path) - 1 && mem_rd8(st, p) != 0; p++) {
        path[n++] = (char) mem_rd8(st, p);
    }
    path[n] = 0;
    for (uint32_t k = 0; k <= n; k++) {
        U8(st, 0x00F558F4u + k) = (uint8_t) path[k];
    }
    if (platform_file_read(st, path, 0, NULL, 0) < 0) { /* fopen(path, "rb") (0x005763BC) */
        /* @bin 0x00539BDE sprintf(0x00F558F4, "%c:\%s" (0x005763B4), drive, path) */
        uint32_t k = 0;
        U8(st, 0x00F558F4u + k++) = U8(st, 0x00E38E62u);
        U8(st, 0x00F558F4u + k++) = ':';
        U8(st, 0x00F558F4u + k++) = '\\';
        for (uint32_t i = 0; i <= n; i++) {
            U8(st, 0x00F558F4u + k++) = (uint8_t) path[i];
        }
        char cd[310];
        for (uint32_t i = 0; i < k && i < sizeof(cd); i++) {
            cd[i] = (char) U8(st, 0x00F558F4u + i);
        }
        cd[sizeof(cd) - 1] = 0;
        if (platform_file_read(st, cd, 0, NULL, 0) < 0) {
            return -1;
        }
    }
    /* fclose (0x00561994), then the speech volume and the stream (the sound object 0x00E92BF8) */
    GAME_CALLT(st, 0x00E92BF8u, 0x0055F639u, 1, S32(st, 0x00E38ECCu));
    GAME_CALLT(st, 0x00E92BF8u, 0x0055F32Fu, S32(st, 0x00E38ECCu));
    return GAME_CALLT(st, 0x00E92BF8u, 0x0055F449u, (int32_t) 0x00F558F4u, 1, 1);
}

void mission_picture_draw(game_state *st)
{
    /* @bin 0x0041A850 window 4: the mission start picture ([0x00F55C44] + 1) with the mission's
     * speech (tables 0x00579804 / 0x0057A164, 0x28 bytes per mission), the plain picture, or the
     * victory picture ([0x00F55C44]) with the victory speech (0x0057AAC4 / 0x0057B424) */
    picture_origin(st);
    int32_t kind = S32(st, PICTURE_KIND);
    if (kind == 0) {
        GAME_CALL(st, 0x0051F780u, 0);
        GAME_CALL(st, 0x0051F780u, 1);
        if (S32(st, CUSTOM) != 0) {
            image(st, S32(st, 0x00F55C44u) + 1, 0, 0);
            U8(st, REDRAW_ALL) = 1;
            return;
        }
        if (U8(st, MISSION) == 0x7F) {
            U8(st, REDRAW_ALL) = 1;
            return;
        }
        image(st, S32(st, 0x00F55C44u) + 1, 0, 0);
        S32(st, 0x00F55B8Cu) = 1;
        S32(st, 0x00F55B94u) = 1;
        uint32_t table = U8(st, 0x00E39088u) != 0 ? 0x0057A164u : 0x00579804u;
        mission_speech_play(st, table + 0x28u * (uint32_t) (int32_t) S8(st, MISSION));
        U8(st, REDRAW_ALL) = 1;
        return;
    }
    if (kind == 1) {
        image(st, S32(st, 0x00F55C44u), 0, 0);
        return;
    }
    if (kind != 2) {
        return;
    }
    GAME_CALL(st, 0x0051F780u, 0);
    GAME_CALL(st, 0x0051F780u, 1);
    S32(st, 0x00F55B8Cu) = 1;
    S32(st, 0x00F55B94u) = 1;
    if (S32(st, CUSTOM) != 0) {
        image(st, S32(st, 0x00F55C44u), 0, 0);
        U8(st, REDRAW_ALL) = 1;
        return;
    }
    if (U8(st, MISSION) != 0x7F) {
        image(st, S32(st, 0x00F55C44u), 0, 0);
        uint32_t table = U8(st, 0x00E39088u) == 0 ? 0x0057AAC4u : 0x0057B424u;
        mission_speech_play(st, table + 0x28u * (uint32_t) (int32_t) S8(st, MISSION));
    }
    U8(st, REDRAW_ALL) = 1;
}

void mission_picture_input(game_state *st)
{
    /* @bin 0x0041AA60 window 4 ends when the clock passes 0x006FDCA8 or on a right click: the
     * next window 0x006FDCA4 */
    int32_t clicked = U8(st, RIGHT_RELEASED) != 0;
    int32_t now = (int32_t) platform_clock_ms(st); /* timeGetTime */
    if (now > S32(st, 0x006FDCA8u) || clicked > 0) {
        U8(st, REDRAW_ALL) = 1;
        U8(st, WINDOW_ID) = U8(st, 0x006FDCA4u);
    }
}

static uint32_t step_rec(game_state *st)
{
    int32_t i = S8(st, STEP);
    if (i < 0 || i >= S32(st, CAMPAIGN_COUNT)) {
        return 0;
    }
    return CAMPAIGN + 0x40u * (uint32_t) i;
}

void mission_choice_draw(game_state *st)
{
    /* @bin 0x0041AAA0 window 0x34: the campaign step's map ([0x00F55C1C] + step +0x1C), its title
     * (group 0x90, +0x18) and the chosen mission's description (+0x1C + 0x0C * choice) */
    uint32_t r = step_rec(st);
    if (r == 0) {
        return;
    }
    S32(st, SCREEN_X) = S32(st, LAYOUT_DX);
    S32(st, SCREEN_Y) = S32(st, LAYOUT_DY);
    S32(st, PIC_X) = (0x400 - S32(st, SCR_W)) / 2;
    S32(st, PIC_Y) = (0x300 - S32(st, SCR_H)) / 2;
    S32(st, PIC_W) = S32(st, SCR_W);
    S32(st, PIC_H) = S32(st, SCR_H);
    image(st, S32(st, 0x00F55C14u), 0, 0);
    image(st, S32(st, r + 0x1C) + S32(st, 0x00F55C1Cu), S32(st, SCREEN_X), S32(st, SCREEN_Y));
    GAME_CALL(st, 0x004CCBB0u, 0x90, S32(st, r + 0x18), S32(st, SCREEN_X) + 0x14, S32(st, SCREEN_Y) + 0x19A,
              S32(st, FONT_TITLE), 0);
    uint8_t c = U8(st, CHOICE);
    if (c == 0) {
        GAME_CALL(st, 0x004CCFD0u, 0x90, 0, S32(st, SCREEN_X) + 0x14, S32(st, SCREEN_Y) + 0x1B8, 0x230,
                  S32(st, FONT_HEAD), 0);
        return;
    }
    GAME_CALL(st, 0x004CCFD0u, 0x90, (int32_t) S16(st, r + 0x1C + 0x0Cu * c), S32(st, SCREEN_X) + 0x14,
              S32(st, SCREEN_Y) + 0x1B8, 0x230, S32(st, FONT_HEAD), 0);
}

void mission_choice_marks(game_state *st)
{
    /* @bin 0x00419140 window 0x34 per frame: the missions' marks ([0x00F55C54] + 1 hovered, + 2
     * chosen, + 0) at the step's points (+0x20 / +0x24 + 0x0C * i, +0x38 of them) */
    uint32_t r = step_rec(st);
    if (r == 0 || S32(st, r + 0x38) <= 0) {
        return;
    }
    int32_t i = 0;
    uint32_t p = r + 0x24;
    do {
        int32_t y = S32(st, p) + S32(st, LAYOUT_DY);
        int32_t x = S32(st, p - 4) + S32(st, LAYOUT_DX);
        int32_t mark;
        if (GAME_CALL(st, 0x004CF620u, x, y, 0x2C, 0x2C) != 0) {
            mark = 1;
        } else {
            mark = ((int32_t) U8(st, CHOICE) - 1 - i) != 0 ? 0 : 2;
        }
        image(st, S32(st, 0x00F55C54u) + mark, x, y);
        i++;
        p += 0x0C;
    } while (i < S32(st, r + 0x38));
}

void mission_choice_click(game_state *st)
{
    /* @bin 0x00419090 window 0x34, left click: the mission under the mouse is chosen, a fanfare
     * (wavs\Fanfare_Nu1 / _Nu5 by the mission's parity, 0x0047D350) */
    if (U8(st, MOUSE_LEFT_RELEASED) == 0) {
        return;
    }
    uint32_t r = step_rec(st); /* 0 out of range: the original reads address 0x38 */
    if ((int32_t) mem_rd32(st, r + 0x38) <= 0) {
        return;
    }
    uint32_t p = r + 0x24;
    for (int32_t i = 0;; i++, p += 0x0C) {
        if (GAME_CALL(st, 0x004CF620u, (int32_t) mem_rd32(st, p - 4) + S32(st, LAYOUT_DX),
                      (int32_t) mem_rd32(st, p) + S32(st, LAYOUT_DY), 0x2C, 0x2C) != 0) {
            U8(st, CHOICE) = (uint8_t) (i + 1);
            uint32_t sound = (i & 1) != 0 ? 0x0057C018u : 0x0057C030u;
            GAME_CALL(st, 0x0047D350u, (int32_t) sound, 1, 0);
            U8(st, REDRAW_ALL) = 1;
            return;
        }
        if (i + 1 >= (int32_t) mem_rd32(st, r + 0x38)) {
            return;
        }
    }
}

int32_t campaign_step_has_choice(game_state *st, uint32_t self)
{
    /* @bin 0x0041B8F0 (thiscall on the campaign table) the previous step offers more than one
     * mission (+0x38 > 1) */
    int8_t i = S8(st, STEP_PREV);
    if (i < 0) {
        return 0;
    }
    if (i >= S32(st, self + 0x1900)) {
        return 0;
    }
    uint32_t r = self + 0x40u * (uint32_t) (int32_t) i;
    if (r == 0) {
        return 0;
    }
    return S32(st, r + 0x38) > 1;
}

int32_t mission_goal_hint(game_state *st)
{
    /* @bin 0x004E2530 the first missions' hint (group 0x3E) by what the player has built; 1 with
     * the text cursor 0x00E20768 on it (0x004CC110), 0 for none */
    if (S32(st, CUSTOM) != 0) {
        return 0;
    }
    int8_t m = S8(st, MISSION);
    int32_t index = 0;
    if (m == 0) {
        if (U8(st, 0x007027B8u) != 0 && S32(st, 0x007059D8u) <= 0) {
            index = 0x11;
        } else if (U8(st, 0x007027BBu) != 0 && S32(st, 0x00705880u) <= 0) {
            index = 0x12;
        } else if (U8(st, 0x007027BAu) != 0) {
            index = 0x14;
        } else if (U8(st, 0x007027B9u) != 0) {
            index = 0x13;
        } else {
            index = 0x15;
        }
    } else if (m == 1) {
        if (U8(st, 0x007027BDu) != 0) {
            index = 0x16;
        } else if (U8(st, 0x007027BCu) != 0) {
            index = 0x17;
        } else {
            index = 0x18;
        }
    } else if (m == 2) {
        if (U8(st, 0x007027BEu) != 0 && S32(st, 0x00705A74u) <= 0 && S32(st, 0x007057F4u) <= 0) {
            index = 0x19;
        } else if (U8(st, 0x007027C1u) != 0) {
            index = 0x1A;
        } else if (U8(st, 0x007027BFu) != 0) {
            index = 0x1B;
        } else {
            index = 0x1C;
        }
    } else if (m == 3) {
        if (U8(st, 0x007027C2u) != 0) {
            return 0;
        }
        index = 0x21;
    } else if (m == 4) {
        /* @bin 0x004E269F the city's +0x42D4 against the goal 0x00784C2C */
        if (S32(st, 0x00EA5B44u + 0x49D8u * U32(st, 0x00EAAC28u)) == S32(st, 0x00784C2Cu)) {
            return 0;
        }
        if (U8(st, 0x007027C5u) != 0) {
            index = 0x20;
        } else if (U8(st, 0x007027C4u) != 0) {
            index = 0x1D;
        } else if (U8(st, 0x007027C3u) != 0) {
            index = 0x1E;
        } else {
            index = 0x1F;
        }
    } else {
        return 0;
    }
    GAME_CALL(st, 0x004CC110u, 0x3E, index);
    return 1;
}

static void goal_box(game_state *st, int32_t x, int32_t y, int32_t w)
{
    /* @bin 0x00537000 (package frontend) a goal's box */
    GAME_CALL(st, 0x00537000u, 6, x, y, w, 0, 1);
}

void mission_briefing_draw(game_state *st)
{
    /* @bin 0x0041A180 windows 0x35 / 0x3B: on the first draw the mission starts (0x005405B0 on the
     * map name 0x00E37078); then the briefing: the message of 0x00418140 (title, subtitle,
     * difficulty, the goals in boxes, the hint, the body in a scrolling box) */
    if (S32(st, 0x007027A8u) == 0) {
        S32(st, 0x007027A8u) = 1;
        GAME_CALL(st, 0x005405B0u, (int32_t) 0x00E37078u, 0);
        if (U8(st, WINDOW_ID) == 1) {
            return;
        }
    }
    S32(st, MW_TYPE) = GAME_CALL0(st, 0x00418140u);
    S32(st, SCREEN_Y) = S32(st, LAYOUT_DY) + 0x10;
    S32(st, SCREEN_X) = S32(st, LAYOUT_DX) + 0x10;
    S32(st, PIC_W) = S32(st, SCR_W);
    S32(st, PIC_H) = S32(st, SCR_H);
    S32(st, PIC_X) = (0x400 - S32(st, SCR_W)) / 2;
    S32(st, 0x00D3B14Cu) = 0x23;
    S32(st, PIC_Y) = (0x300 - S32(st, SCR_H)) / 2;
    if (U8(st, REDRAW_ALL) == 1) {
        image(st, S32(st, 0x00F55C44u) + 1, 0, 0);
    }
    int32_t sx = S32(st, SCREEN_X), sy = S32(st, SCREEN_Y);
    GAME_CALL(st, 0x00536820u, sx, sy, 0x26, 0x1C);
    uint32_t t = MESSAGE_TYPES_ADDR(st);
    GAME_CALL(st, 0x004CC770u, (int32_t) (U32(st, t + 0x8C) + U32(st, 0x00E36C74u)), S32(st, SCREEN_X) + 0x10,
              S32(st, SCREEN_Y) + 0x10, S32(st, FONT_TITLE), 0);
    t = MESSAGE_TYPES_ADDR(st);
    GAME_CALL(st, 0x004CC770u, (int32_t) (U32(st, t + 0x90) + U32(st, 0x00E36C74u)), S32(st, SCREEN_X) + 0x10,
              S32(st, SCREEN_Y) + 0x2E, S32(st, FONT_HEAD), 0);
    GAME_CALL(st, 0x004CCBB0u, 0x3E, 7, S32(st, SCREEN_X) + 0x1DC, S32(st, SCREEN_Y) + 0x1A1, S32(st, FONT_HEAD), 0);
    if (U8(st, WINDOW_ID) == 0x35 && campaign_step_has_choice(st, CAMPAIGN) != 0) {
        GAME_CALL(st, 0x004CCBB0u, 0x0D, 4, S32(st, SCREEN_X) + 0x32, S32(st, SCREEN_Y) + 0x1A3, S32(st, FONT_HEAD), 0);
    }
    S32(st, TEXT_X) = 0;
    GAME_CALL(st, 0x004CCBB0u, 0x2C, 0xD8, S32(st, SCREEN_X) + 0x96, S32(st, SCREEN_Y) + 0x1A1, S32(st, FONT_HEAD), 0);
    GAME_CALL(st, 0x004CCBB0u, 0x99, S32(st, 0x00E39080u) + 1, S32(st, TEXT_X) + S32(st, SCREEN_X) + 0x96,
              S32(st, SCREEN_Y) + 0x1A1, S32(st, FONT_HEAD), 0);
    GAME_CALL(st, 0x00536A30u, S32(st, SCREEN_X) + 0x10, S32(st, SCREEN_Y) + 0x40, S32(st, 0x00D3B14Cu) + 1, 6);
    GAME_CALL(st, 0x004CCBB0u, 0x3E, 10, S32(st, SCREEN_X) + 0x20, S32(st, SCREEN_Y) + 0x48, S32(st, FONT_TEXT), 0);
    /* @bin 0x0041A3E9 the goals, one per box at the points of 0x005797D4 (x, y pairs) */
    int32_t slot = 0;
    S32(st, 0x00F53F28u) = 100;
    if (S32(st, 0x00784C58u) != 0) {
        int32_t gx = S32(st, 0x005797D4u), gy = S32(st, 0x005797D8u);
        slot = 1;
        goal_box(st, S32(st, SCREEN_X) + gx, S32(st, SCREEN_Y) + gy, 0x0F);
        S32(st, TEXT_X) = 0;
        GAME_CALL(st, 0x004CCBB0u, 0x3E, 0x0B, S32(st, SCREEN_X) + gx + 8, S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
        GAME_CALL(st, 0x004CCE00u, S32(st, 0x00784C5Cu), 0x40, 0x0057C048, S32(st, TEXT_X) + S32(st, SCREEN_X) + gx + 8,
                  S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
    }
    if (S32(st, 0x00784C34u) != 0) {
        int32_t gy = S32(st, 0x005797D8u + 8u * (uint32_t) slot), gx = S32(st, 0x005797D4u + 8u * (uint32_t) slot);
        slot++;
        goal_box(st, S32(st, SCREEN_X) + gx, S32(st, SCREEN_Y) + gy, 0x0F);
        S32(st, TEXT_X) = 0;
        GAME_CALL(st, 0x004CCE00u, S32(st, 0x00784C34u), 0x40, 0x0057C048, S32(st, SCREEN_X) + gx + 8,
                  S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
        GAME_CALL(st, 0x004CCBB0u, 0x1D, S32(st, 0x00784C38u) + 0x14, S32(st, TEXT_X) + S32(st, SCREEN_X) + gx + 8,
                  S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
    }
    uint32_t pt = 0x005797D8u + 8u * (uint32_t) slot;
    int32_t index = 0x0C;
    for (uint32_t v = 0x00784C24u; v < 0x00784C34u; v += 4, index++) {
        if (U8(st, 0x00784C30u + (uint32_t) index) == 0) {
            continue;
        }
        int32_t gy = S32(st, pt), gx = S32(st, pt - 4);
        pt += 8;
        goal_box(st, S32(st, SCREEN_X) + gx, S32(st, SCREEN_Y) + gy, 0x0F);
        S32(st, TEXT_X) = 0;
        GAME_CALL(st, 0x004CCBB0u, 0x3E, index, S32(st, SCREEN_X) + gx + 8, S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
        GAME_CALL(st, 0x004CCE00u, S32(st, v), 0x40, 0x0057C048, S32(st, TEXT_X) + S32(st, SCREEN_X) + gx + 8,
                  S32(st, SCREEN_Y) + gy + 3, S32(st, FONT_LIT), 0);
    }
    if ((uint8_t) mission_goal_hint(st) != 0) {
        int32_t gy = S32(st, 0x005797F8u), gx = S32(st, 0x005797F4u);
        goal_box(st, S32(st, SCREEN_X) + gx, S32(st, SCREEN_Y) + gy, S32(st, 0x00D3B14Cu) - 1);
        GAME_CALL(st, 0x004CC770u, S32(st, 0x00E20768u), S32(st, SCREEN_X) + gx + 8, S32(st, SCREEN_Y) + gy + 3,
                  S32(st, FONT_LIT), 0);
    }
    GAME_CALL(st, 0x00536A30u, S32(st, SCREEN_X) + 0x10, S32(st, SCREEN_Y) + 0xA8, S32(st, 0x00D3B14Cu) - 2, 0x0F);
    S32(st, 0x00D3B168u) = S32(st, FONT_TEXT);
    S32(st, 0x00D3B164u) = S32(st, FONT_LIT);
    S32(st, 0x00D3B154u) = S32(st, SCREEN_X) - 6;
    S32(st, 0x00D3B150u) = S32(st, SCREEN_Y) + 0xAA;
    int32_t lines = S32(st, 0x00D3B138u);
    int32_t visible;
    if (lines == 0) {
        S32(st, 0x00D3B148u) = 0x0F;
        S32(st, 0x00D3B13Cu) = 0x0E;
        U8(st, 0x005C7E10u) = 1;
        U8(st, 0x005C7E38u) = 1;
        t = MESSAGE_TYPES_ADDR(st);
        GAME_CALL(st, 0x004C8070u, (int32_t) (U32(st, t + 0x94) + U32(st, 0x00E36C74u)), S32(st, SCREEN_X) + 0x20,
                  S32(st, SCREEN_Y) + 0xB8, (S32(st, 0x00D3B14Cu) << 4) - 0x10, 1, 0);
        lines = S32(st, 0x00D3B138u);
        visible = S32(st, 0x00D3B13Cu);
        if (lines > visible) {
            S32(st, 0x00D3B134u) = lines - visible;
        } else {
            S32(st, 0x00D3B134u) = 0;
            U8(st, 0x005C7E10u) = 0;
            U8(st, 0x005C7E38u) = 0;
        }
    } else {
        visible = S32(st, 0x00D3B13Cu);
    }
    if (lines > visible) {
        GAME_CALL(st, 0x00536A30u, S32(st, SCREEN_X) + 0x22D, S32(st, SCREEN_Y) + 0xC0, 2, 0x0C);
    }
    S32(st, CLIP_LEFT) = S32(st, SCREEN_X) + 0x13;
    S32(st, CLIP_RIGHT) = (S32(st, 0x00D3B14Cu) << 4) + S32(st, SCREEN_X) + 0x13;
    S32(st, CLIP_TOP) = S32(st, SCREEN_Y) + 0xAB;
    S32(st, CLIP_BOTTOM) = S32(st, SCREEN_Y) + 0x195;
    t = MESSAGE_TYPES_ADDR(st);
    GAME_CALL(st, 0x004C8070u, (int32_t) (U32(st, t + 0x94) + U32(st, 0x00E36C74u)), S32(st, SCREEN_X) + 0x20,
              S32(st, SCREEN_Y) + 0xB4, (S32(st, 0x00D3B14Cu) << 4) - 0x10, 0, 0);
    S32(st, CLIP_LEFT) = 0;
    S32(st, CLIP_RIGHT) = S32(st, SCR_W);
    S32(st, CLIP_TOP) = 0;
    S32(st, CLIP_BOTTOM) = S32(st, SCR_H);
}

void mission_briefing_frame(game_state *st)
{
    /* @bin 0x00418F70 windows 0x35 / 0x3B per frame: the body's scroll buttons and thumb when it
     * scrolls */
    U8(st, REDRAW_SIDEBAR) = 1;
    if (S32(st, 0x00D3B134u) == 0) {
        return;
    }
    GAME_CALL(st, 0x00425730u, S32(st, SCREEN_X) + 0x22A, S32(st, SCREEN_Y) + 0xA8, 0x005C7DF8, 1);
    GAME_CALL(st, 0x00425730u, S32(st, SCREEN_X) + 0x22A, S32(st, SCREEN_Y) + 0x17E, 0x005C7E20, 1);
    GAME_CALL0(st, 0x004C72B0u);
}

static void fullscreen(game_state *st, uint32_t slot)
{
    GAME_CALL0(st, 0x004CF1C0u);
    image(st, S32(st, slot), S32(st, LAYOUT_DX), S32(st, LAYOUT_DY));
}

void picture_37_draw(game_state *st) { fullscreen(st, 0x00F55BE4u); } /* @bin 0x00417E80 */
void picture_38_draw(game_state *st) { fullscreen(st, 0x00F55BDCu); } /* @bin 0x00417EB0 */
void picture_39_draw(game_state *st) { fullscreen(st, 0x00F55BD4u); } /* @bin 0x00417EE0 */

void picture_3a_draw(game_state *st)
{
    /* @bin 0x00417F10 window 0x3A: the picture [0x00F55BFC] with a 0x1C x 10 panel and two texts
     * (group 0x3E, 8 and 9) */
    S32(st, 0x00F53DB8u) = S32(st, LAYOUT_DX) + 0x60;
    S32(st, 0x00F53DB4u) = S32(st, LAYOUT_DY) + 0xA0;
    S32(st, PIC_X) = (0x400 - S32(st, SCR_W)) / 2;
    S32(st, PIC_Y) = (0x300 - S32(st, SCR_H)) / 2;
    S32(st, PIC_W) = S32(st, SCR_W);
    S32(st, PIC_H) = S32(st, SCR_H);
    image(st, S32(st, 0x00F55BFCu), 0, 0);
    GAME_CALL(st, 0x00536820u, S32(st, 0x00F53DB8u), S32(st, 0x00F53DB4u), 0x1C, 10);
    GAME_CALL(st, 0x004CCB30u, 0x3E, 8, S32(st, 0x00F53DB8u) + 0x20, S32(st, 0x00F53DB4u) + 0x18, 0x180,
              S32(st, FONT_TITLE), 0);
    GAME_CALL(st, 0x004CCFD0u, 0x3E, 9, S32(st, 0x00F53DB8u) + 0x20, S32(st, 0x00F53DB4u) + 0x50, 0x180,
              S32(st, FONT_HEAD), 0);
}

/* ---- port table ---- */

#define RUN0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN0(run_picture, mission_picture_draw)
RUN0(run_picture_input, mission_picture_input)
RUN0(run_choice, mission_choice_draw)
RUN0(run_marks, mission_choice_marks)
RUN0(run_click, mission_choice_click)
RUN0(run_briefing, mission_briefing_draw)
RUN0(run_briefing_frame, mission_briefing_frame)
RUN0(run_37, picture_37_draw)
RUN0(run_38, picture_38_draw)
RUN0(run_39, picture_39_draw)
RUN0(run_3a, picture_3a_draw)
static int32_t run_speech(game_state *st, const int32_t *a) { return mission_speech_play(st, (uint32_t) a[0]); }
static int32_t run_hint(game_state *st, const int32_t *a) { (void) a; return mission_goal_hint(st); }
static int32_t run_has_choice(game_state *st, const int32_t *a)
{
    (void) a;
    return campaign_step_has_choice(st, CAMPAIGN);
}

const ported_function mission_ports[] = {
    {0x0041A850u, "mission_picture_draw", 0, 0, 0, run_picture, 0},
    {0x0041AA60u, "mission_picture_input", 0, 0, 0, run_picture_input, 0},
    {0x00539B20u, "mission_speech_play", 1, 32, 0, run_speech, 0},
    {0x0041AAA0u, "mission_choice_draw", 0, 0, 0, run_choice, 0},
    {0x00419140u, "mission_choice_marks", 0, 0, 0, run_marks, 0},
    {0x00419090u, "mission_choice_click", 0, 0, 0, run_click, 0},
    {0x0041B8F0u, "campaign_step_has_choice", 0, 8, CAMPAIGN, run_has_choice, 0},
    {0x004E2530u, "mission_goal_hint", 0, 8, 0, run_hint, 0},
    {0x0041A180u, "mission_briefing_draw", 0, 0, 0, run_briefing, 0},
    {0x00418F70u, "mission_briefing_frame", 0, 0, 0, run_briefing_frame, 0},
    {0x00417E80u, "picture_37_draw", 0, 0, 0, run_37, 0},
    {0x00417EB0u, "picture_38_draw", 0, 0, 0, run_38, 0},
    {0x00417EE0u, "picture_39_draw", 0, 0, 0, run_39, 0},
    {0x00417F10u, "picture_3a_draw", 0, 0, 0, run_3a, 0},
};
const int mission_ports_count = sizeof(mission_ports) / sizeof(mission_ports[0]);
