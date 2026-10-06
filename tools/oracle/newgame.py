"""Imhotep - starting a new game in the oracle (roadmap phase 6, docs/newgame.md).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/newgame.py map:NAME | mission:N [--difficulty D]

Every new city starts in 0x005405B0(0x00E37078, flag) (docs/newgame.md): a custom scenario
(maps\\NAME.map; 0x00E38EBC = 2) or a campaign mission (record N of mission1.pak; 0x00E38EBC = 0,
0x00E38EA8 = N, 0x007027B4 = campaign 1 Pharaoh / 2 Cleopatra). The menus only set these inputs.
start() sets them on a booted game and returns the call (address, args) that starts it, so that a
caller can run it in the original alone or through the deep recorder (harness.compare).
"""
import struct
import sys

START = 0x005405B0
NAME = 0x00E37078          # char[65] scenario / map name
KIND = 0x00E38EBC          # 0 campaign record, 2 custom scenario (docs/formats/save.md chunk 91)
RECORD = 0x00E38EA8        # u8 mission1.pak record
CAMPAIGN = 0x007027B4      # u8 1 Pharaoh, 2 Cleopatra
DIFFICULTY = 0x00E39080    # i32 0 very easy .. 4 very hard


def inputs(g, what, difficulty=None, campaign=1):
    """Set the start inputs of `what` ('map:NAME' or 'mission:N') on game g; returns the call."""
    kind, _, arg = what.partition(':')
    if kind == 'map':
        g.write(NAME, arg.encode()[:64].ljust(65, b'\0'))
        g.write(KIND, struct.pack('<i', 2))
        g.write(CAMPAIGN, b'\0')
    elif kind == 'mission':
        g.write(KIND, struct.pack('<i', 0))
        g.write(RECORD, bytes([int(arg)]))
        g.write(CAMPAIGN, bytes([campaign]))
    else:
        raise ValueError('map:NAME or mission:N, not %r' % what)
    if difficulty is not None:
        g.write(DIFFICULTY, struct.pack('<i', difficulty))
    return START, [NAME, 0]


def main():
    from game import Game
    argv = sys.argv[1:]
    difficulty = None
    if '--difficulty' in argv:
        k = argv.index('--difficulty')
        difficulty = int(argv[k + 1])
        del argv[k:k + 2]
    g = Game()
    g.boot()
    addr, args = inputs(g, argv[0], difficulty)
    g.call(addr, *args)
    print('%s: year %d, map %d x %d, treasury %d' % (argv[0], g.i32(0x0078D7BC), g.i32(0x00BCA2D0),
                                                      g.i32(0x00BCA2D4), g.i32(0x00EA1870 + 0x49D8 + 0xC)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
