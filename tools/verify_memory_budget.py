#!/usr/bin/env python3
"""Original native disjoint allocator validation, by djultra64. No ROM bodies tracked."""
from pathlib import Path
import re,tempfile,subprocess,argparse,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');p.add_argument('--snapshot',type=Path,default=None);a=p.parse_args()
if a.snapshot is not None and not a.snapshot.exists():raise SystemExit('Requested private heap snapshot missing.')
names=['800B3770','800B8038','800B8CD0','800C15F0','800B3C70','800B51F4','800B7DE0','800B7E88','800B8878','800B8A10','800B4408','800B34C4','800BC250','800C1550','800B32D0'];bodies={};high_ram_refs=[]
for f in (root/'generated/guest').glob('funcs_*.c'):
 s=f.read_text();high_ram_refs+=re.findall(r"// (0x[0-9A-F]+): lui\s+[^\n]*0x80[4-7][0-9A-F]",s)
 for n in names:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',s,re.S)
  if m:bodies[n]=m[0]
assert len(bodies)==len(names)
assert not high_ram_refs, "Audit new original cached high-RAM fixed address before expanding pool"
with tempfile.TemporaryDirectory(prefix='clay-memory-')as tmp:
 t=Path(tmp);guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join('void '+n+'(uint8_t*,recomp_context*);'for n in ['osGetThreadId_recomp','osRecvMesg_recomp','osSendMesg_recomp','osCreateMesgQueue_recomp'])+'\n'.join(bodies.values())+'\n}\n');binary=t/'test'
 subprocess.run([a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(root/'host'),'-I'+str(root/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(root/'tests/memory_budget.cpp'),str(root/'host/memory_budget.cpp'),str(guest),'-o',str(binary)],check=True)
 run=subprocess.run([str(binary)]+([str(a.snapshot)]if a.snapshot else []),check=True,capture_output=True,text=True);print(run.stdout,end='')
 (root/'analysis/memory_budget_evidence.json').write_text(json.dumps({'author':'djultra64','result':'PASS','stdout':run.stdout,'snapshot':str(a.snapshot)if a.snapshot else 'Independent original constructor/init/AllocPtr reproducer','native_routines':names,'reservation':'80400000..80600000','scope':'Actual original linked-chunk AddMemory/AllocPtr/FreePtr executed against original frozen selector heap. No renderer or native full-scene claim.','negative_cases':['original190128OOM','dirtyreservation','4MiBreport','badchunkbacklink'],'native_high_ram_lui_references':high_ram_refs},indent=2)+'\n')
