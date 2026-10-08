"""Imhotep - differential test: the Bink video player 0x00413580 against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_video_check.py [--display WxHxF ...] [--save PATH ...]

The original boots with a display and loads each save; the player 0x00413580 (thiscall on its object
[0x006FDC54]) is called on a period video, fullscreen (0x00E38E5C = 1) and windowed, in two ways:
  - BinkOpen fails, as it does in the oracle (BINKW32 is not emulated): the streams stopped, nothing else;
  - BinkOpen opens a 560 x 333 video with no frame (tools/oracle/game.py Game.bink; the port's
    video_open answers the same: state_replay --video): 0x00413690 then makes its state effects without
    its frame loop: the screen cleared (0x00426F60), the window id 0x4A and back (0x00413420), and in
    fullscreen the display switched to 640 x 480 and back to the resolution of before (0x004D01E0,
    0x004CFDC0, 0x00414BD0, 0x004E2AB0, 0x004D0220 with DirectDraw's set-up 0x004D0060, 0x00402DA0).
The state and the back buffer (the new one after a mode change) must be equal (tools/oracle/harness.py).
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
import harness  # noqa: E402

SAVES = ['local/saves/downloaded/1592/39-Actium-Dec30c.sav', 'local/saves/downloaded/1564/CaesarAlan-Iken.sav']
DISPLAYS = ['1024x768x565', '800x600x565', '640x480x555']
VIDEO = b'Binks\\High\\pre_dynastic_big.bik\0'  # the period videos' path (0x0041BB70)
PLAYER = 0x00413580
OBJECT = 0x006FDC54


def main():
    argv = sys.argv[1:]
    displays, saves = [], []
    while '--display' in argv:
        k = argv.index('--display')
        displays.append(argv[k + 1])
        del argv[k:k + 2]
    while '--save' in argv:
        k = argv.index('--save')
        saves.append(argv[k + 1])
        del argv[k:k + 2]
    failed = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        for display in displays or DISPLAYS:
            for save in saves or SAVES:
                for bink in (None, (560, 333)):
                    for fullscreen in (1, 0):
                        g = harness.oracle_state(os.path.join(ROOT, save), 0, display)
                        g.bink = bink
                        name = g.alloc(len(VIDEO))
                        g.write(name, VIDEO)
                        g.write(0x00E38E5C, bytes([fullscreen, 0, 0, 0]))
                        harness.THIS[PLAYER] = g.u32(OBJECT)
                        label = '%-13s %-22s %-14s %s' % (display, os.path.basename(save)[:22],
                                                          'opens %dx%d' % bink if bink else 'fails to open',
                                                          'fullscreen' if fullscreen else 'windowed')
                        try:
                            written, ranges, bad = harness.compare(g, [(PLAYER, [name, 40, 70])], tmp)
                        except RuntimeError as e:
                            print('  %s: %s' % (label, e))
                            return 2
                        total += 1
                        ok = not ranges and not bad and not harness.compare.fallback
                        failed += not ok
                        print('  %s  %s (%d bytes written, resolution %d, window 0x%02X)' % (
                            label, 'equal' if ok else 'DIFFERENT', written, g.i32(0x00E38E68), g.read(0x00E38DDA, 1)[0]),
                            flush=True)
                        for addr, n in ranges[:6]:
                            print('      %s: %d bytes' % (harness.diff.name_address(addr), n))
                        for line in harness.compare.fallback[:3]:
                            print('      ' + line)
    print('oracle_video_check: %d / %d video cases equal to the original' % (total - failed, total))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
