#!/usr/bin/env python3
"""Verify native prompts with the original private guest font/code; djultra64."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-root', type=Path, default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
parser.add_argument('--cxx', default='c++')
args = parser.parse_args()
font = ROOT / 'analysis/font_private/font.bin'
if not font.exists():
    if not (ROOT / 'probe-output/base.z64').exists():
        raise SystemExit('Private ROM/font unavailable; native prompt verification requires the local user ROM')
    subprocess.run(['python3', str(ROOT / 'tools/font_decode_probe.py')], cwd=ROOT, check=True)
if font.stat().st_size != 15148:
    raise SystemExit('Unexpected decoded original font extent')
# Do not embed ROM-derived guest functions or fonts into tracked test sources.
functions = {}
for path in (ROOT / 'generated/guest').glob('funcs_*.c'):
    source = path.read_text()
    for address in ('80071738', '8007191C', '800C2D1C', '8007145C', '80072C4C'):
        match = re.search(r'RECOMP_FUNC void func_' + address + r'\([^\n]*\).*?;\}', source, re.S)
        if match:
            body = re.sub(r'\s*clay::input::prepare_native_prompt_(?:measure|draw)\(rdram,\s*ctx\);', '', match[0])
            body = re.sub(r"\s*clay::input::restore_native_prompt_font\(rdram,\s*ctx->r4\);", "", body)
            functions[address] = body
if len(functions) != 5:
    raise SystemExit('Original native width/draw/strlen recompiled functions are missing')
with tempfile.TemporaryDirectory(prefix='clay-native-prompts-') as directory:
    work = Path(directory)
    guest = work / 'private_guest.cpp'
    guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n' + '\n'.join(functions.values()) + '\n}\n')
    stb = work / 'stb.cpp'
    stb.write_text('#define STB_IMAGE_IMPLEMENTATION\n#include "stb/stb_image.h"\n')
    runtime = args.sdk_root / 'deps/N64ModernRuntime'
    include = [ROOT / 'host', ROOT / 'generated/guest', runtime / 'N64Recomp/include', runtime / 'thirdparty/json', args.sdk_root / 'deps/rt64/src/contrib']
    command = [args.cxx, '-std=c++20', '-O1', '-UNDEBUG', '-pthread']
    command += ['-I' + str(path) for path in include]
    command += [str(ROOT / path) for path in ['tests/native_prompts.cpp', 'host/native_prompts.cpp', 'host/input.cpp', 'host/input_hud.cpp']]
    command += [str(guest), str(stb), '-o', str(work / 'verify')]
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run([str(work / 'verify')], cwd=ROOT, check=True)
