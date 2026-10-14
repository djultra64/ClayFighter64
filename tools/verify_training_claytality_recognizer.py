#!/usr/bin/env python3
"""Private native Claytality input recognition diagnostic; excludes execution eligibility. djultra64."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
from probe_training_states import ROOT,functions
from verify_training_moves import hybrid_definitions,records

FIXTURE=r'''
#include "recomp.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <string>
extern "C" {
void func_800791A8(uint8_t*,recomp_context*);
void osGetThreadId_recomp(uint8_t*,recomp_context*c){c->r2=1;}
void osRecvMesg_recomp(uint8_t*,recomp_context*c){c->r2=0;}
void osSendMesg_recomp(uint8_t*,recomp_context*c){c->r2=0;}
}
int main(int argc,char**argv){
 assert(argc==5);const std::string mode=argv[4];const bool wrong=mode.find("wrong-directions")!=std::string::npos,mirrored=mode.find("mirror")!=std::string::npos;std::ifstream f(argv[1],std::ios::binary);
 std::vector<uint8_t> definition((std::istreambuf_iterator<char>(f)),{}),ram(0x800000);
 auto*rdram=ram.data();constexpr uint32_t world=0x80100000,actor=0x80300000,other=0x80310000,table=0x80200000;
 for(unsigned n=0;n<definition.size();n++)MEM_BU(n,gpr(S32(table)))=definition[n];
 MEM_W(0x1e8,gpr(S32(actor)))=1;MEM_W(0x1ec,gpr(S32(actor)))=table;
 MEM_W(0x2a8,gpr(S32(actor)))=world;MEM_W(0x2ac,gpr(S32(actor)))=other;
 MEM_W(0x44,gpr(S32(actor)))=mirrored?0x42c80000:int32_t(0xc2c80000);MEM_W(0x44,gpr(S32(other)))=mirrored?int32_t(0xc2c80000):0x42c80000;
 constexpr uint16_t attacks[]{0x4000,2,8,0x8000,4,1};
 for(unsigned n=0;n<6;n++)MEM_H(0x24c+n*2,gpr(S32(actor)))=attacks[n];
 const auto half=[&](unsigned p){return unsigned(MEM_BU(p,gpr(S32(table))))*256+MEM_BU(p+1,gpr(S32(table)));};
 const auto raw=[&](unsigned normalized){
  constexpr unsigned masks[]{0x800,0x400,0x100,0x200,0x20,0x10,0x4000,2,8,0x8000,4,1};
  unsigned bits=normalized&0x7fff;assert((bits&~0xfffu)==0);if(normalized&0x8000)bits&=0u-bits;
  unsigned value=0;for(unsigned n=0;n<12;n++)if(bits&(1<<n))value|=masks[(wrong!=mirrored)&&n==2?3:(wrong!=mirrored)&&n==3?2:n];return value;
 };
 unsigned tick=0;const auto run=[&](unsigned buttons,unsigned ticks){
  for(unsigned n=0;n<ticks;n++){
   MEM_W(0x9c,gpr(S32(world)))=++tick;recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807ff000));
   c.r4=gpr(S32(actor));c.r5=buttons;func_800791A8(rdram,&c);
  }
 };
 const unsigned duration=std::stoul(argv[2]),variant=std::stoul(argv[3]),motion_count=half(0x28),offset=half(0x2a),pattern=half(0x2e)+variant*36;
 assert(variant<half(0x2c)&&pattern+36<=definition.size()&&half(pattern));
 const unsigned hold_mask=half(0x24);const int hold_ticks=int16_t(half(0x26));
 if(mode=="no-input"){run(0,100);}
 else {
 if(hold_mask&&hold_ticks>0){
  unsigned charge=unsigned(hold_ticks)+2;
  if(mode=="short-charge")charge=unsigned(hold_ticks)/2;
  if(mode=="no-charge")charge=0;
  if(charge){run(raw(hold_mask),charge);run(0,2);}
 }
 for(unsigned n=0;n<motion_count;n++){run(raw(half(offset+n*2)),duration);run(0,2);}
 run(raw(half(pattern)),duration);
 }
 const unsigned count=MEM_W(0xa24,gpr(S32(actor)));bool queued=false;
 for(unsigned n=0;n<64;n++)if(uint32_t(MEM_W(0xa28+n*20,gpr(S32(actor))))==table&&uint32_t(MEM_W(0xa2c+n*20,gpr(S32(actor))))==table+pattern&&MEM_W(0xa30+n*20,gpr(S32(actor)))>0)queued=true;
 printf("Native791A8 duration=%u ticks variant=%u mode=%s charge=%d queued=%s index=%u\n",duration,variant,mode.c_str(),hold_mask?hold_ticks:0,queued?"true":"false",count);
 return queued?0:14;
}
'''


def finishers(data):
 """All enabled native type3 record/variant pairs; no external move lists."""
 count,pos,_=records(data)
 for record in range(count):
  length=struct.unpack_from('>I',data,pos)[0]
  motions,offset,variants,pattern=struct.unpack_from('>HHHH',data,pos+0x28)
  if data[pos+6]==3:
   for variant in range(variants):
    trigger=struct.unpack_from('>H',data,pos+pattern+variant*36)[0]
    if trigger:
     tokens=list(struct.unpack_from('>'+str(motions)+'H',data,pos+offset))+[trigger]
     if any(token&0x7000 for token in tokens):raise ValueError('Unverified normalized command bits')
     hold_mask,hold_ticks=struct.unpack_from('>Hh',data,pos+0x24)
     yield {'record':record,'variant':variant,'data':data[pos:pos+length],
            'directional':any(token&12 for token in tokens),
            'hold_mask':hold_mask,'hold_ticks':hold_ticks}
  pos+=length


def main():
 p=argparse.ArgumentParser(description=__doc__+' Default: all16 integrated fighters.')
 selection=p.add_mutually_exclusive_group();selection.add_argument('--fighter',type=int);selection.add_argument('--all',action='store_true')
 p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));a=p.parse_args()
 native=functions(ROOT/'generated/guest');names=('800791A8','80084610','8008480C','80084ADC','80084C50')
 source='#include "recomp.h"\nextern "C" {\n'
 source+='\n'.join('void func_'+n+'(uint8_t*,recomp_context*);'for n in names)+'\n'
 source+='void osGetThreadId_recomp(uint8_t*,recomp_context*);void osRecvMesg_recomp(uint8_t*,recomp_context*);void osSendMesg_recomp(uint8_t*,recomp_context*);\n'
 source+='\n'.join(native['func_'+n]for n in names)+'\n}\n'
 with tempfile.TemporaryDirectory(prefix='clay-private-recognizer-')as d:
  d=Path(d);guest=d/'guest.cpp';guest.write_text(source);fixture=d/'fixture.cpp';fixture.write_text(FIXTURE);binary=d/'recognizer'
  subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),str(fixture),str(guest),'-o',str(binary)],check=True)
  tested=positive=negative=fighters=charged=0;synthetic_source=None
  def run(record,variant,duration,mode,expected,description):
   result=subprocess.run([str(binary),str(record),str(duration),str(variant),mode],capture_output=True,text=True)
   if result.returncode!=expected:
    raise RuntimeError(f'{description}: native recognition expected {expected}, got {result.returncode}; {result.stdout} {result.stderr}')
  for ident,path in hybrid_definitions(d,a.sdk_root):
   if a.fighter is not None and ident!=a.fighter:continue
   fighters+=1;commands=list(finishers(path.read_bytes()))
   print(f'Integrated fighter ID {ident}: {len(commands)} enabled Claytality record/variants',flush=True)
   for command in commands:
    record=d/'record.bin';record.write_bytes(command['data']);variant=command['variant']
    description=f'fighter {ident} record {command["record"]} variant {variant}'
    run(record,variant,6,'no-input',14,description+' negative no input');negative+=1
    for duration in (4,6,8):
     for mode in ('normal','mirror'):
      run(record,variant,duration,mode,0,description+f' {duration} ticks {mode}');positive+=1
    if command['directional']:
     for mode in ('wrong-directions','wrong-directions-mirror'):
      run(record,variant,6,mode,14,description+' reversed physical direction');negative+=1
    if command['hold_mask'] and command['hold_ticks']>0:
     charged+=1
     for mode in ('no-charge','short-charge'):
      run(record,variant,6,mode,14,description+' incomplete native charge');negative+=1
    elif synthetic_source is None:synthetic_source=command
    tested+=1
  if not fighters:raise ValueError('Unknown integrated fighter')
  if a.fighter is None and fighters!=16:raise RuntimeError('Integrated roster verification incomplete')
  if synthetic_source is not None:
   # Authored diagnostic prerequisite on a PRIVATE copied record, not an added
   # game move: exercises the original16-bit hold-mask/tick recognizer contract.
   synthetic=bytearray(synthetic_source['data']);struct.pack_into('>Hh',synthetic,0x24,4,300)
   record=d/'synthetic-charge.bin';record.write_bytes(synthetic);variant=synthetic_source['variant']
   for mode,expected in (('normal',0),('mirror',0),('no-charge',14),('short-charge',14)):
    run(record,variant,6,mode,expected,'synthetic300-tick native charge prerequisite')
   print('PASS synthetic16-bit native charge prerequisite:300ticks both orientations, missing/short charge rejected',flush=True)
  print(f'PASS {fighters} integrated fighters, {tested} enabled Claytality record/variants, '
        f'{positive} positive matches, {negative} negative controls, {charged} authentic charged variants. '
        'Input recognition only; native execution eligibility, distance, finisher animation and success events excluded.')


if __name__=='__main__':main()
