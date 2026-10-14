#!/usr/bin/env python3
"""Check authentic text width and glyph dispatch using original guest routines; djultra64."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[1];O=R/'analysis/font_private';names=['80071738','8007191C','800C2D1C'];funcs={}
for p in (R/'generated/guest').glob('funcs_*.c'):
 for n in names:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',p.read_text(),re.S)
  if m:funcs[n]=m[0]
(O/'text_guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(funcs.values())+'\n}\n')
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
subprocess.run(['g++','-O1','-I'+sdk,'-I'+str(R/'generated/guest'),str(R/'analysis/font_text_probe.cpp'),str(O/'text_guest.cpp'),'-o',str(O/'text_probe')],check=True)
subprocess.run([str(O/'text_probe')],cwd=R,check=True)
