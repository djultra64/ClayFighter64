#!/usr/bin/env python3
"""Private original spinning-logo resource probe. Research by djultra64."""
from pathlib import Path
import subprocess,struct,json,collections,hashlib
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'local/logo64'
def export():
 ram=(OUT/'rdram.bin').read_bytes();bank=int((OUT/'allocations.txt').read_text().splitlines()[-1].split()[1],16)&0x1fffffff
 parts=[struct.unpack_from('>I',ram,bank+12+i*8)[0] for i in range(4)]
 begin=bank+parts[1];end=bank+parts[2];vertices={};triangles=[];commands=[]
 for offset in range(0xe0,end-begin,8):
  word,arg=struct.unpack_from('>II',ram,begin+offset);op=word>>24;commands.append({'offset':offset,'word':f'{word:08x}','argument':f'{arg:08x}'})
  if op==0xb8:break
  if op==4:
   count=(word>>10)&63;first=(word>>17)&127;address=arg&0xffffff
   if count>32 or address+count*16>end or address<begin:raise ValueError('Unexpected vertex bounds')
   for i in range(count):vertices[first+i]=address+i*16
  elif op in (0xb1,0xbf):
   for encoded in ([word&0xffffff,arg] if op==0xb1 else [arg]):
    if encoded:triangles.append([vertices[(encoded>>s&255)//2] for s in (16,8,0)])
 xyz={v:struct.unpack_from('>3h',ram,v) for t in triangles for v in t};graph=collections.defaultdict(set)
 for t in triangles:
  for x in t:
   for y in t:graph[xyz[x]].add(xyz[y])
 groups=[]
 while graph:
  todo=[next(iter(graph))];component=set()
  while todo:
   x=todo.pop()
   if x in component:continue
   component.add(x);todo.extend(graph.pop(x,()))
  groups.append(component)
 names=['large_three','numerator_one','original_six','denominator_three','fraction_slash']
 groups_info=[]
 for name,component in zip(names,groups):
  points=sorted(component);index={p:i+1 for i,p in enumerate(points)};faces=[[xyz[v] for v in t] for t in triangles if xyz[t[0]] in component]
  obj='\n'.join('v %d %d %d'%p for p in points)+'\n'+'\n'.join('f '+' '.join(str(index[p]) for p in t) for t in faces)+'\n';(OUT/(name+'.obj')).write_text(obj)
  groups_info.append({'name':name,'vertices':len(points),'triangles':len(faces),'bounds':[[min(p[i] for p in points),max(p[i] for p in points)] for i in range(3)]})
 records=[{'offset':v-begin,'xyz':xyz[v],'normal_bytes':list(ram[v+12:v+15]),'alpha':ram[v+15],'uv':struct.unpack_from('>2h',ram,v+8)} for v in sorted(xyz)]
 (OUT/'vertex-records.json').write_text(json.dumps({'vertices':records,'triangles':[[v-begin for v in t] for t in triangles]},indent=2)+'\n')
 (OUT/'resource1.bin').write_bytes(ram[begin:end]);(OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 texture=bank+parts[3];header=struct.unpack_from('>H',ram,texture+2)[0];width,height=struct.unpack_from('>2H',ram,texture+6);stride=struct.unpack_from('>H',ram,texture+14)[0]
 from PIL import Image
 pixels=[]
 for y in range(height):
  for x in range(width):
   index=ram[texture+header+y*stride+x];color=struct.unpack_from('>H',ram,texture+16+index*2)[0];pixels.append(((color>>11&31)*255//31,(color>>6&31)*255//31,(color>>1&31)*255//31,255 if color&1 else 0))
 im=Image.new('RGBA',(width,height));im.putdata(pixels);im.save(OUT/'original-material.png')
 report={'author':'djultra64','rom_bank':'0xBB7AA0','bank_ram_fixture':hex(bank|0x80000000),'resource_index':1,'resource_ram_fixture':hex(begin|0x80000000),'resource_bytes':end-begin,'resource_sha256':hashlib.sha256(ram[begin:end]).hexdigest(),'mesh_display_list_offset':224,'display_list_bytes':len(commands)*8,'triangles':len(triangles),'vertex_records':len(xyz),'groups':groups_info,'textures':[{'resource_index':3,'width':width,'height':height,'stride':stride,'format':'CI8/RGBA5551','header_bytes':header,'native_bind_pc':'0x8005C4E4','native_bind_function':'0x8005B680'}],'material':'Texture binding outside resource DL in original5C234; preserve resource3 and original lighting/renderstate','scope':'Native original mesh decoder plus geometry export; no logo modification or GUI'}
 rom=(ROOT/'probe-output/base.z64').read_bytes()
 report.update({'packed_bank_bytes':152032,'packed_bank_sha256':hashlib.sha256(rom[0xbb7aa0:0xbb7aa0+152032]).hexdigest(),'replacement_call_pc':'0x8005D250','replacement_call_rom':'0x5DE50','expected_opcode':'0c01708d','resource_hash_note':'Decoded bytes include fixture-specific absolute vertex addresses; do not use whole decoded hash as live relocated resource identity.'})
 (OUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
 import matplotlib;matplotlib.use('Agg')
 import matplotlib.pyplot as plt
 from matplotlib.collections import PolyCollection
 fig,ax=plt.subplots(figsize=(10,5),facecolor='black');ax.set_facecolor('black')
 poly=[[xyz[v][:2] for v in t] for t in triangles];ax.add_collection(PolyCollection(poly,facecolors='#ffd830',edgecolors='#8e7013',linewidths=.3));ax.autoscale();ax.set_aspect('equal');ax.axis('off');fig.savefig(OUT/'native-front.png',facecolor='black',bbox_inches='tight');plt.close(fig)
 print(json.dumps(report,indent=2))
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 (OUT/'.gitignore').write_text('*\n')
 source='''#define main original_fixture_main
#include "../../tests/sc_roster.cpp"
#undef main
extern "C" void func_80004730(uint8_t*,recomp_context*);
int main(){
 rom=file("probe-output/base.z64");auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));
 for(uint32_t i=0x1000;i<0xF9980;i++)MEM_B(i-0xC00,gpr(S32(0x80000000)))=rom[i];
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));c.r4=0xBB7AA0;func_80004730(rdram,&c);
 uint32_t bank=c.r2;printf("bank %08x\\n",bank);
 FILE* f=fopen("local/logo64/rdram.bin","wb");for(unsigned i=0;i<0x800000;i++)fputc(rdram[i^3],f);fclose(f);
 f=fopen("local/logo64/allocations.txt","w");for(auto[p,n]:allocations)fprintf(f,"%08x %u\\n",p,n);fprintf(f,"bank %08x\\n",bank);fclose(f);
 for(auto[p,n]:allocations)for(unsigned i=0;i<32;i++)if(MEM_BU(n+i,gpr(S32(p)))!=0xA5)return 9;
 puts("Original native mesh decoder allocation guards passed");free(rdram);
}'''
 (OUT/'probe.cpp').write_text(source)
 sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
 subprocess.run(['g++','-std=c++20','-O1','-I'+sdk,'-I'+str(ROOT/'generated/guest'),str(OUT/'probe.cpp'),str(ROOT/'host/sc_assets.cpp'),str(ROOT/'host/sc_roster.cpp'),str(ROOT/'local/sc_roster/mesh-guest.cpp'),'-o',str(OUT/'probe')],check=True)
 subprocess.run([str(OUT/'probe')],cwd=ROOT,check=True,timeout=30)
 export()
if __name__=='__main__':main()
