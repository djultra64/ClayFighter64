#!/usr/bin/env python3
"""Throwaway binary feasibility probe, by djultra64. This is not a game port.

Only accepts the two exact locally supplied NTSC-U images. No game data is
embedded in this script. Generated ROMs/C/binaries stay in ignored output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
IDS = ['BMF', 'BLOB', 'BONKER', 'BOOGER', 'KILN', 'EWJ', 'HIGHFIVE',
       'HOBO', 'HOUNGAN', 'ICKY', 'KUNGPOW', 'LIBERTY', 'LOCKJAW',
       'SUMO', 'TAFFY', 'THOPPY', 'ZAPPA']
EXTRAS = [6, 11, 12, 16]
IMAGES = {
    'base': ('Clay Fighter 63 1-3 (USA).zip',
             '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07',
             0xD9580, 0xD9608),
    'sc': ("Clay Fighter - Sculptor's Cut (USA).zip",
           '9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f',
           0xAB260, 0xAB2E8),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load_rom(path):
    with zipfile.ZipFile(path) as archive:
        entries = [n for n in archive.namelist()
                   if n.lower().endswith(('.z64', '.n64', '.v64'))]
        if len(entries) != 1:
            raise ValueError('Expected exactly one ROM in ZIP')
        data = bytearray(archive.read(entries[0]))
    if data[:4] == bytes.fromhex('37804012'):
        data[0::2], data[1::2] = data[1::2], data[0::2]
    elif data[:4] == bytes.fromhex('40123780'):
        for offset in range(0, len(data), 4):
            data[offset:offset + 4] = data[offset:offset + 4][::-1]
    if data[:4] != bytes.fromhex('80371240'):
        raise ValueError('Invalid N64 ROM byte order')
    return data


def records(data, ranges, names):
    starts = struct.unpack_from('>17I', data, ranges)
    ends = struct.unpack_from('>17I', data, ranges + 68)
    pointers = struct.unpack_from('>17I', data, names)
    result = []
    for index, (start, end, pointer) in enumerate(zip(starts, ends, pointers)):
        offset = pointer - 0x80000400 + 0x1000
        name = data[offset:data.index(0, offset)].decode('ascii')
        assert name == 'CHAR_' + IDS[index], (index, name)
        assert 0 <= start < end <= len(data)
        if index < 16:
            assert end == starts[index + 1]
        payload = data[start:end]
        result.append(dict(id=index, name=name, start=start, end=end,
                           size=end-start, sha256=sha(payload),
                           nullchar=payload.startswith(b'nullchar\0'),
                           header_hex=payload[:64].hex()))
    return result


def native_probe(out, n64recomp, include_dir):
    # The exact original MIPS name lookup routine, independently identified
    # through its reference to the 17-entry character-name pointer table.
    (out / 'base.symbols.toml').write_text('''[[section]]
name = "probe"
rom = 0x1000
vram = 0x80000400
size = 0xF7D80
functions = [
 { name = "cf63_character_name", vram = 0x80001BAC, size = 0x34 },
]
''')
    (out / 'base.recomp.toml').write_text('''[input]
symbols_file_path = "base.symbols.toml"
rom_file_path = "base.z64"
output_func_path = "native"
''')
    run = subprocess.run([str(n64recomp), str(out / 'base.recomp.toml')],
                         check=True, text=True, capture_output=True)
    (out / 'recompiler.log').write_text(run.stdout + run.stderr)
    subprocess.run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra',
                    '-Wno-unused-variable', '-Wno-unused-parameter',
                    '-I', str(include_dir), '-I', str(out / 'native'),
                    str(ROOT / 'probes/native_lookup.c'),
                    *map(str, sorted((out / 'native').glob('*.c'))),
                    '-lm', '-o', str(out / 'native_lookup')], check=True)
    test = subprocess.run([str(out / 'native_lookup'), str(out / 'base.z64')],
                          check=True, capture_output=True, text=True)
    (out / 'native.log').write_text(test.stdout + test.stderr)
    return {'status': 'passed', 'checks': 19,
            'scope': 'Original 63⅓ character-name lookup only; no game runtime',
            'log': test.stdout.strip()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'probe-output')
    parser.add_argument('--n64recomp', type=Path)
    parser.add_argument('--include-dir', type=Path)
    args = parser.parse_args()
    if bool(args.n64recomp) != bool(args.include_dir):
        parser.error('--n64recomp and --include-dir must be supplied together')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    roms, tables = {}, {}
    for key, (filename, expected, ranges, names) in IMAGES.items():
        data = load_rom(ROOT / filename)
        assert sha(data) == expected, f'Unsupported revision: {key}'
        roms[key] = data
        tables[key] = records(data, ranges, names)
        (out / (key + '.z64')).write_bytes(data)
    graft = bytearray(roms['base'])
    imported = []
    for index in EXTRAS:
        original, donor = tables['base'][index], tables['sc'][index]
        assert original['nullchar'] and original['size'] == 16
        assert not donor['nullchar'] and donor['header_hex'].startswith('0446')
        payload = roms['sc'][donor['start']:donor['end']]
        start = (len(graft) + 15) & ~15
        graft.extend(bytes(start - len(graft)))
        graft.extend(payload)
        for table_offset, value in [(0xD9580, start),
                                    (0xD9580 + 68, len(graft))]:
            struct.pack_into('>I', graft, table_offset + index * 4, value)
        imported.append(dict(id=index, name=donor['name'], start=start,
                             end=len(graft), size=len(payload), sha256=sha(payload)))
    allowed = {base + index * 4 + byte for base in (0xD9580, 0xD95C4)
               for index in EXTRAS for byte in range(4)}
    changed = {i for i, (a, b) in enumerate(zip(roms['base'], graft)) if a != b}
    assert changed <= allowed
    for item in imported:
        assert sha(graft[item['start']:item['end']]) == item['sha256']
        index = item['id']
        assert struct.unpack_from('>I', graft, 0xD9580 + index * 4)[0] == item['start']
        assert struct.unpack_from('>I', graft, 0xD95C4 + index * 4)[0] == item['end']
    for item in tables['base']:
        assert sha(graft[item['start']:item['end']]) == item['sha256']
    (out / 'graft-STRUCTURAL-ONLY-NOT-BOOT-TESTED.z64').write_bytes(graft)
    native = native_probe(out, args.n64recomp, args.include_dir) if args.n64recomp else {
        'status': 'not_run'}
    report = dict(author='djultra64', classification='throwaway feasibility probe',
                  images={key: dict(size=len(data), sha256=sha(data),
                                    characters=tables[key]) for key, data in roms.items()},
                  graft=dict(status='structural_checks_passed', imports=imported,
                             changed_base_bytes=len(changed),
                             unchanged_original_payloads=17, size=len(graft),
                             sha256=sha(graft), boot_tested=False,
                             crc_repaired=False, moves_adapted=False),
                  native=native, graphics=dict(widescreen='not_tested',
                                              output_4k='not_tested',
                                              interpolation_120fps='not_tested'),
                  conclusion='Shared IDs and isolated donor blocks verified. '
                             'Playable integration is unproven; port character '
                             'routines, decode assets and test combat next.')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    for item in imported:
        print(f"{item['name']}: donor {item['size']} bytes -> 63⅓ appended slot")
    print(f'Original ROM prefix: only {len(changed)} table bytes changed; '
          'all 17 original payload ranges retained')
    print('Native lookup:', native['status'])
    print('Playable integration and graphics enhancements: NOT VERIFIED')
    print(out / 'report.json')


if __name__ == '__main__':
    main()
