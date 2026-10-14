#!/usr/bin/env python3
"""Prepare private original native final fixtures for Windows; djultra64."""
from pathlib import Path
import re,ast
root=Path(__file__).resolve().parents[2];functions={}
for path in (root/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void func_(800[0-9A-F]+)\([^\n]*\).*?;\}',path.read_text(),re.S):functions[m[1]]=m[0]
def write(name,bodies,extra=''):
 (root/'generated'/name).write_text('#include "recomp.h"\n#include "funcs.h"\n'+extra+'\nextern "C" {\n'+'\n'.join(bodies)+'\n}\n')
def list_from(script,var):
 tree=ast.parse((root/script).read_text())
 for node in ast.walk(tree):
  if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name)and t.id==var for t in node.targets):return ast.literal_eval(node.value)
 raise ValueError(var)
write('windows_memory_budget_guest.cpp',[functions[n]for n in list_from('tools/verify_memory_budget.py','names')], 'extern "C" { void osGetThreadId_recomp(uint8_t*,recomp_context*);void osRecvMesg_recomp(uint8_t*,recomp_context*);void osSendMesg_recomp(uint8_t*,recomp_context*);void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);}')
ui=[]
for n in list_from('tools/verify_ui_background.py','names'):
 b=functions[n];b=re.sub(r'clay::ui::draw_(?:fullscreen_fill|transition_band)\(rdram,\s*ctx,\s*&?(func_[0-9A-Fa-f]+)\);',r'\1(rdram,ctx);',b);b=re.sub(r'^.*clay::ui::.*\n','',b,flags=re.M);ui.append(b)
sdk=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK')
rdp=(sdk/'deps/rt64/src/hle/rt64_rdp.cpp').read_text()
fillrect=rdp[rdp.index('void RDP::fillRect(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry,'):]
quantizer=re.search(r'if \(\(mode == G_CYC_COPY\) \|\| \(mode == G_CYC_FILL\)\) \{([^}]+)\}',fillrect)
assert quantizer and 'lry |= 3;' in quantizer[1]
ui.append('int rt64_fill_lower_edge(int lry){int lrx=0;'+quantizer[1]+'return lry;}')
write('windows_ui_background_guest.cpp',ui)
write('windows_options_border_guest.cpp',[functions['8005B1E8']])
s=(root/'local/sc_portraits/guest.cpp').read_text()
for n in list_from('tools/verify_vs_alignment.py','names'):
 b=functions[n];b=re.sub(r'^.*clay::(?:begin_sprite_slices|end_sprite_slices|limit_sprite_slice_rows)\(.*\);\n','',b,flags=re.M)
 s=re.sub(r'(?:RECOMP_FUNC )?void func_'+n+r'\([^\n]*\)\s*\{[^\n]*\}\n','',s)
 if n=='800A6520':b=b.replace('{\n','{\n    vs_capture_sprite(rdram,ctx);\n',1)
 s=s[:-2]+b+'\n}\n'
s=s.replace('"../../host/sc_assets.h"','"sc_assets.h"').replace('#include "recomp.h"','#include "recomp.h"\nextern "C" void vs_capture_sprite(uint8_t*,recomp_context*);',1)
(root/'generated/windows_vs_alignment_guest.cpp').write_text(s)
b=functions['8009B6B4'];prefix=b[b.index('    // 0x8009B6B4:'):b.index('L_8009B744:')];bounds=b[b.index('L_8009B7B0:'):b.index('    // 0x8009B7FC:')];select=b[b.index('L_8009B854:'):b.index('    // 0x8009B890:')]
body='RECOMP_FUNC void clay_sound_selector(uint8_t*rdram,recomp_context*ctx){\n'+prefix+'L_8009B744:\n'+bounds+'ctx->r2=0;ctx->r29=ADD32(ctx->r29,0x38);return;\n'+select+'ctx->r2=MEM_W(0x1c,ctx->r29);ctx->r29=ADD32(ctx->r29,0x38);return;\n}\n'
write('windows_sound_guest.cpp',[functions['800C1A04'],functions['800C1910'],body])
print('Prepared original memory/UI/border/VS/sound native Windows fixtures')

# Dedicated title replacement closure; keep synth selector out of this target.
write("windows_title_audio_guest.cpp",[functions[n]for n in ["800C1A04","800C1910","800C1520"]])

# Actual selector matrix/name sprite helpers; keep game preview dependency mocked
# only in the bounded CPU contract fixture, never in the native executable.
selector=[]
for n in list_from('tools/verify_selector_layout.py','names'):
 b=functions[n];b=re.sub(r'^.*clay::(?:begin_sprite_slices|end_sprite_slices|limit_sprite_slice_rows)\(.*\);\n','',b,flags=re.M);selector.append(b)
selector_source=functions['8005E37C']
slot=re.search(r'    // 0x8005E524:.*?(?=    // 0x8005E5D0:)',selector_source,re.S)[0]
assert slot.count('unsigned(MEM_W(0x40,ctx->r29))')==2
selector.append('void selector_panel_slot(uint8_t*rdram,recomp_context*ctx){uint64_t hi=0,lo=0,result=0;int c1cs=0;\n'+slot+'return;\n}')
write('windows_selector_layout_guest.cpp',selector,'#include "selector_layout.h"')

# Authentic donor biographies: preserve original scroller and font/letter animation.
bios=[]
for n in list_from('tools/verify_sc_bios.py','names'):
 b=functions[n]
 b=re.sub(r'clay::bios::install\(rdram,\s*ctx\);','',b)
 b=re.sub(r'clay::input::prepare_native_prompt_(?:measure|draw)\(rdram,\s*ctx\);','',b)
 bios.append(b)
write('windows_sc_bios_guest.cpp',bios)
