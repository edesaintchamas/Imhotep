/* Imhotep - window 9: info panels (phrase) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The phrase a figure says in the info window: chosen by 0x0053C100 when the open 0x004F7E50 lists
 * the figure (figure +0x7D the phrase, +0x131 the mood picture, +0x7C a rotating counter, global
 * 0x00EADCDC), from the figure's type, action and the city's state; walkers of 20 types fall back
 * on the city's mood (0x0053C060 over the 8 categories of 0x0053D3A0). The speech file of the
 * selected row is played by 0x0053D0B0 (callback 0x00406700, the terrain panel 0x004FA530):
 * the figure's sound for 9 types, then "audio\voice\walker\" + the phrase's file (0x0053BFE0,
 * 0x00539A70). */
#include "info.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

/* the original compares a byte with 500, 600 and 800 (always false, see below): kept as it is */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#define PHRASE 0x00EADCDCu      /* i32 the phrase chosen (figure +0x7D) */
#define SPEECH_KIND 0x00EADCE0u /* i32 the speech table's entry for the figure's type, - 0xC9 */
#define MOOD 0x00F55BA4u        /* u8[8] the city's mood per category (0x0053D3A0) */
#define EVENTS 0x00785D70u      /* the event list (0x0044BB90 thiscall) */
#define DIST_X 0x00D3B4FCu      /* i32 |dx| scratch of the distance */
#define DIST_Y 0x00D3B4F8u      /* i32 |dy| */
#define SPEECH_PATH 0x00F55A20u /* char[] the speech file's path (0x00539A70) */

#define FIG(f, off) (FIGURE_ADDR(f) + (off))
#define CITY(st, addr) S32(st, CURRENT_CITY(st) + ((addr) - CITY_BASE))

/* @bin 0x0053D3A0 the city's mood: 8 categories (0: 0xEA1888, 1: 0xEA4388, 2: a pending event,
 * 3: 10 pairs 0xEA43A0 / 0xEA439C, 4: the 5 bytes 0xEA5C0A, 5: 0xEA5B48 or an event with bits 4 and 2,
 * 6: 0xEA4468 / 0xEA1894, 7: 0xEA5BF4), each 8 / 6 / 4 (*bad = 0) or 1 (or 0, or the previous
 * category's value: the register is not reset) into buf[8]; returns their sum, corrected for
 * categories 3 and 6 */
