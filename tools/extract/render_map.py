#!/usr/bin/env python3
# Imhotep - render a Pharaoh .map from its image grid, using the original graphics.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""Render a scenario map to PNG (default orientation, no figures, no overlays).

  python3 tools/extract/render_map.py "Small Oasis"
  python3 tools/extract/render_map.py Alexandria --scale 0.25

Writes local/extract/maps/<name>/render.png. Tile images come from the core terrain rebuild
(core/map/terrain.c, run through build/terrain_dump, built on demand with `make tools`).
Format docs: docs/formats/map.md §6,
docs/formats/graphics.md. Every rule used here is tagged with where it was read in Pharaoh.exe.
"""
import argparse, json, os, struct, subprocess, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import extract_graphics as G  # noqa: E402
import read_map as M  # noqa: E402
from PIL import Image  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
GRID = 228

# @bin 0x00414420 startup: graphics families loaded into slots 0..29 in this order
# (FUN_0053dd10 / FUN_0053dd40 -> FUN_0053dd90(name, -1)). Missions may later reload a slot
# with another family at the same id base (FUN_0053dd90 with an explicit slot); not handled here.
STARTUP_SLOTS = [
    "Pharaoh_Unloaded", "SprMain", "Hyksos", "Pharaoh_General", "Pharaoh_Terrain",
    "SPHINX1A", "SPHINX1B", "SPHINX1C", "SPHINX1D", "SPHINX_EXTRA", "TEMPLE_NILE", "empire",
    "SprAmbient", "Pharaoh_Fonts", "Egyptian", "SprMain2", "expansion", "PyramidDummy",
    "CaesareumDummy", "LtHouseDummy", "LibMainDummy", "LibMainDummy", "LibMainDummy", "LibExtra",
    "MausCarryoverDummy", "TombThutDummy", "AbuSimbelDummy", "TombTutDummy", "TombSetiDummy",
    "TombRamDummy",
]
SYSTEM_IMAGES = 200

EDGE_DRAW_TILE = 0x40   # @bin 0x00430EF0 (and other city draw routines) test edge & 0x40
MIN_VISIBLE_ID = 6      # @bin 0x00430530 tiles with image id < 6 are left out of the view table
G_NEW_CLIFF = 743       # @bin 0x00430EF0 does not draw images 0x30..0x47 of this group

class ImageTable:
    """Global image id -> decoded image, following the loader's id arithmetic."""

    def __init__(self, data_dir: Path):
        self.data_dir = data_dir
        files = {p.stem.lower(): p for p in data_dir.glob("*.sg3")}
        # @bin 0x0053DB60: the image and group running totals both start at 1 (id 0 = no image)
        self.slots, base = [], 1
        for slot, name in enumerate(STARTUP_SLOTS):
            path = files[name.lower()]
            pak = G.parse_sg3(path.read_bytes())
            G.assign_offsets(pak["entries"])
            # @bin 0x0053DFF0: slots other than 0 skip the 200 system.bmp images; the entries
            # copied are file entries 1+skip .. last_index, to global ids base, base+1, ...
            skip = SYSTEM_IMAGES if slot > 0 and pak["bitmaps"] and \
                pak["bitmaps"][0]["name"].lower() == "system.bmp" else 0
            count = pak["header"][4] - skip
            self.slots.append({"slot": slot, "name": name, "path": path, "pak": pak,
                               "base": base, "count": count, "skip": skip, "data": None})
            base += count   # @bin 0x0053DD90 running total of entries
        self.cache = {}
        # @bin 0x0053DFF0: runtime group numbers start at 1 and run over the non-empty group slots
        # of each family in slot order (file group value - skip > 0); first image id per group.
        self.group_first, g = {}, 1
        for s in self.slots:
            values = struct.unpack_from("<300H", s["path"].read_bytes(), 80)
            for v in values:
                if v - s["skip"] > 0:
                    self.group_first[g] = s["base"] + v - s["skip"] - 1
                    g += 1

    def locate(self, gid: int):
        for s in self.slots:
            if s["base"] <= gid < s["base"] + s["count"]:
                return s, gid - s["base"] + s["skip"] + 1
        return None, None

    def image(self, gid: int):
        if gid in self.cache:
            return self.cache[gid]
        s, local = self.locate(gid)
        img = None
        if s is not None:
            e = s["pak"]["entries"][local]
            if e["mirror"]:   # @bin 0x0053E590: draw entry local+mirror flipped
                src = s["pak"]["entries"][local + e["mirror"]]
                img = self._decode(s, src)
                img = img.transpose(Image.FLIP_LEFT_RIGHT) if img else None
            else:
                img = self._decode(s, e)
        self.cache[gid] = img
        return img

    def _decode(self, s, e):
        if not e["length"]:
            return None
        if s["data"] is None:
            s["data"] = s["path"].with_suffix(".555").read_bytes()
        buf = G.load_pixels(e, s["pak"]["bitmaps"], s["data"], self.data_dir, {})
        img, _ = G.decode_entry(e, buf)
        return img


