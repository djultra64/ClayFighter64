#!/usr/bin/env python3
"""Prepare current private Training contracts for the Windows transfer. djultra64."""
from pathlib import Path
import argparse
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    args=parser.parse_args()
    destination=ROOT/'generated/windows_training_guest.cpp'
    subprocess.run([sys.executable,str(ROOT/'tools/verify_training.py'),'--sdk-root',str(args.sdk_root),
                    '--prepare-only',str(destination)],check=True,cwd=ROOT)
    subprocess.run([sys.executable,str(ROOT/'tools/verify_main_menu.py'),'--sdk-root',str(args.sdk_root),
                    '--prepare-only',str(ROOT/'generated/windows_main_menu_guest.cpp')],check=True,cwd=ROOT)
    for name in ['stage','selector','position']:
        subprocess.run([sys.executable,str(ROOT/f'tools/verify_training_{name}.py'),'--sdk-root',str(args.sdk_root),
                        '--prepare-only',str(ROOT/f'generated/windows_training_{name}_guest.cpp')],check=True,cwd=ROOT)
    (ROOT/'generated/windows_training_moves_markers.cpp').write_text('#include "native_prompts.h"\nnamespace clay::input {std::string binding_marker(Action a){return "{ACTION:"+std::to_string(int(a))+"}";}}\n')
    required=['host/training_guest.cpp','host/training_session.cpp','host/training_preferences.cpp',
              'tests/training_guest.cpp','tests/training_options.cpp','tests/training_session.cpp',
              'host/portable_paths.cpp','tests/portable_paths.cpp']
    for relative in required:
        if not (ROOT/relative).is_file():raise RuntimeError('Missing Windows Training input: '+relative)
    print('Prepared '+str(destination.relative_to(ROOT))+'; stage current host/, tests/ and tools/windows/ together.')


if __name__=='__main__':main()
