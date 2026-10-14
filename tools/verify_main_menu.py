#!/usr/bin/env python3
"""Original main-menu navigation and guarded host exit regression, djultra64."""
from pathlib import Path
import argparse,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');p.add_argument('--prepare-only',type=Path);a=p.parse_args()
source=next(f.read_text() for f in (root/'generated/guest').glob('funcs_*.c') if 'RECOMP_FUNC void func_80067980(' in f.read_text())
body=re.search(r'RECOMP_FUNC void func_80067980\([^\n]*\).*?;\}',source,re.S)[0]
assert '    if(clay::main_menu::confirm_exit(rdram))goto L_800684F4;' in body,'Regenerate original main-menu exit hook before verification'
assert body.count('clay::main_menu::observe_input(rdram);')==1,'Main-menu release observation must run once after the native input aggregation; duplicate calls consume a held-button gate'
assert 'func_800A0CA8(rdram, ctx);\n    clay::main_menu::observe_input(rdram);' in body,'Observe main-menu input immediately after the original aggregation'
body=re.sub(r'^.*clay::port::update_native_options.*\n','',body,flags=re.M)
body=re.sub(r'^.*clay::main_menu::confirm_exit.*\n','',body,flags=re.M)
body=re.sub(r'^.*clay::main_menu::restore_selection.*\n','',body,flags=re.M)
assert body.count('L_800681D4:')==1
body=body.replace('L_800681D4:','L_800681D4:\n    if(!main_menu_baseline && clay::main_menu::confirm_exit(rdram))goto L_800684F4;')
body=body.replace('    return;','    clay::main_menu::restore_selection(rdram);\n    return;')
idle=re.search(r'L_8006B148:.*?(?=L_8006B1A0:)',source,re.S)[0]
idle=idle.replace('goto L_8006B1A0;','return;').replace('goto L_8006B330;','return;')
idle='void main_menu_idle_timeout(uint8_t*rdram,recomp_context*ctx){\n'+idle+'\n}\n'
fixture='#include \"recomp.h\"\n#include \"funcs.h\"\n#include \"main_menu.h\"\nextern bool main_menu_baseline;\nextern \"C\"{\n'+body+idle+'\n}\n'
if a.prepare_only:
 a.prepare_only.parent.mkdir(parents=True,exist_ok=True);a.prepare_only.write_text(fixture);raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='clay-main-menu-')as temp:
 t=Path(temp);g=t/'guest.cpp';g.write_text('#include "recomp.h"\n#include "funcs.h"\n#include "main_menu.h"\nextern bool main_menu_baseline;\nextern "C"{\n'+body+idle+'\n}\n');exe=t/'test'
 subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/'tests/main_menu.cpp'),str(root/'host/main_menu.cpp'),str(g),'-o',str(exe)],check=True)
 baseline=subprocess.run([str(exe),str(root/'probe-output/base.z64'),'baseline']);assert baseline.returncode==14,'Original menu must reproduce unavailable third row'
 subprocess.run([str(exe),str(root/'probe-output/base.z64')],check=True)
 subprocess.run([str(exe),str(root/'probe-output/base.z64'),'start'],check=True)