def screen_of(x: int, y: int) -> tuple:
    # @bin 0x00430530 (orientation 0): view row = 1 + x + y (15 px), half-column = 230 + x - y (30 px)
    return (230 + x - y) * 30, (1 + x + y) * 15


TERRAIN_DUMP = ROOT / "build" / "terrain_dump"


def rebuilt_grids(map_path: Path, work_dir: Path) -> tuple:
    """Run the core terrain rebuild (core/map/terrain.c, via build/terrain_dump) on a map and
    return (image ids, edge bytes, statistics)."""
    if not TERRAIN_DUMP.exists():
        subprocess.run(["make", "-s", "tools"], cwd=ROOT, check=True)
    out = work_dir / "terrain.imtd"
    env = dict(os.environ, IMHOTEP_GAME_DIR=str(G.GAME_DIR))
    run = subprocess.run([str(TERRAIN_DUMP), str(map_path), str(out)], env=env,
                         capture_output=True, text=True)
    if run.returncode != 0:
        sys.exit(f"terrain_dump failed: {run.stderr.strip()}")
    data = out.read_bytes()
    if data[:4] != b"IMTD" or struct.unpack_from("<I", data, 4)[0] != 1:
        sys.exit(f"{out}: unexpected terrain dump format")
    ids = struct.unpack_from(f"<{GRID * GRID}I", data, 8)
    edge = data[8 + 4 * GRID * GRID: 8 + 5 * GRID * GRID]
    return ids, edge, json.loads(run.stdout)


def render(map_path: Path, data_dir: Path, work_dir: Path, raw: bool = False) -> Image.Image:
    table = ImageTable(data_dir)
    if raw:   # the image grid stored in the file, as the map editor left it
        m = M.read_map(map_path.read_bytes())
        ids = struct.unpack(f"<{GRID * GRID}I", m["chunks"][4]["data"])
        edge = m["chunks"][5]["data"]
        render.stats = {}
    else:     # @bin 0x004D52A0: what the game shows at scenario start (core/map/terrain.c)
        ids, edge, render.stats = rebuilt_grids(map_path, work_dir)

    cliff = table.group_first[G_NEW_CLIFF]
    draws = []
    for off, gid in enumerate(ids):
        if gid < MIN_VISIBLE_ID or not edge[off] & EDGE_DRAW_TILE or cliff + 0x30 <= gid < cliff + 0x48:
            continue
        draws.append((off % GRID + off // GRID, off % GRID - off // GRID, off, gid))
    draws.sort()

    placed = []
    for _, _, off, gid in draws:
        img = table.image(gid)
        if img is None:
            continue
        n = max(1, (img.width + 2) // 60)
        # @bin 0x0047BF50: draw tile = block tile (0, n-1), so the block's top tile is (x, y-n+1)
        x0, y0 = off % GRID, off // GRID - (n - 1)
        tx, ty = screen_of(x0, y0)               # top tile of the footprint (centre x, top y)
        left = tx - 29 - (n - 1) * 30
        top = ty + 30 * n - img.height             # footprint sits at the bottom of the image
        placed.append((left, top, img))
    x_min = min(p[0] for p in placed); y_min = min(p[1] for p in placed)
    x_max = max(p[0] + p[2].width for p in placed); y_max = max(p[1] + p[2].height for p in placed)
    canvas = Image.new("RGBA", (x_max - x_min, y_max - y_min), (0, 0, 0, 255))
    for left, top, img in placed:
        canvas.alpha_composite(img, (left - x_min, top - y_min))
    return canvas


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("map")
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--raw", action="store_true", help="draw the image ids stored in the file instead of the core terrain rebuild")
    ap.add_argument("--out", default=str(ROOT / "local" / "extract" / "maps"))
    args = ap.parse_args()
    path = Path(args.map)
    if not path.suffix:
        path = path.with_suffix(".map")
    if not path.exists():
        path = G.GAME_DIR / "Maps" / path.name
    out_root = Path(args.out).resolve()
    if not out_root.is_relative_to((ROOT / "local").resolve()):
        sys.exit(f"Refusing to write game content outside local/: {out_root}")
    out = out_root / path.stem
    out.mkdir(parents=True, exist_ok=True)
    img = render(path, G.GAME_DIR / "Data", out, raw=args.raw)
    if args.scale != 1.0:
        img = img.resize((int(img.width * args.scale), int(img.height * args.scale)), Image.LANCZOS)
    name = "render_raw.png" if args.raw else "render.png"
    img.save(out / name)
    print(f"{path.name}: {img.width}x{img.height} -> {out / name}  {getattr(render, 'stats', {})}")


if __name__ == "__main__":
    main()
