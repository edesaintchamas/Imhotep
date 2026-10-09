"""Imhotep - randomised play sessions (roadmap 8.3): seeded, reproducible sessions a player could make,
driven live on build/imhotep, then checked against the original (tests/differential/oracle_play_check.py).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/fuzz.py gen SEED [--frames N] [--mode M] [--out DIR]
        writes DIR/SEED.txt (the session: build/imhotep --script lines) and DIR/SEED.player/ (the
        player directory it starts from: Pharaoh.inf, and for mode `load` a family with corpus saves)
  [FUZZ_IDLE=0.3] local/venv/bin/python tools/oracle/fuzz.py run SEEDS [--frames N] [--mode M] [-j 2] [--no-oracle]
        SEEDS: numbers or ranges (1-20). Per seed: the session made (its gaps.log and exit status: the
        unported functions it reached, a crash), then oracle_play_check (stopping at the first differing
        frame). Each finding gets a minimal case (the session's lines up to its frame) in
        local/oracle/phase8/fuzz/cases/ and a line in local/oracle/phase8/fuzz/findings.md (gaps once per
        address); every seed a line in runs.log

How a session is made: build/imhotep runs with `--script -` (platform/main.c): before each script line it
prints "ready FRAME WINDOW", and `peek ADDR LEN` lines (which take no frame) read the state. The fuzzer
picks the next action from the window the game is in and from the state (the sidebar's button tables
0x005C7300 / 0x005C7370 / 0x005C7980 at the sidebar's x 0x00F53F64, the screen size 0x00E38DB0 /
0x00E38DAC), records it and sends it. The clock is scripted (--det-clock 0,25), so the recorded lines
replayed from a file (`--script FILE`) make the same session, frame for frame.

Modes (how the session gets into a game, through the menus as a player does):
  family   a new family (name typed), Begin Family History: the first mission;
  mission  a new family, Choose a Mission: a random period and mission;
  custom   a new family, Custom Missions: a random bundled map;
  load     the player directory holds a family `Fuzz` and corpus saves (local/saves/downloaded,
           local/saves/generated): Load Saved Game, a random one.
Then random actions, weighted towards meaningful ones: build menus and placing buildings and roads (clicks
and drags on the city), clearing, info panels (right clicks), advisors and their buttons, the overlays menu
and keys, the top menus (File: save and load, Options, Help, Overseers), the empire map, the message list,
scrolling (arrow keys, screen edges), rotation, speed and pause, hotkeys, Esc and its quit box (answered),
long waits so the simulation runs. Coordinates stay on the screen. Windows the fuzzer has no model of get
random clicks inside the screen, right clicks and Esc. Layout knowledge comes from the ported code
(present/sidebar/input.c, present/ui/windows.c) and the game's own tables read live.
"""
import json
import os
import random
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'differential'))
GAME = os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'imhotep')
SAVES = [os.path.join(ROOT, 'local', 'saves', d) for d in ('downloaded', 'generated')]
FAMILY = 'Fuzz'
MODES = ['family', 'mission', 'custom', 'load']

WINDOW_CITY = 1
FRONTEND = {60, 0, 54, 47, 48, 13, 23, 52, 53, 61, 83, 4, 59}


def corpus_saves():
    out = []
    for base in SAVES:
        for d, _, files in os.walk(base):
            out += [os.path.join(d, f) for f in files if f.lower().endswith('.sav')]
    return sorted(out)


def options(display=(1024, 768), windowed=False):
    from oracle_play_check import options as o
    return o(display, windowed)


def make_player(path, mode, rng, family_dat, display=(1024, 768)):
    """the player directory a session starts from (also given to the original by oracle_play_check)"""
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path)
    with open(os.path.join(path, 'Pharaoh.inf'), 'wb') as f:
        f.write(options(display))
    saves = []
    if mode == 'load':
        os.makedirs(os.path.join(path, 'save', FAMILY))
        shutil.copy(family_dat, os.path.join(path, 'save', FAMILY + '.dat'))
        pool = corpus_saves()
        for k, src in enumerate(rng.sample(pool, min(len(pool), rng.randint(1, 4)))):
            rel = os.path.relpath(src, ROOT)
            name = 'f%d-%s' % (k, os.path.basename(src))
            shutil.copy(src, os.path.join(path, 'save', FAMILY, name))
            saves.append((name, rel))
    return saves


