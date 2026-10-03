#!/usr/bin/env python3
"""Print the ID of the next open item in docs/unverified.md (exit 1 when none).

Items listed in local/loop-skip.txt (one ID per line, e.g. failed in a previous run) are skipped.
"""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
doc = (ROOT / "docs" / "unverified.md").read_text()
skip_file = ROOT / "local" / "loop-skip.txt"
skip = set(skip_file.read_text().split()) if skip_file.exists() else set()

for line in doc.splitlines():
    m = re.match(r"\|\s*([A-Za-z0-9_-]+)\s*\|", line)
    if m and m.group(1) not in {"ID"} and m.group(1) not in skip and not line.startswith("|---"):
        print(m.group(1))
        sys.exit(0)
sys.exit(1)
