"""Imhotep - a test option of the SDL game (build/imhotep --boot-state): a booted state made by the oracle.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python platform/oracle_boot.py [--display 1024x768x555] [--no-sound] [--save SAVE]
                                                [--out local/sdl/boot]

The game boots through core (core/boot, platform/main.c) and needs only the game files. For tests,
`build/imhotep --boot-state OUT` starts instead from a game booted by the original itself in the
oracle (tools/oracle, Pharaoh.exe in an emulator, read from the user's own installation), to compare a
session with the original's from the very same state: this script writes it under local/ (git-ignored;
it holds game data, never commit it):

  OUT.state  the state file (core/state/state.h: data segments, simulation heap, blocks)
  OUT.heap   the oracle's other heap (0x68000000 up to its top): the pixel buffers of the graphics
             slots, the fonts, the text files, the sound registry, the back buffer and the spare
             screen (DirectDraw surfaces); the game maps it at the same address (platform/memory.c)
  OUT.txt    base and size of OUT.heap, the display

--save loads a saved game after the boot (the original's loader 0x004D5100), for a state that opens
in the city. The default display is 555: the .555 pixel data is then used as it is stored, no 565
conversion (0x004CDF20) is needed on the platform's side.
"""
import argparse
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

from emu import OTHER_HEAP_BASE  # noqa: E402
from game import Game  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--display', default='1024x768x555')
    ap.add_argument('--no-sound', action='store_true')
    ap.add_argument('--save')
    ap.add_argument('--out', default=os.path.join(ROOT, 'local', 'sdl', 'boot'))
    a = ap.parse_args()
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    if a.no_sound:
        g = Game.booted(display=a.display, sound=False)
    else:
        g = Game(display=a.display, sound=True)  # a sound boot cannot be snapshotted
        g.boot()
    if a.save:
        r = g.load_save(open(a.save, 'rb').read())
        if r != 0:
            sys.exit('the original could not load %s (%d)' % (a.save, r))
    open(a.out + '.state', 'wb').write(g.state_bytes())
    heap = g.read(OTHER_HEAP_BASE, g.other_heap - OTHER_HEAP_BASE)
    open(a.out + '.heap', 'wb').write(heap)
    info = g.screen_info()
    with open(a.out + '.txt', 'w') as f:
        f.write('heap_base %08X\nheap_size %d\n' % (OTHER_HEAP_BASE, len(heap)))
        f.write('display %d %d %d %d %08X\n' % (info[1], info[2], info[3], info[4], info[0]))
        f.write('window %d\n' % g.read(0x00E38DDA, 1)[0])
    print('%s.state, .heap (%d bytes), window %d, back buffer %08X' %
          (a.out, len(heap), g.read(0x00E38DDA, 1)[0], info[0]))


if __name__ == '__main__':
    main()