class Game:
    """build/imhotep driven line by line (platform/main.c --script -)"""

    def __init__(self, player, log, args=()):
        env = dict(os.environ, IMHOTEP_PLAYER_DIR=player)
        self.err = open(log, 'w')
        self.p = subprocess.Popen([GAME, '--hidden', '--no-intro', '--no-sound', '--no-video', '--det-clock', '0,25',
                                   '--script', '-'] + list(args), env=env, stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=self.err, text=True, bufsize=1)
        self.lines = []
        self.at = []
        self.wins = []
        self.frame = 0
        self.window = -1
        self.alive = True
        self.ready()

    def ready(self):
        while True:
            line = self.p.stdout.readline()
            if not line:
                self.alive = False
                return False
            if line.startswith('ready '):
                _, f, w = line.split()
                self.frame, self.window = int(f), int(w)
                return True

    def send(self, line, record=True):
        if not self.alive:
            return False
        if record:
            self.lines.append(line)
            self.at.append(self.frame)  # the line is read after this frame: it acts from the next one
            self.wins.append(self.window)
        try:
            self.p.stdin.write(line + '\n')
            self.p.stdin.flush()
        except BrokenPipeError:
            self.alive = False
            return False
        if line.startswith('#'):
            return self.ready()
        return self.ready()

    def peek(self, addr, n):
        if not self.alive:
            return b''
        try:
            self.p.stdin.write('peek %X %d\n' % (addr, n))
            self.p.stdin.flush()
        except BrokenPipeError:
            self.alive = False
            return b''
        while True:
            line = self.p.stdout.readline()
            if not line:
                self.alive = False
                return b''
            if line.startswith('peek '):
                parts = line.split()
                return bytes.fromhex(parts[2]) if len(parts) > 2 else b''

    def u32(self, addr):
        b = self.peek(addr, 4)
        return int.from_bytes(b, 'little', signed=True) if len(b) == 4 else 0

    def close(self):
        if self.alive:
            self.send('quit')
        try:
            self.p.stdin.close()
        except Exception:
            pass
        try:
            self.p.wait(timeout=60)
        except subprocess.TimeoutExpired:
            self.p.kill()
            self.p.wait()
        self.err.close()
        return self.p.returncode


