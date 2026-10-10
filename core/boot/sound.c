/* Imhotep - the boot: the sound start-up (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * WinMain's 0x005394F0 (docs/presentation.md 6.1): the sound driver opened (0x0055E6C4, Miles: the
 * platform's), the sound registry (500 names of sound files, in memory outside the state: the sound
 * object's first three dwords hold its capacity, count and entries {name, owned}), music.txt read into
 * the music tables of 0x00E39180, and the sound tables (building, ambient, interface and figure sounds:
 * indexes into the registry). The registry records a file only when it exists (fopen in the original;
 * here the platform's file query). The builders' lists of calls come from tools/gen/gen_boot_data.py
 * (core/boot/boot_data.c). */
#include <stdlib.h>
#include <string.h>

#include "boot/boot_internal.h"
#include "boot/monuments.h"
#include "frame/sound.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"


static uint8_t rd8(game_state *st, uint32_t a)
{
    const uint8_t *p = state_ptr(st, a, 1);
    return p ? *p : 0;
}

static void cstr(game_state *st, uint32_t a, char *out, size_t cap)
{
    size_t i = 0;
    for (; i + 1 < cap; i++) {
        out[i] = (char) rd8(st, a + (uint32_t) i);
        if (!out[i]) {
            return;
        }
    }
    out[i] = 0;
}

static int file_exists(game_state *st, const char *path)
{
    return platform_file_read(st, path, 0, NULL, 0) >= 0; /* fopen(path, "rb") succeeds */
}

/* @bin 0x0055E6C4 (thiscall on the sound object) the driver: Windows NT before 5 has no sound (+0x200 :=
 * 1; the platform's answer: not such a system); Miles started (AIL_startup, set_preference); the wave
 * format at +0x17C from the rate (+0x18C), bits (+0x190) and channels (+0x194); AIL_waveOutOpen (the
 * driver at +0x178: the platform's handle); five voices (0x0055DEA2) */
static int32_t driver_open(game_state *st, const boot_host *host, uint32_t self)
{
    U16(st, self + 0x17C) = 1;
    U16(st, self + 0x17E) = U16(st, self + 0x194);
    U32(st, self + 0x180) = U32(st, self + 0x18C);
    int32_t bits = S32(st, self + 0x190);
    int32_t bytes = (bits + (int32_t) ((uint32_t) (bits >> 31) & 7u)) >> 3; /* signed bits / 8 */
    S32(st, self + 0x184) = (int32_t) ((uint32_t) S32(st, self + 0x18C) * (uint32_t) bytes * (uint32_t) S32(st, self + 0x194));
    U16(st, self + 0x188) = (uint16_t) (bytes * (int16_t) S32(st, self + 0x194));
    U16(st, self + 0x18A) = U16(st, self + 0x190);
    uint32_t driver = host && host->handle ? host->handle(host->ctx, st, BOOT_SOUND_DRIVER, 0) : 0;
    U32(st, self + 0x178) = driver;
    if (driver == 0) {
        return 0; /* OutputDebugStringA "Unable to initialize digital sound" */
    }
    /* @bin 0x0055DEA2 (5): nothing allocated yet (+0x19C 0); four blocks outside the state: the sample
     * handles (+0x19C, 4 bytes each), +0x1A0 (4), the voices (+0x1A8, 0x28), +0x1AC (0xC) */
    const int32_t n = 5;
    if (U32(st, self + 0x19C) != 0 || U32(st, self + 0x1A0) != 0 || U32(st, self + 0x1A8) != 0 ||
        U32(st, self + 0x1AC) != 0) {
        state_unported(st, 0x0055EC7Cu); /* voices open: ended and released first */
        return 0;
    }
    uint32_t handles = boot_alloc(st, host, (uint32_t) n << 2, 0x0055E02Eu);
    U32(st, self + 0x19C) = handles;
    if (!handles) {
        return 0;
    }
    U32(st, self + 0x1A0) = boot_alloc(st, host, (uint32_t) n << 2, 0x0055E05Fu);
    if (!U32(st, self + 0x1A0)) {
        return 0;
    }
    U32(st, self + 0x1A8) = boot_alloc(st, host, (uint32_t) n * 0x28, 0x0055E090u);
    if (!U32(st, self + 0x1A8)) {
        return 0;
    }
    U32(st, self + 0x1AC) = boot_alloc(st, host, (uint32_t) n * 0xC, 0x0055E0C1u);
    if (!U32(st, self + 0x1AC)) {
        return 0;
    }
    S32(st, self + 0x198) = n;
    for (int32_t i = 0; i < n; i++) {
        /* AIL_allocate_sample_handle(driver) */
        uint32_t h = host->handle(host->ctx, st, BOOT_SOUND_SAMPLE, i);
        P32(st, handles + 4u * (uint32_t) i) = h;
        if (h == 0) {
            break;
        }
    }
    /* @bin 0x0055E5D2 each voice: -1, -1, 100, 0x40, 1, 0, -1, 0, 0, 0 */
    for (int32_t i = 0; i < S32(st, self + 0x198); i++) {
        static const uint32_t voice[10] = {0xFFFFFFFFu, 0xFFFFFFFFu, 100, 0x40, 1, 0, 0xFFFFFFFFu, 0, 0, 0};
        for (uint32_t k = 0; k < 10; k++) {
            P32(st, U32(st, self + 0x1A8) + (uint32_t) i * 0x28 + 4 * k) = voice[k];
        }
    }
    for (int32_t i = 0; i < S32(st, self + 0x198); i++) {
        P32(st, U32(st, self + 0x1AC) + (uint32_t) i * 0xC) = 0xFFFFFFFFu;
        P32(st, U32(st, self + 0x1AC) + 4 + (uint32_t) i * 0xC) = 0;
        P32(st, U32(st, self + 0x1AC) + 8 + (uint32_t) i * 0xC) = 0;
    }
    return 1;
}

