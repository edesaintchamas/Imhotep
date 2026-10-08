# Presentation layer (roadmap phase 7): survey

Reference: GOG Pharaoh + Cleopatra 2.1. This file surveys how `Pharaoh.exe` puts the game on the
screen and in the speakers: the frame loop, DirectDraw and the back buffer, the blitters, the city
view, the windows, input, sound, music and video. It asks whether these can be ported the way
phases 1–6 were (from the binary, each function checked against the original run in the emulator)
and lays out a plan for that. Nothing here is ported yet.

Evidence marks, as in docs/commands.md:

- **dis**: read in the disassembly (`tools/oracle/disasm.py`) or the instruction xref.
- **dec**: read in the Ghidra decompilation only.
- **run**: observed in the oracle. The scripts are throwaway scratch files, not in the repo; §4.1
  describes the method so the runs can be reproduced.
- **assumed**: an interpretation (usually a name or a player-facing meaning).

## 0. Verdict

**Feasible. The original's own drawing code runs headless in the oracle, producing pixel-exact
frames (run).**

- WinMain's DirectDraw set-up `0x004CF960` was given emulated DirectDraw COM objects and stubbed
  window calls. The back buffer is then a plain memory block whose address the game keeps in
  `0x00E92E18`.
- After that set-up, the whole per-frame function `0x004D6B60` runs unmodified on a loaded save
  (§4). It takes 0.04–0.1 s per frame in the emulator, more for windows that load art on first use.
- It draws the city view, sidebar, minimap, top menu and texts at 640×480, 800×600 and 1024×768, in
  both 555 and 565 modes.
- In a sweep, 87 of 88 window ids drew a screen without error. Id 3 faulted because it needs a menu
  opened first.
- Two runs from the same state give identical buffers. The frame reads the clock, the cursor and the
  keys only through imports, which the oracle supplies.
- All drawing writes go through one pointer and one pitch (§2.3). A ported renderer can therefore
  draw into a plain array and be compared byte for byte with the original's buffer.

The survey also found that **frames are not pure presentation**. The per-frame code writes state
that the simulation reads (§1.4, run): the CRT `rand` seed, the sprite grid (save chunk 14), the
image grid (chunk 7), city goods flags and the ambient slots (chunk 92). A city advanced with frames
differs from the same city advanced by bare ticks, down to figure records. The port has to treat
these frame-side writes as simulation, in core/, with the clock as an input.

## 1. The frame loop

### 1.1 Call path (dis)

WinMain `0x00413ED0` runs its message loop. When no message is pending it calls `0x004143C0`, which
checks for lost surfaces (`IsLost`, vtable +0x60; `Restore` through `0x004CFFE0`) and then calls the
frame `0x004D6B60`.

`0x004D6B60` does the following, in order (dis for the order, dec for the conditions):

| # | Call | Role |
|---|---|---|
| 1 | `++u32 0x00E390EC`; `0x00E92E0C := timeGetTime()` | frame counter; **frame clock** (every later use of time in the frame reads `0x00E92E0C`) |
| 2 | `0x0040F270` | animation timers: first clears the 51 u8 flags at `0x00605B38` (rep stosd, stosw, stosb), then sets each timer's flag when its period has passed and restarts it (dis; corrected by the port, core/frame/frame.c: the earlier "k = 0..12" read of the decompile was wrong) |
| 3 | `0x004DDCB0` → n | pacing (§1.2): returns 10000 or 0 |
| 4 | loop while n > 0: `++0x00E94DBC`; **`0x004D7090` (tick)**; then `0x004D5C30` | **one tick per frame**: after the tick, n is reset to 0 unless the fast-forward flag `u8 0x00E3908C` is set (set by `0x00452370`, the day cascade; the loop then runs up to 10000 ticks and stops on a right button up seen with `PeekMessageA`) (dis 0x004D6B94–0x004D6C4B). `0x004D5C30` is the autosave after a tick (`autosave.sav`, `autosave_replay.sav`, `autosave_history.sav`; writes city +0x4724 and calls the saver `0x004D4E20`) |
| 5 | `0x004CF210` | mouse sampling (§5) |
| 6 | `0x004D71C0(0)` | full redraw of the current window's background when `u8 0x00E38DCA` ≠ 0 (switch on the window id, §3.4) |
| 7 | `0x004D6070` | `0x00E38DCA := 1` if `0x00E94D98` ≠ 0 |
| 8 | `0x004D75D0` | per-frame draw of the current window (city view every frame, §3) |
| 9 | `0x004D8310` | input dispatch of the current window (22 KB; docs/commands.md §1.1) |
| 10 | `0x004CF1F0` | clears the one-frame key flags `0x00E38DFC`, `0x00E38E02`, `0x00E38DF6` |
| 11 | `0x004C8750`, `0x004C9750` | tooltip id `0x00D3A6E4` from what the mouse hovers, with a delay measured on the frame clock (`0x00D3A6C8`) (dec) |
| 12 | `0x0051F0A0(0)`, `0x0051F680(1)` | top menu bar texts (money, population, date) and their refresh (dec) |
| 13 | `0x004E2720` | on-screen text messages (dec, assumed) |
| 14 | if `0x00D3A6E4`: `0x004CABD0` (tooltip draw), `0x00405F80` when it is 0x23, `0x004CA930` | tooltip |
| 15 | `0x00536620` | text drawn over everything (dec, assumed) |
| 16 | **`0x004D0280(0)`** (present) | unless a Bink video is open (`0x006FDC40`), in which case `0x005605A0` / `0x00416BC0` drive it |
| 17 | `0x0041D560` | city ambient-sound slots (§6.2) |
| 18 | window-dependent `0x004CF460(n)` | mouse cursor shape |
| 19 | music and ambient | battle music, `0x00539820` music, `rand()` → `0x0053ACF0` random city sound, `0x0053B960` ambient stream, fades `0x0053B6F0` (§6) |

### 1.2 Pacing and game speed (dis/dec, table read from the exe)

`0x004DDCB0` measures `dt = timeGetTime() − 0x00E94DB8` (negative → 10000). A tick is due when
all of the following hold:

- `i = (100 − speed) / 10 < 10`, where speed is `i32 0x00E38E6C`, the speed percentage (10–100
  in steps of 10, set by `0x00403320` from the `[` / `]` keys and the speed dialog; default 90 at
  `0x004147F0`; 100 in the saves seen, run);
- `u8 0x00E38E60` = 0. This is **the pause flag**, toggled by the P key in the WndProc (dis
  0x00415157–0x00415161; the WndProc also sets window 1 there). docs/commands.md §1.1 calls it
  "always 0"; that is wrong, because the WndProc is not a Ghidra function and its writes were
  missed. The command gates on it therefore mean "paused".
- the window allows ticks;
- `0x006059E4` = 0 (a construction drag is in progress, set by `0x00405C20`) and `0x00E38DCC` = 0
  (the map is scrolling, set by `0x004CF660`);
- `dt ≥ table[i] + 2` ms, with `table 0x005E1DB4 = {0, 20, 35, 55, 80, 110, 160, 240, 350, 500, 700}`.

The window allows ticks when its flag in `u32 0x005E1C50[window]` is 0. That table has 0 for ids 1,
2, 6, 8, 0x1D–0x20, 0x23, 0x27, 0x48, 0x49, 0x56, 0x57; ids 0x12 and 0x23 instead require
`0x00D3A6C4` = 1.

When a tick is due, `0x00E94DB8 := now`. The game therefore advances **at most one tick per frame**
(fast-forward aside). At 100 % speed it advances once every ≥ 2 ms, so the tick rate is the frame
rate. At lower speeds it waits `table[i] + 2` ms between ticks.

The present `0x004D0280` throttles nothing: it blits every frame. So there is no fixed frame rate.
Present, rendering and sound set the frame rate (assumed: how fast DirectDraw returned on the target
machines).

### 1.3 Present (dis/dec)

`0x004D0280(force)` adds `0x004D0040()` (ms since the last present, `timeGetTime`) to `0x00E92E08`.
It returns early when DirectDraw is not set up (`0x005E06B0` = 1, set by `0x004D0220` until it
succeeds) or when `0x00F54054` ≠ 0 and `0x00E92E04` ≠ 0.

- **Fade.** In some windows, once `0x0078D7A0` (> 0) is a multiple of 0x88, it fades the screen:
  `0x004D05F0`, then `0x004D0650` eight times, presenting each step.
