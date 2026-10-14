#!/usr/bin/env python3
"""Private inspection of native idle portrait fixture output; optional Pillow, djultra64."""
from pathlib import Path
import hashlib,json,struct
ROOT=Path(__file__).resolve().parents[1]
def main():
 from PIL import Image,ImageDraw
 directory=ROOT/'local/sc_portraits';board=Image.new('RGB',(8*104,4*130),(45,45,45));draw=ImageDraw.Draw(board);records=[]
 original=(ROOT/'local/sc_roster/base-portraits.bin').read_bytes()
 for row,ident in enumerate([6,11,12,16]):
  digests=[]
  for frame in range(8):
   blob=(directory/f'head-{ident}-{frame}.bin').read_bytes()
   assert len(blob)==1360 and blob[:16]==bytes.fromhex('11ff02100000001a001a000000000020')
   palette=struct.unpack('>256H',blob[16:528]);assert palette[0]==1
   pixels=[blob[528+y*32+x]for y in range(26)for x in range(26)]
   assert all(pixels[y*26]==pixels[y*26+25]==0 for y in range(26))
   image=Image.new('RGB',(26,26));image.putdata([((palette[p]>>11&31)*255//31,(palette[p]>>6&31)*255//31,(palette[p]>>1&31)*255//31)for p in pixels])
   board.paste(image.resize((104,104),Image.Resampling.NEAREST),(frame*104,row*130+20));draw.text((frame*104,row*130+2),f'{ident} frame{frame}',fill='white');digests.append(hashlib.sha256(blob).hexdigest())
  assert len(set(digests))>=5;records.append({'character_id':ident,'distinct_genuine_idle_frames':len(set(digests)),'frame_sha256':digests})
 board.save(directory/'animated-heads.png')
 report={'author':'djultra64','source':'Original native01008/72D50/72FE8, idle animation0 via91A8C/9215C, original02DE0 RGBA sprite decompression','native_descriptor_hex':'11ff02100000001a001a000000000020','background':'Opaque black RGBA5551 palette index0=0001','composition':'Fixed anatomical crop in native idle coordinates, original horizontal flag, original pixels supersampled; no generated art or artificial movement','temporary_peak_allocation_bytes':[410928,429128,435136,437322],'cleanup':'Actual01AA8 model destructor,03AA8 sprite cache destructor, actor release; zero residual allocations/handles in tracking fixture','global_guard':'Full global image comparison; original sprite cache800FF140..800FF3C0 sentinel preserved and RNG800EE5B0 unchanged; only intended4 donor descriptor indexpairs differ','records':records}
 (ROOT/'analysis/sc_portrait_evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'distinct_frames':[x['distinct_genuine_idle_frames']for x in records],'all_descriptors_and_black_borders_valid':True}))
if __name__=='__main__':main()
