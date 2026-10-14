#!/usr/bin/env python3
"""Compare all26 private stage resources with the actual original mesh loader/codecs; djultra64."""
from pathlib import Path
import re,subprocess,json,struct,hashlib
R=Path(__file__).resolve().parents[1];O=R/'local/sc_stages/meshes';O.mkdir(parents=True,exist_ok=True);sdk=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include')
functions={}
for file in(R/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void (func_800[0-9A-F]+)\([^\n]*\).*?;\}',file.read_text(),re.S):functions[m[1]]=m[0]
fixture={'func_800B3C70','func_800B4408','func_800B4040','func_800B474C','func_800B59D0','func_800B6A00','func_80097E54','func_800B9154','func_800B8FC8'};needed=set();todo=['func_80004730']
while todo:
 n=todo.pop()
 if n in needed or n in fixture:continue
 needed.add(n);assert n in functions,n;todo.extend(set(re.findall(r'(func_800[0-9A-F]+)\(rdram,\s*ctx\)',functions[n]))-needed-fixture)
(O/'guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(functions[n]for n in sorted(needed))+'\n}\n')
source=r'''#include "recomp.h"
#include <vector>
#include <map>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
static std::vector<uint8_t>rom;static unsigned slot=0;static std::map<uint32_t,uint32_t>handles,alloc;
static uint32_t allocate(uint8_t*rdram,uint32_t n){if(n>0x3FF00||slot>=23)throw std::runtime_error("guarded fixed allocation slots");uint32_t p=0x80200000+(slot++)*0x40000;alloc[p]=n;for(unsigned i=0;i<64;i++)MEM_B(n+i,gpr(S32(p)))=0xA5;return p;}
extern "C" {
void func_80004730(uint8_t*,recomp_context*);
void func_800B3C70(uint8_t*rdram,recomp_context*c){c->r2=gpr(S32(allocate(rdram,c->r5)));}
void func_800B4040(uint8_t*rdram,recomp_context*c){uint32_t p=allocate(rdram,c->r5),h=allocate(rdram,4);handles[h]=p;c->r2=gpr(S32(h));}
void func_800B4408(uint8_t*,recomp_context*c){if(c->r5&&!alloc.count(c->r5))throw std::runtime_error("invalid free");}
void func_800B474C(uint8_t*,recomp_context*c){if(!handles.count(c->r5))throw std::runtime_error("invalid handle");}
void func_800B6A00(uint8_t*,recomp_context*c){c->r2=gpr(S32(handles.at(c->r5)));}
void func_800B59D0(uint8_t*,recomp_context*c){c->r2=60;}
void func_80097E54(uint8_t*rdram,recomp_context*c){uint32_t d=c->r4,s=c->r5,n=c->r6;if(uint64_t(s)+n>rom.size()||(d&0x7fffff)+n>0x800000)throw std::runtime_error("DMA bounds");for(unsigned i=0;i<n;i++)MEM_B(i,gpr(S32(d)))=rom[s+i];}
void func_800B9154(uint8_t*,recomp_context*){throw std::runtime_error("native fatal");}
void func_800B8FC8(uint8_t*,recomp_context*){throw std::runtime_error("native codec fatal");}
void do_break(uint32_t){throw std::runtime_error("MIPS break");}
void switch_error(const char*,uint32_t,uint32_t){throw std::runtime_error("MIPS switch");}
}
int main(int argc,char**argv){if(argc!=4)return 1;std::ifstream f(argv[1],std::ios::binary);rom={std::istreambuf_iterator<char>(f),{}};auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));
 // Original63 globals/codec tables are retained even when DMA source is SC.
 std::ifstream base("probe-output/base.z64",std::ios::binary);std::vector<uint8_t>b{std::istreambuf_iterator<char>(base),{}};for(unsigned i=0x1000;i<0xF9980;i++)MEM_B(i-0xc00,gpr(S32(0x80000000)))=b[i];
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807ff000));c.r4=strtoul(argv[2],nullptr,0);func_80004730(rdram,&c);uint32_t bank=c.r2;unsigned n=alloc.at(bank);FILE*out=fopen(argv[3],"wb");for(unsigned i=0;i<n;i++)fputc(MEM_BU(i,gpr(S32(bank))),out);fclose(out);
 for(auto[p,size]:alloc)for(unsigned i=0;i<64;i++)if(MEM_BU(size+i,gpr(S32(p)))!=0xA5)throw std::runtime_error("native decode allocation overrun");printf("%08x %u %u\n",bank,n,slot);free(rdram);
}'''
(O/'probe.cpp').write_text(source);subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(sdk),'-I'+str(R/'generated/guest'),str(O/'probe.cpp'),str(O/'guest.cpp'),'-o',str(O/'probe')],check=True)
feas=json.loads((R/'analysis/sc_stage_feasibility.json').read_text());results={}
for edition in ['base','sc']:
 results[edition]=[]
 for stage in feas['images'][edition]['stages']:
  id=stage['id'];path=O/f'{edition}-{id}.bin';run=subprocess.run([str(O/'probe'),str(R/f'probe-output/{edition}.z64'),stage['mesh_range'][0],str(path)],cwd=R,check=True,capture_output=True,text=True);address,size,slots=run.stdout.split();data=path.read_bytes();assert len(data)==int(size);results[edition].append({'stage':stage,'data':data,'address':int(address,16),'allocation_slots':int(slots)})