- **Copy.** Otherwise, when 50 ms have accumulated, or `0x00E38DC8` (present now) is set, or force =
  1, it copies the back buffer to the screen with `0x004D03F0`:
  - **fullscreen** (`0x00E38E5C` ≠ 0): `BltFast(primary, 0, 0, back, rect 0x00E39130)`;
  - **windowed**: `Blt` to the client rectangle, converted by `0x004CBEA0`.
- **City windows.** For windows 1, 3, 0x1E, 0x1F, 0x20 and 0x49, while `0x0078D7A0` > 0, it first
  redraws the sidebar `0x0051E990` and calls `0x004D05F0(1, 1, 0, 0, 1)` (assumed: the earthquake or
  flash effect of `0x0078D7A0`).

**The screen therefore shows the back buffer at most every 50 ms (≈ 20 fps)**, although the frame
loop runs faster. Pixel checks compare the back buffer, not the presented screen.

### 1.4 What a frame writes into the state (run)

A write hook on the state mirror (0x0056F000–0x01267AA8) and on the simulation heap was set during
frames on Actium (window 1, 1024×768). Two passes were made:

- 2 frames with no tick: 1,325 addresses written in the first frame, 785 in the second; no write to
  the simulation heap.
- 50 frames with a running clock (one tick each), compared with 50 bare ticks of `0x004D7090` from
  the same loaded state: 125 differing runs.

Most writes are presentation globals that the tick never reads. These are:

- the clip rectangle `0x00D3B550`–`0x00D3B55C`;
- blitter scratch `0x00D3B4C0`–`0x00D3B5E0`;
- text cursor `0x00E20768`–`0x00E20799`;
- mouse `0x00E38DC0`–`0x00E38E40`;
- city-view scratch `0x0070E3C0`–`0x0070E490`;
- the view-to-tile table `0x0070E494` (0xE7 × 0x1C9 i32);
- tooltip `0x00D3A6C8`;
- sidebar `0x00F53DA0`–`0x00F53F74`;
- button tables `0x005C28AC`, `0x005C7538`, `0x005C91F0`;
- ambient counters `0x00F55B50`–`0x00F55B84`.

The following writes, however, reach state that the simulation reads or that is saved:

| State | Written by | Effect |
|---|---|---|
| CRT `rand` seed `0x00604950` | `0x004D6B60` calls `0x00562348` **unconditionally once per frame in window 1**, then 3–4 more in `0x0053ACF0` and 1 in `0x00539820` when sound, city sounds or music are on (dis/dec) | The simulation's figure actions draw from the same `rand` (`0x004AF140`, `0x004B4800`, …; 31 callers). Run: after 50 frames, figure +0x92 of figure 1 and figures created by `0x004B4800` differ from the bare-tick run. The original's `rand` sequence therefore depends on the frame rate and on the sound settings. |
| sprite grid, chunk 14 `0x00ADC36C` | `0x0040F2C0` (from the city-view passes `0x00436B60`, `0x00439F10`): advances a building's animation frame on visible tiles, when its timer flag `0x00605B38[k]` (step 2 above) is set (dec) | Run: 8 runs of bytes differ after 50 frames. The simulation reads chunk 14 (`core/figure/actions_a.c`, `actions_d.c`, `core/map/routing.c`, `core/city/day2_farm.c`). Whether these readers ever see an animated building tile is **open** (§10). |
| image grid, chunk 7 `0x00B7482C` | `0x00430EF0` (terrain pass) cycles the image ids of visible water tiles when the 60 ms flag `0x0070E3C4` is set (dis 0x0043134E) | Run: a diamond of visible tiles differs after 50 frames. Whether the simulation reads those image ids is **open**. |
| city goods flags `0x00EA41FC`, `0x00EA4205` (+ player) | `0x005596D0`, called by the storage-contents draw `0x00442280` (dec, run) | Same function as the scenario start's goods lists (ported in core/state/scenario_start.c). The flags only go from 0 to 1. Drawing a storage building makes them current. |
| ambient slots, chunk 92 `0x00702900` | `0x0041D560` (clock) and `0x00430EF0` → `0x0041D410` (visibility) (dec) | Saved. Nothing in the simulation reads them (§6.2). |
| building 0 type `0x007999E8` | `0x00430EF0` writes 0x27 before `0x0041D410(0, 2)` (dec, sound survey) | Building 0 is the null record. Whether anything reads its type is open. |
| hovered tile `0x00BCA2C4`–`0x00BCA308` | `0x00405F80` (frame step 14 and the input dispatch) | Input of the phase-5 commands (docs/commands.md §1.1). |
| autosave | `0x004D5C30` after each tick | Writes city +0x4724 (`0x00EA5F8C` + player) and a save file. |

**Consequence.** "Tick only" lockstep (phases 4–6) checks the simulation as an idle game with no
frames. A playable port must also run the frame-side writes above, in the original's order, with
the clock (and the cursor) as inputs. A test can then compare **frame by frame**: state and buffer
after each `0x004D6B60`, with scripted clock, cursor and keys (§9.2).

## 2. Drawing

### 2.1 DirectDraw set-up `0x004CF960` (dis/dec, run)

It is called once by WinMain after `0x004149D0` (the settings read). Its steps:

- It seeds the frame-tick shift registers `0x00D3B384` := 0x54657687, `0x00D3B380` := 0x72641663
  (they are also saved and loaded) and `0x00D3B37C`.
- **Resolution.** It sets the resolution from `0x00E38E68` (1 = 640×480, 2 = 800×600, 3 =
  1024×768; `0x004CFD60` sets the same from a parameter):
  - width `0x00E38DB0` = `0x00E38D9C` = `0x00D3B558`;
  - height `0x00E38DAC` = `0x00E38D98` = `0x00D3B550`;
  - offsets of a 640×480 layout `0x00E38DA8` = (w − 640)/2, `0x00E38DA4` = (h − 480)/2;
  - **pitch `0x00E38D94` = 2·w**; buffer size `0x00E38D8C`.
- **Window.** It registers the class `WinSJBClass` with WndProc `0x00414CD0`, loads 19 cursors
  (`0x00E39090`–`0x00E390D8`), creates the window `0x00E39140` and makes the Bink object
  (`0x004133D0`, `0x006FDC54`).
- **gfxBegin `0x004D0220`.** It calls `0x004D0060`, then the clearing `Blt`s `0x00426F60`, then sets
  `0x00E3912C` = 1 and `0x00E38DCA` = 1. `0x004D0060` does the following:
  - `0x00426B40`: `GetDeviceCaps`; `DirectDrawCreate` → `0x00E39148`; `SetCooperativeLevel` (8
    windowed / 0x13 fullscreen); in fullscreen, `SetDisplayMode(w, h, 16)`, retrying at lower
    resolutions with `0x004CFDC0`.
  - `0x00426FF0`: primary surface `0x00E39178` (caps 0x200). Back buffer `0x00E39174` from
    `0x00427480(w, h, 1)`: an off-screen plain surface in **system memory** (caps 0x840). It locks
    the back buffer once and keeps `lpSurface` in **`0x00E92E18`** (dis 0x0042724E–0x00427257:
    descriptor +0x24). It creates and attaches the clipper (windowed only).
  - **Pixel format** `0x00E92DFC` from `GetSurfaceDesc` (vtable +0x58) masks: red 0xFC00 → 3
    (655), green 0x07E0 → 2 (565), blue 0x3F → 1 (556), else 0 (555); 1 and 3 are forced to 2;
    `0x00E3914C` = 0x555 / 0x565.
  - `0x004D0480` on the object `0x006DE518`: locks the front and back surfaces once and keeps their
    pointers (+0x24, +0x28), size (+8, +0xC) and a 565 flag (+0x10).

The surfaces are never locked again for drawing. Every blitter writes through `0x00E92E18`, which
works because the back buffer is a system-memory surface whose memory does not move. Only the
Smacker player (`SmackToBuffer` into a locked surface) and the Bink player (`0x00413470`: Lock /
`BinkCopyToBuffer` / Unlock on the primary surface `0x00E39178`) lock surfaces (sound survey).

### 2.2 The oracle without it

`tools/oracle/game.py` skips `0x004CF960`, so all of the following are 0 in the oracle's states:

- the resolution globals;
- the sidebar x `0x00F53F64` (set by `0x0051E930` from the width);
- the city viewport `0x0070E430`–`0x0070E490` (`0x004303A0`, from `0x004DDEC0`, by width).

