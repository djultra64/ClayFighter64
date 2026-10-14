#!/usr/bin/env python3
"""Run the original special-record dispatch against each actual donor CharDef; djultra64."""
from pathlib import Path
import argparse,re,subprocess,tempfile,json
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compare-sc',action='store_true',help='Compare independently recompiled actual Sculptors Cut helpers')
parser.add_argument('--sdk',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime'))
parser.add_argument('--n64recomp',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK/toolchain/n64recomp-build/N64Recomp'))
parser.add_argument('--report',type=Path,help='Optional evidence JSON; tests leave the workspace untouched by default')
args=parser.parse_args()
functions={}
for p in (root/'generated/guest').glob('funcs_*.c'):
 for name in ['80083A10','8008307C','800C1520']:
  m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',p.read_text(),re.S)
  if m:functions[name]=m[0]
if len(functions)!=3:raise SystemExit('Missing original special/AI dispatch or RNG')
with tempfile.TemporaryDirectory(prefix='clay-special-dispatch-') as d:
 d=Path(d);source=d/'guest.cpp';source.write_text('#include "recomp.h"\nextern "C" {\n'+'void func_800C1520(uint8_t*,recomp_context*);\n'+'\n'.join(functions.values())+'\n}\n')
 sdk=args.sdk/'N64Recomp/include'
 binary=d/'dispatch'
 command=['g++','-std=c++20','-O1','-I'+str(sdk),str(root/'tests/sc_combat_dispatch.cpp'),str(source)]
 if args.compare_sc:
  (d/'symbols.toml').write_text('''[[section]]
name="main"
rom=0x1000
vram=0x80000400
size=0xD0000
functions=[
 {name="sc_action_resolve",vram=0x80068550,size=0x28C},
 {name="sc_special_resolve",vram=0x800687DC,size=0x34},
 {name="sc_original_rng",vram=0x80097BC0,size=0x2C}
]
''')
  (d/'config.toml').write_text('[input]\nsymbols_file_path="symbols.toml"\nrom_file_path='+json.dumps(str(root/'probe-output/sc.z64'))+'\noutput_func_path="sc_guest"\nfunctions_per_output_file=30\nuses_mips3_float_mode=false\n')
  subprocess.run([str(args.n64recomp),str(d/'config.toml')],check=True)
  sc_source=d/'sc_guest/funcs_0.c';sc=sc_source.read_text()
  # Fix C++ goto scope only; leave the generated assignment at its original instruction.
  marker='gpr jr_addend_80068590 = ctx->r2;'
  if sc.count(marker)!=1:raise SystemExit('Unexpected SC jump-table declaration')
  sc=sc.replace(marker,'jr_addend_80068590 = ctx->r2;').replace('RECOMP_FUNC void sc_action_resolve(uint8_t* rdram, recomp_context* ctx) {','RECOMP_FUNC void sc_action_resolve(uint8_t* rdram, recomp_context* ctx) {\n    gpr jr_addend_80068590 = 0;')
  sc_source.write_text(sc)
  command+=['-DCLAY_COMPARE_SC',str(sc_source)]
 subprocess.run(command+['-o',str(binary)],check=True)
 results=[]
 for ident in [3,6,11,12,16]:
  path=root/'analysis/sc_assets'/f'{"base" if ident==3 else "sc"}_{ident}.decoded'
  p=subprocess.run([str(binary),str(path)],capture_output=True,text=True,timeout=5)
  results.append({'character_id':ident,'exit_code':p.returncode,'stdout':p.stdout,'stderr':p.stderr});print(ident,p.returncode,p.stdout)
 red=subprocess.run([str(binary),str(root/'analysis/sc_assets/sc_6.decoded'),str(root/'analysis/sc_assets/base_0.decoded')],capture_output=True,text=True,timeout=5)
 print('wrongFrostydefinitionRED',red.returncode,red.stderr)
 report={'red_wrong_frosty_definition':{'exit_code':red.returncode,'stderr':red.stderr},'author':'djultra64','scope':'Actual original83A10 special-record dispatch, original8307C AI/action-category resolution and originalC1520 RNG against actual donor definitions. No full AI decision state, move recognition, damage or gameplay claim.','results':results}
 report['compared_actual_sc_helpers']=args.compare_sc
 if args.compare_sc:report['sc_helpers']={'action':{'rom':'0x69150','vram':'0x80068550'},'special':{'rom':'0x693DC','vram':'0x800687DC'},'rng':{'rom':'0x987C0','vram':'0x80097BC0'}}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 if any(r['exit_code'] for r in results) or red.returncode not in (6,7):raise SystemExit(1)
