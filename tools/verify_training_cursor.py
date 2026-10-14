#!/usr/bin/env python3
"""Execute the game's native cursor drawing branches for Training. djultra64."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from probe_training_states import ROOT, functions, guard_rom
from verify_training_selector import section

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root",type=Path,default=Path("/tmp/Blast Corps source review b48mc3gh/independent SDK"))
    sdk=parser.parse_args().sdk_root
    guard_rom((ROOT/'probe-output/base.z64').read_bytes())
    body=functions(ROOT/'generated/guest')['func_8005E37C']
    native='#include "recomp.h"\nextern "C" {\n'
    native+='uint64_t clay_sc_roster_coordinates(uint8_t*);\nvoid func_800A6520(uint8_t*,recomp_context*);\nvoid func_800A87C4(uint8_t*,recomp_context*);\n'
    native+='void native_cursor_draw(uint8_t*rdram,recomp_context*ctx){\n'
    native+=section(body,0x8005EC2C,0x8005F340)
    native+='\nL_8005F340:return;\n}\n}\n'
    with tempfile.TemporaryDirectory(prefix='clay-training-cursors-') as td:
        td=Path(td);guest=td/'guest.cpp';guest.write_text(native);binary=td/'cursor'
        subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(ROOT/'host'),'-I'+str(sdk/'deps/N64ModernRuntime/N64Recomp/include'),str(ROOT/'tests/training_cursor.cpp'),str(ROOT/'host/selector_layout.cpp'),str(ROOT/'host/graphics_config.cpp'),str(guest),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':main()
