#!/usr/bin/env python3
"""Execute both original native sprite writers; verify shared-edge UV continuity. djultra64."""
import argparse,re,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');p.add_argument('--capacity',action='store_true');p.add_argument('--fourbit',action='store_true');a=p.parse_args()
names=['800A6520','800A87C4','800A5868','800A5940','800A59B0','800B1F38','800B1ED0','800C1D40'];bodies={}
for source in (root/'generated/guest').glob('funcs_*.c'):
 s=source.read_text()
 for name in names:
  m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',s,re.S)
  if m:
   body=re.sub(r'^.*clay::(?:begin_sprite_slices|end_sprite_slices|limit_sprite_slice_rows)\(.*\);\n','',m[0],flags=re.M)
   if name=="800A87C4":body=body.replace("L_800A94C4:","L_800A94C4:\n if(sprite_seam_fixture_limit)clay::limit_sprite_slice_rows(rdram,ctx,0x18c);")
   if name=="800A6520":body=body.replace("L_800A75E8:","L_800A75E8:\n if(sprite_seam_fixture_limit)clay::limit_sprite_slice_rows(rdram,ctx,0x154);")
   bodies[name]=body
if len(bodies)!=len(names):raise SystemExit('Original native sprite routines unavailable')
with tempfile.TemporaryDirectory(prefix='clay-sprite-seams-') as t:
 t=Path(t);guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\n#include "sprite_seams.h"\nextern bool sprite_seam_fixture_limit;\nextern "C" {\n'+'\n'.join(bodies.values())+'\n}\n')
 binary=t/'sprite_seams';subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/('tests/sprite_fourbit_guard.cpp' if a.fourbit else 'tests/sprite_capacity.cpp' if a.capacity else 'tests/sprite_seams.cpp')),str(root/'host/sprite_seams.cpp'),str(guest),'-o',str(binary)],check=True)
 if a.capacity or a.fourbit:
  subprocess.run([str(binary)],check=True);raise SystemExit(0)
 red=subprocess.run([str(binary),'original'],capture_output=True,text=True);print(red.stdout,end='');print(red.stderr,end='')
 if red.returncode!=1:raise SystemExit('Expected original shared-edge discontinuity RED exit1')
 subprocess.run([str(binary)],check=True)
