#!/usr/bin/env python3
"""Launch a ClayFighter 64 portable installation. djultra64."""
from pathlib import Path
import os
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.release.import_assets import installed, main as install_main, windows_message

def main():
    arguments=sys.argv[1:]
    if '--help' not in arguments and not installed(ROOT):
        print('ClayFighter 64 needs BOTH USA ROMs to install its fused roster.')
        saved=sys.argv;sys.argv=[saved[0]]
        try:code=install_main()
        finally:sys.argv=saved
        if code:return code
    binary=ROOT/('clayfighter64.exe' if os.name=='nt' else 'clayfighter64')
    windowed = os.name == 'nt' and Path(sys.executable).name.lower() == 'pythonw.exe'
    try:
        code = subprocess.call([str(binary),'--rom','probe-output/base.z64',
                                '--sc-assets','local/sc_roster/asset-image-roster.z64',*arguments],
                               cwd=ROOT, creationflags=0x08000000 if windowed else 0)
        if windowed and code: windows_message(f'ClayFighter 64 could not start (error {code}).', True)
        return code
    except OSError as error:
        if windowed: windows_message(f'ClayFighter 64 could not start: {error}', True)
        else: print(f'ClayFighter 64 could not start: {error}', file=sys.stderr)
        return 1
if __name__=='__main__':sys.exit(main())
