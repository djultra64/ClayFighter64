#!/usr/bin/env python3
"""Training native contracts extracted from the user's original guest code. djultra64."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import re
from probe_training_states import ROOT, functions, excerpt, guard_rom


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contract',action='store_true')
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--prepare-only',type=Path)
    args=parser.parse_args()
    guard_rom((ROOT/'probe-output/base.z64').read_bytes())
    funcs=functions(ROOT/'generated/guest')
    snippets=[('damage','8007C338',0x8007C81C,0x8007C880),
              ('timer','80087158',0x800872F0,0x80087408),
              ('reserve','8007396C',0x80074C0C,0x80074C4C),
              ('depletion','8007C338',0x8007E000,0x8007E080)]
    source='#include "recomp.h"\n#include "training_guest.h"\nextern "C" {\nvoid func_80080880(uint8_t*,recomp_context*);\nvoid func_800790F0(uint8_t*,recomp_context*);\n'
    for name,address,start,stop in snippets:
        body=excerpt(funcs[f'func_{address}'],start,stop)
        source+=f'void training_original_{name}(uint8_t* rdram,recomp_context* ctx) {{\n bool c1cs=false;\n'+body+f'\nL_{stop:08X}: return;\n}}\n'
    source+=funcs['func_80079004']+'\n'
    source+='void training_original_ready(uint8_t* rdram,recomp_context* ctx){\n'
    source+=excerpt(funcs['func_80087158'],0x80088020,0x80088050)
    source+='\nL_80088B58: return;\n}\n'
    source+='}\n'
    if args.prepare_only:
        args.prepare_only.parent.mkdir(parents=True,exist_ok=True)
        args.prepare_only.write_text(source)
        return
    with tempfile.TemporaryDirectory(prefix='clay-training-contract-') as temp:
        temp=Path(temp);guest=temp/'guest.cpp';guest.write_text(source);binary=temp/'contract'
        command=['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(ROOT/'host'),
                        '-I'+str(args.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),
                        str(ROOT/'tests/training_guest.cpp'),str(guest),
                        str(ROOT/'host/training_guest.cpp'),str(ROOT/'host/training_position.cpp'),str(ROOT/'host/training_session.cpp'),
                        str(ROOT/'host/training_preferences.cpp'),
                        str(ROOT/'host/main_menu.cpp')]
        subprocess.run(command+['-o',str(binary)],check=True)
        subprocess.run([str(binary),str(ROOT/'probe-output/base.z64')],check=True)
        subprocess.run([str(binary),str(ROOT/'probe-output/base.z64'),'training'],check=True)
        # The unchanged original must fail the final-reserve training requirement.
        guest.write_text(re.sub(r'^.*clay::training::damage_applied.*\n','',source,flags=re.M))
        subprocess.run(command+['-o',str(binary)],check=True)
        red=subprocess.run([str(binary),str(ROOT/'probe-output/base.z64'),'training'])
        if red.returncode!=14:raise RuntimeError('Original final-reserve damage must reproduce missing Training KO protection')


if __name__=='__main__':
    main()
