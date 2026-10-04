# Building model files (`Pharaoh_Model_<difficulty>.txt`)

Reference: GOG Pharaoh + Cleopatra 2.1. Loader **@bin 0x00540C90** (called from the scenario
setup `0x00413ED0` and the scenario start `0x004D52A0`). Parser: `core/model/building_model.c`.

## Choice of file

The file is chosen by the difficulty (save chunk 174, `0x00E39080`) through the table
`0x00601908`: 0 `VeryEasy`, 1 `Easy`, 2 `Normal`, 3 `Hard`, 4 `Impossible`. The five files
differ.

## Parsing (**@bin**, exact)

The file is read into a zeroed 100,000-byte buffer. Every search runs over the file length from
the current position.

1. Find `ALL BUILDINGS` (`0x004CBDD0`; offset 0 if absent).
2. Count the `{` that follow, up to 299 (`0x004CBE30`). If the count is not 0x101 (257), the game
   shows "Building model files out of sync" and asks whether to continue.
3. From `ALL BUILDINGS`, read **237 records** into `0x00705EA4 + type · 0x28`. Records stop when
   the target passes `0x007083AB`. For each record:
   - skip past the next `{`;
   - then 10 times: skip to the next digit or `-` (`0x004CC0E0`, at most 999 bytes), read the
     number (`0x004CC070`: an optional `-`, then decimal digits with place values from
     `0x005E058C` = 1, 10, 100, …), and skip past the next `,`.
4. From `ALL HOUSES`, read **20 records of 22 numbers** into `0x0070CDCC + level · 0x58` the same
   way.

Text outside the braces (names, comments) is ignored, apart from the characters the searches
stop on.

## Building record (10 × i32, `0x00705EA4 + type · 0x28`)

| # | Field (file header) | Read by |
|---|---|---|
| 0 | a: cost | |
| 1 | b: initial desirability | **@bin 0x0046A0C0** (`0x00705EA8`) |
| 2 | c: desirability step (rings) | **@bin 0x0046A0C0** |
| 3 | d: desirability step size | **@bin 0x0046A0C0** |
| 4 | e: desirability range (rings, at most 6 used) | **@bin 0x0046A0C0** |
| 5 | f: employees | **@bin 0x00462180, 0x00468B70** (`0x00705EB8`) |
| 6 | g: fire risk increment | **@bin 0x00468B70** |
| 7 | h: damage risk increment | **@bin 0x00468B70** |
| 8, 9 | (unlabelled in the file header) | *not traced* |

The record's position in the file is the building type (the leading `N,` on each line is not
read). Type names come from text group 41 (`text.md`). The house record fields are listed in the
file header (a – v: evolve thresholds, needs, crime, prosperity, capacity, tax, malaria, disease);
the house evolution code (`0x00454190`) reads them. They are not yet checked field by field.
