# Naming the state (phase 10)

Phase 10 replaces raw addresses in core/ and present/ by names, without changing the code the
compiler makes. A rewrite is accepted when `python3 tools/check_same_code.py` reports every object
the same, and `make -s check-files provenance` passes.

## Where names live

| What | Where | Form |
|---|---|---|
| record arrays (base, size, count, `X_ADDR(id)`) | `core/state/records.h`; messages in `layout/message.h` | `BUILDING_ADDR(id)`, `FIGURE_ADDR(id)`, `CURRENT_CITY(st)`, `MESSAGE_ADDR(k)` |
| record fields | `core/state/layout/<record>.h` (`building.h`, `figure.h`, `city.h`, `message.h`, ...) | offsets: `BLD_TYPE`, `FIG_ACTION`, `CITY_POPULATION`, `MSG_ID` |
| named globals | `core/state/layout/g_<area>.h` (`g_building.h`, `g_city.h`, `g_map.h`, `g_message.h`, `g_time.h`, `g_ui.h`, `g_figure.h`, `g_graphics.h`, ...); presentation-only globals in `present/layout/g_<area>.h` | `G_BUILDING_HIGHEST`, `G_MAP_START` |
| fields of a smaller structure | next to the structure's base, in its area header | `FM_X` (formations, `figure.h`), `GFXSLOT_IMAGES` (`g_graphics.h`), `ROUTE_KIND` (`g_scenario.h`) |
| original functions | `core/state/functions.h`, **generated** by `tools/gen/gen_functions.py` from the port tables | `FN_MESSAGE_POST` |
| everything at once | `#include "state/layout.h"` (core); `#include "present_layout.h"` (present/: core's plus `present/layout/g_*.h`) | |

A field of a smaller structure (a table entry of the exe, an object) is named with the structure's
prefix next to its base, e.g. `MSGTYPE_VIDEO` (entries of `G_MESSAGE_TYPES`), `NOTICE_WIDTH`
(`G_NOTICES`), `MONOBJ_PART_TABLE` (a monument object). Strides and counts of tables get names too
(`G_TYPE_TABLE_STRIDE`, `G_NOTICE_COUNT`).

## How code uses them

- Widths and signedness stay at the access, exactly as they were: `S16(st, b + BLD_TYPE)`,
  `U8(st, CURRENT_CITY(st) + CITY_U468B)`. Names carry no type; S16 stays S16.
- Arrays keep their index arithmetic: `S32(st, G_MESSAGE_PENDING + 4 * i)`,
  `U8(st, c + CITY_GOD_MOOD + (uint32_t) god)`; an element of a named field is `FIELD + 4 * k`.
- An address inside a record array is written from the array: `CITY_BASE + CITY_U28B6 + 2 * i`,
  `BUILDING_ADDR(BUILDING_COUNT) + BLD_CRIME_RISK`, not a raw constant.
- Calls: `GAME_CALL(st, FN_NOTICE_ADD_4E2970, ...)`, `GAME_CALLT(st, G_MONUMENT_E1, FN_..., ...)`,
  `state_unported(st, FN_...)`. The port tables keep the raw address: it is the entry's evidence.
- Comments keep addresses: `@bin 0x...` and the original's addresses in prose are evidence.
- Module headers that already named a global (e.g. `ROAD_ACCESS_X` in building/day_access.h) keep
  their name until their users are rewritten; rewritten code uses the layout name.

## How names are formed, and their evidence

- Prefix per kind: `G_` global, `FN_` function, `BLD_` building field, `FIG_` figure field, `CITY_`
  city field, `MSG_` message field; table and object fields by their structure (`MSGTYPE_`,
  `NOTICE_`, `MONOBJ_`).
- A name says what the evidence says, in the repository's words: `docs/formats/` (save.md §5 for the
  records), the ported functions' names and comments, `docs/provenance/ported.md`, the save schema's
  chunk numbers, and names other ports already gave the address in a local `#define`. Never from
  memory of the game, wikis or other projects.
- When the meaning is not established, the name is neutral: the record prefix and the offset
  (`BLD_U7F`, `CITY_U4478`), or `G_U` and the address (`G_UE951D8`). The comment says what is known
  (who writes it, what is tested). Rename it when the meaning is found.
- One offset with different meanings for different record types gets one name per meaning
  (`BLD_HOUSE_INVENTORY`, `BLD_INDUSTRY_PROGRESS`, `BLD_MONUMENT_KIND` at +0x54); the code picks the
  one its type check implies.
