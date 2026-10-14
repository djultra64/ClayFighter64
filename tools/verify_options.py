#!/usr/bin/env python3
"""Hidden original-font native Options verification by djultra64."""
from pathlib import Path
import argparse,re,subprocess,tempfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdk-root",type=Path,default=Path("/tmp/Blast Corps source review b48mc3gh/independent SDK"))
parser.add_argument("--cxx",default="c++")
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
sdk=args.sdk_root/"deps/N64ModernRuntime/N64Recomp/include"
font=root/"analysis/font_private/font.bin"
if not font.exists():
 if not (root/"probe-output/base.z64").exists():raise SystemExit("Private user font/ROM required")
 subprocess.run(["python3",str(root/"tools/font_decode_probe.py")],cwd=root,check=True)
with tempfile.TemporaryDirectory(prefix='clay-options-') as t:
 t=Path(t);bodies=[]
 for name in ['80071738','8007191C','800C2D1C','80063EFC','8006464C','80064C20','80064180','800B2154','800B1FF0','800C1520','80064870','8006493C']:
  for p in (root/'generated/guest').glob('funcs_*.c'):
   m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',p.read_text(),re.S)
   if m:
    body=m[0]
    body=re.sub(r'^.*clay::input::prepare_native_prompt_(?:draw|measure).*\n','',body,flags=re.M)
    if name=="80063EFC":body=body.replace("    return;","    native_animation_seeded(rdram);\n    return;",1)
    if name=="8006493C":body=body.replace("{\n","{\n native_exit_draw_called();\n",1)
    if name=="80064180":body=body.replace("{\n","{\n native_letter_draw_called();\n",1)
    bodies.append(body);break
  else:raise RuntimeError('missing original function '+name)
 guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\nvoid native_letter_draw_called();\nvoid native_exit_draw_called();\nvoid native_animation_seeded(uint8_t*);\n'+'\n'.join(bodies)+'\n}\n')
 binary=t/'test'
 subprocess.run([args.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(sdk),'-I'+str(root/'host'),'-I'+str(root/'generated/guest'),str(root/'tests/options.cpp'),str(root/'host/port_options.cpp'),str(root/'host/graphics_config.cpp'),str(guest),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(root/'analysis/font_private/font.bin'),str(root/'probe-output/base.z64')],check=True)
