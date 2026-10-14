#!/usr/bin/env python3
"""Private native Training state discovery and fixture extraction. djultra64.

Generated instruction listings and debugger recordings stay in analysis/training.
This tool observes guest state; it never patches life, timers or game behavior.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_ROM = '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'


def functions(directory):
    result = {}
    for path in sorted(directory.glob('funcs_*.c')):
        source = path.read_text()
        starts = list(re.finditer(r'^RECOMP_FUNC void (\w+)\([^\n]*\) \{', source, re.M))
        for i, start in enumerate(starts):
            end = starts[i + 1].start() if i + 1 < len(starts) else len(source)
            result[start[1]] = source[start.start():end].strip()
    return result


def excerpt(body, first, stop):
    # Start at the label, if present, so branches back to it remain valid.
    marker = f'    // 0x{first:08X}:'
    begin = body.index(marker)
    label = f'L_{first:08X}:\n'
    if body[max(0, begin-len(label)):begin] == label:
        begin -= len(label)
    end = body.index(f'    // 0x{stop:08X}:', begin)
    last_label = f'L_{stop:08X}:\n'
    if body[max(0, end-len(last_label)):end] == last_label:
        end -= len(last_label)
    return body[begin:end]


def guard_rom(image):
    if hashlib.sha256(image).hexdigest() != EXPECTED_ROM:
        raise ValueError('Unsupported original ROM revision for Training probes')


def write_gdb(path):
    # 87158 is called at 3F7E0 after input delivery and before actor updates.
    # Word-swapped RDRAM stores aligned u32/f32 in native byte order.
    path.write_text('''set pagination off
set breakpoint pending on
python
import gdb, json, struct
class TrainingProbe(gdb.Breakpoint):
    def __init__(self):
        super().__init__('func_80087158')
        self.previous = None
        self.ticks = 0
    def stop(self):
        self.ticks += 1
        try:
            frame = gdb.newest_frame()
            ram = int(frame.read_var('rdram'))
            ctx = frame.read_var('ctx').dereference()
            world = int(ctx['r4']) & 0xffffffff
            inferior = gdb.selected_inferior()
            def data(address):
                if address < 0x80000000 or address > 0x807ffffc or address % 4:
                    raise ValueError('invalid aligned guest pointer')
                return bytes(inferior.read_memory(ram + (address & 0x7fffff), 4))
            def word(address): return struct.unpack('=I', data(address))[0]
            def scalar(address): return struct.unpack('=f', data(address))[0]
            p1, p2 = word(world + 0x124), word(world + 0x128)
            event = {'event':'training_state_probe','tick':self.ticks,
                     'world':world, 'phase':word(world+0x50),
                     'timer':scalar(world+0x34),'dt':scalar(world+0x2c8c),
                     'final_reserve':word(world+0x30),
                     'pause':word(0x8010849c), 'attract':word(0x80106d58),
                     'actors':[{'ptr':p, 'hp':word(p+0x2c0),
                         'depleted':word(p+0x2c4), 'state':word(p+0x168),
                         'finish':word(p+0x318), 'control':word(p+0x260),
                         'input':word(p+0x258)} for p in (p1,p2)]}
            key = (event['phase'], event['pause'], event['final_reserve'],
                   tuple((a['hp'],a['depleted'],a['state'],a['finish']) for a in event['actors']))
            if key != self.previous or self.ticks % 60 == 0:
                print(json.dumps(event))
                self.previous = key
        except (gdb.error, ValueError) as error:
            print(json.dumps({'event':'training_probe_error','reason':str(error)}))
        return False
TrainingProbe()
end
run
quit
''')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/training')
    args = parser.parse_args()
    guard_rom((ROOT/'probe-output/base.z64').read_bytes())
    args.output.mkdir(parents=True, exist_ok=True)
    funcs = functions(ROOT/'generated/guest')
    names = ('8003C740', '8003D940', '8003ED2C', '80061474', '80067980',
             '80072D50', '8007396C', '8007C338', '80085704', '80087158')
    for address in names:
        body = funcs[f'func_{address}']
        instructions = list(dict.fromkeys(line.strip() for line in body.splitlines()
                                         if '// 0x' in line))
        (args.output/f'func_{address}.asm').write_text('\n'.join(instructions)+'\n')
    write_gdb(args.output/'state-probe.gdb')
    print(json.dumps({'author':'djultra64', 'functions':len(names),
                     'output':str(args.output), 'scope':'Read-only discovery, no training implementation claim'}))


if __name__ == '__main__':
    main()
