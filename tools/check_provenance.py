#!/usr/bin/env python3
"""Provenance report and checks for core/.

  python3 tools/check_provenance.py            # report + fail on untagged core files
  python3 tools/check_provenance.py --diff REF # also require a tag in every changed hunk vs REF

Tags: @bin, @diff (confirmed), @hint (hint), @unverified (unverified), @from (code origin),
@imhotep (a display extension that is not a rule of the original: docs/enhanced.md; never in the
simulation's directories, SIM_DIRS).
"""
import re, subprocess, sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CORE = ROOT / "core"
SRC_EXT = {".c", ".h", ".cpp", ".hpp", ".cc", ".hh"}
RULE_TAG = re.compile(r"@(bin|diff|hint|unverified|imhotep)\b")
SIM_DIRS = ("core/sim/tick", "core/city", "core/building", "core/figure", "core/map", "core/command")
FROM_TAG = re.compile(r"@from\b")
UNVERIFIED_DOC = ROOT / "docs" / "unverified.md"
# Lines that are not logic: blanks, comments, braces, includes, declarations-only noise.
NON_LOGIC = re.compile(r"^\s*($|//|/\*|\*|#include|#pragma|[{}]\s*;?$)")


def source_files():
    """core/ and present/ (the ported presentation code follows the same rules)"""
    present = CORE.parent / "present"
    return [p for d in (CORE, present) if d.is_dir() for p in d.rglob("*") if p.suffix in SRC_EXT]


def report() -> int:
    counts, untagged = Counter(), []
    for p in source_files():
        text = p.read_text(errors="replace")
        tags = RULE_TAG.findall(text)
        counts.update(tags)
        counts["from"] += len(FROM_TAG.findall(text))
        logic = [l for l in text.splitlines() if not NON_LOGIC.match(l)]
        if logic and not tags:
            untagged.append(p.relative_to(ROOT).as_posix())
    in_sim = [p.relative_to(ROOT).as_posix() for p in source_files()
              if p.relative_to(ROOT).as_posix().startswith(SIM_DIRS) and "@imhotep" in p.read_text(errors="replace")]
    confirmed = counts["bin"] + counts["diff"]
    total = confirmed + counts["hint"] + counts["unverified"]
    print("Provenance report (core/)")
    print(f"  confirmed : {confirmed}  (@bin {counts['bin']}, @diff {counts['diff']})")
    print(f"  hint      : {counts['hint']}")
    print(f"  unverified: {counts['unverified']}")
    print(f"  @from     : {counts['from']} (code origin, licence)")
    print(f"  @imhotep  : {counts['imhotep']} (display extensions, docs/enhanced.md; not rules)")
    if in_sim:
        print("FAIL: @imhotep (display extensions) in the simulation's code:")
        print("\n".join(f"  {u}" for u in in_sim))
        return 1
    if total:
        print(f"  confirmed ratio: {100 * confirmed / total:.1f}%")
    if counts["unverified"] and UNVERIFIED_DOC.exists():
        # Light check only: the doc must not be empty when unverified tags exist.
        if "_(none yet)_" in UNVERIFIED_DOC.read_text():
            print("FAIL: @unverified tags exist but docs/unverified.md has no entries")
            return 1
    if untagged:
        print("FAIL: core files with logic and no provenance tag:")
        print("\n".join(f"  {u}" for u in untagged))
        return 1
    return 0


def check_diff(ref: str) -> int:
    out = subprocess.run(["git", "diff", "-U0", ref, "--", "core/"],
                         capture_output=True, text=True, cwd=ROOT)
    if out.returncode != 0:
        print(out.stderr)
        return 1
    failures, hunk, current = [], [], None
    def flush():
        added = [l[1:] for l in hunk if l.startswith("+") and not l.startswith("+++")]
        logic = [l for l in added if not NON_LOGIC.match(l)]
        if logic and not any(RULE_TAG.search(l) for l in added):
            failures.append(f"{current}: {logic[0].strip()[:80]}")
    for line in out.stdout.splitlines():
        if line.startswith("+++ "):
            current = line[6:]
        elif line.startswith("@@"):
            if hunk:
                flush()
            hunk = []
        else:
            hunk.append(line)
    if hunk:
        flush()
    # New files not yet added to git are invisible to `git diff`: check them whole.
    untracked = subprocess.run(["git", "ls-files", "--others", "--exclude-standard", "core/"],
                               capture_output=True, text=True, cwd=ROOT).stdout.split()
    for f in untracked:
        if Path(f).suffix not in SRC_EXT:
            continue
        current = f
        hunk = ["+" + l for l in (ROOT / f).read_text(errors="replace").splitlines()]
        flush()
    if failures:
        print(f"FAIL: changed hunks without provenance tag (vs {ref}):")
        print("\n".join(f"  {f}" for f in failures))
        return 1
    print(f"OK: every changed hunk in core/ carries a provenance tag (vs {ref})")
    return 0


def main() -> None:
    code = report()
    if "--diff" in sys.argv:
        ref = sys.argv[sys.argv.index("--diff") + 1]
        code |= check_diff(ref)
    sys.exit(code)


if __name__ == "__main__":
    main()
