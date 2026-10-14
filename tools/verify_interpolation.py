#!/usr/bin/env python3
"""Hidden native sprite DL bounds and interpolation guards; djultra64."""
import argparse,re,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args()
include=['-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include')]
with tempfile.TemporaryDirectory(prefix='clay-interpolation-') as t:
 t=Path(t);bodies={}
 for source in (root/'generated/guest').glob('funcs_*.c'):
  s=source.read_text()
  for name in ['800A87C4','800A5868','800A5940','800A59B0','800438D8']:
   m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',s,re.S)
   if m:bodies[name]=m[0]
 if len(bodies)!=5:raise SystemExit('Original private sprite writer/state routines unavailable')
 guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(bodies.values())+'\n}\n')
 body=bodies.pop('800438D8')
 body=re.sub(r'^.*clay::(?:begin_actor_piece|end_actor_piece)\(.*\);\n','',body,flags=re.M)
 body=body.replace('    func_800A87C4(rdram, ctx);','    clay::begin_actor_piece(rdram,ctx);\n    func_800A87C4(rdram, ctx);\n    clay::end_actor_piece(rdram);')
 bodyguest=t/'body_guest.cpp';bodyguest.write_text('#include "recomp.h"\n#include "funcs.h"\n#include "interpolation.h"\nextern "C" {\n'+body+'\n}\n')
 guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(bodies.values())+'\n}\n')
 for name,sources in [('body',['tests/interpolation_body.cpp','host/interpolation.cpp','host/graphics_config.cpp']),('guards' ,['tests/interpolation.cpp','host/interpolation.cpp','host/graphics_config.cpp']),('payload',['tests/interpolation_payload.cpp'])]:
  command=[a.cxx,'-std=c++20','-O1','-UNDEBUG']+include+[str(root/s) for s in sources]
  if name=='payload':
   command.extend(['-include',str(root/'host/sprite_seams.h'),str(root/'host/sprite_seams.cpp'),str(guest)])
  if name=='body':command.append(str(bodyguest))
  binary=t/name;subprocess.run(command+['-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