class Fuzzer:
    def __init__(self, seed, mode, frames, game):
        self.rng = random.Random(seed)
        self.mode = mode
        self.frames = frames
        self.g = game
        self.outside = 0      # actions in a row outside the city view
        self.in_city = False
        self.roads = []       # roads dragged on the screen (x0, y0, x1, y1)
        self.idle = float(os.environ.get('FUZZ_IDLE', '0'))
        self.orphan_stop = False  # extra share of long waits in the city
        self.paused = False

    # ---- layout read from the game ----
    def screen(self):
        w, h = self.g.u32(0x00E38DB0), self.g.u32(0x00E38DAC)
        return (w, h) if w > 0 and h > 0 else (1024, 768)

    def sidebar_buttons(self):
        """the expanded sidebar's enabled buttons: (table, index, x, y, w, h), read live (present/sidebar/input.c)"""
        sx = self.g.u32(0x00F53F64)
        out = []
        for table, count in ((0x005C7300, 2), (0x005C7370, 16), (0x005C7980, 2)):
            raw = self.g.peek(table, 0x24 * count)
            for k in range(count):
                e = raw[0x24 * k:0x24 * k + 0x24]
                if len(e) < 0x24 or not e[0x18]:
                    continue
                x, y, w, h = (int.from_bytes(e[i:i + 2], 'little', signed=True) for i in (0, 2, 4, 6))
                out.append((table, k, sx + x, 0x1E + y, w, h))
        return out

    def city_rect(self):
        w, h = self.screen()
        sx = self.g.u32(0x00F53F64)
        right = sx if 0 < sx < w else w - 186
        return 4, 26, right - 4, h - 4

    # ---- primitives ----
    def act(self, line):
        return self.g.send(line)

    def wait(self, n):
        return self.act('wait %d' % max(1, n))

    def click(self, x, y, right=False):
        w, h = self.screen()
        x, y = max(0, min(w - 1, int(x))), max(0, min(h - 1, int(y)))
        return self.act('%s %d %d' % ('rclick' if right else 'click', x, y))

    def fclick(self, x, y, right=False):
        """a click on the front end's 640 x 480 layout, given at 1024 x 768 (its origin 192, 144): placed at
        the layout's origin 0x00E38DA8 / 0x00E38DA4 of the current mode"""
        dx, dy = self.g.u32(0x00E38DA8), self.g.u32(0x00E38DA4)
        return self.click(x - 192 + dx, y - 144 + dy, right)

    def drag(self, x0, y0, x1, y1, steps=3):
        self.act('down %d %d' % (x0, y0))
        for k in range(1, steps + 1):
            self.act('move %d %d' % (x0 + (x1 - x0) * k // steps, y0 + (y1 - y0) * k // steps))
            self.wait(self.rng.randint(1, 4))
        return self.act('up %d %d' % (x1, y1))

    def city_point(self):
        x0, y0, x1, y1 = self.city_rect()
        r = self.rng
        # mostly near the middle of the view, where a player builds
        if r.random() < 0.7:
            cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
            return (max(x0, min(x1, int(r.gauss(cx, (x1 - x0) / 5)))),
                    max(y0, min(y1, int(r.gauss(cy, (y1 - y0) / 5)))))
        return r.randint(x0, x1), r.randint(y0, y1)

    def table_click(self, entry, x, y):
        """a click inside a button of a sidebar-style table (entries of 0x24: x, y, w, h) placed at x, y"""
        e = self.g.peek(entry, 8)
        if len(e) == 8:
            bx, by, bw, bh = (int.from_bytes(e[i:i + 2], 'little', signed=True) for i in (0, 2, 4, 6))
            x += bx + self.rng.randint(2, max(2, bw - 3))
            y += by + self.rng.randint(2, max(2, bh - 3))
        return self.click(x, y)

    # ---- the front end: into a game ----
    def frontend(self):
        r, w = self.rng, self.g.window
        if w == 60:      # the title: click to start
            return self.fclick(512, 400) and self.wait(30)
        if w == 0:       # main menu: Play (rarely Greatest Families); never the map editor (out of scope)
            if r.random() < 0.08:
                return self.fclick(512, 376) and self.wait(30)
            return self.fclick(512, 256) and self.wait(30)
        if w == 54:      # a family name
            self.act('text %s' % FAMILY)
            self.wait(len(FAMILY) * 2 + 4)
            return self.act('key Return') and self.wait(30)
        if w == 47:      # the family registry: the first family, Proceed
            self.fclick(380, 280)
            self.wait(10)
            return self.fclick(652, 489) and self.wait(30)
        if w == 48:      # the family's menu
            target = {'family': 285, 'mission': 333, 'load': 381, 'custom': 429}[self.mode]
            if self.mode == 'load' and r.random() < 0.15:
                target = r.choice([285, 333, 429])
            return self.fclick(512, target) and self.wait(40)
        if w == 13:      # the saved games: one of the list, then OK
            if self.in_city and r.random() < 0.3:  # from File > Load game: cancelled
                return self.fclick(r.randint(200, 820), r.randint(150, 620), right=True) and self.wait(20)
            self.fclick(400, 280 + 16 * r.randint(0, 3) + 6)
            self.wait(10)
            return self.fclick(602, 491) and self.wait(120)
        if w == 83:      # greatest families: right click to continue
            return self.fclick(512, 400, right=True) and self.wait(30)
        if w == 23:      # the periods: a random one (Pharaoh / Cleopatra), then Begin
            # (a new family has only the first period open: Predynastic, 283 447)
            if r.random() < 0.6:
                self.fclick(283, 447)
            else:
                self.fclick(r.choice([283, 435]), r.choice([418, 447, 478, 548, 578]))
            self.wait(15)
            return self.fclick(800, 595) and self.wait(160)
        if w == 52:      # a campaign step's map: one of its missions (present/ui/empire/mission.c 0x00419090), start
            step = int.from_bytes(self.g.peek(0x00E38EAB, 1), 'little', signed=True)
            if 0 <= step < self.g.u32(0x006FF608):
                rec = 0x006FDD08 + 0x40 * step
                count = self.g.u32(rec + 0x38)
                dx, dy = self.g.u32(0x00E38DA8), self.g.u32(0x00E38DA4)
                if 0 < count < 10:
                    k = r.randrange(count)
                    x, y = self.g.u32(rec + 0x20 + 0xC * k) + dx, self.g.u32(rec + 0x24 + 0xC * k) + dy
                    self.click(x + r.randint(4, 40), y + r.randint(4, 40))  # (state coordinates)
                    self.wait(15)
            return self.table_click(0x00579788, self.g.u32(0x006FDCA0) + 0x244, self.g.u32(0x006FDC9C) + 0x19A) and \
                self.wait(60)
        if w == 59:      # the mission briefing from the city (window 0x3B): its button 0x00579788 (present/ui/dispatch.c)
            if r.random() < 0.3:
                return self.other_window()
            return self.table_click(0x00579788, self.g.u32(0x006FDCA0) + 0x234, self.g.u32(0x006FDC9C) + 0x19A) and \
                self.wait(30)
        if w == 61:      # the custom missions: a map of the list, then Go to city
            self.fclick(r.randint(220, 400), 372 + 16 * r.randint(0, 14))
            self.wait(10)
            if r.random() < 0.2:
                self.fclick(r.choice([567, 585]), 544)
                self.wait(8)
            return self.fclick(803, 596) and self.wait(160)
        if w == 53:      # the mission briefing: difficulty sometimes, to the city
            if r.random() < 0.3:
                self.fclick(r.choice([326, 343]), 584)
                self.wait(10)
            return self.fclick(785, 583) and self.wait(60)
        return self.other_window()

    # ---- other windows: random input inside the screen ----
    def other_window(self):
        r = self.rng
        w, h = self.screen()
        p = r.random()
        if p < 0.55:
            # dialogs are drawn centred: clicks mostly in the middle 640 x 480
            cx, cy = w // 2, h // 2
            return self.click(r.randint(cx - 320, cx + 319), r.randint(max(0, cy - 240), cy + 239)) and \
                self.wait(r.randint(3, 30))
        if p < 0.70:
            return self.click(r.randint(0, w - 1), r.randint(0, h - 1), right=True) and self.wait(r.randint(3, 20))
        if p < 0.80:
            return self.click(r.randint(0, w - 1), r.randint(0, h - 1)) and self.wait(r.randint(3, 20))
        if p < 0.90:
            self.act('answer 1')  # the quit box: No (Yes ends the program)
            return self.act('key Escape') and self.wait(r.randint(5, 20))
        if p < 0.95:
            return self.act('key %s' % r.choice(['Return', 'Up', 'Down', 'PageUp', 'PageDown', 'Space'])) and \
                self.wait(r.randint(3, 15))
        return self.wait(r.randint(20, 200))

    # ---- the city ----
    def menu_rows(self):
        """the open build menu's rows on the screen: (x0, y0, x1, y1), from its table 0x005C28A8 placed as
        build_menu_input 0x00403E00 places it (present/ui/windows.c)"""
        sx = self.g.u32(0x00F53F64)
        if self.g.u32(0x00F53F68) == 1:  # collapsed
            sx = self.g.u32(0x00F53F60)
        x = sx - 0x102 + 0xA1 - (self.g.u32(0x00E94DAC) + 0x14)
        y = self.g.u32(0x007056E8) + 0x6E
        n = max(0, min(30, self.g.u32(0x007056F0)))
        raw = self.g.peek(0x005C28A8, 0x1C * n)
        rows = []
        for k in range(n):
            e = raw[0x1C * k:0x1C * k + 8]
            if len(e) == 8:
                x0, y0, x1, y1 = (int.from_bytes(e[i:i + 2], 'little', signed=True) for i in (0, 2, 4, 6))
                rows.append((x + x0, y + y0, x + x1, y + y1))
        return rows

    def near_road(self):
        """a point next to a road built in this session (most buildings need one), else anywhere"""
        r = self.rng
        if self.roads and r.random() < 0.8:
            x0, y0, x1, y1 = r.choice(self.roads[-6:])
            t = r.random()
            cx0, cy0, cx1, cy1 = self.city_rect()
            return (max(cx0, min(cx1, int(x0 + (x1 - x0) * t) + r.randint(-40, 40))),
                    max(cy0, min(cy1, int(y0 + (y1 - y0) * t) + r.randint(-25, 25))))
        return self.city_point()

    def build(self):
        r = self.rng
        buttons = {b[1]: b for b in self.sidebar_buttons() if b[0] == 0x005C7370 and b[1] < 12}
        if not buttons:
            return self.other_window()
        # entry 0 housing, 1 roads, 2 clear land (menus 1, 3, 2: one building, chosen at once), the others menus
        p = r.random()
        k = 1 if p < 0.25 and 1 in buttons else 0 if p < 0.45 and 0 in buttons else \
            2 if p < 0.52 and 2 in buttons else r.choice(sorted(buttons))
        t, k, x, y, bw, bh = buttons[k]
        self.click(x + r.randint(2, max(2, bw - 3)), y + r.randint(2, max(2, bh - 3)))
        self.wait(r.randint(4, 12))
        if self.g.window == 6:
            rows = self.menu_rows()
            if rows and r.random() < 0.9:
                x0, y0, x1, y1 = r.choice(rows)
                self.click(r.randint(x0 + 2, max(x0 + 2, x1 - 3)), r.randint(y0 + 2, max(y0 + 2, y1 - 3)))
            else:
                sx = self.g.u32(0x00F53F64)
                self.click(sx - r.randint(10, 260), r.randint(60, 600))
            self.wait(r.randint(4, 10))
            if self.g.window == 6:  # a submenu, or a miss
                rows = self.menu_rows()
                if rows and r.random() < 0.8:
                    x0, y0, x1, y1 = r.choice(rows)
                    self.click(r.randint(x0 + 2, max(x0 + 2, x1 - 3)), r.randint(y0 + 2, max(y0 + 2, y1 - 3)))
                    self.wait(6)
        if self.g.window != WINDOW_CITY:
            return True
        cx0, cy0, cx1, cy1 = self.city_rect()
        for _ in range(r.randint(1, 4)):
            if k == 1:   # roads: a drag, remembered for the buildings that follow
                x0, y0 = self.near_road() if self.roads and r.random() < 0.5 else self.city_point()
                dx, dy = r.choice([(1, 0.5), (1, -0.5), (-1, 0.5), (-1, -0.5)])
                n = r.randint(40, 300)
                x1, y1 = max(cx0, min(cx1, int(x0 + dx * n))), max(cy0, min(cy1, int(y0 + dy * n)))
                self.drag(x0, y0, x1, y1, r.randint(2, 6))
                self.roads.append((x0, y0, x1, y1))
            else:
                x0, y0 = self.near_road()
                if r.random() < (0.6 if k in (0, 2) else 0.25):
                    dx, dy = r.randint(-120, 120), r.randint(-70, 70)
                    self.drag(x0, y0, max(cx0, min(cx1, x0 + dx)), max(cy0, min(cy1, y0 + dy)), r.randint(1, 5))
                else:
                    self.click(x0, y0)
            self.wait(r.randint(2, 30))
            if self.g.window != WINDOW_CITY:
                return True
        if r.random() < 0.5:
            x0, y0 = self.city_point()
            self.click(x0, y0, right=True)  # leaves the building mode
            self.wait(5)
        return True

    def sidebar_other(self):
        r = self.rng
        buttons = [b for b in self.sidebar_buttons() if not (b[0] == 0x005C7370 and b[1] < 12)]
        if not buttons:
            return self.other_window()
        t, k, x, y, bw, bh = r.choice(buttons)
        self.click(x + r.randint(1, max(1, bw - 2)), y + r.randint(1, max(1, bh - 2)), right=r.random() < 0.1)
        return self.wait(r.randint(10, 40))

    # the top menus (x of the title, rows): rows at 42 + 20 k (File's last row, Exit game, left out)
    MENUS = ((20, 5), (80, 8), (128, 6), (200, 12))

    def top_menu(self):
        r = self.rng
        x, rows = r.choice(self.MENUS)
        self.click(x, 8)
        self.wait(r.randint(4, 10))
        if self.g.window == 3:
            if r.random() < 0.1:
                return self.click(r.randint(0, 900), r.randint(100, 600)) and self.wait(5)
            self.click(x - 5 + r.randint(5, 80), 42 + 20 * r.randint(0, rows - 1) + r.randint(-5, 5))
            self.wait(r.randint(10, 30))
        return True

    def save_load(self):
        """File > Save game (a name typed, Return) or File > Load game (a saved game of the list, OK)"""
        r = self.rng
        self.click(20, 8)
        self.wait(6)
        if self.g.window != 3:
            return True
        load = r.random() < 0.4
        self.click(40, 82 if load else 102)
        self.wait(20)
        if self.g.window in (WINDOW_CITY, 3):
            return True
        if load:
            self.fclick(400, 286 + 16 * r.randint(0, 4))
            self.wait(10)
            self.fclick(602, 491)
            return self.wait(120)
        if r.random() < 0.85:
            if r.random() < 0.7:
                for _ in range(r.randint(0, 20)):
                    self.act('key Backspace')
                    self.wait(1)
                self.act('text s%d' % r.randint(0, 3))
                self.wait(10)
            self.act('key Return')
            self.wait(60)
        return True

    def keys(self):
        r = self.rng
        p = r.random()
        if p < 0.45:   # scrolling and rotation move the city under the remembered roads
            self.roads = []
        if p < 0.35:   # scrolling
            k = r.choice(['Left', 'Right', 'Up', 'Down'])
            for _ in range(r.randint(1, 6)):
                self.act('key %s' % k)
                self.wait(r.randint(1, 4))
            return True
        if p < 0.45:   # rotation
            return self.act('key %s' % r.choice(['Home', 'End', 'PageUp', 'PageDown'])) and self.wait(r.randint(5, 30))
        if p < 0.6:    # speed, pause (and the pause ended a little later, mostly)
            c = r.choice(['[', ']', ']', 'p'])
            self.act('text %s' % c)
            if c == 'p' and r.random() < 0.85:
                self.wait(r.randint(4, 100))
                self.act('text p')
            return self.wait(r.randint(4, 40))
        if p < 0.75:   # overlays
            c = r.choice('fdctwhxyz')
            return self.act('text %s' % c) and self.wait(r.randint(10, 60))
        if p < 0.85:   # advisors
            c = r.choice('`1234567890-=')
            return self.act('text %s' % c) and self.wait(r.randint(10, 40))
        if p < 0.9:
            return self.act('text %s' % r.choice('lm ')) and self.wait(r.randint(5, 20))
        if p < 0.94:
            return self.act('key %s' % r.choice(['F1', 'F2', 'F3', 'F4', 'Space', 'Insert', 'Delete', 'Backspace',
                                                 'Return'])) and self.wait(r.randint(5, 20))
        if p < 0.95:   # the resolution keys (F8 asks first: the box's first button), back to 1024 x 768 mostly.
            # A mode change opens window 62 (keep this resolution?, reverted after 10 s to the window it came
            # from): no second key while it is open, or it returns to itself and reverts every frame (the
            # original does too; the oracle's emulated heap runs out after about 60 mode changes)
            self.act('key %s' % r.choice(['F7', 'F8', 'F9']))
            self.wait(r.randint(10, 200))
            if self.g.window == 62:
                if r.random() < 0.5:
                    self.wait(450)  # reverted
                else:
                    self.other_window()
            if self.g.window == WINDOW_CITY and r.random() < 0.7:
                self.act('key F9')
                self.wait(r.randint(10, 60))
                if self.g.window == 62 and r.random() < 0.5:
                    self.wait(450)
            return self.wait(10)
        self.act('answer 1')  # the quit box: No (Yes ends the program)
        return self.act('key Escape') and self.wait(r.randint(10, 40))

    def edge_scroll(self):
        r = self.rng
        self.roads = []
        w, h = self.screen()
        x, y = r.choice([(0, r.randint(30, h - 30)), (w - 1, r.randint(30, h - 30)),
                         (r.randint(30, w - 200), 0), (r.randint(30, w - 200), h - 1)])
        self.act('move %d %d' % (x, y))
        self.wait(r.randint(5, 60))
        x0, y0 = self.city_point()
        return self.act('move %d %d' % (x0, y0))

    def city(self):
        r = self.rng
        if r.random() < self.idle:  # long sessions: the city left to run (months of ticks)
            return self.wait(int(r.choice([400, 800, 1500, 3000]) * r.uniform(0.5, 1.0)))
        p = r.random()
        if p < 0.30:
            return self.build()
        if p < 0.40:
            x, y = self.city_point()
            return self.click(x, y, right=True) and self.wait(r.randint(10, 40))
        if p < 0.48:
            return self.sidebar_other()
        if p < 0.55:
            return self.top_menu()
        if p < 0.58:
            return self.save_load()
        if p < 0.75:
            return self.keys()
        if p < 0.80:
            return self.edge_scroll()
        if p < 0.85:
            x, y = self.city_point()
            return self.click(x, y) and self.wait(r.randint(2, 20))
        if p < 0.88:
            x, y = self.city_point()
            return self.act('move %d %d' % (x, y)) and self.wait(r.randint(5, 50))
        return self.wait(int(r.choice([60, 200, 400, 800, 1500, 3000]) * r.uniform(0.5, 1.0)))

    def run(self):
        while self.g.alive and self.g.frame < self.frames:
            if self.orphan_stop and os.getppid() == 1:  # a pool worker whose parent is gone
                break
            w = self.g.window
            if self.g.frame < 30:  # the start-up (the title comes up)
                self.wait(30)
                continue
            if w == WINDOW_CITY:
                self.outside = 0
                self.in_city = True
                ok = self.city()
            elif w in FRONTEND and not self.in_city or w in (60, 0, 54, 47, 48, 13, 23, 52, 53, 59, 61):
                ok = self.frontend()
            else:
                self.outside += 1
                if self.outside > 12:
                    # back towards the city: right click and Esc (the quit box answered No)
                    self.outside = 0
                    self.click(512, 384, right=True)
                    self.wait(5)
                    if self.g.window != WINDOW_CITY:
                        self.act('answer 1')
                        self.act('key Escape')
                        ok = self.wait(10)
                    ok = True
                else:
                    ok = self.other_window()
            if not ok:
                break
        return self.g.alive


def read_gaps(player):
    """gaps.log lines of a player directory: (address, kind, window, frame)"""
    out = []
    path = os.path.join(player, 'gaps.log')
    if os.path.exists(path):
        for line in open(path):
            # 0xADDR KIND... window W frame F
            parts = line.split()
            if len(parts) >= 5 and parts[-4] == 'window':
                out.append((parts[0], ' '.join(parts[1:-4]), int(parts[-3]), int(parts[-1])))
    return out


def prefix(lines, at, frame, tail=('wait 2', 'quit')):
    """the session's lines that act up to `frame` (a minimal reproduction of what happens at it)"""
    keep = [l for l, a in zip(lines, at) if a < frame and l != 'quit']
    return keep + list(tail)


def generate(seed, frames, out, mode=None, verbose=False, display=None):
    rng = random.Random('mode-%s' % seed)
    mode = mode or rng.choice(MODES + ['load', 'load'])  # corpus cities: the most varied
    if display is None:
        display = rng.choice([(1024, 768)] * 4 + [(800, 600), (640, 480)])
    os.makedirs(out, exist_ok=True)
    player = os.path.join(out, '%s.player' % seed)
    dat = os.path.join(ROOT, 'local', 'oracle', 'phase8', 'fuzz', 'family.dat')
    saves = make_player(player, mode, rng, dat, display)
    work = player + '.run'
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(player, work)
    g = Game(work, os.path.join(out, '%s.gen.log' % seed))
    f = Fuzzer(seed, mode, frames, g)
    f.orphan_stop = WORKER
    g.lines.append('# fuzz seed %s mode %s frames %d display %dx%d' % (seed, mode, frames, display[0], display[1]))
    g.at.append(0)
    g.wins.append(-1)
    for name, rel in saves:
        g.lines.append('# save %s %s' % (name, rel))
        g.at.append(0)
        g.wins.append(-1)
    f.run()
    rc = g.close()
    session = os.path.join(out, '%s.txt' % seed)
    with open(session, 'w') as fh:
        fh.write('\n'.join(g.lines) + '\n')
    with open(os.path.join(out, '%s.json' % seed), 'w') as fh:
        json.dump({'seed': str(seed), 'mode': mode, 'display': list(display), 'frames': g.frame, 'status': rc, 'at': g.at, 'windows': g.wins,
                   'gaps': read_gaps(work)}, fh)
    if verbose:
        print('fuzz: seed %s mode %s %dx%d: %d lines, %d frames, exit %s' % (seed, mode, display[0], display[1],
                                                                          len(g.lines), g.frame, rc))
    return session, player, rc


FUZZ = os.path.join(ROOT, 'local', 'oracle', 'phase8', 'fuzz')
WORKER = False  # in a pool worker (run -j): its work stops when the parent is gone


def worker_init():
    global WORKER
    WORKER = True
    import oracle_play_check
    oracle_play_check.STOP_ORPHAN = True


def run_seed(seed, frames, mode=None, oracle=True):
    """one seed: the session made on the game (gaps, crash), then compared with the original (stops at the
    first differing frame). Returns a result dict; the cases are written to FUZZ/cases."""
    out = os.path.join(FUZZ, 'sessions')
    cases = os.path.join(FUZZ, 'cases')
    os.makedirs(cases, exist_ok=True)
    session, player, rc = generate(seed, frames, out, mode)
    meta = json.load(open(os.path.join(out, '%s.json' % seed)))
    lines = open(session).read().splitlines()
    at = meta['at']
    res = {'seed': str(seed), 'mode': '%s %dx%d' % ((meta['mode'],) + tuple(meta.get('display', (1024, 768)))), 'frames': meta['frames'], 'status': rc, 'gaps': [],
           'crash': None, 'diff': None, 'compared': 0, 'equal': None}

    def case(name, frame):
        path = os.path.join(cases, name)
        with open(path, 'w') as fh:
            fh.write('\n'.join(prefix(lines, at, frame)) + '\n')
        return os.path.relpath(path, FUZZ)
    for addr, kind, window, frame in meta['gaps']:
        res['gaps'].append((addr, kind, window, frame, case('%s-gap-%s.txt' % (seed, addr), frame + 1)))
    if rc not in (0, None):
        res['crash'] = (rc, meta['frames'], case('%s-crash.txt' % seed, 10 ** 9),
                        open(os.path.join(out, '%s.gen.log' % seed)).read()[-400:])
    if oracle and rc == 0:
        import oracle_play_check as opc
        report = []
        work = os.path.join(FUZZ, 'work', str(seed))
        r = opc.check(session, tuple(meta.get('display', (1024, 768))), keep=work, player_src=player, stop=True,
                      report=report.append)
        res['compared'] = r['frames']
        res['equal'] = not r['bad']
        res['notes'] = [l.strip() for l in report if 'UNINIT' in l]
        report = [l for l in report if 'UNINIT' not in l]
        if r['bad']:
            k = r['bad'][-1]
            res['diff'] = (k, '\n'.join(report), case('%s-diff-f%d.txt' % (seed, k), k))
        else:
            import shutil
            shutil.rmtree(work, ignore_errors=True)
    return res


def known_findings():
    path = os.path.join(FUZZ, 'findings.md')
    return open(path).read() if os.path.exists(path) else ''


def record(res, log):
    """findings.md lines for a seed's new findings (gaps by address once), and a line in runs.log"""
    known = known_findings()
    new = []
    s = res['seed']
    for addr, kind, window, frame, case in res['gaps']:
        if addr not in known and addr not in ''.join(new):
            new.append('- gap %s (%s): seed %s (%s), window %d, frame %d; case %s' % (addr, kind, s, res['mode'],
                                                                                    window, frame, case))
    if res['crash']:
        rc, frame, case, tail = res['crash']
        new.append('- crash: seed %s (%s), exit %s near frame %d; case %s; %s' % (
            s, res['mode'], rc, frame, case, tail.strip().splitlines()[-1] if tail.strip() else ''))
    for note in res.get('notes', [])[:3]:
        new.append('- note: seed %s (%s), %s' % (s, res['mode'], note))
    if res['diff']:
        k, text, case = res['diff']
        first = text.strip().splitlines()[0].strip() if text.strip() else ''
        new.append('- difference: seed %s (%s), %s; case %s' % (s, res['mode'], first, case))
        detail = [l for l in text.splitlines()[1:8]]
        new += ['  ' + l.strip() for l in detail]
    if new:
        path = os.path.join(FUZZ, 'findings.md')
        if not os.path.exists(path):
            open(path, 'w').write('# Phase 8.3 randomised sessions: findings\n\n'
                                  'Made by tools/oracle/fuzz.py run; cases are session prefixes (build/imhotep '
                                  '--script; replay with tests/differential/oracle_play_check.py CASE --player '
                                  'sessions/SEED.player --stop).\n\n')
        with open(path, 'a') as fh:
            fh.write('\n'.join(new) + '\n')
    line = 'seed %s mode %s frames %d status %s compared %d %s gaps %d%s' % (
        s, res['mode'], res['frames'], res['status'], res['compared'],
        'equal' if res['equal'] else ('DIFF@%d' % res['diff'][0] if res['diff'] else 'not compared'),
        len(res['gaps']), ' CRASH' if res['crash'] else '')
    with open(log, 'a') as fh:
        fh.write(line + '\n')
    print('fuzz: ' + line, flush=True)


def run(seeds, frames, mode, jobs, oracle):
    from concurrent.futures import ProcessPoolExecutor, as_completed
    log = os.path.join(FUZZ, 'runs.log')
    if jobs <= 1:
        for seed in seeds:
            record(run_seed(seed, frames, mode, oracle), log)
        return
    with ProcessPoolExecutor(min(jobs, 2), initializer=worker_init) as pool:
        futs = {pool.submit(run_seed, seed, frames, mode, oracle): seed for seed in seeds}
        for fut in as_completed(futs):
            try:
                record(fut.result(), log)
            except Exception as e:
                with open(log, 'a') as fh:
                    fh.write('seed %s ERROR %r\n' % (futs[fut], e))
                print('fuzz: seed %s ERROR %r' % (futs[fut], e), flush=True)


def main():
    argv = sys.argv[1:]
    if len(argv) < 2 or argv[0] not in ('gen', 'run'):
        print(__doc__)
        return 2
    frames, out, mode, jobs = 3000, os.path.join(FUZZ, 'sessions'), None, 1
    for opt in ('--frames', '--out', '--mode', '-j'):
        if opt in argv:
            k = argv.index(opt)
            v = argv[k + 1]
            del argv[k:k + 2]
            if opt == '--frames':
                frames = int(v)
            elif opt == '--out':
                out = v
            elif opt == '--mode':
                mode = v
            else:
                jobs = int(v)
    oracle = '--no-oracle' not in argv
    argv = [a for a in argv if a != '--no-oracle']
    seeds = []
    for a in argv[1:]:
        if '-' in a and a.replace('-', '').isdigit():
            lo, hi = a.split('-')
            seeds += [str(x) for x in range(int(lo), int(hi) + 1)]
        else:
            seeds.append(a)
    if argv[0] == 'gen':
        for seed in seeds:
            generate(seed, frames, out, mode, verbose=True)
        return 0
    run(seeds, frames, mode, jobs, oracle)
    return 0


if __name__ == '__main__':
    sys.exit(main())