def geometry(data,address,offset):
 cache={};vertices=set();triangles=[];textures=set();commands=[]
 def resolve(p):
  p=p&0x7fffffff;base=address&0x7fffffff;assert base<=p<base+len(data);return p-base
 for pos in range(offset+struct.unpack_from('>H',data,offset+0x24)[0],len(data)-7,8):
  word,arg=struct.unpack_from('>II',data,pos);op=word>>24;commands.append(word)
  if op==0xB8:break
  if op==4:
   count=(word>>10)&63;first=(word>>17)&127;start=resolve(arg);assert count<=32 and start+count*16<=len(data)
   for j in range(count):cache[first+j]=data[start+j*16:start+(j+1)*16];vertices.add(cache[first+j])
  elif op in [0xBF,0xB1]:
   for indices in ([word&0xffffff,arg]if op==0xB1 else[arg]):
    if indices:triangles.append([cache[(indices>>shift&255)//2].hex()for shift in [16,8,0]])
  elif op==0xFD:textures.add(resolve(arg))
  elif op==6:raise AssertionError('Unexpected nested stage DL; extend original command proof before classification')
 else:raise AssertionError('No native DL end')
 return {'vertices':sorted(v.hex()for v in vertices),'triangles':triangles,'texture_addresses':sorted(textures),'command_words':commands}
def compare_geometry(x,y,address,parts):
 rows=[]
 for p in parts:
  if x[p['base_offset']:p['base_offset']+4]!=b'\0\3\0\0':continue
  a=geometry(x,address,p['base_offset']);b=geometry(y,address,p['sc_offset']);rows.append({'resource':p['id'],'base_unique_vertices':len(a['vertices']),'sc_unique_vertices':len(b['vertices']),'base_triangles':len(a['triangles']),'sc_triangles':len(b['triangles']),'vertex_attributes_equal':a['vertices']==b['vertices'],'triangle_geometry_attributes_equal':a['triangles']==b['triangles'],'gbi_command_words_equal':a['command_words']==b['command_words']})
 return rows
sha=lambda b:hashlib.sha256(b).hexdigest();comp=[]
for id in range(26):
 a,b=results['base'][id],results['sc'][id];x,y=a['data'],b['data'];assert a['address']==b['address'],'fixed fixture returned addresses differ'
 # Native decoded bank has12byte header plus part offset/reserved records.
 # Loaded sizeword is0: derive spans from the next resource offset, not this reserved word.
 count=struct.unpack_from('>I',x,4)[0];count_y=struct.unpack_from('>I',y,4)[0];assert count==count_y and 0<count<1024
 parts=[]
 for i in range(count):
  ox=struct.unpack_from('>I',x,12+i*8)[0];oy=struct.unpack_from('>I',y,12+i*8)[0];nx=(struct.unpack_from('>I',x,12+(i+1)*8)[0]if i+1<count else len(x))-ox;ny=(struct.unpack_from('>I',y,12+(i+1)*8)[0]if i+1<count else len(y))-oy;assert ox+nx<=len(x)and oy+ny<=len(y)
  xx,yy=x[ox:ox+nx],y[oy:oy+ny];kind='sprite_texture' if xx[:1]in [b'\x10',b'\x11',b'\x30',b'\x31',b'\x43'] else 'mesh_or_other'
  parts.append({'id':i,'base_offset':ox,'sc_offset':oy,'base_bytes':nx,'sc_bytes':ny,'kind':kind,'equal':xx==yy,'base_sha256':sha(xx),'sc_sha256':sha(yy),'changed_bytes':sum(u!=v for u,v in zip(xx,yy))+abs(len(xx)-len(yy))})
 comp.append({'id':id,'base_name':a['stage']['name'],'sc_name':b['stage']['name'],'decoded_equal':x==y,'base_bytes':len(x),'sc_bytes':len(y),'base_sha256':sha(x),'sc_sha256':sha(y),'same_fixed_relocation_address':hex(a['address']),'changed_bytes':sum(u!=v for u,v in zip(x,y))+abs(len(x)-len(y)),'resource_count':count,'resources':parts,'geometry':compare_geometry(x,y,a['address'],parts)if id==5 else[]})
report={'author':'djultra64','scope':'Read-only original63 loader/codecs with ROM DMA + fixed bounded allocator fixture; no game/runtime/image changes. Decoded native relocated output comparison at equal addresses. Resourcekind heuristic needs separate descriptor proof before texture/geometry classification.','original_functions':[n[5:]for n in sorted(needed)],'fixture_functions':sorted(fixture),'allocation_guards':'64-byte guard after every allocation verified, fixed256KiB slots ensure relocationaddress equivalence across editions','comparisons':comp};(R/'analysis/sc_stage_mesh_probe.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps([{k:v for k,v in c.items()if k not in ['resources','base_sha256','sc_sha256']}for c in comp],indent=2))
