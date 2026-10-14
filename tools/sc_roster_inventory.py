#!/usr/bin/env python3
"""Inventory exact original selector instructions for scoped native hooks; djultra64."""
from pathlib import Path
import re,struct,json
R=Path(__file__).resolve().parents[1]
B=(R/'probe-output/base.z64').read_bytes()
def signed(v):return v-65536 if v&32768 else v
def main():
 funcs={}
 for p in (R/'generated/guest').glob('funcs_*.c'):
  for m in re.finditer(r'RECOMP_FUNC void (func_\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
 hooks=[];candidates=[]
 def record(fn,pc,category,new,reason,confidence='verified'):
  w=struct.unpack_from('>I',B,pc-0x80000000+0xC00)[0];h={'function':fn,'instruction_vram':hex(pc),'expected_rom_word':f'{w:08x}','category':category,'new_statement':new,'reason':reason,'confidence':confidence};(hooks if confidence=='verified' else candidates).append(h)
 relevant={n for n,s in funcs.items()if '-0X53E8' in s or '0x53E8' in s}|{'func_8005DDCC','func_8005D484'}
 for fn in sorted(relevant):
  s=funcs[fn];addresses=sorted(set(int(a,16)for a in re.findall(r'// 0x([0-9A-F]+):',s)));regs={0:('c',0,None),29:('sp',0,None)};stack={};kill=None
  for pc in addresses:
   if kill==pc:
    for r in [*range(1,16),24,25,31]:regs.pop(r,None)
   w=struct.unpack_from('>I',B,pc-0x80000000+0xC00)[0];op=w>>26;rs=(w>>21)&31;rt=(w>>16)&31;rd=(w>>11)&31;sh=(w>>6)&31;f=w&63;im=signed(w&65535);a=regs.get(rs);v=regs.get(rt);result=None;dest=None
   if op==15:dest=rt;result=('c',(w&65535)<<16,pc)
   elif op==9:
    dest=rt
    if a and a[0]=='c':result=('c',(a[1]+im)&0xffffffff,pc)
    elif a and a[0] in ('sel','sp','col','row','root'):result=(a[0],None if a[1]is None else a[1]+im,pc)
    if a and a[0]=='sel' and im==0x2C:record(fn,pc,'flags_address',f'ctx->r{rt} = ADD32(ctx->r{rs}, clay_sc_roster_flag_offset());','Address of packed per-slot selector flags; redirect to appended sidecar in16 mode')
    if im==-0x5418:record(fn,pc,'coordinates_base',f'ctx->r{rt} = clay_sc_roster_coordinates(rdram);','Original12-coordinate array at800DABE8 needs16 coordinates in selector sidecar')
   elif op in (0x23,0x21,0x25,0x24,0x20):
    dest=rt
    if a and a[0]=='c' and op==0x23:
     addr=(a[1]+im)&0xffffffff
     if addr==0x800DDD30:result=('sel',0,pc)
     if addr==0x800DDD40:result=('choice',0,pc)
    elif a and a[0]=='sp' and a[1]is not None:result=stack.get(a[1]+im)
    if a and a[0]=='sel':
     off=im if a[1]is None else im+a[1]
     if op==0x23 and off in (0x78,0x80):result=('col',0,pc)
     if op==0x23 and off in (0x7C,0x84):result=('row',0,pc)
     if op in (0x21,0x25) and im==0x2C:record(fn,pc,'flags_read',f'ctx->r{rt} = MEM_{"H" if op==0x21 else "HU"}(clay_sc_roster_flag_offset(), ctx->r{rs});','Per-slot flags from selector global800DDD30; preserve other scalarfields')
    if a and a[0]=='choice' and im==0x20 and op in (0x21,0x25) and fn in ('func_8005FC68','func_80061474'):record(fn,pc,'navigation_unlock_flags',f'ctx->r{rt} = clay_sc_roster_unlock_flags(MEM_{"H" if op==0x21 else "HU"}(0X20, ctx->r{rs}));','Only native selector navigation reads exposelogicalfourth-column bit8 forZappa; stored originalmask unchanged')
    if op in (0x21,0x25) and im in (-0x5418,-0x5416):record(fn,pc,'coordinates_read',f'ctx->r{rt} = MEM_{"H" if op==0x21 else "HU"}(0, clay_sc_roster_coordinate_address(rdram, ADD32(ctx->r{rs}, {im})));','Native cursor/worldcoordinate read from original12-position table')
    if op==0x25 and im==-0x53E8:record(fn,pc,'slot_lookup',f'ctx->r{rt} = clay_sc_roster_id_from_slot((uint32_t(ADD32(ctx->r{rs}, -0x53E8)) - 0x800DAC18u) / 2);','Read mapped characterID without overwriting adjacent ROMfloat data')
   elif op in (0x2B,0x29):
    if a and a[0]=='sp' and a[1]is not None:stack[a[1]+im]=v
    if a and a[0]=='sel':
     off=im if a[1]is None else im+a[1]
     if op==0x29 and im==0x2C:record(fn,pc,'flags_write',f'MEM_H(clay_sc_roster_flag_offset(), ctx->r{rs}) = ctx->r{rt};','Per-slot flag write must target expanded32byte sidecar')
     if op==0x2B and off in (0x78,0x80) and v and v[0]=='c' and v[1]==2 and v[2]:record(fn,v[2],'column_wrap_constant',f'ctx->r{rt} = clay_sc_roster_columns() - 1;','Original col2 wrap destination becomes col3 in4-column selector')
   elif op==0:
    if f==0:
     dest=rd
     if v and v[0]=='row' and sh==1:result=('row2',rt,pc)
     elif v and v[0]=='c':result=('c',(v[1]<<sh)&0xffffffff,pc)
    elif f in (0x21,0x25):
     dest=rd
     if a and a[0]=='row2' and v and v[0]=='row':
      record(fn,pc,'row_stride',f'ctx->r{rd} = clay_sc_roster_row_stride(ctx->r{rt});','Original row*3 index becomes row*4, leaving four rows intact')
     if a and a[0]=='sel' or v and v[0]=='sel':result=('sel',None,pc)
     elif a and a[0]=='c' and v and v[0]=='c':result=('c',(a[1]+v[1])&0xffffffff,pc)
     elif a and a[0]=='sp' and v and v[0]=='c':result=('sp',a[1]+v[1],pc)
     elif a and a[0]=='c':result=('root',a[1],pc)
     elif v and v[0]=='c':result=('root',v[1],pc)
    elif f not in (8,0x18,0x19,0x1A,0x1B):dest=rd
   elif op in (4,5):
    for scalar,constant,cr in [(a,v,rt),(v,a,rs)]:
     if scalar and scalar[0]=='col' and constant and constant[0]=='c' and constant[1]==2 and constant[2]:record(fn,constant[2],'column_bound_constant',f'ctx->r{cr} = clay_sc_roster_columns() - 1;','Native branch compares lastcol2; scoped4-column bound becomes3')
   elif op==0xA:
    dest=rt
    if a and a[0]=='col' and im==3:record(fn,pc,'column_bound',f'ctx->r{rt} = SIGNED(ctx->r{rs}) < clay_sc_roster_columns() ? 1 : 0;','Horizontal navigation wraps at4 columns, original3 mode preserved')
   elif op not in (2,3,4,5,6,7,0x11,0x2A,0x2E,0x31,0x39):dest=rt
   if op==3:kill=pc+8
   if dest and dest!=29:
    if result:regs[dest]=result
    else:regs.pop(dest,None)
   elif dest==29 and result:regs[29]=result
   if op==9 and rs==0 and im==12:record(fn,pc,'slot_count_candidate',f'ctx->r{rt} = clay_sc_roster_count();','Review native12-slot loop/search/random modulus; never blanketchange campaignround limits','review')
   if op==0xA and im==12:record(fn,pc,'slot_limit_candidate',f'ctx->r{rt} = SIGNED(ctx->r{rs}) < clay_sc_roster_count() ? 1 : 0;','Review native slot/search bound; preserve campaignrules','review')
   if fn=='func_800636E4' and op==9 and rs==0 and im==3:record(fn,pc,'inverse_columns',f'ctx->r{rt} = clay_sc_roster_columns();','Inverse IDlookup computes cursor row and column by division/modulo3')
 # Reviewed twelve-slot counts: sound IDs12 remain unchanged at62258/62360.
 sound={0x80062258,0x80062360}
 kept=[]
 for c in candidates:
  if int(c['instruction_vram'],16) not in sound:
   c['confidence']='verified';c['category']='slot_count';c['reason']='Reviewed draw loop, inverseIDsearch, per-slot animation counter or random eligible-slot modulus';hooks.append(c)
  else:kept.append(c)
 candidates=kept
 record('func_8005E37C',0x8005E700,'slot_unlock_predicate','ctx->r1 = clay_sc_roster_slot_unlocked(rdram, ctx->r12);','Original first9 cutoff cannot classify preservedoriginal secretcharacters shifted toindices12..14; use actual ID/unlockbits')
 record('func_8005E37C',0x8005E714,'secret_bit_shift','ctx->r15 = clay_sc_roster_secret_shift(ctx->r12);','Original(slot+23)%32 mapsold9..11 tobits0..2; preservedSCpositions needID-based mapping')
 # Dedupe annotations arising from repeated generated delay slots/constantproducer recognition.
 hooks=list({(h['function'],h['instruction_vram'],h['category']):h for h in hooks}.values())
 artifact={'author':'djultra64','status':'Exact original instruction inventory; review slot-count candidates and unresolved flag/bounds before atomic integration','module':['host/sc_roster.h','host/sc_roster.cpp'],'private_image':'local/sc_roster/asset-image-roster.z64','image_sha256':'26ccae86b452d4bacadf429e0b241751aeef58a41d676e7d6173974414dafd7d','layout':[ [0,1,2,6],[10,15,8,11],[14,5,9,12],[13,4,3,16]],'selector_allocation':{'function':'func_8005DDCC','instruction_vram':'0x8005DDF4','expected_rom_word':'240500b8','new_statement':'ctx->r5 = clay_sc_roster_state_bytes();','original_bytes':184,'expanded_bytes':280,'flags_offset':184,'coordinates_offset':216},'install':{'function':'func_8005DDCC','location':'Before final return after original epilogue','call':'clay_sc_roster_install(rdram,ctx);'},'portrait_shared_posthook':{'function':'func_800AFFE0','entry':'const gpr portrait_source=ctx->r4;','every_return':'clay_sc_roster_augment_portraits(rdram,ctx,portrait_source);','scope':'Only verifiedroster mode and sourceROMB6BA90; bothselector and genuinecachedHUD builder5AC38 reloadthisbank'},'verified_hooks':hooks,'review_candidates':candidates,'missing_required_hooks':['All direct 0x30/36/3C/42 per-row flip initializers need sidecar reset handling (install resets16flags)','Native3D tile border/background must match16positions; do not claim coherentUI from portrait/table presence alone']}
 existing=R/'config/sc_roster_hooks.json'
 if existing.exists():
  previous=json.loads(existing.read_text())
  for key in ('native_grid_draw','native_grid_release'): 
   if key in previous:artifact[key]=previous[key]
 (R/'config/sc_roster_hooks.json').write_text(json.dumps(artifact,indent=2)+'\n')
 print('verified hooks',len(hooks),'review candidates',len(candidates));print({c:sum(h['category']==c for h in hooks)for c in sorted({h['category']for h in hooks})})
if __name__=='__main__':main()
