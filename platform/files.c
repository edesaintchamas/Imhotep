/* Imhotep - the game's files as the original sees them (see files.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "platform/files.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define make_dir(path) _mkdir(path)
#else
#include <unistd.h>
#define make_dir(path) mkdir(path, 0755)
#endif

static char game_root[1024] = "local/game/app";
static char player_root[1024] = "local/player";

void files_init(const char *game, const char *player)
{
    if (game) {
        snprintf(game_root, sizeof(game_root), "%s", game);
    }
    if (player) {
        snprintf(player_root, sizeof(player_root), "%s", player);
    }
    make_dir(player_root);
}

const char *files_player_dir(void)
{
    return player_root;
}

/* the parts of a game path: drive dropped, '\\' and '/' separators, empty and "." parts skipped */
static int split(const char *gpath, char parts[][256], int max)
{
    const char *s = gpath;
    if (isalpha((unsigned char) s[0]) && s[1] == ':') {
        s += 2;
    }
    int n = 0;
    while (*s) {
        while (*s == '\\' || *s == '/') {
            s++;
        }
        if (!*s) {
            break;
        }
        size_t k = 0;
        while (*s && *s != '\\' && *s != '/') {
            if (k < 255) {
                parts[n][k++] = *s;
            }
            s++;
        }
        parts[n][k] = 0;
        if (strcmp(parts[n], ".") == 0) {
            continue;
        }
        if (++n == max) {
            return -1;
        }
    }
    return n;
}

/* the entry of dir named `name` case-insensitively (an exact match preferred) into out; 0 or -1 */
static int lookup(const char *dir, const char *name, char *out, size_t size)
{
    char p[2048];
    snprintf(p, sizeof(p), "%s/%s", dir, name);
    struct stat sb;
    if (stat(p, &sb) == 0) {
        snprintf(out, size, "%s", p);
        return 0;
    }
    DIR *d = opendir(dir);
    if (!d) {
        return -1;
    }
    struct dirent *e;
    int found = -1;
    while ((e = readdir(d))) {
        if (strcasecmp(e->d_name, name) == 0) {
            snprintf(out, size, "%s/%s", dir, e->d_name);
            found = 0;
            break;
        }
    }
    closedir(d);
    return found;
}

/* resolve the first `n` parts under root; 0 and the host path, or -1 */
static int resolve_parts(const char *root, char parts[][256], int n, char *out, size_t size)
{
    char cur[2048];
    snprintf(cur, sizeof(cur), "%s", root);
    for (int i = 0; i < n; i++) {
        char next[2048];
        if (strcmp(parts[i], "..") == 0) {
            return -1; /* stay inside the roots */
        }
        if (lookup(cur, parts[i], next, sizeof(next)) != 0) {
            return -1;
        }
        snprintf(cur, sizeof(cur), "%s", next);
    }
    snprintf(out, size, "%s", cur);
    return 0;
}

int files_resolve(const char *gpath, char *out, size_t size)
{
    char parts[32][256];
    int n = gpath ? split(gpath, parts, 32) : -1;
    if (n <= 0) {
        return -1; /* an empty name is no file */
    }
    const char *roots[2] = {player_root, game_root};
    for (int r = 0; r < 2; r++) {
        struct stat sb;
        if (resolve_parts(roots[r], parts, n, out, size) == 0 && stat(out, &sb) == 0 && S_ISREG(sb.st_mode)) {
            return 0;
        }
    }
    return -1;
}

