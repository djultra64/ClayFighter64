#!/usr/bin/env python3
"""Execute the generated original ROM character filter natively; authored by djultra64."""
import pathlib,re,struct,subprocess,json,hashlib
import argparse
root=pathlib.Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--include-dir',type=pathlib.Path,
 default=pathlib.Path('/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'))
args=parser.parse_args()
rom=(root/'probe-output/base.z64').read_bytes()
roster_addr=0x800DAC18;off=roster_addr-0x80000400+0x1000
roster=list(struct.unpack('>12H',rom[off:off+24]))
assert roster==[0,1,2,10,15,8,14,5,9,13,4,3]
assert rom[0x5E260:0x5E264].hex()=='a5000020'
text='\n'.join(p.read_text() for p in sorted((root/'generated/guest').glob('funcs_*.c')))
match=re.search(r'RECOMP_FUNC void func_8005D484\([^\n]*\).*?(?=RECOMP_FUNC|\Z)',text,re.S)
assert match and 'MEM_H(ctx->r14, 0X20)' in match[0]
source=root/'analysis/unlock_filter_native.cpp';source.write_text('#include "recomp.h"\nextern "C" {\nint clay_sc_roster_extra_allowed(uint64_t){return 0;} // Explicit classic-mode fixture.\nint clay_sc_roster_unlock_flags(uint64_t flags){return int(flags); }\n'+match[0]+'\n}\n')
header=str(args.include_dir)
binary=root/'analysis/unlock_selection_test'
subprocess.run(['g++','-std=c++20','-O2','-I'+header,str(root/'tests/unlock_selection.cpp'),str(source),'-o',str(binary)],check=True)
run=subprocess.run([str(binary)],check=True,text=True,capture_output=True);print(run.stdout,end='')
(root/'analysis/unlock_verification.json').write_text(json.dumps({'author':'djultra64','rom_sha256':hashlib.sha256(rom).hexdigest(),'rom_initializer_verified':'ROM0x5E260 a5000020 / CPU0x8005D660 sh zero,0x20(t0)','roster_table_verified':{'vram':hex(roster_addr),'rom_offset':hex(off),'ids':roster},'original_generated_filter':'func_8005D484','native_check_exit_code':run.returncode,'native_check_output':run.stdout},indent=2)+'\n')
