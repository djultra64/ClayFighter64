#!/usr/bin/env python3
"""Native stage polygon/coordinate regressions for Training reset. djultra64."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from probe_training_states import ROOT, functions, guard_rom


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--private',action='store_true')
    parser.add_argument('--prepare-only',type=Path)
    args=parser.parse_args()
    guard_rom((ROOT/'probe-output/base.z64').read_bytes())
    native=functions(ROOT/'generated/guest')
    names=('80096D10','80096C4C','80096C9C','80086CB4','80087018','80087054','800C1D40')
    source='#include "recomp.h"\nextern "C" {\n'
    source+='\n'.join('void func_'+name+'(uint8_t*,recomp_context*);' for name in names)+'\n'
    source+='\n'.join(native['func_'+name] for name in names)+'\n}\n'
    if args.prepare_only:
        args.prepare_only.parent.mkdir(parents=True,exist_ok=True);args.prepare_only.write_text(source);return
    with tempfile.TemporaryDirectory(prefix='clay-training-position-') as directory:
        directory=Path(directory);guest=directory/'guest.cpp';guest.write_text(source);binary=directory/'position'
        subprocess.run(['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(ROOT/'host'),
                        '-I'+str(args.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),
                        str(ROOT/'tests/training_position.cpp'),str(ROOT/'host/training_position.cpp'),str(guest),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
        if args.private:
            for ident in range(26):
                path=ROOT/f'local/sc_stages/definitions/base-{ident:02}.bin'
                if not path.exists():raise RuntimeError('Missing private original stage definition: '+str(ident))
                print('Original stage',ident,flush=True)
                subprocess.run([str(binary),str(path)],check=True)
            print('PASS all 26 private original stage polygons, 156 side/corner cases')


if __name__=='__main__':main()
