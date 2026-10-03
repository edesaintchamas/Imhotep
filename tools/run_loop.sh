#!/usr/bin/env bash
# Headless Claude Code loop: one branch per open item in docs/unverified.md.
# Claude Code edits files; this script runs the tests and commits. You review and merge.
#   MAX_ITEMS=3 tools/run_loop.sh
set -euo pipefail
cd "$(dirname "$0")/.."
MAX_ITEMS="${MAX_ITEMS:-3}"
BASE="$(git rev-parse --abbrev-ref HEAD)"
mkdir -p local/logs
make verify

n=0
while [ "$n" -lt "$MAX_ITEMS" ] && item="$(python3 tools/next_unverified.py)"; do
  n=$((n + 1))
  branch="re/$item"
  echo "== [$n/$MAX_ITEMS] $item"
  git switch -c "$branch"
  claude -p "$(cat prompts/verify_item.md)

Item: $item" \
    --allowedTools "Read,Edit,Write,Grep,Glob,Bash(make test*),Bash(make provenance*),Bash(python3 tools/*)" \
    --permission-mode acceptEdits \
    --output-format json > "local/logs/$item.json" || true

  if make test && python3 tools/check_provenance.py --diff "$BASE"; then
    git add -A && git commit -m "re: $item"
    echo "   committed on $branch"
  else
    git stash push -u -m "failed $item" >/dev/null || true
    echo "$item" >> local/loop-skip.txt
    echo "   FAILED, changes stashed, item skipped"
  fi
  git switch "$BASE"
done
echo "Done: $n item(s). Review branches with: git branch --list 're/*'"
