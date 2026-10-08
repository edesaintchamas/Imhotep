/* Imhotep - the port's boot on a state file (the port's side of tests/differential/oracle_boot_check.py).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   oracle_boot_run IN OUT --game DIR [--steps A-B] [--display W,H,FORMAT] [--sound]
 *                   [--arena FILE] [--allocs FILE] [--free-sites SITE,...] [--arena-out FILE] [--player DIR]
 *
 * --player DIR: the player's files (the options file Pharaoh.inf that the settings read), looked in
 * before the game directory, as the oracle's Game.files are (oracle_boot_check.py writes the
 * original's there).
 * IN is a state file (core/state/state.h), or "exe": a new state from DIR/Pharaoh.exe's sections
 * (platform/exe_image.c), the start of a boot. Steps A..B of core/boot (game_boot's steps,
 * boot_step_name) run natively; OUT gets the state.
 *
 * Memory outside the state: one arena at 0x68000000 (the oracle's other heap), mapped into the state;
 * --arena FILE gives its first bytes (the oracle's other heap before step A: the text files, ...).
 * --allocs FILE is the original's allocations in that memory during the steps, one per line
 * "SITE SIZE ADDR" (hex, decimal, hex; SITE the return address of the original's call to operator new /
 * malloc, 0053F726 for the pixel loads of 0x0053F660, 'S' + SURFACE for DirectDraw surfaces): each
 * allocation of the port (boot_host.alloc with its site, the platform's pixel loads) takes the next
 * line of its site, which must have its size, and gets that line's address, so that the port's
 * pointers are the original's. Lines of other sites that the port passes over (memory the port does
 * not allocate: the graphics heap's list nodes, ...) are printed "skipped SITE SIZE ADDR"; an
 * allocation with no line left (or of another size) fails the run (exit 4). Without --allocs the
 * arena is bumped in 16-byte steps from the end of --arena (a game's own run). --free-sites: sites
 * whose sizes differ from the original's for a known reason (tests/differential/oracle_boot_check.py
 * FREE_SITES): their blocks come from the arena's last 16 MiB and the original's line is passed over.
 * Prints "alloc SITE SIZE ADDR" per allocation, "unported ADDR" when a step reaches an original
 * function that is not ported (exit 3), "step K NAME -> R" per step. --arena-out writes the arena up
 * to its last allocation (the back buffer of a display boot is in it). */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "boot/boot.h"
#include "platform/exe_image.h"
#include "present/present.h"
#include "sim/platform.h"
#include "state/state.h"

#define ARENA_CAP 0x10000000u /* the oracle's other heap (tools/oracle/emu.py OTHER_HEAP_SIZE) */

typedef struct {
    uint32_t site, size, addr;
    int taken;
} sched_line;

static struct {
    uint32_t free_sites[16];
    int nfree;
    uint32_t free_next; /* the next address of the free sites' region */
    boot_arena arena;
    sched_line *lines;
    int nlines, failed;
    const char *game;
    const char *player; /* --player DIR: the player's files (Pharaoh.inf), looked in first */
    uint32_t top;
} H;

/* DIR + the game's relative path, '\\' as '/', each part matched case-insensitively (as
 * tools/oracle/game.py and state_replay resolve them) */
static void resolve(const char *dir, const char *path, char *full, size_t cap)
{
    snprintf(full, cap, "%s", dir);
    for (const char *p = path; *p;) {
        char part[256];
        size_t n = 0;
        while (*p && *p != '\\' && *p != '/' && n + 1 < sizeof(part)) {
            part[n++] = *p++;
        }
        part[n] = 0;
        while (*p == '\\' || *p == '/') {
            p++;
        }
        DIR *d = opendir(full);
        const char *match = part;
        struct dirent *e;
        while (d && (e = readdir(d)) != NULL) {
            if (strcasecmp(e->d_name, part) == 0) {
                match = e->d_name;
                break;
            }
        }
        size_t l = strlen(full);
        snprintf(full + l, cap - l, "/%s", match);
        if (d) {
            closedir(d);
        }
    }
}

