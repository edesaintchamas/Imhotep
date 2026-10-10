#!/usr/bin/env python3
"""Imhotep - fail if the Pharaoh.exe hash the game embeds (platform/fingerprint.c) is not the
reference one recorded in docs/source-fingerprint.json.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ref = json.load(open(os.path.join(ROOT, 'docs', 'source-fingerprint.json')))['files']
want = next(v for k, v in ref.items() if k.lower() == 'pharaoh.exe')
src = open(os.path.join(ROOT, 'platform', 'fingerprint.c')).read()
m = re.search(r'FINGERPRINT_EXE_SHA256\[\]\s*=\s*"([0-9a-f]{64})"', src)
if not m or m.group(1) != want:
    sys.exit('platform/fingerprint.c: the embedded Pharaoh.exe hash is %s, docs/source-fingerprint.json has %s'
             % (m.group(1) if m else 'missing', want))
print('OK: the embedded Pharaoh.exe hash is the reference one')
