#!/usr/bin/env python3
"""Record or verify SHA-256 hashes of the reference game installation.

  python3 tools/fingerprint.py --init    # record hashes into docs/source-fingerprint.json
  python3 tools/fingerprint.py           # verify the installation against the record

Game directory: $IMHOTEP_GAME_DIR (default: local/game/app).
Only hashes and relative paths are stored; no game content.
"""
import hashlib, json, os, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RECORD = ROOT / "docs" / "source-fingerprint.json"
GAME_DIR = Path(os.environ.get("IMHOTEP_GAME_DIR", ROOT / "local" / "game" / "app"))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def scan() -> dict:
    if not GAME_DIR.is_dir():
        sys.exit(f"Game directory not found: {GAME_DIR} (set IMHOTEP_GAME_DIR or run make extract)")
    files = {}
    for p in sorted(GAME_DIR.rglob("*")):
        if p.is_file():
            files[p.relative_to(GAME_DIR).as_posix()] = sha256(p)
    return files


def main() -> None:
    current = scan()
    if "--init" in sys.argv:
        RECORD.write_text(json.dumps(
            {"reference": "GOG Pharaoh + Cleopatra 2.1 (Pharaoh.exe 2.1.0.0)", "files": current}, indent=2) + "\n")
        print(f"Recorded {len(current)} files in {RECORD.relative_to(ROOT)}")
        return
    if not RECORD.exists():
        sys.exit("No fingerprint recorded yet. Run: python3 tools/fingerprint.py --init")
    expected = json.loads(RECORD.read_text())["files"]
    missing = sorted(set(expected) - set(current))
    extra = sorted(set(current) - set(expected))
    changed = sorted(k for k in expected.keys() & current.keys() if expected[k] != current[k])
    for label, items in (("MISSING", missing), ("CHANGED", changed), ("EXTRA", extra)):
        for i in items:
            print(f"{label}: {i}")
    if missing or changed:
        sys.exit("Installation does not match the reference fingerprint.")
    print(f"OK: {len(expected)} files match the reference" + (f" ({len(extra)} extra files ignored)" if extra else ""))


if __name__ == "__main__":
    main()
