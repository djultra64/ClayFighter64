#!/usr/bin/env python3
"""Recompile ClayFighter 63 1/3's own audio microcode. Project work: djultra64."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rsp-recomp', type=Path, required=True)
args = parser.parse_args()
rom = (ROOT / 'probe-output/base.z64').read_bytes()
text = rom[0xD35F0:0xD4410]
targets = list(struct.unpack('>16H', rom[0xF76D0:0xF76F0]))
expected = [0x1118, 0x1470, 0x11DC, 0x1B38, 0x1214, 0x187C, 0x1254, 0x12D0,
            0x12EC, 0x1328, 0x140C, 0x1294, 0x1E24, 0x138C, 0x170C, 0x144C]
if targets != expected or text[:8] != bytes.fromhex('2018036020170f90'):
    raise SystemExit('Unexpected ROM audio microcode; refusing mismatched generation')
subprocess.run([str(args.rsp_recomp.resolve()), str(ROOT / 'config/audio-rsp.toml')],
               cwd=ROOT, check=True)
output = ROOT / 'generated/audio.cpp'
evidence = {
    'project_work': 'djultra64',
    'source': 'Original ClayFighter 63 1/3 (USA) cartridge audio aspMain microcode',
    'rom_sha256': hashlib.sha256(rom).hexdigest(),
    'text_rom_offset': '0xD35F0', 'text_size': '0xE20', 'text_rsp_address': '0x1080',
    'text_cpu_address': '0x800D29F0',
    'text_sha256': hashlib.sha256(text).hexdigest(),
    'audio_data_rom_offset': '0xF76C0', 'audio_data_cpu_address': '0x800F6AC0',
    'command_table_rom_offset': '0xF76D0',
    'command_targets': [hex(x) for x in targets],
    'cpu_evidence': [
        '0x8009A7DC: addiu t5,t5,0x29F0 after lui t5,0x800D; stores audio OSTask.ucode',
        '0x8009A808: addiu t9,zero,0x1000; OSTask.ucode_size is DMA capacity, not text extent',
        '0x8009A830: addiu t4,t4,0x6AC0 after lui t4,0x800F; stores audio OSTask.ucode_data',
        '0x8009D6C0..0x8009D6D0 calculates boot length 0x800D29F0 - 0x800D2920 = 0xD0',
        'ROM 0xD4410 starts next graphics microcode; audio final jump to 0x1118 at ROM 0xD4404',
    ],
    'function': 'RspExitReason clayfighter_audio_rsp(uint8_t*, uint32_t)',
    'generated_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
    'generation_exit_code': 0,
}
(ROOT / 'analysis/audio-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
print(f'Generated {output} from original audio microcode ({len(text):#x} bytes)')
