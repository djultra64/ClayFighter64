#!/usr/bin/env python3
"""Private asset compatibility evidence, by djultra64; no playable integration claim."""
import pathlib,re,struct,json,hashlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
HEADERS='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
def main():
 out=ROOT/'analysis/sc_assets';out.mkdir(parents=True,exist_ok=True)
 text='\n'.join(p.read_text() for p in (ROOT/'generated/guest').glob('funcs_*.c'))
 m=re.search(r'RECOMP_FUNC void func_800BD354\([^\n]*\).*?;\}',text,re.S);assert m
 src=out/'base_codec16.cpp';src.write_text('#include "recomp.h"\nextern "C" {\n'+m[0]+'\n}\n')
 binary=out/'decode';subprocess.run(['g++','-std=c++20','-O2','-I'+HEADERS,str(ROOT/'tests/sc_decode.cpp'),str(src),'-o',str(binary)],check=True)
 report=[]
 for kind,table,expected,ids in [('base',0xD9580,'357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07',[0,3,4,10,13]),('sc',0xAB260,'9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f',[6,11,12,16])]:
  rom=(ROOT/('probe-output/'+kind+'.z64')).read_bytes();assert hashlib.sha256(rom).hexdigest()==expected
  for i in ids:
   start=struct.unpack_from('>I',rom,table+i*4)[0];end=struct.unpack_from('>I',rom,table+68+i*4)[0];block=rom[start:end];hdr=struct.unpack_from('>16I',block);offset,size=hdr[8:10];packet=block[offset:offset+size];assert len(packet)==size
   p=out/f'{kind}_{i}.packet';d=out/f'{kind}_{i}.decoded';p.write_bytes(packet)
   run=subprocess.run([str(binary),str(p),str(d)],text=True,capture_output=True,check=True);data=d.read_bytes()
   record={'rom':kind,'id':i,'source_block':[hex(start),hex(end)],'source_header':[hex(x) for x in hdr],'packet_offset':hex(offset),'packet_size':size,'decoded_size':len(data),'decoded_sha256':hashlib.sha256(data).hexdigest(),'decoded_header_hex':data[:64].hex(),'native_decoder_stdout':run.stdout.strip()};report.append(record);print(kind,i,len(data),data[:40].hex())
 (ROOT/'analysis/sc_decode_evidence.json').write_text(json.dumps({'author':'djultra64','scope':'Native original63 codec16 compatibility, not rendering or combat proof','records':report},indent=2)+'\n')
if __name__=='__main__':main()