static int32_t phrase_mood(game_state *st, uint32_t buf, uint8_t *bad)
{
    U32(st, buf) = 0;
    U32(st, buf + 4) = 0;
    uint8_t emergency = 0, pending = 0;
    int32_t sum = 0;
    int32_t v = 0; /* ebx: kept from one category to the next */
    /* @bin 0x0053D3CB the first event with bit 4 of +0x28: bit 2 too marks it */
    uint32_t e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, 0, 0);
    while (e != 0) {
        if ((P32(st, e + 0x28) >> 4) & 1) {
            if ((P32(st, e + 0x28) >> 2) & 1) {
                emergency = 1;
            }
            break;
        }
        e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, (int32_t) e, 0);
    }
    /* @bin 0x0053D406 any pending event (0x0044D700) */
    e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, 0, 0);
    while (e != 0) {
        if ((uint8_t) GAME_CALLT0(st, e, 0x0044D700u) != 0) {
            pending = 1;
            break;
        }
        e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, (int32_t) e, 0);
    }
    for (int32_t i = 0; i < 8; i++) {
        int32_t a, b;
        switch (i) {
        case 0: /* @bin 0x0053D452 */
            a = CITY(st, 0x00EA1888u);
            if (a < 0x14) {
                v = 8, *bad = 0;
            } else if (a < 0x1E) {
                v = 6, *bad = 0;
            } else if (a < 0x28) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        case 1: /* @bin 0x0053D48C */
            a = CITY(st, 0x00EA4388u);
            if (a < 1) {
                v = 6, *bad = 0;
            } else if (a < 2) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        case 2: /* @bin 0x0053D4C5 a pending event, no fort (0x00BCB9B4) and not 0xEA5ED8: 8; else unchanged */
            if (!pending) {
                v = 1;
            } else if (S32(st, 0x00BCB9B4u) == 0
                       && U8(st, CURRENT_CITY(st) + (0x00EA5ED8u - CITY_BASE)) == 0) {
                v = 8, *bad = 0;
            }
            break;
        case 3: /* @bin 0x0053D505 100 - 100 * sum(+0x43A0) / sum(+0x439C), 10 entries of 0x14 */
            a = 0, b = 0;
            for (int32_t k = 0; k < 10; k++) {
                a = (int32_t) ((uint32_t) a + (uint32_t) CITY(st, 0x00EA43A0u + 0x14u * (uint32_t) k));
            }
            for (int32_t k = 0; k < 10; k++) {
                b = (int32_t) ((uint32_t) b + (uint32_t) CITY(st, 0x00EA439Cu + 0x14u * (uint32_t) k));
            }
            if (b == 0) {
                v = 1;
                break;
            }
            a = 100 - (int32_t) ((uint32_t) a * 100u) / b;
            if (a > 0x14) {
                v = 8, *bad = 0;
            } else if (a > 0xA) {
                v = 6, *bad = 0;
            } else if (a > 5) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        case 4: { /* @bin 0x0053D58E the lowest of 5 signed bytes +0x439A (from 100) */
            int32_t m = 100;
            for (int32_t k = 0; k < 5; k++) {
                int32_t x = S8(st, CURRENT_CITY(st) + (0x00EA5C0Au - CITY_BASE) + (uint32_t) k);
                if (x < m) {
                    m = x;
                }
            }
            if (m <= 0x14) {
                v = 6, *bad = 0;
            } else if (m <= 0x1E) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        }
        case 5: /* @bin 0x0053D5DE */
            if (emergency) {
                v = 0;
                break;
            }
            a = CITY(st, 0x00EA5B48u);
            if (a <= 0) {
                v = 8, *bad = 0;
            } else if (a < 0x14) {
                v = 6, *bad = 0;
            } else if (a < 0x28) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        case 6: /* @bin 0x0053D638 100 * 0xEA4468 / 0xEA1894 */
            b = CITY(st, 0x00EA1894u);
            if (b == 0) {
                v = 1;
                break;
            }
            a = (int32_t) ((uint32_t) CITY(st, 0x00EA4468u) * 100u) / b;
            if (a > 0x11) {
                v = 8, *bad = 0;
            } else if (a > 0xA) {
                v = 6, *bad = 0;
            } else if (a > 5) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        default: /* @bin 0x0053D68C */
            a = CITY(st, 0x00EA5BF4u);
            if (a <= 0) {
                v = 6, *bad = 0;
            } else if (a < 0x14) {
                v = 4, *bad = 0;
            } else {
                v = 1;
            }
            break;
        }
        sum = (int32_t) ((uint32_t) sum + (uint32_t) v);
        U8(st, buf + (uint32_t) i) = (uint8_t) v;
    }
    /* @bin 0x0053D6DA category 3 at 1 with category 6 set: 3 cleared; 3 above 1 with 6 at 1: 6 cleared */
    uint8_t c3 = U8(st, buf + 3);
    if (c3 == 1) {
        if (U8(st, buf + 6) >= 1) {
            U8(st, buf + 3) = 0;
            return sum - 1;
        }
    } else if (c3 > 1 && U8(st, buf + 6) == 1) {
        U8(st, buf + 6) = 0;
        sum--;
    }
    return sum;
}

/* @bin 0x0053C060 the mood phrase: 9 when no category is bad; else the category where the shares
 * (100 * value / sum) reach 0x00D3B374 & (their total), 8 when that category's value is 1 */
static int32_t phrase_mood_pick(game_state *st)
{
    uint8_t ok = 1;
    int32_t total = phrase_mood(st, MOOD, &ok);
    int32_t all = 0;
    for (int32_t i = 0; i < 8; i++) {
        all += (int32_t) U8(st, MOOD + (uint32_t) i) * 100 / total;
    }
    int32_t pick = S32(st, 0x00D3B374u) & all;
    if (ok) {
        return 9;
    }
    int32_t acc = 0, i;
    for (i = 0; i < 8; i++) {
        acc += (int32_t) U8(st, MOOD + (uint32_t) i) * 100 / total;
        if (pick <= acc) {
            break;
        }
    }
    if (i == 8 && pick > acc) {
        i = 7;
    }
    return U8(st, MOOD + (uint32_t) i) == 1 ? 8 : i;
}

/* @bin 0x0053C57E the larger of |x - dest x| (+0x24, +0x2C) and |y - dest y| (+0x26, +0x2E), written
 * to 0x00D3B4FC / 0x00D3B4F8 */
