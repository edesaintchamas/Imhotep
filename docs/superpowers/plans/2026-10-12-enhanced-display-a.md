# Enhanced display, milestone A: any size, crisp scaling, resizable window

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** the game draws at the window's size (at least 1024x768 game pixels), scaled by a whole number,
follows window resizes and full screen, while the three original modes run the original code unchanged.

**Architecture:** one hook in core: when the platform says "fit" and the resolution is 3, the screen size
globals take the platform's size instead of 1024x768. Everything derived from them (pitch, layout origin,
clip, back buffer, spare screen) already follows. The few places that test an exact width or height get an
`else` branch for other sizes (sidebar, top bar, view geometry, info panel, 1024x768 pictures). The
platform computes the size, resizes, and asks core to redo the mode (the original's F9 sequence).

**Tech Stack:** C11, SDL2, the project Makefile; oracle checks in Python (local/venv).

**Spec:** docs/superpowers/specs/2026-10-12-enhanced-display-design.md

## Global Constraints

- With an original size (640x480, 800x600, 1024x768) the code executed is today's: new code only in `else`
  branches reached by other sizes, or behind the fit flag.
- `G_RESOLUTION` (0x00E38E68) stays 3 in fit mode.
- Fit size: at least 1024x768, at most 4096x4096 game pixels.
- New code tagged `@imhotep enhanced <what>`; `make provenance` counts the tag.
- Display settings in `imhotep.cfg` (`display=fit|original`, `scale=auto|1|2|3`), never in `Pharaoh.inf`.
- Hidden and scripted runs (`--hidden`, `--script`) default to `display=original`.
- `make test` and `tests/differential/oracle_play_check.py` stay green.

## Review Focus

1. Resizing while a video, the main menu or a window that the original excludes from F9 (windows 0x0E,
   0x0F, 0x48) is open: the change waits until it may happen, no crash, no half-drawn layout.
2. A window made smaller than the minimum: the game stays at 1024x768 and is scaled down, clicks still
   land where they're drawn.
3. Switching full screen (F6) in fit mode: the game takes the desktop's size and back.
4. Loading a save made at 1024x768 in fit mode at 1920x1080 and the reverse: camera within bounds.
5. Sidebar collapse/expand animation at a non-original width: no trail of old pixels, right edge drawn.

---

## File structure

- `core/sim/platform.h`, `core/sim/platform.c`: new callback `display_fit` and `platform_display_fit()`.
- `core/frame/display.c`: the fit size in `display_resolution_set`; new `display_fit_apply()`.
- `core/boot/display.c`: the fit size at boot (0x004CF990).
- `core/newgame/reset.c`: `view_geometry_a/b` branch for other widths; helper `view_tiles()`.
- `present/sidebar/sidebar.c`: sidebar x, top bar, full redraw, slide.
- `present/ui/info/info.c`, `present/ui/empire/mission.c`, `present/ui/windows.c`, `present/draw/image.c`,
  `present/ui/frontend/campaign.c`: positions for other sizes.
- `platform/main.c`: fit size, scale, resize, full screen, `--display`, desktop size.
- `platform/install.c` / `install.h`: read `display` and `scale` from `imhotep.cfg`.
- `tests/unit/display_test.c`, Makefile `unit`: unit tests of the size maths.
- `tools/provenance.py` (or the script `make provenance` runs): count `@imhotep`.
- `docs/enhanced.md`, `docs/presentation.md`, README, `--help`.

### Task 1: the fit hook in core

**Files:** core/sim/platform.h, core/sim/platform.c, core/frame/display.c, core/boot/display.c,
tests/unit/display_test.c, Makefile.

**Produces:** `int platform_display_fit(game_state *st, int32_t *w, int32_t *h)` (1 and the size when the
platform is in fit mode, else 0); `void display_fit_apply(game_state *st)` (the F9 sequence for mode 3,
without the confirmation; nothing in windows 0x0E, 0x0F, 0x48; returns without effect when the size is
already the fit size).

- [ ] Write `tests/unit/display_test.c`: a `sim_platform` whose `display_fit` returns 1512x982; after
      `display_mode_set(st, 3)`: `G_SCREEN_W == 1512`, `G_SCREEN_H == 982`, `G_LAYOUT_DX == (1512-640)/2`,
      `G_SCREEN_PITCH == 3024`, `G_RESOLUTION == 3`; with `display_fit` returning 0: 1024x768; mode 2 with
      fit on: 800x600.
- [ ] Makefile `unit`: build and run `build/tests/display_test` (links libimhotep_core.a).
- [ ] Run, expect failure (no callback).
- [ ] Add `int (*display_fit)(void *ctx, int32_t *w, int32_t *h);` to `sim_platform`; `platform_display_fit`.
- [ ] `display_resolution_set`, mode 3: `@imhotep enhanced fit` — when `platform_display_fit` gives a size,
      use it (clamped to 1024..4096 x 768..4096).
- [ ] Same at boot (core/boot/display.c, the 0x004CF990 size).
- [ ] `display_fit_apply` in core/frame/display.c: `resolution_key`'s sequence (wndproc.c) for mode 3, then
      redraw; skip when `G_WINDOW` is 0x0E, 0x0F or 0x48 or the size already matches. `resolution_key` stays
      as it is.
- [ ] Run the unit test, `make core`, `python3 tools/check_same_code.py` must show only display.c,
      platform.c and boot/display.c changed.
- [ ] Commit.

### Task 2: view geometry at any width

**Files:** core/newgame/reset.c, tests/unit/display_test.c.

**Produces:** `void view_tiles(int32_t right, int32_t h, int32_t *cols, int32_t *rows)`:
`cols = (right + 2 + 59) / 60`, `rows = (h - 24) / 15 + 1`.

- [ ] Test: `view_tiles` gives the original table for (sidebar x, H) = 640: (0x1DE,480) 8x31 and
      (0x256,480) 10x31; 800: (0x256,600) 10x39, (0x2CE,600) 12x39; 1024: (0x346,768) 14x50,
      (0x3BE,768) 16x50; and 1920x1080 expanded (1734,1080): 29x71.
- [ ] Implement; in `view_geometry_a` (collapsed) and `_b` (expanded), the `else` (width not 640/800/1024)
      uses `view_tiles(sidebar x for that state, G_SCREEN_H)` then `view_geometry_set`, then
      `G_CV_VIEW_RIGHT` := that sidebar x (the city never draws under the sidebar). `@imhotep enhanced`.
      The 640 fallback (8, 0x1F) stays for widths below 1024 that are not original (not reachable).
- [ ] Unit test, check_same_code (only reset.c changes), commit.

### Task 3: sidebar and top bar at any width

**Files:** present/sidebar/sidebar.c.

Rules for width W not 640/800/1024 (always >= 1024): 1024's values moved by `d = W - 1024`.

- [ ] `sidebar_set_x`: x = W - 186, collapsed = W - 66.
- [ ] `topbar_draw_background`: draw the background at x = 0, then at x = 1024k for k up to W, and
      finally at x = W - 1024 (right part).
- [ ] `topbar_draw` texts: the 1024 values plus d for the treasury, population and date (they sit over the
      right copy of the background).
- [ ] `sidebar_draw`: right edge image (+2) at 1000 + d, and repeated below at y = 768, ... while < H;
      bottom strip `RECT_FILL(0, H - 9, W, 9)`; the panel's lower part (+4 / +5) at y = 0x1E0 and repeated
      every image height down to H (the last copy at H - image height). Editor: unchanged (out of scope).
- [ ] `sidebar_slide`: clip right = 1000 + d; the lower parts as in `sidebar_draw`.
- [ ] Build, check_same_code (only sidebar.c), commit. Visual check in Task 6.

### Task 4: other positions

**Files:** present/ui/info/info.c, present/ui/empire/mission.c, present/ui/windows.c,
present/draw/image.c, present/ui/frontend/campaign.c (and any other found by the sweep:
`grep -nE "0x400 *-|0x300 *-|== *0x(280|320|400|1E0|258|300)" present core`).

- [ ] info.c: a height not 480/600/768 uses the 768 position plus `(H - 768) / 2`.
- [ ] Pictures centred with `(0x400 - w) / 2`: keep the expression for W = 1024; for other W use
      `(W - w) / 2` (and `(H - h) / 2`), with their click areas moved the same; screen outside them black.
- [ ] Record every changed place in `docs/enhanced.md` (function, address, original, enhanced).
- [ ] Build, check_same_code, commit.

### Task 5: platform

**Files:** platform/main.c, platform/install.c, platform/install.h.

- [ ] `imhotep.cfg`: `display` (`fit` default; `original`), `scale` (`auto` default, 1..3); `--display
      fit|original|WxH` (WxH: fit mode at that fixed size, for tests) and `--scale` keeps its meaning for
      original modes. Hidden/scripted runs: `original` unless `--display`.
- [ ] SDL video initialised before the boot; fit size from the usable display bounds (window opened at
      them, minus nothing) and the pixel ratio: `s` = auto (drawable / points, at least 1) or the setting;
      game size = `pixels / s`, raised to 1024x768 (then `s` lowered; at `s = 1` the picture is scaled
      down). `cb_display_fit` answers it when `display=fit`.
- [ ] The fake desktop (`SCREEN_W/H`, `app.screen_w/h`, boot's `.width/.height`) = the fit size in fit
      mode, so edge scrolling fires at the window's edges and the boot's resolution rules see >= 1024.
- [ ] `SDL_RenderSetIntegerScale(TRUE)` when the drawable is at least the game size; window minimum size.
- [ ] Resize: on SIZE_CHANGED, recompute the fit size; when it differs, apply it 150 ms after the last
      resize event (`display_fit_apply`); `cb_set_display` must not resize the window in fit mode.
- [ ] Full screen: `cb_set_display` follows `G_FULLSCREEN` with `SDL_SetWindowFullscreen(DESKTOP)` / 0;
      the size change that follows is a resize.
- [ ] `--help`, README.
- [ ] Build; `oracle_play_check.py` (original mode) still 1042/1042; commit.

### Task 6: checks at several sizes

- [ ] `--display 1024x768` (fit forced to 1024x768): `oracle_play_check.py` equal (the general rules
      reduce to the original ones).
- [ ] Screenshots (`--hidden --display WxH --script`, `shot`) at 1280x768, 1512x982, 1920x1080,
      2560x1440: main menu, city expanded / collapsed, a dialog, empire map, mission briefing; inspect;
      a small script checks the sidebar's right edge column is not black and no column of the city view
      is left undrawn.
- [ ] Simulation independence: the same save advanced N frames with no input at 1024x768 and 1920x1080
      gives the same save file except the camera and display globals (compare with `build/save_dump`).
- [ ] Fix what they find; commit.

### Task 7: provenance, docs, release 1.1.0

- [ ] `make provenance` counts `@imhotep enhanced`.
- [ ] docs/enhanced.md complete; docs/presentation.md points to it; README (display settings, F9 / F6,
      Retina); CLAUDE.md status line.
- [ ] `make test`; version 1.1.0; tag; release workflow; macOS asset (as 1.0.x).
