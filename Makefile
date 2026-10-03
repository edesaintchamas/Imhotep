# Imhotep — development commands. See CLAUDE.md.

IMHOTEP_GAME_DIR ?= local/game
GHIDRA_HOME      ?= $(HOME)/ghidra
PHARAOH_INSTALLER ?=
GAME_EXE         ?= $(IMHOTEP_GAME_DIR)/Pharaoh.exe
GHIDRA_PROJ_DIR  := local/ghidra
GHIDRA_EXPORT    := local/ghidra-export
export IMHOTEP_GAME_DIR

.PHONY: help verify fingerprint extract refs analyze provenance check-files unit differential test

help:
	@grep -E '^[a-z-]+:.*##' $(MAKEFILE_LIST) | sed 's/:.*##/ —/'

verify: ## Check game files against docs/source-fingerprint.json
	python3 tools/fingerprint.py

fingerprint: ## Record the reference fingerprint (once, on the reference installation)
	python3 tools/fingerprint.py --init

extract: ## Extract the GOG installer into local/game (needs: brew install innoextract)
	@test -n "$(PHARAOH_INSTALLER)" || (echo "Set PHARAOH_INSTALLER=/path/to/setup_pharaoh_*.exe" && exit 1)
	mkdir -p $(IMHOTEP_GAME_DIR)
	innoextract --output-dir $(IMHOTEP_GAME_DIR) "$(PHARAOH_INSTALLER)"

refs: ## Clone reference projects into refs/ (read-only, git-ignored)
	mkdir -p refs
	test -d refs/julius    || git clone --depth 1 https://github.com/bvschaik/julius refs/julius
	test -d refs/augustus  || git clone --depth 1 https://github.com/Keriew/augustus refs/augustus
	test -d refs/akhenaten || git clone --depth 1 https://github.com/dalerank/Akhenaten refs/akhenaten

analyze: verify ## Ghidra headless analysis and export to local/ghidra-export
	@test -f "$(GAME_EXE)" || (echo "Executable not found: $(GAME_EXE) (set GAME_EXE)" && exit 1)
	mkdir -p $(GHIDRA_PROJ_DIR) $(GHIDRA_EXPORT)
	"$(GHIDRA_HOME)/support/analyzeHeadless" $(GHIDRA_PROJ_DIR) pharaoh \
		-import "$(GAME_EXE)" -overwrite \
		-scriptPath tools/ghidra -postScript ExportFunctions.py $(GHIDRA_EXPORT)

provenance: ## Provenance report for core/
	python3 tools/check_provenance.py

check-files: ## Fail if game content is tracked by git
	python3 tools/check_no_game_files.py

unit: ## Unit tests (to be wired when the build exists)
	@if [ -f CMakeLists.txt ]; then cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure; else echo "unit: no build yet"; fi

differential: ## Differential tests against local/saves (to be written with the save reader)
	@echo "differential: not implemented yet"

test: check-files provenance unit differential ## All checks
