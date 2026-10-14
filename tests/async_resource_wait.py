#!/usr/bin/env python3
"""Compile original guest poll, original VI actor and actual native SDK scheduler.
No guest status is written by a host completion shortcut. djultra64.
"""
import argparse
import tempfile
import subprocess
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,help='Optional evidence output; CTest leaves the repository untouched')
parser.add_argument('--sdk',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime'))
args=parser.parse_args()
SDK=args.sdk
temporary=tempfile.TemporaryDirectory(prefix='clay-async-resource-')
TMP=Path(temporary.name)
s14=(ROOT/'generated/guest/funcs_14.c').read_text();s5=(ROOT/'generated/guest/funcs_5.c').read_text()
def function(text,name):
 begin=text.index('RECOMP_FUNC void '+name+'(');end=text.find('RECOMP_FUNC void ',begin+1)
 return text[begin:end if end>=0 else len(text)]
start=s5.index('L_8003DAEC:');end=s5.index('L_8003DAFC:',start)
loop=s5[start:end]+'return;\n'
yield_site='yield_self_1ms(rdram);'
if loop.count(yield_site)!=1:
 raise SystemExit('Actual generated wait must contain exactly one verified yield_self_1ms at 0x8003DAEC')
baseline=loop.replace(yield_site,'',1)
head='#include "recomp.h"\nextern "C" {\n'
for f in ['func_80098A14','func_80098E98','func_800990E4','osGetThreadId_recomp','osRecvMesg_recomp','osSendMesg_recomp','osViGetCurrentFramebuffer_recomp','osViSwapBuffer_recomp','osGetTime_recomp']:
 head+='void '+f+'(uint8_t*,recomp_context*);\n'
head+='void yield_self_1ms(uint8_t*);\n}\n'
body=head+'\n'+''.join(function(s14,n) for n in ['func_80098A14','func_80098E98','func_800990E4'])
body+='extern "C" void original_wait(uint8_t* rdram,recomp_context* ctx){\n'+baseline+'}\n'
body+='extern "C" void yielding_wait(uint8_t* rdram,recomp_context* ctx){\n'+loop+'}\n'
(TMP/'guest.cpp').write_text(body)
includes=['host',SDK/'N64Recomp/include',SDK/'ultramodern/include',SDK/'thirdparty',SDK/'thirdparty/concurrentqueue']
source=[ROOT/'tests/async_resource_wait.cpp',TMP/'guest.cpp',ROOT/'generated/threadqueue_trace.cpp']+[SDK/('ultramodern/src/'+n+'.cpp') for n in ['threads','scheduling','mesgqueue']]
cmd=['g++','-std=c++20','-O1','-DNDEBUG','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-pthread']+['-I'+str(ROOT/i if isinstance(i,str) else i) for i in includes]+[str(s) for s in source]+['-o',str(TMP/'wait')]
p=subprocess.run(cmd,capture_output=True,text=True);print('compile',p.returncode,p.stderr[-3000:])
if p.returncode:raise SystemExit(p.returncode)
results={}
for mode in ['original','yield']:
 try:
  p=subprocess.run([str(TMP/'wait'),mode],capture_output=True,text=True,timeout=2)
  results[mode]={'exit_code':p.returncode,'stdout':p.stdout,'stderr':p.stderr};print(mode,p.returncode,p.stderr)
 except subprocess.TimeoutExpired:
  results[mode]={'timeout_seconds':2};print(mode,'TIMEOUT')
if args.report:
 args.report.write_text(json.dumps(results,indent=2)+'\n')
temporary.cleanup()
if 'timeout_seconds' not in results['original'] or results['yield'].get('exit_code')!=0:raise SystemExit(1)