All of these are in the state mirror. With the emulated set-up (§4) they take the values of a real
run (1024×768: sidebar x 0x346, viewport `0x0070E430` = 0x2FD, `0x0070E438` = 0x346; run).
Changing the oracle's boot therefore changes these bytes in every reference state. This is
presentation state, but the lockstep and load checks must be re-run when the boot changes (§10).

### 2.3 Back buffer and blitters (dis/dec)

- **One target.** Every pixel write goes through `u32 0x00E92E18` (back buffer) + y·`i32
  0x00E38D94` (pitch) + 2·x. That is 43 functions referencing `0x00E92E18` (xref). Examples:
  - `0x0040FA40` (put pixel);
  - the plain, mirrored, masked and RLE blitters `0x0040FED0`–`0x00412xxx`;
  - the isometric footprint blitters `0x00427A20` (one 58×30 tile), `0x0042C740`, `0x0042C9A0`,
    `0x0042CEE0` (2×2 … 6×6), `0x0042D810`, `0x0042E680`;
  - the colour-tinted variants `0x00428C00`, `0x0042A160`, `0x0042B440` (taken when `0x0070E120` ≠
    0: the construction ghost's red / green tint 0x1FE3, 0xF863);
  - the screen clear `0x004CF1C0`.
- **Clip rectangle** `0x00D3B550` (bottom), `0x00D3B554` (top), `0x00D3B558` (right), `0x00D3B55C`
  (left). The city view sets it to its viewport and restores the whole screen after (`0x00430D30`,
  dec).
- **Image draw `0x0053E590`.** It is a thiscall on the graphics object (205 callers); its arguments
  are image id, x and y. It reads the 64-byte entry at `this + 0x180E48 + id·64` (docs/formats/
  graphics.md §5, §10) and picks the blitter by type, `compressed` and footprint size.
  - External images (`+0x34`) are read on first use from `Data\<name>.555` into the spare screen
    `0x00E92E1C` (`0x004CE480`) and converted (`0x004CDF20` / `0x004CDEB0`).
  - `0x0053F250` and `0x0053F4F0` are its variants for building tops and for tinted or
    alpha-masked images (dec, assumed); `0x0053F4A0` draws a sub-rectangle (minimap).
- **Pixel format.** The `.555` data is X1R5G5B5. In mode 2 the loader converts it once to 565
  (graphics.md §7, `0x004CDF20`, `0x004CDEB0`). The blitters copy 16-bit words with no further
  conversion. Run: the same frame drawn in mode 0 (555, data as stored) and in mode 2 (565,
  converted) both show natural colours when the buffer is decoded in that mode. This settles the
  "555 assumed" point of graphics.md §7 for the drawing side.
- **Fills.** `0x004CD9D0(x, y, w, h, colour)` and `0x004CDA40(x, y, w, h, colour)` fill rectangles
  (dec, assumed). The bordered UI panels are tiled images (`0x00425730` button table drawing,
  `0x004CD3E0` inner panel).
- **Text.** The fonts are images.
  - `0x004CC770(text, x, y, font, colour)` draws a string glyph by glyph through `0x004CCA60`
    (→ `0x0053E590`). The character map is the table `0x005E049C`.
  - The fonts are the image groups `0x00F56F84` (small), `0x00F56FB4` and `0x00F56FBC` (large),
    and others (6 px space).
  - Shadowed and outlined variants are selected by `0x00D3B510`, `0x00D3B514` and `0x00D3B518`.
  - `0x004CBF30` measures a string; `0x004CC720` centres one; `0x004CCE00` / `0x004CCE90` draw
    numbers; `0x004CC110(group, index)` sets the text cursor `0x00E20768` into
    `Pharaoh_Text.eng`.
  - **GDI is used only by the loading screen `0x004CF7B0`**: `GetDC` at 0x004CF7D2,
    `GetTextExtentPointA` at 0x004CF7EE and `TextOutA` at 0x004CF8C6 are the only call sites of
    GDI text in the exe (xref).
- **Image index.** This is the phase-1 work: core/graphics (`image_groups`, `sg3`),
  docs/formats/graphics.md.
  - The `.sg3` index loading `0x0053DFF0` is already ported (it writes state).
  - The `.555` read `0x0053F660` is the platform request `platform_pixels_load`, which returns a
    buffer outside the state.
  - The slot object (images at `this + 0x180E48`) is state. The pixel data the blitters read is
    not.

### 2.4 Determinism of a drawn frame (run)

Given the state mirror, the pixel data, the previous contents of the back buffer and the frame
inputs, a frame's buffer is a pure function. The frame inputs are:

- `timeGetTime`;
- `GetCursorPos`;
- `GetAsyncKeyState`;
- the messages the WndProc turned into globals.

Two oracle runs from the same save gave identical buffers (SHA-1 equal).

Many windows draw only their panel over what the buffer already holds: only the full redraw
`0x004D71C0` repaints the background. The previous buffer is therefore an input of the comparison,
or the test clears both buffers first.

## 3. The city view and the windows

### 3.1 City view (dec)

`0x004D75D0` (window 1) calls `0x004DDDC0` → **`0x00430D30(camera x 0x00BCA2BC, camera y 0x00BCA2C0,
map start 0x00BCA2D8)`**. That function sets the clip to the viewport `0x0070E430`–`0x0070E43C` and
raises the 60 ms animation flag `0x0070E3C4` from the frame clock. It then draws, in this order:

1. **Normal view** (`u32 0x00E94E0C` = 0, no overlay):
   - `0x00430EF0`, the ground pass: terrain and building footprints row by row over the
     view-to-tile table `0x0070E494`; water animation; ambient counting near the centre; the
     tile under `0x006059D0` remembered.
   - `0x00436B60(0)`, the upper pass. It draws building tops with their animations (`0x0040F2C0`),
     figures (`0x00438E50`, `0x00436310`), storage contents (`0x00442280`) and effects; its
     callees include `0x004C0160`, `0x004E5E90`, `0x00542E10` and `0x0041FFF0`–`0x00421920`.
   - `0x0051F780`.
   - `0x0043B630`, the construction ghost for the chosen building type `0x006059E8` at the
     hovered tile `0x00BCA300`. It is drawn with the tinted blitters; for ferries, bridges and
     monuments it uses the core "hover" functions already ported, core/command/hover.c.
   - `0x00439740(0)`.
2. **Overlay view** (`0x00E94E0C` ≠ 0 and ≠ 0x33):
   - `0x004316F0` (overlay ground);
   - `0x0051F780`;
   - `0x00439F10` (overlay tops: columns and service walkers);
   - `0x0043B630`;
   - `0x00439740(9999)`.
3. `0x0043E300` (assumed: the selected-legion and flag markers). Then the clip goes back to the
   whole screen.

The minimap is `0x0043E810` (from `0x0051EFB0`), with `0x0043E670` and `0x0043EE10`. The sidebar
is `0x0051E990` (full redraw: panels `0x0053E590`, buttons `0x00425730`, `0x0051EC90`, `0x0051ED30`,
`0x0051EDD0`, minimap) and `0x0051EC90` / `0x0051ED30` / `0x0051EFB0` / `0x0051F790` per frame.
The top menu bar is `0x0051F0A0` / `0x0051F680`.

### 3.2 Window table

The current window is `u8 0x00E38DDA`. Each id has three entry points:

- a full-redraw function: the `0x004D71C0` switch, on `0x00E38DCA`;
- a per-frame function: the `0x004D75D0` chain;
- an input function: the `0x004D8310` chain.

The ids (dec, names **assumed** from the screens drawn in §4.2 and from docs/commands.md):

| Id | Screen (assumed) | Full redraw `0x004D71C0` | Per frame `0x004D75D0` |
|---|---|---|---|
| 0 | main menu | `0x00520AB0(0)` | music, `0x0051F780`, `0x004180C0` |
| 1 | city view | `0x0051E990` | `0x004DDDC0`, `0x0051EC90`, `0x0051ED30`, `0x0051EFB0`, `0x0051F790`, `0x0043E1A0`, `0x00534610`, `0x00534DF0`, `0x00535080`, `0x00536120`, `0x004E1320`, `0x004E61F0` (editor mode `0x00605A3C`: `0x00533E70`) |
| 3 | city view + top menu open | `0x0051E990` | `0x004DDDC0`, `0x00425430` |
| 4 | mission start screen | `0x0041A850` | – |
| 5 | "about" / info box | `0x004C5CC0` | `0x004C7340` |
| 6 | build menu | `0x0051FFD0` | `0x004DDDC0`, `0x00520000` |
| 7 | advisors | `0x00520EA0` | (input chain) |
| 8 | overlays menu | `0x005208C0` | `0x004DDDC0`, `0x005208F0` |
| 9 | building / figure info | `0x0051E990`, `0x004F8E60` | |
| 0x0A–0x10 | options dialogs: display, sound, speed, file save / load / delete (0x0D, 0x0F, 0x10: `0x00532340`, `0x00530D70`, `0x005325F0`) | | |
| 0x11 | message list | `0x0051E990`, `0x004E1870` | `0x004E1A50` |
| 0x13 | Smacker video | | `0x00538FA0` |
| 0x14, 0x15, 0x16 | empire map (0x14 `0x0052D4D0`, 0x15 `0x0052D4E0`) | | |
| 0x17 | mission briefing | `0x0041C2D0` | `0x0041BE10` |
| 0x18–0x1D | scenario editor dialogs | | |
| 0x1E, 0x1F, 0x20 | choose a target for a legion / transport ship / warship | `0x0051E990` | |
| 0x23 | tooltip-driven help | `0x004CAF10` | |
| 0x26–0x2E | dialogs over the hall (0x28 family / kingdom `0x00520F40` + `0x00559E10` + `0x0055A0E0` + `0x0055AD30`; 0x2A–0x2E) | | |
| 0x2F–0x33, 0x3B, 0x40, 0x41, 0x44 | front end: family, campaign choice, intro screens (0x44 at boot → 0x41 → 0x3C) | | |
| 0x34, 0x35, 0x3B | mission selection / campaign maps | `0x0041AAA0`, `0x0041A180` | |
| 0x37–0x3A | full-screen pictures (`0x00417E80` …) | | |
| 0x3C–0x3E | front-end menus (`0x00520E00`, `0x0041E5C0`, `0x00534540`) | | |
| 0x3F | yes / no popup | `0x00425570` | `0x00425640` |
| 0x43 | `0x004191F0` | | |
| 0x45–0x48 | settings dialogs | | |
| 0x49 | city view variant (ticks allowed) | `0x0051E990` | |
| 0x4A | Bink video | | `0x00413470` |
| 0x4C–0x57 | hall dialogs (festival, family records …) | | |

Input functions (dec, from `0x004D8310`):

| Window | Input function or button table |
|---|---|
| 0 | `0x004261D0` on `0x0057BD88` |
| 3 | `0x00425F20` / `0x00426090` |
| 4 | `0x0041AA60` |
| 5 | `0x004C7770` |
| 6 | `0x00403E00`, tables `0x005C7760`, `0x005C6CB8` |
| 8 | `0x005C2BF0`, `0x00409E40` |
| 9 | `0x004F96E0` |
| 0x0B | `0x005C3070` |
| 0x0C | `0x005C3038` |
| 0x0E | `0x005C4208` |
| 0x11 | `0x004E1E10` |
| 0x14 | `0x00442730` |
| 0x17 | `0x0057BE30` / `0x0057BE78` |
| 0x1E | `0x0040C440` |
| 0x1F | `0x0040BA40` |
| 0x20 | `0x0040B830` |
| 0x3F | `0x00426530` |
| 0x43 | `0x005C65D0` |
| 0x4F | `0x005F78F0` |
| 0x55 (cheat input) | `0x0041D310` |

Window 2 is the sidebar sliding (scroll only, per-frame `0x0051FD60`). 0x56 is the message
options.

In the sweep (§4.2) every id drew a screen. Ids 0x40/0x41/0x42/0x44 switched to another id on their
own. Id 3 faulted on a null menu pointer (it needs a menu opened first).

### 3.3 Advisors, info panels, messages

These are windows of the table: 7 for the advisors (`0x00520EA0` and the advisor pages), 9 for
info (`0x004F96E0`, `0x004F8E60`; docs/commands.md §1.4) and 0x11 for the message list.

The message pop-ups are drawn by `0x004C5E60` / `0x004C6C70`. They can start a Smacker video
(§7).

The modal message box `0x00538CD0` runs its own message loop and draws with `0x00538C80` →
`0x00538940`, calling `0x004CF210`, `0x004D71C0` and `0x004D0280` itself. In the oracle it is
replaced (game.py `_message_box`).

## 4. The original's drawing in the oracle

### 4.1 Method (run)

The scratch extension of `Game` does four things.

**1. Window and GDI stubs.** It stubs these Win32 calls:

| Calls | Return |
|---|---|
| `GetDC`, `ReleaseDC` | fixed handles, 1 |
| `GetDeviceCaps` | 16 bpp |
| `CoInitialize`, `FindWindowA` | 0 |
| `LoadIconA`, `LoadCursorA`, `GetStockObject` | fixed handles |
| `RegisterClassA` | an atom |
| `CreateWindowExA` | an hwnd |
| `UpdateWindow`, `SetFocus`, `Get` / `SetWindowLongA`, `SetRect`, `AdjustWindowRectEx`, `GetMenu`, `SetWindowPos`, `SystemParametersInfoA`, `GetWindowRect`, `GetPaletteEntries` | trivial values |
| `GetCursorPos` | a scripted position |
| `GetAsyncKeyState`, `SetCursor`, `PeekMessageA` | 0 |

**2. Emulated DirectDraw.** `DirectDrawCreate` returns an object whose vtable entries are
stdcall stubs, built with extra import-stub slots past the real imports:

- IDirectDraw: `Release`, `CreateClipper`, `CreateSurface`, `SetCooperativeLevel`,
  `SetDisplayMode`.
- IDirectDrawSurface: `Release`, `Blt`, `BltFast`, `Flip`, `GetPixelFormat`, `GetSurfaceDesc`,
  `IsLost`, `Lock`, `Restore`, `SetClipper`, `Unlock`.
- IDirectDrawClipper: `SetHWnd`.
- `CreateSurface` allocates w·h·2 bytes in the other heap.
- `Lock` and `GetSurfaceDesc` fill the `DDSURFACEDESC`: height +8, width +0xC, pitch +0x10,
  `lpSurface` +0x24, pixel format +0x48 with the masks of the chosen mode.
- `Blt` and `BltFast` copy the source surface to the destination.
- An unknown vtable slot stops the run.

**3. Boot.** `Game.BOOT` runs with `0x004CF960(hInstance)` inserted after `0x004149D0` and with the
`video` step removed: `0x004CF960` makes the Bink object itself. The resolution `0x00E38E68` and
the fullscreen flag `0x00E38E5C` are set first. The game's log then reads:

- `OK :DD SetCooperative Level.`, `OK :DD Set resolution.`, `OK :GFX DirectDraw enabled.`;
- `OK :DD Creating one Vram only.`, `OK :DD can lock primary surface`, `OK :DD Surface created.`;
- `OK :DD surface_format is RGB 565.` (or 555).

**4. Load and draw.**

- Load a save with `load_save`.
- Set the window `0x00E38DDA` and the redraw flag `0x00E38DCA`, and call `0x004D6B60`. The present
  `0x004D0280` stays the oracle's no-op.
- Read the back buffer at `u32 0x00E92E18` (pitch `0x00E38D94`, size `0x00E38DB0` ×
  `0x00E38DAC`) and write it as a PNG.
- Frames with ticks: `timeGetTime` returns a scripted clock.

### 4.2 Results (run)

- **City view.** Actium (`local/saves/downloaded/1592/39-Actium-Dec30c.sav`) was drawn at 1024×768
  in 565, 800×600 in 565 and 640×480 in 555. The image shows:
  - the full city view: terrain, roads, buildings with tops, figures, ships, water;
  - the sidebar with the minimap, buildings panel and portrait;
  - the top menu (File / Options / Help / Overseers, "Db 2729", "Pop 1105", "Dec 30 BC");
  - the red "Months until victory" text.

  Frame times were 0.1 s for the first frame (art loads) and then 0.04 s. Two separate runs gave
  identical buffers.
- **Windows.** All window ids 0x00–0x57 were drawn from the same loaded state (window set, redraw
  flag, two frames each). 87 drew a screen, with 0.3–4 s per id including art loads, and no
  unknown import was hit. The screens include:
  - the advisor hall;
  - the options dialogs and the message list;
  - the empire map;
  - the mission briefing and the scenario-editor dialogs;
  - the family, campaign and mission-choice screens;
  - the front-end pictures and the Cleopatra splash;
  - the BreakAway logo;
  - the yes/no popup.

  Dialog-only windows draw over the previous screen (§2.4). Window 3 faulted (null menu pointer at
  0x0042600C).
- **Frames with ticks.** 50 frames with the clock advancing 200 ms per frame ran 50 ticks (speed
  100). The state comparison with 50 bare ticks is in §1.4.
- **Not reached by the sweep.** It needs scripted set-up or input:
  - overlays;
  - specific advisor pages;
  - info panels of a selected building;
  - menus opened from the top bar;
  - the construction ghost while dragging;
  - fades and the loading screen.

The images are game output: they stay outside the repository (CLAUDE.md).

### 4.3 What blocked, what remains

Nothing blocked the city view or the windows. Remaining problems:

- The **present** `0x004D0280` is still the oracle's no-op. Running it needs only `Blt` /
  `BltFast` and `timeGetTime`, which the emulation has. The fade path `0x004D05F0` /
  `0x004D0650` is not tried yet.
- **Bink** frames go to the locked *primary* surface (`0x00413470`) and need `BinkCopyToBuffer`
  emulated: decode with a free decoder (§7) or skip. **Smacker** has no files in this install.
- **Text in the loading screen** is GDI (`TextOutA`). The emulation can record the calls (text and
  position) but has no font to draw them: compare the call list, not pixels.
- **Modal loops** (`0x00538CD0` message box, `0x00413690` Bink loop) need their own scripted
  input in the oracle, as `_message_box` does now.

## 5. Input

Ghidra did not make the WndProc `0x00414CD0` a function, so it is missing from
`local/ghidra-export` and the xref tool does not see its accesses. Everything below on it is read
from the disassembly (dis).

### 5.1 WndProc `0x00414CD0` (dis)

**Before the switch.**

- When a Bink video is open (`0x006FDC40`), messages go to `0x00560330` / `0x005602F0`.
- In window 0x54 only `WM_SETCURSOR`, `WM_PAINT` and mouse messages pass.
- Every message goes through `0x0041D110(msg, wParam)`, the window-1 hot keys:
  - Alt+1 / Alt+2: screenshots `shot1.bmp` / `shot2.bmp`;
  - Ctrl+Alt+C: the cheat input, window 0x55;
  - Ctrl+Alt+F: `0x0041BAE0`.

**Messages handled.**

| Message | Effect |
|---|---|
| `WM_MOUSEMOVE` | i16 `0x00E38E0A` / `0x00E38E08` = client x / y |
| `WM_LBUTTONDOWN` / `UP` | button bit 0 of `u8 0x00E38E07`; `WM_LBUTTONDOWN` also sets the click latch `0x00E38E02` |
| `WM_RBUTTONDOWN` / `UP` | bit 1 of `u8 0x00E38E07` |
| `WM_MOVE` / `WM_SIZE` | client rectangle → `0x00E39130`–`0x00E3913C`, present, redraw |
| `WM_ACTIVATE` / `WM_ACTIVATEAPP` | stop streams, restore surfaces |
| `WM_PAINT` | present |
| `WM_CLOSE` | `0x00D3B43D` := 3 (quit) |
| `WM_DISPLAYCHANGE` | `0x00E38DBC`, `0x00E38DB8`, `0x00E38DB4` |
| `WM_KEYDOWN`, `WM_KEYUP`, `WM_CHAR`, `WM_SYSKEYDOWN` | §5.3 |

Double clicks, the middle button and `WM_SETCURSOR` are not handled.

### 5.2 Mouse per frame (dis/dec)

- `0x004CF210`, once per frame:
  - it reads `GetCursorPos` into `0x00E38DC4` / `0x00E38DC0` and, in fullscreen, into the 16-bit
    `0x00E38E0A` / `0x00E38E08`;
  - it scales them to the screen with `0x004CDE60` and `0x004CDE40` against the client rectangle
    `0x00E39130`–`0x00E3913C` → `0x00E38E34` (x) and `0x00E38E30` (y);
  - it derives from the button bits `u8 0x00E38E07` (bit 0 left, bit 1 right, written by the
    WndProc) the flags left pressed `0x00E38E03`, held `0x00E38E00`, released `0x00E38E04`,
    right pressed `0x00E38DF7` and released `0x00E38DF8`, "moved" `0x00E38E40`, and the time
    since the last move `0x00E38E3C`;
  - a left press calls `0x0053B090(0)` (click sound);
  - the drag state `0x00E38E14` goes to 2 past 50 pixels.
- **Hovered tile.** `0x00405F80` (docs/commands.md §1.1).
- **Dispatch.** `0x004D8310` → per-window handlers → button tables (`0x00425990`, `0x004261D0`,
  `0x00425C40`) → callbacks with parameters `0x00D3B198` / `0x00D3B194`. These callbacks are the
  phase-5 commands (docs/commands.md §1.2, §4).
- **Scaling.** The scaling is `x · (W·100 / client width) / 100`, through `0x004CDE60` (a·100/b,
  0 if b = 0) and `0x004CDE40` (a·b/100), both with idiv truncation.

### 5.3 Keyboard (dis: WndProc jump tables)

**`WM_KEYDOWN`.**

- **Modifiers.** Ctrl → `0x00E92E20`, Shift → `0x00E92E24` (no reader).
- **Arrows.** They set scroll flags `0x00E92E2C`–`0x00E92E38`, and move list selections in the
  dialogs.
- **Page Up / Page Down / Home.** Rotate the city view (`0x00409EB0`).
- **Text-field keys.** `0x004CC320`–`0x004CC3E0`.
- **Bookmarks.** F1–F4 jump to bookmark `0x00416AC0(n)`; with Ctrl they store it
  (`0x00416B10(n)` → `0x00E94DDC`, saved chunk 130).
- **Display keys.**
  - F5 re-centres the window.
  - F6 toggles fullscreen `0x00E38E5C`.
  - F7 / F8 / F9 change the resolution (`0x004CFDC0(1/2/3)`, assumed).
  - F12 toggles `0x00775680` (minimap).
- **Esc.** Quit box `0x00538CD0`.
- **Debug keys.** They are gated by `u8 0x00578C68` = 0. That byte is 1 in the exe and no writer
  was found, so they are inactive. One of them, Ctrl+A, toggles `0x00605A0C`, which the place
  function `0x004728A0` reads to skip costs.

**`WM_CHAR`.**

- **Text fields.** Characters go first to the text field: `0x004CC250` → `0x004CC400`. The active
  field is `0x00E20990`; `0x004D8310` clears it every frame and a window re-arms it with
  `0x004CC500(n)`. Buffers are at `0x00E2079C`.
- **Speed.** `[` / `]` → `0x00416BA0` → `0x00403320`, speed −10 / +10.
- **Pause.** P toggles the pause `0x00E38E60` (§1.2).
- **M** freezes the hovered tile (`0x00605A5C`).
- **City overlays.** F, D, C, T, W, H, X, Y, Z → `0x004168E0(id)` (overlay `0x00E94E0C`, previous
  `0x00E94E10`; Space swaps them).
- **Advisors.** `` ` ``, 1–9, 0, −, = → `0x00416990` → `0x0040A0E0`.
- **Legions.** L → next legion `0x00416A20`.
- **Orders.** In window 9 (legion or ship info) and windows 0x1E / 0x1F / 0x20, letter keys call
  the phase-5 order functions with `0x00D3B198` set.
- **Window 0x1E writes the formation directly.** Its T / L / N / M keys write formation +0x2A and
  +0x62 **inside the WndProc**. That is a state change outside any phase-5 command function, and
  it has to become a command.

**`WM_SYSKEYDOWN`.** Alt+D toggles `0x006FDC50`; Alt+X = Esc; Alt+Z → `0x00406090`.

**`GetAsyncKeyState` call sites** (xref):

- VK_MENU in `0x0041D110`.
- **'R' (0x52), tested for `== 0x8001`** in `0x0043B630`, `0x00441F80`, `0x004BFCD0` and
  `0x004C0F30`, i.e. **inside the city-view draw and construction preview**. It flips the building
  orientation `0x00BC9714`, which the place command reads. A state change made by drawing with a
  key as input.
- F2 / F5 / F6 in the debug code of `0x004E61F0` (gated by `0x006044B4`).

### 5.4 Scrolling (dec + dis)

- **Direction.** `0x004CF660` returns a direction 0–7, or 8 for none.
  - It is rate-limited by `0x004DDD60` on `timeGetTime`: it fires every
    `((100 − scroll speed 0x00E38E70) / 10) · 12 + 2` ms (default scroll speed 70).
  - It combines the screen edges (cursor within 1 px of the screen size `0x00E38DB8` /
    `0x00E38DB4`) with the arrow flags.
  - It sets `0x00E38DCC` (scrolling; this also blocks ticks).
- **Apply.** `0x004DDDE0(dir)` moves the camera `0x00BCA2BC` / `0x00BCA2C0` (saved chunks 43 and
  44) and calls `0x0041D6F0` and `0x004DE080`.

### 5.5 City input path (dec)

`0x004D8310` on window 1 does the following, in order:

1. The menu bar (`0x00426100` / `0x00426030`, table `0x005C91F0`).
2. `0x00417B40`.
3. Scrolling.
4. The minimap (`0x00406190`, `0x004061E0`, `0x00406250`).
5. The sidebar button tables, each after its hover `0x00425BC0`: `0x00425990` on `0x005C7300`,
   `0x004261D0` on `0x005C6CB8`, `0x00425990` on `0x005C7370` and `0x005C7980`.
6. The hovered tile `0x00405F80`; it returns when off the map.
7. The mouse actions:
   - left press → `0x0040B630` / `0x0040B790` / `0x0040B6D0`, else `0x00405C20`;
   - held → `0x00405D90`;
   - released → `0x00405E90`;
   - right click → `0x00405BA0`, else `0x004F7E50`.

### 5.6 Cursor (dis)

`0x004CF460(n)` calls `SetCursor` with one of the 19 cursors loaded by `0x004CF960` (resource ids
0x66, 0x96, 0x82, 0x8E, 0x90–0x95, 0x98–0x9D, 0x9F, 0xA0) and keeps n in `0x00E390DC`. The end of
the frame picks n by window:

- city: 2 when the build type is 9, else 0;
- 0x49: 10;
- 0x1E: by formation layout, and over targets;
- 0x1F / 0x20: by the figure under the mouse.

The platform draws these cursors itself. They are Windows resources in the exe (`.rsrc`), read at
run time from the user's `Pharaoh.exe`.

## 6. Sound and music

### 6.1 Driver (dis/dec, from the sound survey)

The sound driver is Miles (`MSS32.DLL`, 41 `AIL_*` imports, IAT 0x0056F268–0x0056F308), wrapped by
a C++ object at `0x00E92BF8` (code 0x0055DEA2–0x005601xx).

- **Constructor `0x0055E38B`.** 22050 Hz, 16 bit, stereo. Stream volumes +0x14C (music), +0x154
  (speech), +0x158 (ambient).
- **Start-up `0x005394F0`.** This step is skipped by the oracle. It does the following:
  - `AIL_startup`, `AIL_set_preference`, `AIL_waveOutOpen`;
  - allocates **5 sample handles** (`0x0055DEA2(5)`); `0x00F55B88` = sound on;
  - creates a 500-entry name registry (`0x0055DC63`);
  - reads `music.txt` (`0x00539250`) and builds the per-building-type sound table `0x00F54670`
    (`0x00539D60`), among others.
  - The 3D-provider code (`0x0055F697`, `0x0055FF20`) has no callers.
- **Samples.** `0x0055F0E8` plays from the request block `0x00E92DA8`: sound id, volume, pan,
  loops. If that sound is already playing it does nothing. Otherwise it takes a free handle and
  loads the file lazily with `AIL_file_read`.
  - **UI sounds** `0x0053B090(id < 0x13)`: table `0x00F543B8`, volume `0x00E38EC4`, centre pan.
  - **Figure sounds** `0x0053B4A0(figure, sound < 0x21, channel < 5)`:
    - table `0x00F54120`;
    - pan `ftol(128.0 / view width · screen x)` (x87);
    - silent when off screen, except for 16 figure types, which play at volume / 4.
  - **Random city sound** `0x0053ACF0` and the building info sound `0x004F8E60`.
- **Streams.** `0x0055F449(name, channel, loops)`: `AIL_open_stream`, `AIL_start_stream`, with
  channel 0 = music, 1 = speech, 2 = ambient.
  - `0x0055F561` closes a stream; `0x0055F5A1` tests whether it is playing.
  - `0x0055F5F4` / `0x0055F639` get and set a stream's volume.
  - `0x0053B6F0(channel)` fades by −5 per frame, then closes.

### 6.2 Ambient (dec)

- **Slots.** `0x0041D560` keeps 70 slots of 0x80 bytes at `0x00702900` (chunk 92). The ground
  pass fills them through `0x0041D410` (building type → slot, table `0x0057C780`). Every 2000 ms of
  frame clock the oldest ready slot is "played" through `0x0047D350`, which is `xor eax, eax; ret`.
  It plays nothing, but its saved fields follow the wall clock.
- **What is heard.** `0x0053B960` picks an ambient MP3 on stream 2 from the largest of the 12
  counters at `0x00F55B50`, which the ground pass counts within ±3 tiles of the view centre:
  - housing 1–5, chosen by population (city +0x24) thresholds 500 / 2000 / 5000 / 8000;
  - industrial, meadow, desert, floodplain, rock, water, vegetation;
  - monument (files missing).

### 6.3 Music (dec)

- **music.txt.** `0x00539250` parses it: tune, M/A, mission, delay after the tune, min and max
  population. Up to 54 missions, 50 M and 10 A tunes; the install has 905 M lines.
- **Selection.** `0x00539730` keeps the M tunes whose population window holds the population.
  `0x00539820` cycles: next M tune → random A tune (`rand`) → wait the delay (`GetTickCount`).
- **Fixed tracks.**
  - `audio\music\battle.mp3` when city +0x4634 ≥ 16 or +0x49A0 ≥ 5 (frame step 19; that these
    count enemies is assumed);
  - `audio\music\setup.mp3` in the front end.
- **Plagues.** They start ambient streams from the simulation (`0x004E6ED0`, `0x004E70B0`,
  `0x004E7700`).

### 6.4 Files (file bytes)

| Folder | Count | Format |
|---|---|---|
| `AUDIO/Wavs` | 263 | RIFF PCM 22050 Hz 16 bit; 261 mono, 2 stereo; no ADPCM |
| `AUDIO/Voice/Walker` | 374 | PCM 22050 Hz 16 bit mono |
| `AUDIO/Music` | 39 MP3 | 37 are MPEG-2 layer 3, 160 kbps, 22050 Hz stereo; 2 are 44.1 kHz |
| `AUDIO/Ambient` | 15 MP3 | 64 kbps |
| `AUDIO/Voice/Mission` | 139 MP3 | mostly 22.05 kHz 64 kbps |

`MP3DEC.ASI` is the Miles MP3 decoder plug-in. The `.M3D` files are the unused 3D providers.

Speech:

- `0x00539B20` builds the mission briefing and victory speech names (`audio\voice\mission\
  NNN_mission.mp3`, …), played from `0x0041A850`.
- `0x00539A70` plays walker speech (`audio\voice\walker\…`) on stream 1, when `0x00E38E65` is set.

### 6.5 Mapping to core's requests

Ported in package 7.7 (core/frame/sound.c, core/sim/platform.c; check:
tests/differential/oracle_sound_check.py, docs/oracle.md "Sound"). The split, from the binary:

- **State (core).** Everything the game decides from the state: the ambient slots `0x0041D560` /
  `0x0041D410`, the music steps `0x00539820` / `0x00539730`, the ambient stream `0x0053B960`, the random
  city sound `0x0053ACF0` (its 3–4 CRT `rand` draws), the fades `0x0053B6F0` / `0x0053B790`, and for the
  figure, interface and city sounds which sample, at what volume and pan: the sound tables, the
  on-screen test (`0x004F0BA0`, ported as `view_tile_to_screen`), the quarter volume of the 16 figure
  types heard off the screen, the pan `__ftol(x · (128.0f / view width))` (x87, reproduced on integers
  with core/city/day_x87.h). The sound object's fields that the wrappers keep in the state (the default
  stream volumes +0x14C / +0x154 / +0x158, the stream handles +0x1D8 as "open" flags) are written by core.
- **Platform (the driver's mixing).** What `0x0055F0E8` does with a request block (it reads only the
  block, the registry and its own arrays outside the state, and writes nothing in the state): whether
  the same sound is still playing, which of the 5 voices plays it, loading and caching the file,
  reverb; and Miles itself (open, close, status, volume of streams).

| Original | Request (core/sim/platform.h) |
|---|---|
| `0x0055F449` open and play a stream | `stream_play(name, channel, loops, volume)` → handle (0: not opened) |
| `0x0055F561` close | `stream_stop(channel)` |
| `0x0055F5A1` playing | `stream_playing(channel)` |
| `0x0055F5F4` / `0x0055F639` volume | `stream_volume(channel)` / `stream_set_volume(channel, volume)` |
| `0x0055F38D` / `0x0055F32F` / `0x0055F3EB` default volume | kept in the state; `stream_playing`, `stream_set_volume` |
| `0x0055F0E8` sample (from `0x0053B4A0`, `0x0053B090`, `0x0053ACF0`, `0x004F8E60`) | `sample_play(sound, file, volume, pan, loops)` |
| `0x00538E30`, `0x00413580`, `0x00539170` | `video_play`, `video_modal`, `video_close` |

The sound start-up `0x005394F0` (driver, sound registry, `music.txt`, the sound tables) is not ported:
it is WinMain's, and its registry lives outside the state (allocated by `0x0055DC63`).

## 7. Video (dis/dec, file bytes)

- **Bink** (`BINKW32.DLL`, 12 imports). The object is at `0x006FDC54`.
  - `0x00413580` opens a video (`BinkSetSoundSystem(BinkOpenMiles)`, `BinkOpen`) and sets window
    0x4A.
  - `0x00413690` is a blocking loop: `BinkDoFrame`, `BinkNextFrame`, copy `0x00413470`,
    `BinkWait`, a key or click skips.
  - `0x00413470` locks the **primary** surface and copies the frame centred, not into
    `0x00E92E18`.
  - Files: `BINKS/High/` Intro, pre_dynastic, archaic, old_kingdom, middle_kingdom, new_king (the
    period videos `0x0041BB70`), win_grand (`0x00418640`). All are `BIKf` 560×333, 24 fps, with
    Bink audio (RDFT, 22050 Hz stereo).
  - `Binks\low\*_small.bik` (chosen when `0x00E38EDC` < 0xE9; assumed: a CPU speed threshold) are
    not in the install.
- **Smacker** (`SMACKW32.DLL`, 7 imports).
  - `0x00538E30` opens and `0x00538FA0` steps the video per frame (window 0x13), with
    `SmackToBuffer` into the locked back or primary surface.
  - Names: `smk\logo`, `intro`, `credits`, `victory_senate`, `victory_balcony`, `win_game`, and
    the message videos named in `Pharaoh_MM.eng` (`1st_chariot`, `army_win`, `quake`, `sick`,
    `spy_army`, `spy_barbarian`, …).
  - **There are no `.smk` files in the GOG install**, so every Smacker open fails. That is what
    the original does on this install too, and what core's `platform_video_play` returning 0
    reproduces.
- **Free decoders.** The Homebrew `ffmpeg` (libavcodec) has `binkvideo`, `binkaudio_rdft`,
  `smackvid`, `smackaud` and `mp3`. A null decode of `pre_dynastic_big.bik` succeeded. The platform
  can play the Bink videos through libavcodec (LGPL, dynamically linked) or skip them.

## 8. Proposed architecture

CLAUDE.md keeps core/ for the deterministic simulation and platform/ for SDL, input, audio and
files. The ported drawing is deterministic C working on the game state, but it is not simulation.
Proposal:

```
core/         simulation (as now) + the frame-side state updates of §1.4 (new: core/frame/)
present/      NEW: ported drawing and UI, deterministic, platform-independent C
              (reads the state, writes a 16-bit pixel buffer; may write the presentation globals
              of the state mirror, never simulation state)
platform/     SDL window and event loop, clock, input → globals, audio (Miles requests →
              SDL_mixer / miniaudio), video (libavcodec), files, pixel buffer → texture
```

- **Presentation globals stay in the state mirror.** The window id, clip rectangle, text cursor,
  mouse globals and the view-to-tile table are in the exe's `.data`, so `game_state` already holds
  them. The ported drawing reads and writes them with the same `U8` / `S32` accessors and
  addresses. The buffer itself is outside the state: `present_target {uint16_t *pixels; int32_t
  pitch, w, h; int32_t format555_565;}`, which plays the role of the pointer `0x00E92E18`. The
  pixel data comes from `platform_pixels_load`, kept by the platform. present/ reads it through
  the slot entries, mapped as the strings are (state.md "Game data outside the segments").
- **core/frame/ (new).** The parts of the frame that change simulation state, as core functions
  whose inputs are explicit:
  - the frame clock `0x00E92E0C`;
  - the animation timers `0x0040F270`;
  - the pacing `0x004DDCB0` (clock in, "tick due" out);
  - the fast-forward loop;
  - the autosave `0x004D5C30`;
  - the per-frame `rand` draws in the original's order and conditions (sound settings are inputs);
  - the hovered tile `0x00405F80`;
  - the ambient slots `0x0041D560`, `0x0041D410`;
  - and the state writes the draw passes make (§1.4): animation frames `0x0040F2C0`, water
    images in `0x00430EF0`, goods flags `0x005596D0`.

  Two of these are only known to run inside drawing loops (sprite and image grid on *visible*
  tiles). So the visible-tile walk of the city view (viewport, `0x0070E494` table, row order) must
  be reproduced in core: either as a core "visit visible tiles" pass called by present/ with the
  same order, or present/ calls core helpers for those writes. Which split is cleanest is open
  (§10). In both cases the order of state writes within a frame stays the original's.
- **Commands.** The phase-5 handlers (core/command) are the leaves of the input dispatch. The
  per-window input functions `0x004D8310` → button tables (`0x00425990`, `0x004261D0`,
  `0x00425C40`) → callbacks are ported in present/. They set the parameters `0x00D3B198` /
  `0x00D3B194` and call the core command through the same table entries (`GAME_CALL` on the
  callback address). Commands thus stay in core and are reached by the original's own dispatch.
- **Frame loop split.**
  - The platform owns the OS loop and the clock. Per frame it:
    1. writes the input globals (cursor, button bits, keys) as the WndProc and `0x004CF210` would;
    2. calls `frame_run(st, clock, inputs)`.
  - `frame_run` is present/'s port of `0x004D6B60`. In the original's order it calls:
    - core's clock and timers, the pacing and the tick (`game_tick`), then the autosave;
    - the redraw and per-frame draw, the input dispatch, the tooltips, the top bar;
    - the present request;
    - the ambient, music and sound requests.
  - Sound and video stay requests (core/sim/platform.h), extended with stream stop and volume.
  - The present request hands the buffer to the platform, at most every 50 ms as `0x004D0280`
    does.
  - Resolution is a setting: 640×480, 800×600 or 1024×768. The original's layouts depend on the
    width, so other sizes would need scaling in the platform (integer scaling of the 16-bit
    buffer).

## 9. Frontier and porting packages

### 9.1 Size (static callees of `0x004D6B60`, minus the functions the tick runs)

Static call graph (Ghidra callees; indirect calls through window and button tables are partly
missing). The reach of the frame excludes the tick's dynamic coverage (local/oracle/coverage.json).
Ported = in `build/state_run --list` (1,861 functions).

| Set | Functions | Code | Ported | Unported |
|---|---|---|---|---|
| frame reach, minus the tick's coverage | 1,863 | 773 KB | 772 (236 KB) | 1,091 (537 KB) |
| entered while drawing the 88 windows (run) | 449 | 220 KB | 87 | 362 (208 KB) |

The functions entered by the sweep are a lower bound for what the screens need. The static reach
includes the command handlers already ported in phase 5 (most of the 772). The unported 537 KB are
the presentation port. That is about the size of the simulation port (the tick reaches 1,185
functions, 463 KB).

Grouped by first root reached (static, unported):

| Group | Roots | Unported fns | Code |
|---|---|---|---|
| graphics primitives | `0x0053E590`, `0x0053F250`, `0x0053F4F0`, `0x004CC720`, `0x004CD9D0`, `0x004CDA40`, `0x0053F660`, `0x004CF1C0` | 131 | 70 KB |
| city view | `0x00430D30` | 162 | 76 KB |
| sidebar, top bar, minimap | `0x0051E990`, `0x0051F0A0`, `0x0051F680`, `0x0051EC90`, … | 78 | 17 KB |
| tooltips | `0x004C8750`, `0x004C9750`, `0x004CABD0`, `0x004CA930` | 14 | 11 KB |
| sound, ambient, music | `0x0041D560`, `0x00539820`, `0x0053ACF0`, `0x0053B6F0`, `0x0053B960`, `0x0055F449`, … | 12 | 4 KB |
| input dispatch and windows (all screens) | `0x004D8310`, `0x004CF210`, `0x00405F80`, `0x004CF460` | 547 | 294 KB |
| per-frame window draw not reached from input | `0x004D71C0`, `0x004D75D0` | 115 | 60 KB |
| frame-only rest | `0x0040F270`, `0x004DDCB0`, `0x004D5C30`, `0x004E2720`, … | 32 | 5 KB |

Most window code hangs under the input dispatcher, because input functions redraw their window.
Separating "draw" from "input" therefore cuts no packages. The cut that works is **per window**.
24 unported presentation functions use x87 (sound pan `0x0055D290`–`0x0055D9C0`, `0x00434AA0`,
`0x004D8310`, `0x004E61F0`, …): the integer-exact rule of docs/porting.md applies to those that
write state; for pixels, see §10.

### 9.2 Check methods

- **Pixel buffer, function level.** Extend `tools/oracle` (harness and replay) with the emulated
  DirectDraw (§4.1). Then:
  - run an original drawing function on a state with a given buffer (cleared or prefilled);
  - run the port on the same state, with the same pixel data mapped at the same addresses (dump
    the other heap's image buffers, as the strings are mapped) and the same initial buffer;
  - compare the buffer byte for byte, plus the state mirror as now.

  Unported callees are replayed. The recorder must then also record writes to the buffer range,
  which is a fixed block, so it can.
- **Frame level.** `lockstep --frames`: the original's `0x004D6B60` against the port's
  `frame_run`, frame by frame, with a scripted clock, cursor, button bits and keys. Compare the
  state (simulation included) and the buffer after every frame. This also checks the frame-side
  state writes of §1.4 and the `rand` consumption.
- **Requests.** Sound, music and video requests are compared as call sequences (name, channel,
  volume, pan), as the current replay checks unported calls.
- **Text-only GDI.** The loading screen's `TextOutA` calls are compared as (x, y, string) lists.

### 9.3 Proposed packages, in order

| # | Package | Content | Check |
|---|---|---|---|
| 7.0 | oracle display | emulated DirectDraw boot in `tools/oracle` (`Game(display=...)`), buffer access, PNG dump tool, replay recording of buffer writes, mapping of pixel data for the port's side; re-run lockstep / load checks with the real resolution globals | existing checks still pass; buffer of a known frame stable |
| 7.1 | frame skeleton in core/frame | `0x0040F270`, `0x004DDCB0`, the tick loop and `0x004D5C30`, the per-frame `rand` draws, `0x00405F80`, `0x004CF210` (mouse globals from scripted inputs) | lockstep `--frames` with drawing replayed |
| 7.2 | blitters | `0x0053E590` dispatch and the ~40 blitters on `0x00E92E18` (plain, mirrored, RLE, footprint 1×1–6×6, tinted, masked), clip, fills, `0x004CF1C0`, external image load + 555→565 conversions `0x004CDF20` / `0x004CDEB0` | pixel buffer per function, all image types × clip cases × both formats |
| 7.3 | text | `0x004CC770`, `0x004CCA60`, `0x004CBF30`, `0x004CC720`, numbers `0x004CCE00` / `0x004CCE90`, multi-line text and the text cursor | pixel buffer |
| 7.4 | city view | `0x00430D30` passes: ground `0x00430EF0`, upper `0x00436B60`, figures `0x00438E50`, storage `0x00442280`, ghost `0x0043B630`, overlays `0x004316F0` / `0x00439F10`, `0x00439740`, `0x0043E300`; with the state writes of §1.4 in core/frame | pixel buffer + state, on corpus saves × views (camera positions, orientations, overlays) |
| 7.5 | sidebar, minimap, top bar, tooltips | `0x0051E990` family, `0x0043E810`, `0x0051F0A0`, `0x0051F680`, `0x004C8750`, `0x004C9750`, `0x004CABD0` | pixel buffer |
| 7.6 | windows, one package per window group | build menu (6), overlays menu (8), info (9), advisors (7 + pages), messages (0x11, `0x004C5E60` / `0x004C6C70`), popups (0x3F, `0x00538940`), options (0x0A–0x10), empire (0x14–0x16), front end (0, 0x2F–0x44), editor (0x18–0x1D); each with its input function down to the phase-5 callbacks | pixel buffer + state, with scripted input; frame lockstep |
| 7.7 | sound, music | request logic: `0x0041D560` (core/frame: state), `0x00539820` (tune choice), `0x0053B960`, `0x0053ACF0`, `0x0053B4A0` pan and volume, fades `0x0053B6F0`; platform mixer (WAV PCM, MP3) | request sequence vs the original's (`AIL_*` call log in the oracle) |
| 7.8 | video | Bink via libavcodec in platform/, window 0x4A behaviour (`0x00413580`, `0x00413690`); Smacker paths stay "file absent" | behaviour (state after the video), not pixels |
| 7.9 | SDL platform | window, event pump → input globals as the WndProc writes them, clock, present every 50 ms, audio output | manual play; no oracle |

## 10. Open problems

1. **Frames change the simulation** (§1.4).
   - The per-frame CRT `rand` draws depend on the frame rate and the sound settings. A faithful
     game reproduces the original's consumption given the same frame and tick sequence; a
     recorded game (inputs + clock per frame) replays exactly.
   - Whether the sprite-grid and image-grid writes by drawing are ever read by the tick (animated
     building tiles vs the terrain tiles the readers test) is not established. Needed: a readset
     run (tools/oracle/readset.py) with frames interleaved.
2. **Oracle state with a real resolution.** The current reference states hold 0 in the resolution
   globals, sidebar x and viewport. Booting with the emulated DirectDraw changes them, and the
   ported new-game and load code must then match. Re-run every phase-1–6 check before switching the
   default.
3. **Pixel data on the port's side.** `state_replay` returns the original's buffer address for
   `platform_pixels_load`. For pixels it must also map the contents (the oracle's other heap
   blocks) at the same addresses, read-only. The blitters read through pointers into them (entry
   +0, `0x00D3B5F4`).
4. **x87 in drawing.** The figure-sound pan and some overlay, tooltip and editor code use x87. The
   integer rule of CLAUDE.md holds for present/ as for core/: no C `float` or `double`. The x87
   operations are reproduced on integers with core/city/day_x87.h (53-bit precision, round to
   nearest even, `__ftol`), core/city/monument_rating_x87.h (`fsqrt`, `fld dword`) and
   core/sim/x87_float.h (`fst dword`: the value rounded to a float). Examples: the overlay levels
   `0x00434AA0` (present/ui/cityframe.c), the editor's route points (present/ui/empire_editor.c).
5. **Window ids and names** in §3.2 are assumed from screenshots and callees. To be confirmed when
   each window is ported.
6. **Dead or unclear writes.**
   - The ambient slots follow the wall clock and are saved, but nothing reads them.
   - Building 0's type is written by the ground pass.
   - The meaning of `0x0078D7A0` (screen shake or flash in the present).
7. **Present cadence.** The 50 ms present threshold means the original showed at most ~20 fps of
   back-buffer frames. Faithful port: the same, or present every frame (a visible but
   non-simulation difference). To decide.
8. **Video.** The Bink copy goes to the primary surface, not the back buffer. The oracle needs a
   `BinkCopyToBuffer` emulation or a skip, and the `*_small.bik` choice depends on
   `0x00E38EDC` (CPU speed setting, assumed).
9. **Sound requests missing in core:** stream stop and volume (§6.5). Done in package 7.7.
10. **State changes outside the command functions**, made by the WndProc or the drawing (§5.3):
    - the P pause toggle `0x00E38E60` (which also sets window 1);
    - the window-0x1E formation keys (formation +0x2A, +0x62);
    - the 'R' rotation `0x00BC9714`, read with `GetAsyncKeyState` inside the draw;
    - bookmarks;
    - overlay and advisor hot keys.

    These need to become core commands (or present/ handlers checked with the state), as in
    phase 5.
11. **WndProc not in the Ghidra export.** Define it as a function and re-export, so that the
    xref and frontier tools see it. Other non-function code may also hide a writer of the
    debug gate `0x00578C68`.
12. **docs/commands.md §1.1 correction.** `0x00E38E60` is the pause flag, not "always 0" (§1.2).
    commands.md should be updated by its owner.
