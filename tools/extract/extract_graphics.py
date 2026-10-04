#!/usr/bin/env python3
# Imhotep - Pharaoh graphics extractor (.sg3 index + .555 pixel data -> PNG).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
# Portions of the pixel decoding are translated from Akhenaten
# (https://github.com/dalerank/Akhenaten, AGPL-3.0, dalerank and contributors,
# based on Ozymandias by Banderi) and Julius (https://github.com/bvschaik/julius,
# AGPL-3.0, Bianca van Schaik and contributors). Akhenaten's source file carries
# no per-file copyright notice; the project-level notice above applies.
# Translated parts are tagged @from below.
"""Extract one Pharaoh graphics family to PNG.

  python3 tools/extract/extract_graphics.py Pharaoh_General
  python3 tools/extract/extract_graphics.py Data/mausoleum0_extra.sg3 --out local/extract

Reads <game>/Data/<name>.sg3 and its pixel data, and writes
local/extract/<name>/<index>.png plus local/extract/<name>/manifest.json.
<index> is the entry index in the .sg3 (entry 0 is always empty).

Format: docs/formats/graphics.md. This tool is strict: any inconsistency in the
data (overrun, leftover bytes, out-of-bounds pixel) is reported in the manifest
and summarised on stdout instead of being silently patched.

Game directory: $IMHOTEP_GAME_DIR (default: local/game/app). Output must stay
under local/ (game content never leaves local/).
"""
import argparse, json, os, struct, sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: python3 -m pip install Pillow")

ROOT = Path(__file__).resolve().parents[2]
GAME_DIR = Path(os.environ.get("IMHOTEP_GAME_DIR", ROOT / "local" / "game" / "app"))

# --- .sg3 layout (docs/formats/graphics.md) ---
HEADER_INTS = 20                 # 80 bytes
GROUP_SLOTS = 300                # u16 first-image index per group
BITMAP_RECORD = 200
BITMAP_SLOTS = 200
ENTRIES_OFFSET = 80 + 2 * GROUP_SLOTS + BITMAP_SLOTS * BITMAP_RECORD   # 40680
ENTRY_SIZE = 64
GROUP_NAME_SIZE = 48

TRANSPARENT_KEY = 0xF81F         # @bin 0x0040FFD0 plain blitter skips it; footprints are opaque (@bin 0x00427A20)
TYPE_ISOMETRIC = 30
TILE_W, TILE_H = 58, 30          # isometric footprint tile
TILE_BYTES = 1800                # 900 pixels * 2


class DecodeError(Exception):
    pass


def cstr(b: bytes) -> str:
    return b.split(b"\0", 1)[0].decode("latin-1")


def parse_sg3(data: bytes) -> dict:
    header = struct.unpack_from(f"<{HEADER_INTS}I", data, 0)
    max_entries, last_index, num_bitmaps = header[3], header[4], header[5]
    expected = ENTRIES_OFFSET + max_entries * ENTRY_SIZE + GROUP_SLOTS * GROUP_NAME_SIZE
    if len(data) != expected:
        raise DecodeError(f".sg3 size {len(data)} != expected {expected} for max_entries={max_entries}")

    group_first = struct.unpack_from(f"<{GROUP_SLOTS}H", data, 80)
    names_off = ENTRIES_OFFSET + max_entries * ENTRY_SIZE
    groups = []
    for g in range(GROUP_SLOTS):
        name = cstr(data[names_off + g * GROUP_NAME_SIZE: names_off + (g + 1) * GROUP_NAME_SIZE])
        if g == 0 or group_first[g] or name:
            groups.append({"group": g, "first_index": group_first[g], "name": name})

    bitmaps = []
    for b in range(num_bitmaps):
        o = 80 + 2 * GROUP_SLOTS + b * BITMAP_RECORD
        ints = struct.unpack_from("<21I", data, o + 116)
        bitmaps.append({
            "id": b,
            "name": cstr(data[o:o + 65]),
            "comment": cstr(data[o + 65:o + 116]),
            "ints": list(ints),           # meaning partly known, see graphics.md
        })

    entries = []
    for i in range(last_index + 1):
        raw = data[ENTRIES_OFFSET + i * ENTRY_SIZE: ENTRIES_OFFSET + (i + 1) * ENTRY_SIZE]
        offset, length, foot_length, unk12, mirror = struct.unpack_from("<5i", raw, 0)
        width, height = struct.unpack_from("<2h", raw, 20)
        unk24, unk26 = struct.unpack_from("<2H", raw, 24)
        unk28, num_sprites, unk32, sprite_x, sprite_y = struct.unpack_from("<hHhhh", raw, 28)
        entries.append({
            "index": i,
            "offset": offset, "length": length, "uncompressed_length": foot_length,
            "mirror": mirror,
            "width": width, "height": height,
            "num_sprites": num_sprites, "sprite_offset": [sprite_x, sprite_y],
            "anim_flag48": raw[48],
            "type": raw[50], "compressed": raw[51], "external": raw[52], "has_top": raw[53],
            "tile_size55": raw[55],
            "bitmap_id": raw[56],
            "anim_speed58": raw[58],
            "unknown": {"u12": unk12, "u24": unk24, "u26": unk26, "u28": unk28, "u32": unk32,
                        "b38_47": raw[38:48].hex(), "b49": raw[49], "b54": raw[54],
                        "b57": raw[57], "b59_63": raw[59:64].hex()},
        })
    return {"header": list(header), "groups": groups, "bitmaps": bitmaps, "entries": entries}


