# Differential tests

A differential test loads a save produced by the original game at month M, runs the Imhotep
simulation to month M+1, and compares the result with the original's save at month M+1.

Reference saves are **not** in the repository. Each contributor generates the corpus locally
in `local/saves/` from the campaign missions of their own installation:

1. Start a campaign mission in the original game.
2. Save immediately (`<mission>_m00.sav`), then at the start of each month (`_m01`, `_m02`, ...)
   without interacting, unless the scenario description says otherwise.
3. Record the corpus in `local/saves/manifest.json` (mission, month, game version, notes).

Exact procedure and comparison tooling: to be written with the save reader.
