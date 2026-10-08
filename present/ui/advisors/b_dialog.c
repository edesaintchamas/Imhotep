/* Imhotep - the advisors: window 0x4F, the explanation dialog over the hall.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "b.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define CITY_STRIDE 0x49D8u
#define EVENTS 0x00785D70u /* `this` of the event list walk 0x0044BAF0 */

static uint32_t city(game_state *st, uint32_t field)
{
    return field + (uint32_t) (S32(st, 0x00EAAC28u) * (int32_t) CITY_STRIDE);
}

static int32_t cs32(game_state *st, uint32_t field)
{
    return S32(st, city(st, field));
}

static int32_t next_event(game_state *st, int32_t prev)
{
    return GAME_CALLT(st, EVENTS, 0x0044BAF0u, prev);
}

static int32_t event_running(game_state *st, int32_t ev)
{
    return GAME_CALLT0(st, (uint32_t) ev, 0x0044D4C0u) & 0xFF;
}

void advisor_explain_draw(game_state *st)
{
    /* @bin 0x00538050 window 0x4F: the text chosen by the topic 0x00D3B198 (0-11, jump table
     * 0x005387FC) and the city's state (string of group 61, -1: nothing drawn), and the advisor
     * page to offer (city +0x5FD8 := it; its button 0x005F78F0[page] when that advisor is
     * available 0x006FDC88[page]) */
    int32_t text = -1;
    int32_t page = 0;
    uint8_t flood = 0, plague = 0, running = 0; /* @bin 0x00538059 bl, [esp+0x12], [esp+0x13] */
    switch (U32(st, ADV_PARAM1)) { /* @bin 0x00538061 unsigned (ja) */
    case 0: {
        int32_t k = cs32(st, 0x00EA5DB0u);
        switch (k) { /* @bin 0x0053882C table on k - 1 */
        case 1:
        case 5:
            page = 7;
            break;
        case 2:
        case 4:
            page = 1;
            break;
        case 3:
            page = 0x0C;
            break;
        default:
            break;
        }
        int32_t v = cs32(st, 0x00EA1880u);
        if (v < 0x32) {
            text = k + 0x25;
        } else if (v < 0x64) {
            text = 0x2A;
            page = 0;
        }
        break;
    }
    case 1: {
        text = cs32(st, 0x00EA5C80u);
        if (U32(st, city(st, 0x00EA5C80u)) <= 3) { /* @bin 0x005380E5 unsigned (ja), table 0x00538840 */
            page = text < 2 ? 1 : 7;
        }
        int32_t mode = cs32(st, 0x00EA3FB0u);
        if (mode == 1) {
            if (cs32(st, 0x00EA5EB4u) + cs32(st, 0x00EA5EA4u) > 3) {
                text = -1;
            } else if (cs32(st, 0x00EA3FACu) >= 5) {
                text = 0x44;
                page = 0;
            } else if (cs32(st, 0x00EA3F98u) == 0 && cs32(st, 0x00EA18A8u) > 0) {
                if (cs32(st, 0x00EA3F9Cu) >= 0x50) {
                    text = 0x44;
                    page = 0;
                } else {
                    text += 0x3D;
                }
            } else {
                text = 0x3C;
                page = 7;
            }
        } else if (mode == 0) {
            if (cs32(st, 0x00EA3F98u) == 0 && cs32(st, 0x00EA18A8u) > 0) {
                text += 0x46;
            } else {
                page = 7;
                text = 0x45;
            }
        } else {
            text = 0x43;
            page = 0;
        }
        break;
    }
    case 2: {
        page = 1;
        int32_t a = cs32(st, 0x00EA446Cu);
        if (a > 0x11) {
            text = 0x55;
        } else if (a > 0x0A) {
            text = 0x56;
        } else if (a > 5) {
            text = 0x57;
        } else if (a > 0) {
            text = 0x58;
        } else {
            int32_t b = cs32(st, 0x00EA4474u);
            if (b > 0x14) {
                text = 0x59;
            } else if (b > 0x0A) {
                text = 0x5A;
            } else if (b > 5) {
                text = 0x5B;
            } else if (b > 0) {
                text = 0x5C;
            }
        }
        if (a <= 5 && cs32(st, 0x00EA4474u) <= 5) {
            text = 0x5D;
        }
        break;
    }
    case 3: {
        int32_t n = cs32(st, 0x00EA4388u);
        page = 7;
        if (n <= 0) {
            text = 0x63;
        } else {
            /* @bin 0x00538270 [+0x41DC] * 100 / [+0x5E1C] (idiv) */
            int32_t r = (int32_t) ((uint32_t) cs32(st, 0x00EA41DCu) * 100u) / cs32(st, 0x00EA5E1Cu);
            text = r < 0x32 ? 0x64 : (n >= 1) + 0x65;
        }
        break;
    }
    case 4:
        page = 7;
        if (cs32(st, 0x00EA1894u) == 0) {
            text = 0xD4;
        } else {
            int32_t f = GAME_CALL0(st, 0x00447530u);
            /* @bin 0x005382CB food * 100 / population (idiv) */
            int32_t r = (int32_t) ((uint32_t) f * 100u) / cs32(st, 0x00EA1894u);
            if (r > 0x96) {
                text = 0xD9;
            } else if (r > 0x6E) {
                text = 0xD8;
            } else if (r > 0x5F) {
                text = 0xD7;
            } else if (r > 0x55) {
                text = 0xD6;
            } else if (r > 0x46) {
                text = 0xD5;
            } else {
                text = 0xD4;
            }
        }
        break;
    case 5:
        page = 8;
        text = cs32(st, 0x00EA1888u) / 10 + 0x72; /* @bin 0x00538358 toward zero */
        break;
    case 6: {
        /* @bin 0x00538381 the five signed bytes at +0x5C00: count below 40, minimum (from 100),
         * count above 60 */
        int32_t low = 0, high = 0, min = 0x64;
        page = 0x0B;
        uint32_t p = city(st, 0x00EA5C00u);
        for (uint32_t i = 0; i < 5; i++) {
            int8_t v = S8(st, p + i);
            if (v < 0x28) {
                low++;
            }
            if (v < min) {
                min = v;
            }
        }
        for (uint32_t i = 0; i < 5; i++) {
            if (S8(st, p + i) > 0x3C) {
                high++;
            }
        }
        if (low == 0) {
            if (high == 0) {
                text = 0x92;
            } else if (high == 1) {
                text = 0x93;
            } else if (high == 2) {
                text = 0x94;
            }
        } else if (low > 3) {
            text = (min < 0x14) + 0x8A;
        } else if (low == 3) {
            text = (min < 0x14) + 0x8C;
        } else if (low == 2) {
            text = (min < 0x14) + 0x8E;
        } else if (low == 1) {
            text = (min < 0x14) + 0x90;
        }
        break;
    }
    case 7: {
        page = 0x0C;
        int32_t d = cs32(st, 0x00EA44B8u) + cs32(st, 0x00EA44B4u);
        if (d != 0 && (cs32(st, 0x00EA44B0u) + cs32(st, 0x00EA44ACu)) / d < 1) {
            text = 0x9B;
        } else {
            int32_t a = cs32(st, 0x00EA187Cu), b = cs32(st, 0x00EA452Cu);
            text = a > b ? 0x9C : (a < b) + 0x9D;
        }
        break;
    }
    case 8: {
        int32_t v = cs32(st, 0x00EA5FD0u);
        page = 0x0C;
        if (v >= 0x0A) {
            text = 0xA5;
        } else if (v == 0) {
            text = 0xA9;
        } else if (v < 2) {
            text = 0xA8;
        } else {
            text = (v < 5) + 0xA6;
        }
        break;
    }
    case 9: {
        /* @bin 0x0053853C the events of type 0x25 (+4): running (0x0044D4C0), kind +0x30 5, 6 or 7 */
        for (int32_t ev = next_event(st, 0); ev != 0; ev = next_event(st, ev)) {
            if (P16(st, (uint32_t) ev + 4) != 0x25) {
                continue;
            }
            if (event_running(st, ev) != 0) {
                running = 1;
            }
            int16_t kind = P16(st, (uint32_t) ev + 0x30);
            if (kind == 5) {
                plague = 1;
            }
            if (kind == 6 || kind == 7) {
                flood = 1;
            }
        }
        page = 2;
        if (cs32(st, 0x00EA5EB4u) != 0) {
            text = 0xB5;
        } else if (cs32(st, 0x00EA5EA4u) != 0) {
            text = 0xB4;
        } else if (GAME_CALL0(st, 0x00446610u) != 0) {
            text = 0xB3;
        } else if (flood) {
            text = 0xB7;
        } else if (plague) {
            text = 0xB7;
        } else if (running) {
            text = 0xB6;
        } else {
            text = (cs32(st, 0x00EA5EE8u) >= 0) + 0xB1;
        }
        break;
    }
    case 10: {
        /* @bin 0x00538657 running events (+4 nonzero) counted up to 5 */
        int32_t n = 0;
        for (int32_t ev = next_event(st, 0); ev != 0; ev = next_event(st, ev)) {
            if (event_running(st, ev) != 0) {
                if (n >= 5) {
                    break;
                }
                if (P16(st, (uint32_t) ev + 4) != 0) {
                    n++;
                }
            }
        }
        page = 4;
        if (n == 0) {
            text = 0xBF;
        } else if (n <= 2) {
            text = 0xBE;
        } else {
            text = (n <= 4) + 0xBC;
        }
        break;
    }
    case 11: {
        page = 7;
        int32_t v = GAME_CALLT0(st, 0x00D38360u, 0x004BD590u);
        text = (v + 0x13) / 0x14 + 0xC6; /* @bin 0x005386D2 toward zero */
        break;
    }
    default:
        break;
    }
    S32(st, city(st, 0x00EA5FD8u)) = page;
    if (text < 0) {
        return;
    }
    rect_grey(st, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0x3C, 0x240, 0x110, 0);
    panel_draw(st, S32(st, ADV_PANEL_X) + 0x10, S32(st, ADV_PANEL_Y) + 0x90, 0x26, 0x0B);
    layout_group_lines(st, 0x3D, text, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0xA0, 0x264,
                       S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
    layout_group_centred(st, 0x0D, 1, S32(st, ADV_PANEL_X) + 0x10, S32(st, ADV_PANEL_Y) + 0x128, 0x260,
                         S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
    if (U8(st, ADV_AVAILABLE + (uint32_t) page) != 0) {
        buttons_draw(st, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0x110,
                     U32(st, 0x005F78F0u + (uint32_t) page * 4u), 1);
    }
}
