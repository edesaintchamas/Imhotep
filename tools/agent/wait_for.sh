#!/bin/sh
# Imhotep - wait for a background job's output, and always stop.
# Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
#
#   tools/agent/wait_for.sh FILE PATTERN [MAX_SECONDS] [IDLE_SECONDS] [ERROR_PATTERN]
#
# Waits until FILE contains PATTERN (grep -E), then prints the end of FILE and exits 0.
# Also stops, printing the end of FILE, when:
#   - FILE contains an error line (ERROR_PATTERN, grep -E; default: a Python traceback, a killed or
#     crashed process; a check that prints per-case failures is not stopped by them): exit 2;
#   - FILE has not grown for IDLE_SECONDS (default 1200): exit 3 (the job died or hangs);
#   - MAX_SECONDS have passed (default 3600): exit 4.
# Use it instead of hand-written `until grep ...; do sleep; done` loops, which never end when
# the job fails or prints something unexpected.
file=$1 pattern=$2 max=${3:-3600} idle=${4:-1200}
errors=${5:-'^Traceback|^Killed|Segmentation fault|^Abort trap'}
[ -n "$file" ] && [ -n "$pattern" ] || { echo "usage: $0 FILE PATTERN [MAX_SECONDS] [IDLE_SECONDS]"; exit 1; }
start=$(date +%s) last_size=-1 last_change=$start
while :; do
    now=$(date +%s)
    if [ -f "$file" ]; then
        if grep -qE "$pattern" "$file"; then tail -n 40 "$file"; exit 0; fi
        if grep -qE "$errors" "$file"; then
            echo "wait_for: error in $file"; tail -n 40 "$file"; exit 2
        fi
        size=$(wc -c < "$file")
        if [ "$size" != "$last_size" ]; then last_size=$size last_change=$now; fi
    fi
    if [ $((now - last_change)) -ge "$idle" ]; then
        echo "wait_for: $file unchanged for ${idle}s"; [ -f "$file" ] && tail -n 40 "$file"; exit 3
    fi
    if [ $((now - start)) -ge "$max" ]; then
        echo "wait_for: gave up after ${max}s"; [ -f "$file" ] && tail -n 40 "$file"; exit 4
    fi
    sleep 15
done
