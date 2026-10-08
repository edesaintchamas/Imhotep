"""Imhotep - the sound driver in the oracle: a stand-in for Miles (MSS32.DLL) and the translation of
its call log into the port's sound requests.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

The original plays sounds through Miles (41 AIL_* imports), wrapped by the sound object 0x00E92BF8
(docs/presentation.md 6.1). Without a sound device (the oracle's default) every AIL_* stub returns 0
and the sound start-up 0x005394F0 is not run, so the sound flag 0x00F55B88 stays 0 and the game plays
nothing. Game(sound=True) (or IMHOTEP_ORACLE_SOUND=1) runs the start-up at its place in WinMain with
FakeMiles below, a driver that works: the start-up succeeds, the sound tables are built from the
game's files (music.txt, the wav names), and the game's sound logic runs as on a machine with a sound
card. Every AIL_* call is logged in Game.ail_log (emu.py); requests() turns the log into the port's
request vocabulary (core/sim/platform.h), one line per request:

  stream_play CH LOOPS VOL OPENED NAME   0x0055F449: open NAME on channel CH (OPENED 1: it plays,
                                         with LOOPS and VOL; 0: it could not be opened, VOL is '-')
  stream_stop CH                         0x0055F561, and 0x0055F449 before a new stream
  stream_playing CH -> R                 AIL_stream_status = 4 (0x0055F5A1, 0x0055F32F..0x0055F3EB)
  stream_volume CH -> V                  0x0055F5F4
  stream_set_volume CH V                 0x0055F639, 0x0055F32F..0x0055F3EB
  sample_play VOL PAN LOOPS NAME         0x0055F0E8: a sample started (NAME: the file read for it)

state_replay --sound FILE checks the port's requests against these lines, in order, and answers the
port's queries (stream opened, playing, volume) with the original's answers.

FakeMiles's behaviour (the driver's, not the game's): a stream opens when its file exists in the game
directory (names matched case-insensitively) and then plays until it is closed or end_stream() ends
it; its volume is what was last set. Samples are over by the next request (AIL_sample_status 2), so
every sample request gets the first handle; AIL_file_read succeeds for existing files.
"""
import os
import subprocess

from emu import GAME_DIR, ROOT

SMP_DONE, SMP_PLAYING = 2, 4
DRIVER = 0x5D000001
SAMPLE_BASE = 0x5A000001
STREAM_BASE = 0x5B000001
FILE_BASE = 0x5C000000


def game_file_exists(name):
    """a game file by the path the game builds (case-insensitive, '\\' or '/')"""
    path = GAME_DIR
    for part in name.replace('\\', '/').split('/'):
        if not part:
            continue
        if not os.path.isdir(path):
            return False
        match = [f for f in os.listdir(path) if f.lower() == part.lower()]
        if not match:
            return False
        path = os.path.join(path, match[0])
    return os.path.isfile(path)


class FakeMiles:
    """the AIL_* functions the game uses, as a working driver; notes[i] = a string for log entry i (the
    stream's or file's name)"""

    def __init__(self, g):
        self.g = g
        self.samples = 0
        self.streams = {}   # handle -> {'name', 'volume', 'loops', 'started', 'ended'}
        self.next_stream = STREAM_BASE
        self.files = {}     # AIL_file_read pointer -> name
        self.next_file = FILE_BASE
        self.notes = {}
        h = g.handlers
        for name, n, fn in (
                ('AIL_startup', 0, lambda e: 0),
                ('AIL_shutdown', 0, lambda e: 0),
                ('AIL_set_preference', 2, lambda e, p, v: 0),
                ('AIL_waveOutOpen', 4, self.wave_out_open),
                ('AIL_allocate_sample_handle', 1, self.allocate_sample),
                ('AIL_release_sample_handle', 1, lambda e, s: 0),
                ('AIL_init_sample', 1, lambda e, s: 0),
                ('AIL_set_sample_file', 3, lambda e, s, p, b: 1),
                ('AIL_set_sample_loop_count', 2, lambda e, s, n: 0),
                ('AIL_set_sample_pan', 2, lambda e, s, n: 0),
                ('AIL_set_sample_volume', 2, lambda e, s, n: 0),
                ('AIL_set_sample_reverb', 4, lambda e, s, a, b, c: 0),
                ('AIL_start_sample', 1, lambda e, s: 0),
                ('AIL_end_sample', 1, lambda e, s: 0),
                ('AIL_sample_status', 1, lambda e, s: SMP_DONE),
                ('AIL_sample_loop_count', 1, lambda e, s: 0),
                ('AIL_file_read', 2, self.file_read),
                ('AIL_mem_free_lock', 1, lambda e, p: 0),
                ('AIL_open_stream', 3, self.open_stream),
                ('AIL_close_stream', 1, self.close_stream),
                ('AIL_start_stream', 1, self.start_stream),
                ('AIL_stream_status', 1, self.stream_status),
                ('AIL_stream_volume', 1, lambda e, s: self.streams[s]['volume'] if s in self.streams else 0),
                ('AIL_set_stream_volume', 2, self.set_stream_volume),
                ('AIL_set_stream_loop_count', 2, self.set_stream_loops)):
            h['_%s@%d' % (name, 4 * n)] = (n, fn)

    def wave_out_open(self, e, out, a, b, fmt):
        e.set_u32(out, DRIVER)
        return 0

    def allocate_sample(self, e, driver):
        self.samples += 1
        return SAMPLE_BASE + self.samples - 1

    def file_read(self, e, name, dest):
        text = e.cstr(name)
        self.notes[len(self.g.ail_log)] = text
        if not game_file_exists(text):
            return 0
        p = self.next_file
        self.next_file += 0x100
        self.files[p] = text
        return p

    def open_stream(self, e, driver, name, mem):
        text = e.cstr(name)
        self.notes[len(self.g.ail_log)] = text
        if driver != DRIVER or not game_file_exists(text):
            return 0
        s = self.next_stream
        self.next_stream += 1
        self.streams[s] = {'name': text, 'volume': 127, 'loops': 1, 'started': False, 'ended': False}
        return s

    def close_stream(self, e, s):
        self.streams.pop(s, None)
        return 0

    def start_stream(self, e, s):
        if s in self.streams:
            self.streams[s]['started'] = True
        return 0

    def stream_status(self, e, s):
        t = self.streams.get(s)
        return SMP_PLAYING if t and t['started'] and not t['ended'] else SMP_DONE

    def set_stream_volume(self, e, s, v):
        if s in self.streams:
            self.streams[s]['volume'] = v
        return 0

    def set_stream_loops(self, e, s, n):
        if s in self.streams:
            self.streams[s]['loops'] = n
        return 0

    def end_stream(self, channel):
        """the stream playing on `channel` (its handle in the sound object, 0x00E92DD0 + 4 * channel)
        reaches its end: AIL_stream_status says it is done from now on"""
        s = self.g.u32(0x00E92DD0 + 4 * channel)
        if s in self.streams:
            self.streams[s]['ended'] = True


