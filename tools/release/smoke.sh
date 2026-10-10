#!/bin/sh
# Imhotep - smoke test of a packaged game, without the game files (roadmap 9.4).
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/release/smoke.sh EXE VERSION
#
# `EXE --version` must print "imhotep VERSION"; a start with a game directory that does not exist must
# stop with a clean error naming it, status 1. Both load every shared library the executable links:
# a library missing from the package fails here. SMOKE_PATH, if set, is the PATH the executable runs
# with (on Windows, where DLLs are also searched in PATH: the system directories only).
set -u
[ $# = 2 ] || { echo "usage: $0 EXE VERSION"; exit 1; }
exe=$1 version=$2
run() { if [ -n "${SMOKE_PATH:-}" ]; then env PATH="$SMOKE_PATH" "$@"; else "$@"; fi; }
tmp=$(mktemp -d)
run "$exe" --version > "$tmp/version.txt" 2>&1
status=$?
cat "$tmp/version.txt"
[ "$status" = 0 ] || { echo "FAIL: --version exited $status"; exit 1; }
grep -q "^imhotep $version " "$tmp/version.txt" || { echo "FAIL: --version does not print imhotep $version"; exit 1; }
export IMHOTEP_GAME_DIR=missing-game-dir IMHOTEP_PLAYER_DIR="$tmp/player"
run "$exe" --hidden --no-intro --no-sound --no-video \
    > "$tmp/out.txt" 2>&1
status=$?
cat "$tmp/out.txt"
[ "$status" = 1 ] || { echo "FAIL: a start without the game exited $status, not 1"; exit 1; }
grep -q 'missing-game-dir' "$tmp/out.txt" || { echo "FAIL: the error does not name the game directory"; exit 1; }
echo "OK: smoke test of $exe"