/* @bin 0x0055DC63 (thiscall, 500) the registry: none before (0x0055DC18: +8 is 0); a block of 500 entries
 * of 8 bytes after their count (0x00560280: both 0) */
static int32_t registry_create(game_state *st, const boot_host *host, uint32_t self, int32_t cap)
{
    if (cap < 1) {
        return 0;
    }
    if (U32(st, self + 8) != 0) {
        state_unported(st, 0x00560210u); /* an older registry released */
        return 0;
    }
    uint32_t p = boot_alloc(st, host, (uint32_t) cap * 8 + 4, 0x0055DCACu);
    uint32_t entries = 0;
    if (p) {
        P32(st, p) = (uint32_t) cap;
        for (int32_t i = 0; i < cap; i++) {
            P32(st, p + 4 + 8 * (uint32_t) i + 4) = 0;
            P32(st, p + 4 + 8 * (uint32_t) i) = 0;
        }
        entries = p + 4;
    }
    U32(st, self + 8) = entries;
    if (!entries) {
        return 0;
    }
    S32(st, self) = cap;
    S32(st, self + 4) = 0;
    return 1;
}

static int strcmp_at(game_state *st, uint32_t a, uint32_t b)
{
    for (;; a++, b++) {
        uint8_t x = rd8(st, a), y = rd8(st, b);
        if (x != y || !x) {
            return x - y;
        }
    }
}

/* @bin 0x00539C60 (name: a string of the state) the registry's index of the file, added when it exists
 * (0x0055DD3F: an equal name already there gives its index; a full registry -1); a file only on the CD
 * ("%c:\\%s") would be added under a copy of that path (strdup): the platform has no CD; -1 when missing */
int32_t sound_register(game_state *st, uint32_t name)
{
    char path[260];
    cstr(st, name, path, sizeof(path));
    if (!file_exists(st, path)) {
        return -1; /* "D:\\" + name is not on the platform either */
    }
    uint32_t self = G_SOUND;
    int32_t count = S32(st, self + 4);
    if (count >= S32(st, self)) {
        return -1;
    }
    uint32_t entries = U32(st, self + 8);
    if (!entries) {
        return -1;
    }
    for (int32_t i = 0; i < count; i++) {
        uint32_t e = P32(st, entries + 8 * (uint32_t) i);
        if (e != 0 && strcmp_at(st, e, name) == 0) {
            return i;
        }
    }
    P32(st, entries + 8 * (uint32_t) count) = name;
    P32(st, entries + 8 * (uint32_t) count + 4) = 0;
    S32(st, self + 4) = count + 1;
    return count;
}

