#!/usr/bin/env python3
"""Launch an explicit donor-character experiment in the original63 engine. djultra64."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from probes.feasibility import load_rom
from tools.sc_import_assets import build

CHARACTERS = {'highfive': (6, 'High Five'), 'liberty': (11, 'Lady Liberty'),
              'lockjaw': (12, 'Lockjaw'), 'zappa': (16, 'Zappa')}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('character', nargs='?', choices=CHARACTERS, default='highfive')
    args, game_args = parser.parse_known_args()
    base = load_rom(ROOT / 'Clay Fighter 63 1-3 (USA).zip')
    donor = load_rom(ROOT / "Clay Fighter - Sculptor's Cut (USA).zip")
    image, evidence = build(base, donor)
    output = ROOT / 'analysis/sc_assets/asset-image.z64'
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_bytes() != image:
        output.write_bytes(image)
    output.with_suffix('.manifest.json').write_text(json.dumps(evidence, indent=2) + '\n')
    character_id, name = CHARACTERS[args.character]
    print(f'Prueba experimental: {name}. El selector aún conserva sus doce casillas originales.', flush=True)
    print('Enter abre el menú. Z confirma Game Start y luego el personaje; esta prueba fuerza al donante elegido.', flush=True)
    raise SystemExit(subprocess.call([sys.executable, str(ROOT / 'tools/run.py'),
        '--sc-assets', str(output), '--sc-demo-character', str(character_id), *game_args], cwd=ROOT))

if __name__ == '__main__':
    main()
