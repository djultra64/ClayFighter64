#!/usr/bin/env python3
"""Private SC mesh import spike with original63 collision metadata; djultra64."""
from pathlib import Path
import argparse, hashlib, json, struct

ROOT = Path(__file__).resolve().parents[1]
BASE_SHA = '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'
SC_SHA = '9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def build(ids=(0, 5, 11)):
    base = (ROOT / 'probe-output/base.z64').read_bytes()
    donor = (ROOT / 'probe-output/sc.z64').read_bytes()
    assert sha(base) == BASE_SHA and sha(donor) == SC_SHA
    catalog = json.loads((ROOT / 'analysis/sc_stage_feasibility.json').read_text())
    image = bytearray(base)
    stages = {}
    for ident in ids:
        assert 0 <= ident < 26
        original = catalog['images']['base']['stages'][ident]
        sc = catalog['images']['sc']['stages'][ident]
        start, end = (int(x, 16) for x in original['definition_range'])
        mesh_start, mesh_end = (int(x, 16) for x in sc['mesh_range'])
        assert struct.unpack_from('>I', base, 0xd9ec0 + ident*4)[0] == start
        assert struct.unpack_from('>I', base, 0xd9f28 + ident*4)[0] == end
        definition = base[start:end]
        mesh = donor[mesh_start:mesh_end]
        assert definition[:3] == bytes.fromhex('030a01')
        assert mesh[:3] == bytes.fromhex('010a00')
        image.extend(bytes(-len(image) % 16))
        dest = len(image)
        image.extend(definition)
        mesh_dest = len(image)
        image.extend(mesh)
        stages[str(ident)] = {
            'base_name': original['name'], 'sc_name': sc['name'],
            'scope': 'hybrid_original_definition_sc_mesh',
            'definition_source': 'original63; original collision and camera behavior retained',
            'mesh_source': 'authentic Sculptors Cut',
            'definition_sha256': sha(definition), 'mesh_packed_sha256': sha(mesh),
            'appended_range': [dest, len(image)],
            'ram_table_patches': [
                {'address': hex(0x800d92c0 + ident*4), 'value': hex(dest)},
                {'address': hex(0x800d9328 + ident*4), 'value': hex(mesh_dest)},
                {'address': hex(0x800d9390 + ident*4), 'value': hex(mesh_dest)},
            ],
        }
    assert image[:len(base)] == base
    assert len(image) < 0x1000000
    return bytes(image), {
        'author': 'djultra64', 'experimental': True,
        'scope': 'SC geometry/textures with original63 metadata; full SC stage behavior unverified',
        'image_sha256': sha(image), 'base_prefix_sha256': BASE_SHA,
        'base_prefix_bytes': len(base), 'image_bytes': len(image),
        'all_original_bytes_unchanged': True, 'stages': stages,
        'private_asset_do_not_publish': True,
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ids', type=int, nargs='+', default=[0, 5, 11])
    args = parser.parse_args()
    image, report = build(tuple(args.ids))
    out = ROOT / 'local/sc_stages'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'hybrid-image.z64').write_bytes(image)
    (out / 'hybrid-manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
