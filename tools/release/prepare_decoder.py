#!/usr/bin/env python3
"""Generate the local font-decoder fixture from your recompiled ROM. djultra64."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'generated/release-decoder'
OUT.mkdir(parents=True,exist_ok=True)
functions={}
for path in (ROOT/'generated/guest').glob('funcs_*.c'):
    for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',path.read_text(),re.S):
        functions[m[1]]=m[0]
names=['func_800BE590','func_800BE654','func_800BE6BC','func_800BE6C8']
(OUT/'guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(functions[n] for n in names)+'\n}\n')
print(OUT/'guest.cpp')
