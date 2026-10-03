#!/usr/bin/env python3
"""Fail if any file tracked (or staged) by git looks like game content.

Checks extensions known to belong to the game, and large binary files.
"""
import subprocess, sys
from pathlib import Path

BLOCKED_EXT = {
    ".exe", ".dll", ".sg2", ".sg3", ".555", ".map", ".pak", ".sav",
    ".bik", ".smk", ".wav", ".mp3", ".eng", ".inf", ".gpr",
}
BLOCKED_DIRS = ("local/", "refs/")
MAX_BINARY_BYTES = 512 * 1024


def tracked_files() -> list[str]:
    out = subprocess.run(["git", "ls-files", "--cached"], capture_output=True, text=True)
    if out.returncode != 0:
        sys.exit("Not a git repository (run git init first).")
    return [l for l in out.stdout.splitlines() if l]


def is_binary(path: Path) -> bool:
    try:
        return b"\0" in path.open("rb").read(8192)
    except OSError:
        return False


def main() -> None:
    problems = []
    for f in tracked_files():
        p = Path(f)
        if f.startswith(BLOCKED_DIRS):
            problems.append(f"{f}: inside a local-only directory")
        elif p.suffix.lower() in BLOCKED_EXT:
            problems.append(f"{f}: blocked extension {p.suffix}")
        elif p.exists() and p.stat().st_size > MAX_BINARY_BYTES and is_binary(p):
            problems.append(f"{f}: large binary file ({p.stat().st_size} bytes)")
    if problems:
        print("Possible game content tracked by git:")
        print("\n".join(f"  {x}" for x in problems))
        sys.exit(1)
    print("OK: no game content tracked")


if __name__ == "__main__":
    main()
