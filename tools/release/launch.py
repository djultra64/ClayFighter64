#!/usr/bin/env python3
"""Launch a ClayFighter 64 portable installation. djultra64."""
from pathlib import Path
import os
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.release.import_assets import installed, main as install_main

def main():
    arguments=sys.argv[1:]
    if '--help' not in arguments and not installed(ROOT):
        print('ClayFighter 64 needs BOTH USA ROMs to install its fused roster.')
        saved=sys.argv;sys.argv=[saved[0]]
        try:code=install_main()
        finally:sys.argv=saved
        if code:return code
    binary=ROOT/('clayfighter64.exe' if os.name=='nt' else 'clayfighter64')
    return subprocess.call([str(binary),'--rom','probe-output/base.z64',
                            '--sc-assets','local/sc_roster/asset-image-roster.z64',*arguments],cwd=ROOT)
if __name__=='__main__':sys.exit(main())
