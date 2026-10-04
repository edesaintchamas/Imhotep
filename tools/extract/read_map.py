#!/usr/bin/env python3
# Imhotep - Pharaoh .map reader (container + chunks), for format analysis.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""Read a Pharaoh scenario map (.map) and dump its chunks.

  python3 tools/extract/read_map.py "Small Oasis"
  python3 tools/extract/read_map.py local/game/app/Maps/Default.map --chunks --png

Writes local/extract/maps/<name>/summary.json, and optionally every decompressed chunk
(--chunks, as NN_<name>.bin) and grid previews (--png). Format: docs/formats/map.md.

The container is self-describing (chunk table at the start of the file); this tool walks it,
checks that the walk ends exactly at end of file, and decompresses PKWARE chunks, failing loudly
on any mismatch. Chunk names are hints from Akhenaten, not confirmed meanings.
"""
import argparse, json, os, struct, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pkware  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
GAME_DIR = Path(os.environ.get("IMHOTEP_GAME_DIR", ROOT / "local" / "game" / "app"))

GRID = 228                      # grid side, all maps (confirmed: 51984 = 228^2 element grids)
UNCOMPRESSED_PREFIX = 0x80000000  # @bin 0x0055BCF0 raw bytes follow; stored size 0 = zero-filled

# @hint akhenaten:src/io/gamestate/file_schema_map.cpp (names only; sizes come from the file)
CHUNK_NAMES = [
    "magic", "file_version", "schema_count", "schema",
    "image_grid", "edge_grid", "terrain_grid", "bitfields_grid", "random_grid", "elevation_grid",
    "random_iv_0", "random_iv_1", "camera_x", "camera_y",
    "scenario_info", "soil_fertility_grid", "scenario_events", "junk11",
    "empire_map_objects", "empire_map_routes", "vegetation_growth_grid",
    "floodplain_settings", "trade_prices", "moisture_grid",
]


class MapError(Exception):
    pass


def read_map(data: bytes) -> dict:
    if data[:4] != b"MAPS":
        raise MapError(f"bad magic {data[:4]!r}")
    version, count = struct.unpack_from("<II", data, 4)
    schema = [struct.unpack_from("<5I", data, 12 + 20 * i) for i in range(count)]
    chunks, pos = [], 0
    for i, (compressed, addr, elem_size, elem_count, zero) in enumerate(schema):
        size = elem_size * elem_count
        entry = {"index": i, "name": CHUNK_NAMES[i] if i < len(CHUNK_NAMES) else f"chunk{i}",
                 "compressed": compressed, "writer_address": f"0x{addr:08X}",
                 "elem_size": elem_size, "elem_count": elem_count, "size": size,
                 "file_offset": pos, "schema_last": zero}
        if compressed:
            stored = struct.unpack_from("<I", data, pos)[0]
            if stored == 0:
                body, pos = bytes(size), pos + 4
            elif stored == UNCOMPRESSED_PREFIX:
                body, pos = data[pos + 4:pos + 4 + size], pos + 4 + size
            else:
                try:
                    body = pkware.explode(data[pos + 4:pos + 4 + stored], size)
                except pkware.PkError as ex:
                    raise MapError(f"chunk {i}: {ex}")
                pos += 4 + stored
            entry["stored_size"] = stored
        else:
            body, pos = data[pos:pos + size], pos + size
        if len(body) != size:
            raise MapError(f"chunk {i}: truncated ({len(body)} of {size} bytes)")
        entry["data"] = body
        chunks.append(entry)
    if pos != len(data):
        raise MapError(f"chunk walk ended at {pos}, file is {len(data)} bytes")
    return {"version": version, "chunks": chunks}


def scenario_geometry(info: bytes) -> dict:
    # Offsets 44..59 of scenario_info: @hint akhenaten:src/scenario/scenario.cpp iob_scenario_info.
    # Consistency checked on all 33 maps (docs/formats/map.md).
    w, h, start, border = struct.unpack_from("<4i", info, 44)
    return {"width": w, "height": h, "start_offset": start, "border_size": border}


def grid_png(chunk: dict, path: Path) -> None:
    from PIL import Image
    n = chunk["elem_size"]
    fmt = {1: "B", 2: "H", 4: "I"}[n]
    vals = struct.unpack(f"<{GRID * GRID}{fmt}", chunk["data"])
    distinct = sorted(set(vals))
    if len(distinct) <= 256:
        # categorical: stable colour per distinct value
        lut = {v: ((v * 2654435761) >> 8 & 0xFF, (v * 2246822519) >> 12 & 0xFF, (v * 3266489917) >> 16 & 0xFF)
               for v in distinct}
        lut[0] = (0, 0, 0)
        px = [lut[v] for v in vals]
    else:
        lo, hi = distinct[0], distinct[-1]
        px = [(int(255 * (v - lo) / (hi - lo)),) * 3 for v in vals]
    img = Image.new("RGB", (GRID, GRID))
    img.putdata(px)
    img.resize((GRID * 3, GRID * 3), Image.NEAREST).save(path)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("map", help="map name or path (e.g. 'Small Oasis')")
    ap.add_argument("--out", default=str(ROOT / "local" / "extract" / "maps"))
    ap.add_argument("--chunks", action="store_true", help="write each decompressed chunk as .bin")
    ap.add_argument("--png", action="store_true", help="write a preview of each 228x228 grid")
    args = ap.parse_args()

    path = Path(args.map)
    if not path.suffix:
        path = path.with_suffix(".map")
    if not path.exists():
        path = GAME_DIR / "Maps" / path.name
    if not path.exists():
        sys.exit(f"Not found: {path}")
    out_root = Path(args.out).resolve()
    if not out_root.is_relative_to((ROOT / "local").resolve()):
        sys.exit(f"Refusing to write game content outside local/: {out_root}")

    m = read_map(path.read_bytes())
    out = out_root / path.stem
    out.mkdir(parents=True, exist_ok=True)
    geom = scenario_geometry(m["chunks"][14]["data"])
    for c in m["chunks"]:
        if args.chunks:
            (out / f"{c['index']:02d}_{c['name']}.bin").write_bytes(c["data"])
        if args.png and c["elem_count"] == GRID * GRID:
            grid_png(c, out / f"{c['index']:02d}_{c['name']}.png")
    summary = {
        "source": path.name, "format_doc": "docs/formats/map.md",
        "version": m["version"], "geometry": geom,
        "chunks": [{k: v for k, v in c.items() if k != "data"} for c in m["chunks"]],
    }
    (out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")
    print(f"{path.name}: version {m['version']}, {len(m['chunks'])} chunks, "
          f"{geom['width']}x{geom['height']} -> {out}")


if __name__ == "__main__":
    main()