# the Miles wrappers of the sound object 0x00E92BF8 (dis): [start, end) -> where the channel is
# (an int: fixed; ('frame', k): the k-th stack argument of the wrapper, [ebp + 8 + 4k])
WRAPPERS = [
    (0x0055F32F, 0x0055F38D, 1),            # speech volume
    (0x0055F38D, 0x0055F3EB, 0),            # music volume
    (0x0055F3EB, 0x0055F449, 2),            # ambient volume
    (0x0055F449, 0x0055F561, ('frame', 1)),  # open and play (name, channel, loops)
    (0x0055F561, 0x0055F67E, ('frame', 0)),  # close, playing, volume, set volume (channel, ...)
]


def channel_of(ret, frame):
    for lo, hi, where in WRAPPERS:
        if lo <= ret < hi:
            return where if isinstance(where, int) else frame[where[1]]
    return None


def requests(log, notes, start=0, end=None, scopes=None):
    """the port's request lines for the AIL calls log[start:end] (FakeMiles.notes: names). scopes: when
    given, only the calls made inside a recorded call (replay.py) to one of these functions count (the
    platform requests: the port makes them; a replayed function's sound calls are in its record)"""
    out = []
    files = {}      # AIL_file_read pointer -> name
    sample = {}     # handle -> {'file', 'volume', 'pan', 'loops'}
    entries = list(enumerate(log))[:end]  # the earlier calls give the files and sample settings
    i = 0
    while i < len(entries):
        k, (name, args, result, ret, frame, scope) = entries[i]
        i += 1
        ch = channel_of(ret, frame)
        put = out.append if k >= start and (scopes is None or scope is None or scope in scopes) else (lambda line: None)
        if name == 'AIL_open_stream':
            loops = frame[2]
            if result == 0:
                put('stream_play %s %d - 0 %s' % (ch, loops, notes.get(k, '?')))
                continue
            # 0x0055F449 then sets the loop count and the volume and starts it
            folded = [e for _, e in entries[i:i + 3]]  # (name, args, ...)
            names = [e[0] for e in folded]
            if names != ['AIL_set_stream_loop_count', 'AIL_set_stream_volume', 'AIL_start_stream']:
                put('stream_play %s %d ? 1 %s (unexpected %s)' % (ch, loops, notes.get(k, '?'), names))
                continue
            i += 3
            put('stream_play %s %d %d 1 %s' % (ch, folded[0][1][1], folded[1][1][1], notes.get(k, '?')))
        elif name == 'AIL_close_stream':
            put('stream_stop %s' % ch)
        elif name == 'AIL_stream_status':
            put('stream_playing %s -> %d' % (ch, 1 if result == SMP_PLAYING else 0))
        elif name == 'AIL_stream_volume':
            put('stream_volume %s -> %d' % (ch, result))
        elif name == 'AIL_set_stream_volume':
            put('stream_set_volume %s %d' % (ch, args[1]))
        elif name == 'AIL_file_read':
            if result:
                files[result] = notes.get(k, '?')
        elif name == 'AIL_set_sample_file':
            sample.setdefault(args[0], {})['file'] = files.get(args[1], '?')
        elif name == 'AIL_set_sample_loop_count':
            sample.setdefault(args[0], {})['loops'] = args[1]
        elif name == 'AIL_set_sample_pan':
            sample.setdefault(args[0], {})['pan'] = args[1]
        elif name == 'AIL_set_sample_volume':
            sample.setdefault(args[0], {})['volume'] = args[1]
        elif name == 'AIL_start_sample':
            s = sample.get(args[0], {})
            put('sample_play %d %d %d %s' % (s.get('volume', -1), signed(s.get('pan', -1)), s.get('loops', -1),
                                                    s.get('file', '?')))
    return out


def signed(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


_PLATFORM = {}


def platform_functions():
    """the addresses of the platform requests (partial 2 in build/state_run --list): the calls whose
    sound the port makes itself"""
    build = os.environ.get('IMHOTEP_BUILD', 'build')
    if build not in _PLATFORM:
        _PLATFORM[build] = _platform_functions()
    return _PLATFORM[build]


def _platform_functions():
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_run'), '--list'],
                       capture_output=True, text=True, check=True)
    return {int(line.split()[0], 16) for line in r.stdout.splitlines() if line.split()[4] == '2'}
