#!/usr/bin/env python3
"""Private native Claytality recognition, eligibility and dispatch diagnostic. djultra64.

Animation loading/update/playback are observed stubs, not full animation execution.
Original generated routines and decoded data exist only in a temporary directory.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
from probe_training_states import ROOT, functions
from verify_training_moves import hybrid_definitions
from verify_training_claytality_recognizer import FIXTURE, finishers

BOUNDARY = r'''
unsigned animation_calls=0, animation_id=0;
extern "C" {
void func_80077648(uint8_t*,recomp_context*);
void func_80080880(uint8_t*,recomp_context*c){++animation_calls;animation_id=unsigned(c->r5);}
void func_80081918(uint8_t*,recomp_context*){}
void func_8009200C(uint8_t*,recomp_context*){}
}
'''
ELIGIBILITY = r'''
 assert(queued);
 const std::string condition=argv[6];
 const auto floating=[&](uint32_t base,unsigned off,float value){uint32_t bits;std::memcpy(&bits,&value,4);MEM_W(off,gpr(S32(base)))=bits;};
 const int minimum=int16_t(half(0x1a)),maximum=int16_t(half(0x1c)),cooldown=int16_t(half(0x1e));
 int gap=minimum<0?20:minimum+10;if(maximum>=0)gap=minimum<0?maximum/2:(minimum+maximum)/2;
 if(condition=="too-close")gap=minimum-1;
 if(condition=="too-far")gap=maximum+1;
 MEM_W(0x2cb8,gpr(S32(world)))=gap;
 MEM_W(0x50,gpr(S32(world)))=3;
 MEM_W(0xfb0,gpr(S32(other)))=condition=="not-kneeling"?0:0x28;
 MEM_W(0x2b8,gpr(S32(actor)))=condition=="locked"?200:0;
 MEM_W(0x2c8,gpr(S32(actor)))=condition=="full-resource"?300:0;
 floating(actor,0x1a0,condition=="cooldown"?float(cooldown)-1.f:10000.f);
 floating(actor,0x474,condition=="busy"?1.f:0.f);
 floating(actor,0x44,mirrored?float(gap)/2.f:-float(gap)/2.f);
 floating(other,0x44,mirrored?-float(gap)/2.f:float(gap)/2.f);
 const unsigned posture=MEM_BU(0x13,gpr(S32(table)));
 MEM_W(0x170,gpr(S32(actor)))=condition=="wrong-posture"?3:posture;
 if(posture==2)floating(actor,0x48,-20.f);
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807ff000));c.r4=gpr(S32(actor));c.r5=posture==1?0x400:0;c.r6=0;
 func_80077648(rdram,&c);
 const bool dispatched=animation_calls==1&&animation_id==unsigned(int16_t(half(pattern+2)))&&
  uint32_t(MEM_W(0xf2c,gpr(S32(actor))))==table&&uint32_t(MEM_W(0xf30,gpr(S32(actor))))==table+pattern&&
  MEM_W(0x50,gpr(S32(world)))==8&&uint32_t(MEM_W(0x3c,gpr(S32(world))))==actor&&uint32_t(MEM_W(0x40,gpr(S32(world))))==other;
 printf("dispatch=%s animation_calls=%u animation=%u phase=%d gap=%d condition=%s\n",dispatched?"true":"false",animation_calls,animation_id,MEM_W(0x50,gpr(S32(world))),gap,condition.c_str());
 if(condition!="eligible"&&condition!="full-resource")assert(animation_calls==0&&MEM_W(0x50,gpr(S32(world)))==3);
 return dispatched?0:14;
'''


def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--fighter',type=int)
 p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
 a=p.parse_args();native=functions(ROOT/'generated/guest')
 names=('800791A8','80084610','8008480C','80084ADC','80084C50','80077648','800767CC','80083A58')
 source='#include "recomp.h"\nextern "C" {\n'
 source+='\n'.join('void func_'+n+'(uint8_t*,recomp_context*);'for n in names+('80080880','80081918','8009200C'))+'\n'
 source+='void osGetThreadId_recomp(uint8_t*,recomp_context*);void osRecvMesg_recomp(uint8_t*,recomp_context*);void osSendMesg_recomp(uint8_t*,recomp_context*);\n'
 source+='\n'.join(native['func_'+n]for n in names)+'\n}\n'
 fixture=FIXTURE.replace('#include <string>','#include <string>\n#include <cstring>\n'+BOUNDARY).replace('assert(argc==5)','assert(argc==7)')
 # Load authentic executable globals, including native state-action eligibility tables.
 fixture=fixture.replace('for(unsigned n=0;n<definition.size();n++)', 'std::ifstream romfile(argv[5],std::ios::binary);std::vector<uint8_t> rom((std::istreambuf_iterator<char>(romfile)),{});assert(rom.size()>0xf9980);for(unsigned n=0x1000;n<0xf9980;n++)MEM_BU(n-0x1000,gpr(S32(0x80000400)))=rom[n];\n for(unsigned n=0;n<definition.size();n++)')
 fixture=fixture.replace(' return queued?0:14;',ELIGIBILITY)
 with tempfile.TemporaryDirectory(prefix='clay-private-dispatch-')as directory:
  d=Path(directory);(d/'guest.cpp').write_text(source);(d/'fixture.cpp').write_text(fixture);binary=d/'dispatch'
  subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(d/'fixture.cpp'),str(d/'guest.cpp'),'-o',str(binary)],check=True)
  tested=positive=negative=0
  for ident,path in hybrid_definitions(d,a.sdk_root):
   if a.fighter is not None and ident!=a.fighter:continue
   commands=list(finishers(path.read_bytes()));print(f'Integrated fighter {ident}: {len(commands)} enabled Claytality variants',flush=True)
   for command in commands:
    record=d/'record.bin';record.write_bytes(command['data']);data=command['data']
    conditions=[('eligible',0),('full-resource',0),('not-kneeling',14),('locked',14),('busy',14),('wrong-posture',14)]
    minimum,maximum,cooldown=struct.unpack_from('>hhh',data,0x1a)
    if minimum>0:conditions.append(('too-close',14))
    if maximum>=0:conditions.append(('too-far',14))
    if cooldown>0:conditions.append(('cooldown',14))
    for mode in ('normal','mirror'):
     for condition,expected in conditions:
      result=subprocess.run([str(binary),str(record),'6',str(command['variant']),mode,str(ROOT/'probe-output/base.z64'),condition],capture_output=True,text=True)
      if result.returncode!=expected:raise RuntimeError(f'fighter {ident} record {command["record"]} variant {command["variant"]} {mode} {condition}: expected {expected}, got {result.returncode}\n{result.stdout}\n{result.stderr}')
      if expected==0:positive+=1
      else:negative+=1
    tested+=1
  if not tested:raise ValueError('No enabled variants selected')
  print(f'PASS {tested} enabled Claytality record/variants: {positive} native eligibility/dispatch positives and {negative} rejection controls, both orientations. Real recognizer, eligibility and phase8 dispatch; animation/resource loading and playback stubbed, full animation/success excluded.')


if __name__=='__main__':main()
