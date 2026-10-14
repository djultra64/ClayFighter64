#!/usr/bin/env python3
"""Private original ROM biography pipeline fixture; djultra64."""
from pathlib import Path
import re,subprocess,tempfile,argparse
R=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));a=p.parse_args()
names=['8006CC38','8006C5DC','8006C684','8006C6C0','80071738','8007191C','800C2D1C'];found={}
for file in(R/'generated/guest').glob('funcs_*.c'):
 s=file.read_text()
 for n in names:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',s,re.S)
  if m:found[n]=m[0]
assert len(found)==len(names)
# Strip only explicitly known host hooks; retain original guest instructions unchanged.
for n in names:
 for hook in ['clay::bios::install(rdram, ctx);','clay::input::prepare_native_prompt_measure(rdram, ctx);','clay::input::prepare_native_prompt_draw(rdram, ctx);']:
  found[n]=re.sub(re.escape(hook).replace(re.escape(', '), r',\s*'),' ',found[n])
with tempfile.TemporaryDirectory(prefix='clay-native-bios-') as tmp:
 t=Path(tmp);g=t/'guest.cpp';g.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(found.values())+'\n}\n');exe=t/'test'
 subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(R/'host'),'-I'+str(R/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(R/'tests/sc_bios.cpp'),str(R/'host/sc_bios.cpp'),str(g),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(R/'probe-output/base.z64'),str(R/'analysis/font_private/font.bin'),str(R/'local/sc_bios/bios.bin')],check=True)
