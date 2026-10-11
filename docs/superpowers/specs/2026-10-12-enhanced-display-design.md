# Enhanced display: any resolution, crisp scaling, resizable window, city zoom

Date: 2026-10-12. Status: approved in conversation; implementation in two milestones.

## Goal

Imhotep starts in an **enhanced display mode** by default:

- the game fills the screen at any size (widescreen included), showing more of the city, not stretched;
- the window is resizable and the layout follows;
- pixels are crisp (integer scaling, nearest neighbour) on Retina and ordinary screens;
- the city view zooms in and out (50 % to 200 %), as in Augustus.

The original modes (640x480, 800x600, 1024x768) stay selectable and stay pixel-equal to the original.
The simulation and saves behave the same in every mode.

## Non-goals

- A GPU sprite renderer (Augustus's approach). The ported software renderer stays the only renderer.
- Zoom in real screen pixels (the city as its own layer). Possible later; not in this design.
- Display sizes below 1024x768 in enhanced mode.

## Principles

1. **The original path is untouched.** A new branch runs only when the screen size is not one of the
   three original sizes, or when the zoom is not 100 %. With the original sizes and 100 %, the code
   executed is the code that exists today, so every oracle check keeps its meaning.
2. **The simulation sees 1024x768.** In enhanced mode the resolution setting `G_RESOLUTION`
   (0x00E38E68) stays 3. Every rule that switches on it (storage rows 24, advisor and high-score
   positions, boot timing) behaves as in 1024x768. Only the size globals (0x00E38DB0 / 0x00E38DAC and
   those derived from them) hold the real size.
3. **Extensions are tagged.** Each new branch carries `@imhotep enhanced <what>` (a new tag: a display
   extension, not a rule of the original). `make provenance` counts them apart. None may appear in
   `core/sim`, `core/city`, `core/building`, `core/figure`, `core/map` or `core/command`, except the
   view geometry and camera code those already hold for display.
4. **Display settings live in `imhotep.cfg`**, never in `Pharaoh.inf` (original layout) or in saves.

## Milestone A: any size, crisp scaling, resizable window (release 1.1.0)

### A.1 Mode

- New setting `display` in `imhotep.cfg`: `fit` (default for new installs), `640x480`, `800x600`,
  `1024x768`. `fit` sizes the game to the window.
- New setting `scale`: `auto` (default), `1`, `2`, `3`.
- Enhanced size: at least 1024x768 game pixels; at most the back buffer cap (4096x4096).
- The Options screen's resolution choice gets a fourth entry, "Fit window"; choosing an original mode
  sets `display` to it and runs the original change (with its "keep this resolution?" question).

### A.2 Layout rules at size W x H (W >= 1024, H >= 768, not an original size)

Each point is an `else` branch beside the original's size switch:

| Place | Original | Enhanced |
|---|---|---|
| `sidebar_set_x` (0x0051E930), present/sidebar/sidebar.c | x by width 640/800/1024 | expanded W - 186, collapsed W - 66 (1024's distances from the right edge) |
| Sidebar drawing (top bar, buttons, full redraw, slide animation, bottom strip), sidebar.c | per-width offsets | 1024's offsets, moved by W - 1024; the panel's lower part repeated down to H; the menu bar background repeated across W |
| `view_geometry_a/b` (0x004DDEC0, 0x004DDF20), core/newgame/reset.c | tile counts per width | columns: enough tiles of 60 px to reach the sidebar's x; rows: enough half-rows of 15 px to reach H (both rounded up, the clip cuts the rest). For W = 1024, H = 768 this gives the original table's values |
| Info panel position, present/ui/info/info.c | per screen height | the 768 position, moved by the layout origin's vertical offset |
| High-score list, present/ui/dispatch.c, frontend/highscore.c | per `G_RESOLUTION` | unchanged (`G_RESOLUTION` is 3), moved by the layout offset when needed |
| Full-screen 1024x768 pictures (mission briefing, campaign map, windows.c, image.c centring) | `(0x400 - w) / 2` | centred in W x H: offsets `(W - 1024) / 2`, `(H - 768) / 2` added; the rest of the screen black; their click areas move by the same offset |
| Dialogs | centred by the layout origin `((W - 640) / 2, (H - 480) / 2)` | unchanged |

The exact list is produced by a sweep for constants 0x280, 0x320, 0x400, 0x1E0, 0x258, 0x300 and
`G_RESOLUTION` reads in present/ and core/ (milestone A's first task) and recorded in
`docs/enhanced.md`.

### A.3 Platform

- The window is created resizable and high-DPI aware (`SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI`).
- Integer scale `s`: `auto` = drawable pixels / window points (2 on Retina, 1 otherwise), or the
  configured value. The game size is `floor(pixels / s)`. If that is below 1024x768, `s` decreases;
  at `s = 1` and still too small the game size is 1024x768 and the picture is scaled down with
  linear filtering.
- Presentation: `SDL_RenderSetLogicalSize(game size)` with `SDL_RenderSetIntegerScale(SDL_TRUE)` and
  nearest filtering; the remainder is a black border. The renderer's logical mapping converts mouse
  coordinates to game pixels. Original modes use the same path, so they become crisp.
- Resize: on a window size change, the platform asks for a display change at the new size once the size
  has been still for 150 ms; until then the last frame is shown scaled. The change goes through the
  original's display set-up path (core/frame/display.c: new back buffer, layout, WM_DISPLAYCHANGE),
  extended to accept a custom size. No "keep this resolution?" question for resizes and full-screen
  switches.
- Full screen: borderless desktop full screen (`SDL_WINDOW_FULLSCREEN_DESKTOP`), at the desktop's size;
  F6 and the Options entry toggle it, as the original's.
- Hidden and scripted runs (`--hidden`, `--script`) default to the original mode the session asks for;
  `--display` overrides on the command line.

### A.4 Tests

- All existing checks, unchanged, in original modes: `make test`, `oracle_play_check.py`,
  `make present-check`, the fuzzer, `check_same_code.py` (where no enhanced code is involved).
- **Reduction test:** enhanced layout forced at 1024x768 equals the original 1024x768 frame pixel for
  pixel on the reference session's frames.
- **Layout snapshots** at 1280x720 (raised to the 1024x768 minimum height: 1280x768), 1512x982,
  1920x1080, 2560x1440: main menu, city (sidebar expanded and collapsed), a dialog, empire map, mission
  briefing. Checked for: sidebar touching the right edge, no undrawn strip, nothing outside the buffer.
  Golden images kept under `local/` (they contain game graphics).
- **Simulation equality:** the reference session's commands, played at 1024x768 and at 1920x1080,
  give the same simulation state every tick, except the listed display globals, the camera position
  and the view's size in tiles.
- Unit tests: scale choice, view tile counts, sidebar x.

## Milestone B: city zoom (release 1.2.0)

### B.1 Drawing

- Zoom `z` in {50, 75, 100, 125, 150, 200} %, setting `zoom` in `imhotep.cfg` (default 100).
- At z = 100 % nothing changes (the original path).
- Otherwise the city view is drawn by the ported renderer into an off-screen buffer of
  `view size / z` pixels: the back-buffer globals (pointer, pitch, draw size, clip) point at it while
  `city_view_draw` runs, and the view geometry has the tile counts for that size; then they are
  restored. The buffer is scaled into the view rectangle of the back buffer: nearest neighbour for
  z > 100 %, an area average for z < 100 %. The sidebar, menu bar and windows are drawn after, as now.

### B.2 Input

- Mouse positions inside the view rectangle are mapped into off-screen coordinates
  (`vx + (mx - vx) / z`) before the original's tile-under-mouse lookup, so hover, clicks on
  buildings and walkers, construction drags and the ghost use the zoomed picture. Positions outside the
  view are unchanged.
- Controls: the mouse wheel over the city view (zoom about the tile under the mouse) and keys for zoom
  in, zoom out and 100 % (zoom about the view's centre). Before binding them, the original's wheel and
  key handling on the city window is checked; a binding the original uses is replaced by a free one,
  recorded in `docs/enhanced.md`.

### B.3 State

- The zoom is not saved in games. The camera stays the original's (top-left of the view); a zoom change
  or a load at another zoom keeps the view's centre tile.
- Known effects, accepted: ambient city sounds count the tiles in view (more when zoomed out); edge
  scrolling moves the same number of tiles per step at every zoom.

### B.4 Tests

- At 100 % all checks as before.
- Picking: for points of the city, the tile under the mouse at 50, 75, 150 and 200 % equals the tile at
  100 % for the same city point.
- Wheel zoom keeps the tile under the mouse.
- Simulation equality as in A.4, with zoom 50 % and 200 %.

## Documentation

- `docs/enhanced.md`: every changed place (function, address, original behaviour, enhanced behaviour).
- `docs/presentation.md`: a section pointing to it.
- README: the display settings and controls; `--help`.
