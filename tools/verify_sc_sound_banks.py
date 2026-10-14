#!/usr/bin/env python3
"""Bounded original/donor voice and SFX bank equivalence audit; djultra64.
Reads local private ROMs only; imports no engine or sound assets.
"""
from pathlib import Path
import argparse,struct,hashlib,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,default=root/'probe-output/base.z64');p.add_argument('--donor',type=Path,default=root/'probe-output/sc.z64');a=p.parse_args()
base=a.base.read_bytes();donor=a.donor.read_bytes()
def parse(rom,ctl,tbl):
 data=rom[ctl:tbl];used=set();records=[]
 def block(off,size):
  if not 0<=off<=len(data) or off+size>len(data):raise ValueError('CTL reference exceeds source bank')
  used.update(range(off,off+size));return data[off:off+size]
 def u32(off):return struct.unpack('>I',block(off,4))[0]
 def u16(off):return struct.unpack('>H',block(off,2))[0]
 assert block(0,2)==b'B1';nb=u16(2);block(4,nb*4)
 def instrument(off,bi,ii):
  if not off:return
  count=u16(off+14);block(off,16+count*4)
  for si in range(count):
   snd=u32(off+16+si*4);block(snd,16);env,key,wave=[u32(snd+j*4)for j in range(3)];block(env,16);block(key,6);wb=block(wave,20);sample_off,length=struct.unpack_from('>II',wb);kind=wb[8]
   if not 0<length or tbl+sample_off+length>len(rom):raise ValueError('Sample reference exceeds source ROM')
   loop=u32(wave+12);book=u32(wave+16)if kind==0 else 0
   if loop:block(loop,44 if kind==0 else 12)
   if book:
    order,predictors=struct.unpack('>II',block(book,8));assert 0<order<=16 and 0<predictors<=256;block(book,8+order*predictors*16)
   records.append({'bank':bi,'instrument':ii,'sound':si,'wave_type':kind,'sample_bytes':length,'sample_sha256':hashlib.sha256(rom[tbl+sample_off:tbl+sample_off+length]).hexdigest(),'envelope':block(env,16).hex(),'keymap':block(key,6).hex(),'sound_parameters':block(snd+12,4).hex(),'instrument_parameters':block(off,14).hex()})
 for bi in range(nb):
  bank=u32(4+bi*4);count=u16(bank);block(bank,12+count*4);instrument(u32(bank+8),bi,'percussion')
  for ii in range(count):instrument(u32(bank+12+ii*4),bi,ii)
 return data,used,records
banks=[]
for name,bc,bt,dc,dt in [('voices',0xa33c70,0xa38ac0,0xe221a0,0xe26ff0),('combat',0xb141b0,0xb156b0,0xf040c0,0xf055c0),('menu',0xb2f060,0xb30620,0xf1ef70,0xf20530)]:
 bd,bu,br=parse(base,bc,bt);dd,du,dr=parse(donor,dc,dt);assert bu==du;assert all(bd[i]==dd[i]for i in bu);assert br==dr
 diffs=[i for i,(x,y)in enumerate(zip(bd,dd))if x!=y];assert not set(diffs)&bu
 banks.append({'bank':name,'original_ctl_rom':hex(bc),'original_sample_rom':hex(bt),'donor_ctl_rom':hex(dc),'donor_sample_rom':hex(dt),'sound_records':len(br),'all_reachable_ctl_bytes_identical':True,'all_sample_and_sound_parameter_records_identical':True,'differing_unreferenced_padding_offsets':[hex(x)for x in diffs],'original_engine_dispatch_mapping':'identity for every valid bank/instrument/sound ID','records':br})
report={'author':'djultra64','result':'PASS','donor_characters':[6,11,12,16],'audio_engine':'original63 only','extra_audio_assets_required':False,'sound_remapping_required':False,'scope':'Every reachable ALbank/instrument/sound/envelope/keymap/wavetable/ADPCMbook/loop reference is bounds-checked; ALL donor voices/combat/menu bank samples and parameters exist identically in original63. Does not claim an audible test of every donor move.','banks':banks}
(root/'analysis/sc_sound_bank_evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: original63 matches donor all reachable voice/combat/menu bank structures, parameters and sample bytes; counts '+str([(b['bank'],b['sound_records'])for b in banks])+'; no donor sound remapping/import required')
