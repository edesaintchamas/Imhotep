/* Imhotep - where build/imhotep finds the game files and keeps the player's (see install.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE /* realpath, getpid under -std=c11 (glibc; the Makefile also defines it) */
#endif
#include "platform/install.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL.h>

#include "platform/fingerprint.h"

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#define SEP "\\"
#define getpid _getpid
#else
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#define SEP "/"
#endif

/* a path or line formatted into out: 1 when it fits, 0 when it was cut (a cut path names no
 * installation: it is tried and does not qualify) */
static int put(char *out, size_t size, const char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    int n = vsnprintf(out, size, fmt, a);
    va_end(a);
    return n >= 0 && (size_t) n < size;
}

#define PREF_ORG "Imhotep"
#define PREF_APP "Imhotep"
#define CONFIG_NAME "imhotep.cfg"

static int is_dir(const char *path)
{
    struct stat sb;
    return path && *path && stat(path, &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFDIR;
}

/* the per-user data directory, without its trailing separator; 0, or -1 */
static int pref_dir(char *out, size_t size)
{
    char *p = SDL_GetPrefPath(PREF_ORG, PREF_APP); /* made if missing */
    if (!p) {
        return -1;
    }
    put(out, size, "%s", p);
    SDL_free(p);
    size_t n = strlen(out);
    while (n > 1 && (out[n - 1] == '/' || out[n - 1] == '\\')) {
        out[--n] = 0;
    }
    return 0;
}

static void absolute(const char *path, char *out, size_t size)
{
#ifdef _WIN32
    if (!_fullpath(out, path, size)) {
        put(out, size, "%s", path);
    }
#else
    char buf[PATH_MAX];
    put(out, size, "%s", realpath(path, buf) ? buf : path);
#endif
}

/* ---- the remembered directory (imhotep.cfg: lines "key=value") ---- */

static int config_path(char *out, size_t size)
{
    char dir[1024];
    if (pref_dir(dir, sizeof(dir)) != 0) {
        return -1;
    }
    put(out, size, "%s" SEP CONFIG_NAME, dir);
    return 0;
}

static int remembered(char *out, size_t size)
{
    char path[1200], line[1200];
    FILE *f = config_path(path, sizeof(path)) == 0 ? fopen(path, "r") : NULL;
    int found = -1;
    while (f && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strncmp(line, "game=", 5) == 0 && line[5]) {
            put(out, size, "%s", line + 5);
            found = 0;
        }
    }
    if (f) {
        fclose(f);
    }
    return found;
}

static void remember(const char *game, int verbose)
{
    char abs[1024], old[1024], path[1200], tmp[1300];
    absolute(game, abs, sizeof(abs));
    if (remembered(old, sizeof(old)) == 0 && strcmp(old, abs) == 0) {
        return;
    }
    if (config_path(path, sizeof(path)) != 0) {
        return;
    }
    /* written whole then renamed, so concurrent starts never read half a file */
    put(tmp, sizeof(tmp), "%s.%d.tmp", path, (int) getpid());
    FILE *f = fopen(tmp, "w");
    if (!f) {
        return;
    }
    fprintf(f, "# Imhotep: the game directory found at the last start (see imhotep --help)\ngame=%s\n", abs);
    int bad = fclose(f) != 0;
#ifdef _WIN32
    if (!bad) {
        bad = !MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING);
    }
#else
    if (!bad) {
        bad = rename(tmp, path) != 0;
    }
#endif
    if (bad) {
        remove(tmp);
    } else if (verbose) {
        fprintf(stderr, "imhotep: game directory remembered in %s\n", path);
    }
}

/* ---- the usual places ---- */

typedef struct {
    char (*dirs)[1024];
    int n, max;
} places;

static void add(places *p, const char *dir)
{
    if (p->n < p->max && dir && *dir) {
        put(p->dirs[p->n++], sizeof(p->dirs[0]), "%s", dir);
    }
}

/* the names a GOG (or Steam, refused but reported) install of the game takes */
static const char *const game_names[] = {"Pharaoh + Cleopatra", "Pharaoh Gold", "Pharaoh"};
#define NNAMES (int) (sizeof(game_names) / sizeof(game_names[0]))

static void add_names(places *p, const char *parent)
{
    if (!is_dir(parent)) {
        return;
    }
    char d[1024];
    for (int i = 0; i < NNAMES; i++) {
        put(d, sizeof(d), "%s" SEP "%s", parent, game_names[i]);
        add(p, d);
    }
}

