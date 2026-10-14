#!/usr/bin/env python3
"""Verify native stage input with Training hooks; private original source. djultra64."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
from probe_training_states import ROOT, functions, guard_rom


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--prepare-only',type=Path)
    args=parser.parse_args()
    image=(ROOT/'probe-output/base.z64').read_bytes();guard_rom(image)
    # These instructions implement confirm-readiness and stage navigation.
    for address,expected in [(0x8006D54C,'0c02832a'),(0x8006D60C,'29a10065'),
                             (0x8006D654,'ad280000'),(0x8006D6B4,'3328000a'),
                             (0x8006D718,'33190005')]:
        offset=address-0x80000400+0x1000
        assert image[offset:offset+4].hex()==expected,f'Unexpected stage instruction at {address:08X}'
    body=functions(ROOT/'generated/guest')['func_8006D544']
    body=re.sub(r'^.*clay::training::stage_input_polled\(rdram\);.*\n','',body,flags=re.M)
    assert body.count('func_800A0CA8(rdram, ctx);')==1
    assert '    // 0x8006D54C: jal         0x800A0CA8' in body
    body=body.replace('func_8006D544(', 'training_original_stage_input(',1)
    body=body.replace('func_800A0CA8(rdram, ctx);','func_800A0CA8(rdram, ctx);\n    clay::training::stage_input_polled(rdram);')
    source='#include "recomp.h"\n#include "training_stage.h"\nextern "C" {\nvoid func_800A0CA8(uint8_t*,recomp_context*);\n'+body+'\n}\n'
    if args.prepare_only:
        args.prepare_only.parent.mkdir(parents=True,exist_ok=True);args.prepare_only.write_text(source);return
    work=ROOT/'local/tests/training-stage';work.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='native stage ',dir=work) as directory:
        directory=Path(directory);guest=directory/'guest.cpp';guest.write_text(source);binary=directory/'stage-test'
        command=['c++','-std=c++20','-O1','-UNDEBUG','-I'+str(ROOT/'host'),
                 '-I'+str(args.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),
                 str(ROOT/'tests/training_stage.cpp'),str(ROOT/'host/training_stage.cpp'),str(guest),'-o',str(binary)]
        subprocess.run(command,check=True)
        subprocess.run([str(binary),str(directory/'training-stage.cfg')],check=True,cwd=ROOT)


if __name__=='__main__':main()