static int32_t phrase_distance(game_state *st, int32_t f)
{
    int32_t a = S16(st, FIG(f, 0x2C)), c = S16(st, FIG(f, 0x24));
    int32_t d = S16(st, FIG(f, 0x2E)), e = S16(st, FIG(f, 0x26));
    int32_t dx = c > a ? c - a : c < a ? a - c : 0;
    S32(st, DIST_X) = dx;
    int32_t dy = e > d ? e - d : e < d ? d - e : 0;
    S32(st, DIST_Y) = dy;
    return dx < dy ? dy : dx;
}

/* @bin 0x0053C100 the phrase of figure f when the info window lists it */
static void figure_phrase(game_state *st, int32_t f)
{
    /* @bin 0x0053C12D the mood pictures of the mood categories 0..7 */
    static const uint8_t mood_picture[8] = {8, 0xD, 2, 1, 0xB, 5, 1, 0xA};
    if (f <= 0) {
        return;
    }
    uint32_t r = FIGURE_ADDR(f);
    U8(st, r + 0x7D) = 0;
    S32(st, PHRASE) = -1;
    /* @bin 0x0053C178 any pending event (0x0044D700) */
    int pending = 0;
    for (uint32_t e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, 0, 0); e != 0;
         e = (uint32_t) GAME_CALLT(st, EVENTS, 0x0044BB90u, (int32_t) e, 0)) {
        if ((uint8_t) GAME_CALLT0(st, e, 0x0044D700u) != 0) {
            pending = 1;
            break;
        }
    }
    U8(st, r + 0x131) = 0;
    uint8_t type = U8(st, r + 0x0A);
    uint8_t act = U8(st, r + 0x38);
    uint8_t w66 = U8(st, r + 0x66);
    int32_t v; /* eax */
    switch (type) {
    case 0x01: /* @bin 0x0053C1CD the counter 0..2 */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 3) {
            U8(st, r + 0x7C) = 0;
        }
        v = U8(st, r + 0x7C);
        S32(st, PHRASE) = v;
        goto check;
    case 0x02: /* @bin 0x0053C1FA by 0xEA5DB0 */
        switch (CITY(st, 0x00EA5DB0u)) {
        case 1:
            U8(st, r + 0x131) = 1;
            v = 1;
            goto set;
        case 2:
            U8(st, r + 0x131) = 1;
            v = 0;
            goto set;
        case 3:
            U8(st, r + 0x131) = 0xC;
            v = 2;
            goto set;
        default:
            U8(st, r + 0x131) = 1;
            v = 3;
            goto set;
        }
    case 0x03: /* @bin 0x0053C275 counter 0..1, picture 0xD */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 2) {
            U8(st, r + 0x7C) = 0;
        }
        goto counter_d;
    case 0x17: /* @bin 0x0053C289 counter 0..3, picture 0xD */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 4) {
            U8(st, r + 0x7C) = 0;
        }
    counter_d:
        v = U8(st, r + 0x7C);
        U8(st, r + 0x131) = 0xD;
        S32(st, PHRASE) = v;
        goto check;
    case 0x18: /* @bin 0x0053C2BD counter 0..1, picture 0xE */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 2) {
            U8(st, r + 0x7C) = 0;
        }
        v = U8(st, r + 0x7C);
        U8(st, r + 0x131) = 0xE;
        S32(st, PHRASE) = v;
        goto check;
    case 0x62: /* @bin 0x0053C2F1 */
        U8(st, r + 0x131) = 8;
        v = 0;
        goto set;
    case 0x49: /* @bin 0x0053C30D */
        v = act == 0xC;
        goto set;
    case 0x19: /* @bin 0x0053C32E by action (table 0x0053CFF8) */
        switch (act) {
        case 8:
        case 0xC:
            v = 2;
            goto set;
        case 9:
        case 0xA:
            v = 0;
            goto set;
        case 0xB:
        case 0xD:
            v = 1;
            goto set;
        default:
            goto end;
        }
    case 0x55: { /* @bin 0x0053C35E actions 0xF..0x17, 0x1A, 0x1E (table 0x0053D010) */
        int busy = (act >= 0xF && act <= 0x17) || act == 0x1A || act == 0x1E;
        if ((uint8_t) GAME_CALLT0(st, 0x00D05720u, 0x004BF9D0u) != 0) {
            v = 0xB;
            goto set;
        }
        if (!busy) {
            goto end;
        }
    }
        /* fall through */
    case 0x60: /* @bin 0x0053C3B1 */
        U8(st, r + 0x131) = 0xE;
        v = 0xC;
        goto set;
    case 0x05: /* @bin 0x0053C3D0 */
        if (CITY(st, 0x00EA446Cu) > 5) {
            v = 0xA;
            goto set;
        }
        if (CITY(st, 0x00EA4474u) > 5) {
            v = 0xB;
            goto set;
        }
        goto end;
    case 0x4B:
    case 0x5A: /* @bin 0x0053C424 */
        v = act == 0xB;
        U8(st, r + 0x131) = 6;
        S32(st, PHRASE) = v;
        goto check;
    case 0x50: /* @bin 0x0053C443 */
        v = act != 0xA;
        U8(st, r + 0x131) = 0xE;
        S32(st, PHRASE) = v;
        goto check;
    case 0x4F:
    case 0x51:
    case 0x6C: /* @bin 0x0053C462 */
        v = act != 9;
        U8(st, r + 0x131) = 0xE;
        S32(st, PHRASE) = v;
        goto check;
    case 0x27: /* @bin 0x0053C481 */
        if (act == 8) {
            U8(st, r + 0x131) = 6;
            v = 0xA;
            goto set;
        }
        if (act == 9) {
            U8(st, r + 0x131) = 6;
            v = 0xB;
            goto set;
        }
        goto end;
    case 0x1A: /* @bin 0x0053C4D1 */
        v = act != 2;
        U8(st, r + 0x131) = 6;
        S32(st, PHRASE) = v;
        goto check;
    case 0x42: /* @bin 0x0053C4F0 counter 0..2, picture 6 */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 3) {
            U8(st, r + 0x7C) = 0;
        }
        v = U8(st, r + 0x7C);
        U8(st, r + 0x131) = 6;
        S32(st, PHRASE) = v;
        goto check;
    case 0x04: /* @bin 0x0053C524 */
        if (act == 8) {
            if (w66 == 2) {
                U8(st, r + 0x131) = 1;
                v = 0;
                goto set;
            }
            if (w66 == 1) {
                v = 1;
                goto set;
            }
            goto end;
        }
        if (act == 9 || act == 0xA || act == 0xB) {
            goto far;
        }
        goto end;
    case 0x09: /* @bin 0x0053C5DC */
        if (act == 8) {
            if (w66 == 2) {
                v = 1;
                goto set;
            }
            goto end;
        }
        if (act == 9) {
            goto far;
        }
        goto end;
    case 0x26: /* @bin 0x0053C685 */
        if (act != 0xB && act != 0xC) {
            goto end;
        }
        if (phrase_distance(st, f) < 0x19) {
            v = S32(st, PHRASE);
            U8(st, r + 0x131) = 6;
            goto check;
        }
        v = 2;
        U8(st, r + 0x131) = 6;
        S32(st, PHRASE) = v;
        goto check;
    case 0x14: { /* @bin 0x0053C707 */
        uint32_t k = 0x58u * U8(st, r + 0x7F);
        if (act == 0xD) {
            /* @bin 0x0053C71A the record +0x7F of 0x0077AF00 (0x58 bytes): +4 and +0 both 0: 2 */
            v = (S32(st, 0x0077AF04u + k) == 0 && S32(st, 0x0077AF00u + k) == 0) ? 2 : 4;
            U8(st, r + 0x131) = 6;
            S32(st, PHRASE) = v;
            goto check;
        }
        if (act == 0xA) {
            v = GAME_CALL(st, 0x00495B60u, f);
            if (v == 1) {
                S32(st, PHRASE) = v;
                U8(st, r + 0x131) = 6;
                goto check;
            }
            U8(st, r + 0x131) = 6;
            v = v != 2 ? 2 : 0;
            S32(st, PHRASE) = v;
            goto check;
        }
        v = 3;
        U8(st, r + 0x131) = 6;
        S32(st, PHRASE) = v;
        goto check;
    }
    case 0x13:
    case 0x15: { /* @bin 0x0053C7B8 counter 1..2, phrase counter + 1 */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) > 2) {
            U8(st, r + 0x7C) = 1;
        }
        uint32_t k = 0x58u * U8(st, r + 0x7F);
        v = U8(st, r + 0x7C) + 1;
        S32(st, PHRASE) = v;
        if (act == 0xB) {
            if (S32(st, 0x0077AF04u + k) == 0 && S32(st, 0x0077AF00u + k) == 0) {
                v = 0;
                U8(st, r + 0x131) = 6;
                S32(st, PHRASE) = v;
                goto check;
            }
            U8(st, r + 0x131) = 6;
            goto check;
        }
        if (act == 0xA) {
            int32_t player = S32(st, CURRENT_PLAYER_ADDR);
            if (GAME_CALL(st, 0x0045CF50u, f, player, S16(st, r + 0x5E), U8(st, r + 0x6E)) != 0) {
                v = 4;
                U8(st, r + 0x131) = 6;
                S32(st, PHRASE) = v;
                goto check;
            }
            player = S32(st, CURRENT_PLAYER_ADDR);
            if (GAME_CALL(st, 0x0045D180u, f, player, S16(st, r + 0x5E), U8(st, r + 0x6E)) != 0) {
                v = 3;
                U8(st, r + 0x131) = 6;
                S32(st, PHRASE) = v;
                goto check;
            }
            v = S32(st, PHRASE);
        }
        U8(st, r + 0x131) = 6;
        goto check;
    }
    case 0x5B: /* @bin 0x0053C8B8 */
        U8(st, r + 0x131) = 0xB;
        v = 0xA;
        goto set;
    case 0x1B: /* @bin 0x0053C8D7 by the level of the building +0x5A (0x004C5120, table 0x0053D028) */
        switch ((uint32_t) GAME_CALL(st, 0x004C5120u, S16(st, r + 0x5A))) {
        case 0:
            if (S32(st, 0x00D38374u) < 0x3C && U8(st, 0x00D386DCu) != 0) {
                goto phrase_b;
            }
            goto end;
        case 1:
            if (CITY(st, 0x00EA5B48u) >= 0x14) {
                goto end;
            }
            U8(st, r + 0x131) = 5;
            v = 0xB;
            goto set;
        case 2:
            if (CITY(st, 0x00EA4474u) <= 0xA) {
                goto end;
            }
            U8(st, r + 0x131) = 1;
            v = 0xB;
            goto set;
        case 3:
            if (!pending) {
                goto end;
            }
            U8(st, r + 0x131) = 2;
            v = 0xB;
            goto set;
        case 4:
            if ((uint8_t) GAME_CALL(st, 0x004C52D0u, S16(st, r + 0x5A), 2) != 0) {
                U8(st, r + 0x131) = 8;
                v = 0xD;
                goto set;
            }
            if (CITY(st, 0x00EA1880u) < 0x3C) {
                goto phrase_b;
            }
            if (CITY(st, 0x00EA5BF4u) >= 0x3C) {
                goto end;
            }
            U8(st, r + 0x131) = 0xA;
            v = 0xC;
            goto set;
        default:
            goto end;
        }
    case 0x20: /* @bin 0x0053CA43 */
        if (CITY(st, 0x00EA1888u) >= 0x28) {
            goto end;
        }
        U8(st, r + 0x131) = 8;
        v = 0xA;
        goto set;
    case 0x22: /* @bin 0x0053CA58 */
        if (CITY(st, 0x00EA1888u) >= 0x1E) {
            goto end;
        }
        U8(st, r + 0x131) = 8;
        v = 0xA;
        goto set;
    case 0x21: /* @bin 0x0053CA90 a byte compared with 800: always 1 */
        U8(st, r + 0x131) = 8;
        v = ((int32_t) w66 > 0x320) + 1;
        S32(st, PHRASE) = v;
        goto check;
    case 0x0A: /* @bin 0x0053CAB5 counter 0..12, the phrase when action 0xD */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 0xD) {
            U8(st, r + 0x7C) = 0;
        }
        if (act != 0xD) {
            goto end;
        }
        v = U8(st, r + 0x7C);
        S32(st, PHRASE) = v;
        goto check;
    case 0x08: /* @bin 0x0053CAEF a byte compared with 600: always 10 */
        v = ((int32_t) w66 >= 0x258) + 0xA;
        goto set;
    case 0x58: { /* @bin 0x0053CB18 counter 0..3 */
        U8(st, r + 0x7C) = (uint8_t) (U8(st, r + 0x7C) + 1);
        if (U8(st, r + 0x7C) >= 4) {
            U8(st, r + 0x7C) = 0;
        }
        if (act == 5) {
            /* @bin 0x0053CB3C the figures +0x82 and +0x84: types 0x17 / 0x16 */
            uint8_t t1 = U8(st, FIGURE_BASE + (uint32_t) (FIGURE_SIZE * (int32_t) S16(st, r + 0x82)) + 0x0A);
            if (t1 == 0x17) {
                goto phrase_d;
            }
            uint8_t t2 = U8(st, FIGURE_BASE + (uint32_t) (FIGURE_SIZE * (int32_t) S16(st, r + 0x84)) + 0x0A);
            if (t2 == 0x17 || t1 == 0x16 || t2 == 0x16) {
                goto phrase_d;
            }
            v = U8(st, r + 0x7C) + 0xE;
            S32(st, PHRASE) = v;
            goto check;
        }
        /* @bin 0x0053CB9F a byte compared with 500 (never), then with 100 */
        if ((int32_t) w66 >= 0x1F4) {
            v = 0xC;
            goto phrase_set_d;
        }
        if (w66 < 0x64) {
            v = 0xA;
            goto set;
        }
        goto phrase_b;
    }
    case 0x07: { /* @bin 0x0053CBCC 100 * (+0x2C24 + +0x2C20) / the four +0x2C20..+0x2C2C */
        int32_t a = (int32_t) ((uint32_t) CITY(st, 0x00EA4494u) + (uint32_t) CITY(st, 0x00EA4490u));
        int32_t d = (int32_t) ((uint32_t) CITY(st, 0x00EA449Cu) + (uint32_t) CITY(st, 0x00EA4498u) + (uint32_t) a);
        /* the original divides by d unchecked (a division fault when 0) */
        if ((int32_t) ((uint32_t) a * 100u) / d < 0x32) {
            v = 0xA;
            U8(st, r + 0x131) = 0xC;
            S32(st, PHRASE) = v;
            goto check;
        }
        if (w66 >= 7) {
            v = 0xB;
            U8(st, r + 0x131) = 0xC;
            S32(st, PHRASE) = v;
            goto check;
        }
        if (w66 >= 1) {
            v = 0xC;
            S32(st, PHRASE) = v;
            U8(st, r + 0x131) = 0xC;
            goto check;
        }
        v = S32(st, PHRASE);
        U8(st, r + 0x131) = 0xC;
        goto check;
    }
    case 0x59: /* @bin 0x0053CC66 */
        if (CITY(st, 0x00EA6090u) != 0) {
            v = 0xB;
            goto set;
        }
        if ((int32_t) w66 >= 0x1F4) {
            v = 0xE;
            goto phrase_set_d;
        }
        if (w66 >= 0x64) {
            goto phrase_d;
        }
        v = 0xC;
        goto set;
    case 0x4E: { /* @bin 0x0053CCD9 */
        int32_t x = CITY(st, 0x00EA5EA4u);
        if (x != 0) {
            if (act == 0xA) {
                v = 0;
                goto set3;
            }
            if (act == 0xC) {
                v = 1;
                goto set3;
            }
        }
        if (CITY(st, 0x00EA6210u) != 0 && act != 0xA && act != 0xC) {
            v = 2;
            goto set3;
        }
        v = pending ? 3 : 4;
        goto set3;
    }
    case 0x4D: /* @bin 0x0053CD75 */
        if (CITY(st, 0x00EA5EA4u) != 0 && act == 0xA) {
            v = 0;
            goto set3;
        }
        if (CITY(st, 0x00EA6210u) != 0) {
            v = 1;
            goto set3;
        }
        v = pending ? 2 : 3;
        goto set3;
    case 0x2A: { /* @bin 0x0053CDE0 */
        int32_t x = CITY(st, 0x00EA5EA4u);
        if (x == 0) {
            U8(st, r + 0x131) = 2;
            v = 0;
            goto set;
        }
        if (pending) {
            U8(st, r + 0x131) = 2;
            v = 1;
            goto set;
        }
        if (x <= 0xA) {
            U8(st, r + 0x131) = 2;
            v = 2;
            goto set;
        }
        if (x <= 0x1E) {
            U8(st, r + 0x131) = 2;
            v = 3;
            goto set;
        }
        goto end;
    }
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x28:
    case 0x57:
    case 0x69:
        goto end;
    default: /* @bin 0x0053CEDF types without a phrase (table 0x0053CF8C) */
        U8(st, r + 0x7D) = 0xFF;
        S32(st, PHRASE) = -1;
        return;
    }

