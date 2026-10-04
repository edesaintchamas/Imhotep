# The game tick

Reference: GOG Pharaoh + Cleopatra 2.1. Port: `core/sim/tick.c` (from the disassembly). Check:
`tools/oracle/lockstep.py`. All addresses **@bin**.

## Time

51 ticks per day (`0x0078D7CC`, 0..50), 16 days per month (`0x0078D7C4`, 0..15), 12 months per
year (`0x0078D7C0`); year `0x0078D7BC`, total days `0x0078D7C8`. The main loop `0x004D6B60` calls
the tick `0x004D7090` (the oracle calls it directly, once per tick).

## `0x004D7090`, one tick

1. The RNG step (inline; `core/random`, `docs/formats/map.md` §4).
2. `0x004CDC70` (ten animation counters), `0x00485DC0`.
3. If the game is not paused (`0x00605A3C == 0`): the day dispatcher `0x00451C50`, `0x004E7E40`,
   `0x004BE770` (this = floodplain manager `0x00D05720`); then if `0x00D386D4 == 3` and city
   +0x4998 == 2, city +0x4998 := 0.
4. `0x00489B70`, `0x0041FF60`, `0x0044BCA0` (this = `0x00785D70`), `0x004EA2C0` (tail jump).

## `0x00451C50`, the day dispatcher

One step per tick, by the tick counter:

| Tick | Calls | Tick | Calls |
|---|---|---|---|
| 0 | `0x0051F780` (empty) | 0x14 | `0x00455900`, `0x004860D0` |
| 1 | `0x004E9890(1)` | 0x15 | `0x00457AF0` (road access) |
| 2 | `0x00443330` | 0x16 | `0x00458440` |
| 3 | `0x00F53FD0` := 1 | 0x17 | `0x00458540` |
| 4 | `0x004460B0` | 0x18 | `0x00459070` |
| 5 | `0x004B7790(0)` | 0x19 | `0x00459100`, `0x00459350`, `0x004595A0`, `0x0051F780` ×2, `0x00459670` |
| 6 | `0x0051F780` | 0x1B | `0x004555D0` |
| 7 | `0x00468860` | 0x1C | `0x00455430` |
| 8 | `0x0045E040` | 0x1D | `0x004B7790(1)` |
| 9 | `0x00452150` (`0x0078D784` := 0) | 0x1E | `0x00F53FD0` := 1 |
| 0xA | `0x0045AA30` | 0x1F | `0x00462180` |
| 0xB | city +0x24 := sum of 100 i16 at +0x25C4; maximum kept at +0x469C | 0x20 | `0x00444C20`, `0x0046F200` |
| 0xC | `0x004558B0` | 0x21 | `0x004677C0`, `0x00468170` |
| 0x10 | `0x0045AAA0` | 0x22 | `0x004684E0` |
| 0x11 | `0x0045D5D0` | 0x23 | `0x00454E20`; when city +0x4478 != 0 and `0x004C5120`(city +0x472C) == 4, only every other day (city +0x4990) |
| 0x12 | `0x0045E6C0` | 0x24 | `0x00454FA0` |
| 0x13 | `0x00457A40` | 0x25 | desirability `0x0046A0C0`, `0x0046A140` |
| 0x26 | `0x004539E0` | 0x2D | `0x00469B90`, `0x0047E290` |
| 0x27 | `0x004528B0` | 0x2E | `0x0051F780` |
| 0x28 | `0x004525F0` | 0x2F | `0x0046A060` |
| 0x2B | `0x00469730` | 0x30 | `0x004599A0` |
| 0x2C | `0x00468B70` | 0x31 | `0x004550E0` |
| | | 0x32 | `0x0051F780` |

Ticks 0xD–0xF, 0x1A, 0x29, 0x2A do nothing. Then, every tick: three sound timers (`0x0078D79C`,
`0x0078D7A8`, and `0x0078D7A0` in mode 1 of `0x00E38DDA`; at 0, `0x0055F561(2)` on `0x00E92BF8`),
the map reveal (`0x0078D7B0` counting down: water `0x00475CC0` and `0x004776F0` on a column,
`0x0046D840`; at 0 the whole map and `0x004B52F0` for every figure of type 0x5F), the timer
`0x0078D7A4` (clears `0x00EAAF5C`), the tick counter (at 51: 0 and the cascade), and
`0x004F3890` (tail jump) while `0x00EAAF5C` is set.

## `0x00452370`, a new day

Total days + 1, day + 1. After day 15, a new month: day 0; city +0x475C := +0x273C, +0x273C,
+0x4760, +0x4764 := 0, +0x438C + 1; `0x004E81D0`, `0x004599D0`, `0x0045A2E0`, `0x0045A350`,
`0x0045A3F0`, `0x00452160`, `0x004474E0`, `0x0045E420`, `0x004464F0`, `0x0044BA90` (this =
`0x00785D70`), `0x004EA740`, `0x004B7960`, `0x004E1850`, roads `0x0047B570`, water
`0x00475CC0`(whole map), routing grid `0x00517370`, `0x004E1640`; month + 1; `0x004E7700`,
`0x004BC7A0`, `0x00452200`; after month 11 a new year (month 0, `0x00E3908C` := 0,
`0x00BC9694` := 0, year + 1, `0x004E8630`, `0x00459E50`, `0x0045A450` … `0x0045A5D0`,
`0x004448E0`, `0x00455530`, `0x0046E180(1)`, `0x004C5300`), else `0x0046E180(0)`; in months 0 and
6 city +0x471C := 2; `0x004E9700`, `0x004EA000`; the autosave `0x004D4E20("last.sav")` when
`0x00E3908C == 0` and `0x00E38EC2 == 1`. On days 0 and 8 `0x004E8840`, and on day 8 `0x004BE640`
(this = `0x00D05720`); then `0x004E8DB0` and three one-time flags (`0x007027B8`, `0x007027BE`
with message 0x13E, `0x007027B9` with `0x00417AE0(1)` once city +0x24 reaches 150).