- `FN_` names are the port's name (`gen_functions.py`: upper case, notes dropped; a name two
  functions share gets the address appended). Renaming a function is renaming its port, then
  regenerating. The generator reads the port tables the way the build sees them: every `.c` file of
  core/ and present/ goes through the C preprocessor, so rows in included files
  (`present/ui/advisors/X_ports.inc`) and rows made by a macro (`LAY_ENTRY` in
  `core/building/tomb_stones.c`, whose name is two adjacent string literals) get their name too. A
  function without a port has no `FN_` name: code calls it by address (see "What stays raw").
- The city record has one record type: one offset, one name (the merge of phase 10.4 settled the
  fields two packages had named differently). Building and figure offsets keep one name per meaning
  by type, as above.
- Grids (one entry per tile, the save's grid chunks) are named `G_GRID_<what>` in `g_map.h`.
- Each name is defined in one header only (`rename.py --check` reports a name defined twice, even
  with the same value).
- Each name is defined **once**, with its evidence on the line or on the block's heading: an
  `@bin 0x...` address (the function that shows the meaning) or the doc section. The headers carry
  `@bin` tags, so `tools/check_provenance.py` counts them like other core files.

## Rules for a rewrite

1. Pure renames only: the object code must stay identical (`tools/check_same_code.py`). A change
   that is not a rename (a helper, a loop, a struct) is not part of this step.
2. Provenance tags stay with their rule. A local `#define` with its `@bin` tag that moves into a
   layout header keeps its evidence there.
3. `tools/naming/rename.py [--vars f=FIGURE,b=BUILDING] FILE...` does the mechanical part from the headers (address literals,
   local defines duplicating a global, fields of variables holding a record address); it lists what
   it left: offsets with several names, variables it could not type, raw addresses. A variable holds
   a record address only when every assignment in the function is exactly `X_ADDR(...)`; one
   assigned `X_ADDR(...) + FIELD` points at a field and is left alone (before phase 10.4 the rule
   matched the prefix only and added `+ BLD_STATE` to such pointers: the audit found and reverted
   them, `core/command/orders_storage.c`). The field at offset 0 is added only on record addresses.
   The address column of the port tables (`{0x...u, "name", ...}` rows and `*_ENTRY(n, 0x...u)` macro
   rows) is never rewritten. Local macros
   that are a record address (`#define B(id) BUILDING_ADDR(id)`) are followed. `--vars` types
   variables the code does not assign (parameters such as `uint32_t f`): give it only after checking
   that in these files the name always holds that record's address. Review every result by hand: a
   wrong name compiles to the same code.
4. `tools/naming/rename.py --check`: one name per address in the global headers.
5. Remaining numbers that are not addresses or record offsets (message ids, window ids, building
   types, limits) are out of this step.

## Packages and area headers (phase 10.3)

| Package | Directories | Owns |
|---|---|---|
| P1 | core/figure | `g_figure.h`, `figure.h` |
| P2 | core/building | `g_building.h`, `building.h` |
| P3 | core/city | `g_city.h`, `g_message.h`, `g_time.h`, `city.h`, `message.h` |
| P4 | core/command | `g_command.h` |
| P5 | core/map, sim, state, random, graphics, model | `g_map.h` (and every grid), `g_sim.h` (the tick, the RNG `G_RANDOM_*`, `G_CRT_RAND_STATE`), `g_time.h` (the date), `g_graphics.h` (the graphics object, its slots and image entries), `functions.h` (regenerate only); core/model writes tables named by their users (`G_PROPS`, `G_MODEL_BUILDING`, `G_HOUSE_LEVELS`, `G_SENTIMENT_TABLE`) |
| P6 | core/boot, io, newgame | `g_boot.h`, `g_scenario.h` |
| P7 | core/frame, present/play, present/boot | `g_frame.h`, `g_ui.h`, `present/layout/g_play.h` |
| P8a | present/ui (its own files), ui/messages, ui/empire | `present/layout/g_ui_main.h` |
| P8b | present/ui/info | `present/layout/g_ui_info.h` |
| P8c | present/ui/advisors, ui/frontend | `present/layout/g_ui_advisors.h` |
| P8d | present/ui/frontend, ui/empire | `present/layout/g_ui_frontend.h` |
| P9 | present/city, sidebar, draw | `present/layout/g_city_view.h` |

A global used by core lives in a core header (the package that uses it most); present/ headers hold
only what present/ alone uses. `tools/naming/harvest.py` holds this table (`AREAS`).

## Harvested names

Before phase 10 the ports named about 820 addresses in local `#define`s. Wave 0 collected them
(`python3 tools/naming/rename.py --harvest`) into `tools/naming/candidates.tsv` (address, name, area
header, status, uses, the local names with the number of files defining each, the evidence comment)
and seeded each area header with its share, between `harvested` markers. Statuses: `named`
(already in a layout header), `record` (inside a record array: write the record expression, not a
global), `single` / `majority` (the local name), `resolved` (a tie settled by reading the ports'
comments), `generic` (a generic local name qualified by its file), `+clash` (a name two addresses
had: the less used one qualified by its file), `unresolved` (a tie the comments do not settle: the
neutral `G_U<address>`, alternatives in the table). The package agents checked the names they used
and moved them above the markers. At the cleanup (10.4) the empty marker blocks were removed and the
few names left in a block stay under a plain heading ("named after the ports' local #defines"), their
evidence the defining port in the comment. Do not run `--harvest --write` again.

## Pending names

An agent never edits another package's header. A global it needs that belongs to another area (or
that it is not sure where it belongs) goes into its own fragment,
`core/state/layout/pending/<package>.h`, included by `layout.h` (one `#include` line per fragment,
added with the fragment), as an ordinary `#define G_... 0x...u /* evidence [owner header] */`; a
record field that the owner has not named yet goes there too (`BLD_...`, `FIG_...`: rename.py and
resolve.py read the pending files). The owner is said in brackets at the end of the line, or by the
block's heading ("city record fields (owner: city.h, P3)"). There are no fragments now: phase 10.4
merged all of them.