/* a checkout's local/game/app (make extract), from the executable's directory (build/, or deeper) */
static void add_checkout(places *p)
{
    char *base = SDL_GetBasePath();
    if (!base) {
        return;
    }
    char d[1024];
    const char *up = "";
    for (int k = 0; k < 3; k++) {
        put(d, sizeof(d), "%s%slocal" SEP "game" SEP "app", base, up);
        if (is_dir(d)) {
            char abs[1024];
            absolute(d, abs, sizeof(abs));
            add(p, abs);
            break;
        }
        up = k == 0 ? ".." SEP : ".." SEP ".." SEP;
    }
    SDL_free(base);
}

#ifdef _WIN32
/* every game GOG installed: HKLM\SOFTWARE\[WOW6432Node\]GOG.com\Games\<id>, value "path" */
static void add_gog_registry(places *p)
{
    static const char *const roots[] = {"SOFTWARE\\WOW6432Node\\GOG.com\\Games", "SOFTWARE\\GOG.com\\Games"};
    for (int r = 0; r < 2; r++) {
        HKEY games;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, roots[r], 0, KEY_READ, &games) != ERROR_SUCCESS) {
            continue;
        }
        char id[256];
        for (DWORD i = 0;; i++) {
            DWORD idlen = sizeof(id);
            if (RegEnumKeyExA(games, i, id, &idlen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
                break;
            }
            HKEY game;
            if (RegOpenKeyExA(games, id, 0, KEY_READ, &game) != ERROR_SUCCESS) {
                continue;
            }
            char path[1024];
            DWORD type, len = sizeof(path) - 1;
            if (RegQueryValueExA(game, "path", NULL, &type, (BYTE *) path, &len) == ERROR_SUCCESS && type == REG_SZ) {
                path[len] = 0;
                add(p, path);
            }
            RegCloseKey(game);
        }
        RegCloseKey(games);
    }
}
#else
/* <prefix>/drive_c/{GOG Games, Program Files (x86)/GOG Galaxy/Games, ...}/<name> */
static void add_wine_prefix(places *p, const char *prefix)
{
    static const char *const parents[] = {"drive_c/GOG Games", "drive_c/Program Files (x86)/GOG Galaxy/Games",
                                          "drive_c/Program Files/GOG Galaxy/Games", "drive_c/Program Files (x86)/GOG.com",
                                          "drive_c/Program Files (x86)/Steam/steamapps/common"};
    char d[1024];
    for (size_t i = 0; i < sizeof(parents) / sizeof(parents[0]); i++) {
        put(d, sizeof(d), "%s/%s", prefix, parents[i]);
        add_names(p, d);
    }
}

/* each subdirectory of `dir` as a Wine prefix (CrossOver bottles, Lutris and Heroic prefixes) */
static void add_wine_prefixes_in(places *p, const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') {
            continue;
        }
        char sub[1024];
        put(sub, sizeof(sub), "%s/%s", dir, e->d_name);
        add_wine_prefix(p, sub);
    }
    closedir(d);
}
#endif

static int usual_places(places *p)
{
    add_checkout(p);
#ifdef _WIN32
    add_gog_registry(p);
    const char *drive = getenv("SystemDrive");
    char d[1024];
    put(d, sizeof(d), "%s\\GOG Games", drive && *drive ? drive : "C:");
    add_names(p, d);
    static const char *const pf[] = {"ProgramFiles(x86)", "ProgramFiles"};
    for (int i = 0; i < 2; i++) {
        const char *root = getenv(pf[i]);
        if (!root || !*root) {
            continue;
        }
        static const char *const subs[] = {"GOG Galaxy\\Games", "GOG.com", "GOG Games", "Steam\\steamapps\\common", ""};
        for (int k = 0; k < 5; k++) {
            put(d, sizeof(d), *subs[k] ? "%s\\%s" : "%s%s", root, subs[k]);
            add_names(p, d);
        }
    }
#else
    const char *home = getenv("HOME");
    if (home && *home) {
        char d[1024];
        static const char *const parents[] = {"GOG Games", "Games", "Games/Heroic", "games",
                                              ".local/share/Steam/steamapps/common"};
        for (size_t i = 0; i < sizeof(parents) / sizeof(parents[0]); i++) {
            put(d, sizeof(d), "%s/%s", home, parents[i]);
            add_names(p, d);
        }
        put(d, sizeof(d), "%s/.wine", home);
        add_wine_prefix(p, d);
        put(d, sizeof(d), "%s/Games", home); /* Lutris: ~/Games/<game>/drive_c */
        add_wine_prefixes_in(p, d);
#ifdef __APPLE__
        put(d, sizeof(d), "%s/Library/Application Support/CrossOver/Bottles", home);
        add_wine_prefixes_in(p, d);
        put(d, sizeof(d), "%s/Library/Containers/com.isaacmarovitz.Whisky/Bottles", home);
        add_wine_prefixes_in(p, d);
#endif
    }
#ifdef __APPLE__
    add_names(p, "/Applications");
#endif
#endif
    return p->n;
}