int32_t files_read(const char *gpath, uint32_t offset, uint8_t *buf, uint32_t size)
{
    char p[2048];
    if (files_resolve(gpath, p, sizeof(p)) != 0) {
        return -1;
    }
    FILE *f = fopen(p, "rb");
    if (!f) {
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (buf && size > 0 && (long) offset < len) {
        fseek(f, (long) offset, SEEK_SET);
        size_t want = (uint64_t) offset + size > (uint64_t) len ? (size_t) (len - (long) offset) : size;
        if (fread(buf, 1, want, f) != want) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return len > 0x7FFFFFFF ? -1 : (int32_t) len;
}

/* the host path for writing a game path in the player directory: existing parts keep their case,
 * missing directories are created (the original's own directory creation, if any, is not a request:
 * @unverified the game may rely on CreateDirectoryA calls of its own that are not ported) */
static int writable_path(const char *gpath, char *out, size_t size)
{
    char parts[32][256];
    int n = gpath ? split(gpath, parts, 32) : -1;
    if (n <= 0) {
        return -1;
    }
    char cur[2048];
    snprintf(cur, sizeof(cur), "%s", player_root);
    for (int i = 0; i < n; i++) {
        char next[2048];
        if (strcmp(parts[i], "..") == 0) {
            return -1;
        }
        if (lookup(cur, parts[i], next, sizeof(next)) != 0) {
            snprintf(next, sizeof(next), "%s/%s", cur, parts[i]);
            if (i < n - 1 && make_dir(next) != 0 && errno != EEXIST) {
                return -1;
            }
        }
        snprintf(cur, sizeof(cur), "%s", next);
    }
    snprintf(out, size, "%s", cur);
    return 0;
}

int32_t files_write(const char *gpath, const uint8_t *data, uint32_t size)
{
    char p[2048];
    if (writable_path(gpath, p, sizeof(p)) != 0) {
        return -1;
    }
    FILE *f = fopen(p, "wb");
    if (!f) {
        return -1;
    }
    size_t n = size ? fwrite(data, 1, size, f) : 0;
    if (fclose(f) != 0 || n != size) {
        return -1;
    }
    return (int32_t) size;
}

int32_t files_delete(const char *gpath)
{
    char p[2048];
    char parts[32][256];
    int n = gpath ? split(gpath, parts, 32) : -1;
    if (n <= 0 || resolve_parts(player_root, parts, n, p, sizeof(p)) != 0) {
        return -1;
    }
    return remove(p) == 0 ? 0 : -1;
}

int files_match(const char *pattern, const char *name)
{
    /* "*.*" (and a pattern ending in ".*") also matches a name without a dot, as Windows does */
    size_t pl = strlen(pattern);
    if (pl >= 2 && strcmp(pattern + pl - 2, ".*") == 0 && !strchr(name, '.')) {
        char p[256];
        snprintf(p, sizeof(p), "%.*s", (int) (pl - 2), pattern);
        return files_match(p, name);
    }
    const char *p = pattern, *s = name, *star = NULL, *mark = NULL;
    while (*s) {
        if (*p == '?' || (*p && *p != '*' && toupper((unsigned char) *p) == toupper((unsigned char) *s))) {
            p++;
            s++;
        } else if (*p == '*') {
            star = p++;
            mark = s;
        } else if (star) {
            p = star + 1;
            s = ++mark;
        } else {
            return 0;
        }
    }
    while (*p == '*') {
        p++;
    }
    return *p == 0;
}

/* NTFS lists a directory in the order of its index: names compared upper-cased (UTF-16 code units;
 * the game's names are ASCII). @unverified the order of FindNextFileA (docs/oracle.md) */
static int ntfs_cmp(const void *a, const void *b)
{
    const unsigned char *x = *(const unsigned char *const *) a, *y = *(const unsigned char *const *) b;
    while (*x && toupper(*x) == toupper(*y)) {
        x++;
        y++;
    }
    return toupper(*x) - toupper(*y);
}

static char last_pattern[1024];
static char **names;
static int nnames;

static void listing_clear(void)
{
    for (int i = 0; i < nnames; i++) {
        free(names[i]);
    }
    free(names);
    names = NULL;
    nnames = 0;
}

static void listing_build(const char *pattern)
{
    listing_clear();
    snprintf(last_pattern, sizeof(last_pattern), "%s", pattern);
    char parts[32][256];
    int n = split(pattern, parts, 32);
    if (n <= 0) {
        return;
    }
    const char *wild = parts[n - 1];
    const char *roots[2] = {player_root, game_root};
    int cap = 0;
    for (int r = 0; r < 2; r++) {
        char dir[2048];
        if (resolve_parts(roots[r], parts, n - 1, dir, sizeof(dir)) != 0) {
            continue;
        }
        DIR *d = opendir(dir);
        if (!d) {
            continue;
        }
        struct dirent *e;
        while ((e = readdir(d))) {
            if (!files_match(wild, e->d_name)) {
                continue;
            }
            int dup = 0;
            for (int i = 0; i < nnames && !dup; i++) {
                dup = strcasecmp(names[i], e->d_name) == 0;
            }
            if (dup) {
                continue;
            }
            if (nnames == cap) {
                cap = cap ? cap * 2 : 64;
                char **nn = realloc(names, (size_t) cap * sizeof(char *));
                if (!nn) {
                    break;
                }
                names = nn;
            }
            names[nnames++] = strdup(e->d_name);
        }
        closedir(d);
    }
    qsort(names, (size_t) nnames, sizeof(char *), ntfs_cmp);
}

int32_t files_list(const char *pattern, int32_t index, char *name, uint32_t size)
{
    if (!pattern || index < 0 || size == 0) {
        return -1;
    }
    if (index == 0 || strcmp(pattern, last_pattern) != 0) {
        listing_build(pattern);
    }
    if (index >= nnames) {
        return -1;
    }
    snprintf(name, size, "%s", names[index]);
    return (int32_t) strlen(name);
}
