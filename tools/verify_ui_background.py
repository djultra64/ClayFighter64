#!/usr/bin/env python3
"""Private original native UI writer coverage proof; djultra64. No windows."""
from pathlib import Path
import argparse,re,tempfile,subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args()
names=['800396C4','800B23F0','800B1FF0','800B2154','800A5B44','800A45BC','800A4D9C','800A4BF0','800A5940','800A59B0'];bodies={}
for source in (root/'generated/guest').glob('funcs_*.c'):
 s=source.read_text()
 for name in names:
  m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',s,re.S)
  if m:
   body=m[0]
   body=re.sub(r'clay::ui::draw_(?:fullscreen_fill|transition_band)\(rdram,\s*ctx,\s*&?(func_[0-9A-Fa-f]+)\);',r'\1(rdram,ctx);',body)
   body=re.sub(r'^.*clay::ui::.*\n','',body,flags=re.M)
   bodies[name]=body
assert len(bodies)==len(names)
# Compile the authoritative renderer's actual FILL/COPY coordinate quantizer,
# rather than assuming an inclusive +1 pixel when testing native band joins.
rdp=(a.sdk_root/'deps/rt64/src/hle/rt64_rdp.cpp').read_text()
fillrect=rdp[rdp.index('void RDP::fillRect(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry,'):]
quantizer=re.search(r'if \(\(mode == G_CYC_COPY\) \|\| \(mode == G_CYC_FILL\)\) \{([^}]+)\}',fillrect)
assert quantizer and 'lry |= 3;' in quantizer[1]
with tempfile.TemporaryDirectory(prefix='clay-ui-')as temp:
 t=Path(temp);guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C"{\n'+'\n'.join(bodies.values())+'\nint rt64_fill_lower_edge(int lry){int lrx=0;'+quantizer[1]+'return lry;}\n}\n');binary=t/'test'
 subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/'tests/ui_background.cpp'),str(root/'host/ui_background.cpp'),str(root/'host/graphics_config.cpp'),str(guest),'-o',str(binary)],check=True)
 mesh=root/'local/sc_roster/mesh-rdram.bin'
 if not mesh.exists():raise SystemExit('Private original mesh codec fixture missing; run tools/sc_roster_mesh_test.py with local ROM/SDK first')
 subprocess.run([str(binary),str(root/'probe-output/base.z64')]+([str(mesh)]if mesh.exists()else[]),check=True)
