#!/usr/bin/env python3
"""Isolated ROM-verified original Options rectangle helper test; djultra64."""
from pathlib import Path
import re,subprocess,tempfile
R=Path(__file__).resolve().parents[1];rom=(R/'probe-output/base.z64').read_bytes();assert rom[0x67A48:0x67A4C]==bytes.fromhex('0c016c7a')
body=None
for p in(R/'generated/guest').glob('funcs_*.c'):
 m=re.search(r'RECOMP_FUNC void func_8005B1E8\([^\n]*\).*?;}',p.read_text(),re.S)
 if m:body=m[0];break
assert body
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
with tempfile.TemporaryDirectory(prefix='clay-native-border-')as d:
 p=Path(d);(p/'guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+body+'\n}\n')
 subprocess.run(['g++','-std=c++20','-O1','-UNDEBUG','-I'+sdk,'-I'+str(R/'generated/guest'),str(R/'tests/options_border.cpp'),str(R/'host/options_border.cpp'),str(p/'guest.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
