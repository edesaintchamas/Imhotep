"""Imhotep - generate the reference save corpus with the original game code, headless.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/corpus.py [--months N] [--name NAME] SAVE ...
  local/venv/bin/python tools/oracle/corpus.py [--months N] --pak RECORD ...

For each starting state (a saved game, or a record of mission1.pak) the reference Pharaoh.exe is
booted in the emulator (tools/oracle/game.py), the state is loaded with the game's own loader, and
the game's tick function 0x004D7090 runs. At the start of every month the game's own saver
0x004D4E20 writes the state: local/saves/generated/<name>/m00.sav (as loaded) ... mNN.sav.
Each file is added to local/saves/manifest.json ("generated"), dated with build/save_dump.
Game content stays under local/.
"""
import argparse
import fcntl
import hashlib
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game, GAME_DIR  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'local', 'saves', 'generated')
MANIFEST = os.path.join(ROOT, 'local', 'saves', 'manifest.json')
GAME_TICK = 0x004D7090  # one game tick (51 per day)
MONTH = 0x0078D7C0      # month 0..11 (chunk "month")


def pak_record(n):
    """Bytes of record n of mission1.pak, through build/save_dump's reader (campaign_pak.c)."""
    out = os.path.join(ROOT, 'local', 'oracle', 'record.bin')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'save_dump'), '--pak', str(n), '--stored', '--raw', out],
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(r.stderr.strip())
    return open(out, 'rb').read()


def describe(path):
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'save_dump'), '--stored', path], capture_output=True, text=True)
    if r.returncode != 0:
        return {}
    d = json.loads(r.stdout)
    return {'file_version': d['version'], 'date': d['date'],
            'buildings_in_use': sum(1 for b in d['buildings'] if b['state'] == 1), 'figures': len(d['figures'])}


def run(name, data, months, origin):
    out_dir = os.path.join(OUT, name)
    os.makedirs(out_dir, exist_ok=True)
    g = Game()
    g.boot()
    if g.load_save(data) != 0:
        raise RuntimeError('the game could not load %s' % name)
    entries = []

    def emit(k, ticks):
        path = os.path.join(out_dir, 'm%02d.sav' % k)
        blob = g.save()
        open(path, 'wb').write(blob)
        e = {'file': os.path.relpath(path, os.path.dirname(MANIFEST)), 'sha256': hashlib.sha256(blob).hexdigest(),
             'bytes': len(blob), 'start': origin, 'months_run': k, 'ticks_run': ticks}
        e.update(describe(path))
        entries.append(e)
        print('  %s m%02d: %s' % (name, k, e.get('date')), flush=True)

    emit(0, 0)
    ticks = 0
    for k in range(1, months + 1):
        month = g.i32(MONTH)
        while g.i32(MONTH) == month:
            g.call(GAME_TICK)
            ticks += 1
        emit(k, ticks)
    return entries


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[1])
    ap.add_argument('--months', type=int, default=12)
    ap.add_argument('--name')
    ap.add_argument('--pak', action='store_true', help='arguments are mission1.pak record numbers')
    ap.add_argument('sources', nargs='+')
    a = ap.parse_args()
    for src in a.sources:
        if a.pak:
            data, origin = pak_record(int(src)), 'mission1.pak record %s' % src
            name = a.name or 'pak%02d' % int(src)
        else:
            data, origin = open(src, 'rb').read(), os.path.relpath(os.path.abspath(src), os.path.dirname(MANIFEST))
            name = a.name or os.path.splitext(os.path.basename(src))[0].replace(' ', '_')
        t = time.time()
        print('%s (%s)' % (name, origin), flush=True)
        entries = run(name, data, a.months, origin)
        print('  %.0f s' % (time.time() - t), flush=True)
        update_manifest(entries)


def update_manifest(entries):
    """Replace this run's files in the manifest; locked, so runs can go in parallel."""
    with open(MANIFEST + '.lock', 'w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        manifest = json.load(open(MANIFEST)) if os.path.exists(MANIFEST) else {}
        keep = {e['file'] for e in entries}
        generated = [e for e in manifest.get('generated', []) if isinstance(e, dict) and e['file'] not in keep]
        manifest['generated'] = sorted(generated + entries, key=lambda e: e['file'])
        manifest['generator'] = ('tools/oracle/corpus.py: reference Pharaoh.exe (GOG 2.1) run headless in the '
                                 'emulator, saved by the game at each month start')
        json.dump(manifest, open(MANIFEST, 'w'), indent=1)


if __name__ == '__main__':
    main()
