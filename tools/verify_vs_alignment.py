#!/usr/bin/env python3
"""Actual original VS/sprite geometry and donor foot alignment; djultra64. Hidden CPU only."""
from pathlib import Path
import re,subprocess,tempfile,argparse,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args()
source=root/'local/sc_portraits/guest.cpp'
if not source.exists():raise SystemExit('Private native loader fixture missing; run tools/sc_portrait_native.py first')
s=source.read_text();names=['8005DB24','800A6520','800A5868','800A5940','800A59B0','800B1F38','800B1ED0','800C1D40'];bodies={}
for f in (root/'generated/guest').glob('funcs_*.c'):
 text=f.read_text()
 for n in names:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',text,re.S)
  if m:bodies[n]=re.sub(r'^.*clay::(?:begin_sprite_slices|end_sprite_slices|limit_sprite_slice_rows)\(.*\);\n','',m[0],flags=re.M)
assert len(bodies)==len(names)
for n,body in bodies.items():
 name='func_'+n
 s=re.sub(r'(?:RECOMP_FUNC )?void '+name+r'\([^\n]*\)\s*\{[^\n]*\}\n','',s)
 if n=='800A6520':body=body.replace('{\n','{\n    vs_capture_sprite(rdram,ctx);\n',1)
 s=s[:-2]+body+'\n}\n'
s=s.replace('"../../host/sc_assets.h"','"'+str(root/'host/sc_assets.h')+'"').replace('#include "recomp.h"','#include "recomp.h"\nextern "C" void vs_capture_sprite(uint8_t*,recomp_context*);',1)
with tempfile.TemporaryDirectory(prefix='clay-vs-')as t:
 work=Path(t);guest=work/'guest.cpp';guest.write_text(s);binary=work/'vs'
 subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/'tests/vs_alignment.cpp'),str(root/'host/vs_alignment.cpp'),str(root/'host/sc_assets.cpp'),str(guest),'-o',str(binary)],check=True)
 runs=[]
 for i in [1,2,3,4,5,8,9,13,14,15,6,11,12,16]:
  image=root/('probe-output/base.z64'if i not in[6,11,12,16]else'analysis/sc_assets/asset-image.z64')
  run=subprocess.run([str(binary),str(image),str(i)],capture_output=True,text=True,check=True);print(run.stdout,end='');runs.append({'id':i,'stdout':run.stdout})
 (root/'analysis/vs_alignment_evidence.json').write_text(json.dumps({'author':'djultra64','result':'PASS','source':'Original native01008/72FE8/91A8C/9215C/02DE0/5DB24/A6520 and native sprite helper bodies','baseline':'Grounded original-cast median opaque-foot worldY−64; intentionalhoverIcky9 excluded','donor_offsets':{'6':-18,'11':-25,'12':-24,'16':-22},'scope':'VS6D208/6D27C only. Entire64tick nativeidleposes remain unchanged; actual nativegeometry testedP1/P2. No donorengine/UIcoordtableimport and noGPUvisualclaim.','runs':runs},indent=2)+'\n')