/* ---- the search ---- */

static void tell_player(int quiet, const char *title, const char *text)
{
    fprintf(stderr, "imhotep: %s\n%s\n", title, text);
    if (!quiet) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, text, NULL);
    }
}

static const char NEEDED[] =
    "Imhotep plays with the game files of an installation of Pharaoh + Cleopatra, the GOG version 2.1 "
    "(Pharaoh.exe 2.1.0.0): the engine is checked against that exact executable.";

static const char HOW[] =
    "Start Imhotep with\n"
    "    imhotep --game \"FOLDER\"\n"
    "where FOLDER is the game's installation folder (the one holding Pharaoh.exe), or set the "
    "environment variable IMHOTEP_GAME_DIR to it. Imhotep remembers the folder for the next start.";

int install_game_dir(const char *arg, int quiet, int verbose, char *out, size_t size)
{
    char msg[1024], text[4096];
    /* 1, 2: given by the player, taken as given */
    const char *env = getenv("IMHOTEP_GAME_DIR");
    const char *given = arg ? arg : env && *env ? env : NULL;
    if (given) {
        const char *how = arg ? "--game" : "IMHOTEP_GAME_DIR";
        int r = fingerprint_check(given, msg, sizeof(msg));
        if (r != 0) {
            put(text, sizeof(text), "%s (%s).\n\n%s%s%s", msg, how, NEEDED,
                     r == FINGERPRINT_MISSING ? "\n\n" : "", r == FINGERPRINT_MISSING ? HOW : "");
            tell_player(quiet, r == FINGERPRINT_MISSING ? "Pharaoh not found" : "Another version of Pharaoh", text);
            return -1;
        }
        if (verbose) {
            fprintf(stderr, "imhotep: %s (%s)\n", msg, how);
        }
        put(out, size, "%s", given);
        if (arg) {
            remember(given, verbose);
        }
        return 0;
    }
    /* 3: remembered */
    char dir[1024];
    if (remembered(dir, sizeof(dir)) == 0) {
        int r = fingerprint_check(dir, msg, sizeof(msg));
        if (verbose || r != 0) {
            fprintf(stderr, "imhotep: %s (the remembered game directory)%s\n", msg, r != 0 ? ", passed over" : "");
        }
        if (r == 0) {
            put(out, size, "%s", dir);
            return 0;
        }
    }
    /* 4: the usual places */
    static char dirs[64][1024];
    places p = {dirs, 0, 64};
    usual_places(&p);
    char other[1024] = {0};
    for (int i = 0; i < p.n; i++) {
        int r = fingerprint_check(p.dirs[i], msg, sizeof(msg));
        if (r == 0) {
            if (verbose) {
                fprintf(stderr, "imhotep: %s\n", msg);
            }
            put(out, size, "%s", p.dirs[i]);
            remember(p.dirs[i], verbose);
            return 0;
        }
        if (verbose && r == FINGERPRINT_MISMATCH) {
            fprintf(stderr, "imhotep: %s\n", msg);
        }
        if (r == FINGERPRINT_MISMATCH && !other[0]) {
            put(other, sizeof(other), "%s", msg);
        }
    }
    /* none: tell the player what is needed */
    if (other[0]) {
        put(text, sizeof(text), "%s\n\nFound, but refused: %s.\n\n%s", NEEDED, other, HOW);
        tell_player(quiet, "Another version of Pharaoh", text);
    } else {
        put(text, sizeof(text), "%s\n\nNo installation was found in the usual places.\n\n%s", NEEDED, HOW);
        tell_player(quiet, "Pharaoh not found", text);
    }
    return -1;
}

int install_player_dir(char *out, size_t size)
{
    const char *env = getenv("IMHOTEP_PLAYER_DIR");
    if (env && *env) {
        put(out, size, "%s", env);
#ifdef _WIN32
        _mkdir(out);
#else
        mkdir(out, 0755);
#endif
        return is_dir(out) ? 0 : -1;
    }
    return pref_dir(out, size);
}
