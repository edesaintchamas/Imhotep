#!/usr/bin/env python3
# Imhotep - check that a release package holds nothing from the game (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/check_package.py DIR
#
# Fails when a file under DIR
#   - has the content (SHA-256) or the name of any file of the reference installation
#     (docs/source-fingerprint.json lists all 1,335 of them);
#   - has an extension of game content (graphics, sounds, videos, maps, saves, text resources);
#   - is an .exe other than imhotep.exe (the DLLs a Windows package carries are SDL2's, FFmpeg's and
#     their dependencies; a game DLL is caught by its name and content above).
import hashlib, json, sys
from pathlib import Path

GAME_EXT = {".sg2", ".sg3", ".555", ".map", ".pak", ".sav", ".bik", ".smk", ".wav", ".mp3",
            ".eng", ".inf", ".emp", ".m3d", ".tsk", ".asi", ".hashdb", ".gpr"}


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: check_package.py DIR")
    root = Path(sys.argv[1])
    ref = json.loads((Path(__file__).resolve().parents[2] / "docs/source-fingerprint.json").read_text())["files"]
    ref_hashes = set(ref.values())
    ref_names = {Path(k).name.lower() for k in ref}
    problems, count = [], 0
    for p in sorted(root.rglob("*")):
        if p.is_symlink() or not p.is_file():
            continue
        count += 1
        rel = p.relative_to(root)
        name = p.name.lower()
        if hashlib.sha256(p.read_bytes()).hexdigest() in ref_hashes:
            problems.append(f"{rel}: same content as a file of the game")
        elif name in ref_names and name not in ("readme.txt", "license.txt"):
            problems.append(f"{rel}: same name as a file of the game")
        elif p.suffix.lower() in GAME_EXT:
            problems.append(f"{rel}: game content extension {p.suffix}")
        elif p.suffix.lower() == ".exe" and name != "imhotep.exe":
            problems.append(f"{rel}: an executable other than imhotep.exe")
    if problems:
        print("Possible game content in the package:")
        print("\n".join(f"  {x}" for x in problems))
        sys.exit(1)
    if count == 0:
        sys.exit(f"{root}: no files")
    print(f"OK: {count} files, nothing from the game")


if __name__ == "__main__":
    main()