far: /* @bin 0x0053C664 more than 24 tiles from the destination: 2 */
    if (phrase_distance(st, f) < 0x19) {
        goto end;
    }
    v = 2;
    goto set;

set3:
    U8(st, r + 0x131) = 3;
    S32(st, PHRASE) = v;
    goto check;

phrase_b: /* @bin 0x0053C911 */
    v = 0xB;
    goto phrase_set_d;
phrase_d: /* @bin 0x0053CCB7 */
    v = 0xD;
phrase_set_d: /* @bin 0x0053C916 */
    S32(st, PHRASE) = v;
picture_d: /* @bin 0x0053C91B */
    U8(st, r + 0x131) = 0xD;
    U8(st, r + 0x7D) = (uint8_t) v;
    return;

set: /* the phrase v, returned at once */
    U8(st, r + 0x7D) = (uint8_t) v;
    S32(st, PHRASE) = v;
    return;

end: /* @bin 0x0053CE7A */
    v = S32(st, PHRASE);
check: /* @bin 0x0053CE7F no phrase yet: the city's mood for 20 types (table 0x0053D044) */
    if (v == -1) {
        switch (type) {
        case 0x05: case 0x07: case 0x0A: case 0x0F: case 0x10: case 0x11: case 0x12:
        case 0x1B: case 0x1D: case 0x1E: case 0x1F: case 0x20: case 0x22: case 0x23:
        case 0x27: case 0x28: case 0x57: case 0x58: case 0x59: case 0x69:
            v = phrase_mood_pick(st);
            S32(st, PHRASE) = v;
            if (v == 9 || v == 8) {
                goto picture_d;
            }
            U8(st, r + 0x131) = mood_picture[v];
            U8(st, r + 0x7D) = (uint8_t) v;
            return;
        default:
            break;
        }
    }
    U8(st, r + 0x7D) = (uint8_t) v;
}

