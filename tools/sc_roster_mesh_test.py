#!/usr/bin/env python3
"""Test actual original selector mesh decoding and authentic fourth-column geometry; djultra64."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[1];O=R/'local/sc_roster'
subprocess.run(['python3',str(R/'tools/sc_roster_test.py')],check=True)
s=(O/'guest.cpp').read_text();funcs={}
for p in (R/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
for name in ['func_80004730','func_800BD1F0','func_800C03F0','func_800C07C8']:
 s=re.sub(r'void '+name+r'\(uint8_t\*,recomp_context\*\)\{[^\n]*\}\n','',s)
 s=s[:-2]+funcs[name]+'\n}\n'
# Exercise the REAL constructor's reused-state branch and original reset tail.
# This path must retain the already shifted mesh and its cached added column.
constructor=funcs['func_8005DDCC'];prefix=constructor[:constructor.index('    // 0x8005DDEC:')].replace('func_8005DDCC(', 'sc_roster_existing_constructor(',1)
tail=constructor[constructor.index('L_8005DFA4:'):]
s=s[:-2]+prefix+'sc_roster_unsupported("Unexpected fresh selector constructor");return;\n'+tail+'\n}\n'
for name in ['func_800B8DF4','func_800C1520']:
 if not re.search(r'void '+name+r'\(',s):s=s[:-2]+funcs[name]+'\n}\n'
fixtures={'func_800B8FC8','func_8009E36C','func_800B9154','func_800B59D0','func_800B3C70','func_800B4040','func_800B4408','func_800B474C','func_800B51F4','func_800B6A00','func_800B6E74','func_80097E54','func_8005D334','func_8005D3D4','func_8009B6B4','func_800A0CA8','func_800A30C0'}
refs=set(re.findall(r'(func_\w+)\(rdram, ctx\)',s));defs=set(re.findall(r'void (func_\w+)\(',s))
for name in refs-defs-fixtures:s=s[:-2]+f'void {name}(uint8_t*,recomp_context*){{sc_roster_unsupported("{name}");}}\n}}\n'
(O/'mesh-guest.cpp').write_text(s)
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
subprocess.run(['g++','-std=c++20','-O1','-g','-I'+sdk,'-I'+str(R/'generated/guest'),str(R/'tests/sc_roster_mesh.cpp'),str(R/'host/sc_assets.cpp'),str(R/'host/sc_roster.cpp'),str(O/'mesh-guest.cpp'),'-o',str(O/'mesh-test')],check=True)
subprocess.run([str(O/'mesh-test')],cwd=R,check=True,timeout=30)