def rgb555(v: int) -> tuple:
    # @from julius:src/core/image.c to_32_bit (5-bit -> 8-bit expansion: c<<3 | c>>2)
    r, g, b = (v >> 10) & 31, (v >> 5) & 31, v & 31
    return ((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), 255)


def decode_plain(buf: bytes, w: int, h: int, px: list) -> None:
    if len(buf) != 2 * w * h:
        raise DecodeError(f"plain: {len(buf)} bytes for {w}x{h}")
    for k, v in enumerate(struct.unpack(f"<{w * h}H", buf)):
        px[k] = (0, 0, 0, 0) if v == TRANSPARENT_KEY else rgb555(v)


def decode_rle(buf: bytes, w: int, h: int, px: list) -> int:
    # @from akhenaten:src/content/imagepak.cpp convert_compressed
    # @from julius:src/core/image.c convert_compressed
    # 0xFF n  -> skip n transparent pixels; c (<0xFF) -> c raw u16 pixels follow.
    # Returns the number of pixel positions covered.
    pos, i, n = 0, 0, len(buf)
    while i < n:
        c = buf[i]
        if c == 0xFF:
            if i + 1 >= n:
                raise DecodeError("rle: truncated skip")
            pos += buf[i + 1]
            i += 2
        else:
            if i + 1 + 2 * c > n:
                raise DecodeError("rle: truncated run")
            if pos + c > w * h:
                raise DecodeError(f"rle: run past end of image ({pos + c} > {w * h})")
            for v in struct.unpack_from(f"<{c}H", buf, i + 1):
                px[pos] = rgb555(v)
                pos += 1
            i += 1 + 2 * c
    if pos > w * h:
        raise DecodeError(f"rle: skip past end of image ({pos} > {w * h})")
    return pos


# @from akhenaten:src/content/imagepak.cpp FOOTPRINT_X_START_PER_HEIGHT
FOOT_X_START = [28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0,
                0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28]


def decode_foot(buf: bytes, w: int, h: int, px: list) -> None:
    # @from akhenaten:src/content/imagepak.cpp convert_isometric_footprint / convert_footprint_tile
    # n x n diamond of 58x30 tiles, stored row by row from the top tile; tiles on a
    # row are 60 px apart. Footprint sits at the bottom of the image.
    n = (w + 2) // 60
    if len(buf) != TILE_BYTES * n * n:
        raise DecodeError(f"foot: {len(buf)} bytes for n={n}")
    vals = struct.unpack(f"<{len(buf) // 2}H", buf)
    k = 0
    x_start = (n - 1) * TILE_H
    y_base = h - TILE_H * n
    # Upper half: rows with 1..n tiles; lower half: rows with n-1..1 tiles.
    rows = [(i, i) for i in range(n)] + [(i, 2 * n - i - 2) for i in range(n - 2, -1, -1)]
    for i, row_y in rows:
        x = -TILE_H * i + x_start
        y = 15 * row_y + y_base
        for _ in range(i + 1):
            for ty in range(TILE_H):
                for tx in range(FOOT_X_START[ty], TILE_W - FOOT_X_START[ty]):
                    X, Y = x + tx, y + ty
                    if not (0 <= X < w and 0 <= Y < h):
                        raise DecodeError(f"foot: pixel ({X},{Y}) outside {w}x{h}")
                    px[Y * w + X] = rgb555(vals[k])
                    k += 1
            x += TILE_W + 2


