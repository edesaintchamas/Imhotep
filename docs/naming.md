# Naming the state (phase 10)

Phase 10 replaces raw addresses in core/ and present/ by names, without changing the code the
compiler makes. A rewrite is accepted when `python3 tools/check_same_code.py` reports every object
the same, and `make -s check-files provenance` passes.

## Where names live

| What | Where | Form |
|---|---|---|
| record arrays (base, size, count, `X_ADDR(id)`) | `core/state/records.h`; messages in `layout/message.h` | `BUILDING_ADDR(id)`, `FIGURE_ADDR(id)`, `CURRENT_CITY(st)`, `MESSAGE_ADDR(k)` |
| record fields | `core/state/layout/<record>.h` (`building.h`, `figure.h`, `city.h`, `message.h`, ...) | offsets: `BLD_TYPE`, `FIG_ACTION`, `CITY_POPULATION`, `MSG_ID` |
| named globals | `core/state/layout/g_<area>.h` (`g_building.h`, `g_city.h`, `g_map.h`, `g_message.h`, `g_time.h`, `g_ui.h`, `g_figure.h`, ...); presentation-only globals in `present/layout/g_<area>.h` | `G_BUILDING_HIGHEST`, `G_MAP_START` |
| original functions | `core/state/functions.h`, **generated** by `tools/gen/gen_functions.py` from the port tables | `FN_MESSAGE_POST` |
| everything at once | `#include "state/layout.h"` | |

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
  regenerating.
- Each name is defined **once**, with its evidence on the line or on the block's heading: an
  `@bin 0x...` address (the function that shows the meaning) or the doc section. The headers carry
  `@bin` tags, so `tools/check_provenance.py` counts them like other core files.

## Rules for a rewrite

1. Pure renames only: the object code must stay identical (`tools/check_same_code.py`). A change
   that is not a rename (a helper, a loop, a struct) is not part of this step.
2. Provenance tags stay with their rule. A local `#define` with its `@bin` tag that moves into a
   layout header keeps its evidence there.
3. `tools/naming/rename.py FILE...` does the mechanical part from the headers (address literals,
   local defines duplicating a global, fields of variables holding a record address); it lists what
   it left: offsets with several names, variables it could not type, raw addresses. Review every
   result by hand: a wrong name compiles to the same code.
4. `tools/naming/rename.py --check`: one name per address in the global headers.
5. Remaining numbers that are not addresses or record offsets (message ids, window ids, building
   types, limits) are out of this step.