### Merging the fragments (after a wave)

1. `python3 tools/naming/resolve.py --dry`, then without `--dry`: the aliases of
   `tools/naming/aliases.txt` (names a package renamed, `OLD NEW`, applied to all code) and one name
   per address (an area header's name over a pending one, a meaningful one over a neutral one). A
   package branched before a rename: `resolve.py --from REF` maps the names the build reports
   undeclared to this tree's name for the same address. A name defined in a present/ header that core
   uses moves to a core header.
2. Each pending line goes to its owner: the bracketed header, else the header its block's heading
   names, else the area of the nearest named address (a present/ header only when core does not use
   the name); record and structure fields by prefix (`CITY_` city.h, `BLD_` building.h, `FIG_` / `FM_`
   figure.h, `MSG_` / `NOTICE_` message.h, other structures where their base is named). A name
   already defined in an area header is dropped; the moved lines keep their block's heading with the
   package in brackets.
3. Fields: a city offset named twice keeps the meaningful name (an area header's first), the others
   become aliases (added to `aliases.txt`, uses renamed); an area header's neutral name gives way to
   a pending meaningful one, whose comment takes over the evidence of both. Fields of one structure
   under two prefixes are unified (phase 10.4: the pending `FORM_*` became figure.h's `FM_*`), and a
   field named relative to another base is rewritten from the table's base (`G_FIGURE_TYPES +
   FIGTYPE_U04` became `G_PROPS + FIGTYPE_ATTACK`: same constant).
4. Empty the fragments, remove their `#include` lines, then `python3 tools/check_same_code.py`,
   `python3 tools/naming/rename.py --check` and `make -s check-files provenance`.

## Generated tables stay raw

`core/boot/boot_data.c`, `core/io/save_tables.c`, `core/io/save_schema.c`, `core/io/save_relayout.c`,
`core/city/ring_tables.c` and `core/map/terrain_tables.c` are written by `tools/gen/gen_*.py` from
the reference binary: they transcribe the exe's own tables (an address and its value, a chunk
address and its size). They keep raw addresses: the address is the transcription's key and its
evidence (the save schema is where chunk addresses come from in the first place), and naming them
would make the generators depend on the layout headers and change their output with every rename,
for no change in the code.

## What stays raw (phase 10.4)

In hand-written code no state address is left raw outside the definitions of the record bases
(`core/state/records.h`, `core/state/state.h`). Numbers that remain are not state addresses:
terrain flag masks, float bit patterns and multipliers that fall in the address range by chance
(`0xEEFED77F`, `0x7FFFFF`, `0xA3D70A`), message, window and building-type ids, limits. Code
addresses that remain (about 70):

- calls to functions of the original that have no port (`GAME_CALL(st, 0x004DE430u, ...)`, about
  45): they get an `FN_` name with their port;
- `state_unported(st, 0x...)` at an address inside a function (the instruction not reproduced) and
  the call-site tags of `boot_alloc` (the return address the original's allocator saw): not
  function entry points;
- function pointers the original stores in its state (list callbacks, a `qsort` comparator), for
  functions without a port;
- the port tables' address column, and the step names of `core/boot/boot.c` (strings).
