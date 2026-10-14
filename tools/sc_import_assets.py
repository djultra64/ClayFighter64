#!/usr/bin/env python3
"""Build a private optional donor asset image; original63 executable preserved. djultra64."""
import argparse,hashlib,json,pathlib,struct
BASE_SHA='357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'
DONOR_SHA='9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f'
IDS={6:'High Five',11:'Lady Liberty',12:'Lockjaw',16:'Zappa'}
def digest(b):return hashlib.sha256(b).hexdigest()
def build(base,donor):
 if digest(base)!=BASE_SHA or digest(donor)!=DONOR_SHA:raise ValueError('Only exact locally authorized USA revisions supported')
 result=bytearray(base);imports=[];allowed=set()
 for i,name in IDS.items():
  old_start=struct.unpack_from('>I',base,0xD9580+i*4)[0];old_end=struct.unpack_from('>I',base,0xD95C4+i*4)[0]
  if base[old_start:old_end]!=b'nullchar\0'+bytes(7):raise ValueError('Expected original63 null character slot')
  start=struct.unpack_from('>I',donor,0xAB260+i*4)[0];end=struct.unpack_from('>I',donor,0xAB2A4+i*4)[0];payload=donor[start:end]
  header=struct.unpack_from('>16I',payload);assert header[0]>>16==0x446 and header[1]==64
  if header[8]+header[9]>len(payload):raise ValueError('Donor metadata packet out of block')
  destination=(len(result)+15)&~15;result.extend(bytes(destination-len(result)));result.extend(payload)
  for table,value in [(0xD9580,destination),(0xD95C4,len(result))]:
   off=table+i*4;struct.pack_into('>I',result,off,value);allowed.update(range(off,off+4))
  imports.append({'id':i,'name':name,'source_range':[start,end],'destination_range':[destination,len(result)],'payload_sha256':digest(payload),'relative_offsets_preserved':True})
 changed={i for i,(a,b) in enumerate(zip(base,result)) if a!=b};assert changed<=allowed
 assert result[0x1000:0xD3520]==base[0x1000:0xD3520]
 for i in range(17):
  st=struct.unpack_from('>I',base,0xD9580+i*4)[0];en=struct.unpack_from('>I',base,0xD95C4+i*4)[0];assert result[st:en]==base[st:en]
 return bytes(result),{'author':'djultra64','status':'asset_loader_compatibility_verified; menu_renderer_combat_integration_unverified','base_sha256':BASE_SHA,'donor_sha256':DONOR_SHA,'asset_image_sha256':digest(result),'base_cpu_text_unchanged':True,'all_original_payload_ranges_unchanged':True,'changed_original_byte_count':len(changed),'imports':imports,'do_not_publish_donor_or_result':True}
def main():
 root=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=pathlib.Path,default=root/'probe-output/base.z64');p.add_argument('--donor',type=pathlib.Path,default=root/'probe-output/sc.z64');p.add_argument('--output',type=pathlib.Path,default=root/'analysis/sc_assets/asset-image.z64');a=p.parse_args()
 result,report=build(a.base.read_bytes(),a.donor.read_bytes());a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(result);a.output.with_suffix('.manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='imports'},indent=2))
if __name__=='__main__':main()
