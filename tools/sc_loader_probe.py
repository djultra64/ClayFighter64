#!/usr/bin/env python3
"""Execute original63 loader using isolated DMA/allocation fixtures; djultra64."""
import pathlib,re,subprocess,json,os
ROOT=pathlib.Path(__file__).resolve().parents[1]
SDK='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
def main():
 funcs={}
 for p in (ROOT/'generated/guest').glob('funcs_*.c'):
  for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
 seed=['func_80001008','func_8003C650','func_8003C6B4','func_8003C70C','func_80003E94','func_80001E90','func_80003AA8','func_80003B20','func_8000401C','func_80004570','func_800043C0','func_800B9590','func_800BD354','func_800C1550','func_800C15F0','func_800C1A04','func_800C1910','func_80002050','func_80072D50','func_80072FE8','func_80091A80','func_80093C50','func_800B8DF4','func_800B99C0','func_80091A8C','func_8009215C','func_80096B40']
 actual={n:funcs[n] for n in seed}
 for wrapper in ['func_8003C650','func_8003C6B4','func_8003C70C']:
  actual[wrapper]=actual[wrapper].replace('    clay_sc_demo_combat_selection(rdram, ctx->r4);\n','')
 # Original combat ID/global transfer and both loader/parser calls; earlier stage setup is a fixture.
 combat=funcs['func_8003C740'];start=combat.index('    // 0x8003CAB8:');end=combat.index('    // 0x8003CB5C:')
 actual['sc_actual_combat_load']=('void sc_actual_combat_load(uint8_t* rdram,recomp_context* ctx) {\n'+combat[start:end]+'\n return;\n}\n')
 # Experimental donor-only script header adaptation; preserve original context ABI.
 seq=actual['func_80091A8C']
 for old,new in [('ctx->r10 = ADD32(ctx->r9, 0XE);','ctx->r10 = ADD32(ctx->r9, sc_header_bytes(ctx->r9));'),('ctx->r14 = ADD32(ctx->r13, 0XE);','ctx->r14 = ADD32(ctx->r13, sc_header_bytes(ctx->r13));')]:
  if old in seq:
   assert seq.count(old)==1;seq=seq.replace(old,new)
  else:
   native=new.replace("sc_header_bytes(ctx->r9)","clay_sc_sequence_header_bytes(rdram, ctx->r9)").replace("sc_header_bytes(ctx->r13)","clay_sc_sequence_header_bytes(rdram, ctx->r13)")
   assert seq.count(native)==1
 actual['func_80091A8C']=seq
 # Only the exact original CharDef minor-version branch accepts registered adapted5.21.
 parser=actual['func_80072FE8'];marker='// 0x800731B0: beq         $t9, $at, L_800731E0\n    if (ctx->r25 == ctx->r1) {'
 adapted=marker.replace('ctx->r25 == ctx->r1','ctx->r25 == ctx->r1 || clay_sc_definition_supported(rdram, ctx->r15)')
 if marker in parser:
  assert parser.count(marker)==1;parser=parser.replace(marker,adapted)
 else:assert parser.count(adapted)==1
 actual['func_80072FE8']=parser

 refs=set(re.findall(r'(func_[0-9A-F]+)\(rdram, ctx\)','\n'.join(actual.values())))
 fixture={'func_8003C740','func_80001BE0','func_80097E54','func_8009E36C','func_800B3C70','func_800B4040','func_800B4408','func_800B474C','func_800B51F4','func_800B6A00','func_800B6E74','func_800B9154','func_800B8FC8'}
 unsupported=refs-actual.keys()-fixture
 out=ROOT/'analysis/sc_loader';out.mkdir(parents=True,exist_ok=True)
 s='#include "recomp.h"\n#include "funcs.h"\n#include "../../host/sc_assets.h"\nextern "C" {\n'+'\n'.join(actual.values())
 for n in unsupported:s+=f'\nvoid {n}(uint8_t*,recomp_context*) {{ sc_unsupported("{n}"); }}\n'
 s+='\n}\n';s='extern "C" void sc_unsupported(const char*);\nextern "C" int sc_header_bytes(unsigned long);\n'+s
 (out/'guest_slice.cpp').write_text(s)
 steps=int(os.environ.get('SC_PROBE_STEPS','32'));assert 1<=steps<=4096
 binary=out/'loader';subprocess.run(['g++',f'-DSC_PROBE_STEPS={steps}','-std=c++20','-O1','-g','-fno-omit-frame-pointer','-I'+SDK,'-I'+str(ROOT/'generated/guest'),str(ROOT/'tests/sc_loader.cpp'),str(ROOT/'host/sc_assets.cpp'),str(out/'guest_slice.cpp'),'-o',str(binary)],check=True)
 # RED proves unadapted original version branch reaches its fatal diagnostic.
 baseline=s.replace('ctx->r25 == ctx->r1 || clay_sc_definition_supported(rdram, ctx->r15)','ctx->r25 == ctx->r1')
 (out/'guest_baseline.cpp').write_text(baseline)
 red_binary=out/'loader_baseline'
 subprocess.run(['g++','-std=c++20','-O1','-I'+SDK,'-I'+str(ROOT/'generated/guest'),str(ROOT/'tests/sc_loader.cpp'),str(ROOT/'host/sc_assets.cpp'),str(out/'guest_baseline.cpp'),'-o',str(red_binary)],check=True)
 red=subprocess.run([str(red_binary),str(ROOT/'analysis/sc_assets/asset-image.z64'),'6'],text=True,capture_output=True,timeout=30)
 assert red.returncode==86 and 'Fatal diagnostic reached for intended valid character bootstrap' in red.stderr
 results=[]
 for path,ids in [(ROOT/'probe-output/base.z64',[3]),(ROOT/'analysis/sc_assets/asset-image.z64',[6,11,12,16])]:
  for i in ids:
   run=subprocess.run([str(binary),str(path),str(i)],text=True,capture_output=True,timeout=30)
   results.append({'character_id':i,'image':'base' if i==3 else 'structural_graft','exit_code':run.returncode,'stdout':run.stdout,'stderr':run.stderr});print(i,run.returncode,run.stdout,run.stderr)
 (ROOT/'analysis/sc_loader_evidence.json').write_text(json.dumps({'author':'djultra64','scope':'Original63 loader, codec, relocation, palette, actor initializer, CharDef parser and original sequence interpreter; runtime SC format registry tested on all animation slots at the reported step count. All three original combat wrappers3C650/3C6B4/3C70C and shared3C740 hook tested; original3C740 CAB8-CB5B both real actor loads/parsers execute with earlier stage setup fixture. RED wrong-wrapper case loads originalFrosty; GREEN shared hook loads each actualdonor. Deterministic allocator and bounded ROM DMA fixtures. Fatal diagnostics throw; exact registered5.21 version branch tested alongside rejected versions/type/unregistered/interior pointer. Prior fixture allowed fatal version diagnostic to return, invalidating its runtime bootstrap claim. No actual renderer or full combat proof.','actual_guest_functions':seed,'fixture_functions':sorted(fixture),'unsupported_functions_fail_if_reached':sorted(unsupported),'steps_per_animation':steps,'original_version_branch_red':{'character_id':6,'exit_code':red.returncode,'stderr':red.stderr},'tests':results,'all_tests_passed':all(t['exit_code']==0 for t in results),'tested_adapter':'host/sc_assets.cpp with donor-only two-header-offset hooks, strict exact-definition version gate and P1 actual combat selection transfer'},indent=2)+'\n')
 if not all(t['exit_code']==0 for t in results):raise SystemExit('Native SC character compatibility check failed; see analysis/sc_loader_evidence.json')
if __name__=='__main__':main()
