/* Imhotep - the boot: settings, text files, default names (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The settings are globals at 0x00E38E58 .. 0x00E39089: WinMain's defaults (0x004147F0), the
 * Pharaoh.ini keys RAM, CheckCD and CPU (0x00414A90) and the saved options, Pharaoh.inf read whole
 * over 0x00E38E58 (0x004CEB10). Files come from the platform (platform_file_read). */
#include <string.h>

#include "boot/boot_internal.h"
#include "city/text_lookup.h"
#include "newgame/flow2.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"

int32_t boot_file_into(game_state *st, const char *path, uint32_t dst, uint32_t cap)
{
    int32_t size = platform_file_read(st, path, 0, NULL, 0);
    if (size < 0) {
        return -1;
    }
    uint32_t n = (uint32_t) size < cap ? (uint32_t) size : cap;
    uint8_t *p = n ? state_ptr(st, dst, n) : NULL;
    if (n && !p) {
        state_unported(st, dst); /* outside the state and the mapped memory */
        return -1;
    }
    if (n && platform_file_read(st, path, 0, p, n) < 0) {
        return -1;
    }
    return size;
}

static uint8_t rd8(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 1);
    return p ? *p : 0;
}

int32_t boot_atoi(game_state *st, uint32_t p)
{
    /* @bin 0x00561C0B CRT atol (atoi 0x00561C96 calls it): blanks skipped (ctype bit 8 of the table
     * [0x00604C58]), a sign, digits (bit 4), wrapping; the multi-byte path ([0x00604E64] > 1,
     * 0x00565F66) is the CRT's for other locales (the "C" locale's value is 1) */
    if (S32(st, G_CRT_MB_CUR_MAX) > 1) {
        state_unported(st, 0x00565F66u);
        return 0;
    }
    uint32_t ctype = U32(st, G_CRT_CTYPE);
    while (rd8(st, ctype + 2u * rd8(st, p)) & 8) {
        p++;
    }
    uint32_t c = rd8(st, p++);
    uint32_t sign = c;
    if (c == 0x2D || c == 0x2B) {
        c = rd8(st, p++);
    }
    uint32_t v = 0;
    while (rd8(st, ctype + 2u * c) & 4) {
        v = c + v * 10u - 0x30;
        c = rd8(st, p++);
    }
    return (int32_t) (sign == 0x2D ? 0u - v : v);
}

/* the number after the '=' that follows a key found at offset `at` of the buffer (0x004CBE30 finds the
 * '='); 1 and its atoi when offset + index > 0 */
static int ini_number(game_state *st, uint32_t buf, int32_t at, int32_t *out)
{
    int32_t eq = GAME_CALL(st, FN_TEXT_FIND_CHAR, (int32_t) (buf + (uint32_t) at), 0x3D, 20000);
    if (at + eq <= 0) {
        return 0;
    }
    *out = boot_atoi(st, buf + (uint32_t) (at + eq));
    return 1;
}

int32_t boot_ini_read(game_state *st, const boot_host *host)
{
    /* @bin 0x00414A97 a 20,000-byte block (operator new), cleared */
    uint32_t buf = boot_alloc(st, host, 20000, 0x00414A9Cu);
    if (!buf) {
        return 0;
    }
    uint8_t *p = state_ptr(st, buf, 20000);
    if (!p) {
        state_unported(st, buf);
        return 0;
    }
    memset(p, 0, 20000);
    /* @bin 0x00414AB8 fopen("Pharaoh.ini" 0x005764D8, "rb"), fread of up to 20,000 bytes; missing:
     * logged (0x00579290), freed, 0 */
    if (boot_file_into(st, "Pharaoh.ini", buf, 20000) < 0) {
        platform_log(st, G_STR_ERR_SIERRA_INI, 0, 0);
        boot_release(st, host, buf);
        return 0;
    }
    int32_t v;
    /* @bin 0x00414AF9 "RAM" (0x0057928C): 0x00E38ED8 (the test is on offset + index only, so a
     * missing key still reads the number after the buffer's first '=') */
    int32_t at = GAME_CALL(st, FN_TEXT_FIND_STRING, (int32_t) buf, (int32_t) G_STR_RAM, 20000);
    if (ini_number(st, buf, at, &v)) {
        S32(st, G_RAM) = v;
    }
    /* @bin 0x00414B34 "CheckCD" (0x00579284): the byte 0x00E39089, when the key is found (offset > 0) */
    at = GAME_CALL(st, FN_TEXT_FIND_STRING, (int32_t) buf, (int32_t) G_STR_CHECK_CD, 20000);
    if (at > 0 && ini_number(st, buf, at, &v)) {
        U8(st, G_CHECK_CD) = (uint8_t) v;
    }
    /* @bin 0x00414B76 "CPU" (0x00579280): 0x00E38EDC, when the key is found */
    at = GAME_CALL(st, FN_TEXT_FIND_STRING, (int32_t) buf, (int32_t) G_STR_CPU, 20000);
    if (at > 0 && ini_number(st, buf, at, &v)) {
        S32(st, G_CPU) = v;
    }
    boot_release(st, host, buf); /* @bin 0x00414BB9 operator delete */
    return 1;
}

