#!/usr/bin/env python3
"""Private native branding fixture, by djultra64."""
from pathlib import Path
import subprocess,tempfile,argparse
R=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--red',action='store_true');args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='logo64-test-') as d:
 d=Path(d)
 source=R/'host/logo64.cpp'
 if args.red:
  source=d/'red.cpp';source.write_text('#include "logo64.h"\nextern "C" gpr clay_logo64_mesh(uint8_t*,recomp_context*,gpr p){return p;}\nextern "C" void clay_logo64_draw(uint8_t*,recomp_context*){}\n')
 sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
 subprocess.run(['g++','-std=c++20','-O1','-UNDEBUG','-I'+sdk,'-I'+str(R/'host'),str(R/'tests/logo64.cpp'),str(source),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test'),str(R/'local/logo64/resource1.bin')],check=True)
