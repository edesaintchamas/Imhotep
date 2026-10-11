# Enhanced display

The enhanced display is Imhotep's own extension; it is not part of the original game. The game draws
at its window's size, at least 1024 x 768 game pixels. The design is in
docs/superpowers/specs/2026-10-12-enhanced-display-design.md. Each place below is tagged
`@imhotep enhanced fit` in the code, and `make provenance` counts these tags. They may not appear in
the simulation's code.

## Rules

- **When the extension applies.** With an original size (640 x 480, 800 x 600, 1024 x 768), the code
  executed is the original's. The new branches run only for other sizes. The fit size is never 1024
  wide unless it is 768 high: the original's layout tests the width alone, so a taller window 1024 wide
  is drawn 1026 wide.
- **What the simulation sees.** The resolution setting `0x00E38E68` stays 3, so every rule that
  switches on it behaves as in 1024 x 768, for example the storage rows (24) and the high-score list.
  Only the size globals hold the real size: `0x00E38DB0` / `0x00E38DAC` and those made from them
  (pitch, layout origin, clip, back buffer, spare screen).
- **The platform's role.** `sim_platform.display_fit` gives the size; `display_fit_apply` redoes the
  mode after a resize.

## Places

| Place | Original | Enhanced |
|---|---|---|
| `display_resolution_set` 0x004CFD60, core/frame/display.c | 3: 1024 x 768 | 3: the fit size when the platform fits (clamped to 1024..4096 x 768..4096) |
| display creation 0x004CF990, core/boot/display.c | 3: 1024 x 768 | the same |
| settings 0x004147F0, core/boot/settings.c | resolution from Pharaoh.inf | fit: resolution 3, confirmed |
| `display_fit_apply`, core/frame/display.c | (none) | F9's sequence (0x004161DC) for 3 at the new size, without the confirmation; not in windows 0x0E, 0x0F, 0x48 |
| `view_geometry_a/b` 0x004DDEC0 / 0x004DDF20, core/newgame/reset.c | tile counts per width | `view_tiles`: columns to reach the sidebar (rounded up), half-rows to reach the bottom; the view starts left of the screen by less than a tile so that it ends at the sidebar (the city draw skips a tile past its right edge); its clip starts at 0 (a pixel left of the screen would land at the end of the row above) |
| `sidebar_set_x` 0x0051E930, present/sidebar/sidebar.c | x per width | W - 186 expanded, W - 66 collapsed |
| `topbar_draw_background` 0x00536670 | the bar at x 0 / -240 / -360 | the bar repeated every 1000 pixels, its right part at W - 1024 |
| `topbar_draw` 0x0051F0A0 | texts per width | 1024's places + (W - 1024) |
| `sidebar_draw` 0x0051E990 | right edge, bottom strip, lower panel per width | right edge at W - 24, repeated down without its top 30 rows; bottom strip across W; lower panel repeated down |
| `sidebar_slide` 0x0051FD60 | clip right 1000 / 760 | W - 24; lower panels as above |
| image draw 0x0053E590, present/draw/image.c | (1024 x 768 pictures at (0, 0)) | a 1024 x 768 image drawn at (0, 0) on a wider screen: centred on black; windows 0x14 / 0x15 redraw the whole picture |
| info panel place, present/ui/info/info.c | per screen height | 768's rule: centred over the city view, at the top when the mouse is below the middle, else at the bottom |
| high-score list, present/ui/dispatch.c, present/ui/frontend/highscore.c | per resolution | resolution 3's place + the centring offset |

## Platform (platform/main.c)

- **Settings.** `imhotep.cfg` holds `display=fit` (the default) or `display=original`, and
  `scale=auto|1|2|3`. `--display fit|original|WxH` overrides `display`. Tests (`--hidden`, `--script`)
  use `original` unless `--display` says otherwise.
- **The window.** It opens during the boot, once the full-screen option is known from Pharaoh.inf.
  Full screen uses the desktop's size; otherwise the window takes the screen's usable area. It is
  resizable and high-DPI aware.
- **The game's size.** It is the window's pixels divided by the scale. On `auto`, the scale is the
  screen's pixels per point: 2 on Retina. The scale is lowered while the result is under 1024 x 768.
- **Scaling.** Whole-number nearest-neighbour scaling when the window holds the game, smooth scaling
  down when it does not.
- **Resizing.** A resize is applied 150 ms after the window's size stops changing: windowed, the
  screen's new size is delivered first (WM_DISPLAYCHANGE), then the mode is redone and the whole
  interface redrawn. In a window where the original changes no mode, it is tried again later. A window
  that is open keeps the place it was given when it opened (the info panel), as after F9 in the original.
  The script command `resize W H` does the same in tests.
- **The screen.** The screen that GetSystemMetrics answers is the fit size, so edge scrolling fires at
  the window's edges.
- **Full screen.** It follows `0x00E38E5C` (F6, the display options) as borderless desktop full
  screen, in every mode.
- **Choosing a mode.** In fit mode, F9 and the options' 1024 x 768 button mean "fit"; F7 and F8 and
  their buttons give 640 x 480 and 800 x 600. `display=original` gives the true 1024 x 768.

## The simulation and the screen

None of the simulation's rules read the size globals: the only ones that read the display are in
core/frame, core/boot, core/newgame (the view and the start's videos) and core/command (the camera).
One effect crosses over, and the original has it too: the random city sound (0x0053ACF0,
core/frame/sound.c) draws a random point of the view, then more draws by what is on that tile, from
the C library's generator (crt_rand). Some of the simulation (figures, gods, events) uses the same
generator, so the size of the view, the camera and the frame timing change later random outcomes. They
do this in the original between its three modes as well. A mode therefore does not promise the same
random outcomes as another, only the same rules.

## Tests

- **Unit tests.** `make unit` (tests/unit/display_test.c) checks the fit size and its clamps, and
  checks that the view tile rule reproduces the original's table.
- **The reference session.** `tests/differential/oracle_play_check.py` (original mode) stays equal to
  the original on every frame.
- **Screenshots.** Taken at 1280 x 768, 1512 x 982, 1920 x 1080 and 2560 x 1440 (`--display WxH
  --hidden --script`, `shot`): menus, city with the sidebar expanded and collapsed, info panel.
