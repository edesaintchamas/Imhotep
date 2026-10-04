#!/usr/bin/env python3
# Imhotep - dump Pharaoh's text resource (Pharaoh_Text.eng). Format: docs/formats/text.md.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""Dump the text groups of Pharaoh_Text.eng.

  python3 tools/extract/read_text.py            # all groups -> local/extract/text/groups.json
  python3 tools/extract/read_text.py 41 64      # print groups 41 and 64 (index: string)
  python3 tools/extract/read_text.py --messages # Pharaoh_MM.eng -> local/extract/text/messages.json

Game text stays under local/.
"""
import json, os, struct, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1].parent
GAME_DIR = Path(os.environ.get("IMHOTEP_GAME_DIR", ROOT / "local" / "game" / "app"))
HEADER, ENTRIES = 28, 1000
MM_HEADER, MM_ENTRY = 24, 80            # 0x004CEC40 reads 0x13898 = 24 + 1000 * 80 bytes first
# entry fields read by the game (0x004C6C70, 0x004E0CF0, 0x004E1A50): offsets into the data
MM_TEXTS = {"video": 60, "title": 68, "content": 76}


def read_text(path: Path) -> dict:
    d = path.read_bytes()
    if d[:16] != b"Pharaoh textfile":
        sys.exit(f"{path}: bad magic")
    index = [struct.unpack_from("<2I", d, HEADER + 8 * i) for i in range(ENTRIES)]
    data = d[HEADER + 8 * ENTRIES:]
    offsets = sorted(o for o, used in index if used)
    groups = {}
    for g, (o, used) in enumerate(index):
        if not used:
            continue
        end = min([x for x in offsets if x > o] + [len(data)])
        # 0x004CCBB0: strings are NUL-terminated; a NUL right after a control byte (1..31) would
        # not end a string, but the reference file has no such case (docs/formats/text.md).
        groups[g] = [s.decode("cp1252") for s in data[o:end].split(b"\0")[:-1]]
    return groups


def read_messages(path: Path) -> list:
    d = path.read_bytes()
    if d[:16] != b"Pharaoh MM file.":
        sys.exit(f"{path}: bad magic")
    count, used = struct.unpack_from("<2I", d, 16)
    data = d[MM_HEADER + MM_ENTRY * count:]
    out = []
    for i in range(count):
        e = d[MM_HEADER + MM_ENTRY * i: MM_HEADER + MM_ENTRY * (i + 1)]
        if not any(e):
            continue
        m = {"id": i, "type": struct.unpack_from("<h", e, 0)[0], "message_type": struct.unpack_from("<h", e, 2)[0],
             "width_blocks": struct.unpack_from("<h", e, 10)[0], "height_blocks": struct.unpack_from("<h", e, 12)[0],
             "urgent": struct.unpack_from("<i", e, 56)[0]}
        for name, at in MM_TEXTS.items():
            o = struct.unpack_from("<i", e, at)[0]
            if o:
                m[name] = data[o: data.index(b"\0", o)].decode("cp1252")
        out.append(m)
    if len(out) != used:
        print(f"warning: {len(out)} non-empty entries, header says {used}", file=sys.stderr)
    return out


def main() -> None:
    if sys.argv[1:] == ["--messages"]:
        msgs = read_messages(GAME_DIR / "Pharaoh_MM.eng")
        out = ROOT / "local" / "extract" / "text"
        out.mkdir(parents=True, exist_ok=True)
        (out / "messages.json").write_text(json.dumps(msgs, indent=1, ensure_ascii=False))
        print(f"{len(msgs)} messages -> {out / 'messages.json'}")
        return
    groups = read_text(GAME_DIR / "Pharaoh_Text.eng")
    if len(sys.argv) > 1:
        for g in map(int, sys.argv[1:]):
            for i, s in enumerate(groups.get(g, [])):
                print(f"{g}:{i}\t{s}")
        return
    out = ROOT / "local" / "extract" / "text"
    out.mkdir(parents=True, exist_ok=True)
    (out / "groups.json").write_text(json.dumps(groups, indent=1, ensure_ascii=False))
    print(f"{len(groups)} groups, {sum(map(len, groups.values()))} strings -> {out / 'groups.json'}")


if __name__ == "__main__":
    main()
