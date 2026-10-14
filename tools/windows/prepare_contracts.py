#!/usr/bin/env python3
"""Prepare private original guest font functions for native Windows fixtures; djultra64."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
functions={}
for path in (root/'generated/guest').glob('funcs_*.c'):
 for address in ('80071738','8007191C','800C2D1C','8007145C','80072C4C'):
  match=re.search(r'RECOMP_FUNC void func_'+address+r'\([^\n]*\).*?;\}',path.read_text(),re.S)
  if match:
   functions[address]=re.sub(r'\s*clay::input::prepare_native_prompt_(?:measure|draw)\(rdram,\s*ctx\);','',match[0])
if len(functions)!=5:raise SystemExit('Missing original native font functions')
cleanup={key:functions.pop(key) for key in ('8007145C','80072C4C')}
for key in cleanup:cleanup[key]=re.sub(r'\s*clay::input::restore_native_prompt_font\(rdram,\s*ctx->r4\);','',cleanup[key])
(root/'generated/windows_native_font_cleanup_guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(cleanup.values())+'\n}\n')
(root/'generated/windows_native_font_guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(functions.values())+'\n}\n')

# Keep Options' actual per-letter animation closure separate from font/prompt tests.
letters=[]
for address in ('80063EFC','8006464C','80064C20','80064180','800B2154','800B1FF0','800C1520','80064870','8006493C'):
 for path in (root/'generated/guest').glob('funcs_*.c'):
  match=re.search(r'RECOMP_FUNC void func_'+address+r'\([^\n]*\).*?;\}',path.read_text(),re.S)
  if match:
   body=match[0]
   if address=='80064180':body=body.replace('{\n','{\n native_letter_draw_called();\n',1)
   if address=='8006493C':body=body.replace('{\n','{\n native_exit_draw_called();\n',1)
   if address=='80063EFC':body=body.replace('    return;', '    native_animation_seeded(rdram);\n    return;')
   letters.append(body);break
 else:raise SystemExit('Missing original letter animation '+address)
(root/'generated/windows_options_letters_guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\nvoid native_letter_draw_called();\nvoid native_exit_draw_called();\nvoid native_animation_seeded(uint8_t*);\n'+'\n'.join(letters)+'\n}\n')