/* ---- music.txt (0x00539250) ---- */

static int is_delim(char c)
{
    return c == ' ' || c == ',' || c == '\t' || c == '\n'; /* @bin 0x005F908C */
}

/* CRT strtok over the line */
static char *next_token(char **pos, char *start)
{
    char *p = start ? start : *pos;
    if (!p) {
        return NULL;
    }
    while (*p && is_delim(*p)) {
        p++;
    }
    if (!*p) {
        *pos = p;
        return NULL;
    }
    char *t = p;
    while (*p && !is_delim(*p)) {
        p++;
    }
    if (*p) {
        *p++ = 0;
    }
    *pos = p;
    return t;
}

/* @bin 0x00562DCB strtol(s, &end, 10): blanks, a sign, decimal digits, clamped to the int range */
static int32_t crt_strtol10(const char *s)
{
    while (*s == ' ' || (*s >= 9 && *s <= 13)) {
        s++;
    }
    int neg = *s == '-';
    if (*s == '-' || *s == '+') {
        s++;
    }
    uint64_t v = 0;
    int over = 0;
    for (; *s >= '0' && *s <= '9'; s++) {
        v = v * 10 + (uint64_t) (*s - '0');
        if (v > 0x80000000ull) {
            over = 1;
            v = 0x80000000ull;
        }
    }
    if (neg) {
        return over || v == 0x80000000ull ? (int32_t) 0x80000000u : -(int32_t) v;
    }
    return over || v > 0x7FFFFFFFu ? 0x7FFFFFFF : (int32_t) v;
}

/* @bin 0x005395E0 (thiscall on the music tables) a tune of mission `mission` (< 0x36): "A" tunes (at most
 * 10, after the 50 "M" slots) take the delay; "M" tunes (at most 0x32) the delay and the population
 * range; each a 0x70-byte record whose name is the path */
static int32_t tune_add(game_state *st, uint32_t self, const char *path, int32_t mission, int32_t a, int32_t b,
                        int32_t c, int32_t is_a)
{
    if (mission > 0x35) {
        return -1;
    }
    uint32_t rec = self + (uint32_t) mission * 0x1B10u;
    size_t n = strlen(path) + 1;
    if (is_a) {
        int32_t k = S32(st, rec + 0x1B0C);
        if (k == 10) {
            return -1;
        }
        memcpy(st->mem + (rec + (uint32_t) (k + 0x32) * 0x70 - STATE_BASE), path, n);
        uint32_t e = rec + (uint32_t) k * 0x70;
        S32(st, e + 0x1644) = a;
        S32(st, e + 0x1648) = 0;
        S32(st, e + 0x164C) = 0;
        S32(st, rec + 0x1B0C) = k + 1;
        return 1;
    }
    int32_t k = S32(st, rec + 0x1B08);
    if (k == 0x32) {
        return -1;
    }
    uint32_t e = rec + (uint32_t) k * 0x70;
    memcpy(st->mem + (e - STATE_BASE), path, n);
    S32(st, e + 0x64) = a;
    S32(st, e + 0x68) = b;
    S32(st, e + 0x6C) = c;
    S32(st, rec + 0x1B08) = k + 1;
    return 1;
}

/* @bin 0x00539250 music.txt (0x005F9094, opened "rt": text mode, CR LF read as LF, ^Z ends it), line by
 * line (fgets, 200 bytes): ';' comments; "AMBIENT" flags the mission's next "M" tune; else NAME KIND
 * MISSION A B C (the numbers keep their last values when missing) */
