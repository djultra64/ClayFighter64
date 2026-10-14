#!/usr/bin/env python3
"""Extract private original sprite bodies for Windows native guards; djultra64."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2];bodies={}
for source in (root/'generated/guest').glob('funcs_*.c'):
 for name in ['800A87C4','800A5868','800A5940','800A59B0','800438D8']:
  m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',source.read_text(),re.S)
  if m:bodies[name]=m[0]
if len(bodies)!=5:raise SystemExit('Missing original sprite writer')
body=bodies.pop('800438D8');body=re.sub(r'^.*clay::(?:begin_actor_piece|end_actor_piece)\(.*\);\n','',body,flags=re.M)
body=body.replace('    func_800A87C4(rdram, ctx);','    clay::begin_actor_piece(rdram,ctx);\n    func_800A87C4(rdram, ctx);\n    clay::end_actor_piece(rdram);')
(root/'generated/windows_interpolation_body.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\n#include "interpolation.h"\nextern "C" {\n'+body+'\n}\n')
(root/'generated/windows_interpolation_payload.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\n#include "sprite_seams.h"\nextern "C" {\n'+'\n'.join(bodies.values())+'\n}\n')
