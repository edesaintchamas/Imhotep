You are working on Imhotep, a faithful reimplementation of Pharaoh + Cleopatra.
Read CLAUDE.md first and follow it strictly.

Task: resolve ONE item from docs/unverified.md (the item ID is given below).

Steps:
1. Read the item and the code it points to in core/.
2. Locate the rule in the reference binary using local/ghidra-export/ (index.json, then
   functions/<address>.json). refs/ may help you find where to look; it is never proof.
3. If you find it: implement it exactly (integer widths, rounding, ordering), replace the
   @unverified tag with @bin <address> <desc>, add the address to docs/provenance/README.md,
   remove the item from docs/unverified.md, and add or update a test.
4. If you cannot confirm it: do not guess. Add what you learned (candidate addresses,
   reasons for doubt) to the item's "Leads" column and stop.
5. Run `make test`. Do not run git commands.

Finish with a short summary: what you confirmed, with addresses, and what remains uncertain.