/* @bin 0x00539A70 play the speech file "audio\voice\walker\" (0x005F90A4) + name on stream channel 1,
 * when sound is on (0x00F55B88, 0x00E38E65): the path is built in 0x00F55A20 */
static int32_t play_speech(game_state *st, const char *name)
{
    if (S32(st, 0x00F55B88u) == 0 || U8(st, 0x00E38E65u) == 0) {
        return 0;
    }
    uint32_t n = 0;
    do {
        U8(st, SPEECH_PATH + n) = U8(st, 0x005F90A4u + n);
    } while (U8(st, 0x005F90A4u + n++) != 0);
    uint32_t end = 0;
    while (U8(st, SPEECH_PATH + end) != 0) {
        end++;
    }
    n = 0;
    do {
        U8(st, SPEECH_PATH + end + n) = (uint8_t) name[n];
    } while (name[n++] != 0);
    /* @bin 0x00539AD4 the stream volume and the speech channel's default volume (0x00E38ECC) */
    GAME_CALLT(st, 0x00E92BF8u, 0x0055F639u, 1, S32(st, 0x00E38ECCu));
    GAME_CALLT(st, 0x00E92BF8u, 0x0055F32Fu, S32(st, 0x00E38ECCu));
    return GAME_CALLT(st, 0x00E92BF8u, 0x0055F449u, (int32_t) SPEECH_PATH, 1, 1);
}

