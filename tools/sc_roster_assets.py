#!/usr/bin/env python3
"""Prepare private authentic SC portrait resources without changing the active asset image; djultra64."""
from pathlib import Path
import struct,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'local/sc_roster'
DONORS={6:0xf6eea0,11:0xf818d0,12:0xf865d0,16:0xf95d00}
sha=lambda b:hashlib.sha256(b).hexdigest()
def build(base,sc,modelimage):
 image=bytearray(modelimage)
 assert sha(sc)=='9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f'
 assert sha(image)=='05f3d63f8c413520e4ee5fecd0ec400361a5dc11df4a651ef06ad22431c733da'
 records=[]
 for slot,(ident,source) in enumerate(DONORS.items()):
  assert struct.unpack_from('>I',sc,0xabbf0+ident*4)[0]==source
  n=struct.unpack_from('>I',sc,source+8)[0];bank=sc[source:source+n];assert bank[:8]==bytes.fromhex('0100010000000002')
  start=len(image);image.extend(bank);image.extend(bytes((-len(image))%16));end=len(image)
  records.append({'character_id':ident,'sc_id_table_rom':'0xABBF0','source_rom':hex(source),'packed_bytes':n,'bank_sha256':sha(bank),'append_start':start,'append_end':end,'decoded_bytes':struct.unpack_from('>I',bank,20)[0],'resource_count':2})
 assert image[0x1000:0xd3520]==base[0x1000:0xd3520]
 manifest={'author':'djultra64','status':'Private authentic portrait package; no selector/HUD runtime integration claim','base_asset_prefix_bytes':15368752,'base_asset_prefix_sha256':'05f3d63f8c413520e4ee5fecd0ec400361a5dc11df4a651ef06ad22431c733da','size':len(image),'sha256':sha(image),'original_cpu_text_sha256':sha(image[0x1000:0xd3520]),'records':records}
 return bytes(image),manifest
def main():
 image,manifest=build((ROOT/'probe-output/base.z64').read_bytes(),(ROOT/'probe-output/sc.z64').read_bytes(),(ROOT/'analysis/sc_assets/asset-image.z64').read_bytes())
 OUT.mkdir(parents=True,exist_ok=True);(OUT/'asset-image-roster.z64').write_bytes(image);(OUT/'portrait-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
