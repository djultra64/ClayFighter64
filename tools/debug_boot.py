#!/usr/bin/env python3
"""Capture a stalled native boot under its parent debugger. By djultra64."""
import argparse
import json
import os
from pathlib import Path
import signal
import hashlib
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--after', type=float, default=12)
parser.add_argument('game_args', nargs=argparse.REMAINDER)
args = parser.parse_args()
# Keep the debugger's executable/DWARF immutable while parallel work rebuilds.
# Replacing build/claytality64 during a live GDB run corrupts lazy symbol reads.
source_binary = ROOT / 'build/claytality64'
binary_hash = hashlib.sha256(source_binary.read_bytes()).hexdigest()
debug_binary = ROOT / 'local/debug-binaries' / binary_hash / 'claytality64'
debug_binary.parent.mkdir(parents=True, exist_ok=True)
if not debug_binary.exists():
    shutil.copy2(source_binary, debug_binary)
commands = ['gdb', '-batch', '-ex', 'set pagination off', '-ex', 'catch throw', '-ex', 'run',
            '-ex', 'thread apply all bt 24', '-ex', 'info registers',
            '-x', str(ROOT / 'tools/thread_snapshot.gdb'),
            '--args', str(debug_binary), '--seconds', str(args.after + 30),
            *[arg for i,arg in enumerate(args.game_args) if i!=0 or arg!='--']]
with (ROOT / 'analysis/boot-debug.log').open('w') as log:
    process = subprocess.Popen(commands, cwd=ROOT, stdout=log,
                               stderr=subprocess.STDOUT, start_new_session=True)
    deadline = time.monotonic() + args.after
    last = None
    unchanged = 0
    while process.poll() is None and time.monotonic() < deadline:
        time.sleep(1)
        with (ROOT / 'analysis/boot-debug.log').open('rb') as current:
            current.seek(0, 2)
            size = current.tell()
            current.seek(max(0, size - 65536))
            lines = current.read().decode(errors='replace').splitlines()
        for line in reversed(lines):
            if '"event":"progress"' not in line:
                continue
            try:
                progress = json.loads(line)
            except ValueError:
                break
            if last is not None and progress['vi'] != last['vi']:
                unchanged = unchanged + 1 if (progress['gfx'], progress['polls']) == (last['gfx'], last['polls']) else 0
            last = progress
            break
        if unchanged >= 2:
            print('Detected stopped graphics/input while VI continues; capturing debugger state.')
            break
    if process.poll() is None:
        os.kill(process.pid, signal.SIGINT)
    try:
        process.wait(timeout=15)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=5)
print(ROOT / 'analysis/boot-debug.log')
