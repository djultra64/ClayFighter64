#!/usr/bin/env python3
"""Native selector initialization/mode gates and confirmation regression. djultra64."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
from probe_training_states import ROOT, functions, guard_rom


def section(body, first, stop):
    label = re.search(r'^L_' + f'{first:08X}' + r':$', body, re.M)
    start = label.start() if label else body.index(f'    // 0x{first:08X}:')
    end = body.index(f'    // 0x{stop:08X}:', start)
    trailing = re.search(r'L_' + f'{stop:08X}' + r':\s*$', body[start:end])
    if trailing:
        end = start + trailing.start()
    return body[start:end]


def generate():
    guard_rom((ROOT / 'probe-output/base.z64').read_bytes())
    funcs = functions(ROOT / 'generated/guest')
    source = '#include "recomp.h"\nextern "C" {\n'
    source += ('void osStartThread_recomp(uint8_t*,recomp_context*);\n'
               'void func_8005D334(uint8_t*,recomp_context*);\n'
               'void func_8005D3D4(uint8_t*,recomp_context*);\n'
               'void func_8009B6B4(uint8_t*,recomp_context*);\n'
               'int clay_sc_roster_columns();\n')
    source += 'void selector_native_init(uint8_t* rdram,recomp_context* ctx){\n'
    source += section(funcs['func_8005DDCC'], 0x8005E0D0, 0x8005E0D4)
    source += section(funcs['func_8005DDCC'], 0x8005E0D8, 0x8005E0DC)
    source += section(funcs['func_8005DDCC'], 0x8005E0E0, 0x8005E100)
    source += section(funcs['func_800636E4'], 0x800636EC, 0x800636F8)
    source += section(funcs['func_800636E4'], 0x80063708, 0x80063724)
    source += section(funcs['func_800636E4'], 0x800638D8, 0x80063A8C)
    source += '\nL_80063A8C: return;\n}\n'
    source += 'void selector_native_p2_gate(uint8_t* rdram,recomp_context* ctx){\n'
    source += section(funcs['func_8005FC68'], 0x800608E0, 0x80060940)
    source += ('\nctx->r2=1; return;\nL_800613CC: ctx->r2=2; return;\n'
               'L_80061294: ctx->r2=3; return;\nL_80061464: ctx->r2=0;return;\n}\n')
    for name, start, stop, exits in [
        ('p1_confirm', 0x8006047C, 0x8006057C, [0x800608E0, 0x800606B0]),
        ('p2_confirm', 0x80061000, 0x80061100, [0x80061464, 0x80061234])]:
        source += f'void selector_native_{name}(uint8_t* rdram,recomp_context* ctx){{ bool c1cs=false;\n'
        source += section(funcs['func_8005FC68'], start, stop)
        source += '\nreturn;\n' + '\n'.join(f'L_{address:08X}: return;' for address in exits) + '\n}\n'
    # Run the concrete argument setup from the real Training bridge against native branches.
    host = (ROOT / 'host/training_guest.cpp').read_text()
    start = host.index('    ctx->r4=', host.index('void select_fighters('))
    stop = host.index('    original(rdram,ctx);', start) + len('    original(rdram,ctx);')
    source += 'void selector_training_init(uint8_t* rdram,recomp_context* ctx){\n'
    source += 'bool opponent_first=false,unlock_first=false;\nrecomp_func_t* original=&selector_native_init;\n' + host[start:stop] + '\n}\n}\n'
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--prepare-only', type=Path)
    args = parser.parse_args()
    source = generate()
    if args.prepare_only:
        args.prepare_only.parent.mkdir(parents=True, exist_ok=True)
        args.prepare_only.write_text(source)
        return
    with tempfile.TemporaryDirectory(prefix='clay-training-selector-') as directory:
        directory = Path(directory)
        guest = directory / 'guest.cpp'
        guest.write_text(source)
        binary = directory / 'selector'
        subprocess.run(['c++', '-std=c++20', '-O1', '-UNDEBUG', '-I' + str(ROOT / 'host'),
                        '-I' + str(args.sdk_root / 'deps/N64ModernRuntime/N64Recomp/include'),
                        str(ROOT / 'tests/training_selector.cpp'), str(guest), '-o', str(binary)], check=True)
        red = subprocess.run([str(binary), 'single-player'])
        if red.returncode != 14:
            raise RuntimeError('Original single-player mode must block interactive P2')
        subprocess.run([str(binary)], check=True)
    # Check the actual virtual port routing and announcement, without a duplicate implementation.
    host = (ROOT / 'host/main.cpp').read_text()
    routing = host[host.index('static bool get_input('):host.index('RspExitReason clayfighter_audio_rsp')]
    for required in ['rival&&port==1?0:port', '(rival&&port==0)',
                     'port==1&&clay::main_menu::training_requested()',
                     'selection_input_ready(input_states[0].buttons)']:
        if required not in routing:
            raise RuntimeError('Virtual P2 routing contract changed: ' + required)
    print('PASS source contract: virtual P2 connection, P1 routing, P1 suppression and release gate')


if __name__ == '__main__':
    main()