static int32_t file_read(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    (void) ctx;
    char full[4096];
    struct stat sb;
    FILE *f = NULL;
    if (H.player) {
        resolve(H.player, path, full, sizeof(full));
        f = stat(full, &sb) == 0 && S_ISREG(sb.st_mode) ? fopen(full, "rb") : NULL;
    }
    if (!f) {
        resolve(H.game, path, full, sizeof(full));
        f = stat(full, &sb) == 0 && S_ISREG(sb.st_mode) ? fopen(full, "rb") : NULL;
    }
    if (!f) {
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    if (buf && size && (long) offset < n) {
        fseek(f, (long) offset, SEEK_SET);
        size_t want = (size_t) (n - (long) offset) < size ? (size_t) (n - (long) offset) : size;
        if (fread(buf, 1, want, f) != want) {
            n = -1;
        }
    }
    fclose(f);
    return (int32_t) n;
}

static uint32_t take(uint32_t site, uint32_t size)
{
    uint32_t p = 0;
    int free_site = 0;
    for (int i = 0; i < H.nfree; i++) {
        free_site |= H.free_sites[i] == site;
    }
    if (free_site && H.lines) {
        /* a site whose size differs from the original's for a known reason: the original's line is
         * passed over, the block comes from the end of the arena */
        for (int i = 0; i < H.nlines; i++) {
            if (!H.lines[i].taken && H.lines[i].site == site) {
                H.lines[i].taken = 1;
                break;
            }
        }
        p = H.free_next;
        H.free_next += ((size ? size : 1) + 15u) & ~15u;
        printf("alloc %08X %u %08X free\n", site, size, p);
        return p;
    }
    if (!H.lines) {
        p = boot_arena_alloc(&H.arena, size);
    } else {
        for (int i = 0; i < H.nlines; i++) {
            sched_line *l = &H.lines[i];
            if (l->taken || l->site != site) {
                continue;
            }
            if (l->size != size) {
                printf("alloc mismatch: site %08X size %u, the original's next one %u at %08X\n", site, size, l->size,
                       l->addr);
                H.failed = 1;
                return 0;
            }
            l->taken = 1;
            p = l->addr;
            break;
        }
        if (!p) {
            printf("alloc mismatch: site %08X size %u, the original made no other\n", site, size);
            H.failed = 1;
            return 0;
        }
    }
    if (p && p + size > H.top) {
        H.top = p + size;
    }
    printf("alloc %08X %u %08X\n", site, size, p);
    return p;
}

static uint32_t host_alloc(void *ctx, game_state *st, uint32_t size, uint32_t site)
{
    (void) ctx;
    (void) st;
    return take(site, size);
}

static uint32_t host_surface(void *ctx, game_state *st, int32_t w, int32_t h, int32_t primary)
{
    (void) ctx;
    (void) st;
    (void) primary;
    return take(0x53000000u, (uint32_t) w * 2u * (uint32_t) h); /* 'S' sites: surfaces */
}

/* @bin 0x004CDF20 555 -> 565 (0xF81F, the transparent colour, kept) */
static void convert_plain(uint8_t *p, int32_t n)
{
    for (; n > 0; n -= 2, p += 2) {
        uint16_t v = (uint16_t) (p[0] | p[1] << 8);
        if (v != 0xF81F) {
            v = (uint16_t) ((v & 0xFFE0) << 1 | (v & 0x1F));
            p[0] = (uint8_t) v;
            p[1] = (uint8_t) (v >> 8);
        }
    }
}

/* @bin 0x004CDEB0 the same for RLE data: a byte 0xFF is followed by a skip byte (2 bytes passed), any
 * other is a count of pixels converted */
static void convert_rle(uint8_t *p, int32_t n)
{
    while (n != 0 && n > -1) {
        int8_t c = (int8_t) p[0];
        if (c == -1) {
            p += 2;
            n -= 2;
        } else {
            p += 1;
            n -= 1;
            for (; c > 0; c--) {
                uint16_t v = (uint16_t) (p[0] | p[1] << 8);
                v = (uint16_t) ((v & 0x7FE0) << 1 | (v & 0x1F));
                p[0] = (uint8_t) v;
                p[1] = (uint8_t) (v >> 8);
                p += 2;
                n -= 2;
            }
        }
    }
}

static int display_565;

static uint32_t pixels_load(void *ctx, const char *path, uint32_t skip, uint32_t size, int32_t convert, int32_t rle)
{
    /* @bin 0x0053F726 the pixel buffer of 0x0053F660: the file's bytes from `skip` */
    uint32_t p = take(0x0053F726u, size);
    uint8_t *dst = p ? H.arena.mem + (p - H.arena.base) : NULL;
    if (dst && file_read(ctx, path, skip, dst, size) < 0) {
        return 0;
    }
    /* @bin 0x0053F74F the 565 conversion of a 565 display (the platform's: core/sim/platform.h) */
    if (dst && (uint8_t) convert && display_565) {
        if ((uint8_t) rle) {
            convert_rle(dst, (int32_t) size);
        } else {
            convert_plain(dst, (int32_t) size);
        }
    }
    return p;
}

/* the window system's answers as the oracle gives them (tools/oracle/game.py: the window 0x4000, every
 * cursor 0x70); DirectDraw's objects are the original's (the next 'C' line of --allocs) */
static uint32_t host_handle(void *ctx, game_state *st, int32_t kind, int32_t id)
{
    (void) ctx;
    (void) st;
    if (kind == BOOT_WINDOW) {
        return 0x4000;
    }
    if (kind == BOOT_CURSOR) {
        return 0x70;
    }
    if (kind == BOOT_SOUND_DRIVER) {
        return 0x5D000001u; /* tools/oracle/sound.py FakeMiles DRIVER */
    }
    if (kind == BOOT_SOUND_SAMPLE) {
        return 0x5A000001u + (uint32_t) id; /* FakeMiles SAMPLE_BASE + n */
    }
    return take(0x43000000u, 16); /* 'C' */
}

static const char *host_cwd(void *ctx)
{
    (void) ctx;
    return "C:\\"; /* tools/oracle/emu.py GetCurrentDirectoryA */
}

static void parse_display(const char *s, boot_display *d)
{
    sscanf(s, "%d,%d,%d", &d->width, &d->height, &d->format);
}

static int read_allocs(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    char line[256];
    int cap = 0;
    while (fgets(line, sizeof(line), f)) {
        sched_line l = {0};
        char site[32];
        if (sscanf(line, "%31s %u %x", site, &l.size, &l.addr) != 3) {
            continue;
        }
        l.site = site[0] == 'S' ? 0x53000000u : site[0] == 'C' ? 0x43000000u : (uint32_t) strtoul(site, NULL, 16);
        if (H.nlines == cap) {
            cap = cap ? 2 * cap : 1024;
            H.lines = realloc(H.lines, sizeof(sched_line) * (size_t) cap);
        }
        H.lines[H.nlines++] = l;
    }
    fclose(f);
    if (!H.lines) {
        H.lines = calloc(1, sizeof(sched_line));
    }
    return 0;
}

int main(int argc, char **argv)
{
    present_register();
    if (argc < 3) {
        fprintf(stderr, "usage: oracle_boot_run IN OUT --game DIR [--steps A-B] [--display W,H,F] [--sound] "
                        "[--arena FILE] [--allocs FILE] [--arena-out FILE] [--player DIR]\n");
        return 2;
    }
    boot_display display = {0, 0, 0, 0};
    int sound = 0, first = 0, last = -1;
    const char *arena_in = NULL, *arena_out = NULL, *allocs = NULL;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            H.game = argv[++i];
        } else if (strcmp(argv[i], "--player") == 0 && i + 1 < argc) {
            H.player = argv[++i];
        } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
            sscanf(argv[++i], "%d-%d", &first, &last);
        } else if (strcmp(argv[i], "--display") == 0 && i + 1 < argc) {
            parse_display(argv[++i], &display);
        } else if (strcmp(argv[i], "--sound") == 0) {
            sound = 1;
        } else if (strcmp(argv[i], "--arena") == 0 && i + 1 < argc) {
            arena_in = argv[++i];
        } else if (strcmp(argv[i], "--arena-out") == 0 && i + 1 < argc) {
            arena_out = argv[++i];
        } else if (strcmp(argv[i], "--free-sites") == 0 && i + 1 < argc) {
            for (char *t = argv[++i]; *t && H.nfree < 16;) {
                H.free_sites[H.nfree++] = (uint32_t) strtoul(t, &t, 16);
                if (*t == ',') {
                    t++;
                }
            }
        } else if (strcmp(argv[i], "--allocs") == 0 && i + 1 < argc) {
            allocs = argv[++i];
        }
    }
    if (!H.game) {
        fprintf(stderr, "--game DIR is needed\n");
        return 2;
    }
    game_state st;
    if (state_create(&st) != 0) {
        return 2;
    }
    if (strcmp(argv[1], "exe") == 0) {
        if (exe_image_load(H.game, &st) != 0) {
            fprintf(stderr, "cannot read %s/Pharaoh.exe\n", H.game);
            return 2;
        }
    } else {
        FILE *f = fopen(argv[1], "rb");
        if (!f || state_read_file(&st, f) != 0) {
            fprintf(stderr, "cannot read %s\n", argv[1]);
            return 2;
        }
        fclose(f);
    }
    H.arena.base = BOOT_ARENA_BASE;
    H.arena.cap = ARENA_CAP;
    H.arena.mem = calloc(ARENA_CAP, 1);
    if (!H.arena.mem) {
        return 2;
    }
    if (arena_in) {
        FILE *f = fopen(arena_in, "rb");
        if (!f) {
            fprintf(stderr, "cannot read %s\n", arena_in);
            return 2;
        }
        H.arena.used = (uint32_t) fread(H.arena.mem, 1, ARENA_CAP, f);
        H.arena.used = (H.arena.used + 15u) & ~15u;
        fclose(f);
    }
    H.top = H.arena.base + H.arena.used;
    H.free_next = H.arena.base + 0x0F000000u; /* the last 16 MiB of the arena */
    if (allocs && read_allocs(allocs) != 0) {
        fprintf(stderr, "cannot read %s\n", allocs);
        return 2;
    }
    if (boot_arena_map(&H.arena, &st) != 0) {
        return 2;
    }
    sim_platform platform = {0};
    platform.file_read = file_read;
    platform.pixels_load = pixels_load;
    st.platform = &platform;
    boot_host host = {NULL, host_alloc, NULL, host_surface, host_handle, host_cwd, NULL};
    display_565 = display.format == 565;
    int n = boot_step_count(&display, sound);
    if (last < 0 || last >= n) {
        last = n - 1;
    }
    int status = 0;
    for (int k = first; k <= last; k++) {
        int r = boot_step(&st, &host, &display, sound, k);
        printf("step %d %s -> %d\n", k, boot_step_name(&display, sound, k), r);
        if (st.unported) {
            printf("unported %08X\n", st.unported);
            status = 3;
            break;
        }
        if (H.failed) {
            status = 4;
            break;
        }
        if (r != 0) {
            status = 1;
            break;
        }
    }
    for (int i = 0; H.lines && i < H.nlines; i++) {
        if (!H.lines[i].taken) {
            printf("skipped %08X %u %08X\n", H.lines[i].site, H.lines[i].size, H.lines[i].addr);
        }
    }
    FILE *f = fopen(argv[2], "wb");
    if (!f || state_write_file(&st, f) != 0 || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 2;
    }
    if (arena_out) {
        f = fopen(arena_out, "wb");
        uint32_t n_out = H.top - H.arena.base;
        if (!f || fwrite(H.arena.mem, 1, n_out, f) != n_out || fclose(f) != 0) {
            fprintf(stderr, "cannot write %s\n", arena_out);
            return 2;
        }
    }
    return status;
}