int32_t boot_file_read(game_state *st, const char *path, uint32_t dst)
{
    /* @bin 0x004CEB10 fopen(path, "rb"); fread(dst, 1, _filelength(...)); 1, or 0 when it does not
     * open. The file's bytes land at dst whatever their number (Pharaoh.inf: the saved options over
     * 0x00E38E58, 0 bytes in the GOG install) */
    return boot_file_into(st, path, dst, STATE_END - dst) < 0 ? 0 : 1;
}

int32_t boot_settings(game_state *st, const boot_host *host, int32_t screen_width)
{
    /* @bin 0x00414802 .. 0x004148B1 the defaults */
    S32(st, G_OPTIONS) = 0x10;
    S32(st, G_FULLSCREEN) = 1;
    U8(st, G_PAUSED) = 0;
    U8(st, G_UE38E61) = 0;
    S32(st, G_UE38ED4) = 2;
    S32(st, G_RAM) = 0xF;
    S32(st, G_CPU) = 0x42;
    S32(st, G_UE38EE0) = 0;
    U8(st, G_DRIVE) = 0x44;
    U8(st, G_SOUND_EFFECTS_ON) = 1;
    U8(st, G_MUSIC_ON) = 1;
    U8(st, G_SPEECH_ON) = 1;
    U8(st, G_CITY_SOUNDS_ON) = 1;
    U8(st, G_NOTICES_ON) = 1;
    S32(st, G_MOUSE_HELP) = 2;
    S32(st, G_GAME_SPEED) = 0x5A;
    S32(st, G_SCROLL_SPEED) = 0x46;
    U8(st, G_CHECK_CD) = 1;
    S32(st, G_DIFFICULTY) = 2;
    S32(st, G_UE39084) = 1;
    S32(st, G_EFFECTS_VOLUME) = 100;
    S32(st, G_MUSIC_VOLUME) = 0x50;
    S32(st, G_SPEECH_VOLUME) = 100;
    S32(st, G_CITY_VOLUME) = 100;
    S32(st, G_RESOLUTION) = 1;
    boot_ini_read(st, host); /* @bin 0x004148B7 */
    /* @bin 0x004148BC RAM at most 0x17 (signed): 100, 100, resolution 1 */
    if (S32(st, G_RAM) <= 0x17) {
        S32(st, G_GAME_SPEED) = 100;
        S32(st, G_SCROLL_SPEED) = 100;
        S32(st, G_RESOLUTION) = 1;
    }
    /* @bin 0x004148D7 GetSystemMetrics(SM_CXSCREEN): below 800 resolution 1, below 1024 resolution 2,
     * else the default stays (signed compares) */
    if (screen_width < 800) {
        S32(st, G_RESOLUTION) = 1;
    } else if (screen_width < 0x400) {
        S32(st, G_RESOLUTION) = 2;
    }
    S32(st, G_RESOLUTION_SAVED) = S32(st, G_RESOLUTION);
    campaign_period_first(st, 0); /* @bin 0x00414907 */
    S32(st, G_LAST_ADVISOR) = 1;     /* @bin 0x00414916 */
    /* @bin 0x0041491C the saved options: Pharaoh.inf (0x00579274) over 0x00E38E58 */
    boot_file_read(st, "Pharaoh.inf", G_OPTIONS);
    U8(st, G_CHECK_CD) = 1; /* @bin 0x00414924 */
    boot_ini_read(st, host); /* @bin 0x0041492A */
    S32(st, G_UE36C6C) = 1;
    U8(st, G_PAUSED) = 0;
    /* @bin 0x0041493A the words 0x005C9022 / 0x005C9062 by 0x00E38EAC (2: 4, 1: 3) */
    if (S32(st, G_MOUSE_HELP) == 2) {
        S16(st, G_MENU_TEXT_TIPS) = 4;
        S16(st, G_MENU_TEXT_TIPS2) = 4;
    } else if (S32(st, G_MOUSE_HELP) == 1) {
        S16(st, G_MENU_TEXT_TIPS) = 3;
        S16(st, G_MENU_TEXT_TIPS2) = 3;
    }
    if (U8(st, G_NOTICES_ON) == 1) {
        S16(st, G_MENU_TEXT_WARNINGS) = 6; /* @bin 0x00414977 */
    }
    /* @bin 0x00414980 .. 0x004149C0 */
    S16(st, G_MENU_TEXT_EEC) = (int16_t) ((S32(st, G_UE38EEC) != 0) + 9);
    S16(st, G_MENU_TEXT_EC3) = (int16_t) ((U8(st, G_UE38EC3) != 0) + 4);
    S16(st, G_MENU_TEXT_LANGUAGE) = (int16_t) ((U8(st, G_EMPIRE_LANGUAGE) != 0) + 7);
    return 1;
}