/* @bin 0x0053BFE0 the speech file of entry `kind` (> 0), phrase `phrase` (>= 0): kind 0x6C from the
 * table 0x0060110C, else 0x005FB16C + 24 * (20 * (kind - 1) + phrase) (24-byte names) */
static void speech_file(game_state *st, int32_t kind, int32_t phrase)
{
    if (kind <= 0 || phrase < 0) {
        return;
    }
    char name[256] = {0}; /* the original's 32-byte stack buffer (the names are shorter) */
    uint32_t src = kind == 0x6C ? 0x0060110Cu + 24u * (uint32_t) phrase
                                : 0x005FB16Cu + 24u * (uint32_t) (phrase + 20 * kind - 20);
    for (uint32_t n = 0; n < sizeof(name) - 1; n++) {
        name[n] = (char) U8(st, src + n);
        if (name[n] == 0) {
            break;
        }
    }
    play_speech(st, name);
}

/* @bin 0x0053D0B0 the selected row's figure (0x00EADD24[0x00EADCE4]) speaks: the sound of 9 types
 * (0x0053B4A0), then the speech file of its type's entry (table 0x006012EC: -2 by the climate,
 * -3 by the level of its building +0x5A, -4 the entry of its type +0x0B, -1 none) and phrase +0x7D */
