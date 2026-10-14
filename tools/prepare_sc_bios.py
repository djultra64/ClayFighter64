#!/usr/bin/env python3
"""Extract private authentic SC biographies for the original native scroller; djultra64."""
from pathlib import Path
import argparse,struct,json,hashlib,re
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,default=R/'probe-output/sc.z64');p.add_argument('--base-rom',type=Path,default=R/'probe-output/base.z64');p.add_argument('--font',type=Path,default=R/'analysis/font_private/font.bin');p.add_argument('--output',type=Path,default=R/'local/sc_bios/bios.bin');a=p.parse_args()
sc=a.rom.read_bytes();base=a.base_rom.read_bytes();font=a.font.read_bytes();u32=lambda o:struct.unpack_from('>I',font,o)[0]
chars=font[u32(0)-12:].split(b'\\',1)[0].decode('ascii');width={c:struct.unpack_from('>H',font,u32(i*8)-12+6)[0] for i,c in enumerate(chars,1)};width[' ']=sum(width.values())//len(width)
def advance(s):return sum(width[c.upper()] for c in s)
# Original bio renderer 8006C6C0 clips x120..297, scale magnitude0.7 (ROM F2E14).
assert abs(struct.unpack_from('>f',base,0xF2E14)[0]+.7)<1e-6
budget=252 # floor((297-120)/.7); unscaled original native advance, not an arbitrary character guess.
entries=[];meta=[]
for id,offset in [(6,0xCA730),(11,0xCA0F0),(12,0xC9F68),(16,0xC9900)]:
 raw=sc[offset:sc.index(0,offset)];text=raw.decode('ascii');paragraphs=re.split(r'\r\n\s*\r\n',text.strip());lines=[]
 for paragraph in paragraphs:
  if lines:lines.append('')
  line=''
  for word in paragraph.split():
   candidate=(line+' '+word).strip()
   if len(candidate)>23 or advance(candidate)>budget:
    assert line;lines.append(line);line=word
   else:line=candidate
   assert len(line)<=23 and advance(line)<=budget
  if line:lines.append(line)
 assert ' '.join(text.split())==' '.join(' '.join(lines).split()),'word sequence changed'
 payload=b''.join(s.encode('ascii')+b'\0' for s in lines);entries.append(struct.pack('<III',id,len(lines),len(payload))+payload)
 meta.append({'id':id,'source_rom_offset':hex(offset),'source_sha256':hashlib.sha256(raw).hexdigest(),'lines':len(lines),'max_native_advance':max(map(advance,lines)),'max_characters':max(map(len,lines))})
asset=b'CFB1'+b''.join(entries);assert len(asset)<0x4000;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(asset);a.output.with_suffix('.json').write_text(json.dumps({'author':'djultra64','source_rom_sha256':hashlib.sha256(sc).hexdigest(),'native_clip':[120,297],'native_scale':.7,'space_advance':width[' '],'asset_sha256':hashlib.sha256(asset).hexdigest(),'entries':meta},indent=2)+'\n');print(json.dumps(meta))