def load_pixels(e: dict, bitmaps: list, data555: bytes, data_dir: Path, ext_cache: dict) -> bytes:
    if e["external"]:
        name = Path(bitmaps[e["bitmap_id"]]["name"]).stem.lower() + ".555"
        if name not in ext_cache:
            match = [p for p in data_dir.iterdir() if p.name.lower() == name]
            if not match:
                raise DecodeError(f"external file {name} not found")
            ext_cache[name] = match[0].read_bytes()
        src, start = ext_cache[name], e["offset"] - 1        # external offsets are 1-based
    else:
        src, start = data555, e["data_offset"]                # recomputed, see assign_offsets
    if start < 0 or start + e["length"] > len(src):
        raise DecodeError(f"data range {start}+{e['length']} outside file of {len(src)} bytes")
    return src[start:start + e["length"]]


def assign_offsets(entries: list) -> None:
    # @bin 0x0053DFF0 sg3_load_index: the stored offset of internal entries is ignored; data is laid
    # out in entry order starting 4 bytes into the .555. (Fixes the *Dummy.sg3 files, whose stored
    # offsets are 0.)
    pos = 4
    for e in entries[1:]:
        if not e["external"]:
            e["data_offset"] = pos
            pos += e["length"]


def decode_entry(e: dict, buf: bytes) -> tuple:
    w, h = e["width"], e["height"]
    px = [(0, 0, 0, 0)] * (w * h)
    if e["type"] == TYPE_ISOMETRIC:
        foot = e["uncompressed_length"]
        decode_foot(buf[:foot], w, h, px)
        if e["has_top"]:
            decode_rle(buf[foot:], w, h, px)   # top drawn over the foot, from (0,0)
        method = "isometric+top" if e["has_top"] else "isometric"
    elif e["compressed"]:
        decode_rle(buf, w, h, px)
        method = "rle"
    else:
        decode_plain(buf, w, h, px)
        method = "plain"
    img = Image.new("RGBA", (w, h))
    img.putdata(px)
    return img, method


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", help="graphics family name or path to a .sg3 (e.g. Pharaoh_General)")
    ap.add_argument("--out", default=str(ROOT / "local" / "extract"))
    ap.add_argument("--render-mirrors", action="store_true",
                    help="also write mirror entries as a horizontal flip of their target (@bin 0x0053E590)")
    args = ap.parse_args()

    sg3 = Path(args.file)
    if not sg3.suffix:
        sg3 = sg3.with_suffix(".sg3")
    if not sg3.exists():
        sg3 = GAME_DIR / "Data" / sg3.name
    if not sg3.exists():
        sys.exit(f"Not found: {sg3}")
    out_root = Path(args.out).resolve()
    if not out_root.is_relative_to((ROOT / "local").resolve()):
        sys.exit(f"Refusing to write game content outside local/: {out_root}")

    data_dir = sg3.parent
    pak = parse_sg3(sg3.read_bytes())
    data555 = sg3.with_suffix(".555").read_bytes()
    out = out_root / sg3.stem
    out.mkdir(parents=True, exist_ok=True)

    assign_offsets(pak["entries"])
    counts, errors, ext_cache, images = {}, [], {}, {}
    for e in pak["entries"]:
        if e["length"] == 0:
            e["status"] = "mirror" if e["mirror"] else "empty"
            if e["mirror"]:
                e["mirror_of"] = e["index"] + e["mirror"]
            counts[e["status"]] = counts.get(e["status"], 0) + 1
            continue
        try:
            img, method = decode_entry(e, load_pixels(e, pak["bitmaps"], data555, data_dir, ext_cache))
        except DecodeError as ex:
            e["status"] = "error"
            e["error"] = str(ex)
            errors.append((e["index"], str(ex)))
            counts["error"] = counts.get("error", 0) + 1
            continue
        e["status"] = method
        e["png"] = f"{e['index']}.png"
        img.save(out / e["png"])
        images[e["index"]] = img
        counts[method] = counts.get(method, 0) + 1

    if args.render_mirrors:
        for e in pak["entries"]:
            if e["status"] == "mirror" and e["mirror_of"] in images:
                images[e["mirror_of"]].transpose(Image.FLIP_LEFT_RIGHT).save(out / f"{e['index']}.png")
                e["png"] = f"{e['index']}.png"
                e["png_note"] = "horizontal flip of mirror_of (@bin 0x0053E590)"
                counts["mirror rendered"] = counts.get("mirror rendered", 0) + 1

    manifest = {
        "source": sg3.name,
        "pixel_data": sg3.with_suffix(".555").name,
        "format_doc": "docs/formats/graphics.md",
        "header": pak["header"],
        "groups": pak["groups"],
        "bitmaps": pak["bitmaps"],
        "entries": pak["entries"],
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n")

    print(f"{sg3.name}: {len(pak['entries']) - 1} entries -> {out}")
    for k, v in sorted(counts.items()):
        print(f"  {k:15} {v}")
    for idx, msg in errors[:20]:
        print(f"  ERROR entry {idx}: {msg}")
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
