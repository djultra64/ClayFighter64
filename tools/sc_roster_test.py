#!/usr/bin/env python3
"""Run real original graphics resource decoding against the isolated roster module; djultra64."""
from pathlib import Path
import re,subprocess,json
R=Path(__file__).resolve().parents[1];O=R/'local/sc_roster';funcs={}
for p in (R/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
names=['func_800AFFE0','func_800B9590','func_800BD354','func_800C15F0','func_800BE590','func_800BE654','func_800BE6BC','func_800BE6C8','func_8005AB2C','func_8005ABA4','func_8005AC38','func_8005FC68','func_8005D484']
loader=funcs['func_800AFFE0'];loader=re.sub(r'\s*const gpr portrait_source=ctx->r4;', '',loader);loader=re.sub(r'[^\n]*clay_sc_roster_augment_portraits[^\n]*\n','',loader);loader=loader.replace('recomp_context* ctx) {','recomp_context* ctx) {\n const gpr portrait_source=ctx->r4;',1);loader=loader.replace('    return;','    clay_sc_roster_augment_portraits(rdram,ctx,portrait_source);\n    return;');funcs['func_800AFFE0']=loader

for hook in json.loads((R/'config/sc_roster_hooks.json').read_text())['verified_hooks']:
 if hook['function']!='func_8005FC68':continue
 address=int(hook['instruction_vram'],16)
 pattern=rf'(// 0x{address:08X}:[^\n]*\n)[^\n]*'
 funcs[hook['function']]=re.sub(pattern,lambda m:m[1]+'    '+hook['new_statement'],funcs[hook['function']])
fixtures={'func_800B59D0','func_80097E54','func_800B3C70','func_800B4040','func_800B4408','func_800B474C','func_800B51F4','func_800B6A00','func_800B6E74','func_8005D334','func_8005D3D4','func_8009B6B4','func_800A0CA8','func_800B9154','func_800B8FC8','func_8009E36C'}
pending=['func_800'+x for x in ['02050','01E90','01008','01AA8','02DE0','03AA8','72D50','72FE8','91A8C','9215C']]
while pending:
 n=pending.pop()
 if n in names or n in fixtures:continue
 names.append(n)
 pending.extend(re.findall(r'(func_\w+)\(rdram, ctx\)',funcs[n]))
s='#include "recomp.h"\n#include "funcs.h"\n#include "../../host/sc_roster.h"\n#include "../../host/sc_assets.h"\nextern "C" void sc_roster_unsupported(const char*);\nextern "C" {\n'+'\n'.join(funcs[n]for n in names)
refs=set(re.findall(r'(func_\w+)\(rdram, ctx\)',s))
for n in refs-set(names)-fixtures:s+=f'\nvoid {n}(uint8_t*,recomp_context*){{sc_roster_unsupported("{n}");}}\n'
s+='void osStartThread_recomp(uint8_t*,recomp_context*){}\n'
lookup=funcs['func_800636E4'].split('    // 0x800638D8:')[0].replace('func_800636E4(', 'sc_roster_original_lookup(',1)
s+='\n'+lookup+'    ctx->r29=ADD32(ctx->r29,0x38); return;\n;}\n'
for n in ['func_8005DDCC','func_800A45BC','func_800A3AE4','func_80098448']:s+=f'void {n}(uint8_t*,recomp_context*){{}}\n'
# Exercise the original selector tick/wrap block, without its unrelated mesh drawing.
update=re.search(r'    // 0x800625EC:.*?(?=    // 0x80062674:)',funcs.get('func_80062400','') or ''.join(p.read_text() for p in (R/'generated/guest').glob('funcs_*.c')),re.S).group(0)
s+='void sc_roster_animation_tick(uint8_t*rdram,recomp_context*ctx){\n'+update+'return;\n}\n'
# Full original one-slot state machine: idle, hover, leave and confirmation.
# Caller supplies the ORIGINAL 61474 local stack layout without a prologue:
# sp+54 slot; sp+50 P1 selected slot*2; sp+4C P2 selected slot*2;
# sp+5C coordinates+slot*4. sp+58 and guest registers are native scratch.
# Stop at6298C before the original loop increments slot/coordinate pointers.
selector_tick=re.search(r'    // 0x800625A0:.*?(?=    // 0x8006298C:)',funcs['func_80061474'],re.S)
assert selector_tick and 'L_8006298C:' in selector_tick.group(0)
s+='void sc_roster_selector_tick(uint8_t*rdram,recomp_context*ctx){\n'+selector_tick.group(0)+'return;\n}\n'
# Actual native selector lookup chooses idle OR selected-tail base, then phase.
lookup=re.search(r'    // 0x8005E730:.*?(?=    // 0x8005E844:)', ''.join(p.read_text() for p in (R/'generated/guest').glob('funcs_*.c')),re.S).group(0)
s+='void sc_roster_portrait_lookup(uint8_t*rdram,recomp_context*ctx){\n'+lookup+'ctx->r2=ADD32(ctx->r14,ctx->r10);return;\n}\n'
s+='}\n';(O/'guest.cpp').write_text(s)
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
subprocess.run(['g++','-std=c++20','-O1','-g','-I'+sdk,'-I'+str(R/'generated/guest'),str(R/'tests/sc_roster.cpp'),str(R/'host/sc_assets.cpp'),str(R/'host/sc_roster.cpp'),str(O/'guest.cpp'),'-o',str(O/'test')],check=True)
subprocess.run([str(O/'test'),str(O/'asset-image-roster.z64'),str(R/'probe-output/base.z64')],cwd=R,check=True,timeout=30)
