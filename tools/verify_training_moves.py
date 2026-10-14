#!/usr/bin/env python3
"""Verify bounded runtime move parsing, optionally against private decoded tables. djultra64."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def records(data):
    # Native 72FE8 parser publishes actor+1E8/+1EC from CharDef+18/+1C.
    count, start = struct.unpack_from('>II', data, 0x18)
    if not 0 < count <= 1024 or start < 48:
        raise RuntimeError('Invalid native CharDef command section')
    pos, enabled = start, 0
    for _ in range(count):
        if pos + 48 > len(data):
            raise RuntimeError('Truncated native record header')
        size = struct.unpack_from('>I', data, pos)[0]
        motions, motion_offset, variants, pattern_offset = struct.unpack_from('>hHhH', data, pos + 40)
        if not (48 <= size and pos + size <= len(data) and data[pos + 6] <= 3
                and 0 <= motions and 0 <= variants
                and (variants == 0 or pattern_offset >= 48 and pattern_offset + variants * 36 <= size)
                and (motions == 0 or motion_offset >= 48 and motion_offset + motions * 2 <= size)):
            raise RuntimeError('Malformed native command record')
        enabled += sum(struct.unpack_from('>H', data, pos + pattern_offset + n * 36)[0] != 0
                       for n in range(variants))
        pos += size
    return count, start, enabled


def hybrid_definitions(directory, sdk_root):
    from probe_training_states import functions
    from sc_import_assets import build
    # Importer verifies both authorized revisions and every unchanged base range.
    hybrid, _ = build((ROOT / 'probe-output/base.z64').read_bytes(),
                      (ROOT / 'probe-output/sc.z64').read_bytes())
    codec = directory / 'codec.cpp'
    codec.write_text('#include "recomp.h"\nextern "C" {\n' +
                     functions(ROOT / 'generated/guest')['func_800BD354'] + '\n}\n')
    decoder = directory / 'decode'
    subprocess.run(['c++', '-std=c++20', '-O2',
                    '-I' + str(sdk_root / 'deps/N64ModernRuntime/N64Recomp/include'),
                    str(ROOT / 'tests/sc_decode.cpp'), str(codec), '-o', str(decoder)], check=True)
    ids = (0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16)
    for ident in ids:
        start = struct.unpack_from('>I', hybrid, 0xD9580 + ident * 4)[0]
        end = struct.unpack_from('>I', hybrid, 0xD95C4 + ident * 4)[0]
        if not start < end <= len(hybrid):
            raise RuntimeError('Hybrid character bank bounds')
        block = hybrid[start:end]
        offset, size = struct.unpack_from('>II', block, 32)
        if offset + size > len(block):
            raise RuntimeError('Hybrid metadata packet bounds')
        packet, decoded = directory / f'fighter_{ident}.packet', directory / f'fighter_{ident}.decoded'
        packet.write_bytes(block[offset:offset + size])
        subprocess.run([str(decoder), str(packet), str(decoded)], check=True, capture_output=True)
        yield ident, decoded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--private', action='store_true')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='clay-training-moves-') as directory:
        directory = Path(directory)
        stub = directory / 'markers.cpp'
        stub.write_text('#include "native_prompts.h"\nnamespace clay::input {\n'
                        'std::string binding_marker(Action a){return "{ACTION:"+std::to_string(int(a))+"}";}\n}\n')
        binary = directory / 'moves'
        subprocess.run(['c++', '-std=c++20', '-O1', '-g', '-UNDEBUG', '-Wall', '-Wextra',
                        '-I' + str(ROOT / 'host'),
                        '-I' + str(args.sdk_root / 'deps/N64ModernRuntime/N64Recomp/include'),
                        str(ROOT / 'tests/training_moves.cpp'), str(ROOT / 'host/move_list.cpp'),
                        str(stub), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        if args.private:
            total_records = total_variants = fighters = 0
            for ident, path in hybrid_definitions(directory, args.sdk_root):
                count, start, enabled = records(path.read_bytes())
                print('Hybrid fighter ID', ident, flush=True)
                subprocess.run([str(binary), str(path), str(start), str(count), str(enabled)], check=True)
                fighters += 1
                total_records += count
                total_variants += enabled
            assert fighters == 16
            print(f'PASS all {fighters} hybrid fighters: {total_records} records, {total_variants} enabled variants')



if __name__ == '__main__':
    main()