static void figure_speech(game_state *st)
{
    int32_t f = S32(st, INFO_FIGURES + 4u * (uint32_t) S32(st, INFO_FIGURE_ROW));
    if (f <= 0) {
        return;
    }
    uint32_t r = FIGURE_ADDR(f);
    /* @bin 0x0053D0D2 types 0x53 0x67 0x68 0x66 0x45 0x46 0x54 0x52 0x44: sound 0..8, channel 0 */
    static const uint8_t sounding[9] = {0x53, 0x67, 0x68, 0x66, 0x45, 0x46, 0x54, 0x52, 0x44};
    for (int32_t k = 0; k < 9; k++) {
        if (U8(st, r + 0x0A) == sounding[k]) {
            GAME_CALL(st, 0x0053B4A0u, f, k, 0);
            break;
        }
    }
    int32_t v = S32(st, 0x006012ECu + 4u * U8(st, r + 0x0A));
    S32(st, SPEECH_KIND) = v;
    if (v == -2) {
        /* @bin 0x0053D15B the climate (0x005E4C20 + 0x30 * (0x00784CB0 & 0xFF)): 0x44 / 0x45 / 0x46 */
        switch (S32(st, 0x005E4C20u + 0x30u * (U32(st, 0x00784CB0u) & 0xFFu))) {
        case 0x44:
            v = 0xD2;
            break;
        case 0x45:
            v = 0xD1;
            break;
        case 0x46:
            v = 0xD0;
            break;
        default:
            break;
        }
    } else {
        int level = 0;
        if (v == -3) {
            level = 1;
        } else if (v == -4) {
            v = S32(st, 0x006012ECu + 4u * U8(st, r + 0x0B));
            S32(st, SPEECH_KIND) = v;
            level = v == -3;
        }
        if (level) {
            /* @bin 0x0053D1A5 by the level of the building (0x004C5120): 0xE6 + level (tables 0x0053D254,
             * 0x0053D268) */
            uint32_t l = (uint32_t) GAME_CALL(st, 0x004C5120u, S16(st, r + 0x5A));
            v = l <= 4 ? 0xE6 + (int32_t) l : S32(st, SPEECH_KIND);
        }
        if (v == -1) {
            return;
        }
    }
    v = (int32_t) ((uint32_t) v - 0xC9u);
    S32(st, SPEECH_KIND) = v;
    if (v < 0) {
        return;
    }
    int32_t phrase = S8(st, r + 0x7D);
    S32(st, PHRASE) = phrase;
    speech_file(st, v, phrase);
}

