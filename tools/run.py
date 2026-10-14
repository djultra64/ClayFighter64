#!/usr/bin/env python3
"""Launch the locally built ClayTality 64 with the verified original ROM.

Project work: djultra64. ROM data remains local and ignored by Git.
"""
import hashlib
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from probes.feasibility import load_rom


def main():
    binary = ROOT / 'build/claytality64'
    if not binary.is_file():
        raise SystemExit('Build claytality64 first with CMake.')
    image = load_rom(ROOT / 'Clay Fighter 63 1-3 (USA).zip')
    if hashlib.sha256(image).hexdigest() != '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07':
        raise SystemExit('Unsupported ROM revision; expected original 63⅓ USA.')
    rom = ROOT / 'probe-output/base.z64'
    rom.parent.mkdir(exist_ok=True)
    if not rom.is_file() or rom.read_bytes() != image:
        rom.write_bytes(image)
    arguments=sys.argv[1:]
    original='--original' in arguments
    arguments=[a for a in arguments if a!='--original']
    if not original and '--sc-assets' not in arguments and '--help' not in arguments:
        from tools.port_assets import prepare
        donor=load_rom(ROOT / "Clay Fighter - Sculptor's Cut (USA).zip")
        assets=prepare(ROOT,image,donor)
        arguments=['--sc-assets',str(assets),*arguments]
    raise SystemExit(subprocess.call([str(binary), '--rom', str(rom), *arguments], cwd=ROOT))


if __name__ == '__main__':
    main()
