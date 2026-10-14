#!/usr/bin/env python3
"""Actual original selector matrix/sprite geometry regression, djultra64."""
from pathlib import Path
import re,tempfile,subprocess,argparse
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args()
names=['800B09DC','800B0270','800B03BC','800B1048','800B02F8','800A6520','800A5868','800A5940','800A59B0','800B1F38','800B1ED0','800C1D40','800C15F0'];bodies={}
for f in (root/'generated/guest').glob('funcs_*.c'):
 s=f.read_text()
 for n in names:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',s,re.S)
  if m:bodies[n]=re.sub(r'^.*clay::(?:begin_sprite_slices|end_sprite_slices|limit_sprite_slice_rows)\(.*\);\n','',m[0],flags=re.M)
selector_source=next(f.read_text() for f in (root/'generated/guest').glob('funcs_*.c') if 'RECOMP_FUNC void func_8005E37C(' in f.read_text())
slot=re.search(r'    // 0x8005E524:.*?(?=    // 0x8005E5D0:)',selector_source,re.S)[0]
# This includes BOTH actual branch arms: native0/1 selects the direction of
# sliding, while slot>>1 chooses P1/P2. Never mistake branch arm for player.
assert slot.count('unsigned(MEM_W(0x40,ctx->r29))')==2,'Regenerate selector hooks: both panel branch arms must derive player from actual index'
bodies['selector_panel_slot']='void selector_panel_slot(uint8_t*rdram,recomp_context*ctx){uint64_t hi=0,lo=0,result=0;int c1cs=0;\n'+slot+'return;\n}'
assert len(bodies)==len(names)+1
with tempfile.TemporaryDirectory(prefix='clay-selector-layout-')as temp:
 t=Path(temp);guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\n#include "selector_layout.h"\nextern "C"{\n'+'\n'.join(bodies.values())+'\n}\n');exe=t/'test'
 subprocess.run([a.cxx,'-std=c++20','-O1','-g','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/'tests/selector_layout.cpp'),str(root/'host/selector_layout.cpp'),str(root/'host/graphics_config.cpp'),str(guest),'-o',str(exe)],check=True)
 baseline=subprocess.run([str(exe),str(root/'probe-output/base.z64'),'baseline'])
 assert baseline.returncode==1,'Original unmodified bitmap anchors must reproduce miscentering'
 subprocess.run([str(exe),str(root/'probe-output/base.z64')],check=True)
