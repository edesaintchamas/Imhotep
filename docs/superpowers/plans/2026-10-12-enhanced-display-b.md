# Enhanced display, milestone B: city zoom

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** the city view zooms (50, 75, 100, 125, 150, 200 %) with the mouse wheel and Cmd/Ctrl + `=` / `-` / `0`.

**Architecture:**
- **Geometry.** The view's geometry is set for a virtual view of (real size × 100 / z) pixels, so
  the camera bounds, the minimap frame and the tile under the mouse follow.
- **Drawing.** `city_view_draw` (0x00430D30) draws into an off-screen buffer of that size: it points
  the back-buffer globals at it, then restores them. The buffer is then scaled into the real view
  rectangle.
- **The mouse.** Mouse positions inside the view are mapped to virtual coordinates in
  `frame_screen_to_tile` (0x00430790).
- **At 100 %,** nothing changes.

**Tech Stack:** C11, SDL2.

**Spec:** docs/superpowers/specs/2026-10-12-enhanced-display-design.md (milestone B)

## Global Constraints

- At 100 % the code executed is today's (the oracle checks keep their meaning).
- The zoom is a display setting (`imhotep.cfg` `zoom=`), never saved in games, tagged `@imhotep enhanced zoom`.
- Zoom steps: 50, 75, 100, 125, 150, 200.

## Review Focus

1. **Zoom while the sidebar slides, or with a window open over the city:** no corrupted pixels, and
   the zoom applies once back in the city.
2. **Zooming out on a small map** (the view larger than the map): the camera stays within bounds,
   with no crash.
3. **Picking at the view's edges** at 50 % and 200 %: the tile under the mouse matches the picture.
4. **Construction drags across a zoom change:** a click still builds where it is drawn.
5. **Resizing the window while zoomed:** the geometry is recomputed at the same zoom.

---

### Task 1: zoom in the geometry, the mouse and the drawing

**Files:**
- core/sim/platform.h/.c: `display_zoom` callback and `platform_display_zoom()` (100 without it).
- core/newgame/reset.c: `view_geometry_a/b` use the zoomed size when the zoom is not 100.
- core/frame/frame.c: `frame_screen_to_tile` maps the mouse.
- core/frame/display.c: `display_zoom_apply(st, old)` recomputes the geometry and keeps the centre tile.
- present/city/view.c, present/city/zoom.c: the off-screen draw and the scaling.
- tests/unit/display_test.c.

- [ ] Unit tests:
  - `zoom_size(right, h, z)` gives the virtual size;
  - the mouse mapping at 50 % and 200 % maps the view's corners to the virtual view's corners.
- [ ] Implement the parts above; build; `check_same_code` (only these files); commit.

### Task 2: platform controls

- **Input:**
  - the mouse wheel over the city view, zooming about the mouse;
  - Cmd/Ctrl + `=` / `-` / `0` (not passed to the game);
  - `imhotep.cfg` `zoom=` read at start and written when changed;
  - the script command `zoom Z` for tests.
- **After a change:** `display_zoom_apply`, then a full redraw.
- **Check:** screenshots at 50, 75, 150 and 200 % (1920 × 1080). Picking is checked by right-clicking
  a tile and reading the info panel's title. Commit.

### Task 3: docs and release

- docs/enhanced.md, README (controls), `--help`.
- `oracle_play_check.py` at 100 %; `make test`.
- Release 1.1.0 (milestones A and B together, once the user has tried them).
