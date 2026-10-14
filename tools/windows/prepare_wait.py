#!/usr/bin/env python3
"""Extract actual guest poll and VI actors for the Windows regression, djultra64."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
s14=(root/'generated/guest/funcs_14.c').read_text();s5=(root/'generated/guest/funcs_5.c').read_text()
def function(text,name):
 begin=text.index('RECOMP_FUNC void '+name+'(');end=text.find('RECOMP_FUNC void ',begin+1)
 return text[begin:end if end>=0 else len(text)]
start=s5.index('L_8003DAEC:');end=s5.index('L_8003DAFC:',start)
loop=s5[start:end]+'return;\n'
if loop.count('yield_self_1ms(rdram);')!=1:raise SystemExit('Verified original framebuffer-wait yield missing')
head='#include "recomp.h"\nextern "C" {\n'
for name in ['func_80098A14','func_80098E98','func_800990E4','osGetThreadId_recomp','osRecvMesg_recomp','osSendMesg_recomp','osViGetCurrentFramebuffer_recomp','osViSwapBuffer_recomp','osGetTime_recomp']:
 head+='void '+name+'(uint8_t*,recomp_context*);\n'
head+='void yield_self_1ms(uint8_t*);\n}\n'
body=head+''.join(function(s14,n) for n in ['func_80098A14','func_80098E98','func_800990E4'])
body+='extern "C" void original_wait(uint8_t* rdram,recomp_context* ctx){\n'+loop.replace('yield_self_1ms(rdram);','')+'}\n'
body+='extern "C" void yielding_wait(uint8_t* rdram,recomp_context* ctx){\n'+loop+'}\n'
(root/'generated/windows_wait_guest.cpp').write_text(body)
