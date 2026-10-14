#!/usr/bin/env python3
"""Execute original sound relocation and bounded dispatch selection; djultra64.
Synth/queue playback is deliberately outside this isolated lookup fixture.
"""
from pathlib import Path
import argparse,re,subprocess,tempfile,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args();bodies={}
for path in (root/'generated/guest').glob('funcs_*.c'):
 source=path.read_text()
 for name in ['800C1A04','800C1910','8009B6B4']:
  m=re.search(r'RECOMP_FUNC void func_'+name+r'\([^\n]*\).*?;\}',source,re.S)
  if m:bodies[name]=m[0]
assert len(bodies)==3
sound=bodies.pop('8009B6B4');prefix=sound[sound.index('    // 0x8009B6B4:'):sound.index('L_8009B744:')];bounds=sound[sound.index('L_8009B7B0:'):sound.index('    // 0x8009B7FC:')];select=sound[sound.index('L_8009B854:'):sound.index('    // 0x8009B890:')]
# Exact original selector instructions, isolated from synth locks/voice allocation.
body='RECOMP_FUNC void clay_sound_selector(uint8_t* rdram,recomp_context* ctx){\n'+prefix+'L_8009B744:\n'+bounds+'ctx->r2=0;ctx->r29=ADD32(ctx->r29,0x38);return;\n'+select+'ctx->r2=MEM_W(0x1c,ctx->r29);ctx->r29=ADD32(ctx->r29,0x38);return;\n}\n'
with tempfile.TemporaryDirectory(prefix='clay-sound-dispatch-')as temp:
 work=Path(temp);guest=work/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C"{\n'+'\n'.join(bodies.values())+'\n'+body+'}\n');binary=work/'test';includes=[root/'generated/guest',a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include']
 subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG']+['-I'+str(i)for i in includes]+[str(root/'tests/sc_sound.cpp'),str(guest),'-o',str(binary)],check=True)
 result=subprocess.run([str(binary),str(root/'probe-output/base.z64')],check=True,text=True,capture_output=True);print(result.stdout,end='')
 (root/'analysis/sc_sound_dispatch_evidence.json').write_text(json.dumps({'author':'djultra64','result':'PASS','original_guest_functions':['800C1A04 alBnkfNew','800C1910 recursiveBankRelocation','8009B6B4 boundedSoundSelector'],'selector_instruction_ranges':['8009B6B4..8009B740','8009B7B0..8009B7F8','8009B854..8009B88C'],'valid_sound_ids':152,'donor_characters':[6,11,12,16],'mapping':'identity because ALL donor voices/combat/menu bank records andsamples matchoriginal63; see sc_sound_bank_evidence.json','negative_cases':['bankcount','instrumentcount','negativebank','negativeinstrument','soundcount','negativesound'],'scope':'Originalrelocation+exactoriginalboundedlookup; synth locks/voiceallocation/playback excludedfromfixture; noaudibleall-moveclaim','stdout':result.stdout},indent=2)+'\n')
