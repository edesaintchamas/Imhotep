/* Imhotep - access to the reference game files (file system side, not core/).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "game_files.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

const char *game_dir(void)
{
    const char *g = getenv("IMHOTEP_GAME_DIR");
    return g && *g ? g : "local/game/app";
}

uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = n >= 0 ? malloc(n > 0 ? (size_t) n : 1) : NULL;
    if (buf && fread(buf, 1, (size_t) n, f) != (size_t) n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    *len = n > 0 ? (size_t) n : 0;
    return buf;
}

int find_file(const char *dir, const char *stem, const char *ext, char *out, size_t out_len)
{
    char want[256];
    snprintf(want, sizeof(want), "%s%s", stem, ext);
    DIR *d = opendir(dir);
    if (!d) {
        return -1;
    }
    struct dirent *e;
    int found = -1;
    while ((e = readdir(d))) {
        if (strcasecmp(e->d_name, want) == 0) {
            snprintf(out, out_len, "%s/%s", dir, e->d_name);
            found = 0;
            break;
        }
    }
    closedir(d);
    return found;
}

int load_image_groups(const char *game, image_groups *groups)
{
    char data_dir[1024], path[1300];
    const uint8_t *headers[IMAGE_SLOT_COUNT] = {0};
    size_t lengths[IMAGE_SLOT_COUNT] = {0};
    int result = -1;
    snprintf(data_dir, sizeof(data_dir), "%s/Data", game);
    for (int s = 0; s < IMAGE_SLOT_COUNT; s++) {
        if (find_file(data_dir, image_startup_slots[s], ".sg3", path, sizeof(path)) != 0) {
            fprintf(stderr, "missing %s/%s.sg3\n", data_dir, image_startup_slots[s]);
            goto done;
        }
        if (!(headers[s] = read_file(path, &lengths[s]))) {
            fprintf(stderr, "cannot read %s\n", path);
            goto done;
        }
    }
    result = image_groups_build(groups, headers, lengths);
done:
    for (int s = 0; s < IMAGE_SLOT_COUNT; s++) {
        free((void *) headers[s]);
    }
    return result;
}

#define SG3_ENTRIES_OFFSET 40680 /* header 80 + groups 600 + bitmaps 200 x 200 (graphics.md §1) */
#define SG3_ENTRY_BYTES 64

int load_image_sizes(const char *game, const image_groups *groups, image_sizes *sizes)
{
    char data_dir[1024], path[1300];
    int32_t total = groups->slot_base[IMAGE_SLOT_COUNT - 1] + groups->slot_count[IMAGE_SLOT_COUNT - 1];
    sizes->count = total;
    sizes->width = calloc((size_t) total, sizeof(int16_t));
    sizes->height = calloc((size_t) total, sizeof(int16_t));
    if (!sizes->width || !sizes->height) {
        return -1;
    }
    snprintf(data_dir, sizeof(data_dir), "%s/Data", game);
    for (int s = 0; s < IMAGE_SLOT_COUNT; s++) {
        size_t len;
        uint8_t *f;
        if (find_file(data_dir, image_startup_slots[s], ".sg3", path, sizeof(path)) != 0 ||
            !(f = read_file(path, &len))) {
            return -1;
        }
        /* global id base + k is file entry k + skip + 1 (@bin 0x0053DFF0, graphics.md §10) */
        for (int32_t k = 0; k < groups->slot_count[s]; k++) {
            size_t e = SG3_ENTRIES_OFFSET + SG3_ENTRY_BYTES * (size_t) (k + groups->slot_skip[s] + 1);
            if (e + SG3_ENTRY_BYTES <= len) {
                sizes->width[groups->slot_base[s] + k] = (int16_t) (f[e + 20] | f[e + 21] << 8);
                sizes->height[groups->slot_base[s] + k] = (int16_t) (f[e + 22] | f[e + 23] << 8);
            }
        }
        free(f);
    }
    return 0;
}

void free_image_sizes(image_sizes *sizes)
{
    free(sizes->width);
    free(sizes->height);
    sizes->width = sizes->height = NULL;
}

int image_sizes_lookup(void *ctx, int32_t id, int16_t *width, int16_t *height)
{
    const image_sizes *s = ctx;
    if (id < 0 || id >= s->count) {
        return -1;
    }
    *width = s->width[id];
    *height = s->height[id];
    return 0;
}
