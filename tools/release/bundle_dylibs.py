#!/usr/bin/env python3
# Imhotep - copy every non-system dylib an executable needs into an app bundle (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/bundle_dylibs.py APP.app/Contents/MacOS/EXE APP.app/Contents/Frameworks
#
# Follows `otool -L` recursively from the executable; each library outside /usr/lib and /System
# (Homebrew's SDL2, FFmpeg and their dependencies) is copied into Frameworks, given the id
# @executable_path/../Frameworks/NAME, and every reference to it rewritten to that path
# (install_name_tool). @rpath, @loader_path and @executable_path references are resolved the way dyld
# does. Ends by checking that no binary in the bundle still refers to a library outside the system or
# the bundle, then prints the highest minimum macOS version among them (for Info.plist).
import os, re, shutil, subprocess, sys

SYSTEM = ("/usr/lib/", "/System/")
PREFIX = "@executable_path/../Frameworks/"


def run(*args):
    return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def deps(path):
    """the install names path links against (not its own id)"""
    lines = run("otool", "-L", path).splitlines()[1:]
    names = [l.strip().split(" (compatibility")[0] for l in lines if l.strip()]
    own = run("otool", "-D", path).splitlines()[1:]
    return [n for n in names if not own or n != own[0].strip()]


def rpaths(path):
    out, found = run("otool", "-l", path).splitlines(), []
    for i, l in enumerate(out):
        if l.strip() == "cmd LC_RPATH":
            m = re.search(r"path (.*) \(offset", out[i + 2])
            if m:
                found.append(m.group(1))
    return found


def resolve(name, loader, exe):
    def sub(p):
        return p.replace("@loader_path", os.path.dirname(loader)).replace("@executable_path", os.path.dirname(exe))

    if name.startswith("@rpath/"):
        for rp in rpaths(loader) + rpaths(exe):
            cand = os.path.join(sub(rp), name[len("@rpath/"):])
            if os.path.exists(cand):
                return os.path.realpath(cand)
        sys.exit(f"{loader}: cannot resolve {name}")
    cand = sub(name)
    if not os.path.exists(cand):
        sys.exit(f"{loader}: {name} not found")
    return os.path.realpath(cand)


def minos(path):
    out = run("otool", "-l", path).splitlines()
    for i, l in enumerate(out):
        if l.strip() == "cmd LC_BUILD_VERSION":
            for k in range(i, min(i + 6, len(out))):
                m = re.match(r"\s*minos (\S+)", out[k])
                if m:
                    return m.group(1)
        if l.strip() == "cmd LC_VERSION_MIN_MACOSX":
            m = re.match(r"\s*version (\S+)", out[i + 2])
            if m:
                return m.group(1)
    return "0"


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: bundle_dylibs.py EXE FRAMEWORKS_DIR")
    exe, fw = sys.argv[1], sys.argv[2]
    os.makedirs(fw, exist_ok=True)
    copied = {}  # real source path -> bundled name
    todo = [(exe, exe)]  # (binary in the bundle, its original location for @loader_path)
    while todo:
        binary, origin = todo.pop()
        for name in deps(binary):
            if name.startswith(SYSTEM):
                continue
            if name.startswith(PREFIX) and os.path.exists(os.path.join(fw, name[len(PREFIX):])):
                continue
            src = resolve(name, origin, exe)
            base = os.path.basename(src)
            if src not in copied:
                if base in copied.values():
                    sys.exit(f"two libraries named {base}")
                dst = os.path.join(fw, base)
                shutil.copy2(src, dst)
                os.chmod(dst, 0o755)
                run("install_name_tool", "-id", PREFIX + base, dst)
                copied[src] = base
                todo.append((dst, src))
            run("install_name_tool", "-change", name, PREFIX + base, binary)
    binaries = [exe] + [os.path.join(fw, b) for b in sorted(copied.values())]
    for b in binaries:  # Homebrew's rpaths point outside the bundle: drop them
        for rp in rpaths(b):
            if not rp.startswith(("@executable_path", "@loader_path")):
                run("install_name_tool", "-delete_rpath", rp, b)
    bad = []
    for b in binaries:
        for name in deps(b):
            if not name.startswith(SYSTEM) and not (name.startswith(PREFIX)
                                                    and os.path.exists(os.path.join(fw, name[len(PREFIX):]))):
                bad.append(f"{b}: {name}")
    if bad:
        sys.exit("references outside the bundle:\n  " + "\n  ".join(bad))
    for b in binaries:  # install_name_tool invalidated the signatures; arm64 requires one (ad hoc)
        run("codesign", "--force", "--sign", "-", b)
    versions = [minos(b) for b in binaries]
    print(f"bundled {len(copied)} libraries", file=sys.stderr)
    print(max(versions, key=lambda v: tuple(int(x) for x in v.split("."))))


if __name__ == "__main__":
    main()
