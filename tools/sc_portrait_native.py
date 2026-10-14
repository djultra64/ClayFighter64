#!/usr/bin/env python3
"""Extract authentic private idle pieces using the original loader/sequence/sprite parser; djultra64."""
from pathlib import Path
import re,subprocess
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'local/sc_portraits';OUT.mkdir(parents=True,exist_ok=True)
funcs={}
for p in(ROOT/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
s=(ROOT/'analysis/sc_loader/guest_slice.cpp').read_text();added={};pending=['func_80002DE0','func_80001AA8']
fixtures={'func_80097E54','func_800B6A00','func_800B8FC8','func_800B4040','func_800B3C70','func_800B474C','func_800B9154','func_800B4408','func_800B6E74','func_800B51F4','func_8009E36C','func_80001BE0','func_8003C740'}
while pending:
 n=pending.pop()
 if n in added or n in fixtures or re.search(r'RECOMP_FUNC void '+n+r'\(',s):continue
 if n not in funcs:raise RuntimeError(n)
 added[n]=funcs[n]
 pending +=[x for x in re.findall(r'(func_\w+)\(rdram, ctx\)',funcs[n])if x not in fixtures]
for n,body in added.items():
 s=re.sub(r'void '+n+r'\(uint8_t\*,recomp_context\*\) \{[^\n]*\}\n','',s)
 s=s[:-2]+body+'\n}\n'
(OUT/'guest.cpp').write_text(s)
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
subprocess.run(['g++','-std=c++20','-O1','-g','-I'+sdk,'-I'+str(ROOT/'generated/guest'),str(ROOT/'tests/sc_portrait_native.cpp'),str(ROOT/'host/sc_assets.cpp'),str(OUT/'guest.cpp'),'-o',str(OUT/'extract')],check=True)
for ident in [6,11,12,16]:subprocess.run([str(OUT/'extract'),str(ROOT/'analysis/sc_assets/asset-image.z64'),str(ident),str(OUT)],cwd=ROOT,check=True,timeout=30)
