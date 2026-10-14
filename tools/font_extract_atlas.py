#!/usr/bin/env python3
"""Extract native CI4/RGBA5551 font pixels from locally decoded ROM; djultra64."""
from pathlib import Path
import struct,json
from PIL import Image
R=Path(__file__).resolve().parents[1];O=R/'analysis/font_private';b=(O/'font.bin').read_bytes()
u32=lambda o:struct.unpack_from('>I',b,o)[0]
u16=lambda o:struct.unpack_from('>H',b,o)[0]
# Native loader indexes bundle+12; decoded packet starts after outer 12-byte header.
chars=b[u32(0)-12:].split(b'\\0',1)[0].decode('ascii');atlas=Image.new('RGBA',(512,160));meta=[]
for i,c in enumerate(chars,1):
 o=u32(i*8)-12;w=u16(o+6);h=u16(o+8);stride=u16(o+14);pixeloff=u16(o+2)
 bits=4 if b[o]==0x30 else 8
 assert b[o] in (0x30,0x31) and pixeloff==16+(1<<bits)*2 and stride>=((w*bits+7)//8)
 pal=[]
 for p in range(1<<bits):
  v=u16(o+16+p*2);pal.append(((v>>11)*255//31,((v>>6)&31)*255//31,((v>>1)&31)*255//31,255*(v&1)))
 im=Image.new('RGBA',(w,h))
 for y in range(h):
  for x in range(w):
   v=b[o+pixeloff+y*stride+(x//2 if bits==4 else x)];idx=((v>>4)if x%2==0 else(v&15)) if bits==4 else v;im.putpixel((x,y),pal[idx])
 x=((i-1)%16)*32;y=((i-1)//16)*32;atlas.paste(im,(x,y));meta.append({'character':c,'ascii':ord(c),'bundle_offset':o+12,'x':x,'y':y,'width':w,'height':h,'stride_bytes':stride,'palette_offset':o+28,'pixel_offset':o+12+pixeloff})
atlas.save(O/'atlas.png');(O/'atlas.json').write_text(json.dumps({'source_rom_offset':'0xB68EE0','codec':96,'decoded_bytes':len(b),'format':'CI4/CI8 per-glyph RGBA5551 palette','glyphs':meta},indent=2)+'\n')
print(f'{len(meta)} authentic native glyphs extracted; mapping={chars!r}')
