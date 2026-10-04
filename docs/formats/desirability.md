# Desirability grid (save chunk 16, `0x00AC2D4C`, i8 per tile)

Reference: GOG Pharaoh + Cleopatra 2.1. Port: `core/city/desirability.c`. Differential test:
`tests/differential/desirability_check.c` (`build/desirability_check`).

## When

The tick dispatcher `0x00451C50` runs `0x0046A0C0` then `0x0046A140` at tick 37 (0x25) of every
day (51 ticks per day). The whole grid is rebuilt from scratch each time, so a save holds the grid
of the last tick 37 (**@bin**).

## Rules (**@bin**)

1. **Clear.** `0x0046A0C0` zeroes the grid (51,984 bytes).
2. **Buildings.** Buildings 1 … highest id in use (chunk 93, `0x0078D790`), in id order. A
   building counts only when its state (+0x00) is 1; state 3 ("created") is skipped. Each one
   spreads its model values (`model.md`: b value, c step, d step size, e range) around its
   footprint. Inputs: x +0x06, y +0x08, size +0x03, type +0x10.
3. **Tiles.** `0x0046A140` walks the map rectangle, rows then columns:
   - bitfields 0x80 clear and terrain 0x20 (gardens): model record 39, size 1;
   - bitfields 0x80 clear and terrain 0x1000 (rubble): value −2, step 1, step size +1, range 2;
   - bitfields 0x80 set and terrain 0x40 (plaza on road): model record 0x26;
   - bitfields 0x80 set and terrain 2: model record 10;
   - bitfields 0x80 set, neither: the 0x80 bit is cleared, a side effect of this pass.
4. **Spread** (`0x00481830`). For rings r = 1 … min(range, 6): add the current value to ring r.
   Every `step` rings the value changes by `step size` (with step 0, after every ring). Footprint
   sizes 0 and above 6 add nothing.
5. **Ring** (`0x004818A0`). Ring r of an n × n footprint is the square ring at Chebyshev distance
   r. 1×1 rings are generated at run time (`0x0046F8D0`); 2×2 … 6×6 rings are static tables,
   checked against this rule (`tools/gen/gen_ring_tables.py`). The value is passed as a byte, and
   each addition is byte arithmetic that wraps, then clamped to [−100, 100].
   - Fast path, when the whole ring lies within x −1 … width and y −1 … height: every tile is
     changed, then clamped.
   - Edge path: each tile is tested (`0x00481AA0`), and tiles outside −1 … width / height are
     skipped. **Quirks, reproduced:** the clamp is applied to the footprint's own tile, not the
     tile just changed (so that tile can wrap); and for 6×6 footprints the test reads ring r + 1's
     tile list (ring 6 for r = 6) at the same index.

Addition order matters only through the clamping, and the port keeps it: buildings by id, then
tiles in row order.

## Verification (`build/desirability_check`)

- **Player saves** (downloaded, `tests/differential/README.md`): 7 / 7 exact, 363,888 tiles.
  Difficulties 0, 2 and 4, up to 1,770 buildings, including 26 towers.
- **Campaign records:** 49 / 53 exact. The other 4 equal the update over buildings 1 … k exactly:
  their last buildings were placed after the last tick 37 (scenario set-ups saved before the next
  daily update). In total 2,754,187 / 2,755,152 tiles are equal.