static void music_read(game_state *st)
{
    int32_t size = platform_file_read(st, "music.txt", 0, NULL, 0);
    if (size < 0) {
        return;
    }
    char *text = malloc((size_t) size + 1);
    if (!text) {
        return;
    }
    platform_file_read(st, "music.txt", 0, (uint8_t *) text, (uint32_t) size);
    int32_t len = 0;
    for (int32_t i = 0; i < size; i++) { /* text mode */
        if (text[i] == 0x1A) {
            break;
        }
        if (text[i] == '\r' && i + 1 < size && text[i + 1] == '\n') {
            continue;
        }
        text[len++] = text[i];
    }
    int32_t mission = 0, a = 0, b = 1, c = 100;
    int32_t pos = 0;
    while (pos < len) {
        char line[200];
        int32_t n = 0;
        while (n < 199 && pos < len) {
            char ch = text[pos++];
            line[n++] = ch;
            if (ch == '\n') {
                break;
            }
        }
        line[n] = 0;
        if (line[0] == ';' || line[0] == 0) {
            continue;
        }
        char *save = NULL;
        char *name = next_token(&save, line);
        if (!name) {
            continue;
        }
        if (strcmp(name, "AMBIENT") == 0) { /* @bin 0x005F9084 */
            uint32_t rec = G_MUSIC + (uint32_t) mission * 0x1B10u;
            S32(st, rec + 0x1A40 + 4u * (uint32_t) S32(st, rec + 0x1B08)) = 1;
            continue;
        }
        char *kind = next_token(&save, NULL);
        int32_t is_a = kind && strcmp(kind, "A") == 0; /* @bin 0x005F9080 */
        char *t;
        if ((t = next_token(&save, NULL)) != NULL) {
            mission = crt_strtol10(t);
        }
        if ((t = next_token(&save, NULL)) != NULL) {
            a = crt_strtol10(t);
        }
        if ((t = next_token(&save, NULL)) != NULL) {
            b = crt_strtol10(t);
        }
        if ((t = next_token(&save, NULL)) != NULL) {
            c = crt_strtol10(t);
        }
        /* @bin 0x0053944A "audio\\music\\%s"; when it does not open, "%c:\\audio\\music\\%s" (the CD) */
        char path[100];
        int k = 0;
        const char *pre = "audio\\music\\";
        for (; pre[k] && k < 99; k++) {
            path[k] = pre[k];
        }
        for (const char *q = name; *q && k < 99; q++) {
            path[k++] = *q;
        }
        path[k] = 0;
        if (!file_exists(st, path)) {
            char cd[100];
            int m = 0;
            cd[m++] = (char) U8(st, G_DRIVE);
            cd[m++] = ':';
            cd[m++] = '\\';
            for (int q = 0; path[q] && m < 99; q++) {
                cd[m++] = path[q];
            }
            cd[m] = 0;
            memcpy(path, cd, (size_t) m + 1);
        }
        tune_add(st, G_MUSIC, path, mission, a, b, c, is_a);
    }
    free(text);
}

/* ---- the sound tables ---- */

/* @bin 0x00539CF0 (building type, name, kind) kind 0: the building's own sound (+0xC of its 5-dword
 * record at 0x00F54670); kind 1: one of at most 3 variants (count at +0x10) */
static void building_sound(game_state *st, int32_t type, uint32_t name, int32_t kind)
{
    uint32_t rec = G_BUILDING_SOUND_TABLE + 20u * (uint32_t) type;
    if (kind == 0) {
        S32(st, rec + 0xC) = sound_register(st, name);
    } else if (kind == 1 && S32(st, rec + 0x10) < 3) {
        int32_t v = sound_register(st, name);
        S32(st, rec + 4u * (uint32_t) S32(st, rec + 0x10)) = v;
        S32(st, rec + 0x10) += 1;
    }
}

/* @bin 0x0053BC80 (terrain, name, kind) the ambient records (12 dwords from 0x00F54550): kind 0 the
 * terrain's own (+0x28), kind 1 one of at most 10 variants (count at +0x2C) */