static int32_t run_phrase(game_state *st, const int32_t *a) { figure_phrase(st, a[0]); return 0; }
static int32_t run_mood_pick(game_state *st, const int32_t *a) { (void) a; return phrase_mood_pick(st); }
static int32_t run_speech(game_state *st, const int32_t *a) { (void) a; figure_speech(st); return 0; }
static int32_t run_speech_file(game_state *st, const int32_t *a) { speech_file(st, a[0], a[1]); return 0; }
static int32_t run_play_speech(game_state *st, const int32_t *a)
{
    char name[256];
    uint32_t n = 0;
    for (; n < sizeof(name) - 1 && U8(st, (uint32_t) a[0] + n) != 0; n++) {
        name[n] = (char) U8(st, (uint32_t) a[0] + n);
    }
    name[n] = 0;
    return play_speech(st, name);
}

const ported_function info_phrase_ports[] = {
    {0x0053C100u, "info_figure_phrase", 1, 0, 0, run_phrase, 0},
    {0x0053C060u, "info_phrase_mood_pick", 0, 32, 0, run_mood_pick, 0},
    /* its second argument is a flag on the caller's stack: reached only from 0x0053C060 */
    {0x0053D3A0u, "info_phrase_mood", 2, 32, 0, NULL, 0},
    {0x0053D0B0u, "info_figure_speech", 0, 0, 0, run_speech, 0},
    {0x0053BFE0u, "info_speech_file", 2, 0, 0, run_speech_file, 0},
    {0x00539A70u, "info_play_speech", 1, 32, 0, run_play_speech, 0},
};
const int info_phrase_ports_count = sizeof(info_phrase_ports) / sizeof(info_phrase_ports[0]);
