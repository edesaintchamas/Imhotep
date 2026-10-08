"""Imhotep - differential test: the drawing primitives (package blit, present/draw) against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_draw_check.py [--display WxHxF ...] [--save PATH ...]
        [-v] [CASE ...]

The original boots with a display (Game(display=...), docs/oracle.md "Display") and loads each save.
The back buffer is then filled with a fixed pseudo-random pattern (so that transparent pixels, masks
and partial writes show), the clip rectangle and the other inputs of a case are written, and the
case's calls run in the original and in the ports (tools/oracle/harness.compare): the state and the
back buffer must be equal. The state and the buffer are restored between cases. Every case runs at
each display (default: 1024x768 565, 800x600 565, 640x480 555, 1024x768 555) on each save (default
Actium, Iken, AlexC); a case's coordinates are relative to the screen size where they say so.
Each line reports the pixels the original changed, so that a case that draws nothing shows.
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
import harness  # noqa: E402

SAVES = [
    'local/saves/downloaded/1592/39-Actium-Dec30c.sav',
    'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
    'local/saves/downloaded/1591/38-AlexC-Feb15.sav',
]
DISPLAYS = ['1024x768x565', '800x600x565', '640x480x555', '1024x768x555']

DRAW, TOP, TINTED, PAIR, FLIPPED = 0x0053E590, 0x0053F250, 0x0053F4F0, 0x0053F4A0, 0x0053FA40
CLIP = 0x00D3B550  # bottom, top, right, left (i32 each)
TINT = 0x0070E120
GLYPH_COLOUR = 0x00D3B51C

# image ids by kind (docs/formats/graphics.md 5, 12; the startup slots, the same in every save):
# (type, compressed, external, has_top, width) -> ids
IMAGES = {
    'rle': [2, 3, 5],                      # type 0, compressed, 32 x 32
    'mirror_rle': [699, 700, 702],         # mirrored entries of type-0 RLE images
    'figure_mirror': [751, 753, 11485, 23232],  # mirrored entries of figure sprites (walkers, animals)
    'figure': [749, 747, 11481, 23228],    # their targets (RLE)
    'plain': [188, 189],                   # type 0, plain 15 x 15
    'type1': [11916, 11917, 11918],        # type 1 (opaque copy when not clipped)
    'type1_rle_flag': [11925, 12015],      # type 1 with the RLE flag: drawn as plain
    'type10': [6, 7],                      # 16 x 16
    'type12': [15, 16],                    # 24 x 24
    'type13': [1],                         # 32 x 32
    'glyph': [27, 28, 30],                 # type 20 (font)
    'tile': [12521, 12793, 13077],         # type 30, 58 wide
    'fp2': [12798, 13055],                 # type 30, 2 x 2
    'fp3': [12800],
    'top1': [12455, 12458],                # type 30 with tops, 1 x 1 .. 6 x 6
    'top2': [12459, 12465],
    'top3': [12493, 12496],
    'top4': [12497, 12608],
    'top5': [12507, 13020],
    'top6': [12514, 15770],
    'external': [202, 345],                # external, narrower than the screen
    'external_tall': [293],                # external, 162 x 450 (more than the tools' spare screen)
}
WIDE = [292, 423]                          # external 1024 x 768: cropped to the screen when it is smaller
WINDOW = 0x00E38DDA
PICTURE = 0x00F56DB4                       # the image drawn from the last wide picture in windows 0x14 / 0x15


def positions(w, h, clip):
    """(x, y) for an image of w x h: inside, across each edge of the clip rectangle, at the corners,
    and outside it"""
    bottom, top, right, left = clip
    mx, my = (left + right) // 2 - w // 2, (top + bottom) // 2 - h // 2
    xs = [mx, left - w // 2, right - w // 2, left - w - 3, right + 2, left - w + 1, right - 1]
    ys = [my, top - h // 2, bottom - h // 2, top - h - 3, bottom + 2, top - h + 1, bottom - 1]
    out = [(x, my) for x in xs] + [(mx, y) for y in ys[1:]]
    out += [(xs[1], ys[1]), (xs[2], ys[2]), (xs[1], ys[2]), (xs[2], ys[1])]
    return out


def image_size(g, image):
    e = 0x00F55BB0 + 0x180E48 + 64 * image
    w, h = struct.unpack('<hh', g.read(e + 20, 4))
    return w, h


def image_calls(g, kind, clip, fn=DRAW, extra=()):
    calls = []
    for image in IMAGES[kind]:
        w, h = image_size(g, image)
        for x, y in positions(max(w, 1), max(h, 1), clip):
            calls.append((fn, [image, x & 0xFFFFFFFF, y & 0xFFFFFFFF] + list(extra)))
    return calls


def clips(w, h):
    """clip rectangles: the whole screen, and an inner one (a city-view-like viewport)"""
    return {'screen': (h, 0, w, 0), 'inner': (h - 90, 40, w - 170, 30)}


def cases(g):
    """{name: (writes [(address, bytes)], calls)} for the display of game g"""
    _, w, h, _, _ = g.screen_info()
    out = {}
    for cname, clip in clips(w, h).items():
        cw = [(CLIP, struct.pack('<iiii', *clip))]
        for kind in IMAGES:
            if kind.startswith('top'):
                out['%s draw+top %s' % (kind, cname)] = (cw, [c for call in image_calls(g, kind, clip)
                                                               for c in (call, (TOP, call[1]))])
            else:
                out['%s %s' % (kind, cname)] = (cw, image_calls(g, kind, clip))
        tint = cw + [(TINT, struct.pack('<H', 0xF863))]
        for kind in ('rle', 'plain', 'mirror_rle', 'tile', 'fp2', 'top2', 'type1', 'figure', 'figure_mirror'):
            calls = image_calls(g, kind, clip)
            if kind.startswith('top'):
                calls = [c for call in calls for c in (call, (TOP, call[1]))]
            out['%s tinted %s' % (kind, cname)] = (tint + [(TINT, struct.pack('<H', 0x1FE3 if 'f' in kind else 0xF863))],
                                                   calls)
        for kind in ('rle', 'plain', 'mirror_rle', 'type1', 'tile', 'fp2', 'glyph', 'type10', 'type13', 'external',
                     'figure', 'figure_mirror'):
            out['%s flipped %s' % (kind, cname)] = (cw, image_calls(g, kind, clip, FLIPPED))
        for kind in ('rle', 'plain', 'type1', 'figure'):
            out['%s flipped tinted %s' % (kind, cname)] = (tint, image_calls(g, kind, clip, FLIPPED))
        for kind in ('rle', 'plain', 'mirror_rle', 'type1'):
            out['%s mask pass %s' % (kind, cname)] = (cw, image_calls(g, kind, clip, TINTED, [0x1FE3]))
        out['glyph colours %s' % cname] = (cw + [(GLYPH_COLOUR, struct.pack('<H', 0x07E0))], image_calls(g, 'glyph', clip))
        out['pixel pairs %s' % cname] = (cw, image_calls(g, 'plain', clip, PAIR)[:6])
    out['external wide'] = ([], [(f, [i, x, y]) for f in (DRAW, FLIPPED) for i in WIDE for x, y in ((0, 0), (37, 21))])
    for window in (0x14, 0x15):
        out['window 0x%02X picture' % window] = ([(WINDOW, bytes([window]))], [
            (DRAW, [WIDE[0], 0, 0]), (DRAW, [g.u32(PICTURE), 0, 0]), (DRAW, [g.u32(PICTURE), 5, 9]),
            (DRAW, [IMAGES['type1'][0], 50, 60])])
    # lines, rectangles, fills (0x004CD510 and the functions built on it), in and across the screen
    lines = [(10, 10, 300, 10), (10, 10, 10, 300), (5, 7, 200, 90), (200, 90, 5, 7), (5, 7, 90, 200),
             (90, 200, 5, 7), (w - 50, h - 30, w + 40, h + 20), (-20, -30, 60, 45), (100, 100, 100, 100),
             (300, 20, 20, 300), (20, 300, 300, 20), (w // 2, 0, w // 2 + 1, h - 1), (0, h // 2, w - 1, h // 2 + 3)]
    out['lines'] = ([], [(0x004CD510, [v & 0xFFFFFFFF for v in l] + [c]) for l in lines for c in (0xF800, 0x07E0)])
    rects = [(10, 10, 100, 50), (-5, -8, 60, 40), (w - 40, h - 20, 100, 100), (0, 0, w, h), (50, 60, 0, 10),
             (50, 60, 10, 0), (50, 60, -4, 10), (300, 200, 1, 1), (-200, 10, 30, 30)]
    rr = [[v & 0xFFFFFFFF for v in r] for r in rects]
    out['rect outlines'] = ([], [(0x004CD680, r + [0x1234]) for r in rr])
    out['rect bevels'] = ([], [(a, r) for r in rr for a in (0x004CD890, 0x004CD930)])
    out['rect fills'] = ([], [(0x004CD9D0, r + [0x4321]) for r in rr])
    out['rect and-fills'] = ([], [(0x004CDA40, r + [0x7BEF]) for r in rr])
    out['rect greys'] = ([], [(0x004CF090, r + [k]) for r in rr[:3] + rr[7:8] for k in (0, 1, 2)])
    for cname, clip in clips(w, h).items():
        out['rect marching %s' % cname] = ([(CLIP, struct.pack('<iiii', *clip)),
                                             (0x00D3B4F4, struct.pack('<i', 0xF800))],
                                            [(0x004CD710, r) for r in rr] + [(0x004CD810, [])] * 25 +
                                            [(0x004CD710, r) for r in rr[:4]])
    pixels = [(0, 0), (w - 1, h - 1), (17, 23), (w - 1, 0), (5, h - 33), (5, h - 32), (-1, 4), (w, 4)]
    pp = [[v & 0xFFFFFFFF for v in xy] for xy in pixels]
    out['pixels'] = ([(0x00D3B4F0, struct.pack('<i', 0x5A5A))],
                     [(0x0040FA40, xy + [0xABCD]) for xy in pp[:5]] + [(0x0040FA70, xy + [0x0F0F]) for xy in pp[:5]] +
                     [(0x0040FAA0, xy + [m]) for xy in pp[:5] for m in (0xF800, 0xFFFFFFFF)] +
                     [(0x0040FAE0, xy + [0x8001]) for xy in pp[:5]] + [(0x0040FB10, xy) for xy in pp] +
                     [(0x004CD4E0, xy + [0x1111]) for xy in pp])
    screen = g.screen_info()[0]
    out['blocks'] = ([(0x00D3B5E0, struct.pack('<ii', 40, 30)[::1][:4]), (0x00D3B5DC, struct.pack('<i', 30)),
                      (0x00D3B5BE, struct.pack('<h', 12)), (0x00D3B5BC, struct.pack('<h', 9))],
                     [(0x0040F9A0, [screen + 4 * g.screen_info()[3] + 64, g.screen_info()[3] - 48, g.screen_info()[3] - 48]),
                      (0x0040F9F0, [screen + 300 * g.screen_info()[3] + 200, g.screen_info()[3] - 48, g.screen_info()[3] - 48])])
    out.update(text_cases(g, w, h))
    pitch = g.screen_info()[3]
    sizes = [(10, 20, 8, 6), (-20, -10, 5, 4), (w - 40, h - 30, 6, 5), (100, 100, 1, 1), (100, 100, 2, 2), (50, 60, 0, 3),
             (50, 60, 3, 0), (200, 200, 30, 12)]
    out['panels'] = ([], [(a, [v & 0xFFFFFFFF for v in sz]) for a in (0x00536820, 0x00536A30) for sz in sizes])
    out['panel frames'] = ([], [(0x00536C30, [k, x & 0xFFFFFFFF, y & 0xFFFFFFFF, fw & 0xFFFFFFFF, fh & 0xFFFFFFFF, lit])
                                for k, lit in ((0, 0), (1, 1), (0, 1)) for x, y, fw, fh in
                                ((20, 30, 100, 50), (300, 200, 37, 61), (-8, -5, 33, 16), (w - 50, h - 40, 120, 90),
                                 (100, 300, 16, 16), (400, 100, 5, 70), (500, 500, -20, 30), (600, 50, 0, 0))])
    # the layout registry wrappers (0x004D6160 ..): as they are, registry filled first, an override node
    # (at SCRATCH + 64: +0 x, +4 y, +8 / +0xC new place, +0x10 / +0x14 new size, +0x18 next), the layout
    # mode with the cursor inside
    fonts = [g.u32(f) for f in FONTS]
    lay = [(0x004D6160, [147, 0, 300, 40, 200, fonts[0], 0xFFFF, 20, 10]), (0x004D6250, [162, 0, 60, 60, 150, fonts[6], 0xF800, 20, 10]),
           (0x004D62F0, [20, 1, 60, 90, fonts[1], 0xFFFF, 20, 10]), (0x004D6860, [4321, 0x20, 0x0057C048, 60, 120, fonts[2], 0x07E0, 20, 10]),
           (0x004D63D0, [0, 50, 200, 120, 60, 1, 20, 10]), (0x004D65B0, [400, 300, 6, 4, 20, 10])]
    node = SCRATCH + 64
    reg = [(0x00E38DDA, b'\x01'), (node, struct.pack('<iiiiiiI', 40, 50, 100, 70, 90, 30, 0)),
           (0x00E94618 + 16, struct.pack('<I', node)), (0x00E94620 + 16, struct.pack('<I', node))]
    out['layout wrappers'] = ([(0x00E38DDA, b'\x01')], lay)
    out['layout wrappers ready'] = ([(0x00E38DDA, b'\x01'), (0x00E94D9C, struct.pack('<i', 1))], lay)
    out['layout wrappers override'] = (reg + [(0x00E94D9C, struct.pack('<i', 1))],
                                       [(0x004D62F0, [20, 1, 60, 60, fonts[1], 0xFFFF, 20, 10]),
                                        (0x004D63D0, [0, 60, 60, 120, 60, 1, 20, 10]), (0x004D65B0, [60, 60, 6, 4, 20, 10])] + lay)
    for mode in (1, 3, 4):
        out['layout mode %d' % mode] = ([(0x00E38DDA, b'\x01'), (0x00E94D9C, struct.pack('<i', 1)), (0x00E94D98, struct.pack('<i', mode)),
                                         (0x00E38E34, struct.pack('<i', 85)), (0x00E38E30, struct.pack('<i', 140))],
                                        [(0x004D62F0, [20, 1, 60, 130, fonts[1], 0xFFFF, 20, 10]),
                                         (0x004D63D0, [0, 50, 120, 120, 60, 1, 20, 10]), (0x004D65B0, [60, 120, 6, 4, 20, 10])])
    out['screen clear'] = ([], [(0x004CF1C0, [])])
    out['memory fill'] = ([], [(0x0040F8F0, [screen + 100 * pitch + 7, 3 * pitch + 3, 0x5A]),
                               (0x0040F8F0, [screen + 300 * pitch, 0, 0x11])])
    out['invalid ids'] = ([], [(DRAW, [i, 100, 100]) for i in (0, -5, 0x639C, 0x7FFFFFFF, 25139)] +
                          [(TOP, [i, 100, 100]) for i in (0, 2, 0x639C, 12521)])
    # a display mode change (package finish: the resolution keys F6..F9, the 640 x 480 switch around a
    # video 0x00413690): the resolution globals 0x004CFDC0, the screen buffers 0x00414BD0, the view
    # rows 0x004E2AB0, DirectDraw again 0x004D0220 (its set-up 0x004D0060 makes a new back buffer: the
    # one compared after the calls, harness.compare), then drawing into the new buffer
    for mode in (1, 2, 3):
        out['mode change %d' % mode] = ([], [(0x004CFDC0, [mode]), (0x00414BD0, []), (0x004E2AB0, []), (0x004D0220, []),
                                            (DRAW, [IMAGES['rle'][0], 100, 100]), (DRAW, [WIDE[0], 0, 0]),
                                            (0x004CD510, [5, 7, 200, 90, 0xF800])])
    out['directdraw setup'] = ([], [(0x004D0060, []), (DRAW, [IMAGES['plain'][0], 30, 40])])
    return out


FONTS = [0x00F56F84, 0x00F56F8C, 0x00F56F94, 0x00F56F9C, 0x00F56FA4, 0x00F56FAC, 0x00F56FB4, 0x00F56FBC]
SCRATCH = 0x00E3439C   # the multi-line text's line buffer (200 bytes): test strings for the other cases
EDIT = 0x00E34338      # the edited string


def text_address(g, group, index=0):
    """the address of string `index` of group `group` of Pharaoh_Text.eng (as 0x004CC110 finds it)"""
    p = g.u32(0x00E34C38 + 8 * group) + g.u32(0x00E36C70)
    while index > 0:
        if g.read(p, 1)[0] == 0 and (g.read(p - 1, 1)[0] >= 0x20 or g.read(p - 1, 1)[0] == 0):
            index -= 1
        p += 1
    while g.read(p, 1)[0] < 0x20:
        p += 1
    return p


def text_cases(g, w, h):
    fonts = [g.u32(f) for f in FONTS]
    long_text, short_text = text_address(g, 147), text_address(g, 162)
    out = {}
    raw = b'Abc xyz_  #9 [q] gjpqy\x01 \xe7\xc7 \x80\x90 $end\0'
    sw = [(SCRATCH, raw)]
    for k, font in enumerate(fonts):
        out['text font %d' % k] = ([], [(0x004CC770, [a, 20, 30 + 40 * j, font, c]) for j, (a, c) in enumerate(
            ((long_text, 0xFFFF), (short_text, 0xF800), (SCRATCH, 0x07E0)))] +
            [(0x004CC720, [long_text, 10, 200, 300, font, 0x001F]), (0x004CC720, [short_text, 10, 240, 300, font, 0xFFFF]),
             (0x004CC770, [long_text, w - 100, h - 10, font, 0xFFFF]), (0x004CC770, [short_text, (-30) & 0xFFFFFFFF, 5, font, 0])])
        out['text font %d' % k][0].extend(sw)
        out['text measures font %d' % k] = (list(sw), [(0x004CBF30, [a, font]) for a in (long_text, short_text, SCRATCH)] +
                                            [(0x004CC020, [c, font]) for c in (0, 0x20, 0x41, 0x67, 0xE7, 0x7F, 0xFF)] +
                                            [(0x004CD120, [a, font]) for a in (long_text, short_text, SCRATCH, SCRATCH + 3)] +
                                            [(0x004CC6C0, [147, 0, font]), (0x004CC6C0, [20, 1, font])])
    for name, flag in (('outline', 0x00D3B510), ('shadow v', 0x00D3B518), ('shadow', 0x00D3B514)):
        out['text style %s' % name] = (sw + [(flag, struct.pack('<i', 1))],
                                       [(0x004CC770, [a, 20, 30 + 40 * j, fonts[k], 0xFFFF]) for j, a in enumerate((long_text, SCRATCH))
                                        for k in (0, 6)])
    out['text edit field'] = (sw + [(EDIT, b'edit me_ now\0'), (0x00E20780, struct.pack('<i', 1)),
                                    (0x00E20798, b'\x01'), (0x00E20990, struct.pack('<i', 2)),
                                    (0x00E207EC + 8, struct.pack('<i', 5))],
                              [(0x004CC770, [SCRATCH, 30, 30, fonts[0], 0xFFFF]), (0x004CC770, [SCRATCH, 30, 60, fonts[6], 0xFFFF]),
                               (0x004CCB00, [])])
    out['text lines'] = ([], [(0x004CD010, [long_text, 20, 20, ww, fonts[k], 0xFFFF]) for ww in (300, 120, 40, 0)
                              for k in (0, 6)] + [(0x004CCFD0, [147, 0, 400, 300, 250, fonts[1], 0xF800]),
                                                  (0x004CCFD0, [246, 0, 400, 500, 150, fonts[7], 0x07E0])])
    out['text groups'] = ([], [(0x004CCBB0, [grp, idx, 50, 50 + 30 * idx, fonts[idx % 8], 0xFFFF]) for grp, idx in
                               ((162, 0), (20, 1), (20, 0), (147, 0), (305, 2), (10, 5))] +
                          [(0x004CCB30, [grp, idx, 300, 50 + 30 * idx, 200, fonts[(idx + 3) % 8], 0xF800]) for grp, idx in
                           ((162, 0), (20, 1), (147, 0))])
    nums = [0, 7, -15, 123456, 1999999999, -2147483648, 99, 1000000]
    out['numbers'] = ([], [(0x004CCE00, [v & 0xFFFFFFFF, p, s_, 20, 20 + 15 * i, fonts[i % 8], 0xFFFF])
                           for i, (v, p, s_) in enumerate((v, p, s_) for v in nums for p, s_ in
                                                          ((0x20, 0x0057C048), (0x40, short_text), (0, 0x0057C048), (0x2D, long_text)))
                           if 20 + 15 * i < h - 20] +
                      [(0x004CC5C0, [v & 0xFFFFFFFF, pos, plus]) for v in nums for pos, plus in ((0, 0), (3, 1))])
    out['numbers centred'] = ([], [(0x004CCE90, [v & 0xFFFFFFFF, 0x20, 0x0057C048, 100, 30 + 20 * i, 120, fonts[i % 8], 0x07E0])
                                   for i, v in enumerate(nums)] +
                              [(0x004CCF20, [v & 0xFFFFFFFF, short_text, long_text, 300, 30 + 20 * i, 300, fonts[(i + 2) % 8], 0xF81F])
                               for i, v in enumerate(nums)])
    out['counts and amounts'] = ([], [(0x004CCC30, [20, 0, c & 0xFFFFFFFF, 40, 30 + 20 * i, fonts[i % 8], 0xFFFF])
                                      for i, c in enumerate((1, -1, 5, -3, 0))] +
                                 [(0x004CCCB0, [k, v & 0xFFFFFFFF, 300, 30 + 20 * (3 * j + k), fonts[(j + k) % 8], 0xFFE0])
                                  for k in (0, 1, 2) for j, v in enumerate((250, -40, 0))])
    out['text pairs'] = (sw, [(0x004CCD60, [a, b, 50, 40 + 25 * i, 300, fonts[i % 8], 0xFFFF]) for i, (a, b) in enumerate(
        ((short_text, long_text), (0, short_text), (short_text, 0), (0, 0), (SCRATCH + 199, short_text), (long_text, long_text)))])
    field = [(SCRATCH, b'hello\0'), (0x00E20990, struct.pack('<i', 1)), (0x00E2079C + 4, struct.pack('<I', SCRATCH)),
             (0x00E207EC + 4, struct.pack('<i', 2)), (0x00E208A0 + 4, struct.pack('<i', 5)),
             (0x00E208F0 + 4, struct.pack('<i', 12)), (0x00E20850 + 4, struct.pack('<i', 120)),
             (0x00E20940 + 4, struct.pack('<i', fonts[0])), (0x00E2083C + 1, b'\x01')]
    keys = [(0x004CC250, [c]) for c in (0x41, 0x2C, 0x20, 0x7A, 0x1F, 0xE1, 0x95, 0x2D)] + [
        (0x004CC370, []), (0x004CC370, []), (0x004CC330, []), (0x004CC390, []), (0x004CC350, []), (0x004CC360, []),
        (0x004CC250, [0x31]), (0x004CC250, [0x32]), (0x004CC390, []), (0x004CC330, []), (0x004CC320, []), (0x004CC540, [])]
    for k, mode in enumerate((b'\x00', b'\x01')):
        out['edit field keys %d' % k] = (field + [(0x00E2079B, mode)],
                                         keys + [(0x004CC250, [0x41 + i]) for i in range(14)])
    out['edit field draw'] = (field + [(0x00E20780, struct.pack('<i', 1)), (0x00E34238, b'Name: #\0')],
                              [(0x004CC540, []), (0x004CC770, [0x00E34238, 30, 30, fonts[6], 0xFFFF]),
                               (0x004CC1B0, []), (0x004CC4C0, []), (0x004CC400, [0x51])])
    rect = SCRATCH + 120
    rw = sw[:0] + [(rect, struct.pack('<hhhh', 30, 40, 230, 90)), (0x00E38DA8, struct.pack('<ii', 20, 0)[:4]),
                   (0x00E38DA4, struct.pack('<i', 10))]
    out['layout texts'] = (rw, [(0x004CD1F0, [162, 0, rect, fonts[0], 0xFFFF, 5, 6]), (0x004CD240, [147, 0, rect, fonts[6], 0xF800, -5, 30]),
                                (0x004CD290, [1234, 0x20, short_text, rect, fonts[1], 0xFFFF, 0, 60]),
                                (0x004CD2E0, [(-77) & 0xFFFFFFFF, 0x40, 0x0057C048, rect, fonts[2], 0x07E0, 0, 90]),
                                (0x004CD340, [1, 500, rect, fonts[3], 0xFFFF, 10, 120]), (0x004CD340, [2, (-500) & 0xFFFFFFFF, rect, fonts[4], 0xFFFF, 10, 150]),
                                (0x004CD390, [short_text, rect, fonts[5], 0x001F, 40, 200]),
                                (0x004CD1A0, [0, rect, 0, 0, 230]), (0x004CD1A0, [0, rect, 1, 300, 230])])
    return out


def pattern(n):
    """a fixed pseudo-random byte pattern (an LCG), n bytes"""
    out = bytearray(n)
    v = 0x12345678
    for i in range(0, n, 4):
        v = (v * 1103515245 + 12345) & 0xFFFFFFFF
        out[i:i + 4] = struct.pack('<I', v)
    return bytes(out[:n])


def main():
    argv = sys.argv[1:]
    verbose = '-v' in argv
    if verbose:
        argv.remove('-v')
    displays, saves = [], []
    while '--display' in argv:
        k = argv.index('--display')
        displays.append(argv[k + 1])
        del argv[k:k + 2]
    while '--save' in argv:
        k = argv.index('--save')
        saves.append(argv[k + 1])
        del argv[k:k + 2]
    only = argv
    failed = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        for display in displays or DISPLAYS:
            for path in saves or SAVES:
                path = os.path.join(ROOT, path)
                g = harness.oracle_state(path, 0, display)
                info = g.screen_info()
                base = pattern(info[3] * info[2])
                g.set_screen(base)
                snap = g.state_bytes()
                other_mark = g.other_heap
                # the other heap as it is (graphics, fonts, DirectDraw's objects): restored before each
                # case, as the state is (a case may write past the back buffer's end, where the emulated
                # surfaces' COM objects follow it: the mode change cases call them)
                from emu import OTHER_HEAP_BASE
                other = g.read(OTHER_HEAP_BASE, other_mark - OTHER_HEAP_BASE)
                for name, (writes, calls) in cases(g).items():
                    if only and not any(o in name for o in only):
                        continue
                    g.load_state(snap)
                    if g.other_heap > other_mark:
                        g.write(other_mark, bytes(g.other_heap - other_mark))
                        g.other_heap = other_mark
                    g.write(OTHER_HEAP_BASE, other)
                    g.set_screen(base)
                    for a, data in writes:
                        g.write(a, data)
                    try:
                        written, ranges, bad = harness.compare(g, calls, tmp)
                    except RuntimeError as e:
                        print('  %s / %s / %s: %s' % (display, os.path.basename(path), name, e))
                        return 2
                    changed = None
                    if verbose:
                        now = g.screen()
                        changed = sum(1 for i in range(0, len(base), 2) if now[i:i + 2] != base[i:i + 2])
                    total += 1
                    ok = not ranges and not bad
                    failed += not ok
                    print('  %-13s %-22s %-34s %s%s' % (display, os.path.basename(path)[:22], name,
                                                       'equal' if ok else 'DIFFERENT',
                                                       '' if changed is None else ' (%d pixels drawn)' % changed),
                          flush=True)
                    if not ok:
                        pixels, rects = harness.compare.screen
                        if pixels:
                            print('      screen: %d pixels differ: %s' % (pixels, ', '.join(
                                '%dx%d at %d,%d' % (rw, rh, x, y) for x, y, rw, rh in rects[:6])))
                        for addr, n in ranges[:6]:
                            print('      %s: %d bytes' % (harness.diff.name_address(addr), n))
                        for (a, args), ro, rp in bad[:5]:
                            print('      0x%08X(%s): original %d, port %d' % (a, ','.join(map(str, args)), ro, rp))
    print('oracle_draw_check: %d / %d drawing cases equal to the original' % (total - failed, total))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