static void ambient_sound(game_state *st, int32_t terrain, uint32_t name, int32_t kind)
{
    uint32_t rec = G_AMBIENT_SOUNDS + 0x30u * (uint32_t) terrain;
    if (kind == 0) {
        S32(st, rec + 0x28) = sound_register(st, name);
    } else if (kind == 1 && S32(st, rec + 0x2C) < 10) {
        int32_t v = sound_register(st, name);
        S32(st, rec + 4u * (uint32_t) S32(st, rec + 0x2C)) = v;
        S32(st, rec + 0x2C) += 1;
    }
}

void boot_sound_start(game_state *st, const boot_host *host)
{
    /* @bin 0x005394F0 */
    S32(st, G_SOUND_ON) = driver_open(st, host, G_SOUND);
    registry_create(st, host, G_SOUND, 500);
    S32(st, G_MUSIC + 0x59A50u) = 0; /* @bin 0x00539A40 (0) */
    music_read(st);
    music_window(st, G_MUSIC, 1); /* @bin 0x00539730 (1) */
    S32(st, G_AMBIENT_CHOICE) = -1;
    /* @bin 0x00539D60 the building sounds: 5-dword records from 0x00F54670 (to 0x00F55900): -1 x 4, 0 */
    for (uint32_t p = G_BUILDING_SOUNDS;;) {
        S32(st, p - 0xC) = -1;
        S32(st, p - 8) = -1;
        S32(st, p - 4) = -1;
        S32(st, p) = -1;
        S32(st, p + 4) = 0;
        p += 20;
        if (p >= G_BUILDING_SOUNDS + 237u * 20u) {
            break;
        }
    }
    for (int i = 0; i < sound_list_539d60_count; i++) {
        building_sound(st, sound_list_539d60[i].index, sound_list_539d60[i].name, sound_list_539d60[i].kind);
    }
    /* @bin 0x0053BCF0 the ambient sounds: 12-dword records to 0x00F54698: 10 x -1, -1, 0 */
    for (uint32_t p = G_AMBIENT_SOUNDS + 0x28u;; p += 0x30) {
        for (uint32_t k = 0; k < 10; k++) {
            S32(st, p - 0x28 + 4 * k) = -1;
        }
        S32(st, p) = -1;
        S32(st, p + 4) = 0;
        if (p + 0x30 >= G_AMBIENT_SOUNDS + 0x148u) {
            break;
        }
    }
    for (int i = 0; i < sound_list_53bcf0_count; i++) {
        ambient_sound(st, sound_list_53bcf0[i].index, sound_list_53bcf0[i].name, sound_list_53bcf0[i].kind);
    }
    /* @bin 0x0053AF90 the interface sounds: 100 dwords at 0x00F543B8, -1, then 0x0053AF70 */
    for (uint32_t k = 0; k < 100; k++) {
        S32(st, G_INTERFACE_SOUNDS + 4 * k) = -1;
    }
    for (int i = 0; i < sound_list_53af90_count; i++) {
        S32(st, G_INTERFACE_SOUNDS + 4u * (uint32_t) sound_list_53af90[i].index) =
            sound_register(st, sound_list_53af90[i].name);
    }
    /* @bin 0x0053B130 the figure sounds: 0xA5 dwords at 0x00F54120, -1, then 0x0053B100 (5 per figure) */
    for (uint32_t k = 0; k < 0xA5; k++) {
        S32(st, G_FIGURE_SOUNDS + 4 * k) = -1;
    }
    for (int i = 0; i < sound_list_53b130_count; i++) {
        S32(st, G_FIGURE_SOUNDS + 4u * (uint32_t) (sound_list_53b130[i].index * 5 + sound_list_53b130[i].kind)) =
            sound_register(st, sound_list_53b130[i].name);
    }
}

static int32_t run_sound_register(game_state *st, const int32_t *a)
{
    return sound_register(st, (uint32_t) a[0]);
}

const ported_function boot_sound_ports[] = {
    {0x00539C60u, "sound_register", 1, 32, 0, run_sound_register, 0},
};
const int boot_sound_ports_count = sizeof(boot_sound_ports) / sizeof(boot_sound_ports[0]);
