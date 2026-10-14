#!/usr/bin/env python3
"""Verify original CPU Options defaults against persisted schema. djultra64."""
from pathlib import Path
import argparse,re,subprocess,tempfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
parser.add_argument('--cxx',default='c++')
parser.add_argument('--prepare-only',type=Path)
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
source=next(path.read_text() for path in sorted((root/'generated/guest').glob('funcs_*.c')) if 'RECOMP_FUNC void func_8005A918(' in path.read_text())
body=re.search(r'RECOMP_FUNC void func_8005A918\([^\n]*\).*?;\}',source,re.S)[0]
assert body.count('clay::native_preferences::after_initialize(rdram,ctx);')==1
body=body.replace('    clay::native_preferences::after_initialize(rdram,ctx);\n','')
# Only allocation is stubbed; every original default store executes unchanged.
fixture='''#include "native_preferences.h"
#include "recomp.h"
#include <cassert>
#include <vector>
extern "C" void func_800B3C70(uint8_t*,recomp_context*c){assert(c->r5==0x38);c->r2=S32(0x80200000);}
extern "C" void func_8009C3D0(uint8_t*,recomp_context*){}
extern "C" void func_8009C548(uint8_t*,recomp_context*){}
'''+body+'''
int main(){std::vector<uint8_t> ram(0x900000);auto*rdram=ram.data();recomp_context c{};c.r29=S32(0x807ff000);c.f_odd=&c.f0.u32h;
func_8005A918(rdram,&c);assert(c.r29==S32(0x807ff000));assert(clay::native_preferences::capture(rdram)==clay::native_preferences::defaults());}
'''
if args.prepare_only:
 args.prepare_only.parent.mkdir(parents=True,exist_ok=True)
 args.prepare_only.write_text(fixture)
 raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='clay-native-options-') as directory:
 path=Path(directory);cpp=path/'fixture.cpp';cpp.write_text(fixture);binary=path/'fixture'
 subprocess.run([args.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(args.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(cpp),str(root/'host/native_preferences.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],cwd=root,check=True)
print('PASS original native CPU initializer agrees with all 28 persisted Options defaults')
