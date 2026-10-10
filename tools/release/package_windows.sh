#!/bin/bash
# Imhotep - the Windows package: a zip with imhotep.exe and the DLLs it needs (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/package_windows.sh VERSION EXE OUTDIR      (in an MSYS2 UCRT64 shell)
#
# EXE: build/imhotep.exe built with IMHOTEP_VERSION=VERSION (make game), FFmpeg linked statically
# (tools/release/build_deps.sh). The DLLs are those `ldd` finds in the MSYS2 environment
# (/ucrt64/bin: SDL2, and whatever else the executable links there), followed from
# each copied DLL until none is missing; Windows' own DLLs are left out. Writes
# OUTDIR/imhotep-VERSION-windows-x64.zip: imhotep-VERSION-windows-x64/{imhotep.exe, *.dll, README.md,
# LICENSE.txt, licenses/}. LICENSES: FFmpeg's licence (default build-deps/share/licenses); SDL2's comes
# from MSYS2.
set -euo pipefail
[ $# = 3 ] || { echo "usage: $0 VERSION EXE OUTDIR"; exit 1; }
version=$1 exe=$2 out=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
prefix=${MINGW_PREFIX:-/ucrt64}
name=imhotep-$version-windows-x64
mkdir -p "$out"
out=$(cd "$out" && pwd)
stage=$(mktemp -d)/$name
mkdir -p "$stage"
cp "$exe" "$stage/imhotep.exe"

# the environment's DLLs a binary needs (ldd lists them recursively; the loop makes sure)
needed() { ldd "$1" | awk -v p="$prefix/bin/" 'index($3, p) == 1 { print $3 }'; }
todo=("$stage/imhotep.exe")
while [ ${#todo[@]} -gt 0 ]; do
    bin=${todo[0]}
    todo=("${todo[@]:1}")
    for dll in $(needed "$bin"); do
        base=$(basename "$dll")
        if [ ! -e "$stage/$base" ]; then
            cp "$dll" "$stage/$base"
            todo+=("$stage/$base")
        fi
    done
done
echo "$(ls "$stage" | grep -ci '\.dll$') DLLs"

cp "$root/README.md" "$stage/README.md"
cp "$root/LICENSE" "$stage/LICENSE.txt"
cp -R "${LICENSES:-$root/build-deps/share/licenses}" "$stage/licenses"
for dll in "$stage"/*.dll; do  # the licences of the DLLs' MSYS2 packages
    pkg=$(pacman -Qqo "$prefix/bin/$(basename "$dll")" 2>/dev/null || true)
    if [ -n "$pkg" ] && [ -d "$prefix/share/licenses/${pkg#mingw-w64-ucrt-x86_64-}" ]; then
        cp -R "$prefix/share/licenses/${pkg#mingw-w64-ucrt-x86_64-}" "$stage/licenses/"
    fi
done
python3 "$root/tools/release/check_package.py" "$stage"
rm -f "$out/$name.zip"
(cd "$(dirname "$stage")" && zip -qr9 "$out/$name.zip" "$name")
echo "$out/$name.zip"