int32_t boot_text_load(game_state *st, const boot_host *host, const char *text, const char *mm)
{
    /* @bin 0x004CEC40 _open(text, O_BINARY): the 0x1C-byte header into 0x00E36B78, the 8,000-byte group
     * index into 0x00E34C38, the rest (from byte 0x1F5C) into a new block whose address is 0x00E36C70
     * (an older block released first); then the messages: 0x13898 bytes into 0x00E209A0, the rest into
     * a block at 0x00E36C74. 0 when a file does not open */
    static const struct {
        uint32_t pointer, head, site;
    } part[2] = {{G_TEXT_BUFFER, 0x1F5Cu, 0x004CEC93u}, {G_MM_TEXT, 0x13898u, 0x004CED1Au}};
    for (int k = 0; k < 2; k++) {
        const char *path = k ? mm : text;
        int32_t size = platform_file_read(st, path, 0, NULL, 0);
        if (size < 0) {
            return 0;
        }
        if (U32(st, part[k].pointer) != 0) {
            boot_release(st, host, U32(st, part[k].pointer));
            U32(st, part[k].pointer) = 0;
        }
        uint32_t rest = (uint32_t) size - part[k].head;
        uint32_t block = boot_alloc(st, host, rest, part[k].site);
        U32(st, part[k].pointer) = block;
        if (k == 0) {
            platform_file_read(st, path, 0, st->mem + (G_TEXT_HEADER - STATE_BASE), 0x1C);
            platform_file_read(st, path, 0x1C, st->mem + (G_TEXT_GROUPS - STATE_BASE), 8000);
        } else {
            platform_file_read(st, path, 0, st->mem + (G_MM_TEXT_HEAD - STATE_BASE), 0x13898);
        }
        uint8_t *p = block ? state_ptr(st, block, rest) : NULL;
        if (!p) {
            state_unported(st, part[k].site);
            return 0;
        }
        platform_file_read(st, path, part[k].head, p, rest);
    }
    return 1;
}

static void copy_string(game_state *st, uint32_t src, uint32_t dst)
{
    /* strcpy (inlined: rep movsd / movsb over strlen + 1 bytes) */
    for (;;) {
        uint8_t c = rd8(st, src++);
        U8(st, dst++) = c;
        if (!c) {
            break;
        }
    }
}

void boot_names(game_state *st)
{
    /* @bin 0x004149D0 text 9.5 into 0x00E38E74 (the player's name), 9.6 into 0x00E37100 and 0x00E370BC,
     * 9.7 into 0x00E37078 */
    text_seek(st, 9, 5);
    copy_string(st, U32(st, G_TEXT_CURSOR), G_PLAYER_NAME);
    text_seek(st, 9, 6);
    uint32_t s = U32(st, G_TEXT_CURSOR);
    copy_string(st, s, G_FILES_NAME);
    copy_string(st, s, G_LAST_NAME);
    text_seek(st, 9, 7);
    copy_string(st, U32(st, G_TEXT_CURSOR), G_MAP_NAME);
}

/* the boot's functions as ports: without a host (memory outside the state) they are partial */
static int32_t run_settings(game_state *st, const int32_t *a)
{
    (void) a;
    return boot_settings(st, NULL, 1024);
}

static int32_t run_ini(game_state *st, const int32_t *a)
{
    (void) a;
    return boot_ini_read(st, NULL);
}

static int32_t run_file_read(game_state *st, const int32_t *a)
{
    char path[260];
    uint32_t s = (uint32_t) a[0];
    for (uint32_t i = 0; i < sizeof(path); i++) {
        path[i] = (char) rd8(st, s + i);
        if (!path[i]) {
            return boot_file_read(st, path, (uint32_t) a[1]);
        }
    }
    return 0;
}

static int32_t run_names(game_state *st, const int32_t *a)
{
    (void) a;
    boot_names(st);
    return 0;
}

const ported_function boot_settings_ports[] = {
    {0x004147F0u, "boot_settings", 0, 32, 0, run_settings, PORT_PARTIAL},
    {0x00414A90u, "boot_ini_read", 0, 32, 0, run_ini, PORT_PARTIAL},
    {0x004CEB10u, "boot_file_read", 2, 32, 0, run_file_read, 0},
    {0x004CEC40u, "boot_text_load (boot)", 2, 32, 0, NULL, 0},
    {0x004149D0u, "boot_names", 0, 0, 0, run_names, 0},
};
const int boot_settings_ports_count = sizeof(boot_settings_ports) / sizeof(boot_settings_ports[0]);
