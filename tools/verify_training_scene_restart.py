#!/usr/bin/env python3
"""Execute native worker creation/reuse branches around Training scene entry. djultra64."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from probe_training_states import ROOT, functions, guard_rom
from verify_training_selector import section


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    args = parser.parse_args()
    rom = (ROOT / 'probe-output/base.z64').read_bytes()
    guard_rom(rom)
    assert rom[0x3e218:0x3e21c].hex() == '15c0001d'
    body = functions(ROOT / 'generated/guest')['func_8003C740']
    callback = '    clay::input::begin_combat_input(rdram);\n'
    assert body.count(callback) == 1
    assert body.index(callback) < body.index('    // 0x8003D618:')
    native = section(body, 0x8003D604, 0x8003D6A4)
    prefix = r'''
#include "recomp.h"
#include <cstdio>
#include <vector>
static unsigned ready, starts, scenes;
static bool invalid;
namespace clay::input {void begin_combat_input(uint8_t*){++ready;}}
extern "C" {
void osCreateMesgQueue_recomp(uint8_t*,recomp_context*){}
void func_800B0240(uint8_t*,recomp_context* ctx){ctx->r2=7;}
void osCreateThread_recomp(uint8_t*,recomp_context*){}
void osStartThread_recomp(uint8_t*,recomp_context*){++starts;invalid|=ready!=1;}
void func_80098448(uint8_t*,recomp_context*){}
void func_8003D940(uint8_t*,recomp_context*){++scenes;invalid|=ready!=1;}
void scene_entry(uint8_t* rdram,recomp_context* ctx){
'''
    suffix = r'''
return;
}
}
int main(){
    std::vector<uint8_t> memory(0x800000);auto* rdram=memory.data();
    for(int reused=0;reused<2;++reused){
        ready=starts=scenes=0;invalid=false;
        MEM_W(0,S32(0x800dab68))=reused;
        recomp_context ctx{};ctx.r29=gpr(S32(0x807ff000));ctx.f_odd=&ctx.f0.u32h;
        scene_entry(rdram,&ctx);
        if(invalid||ready!=1||scenes!=1||starts!=unsigned(!reused))return 14;
        if(MEM_W(0,S32(0x800dab68))!=1||MEM_W(0,S32(0x800ff104))!=0)return 15;
    }
    puts("PASS native scene initialization before first worker start and reused-worker branch");
}
'''
    with tempfile.TemporaryDirectory(prefix='clay-training-scene-') as temporary:
        source = Path(temporary) / 'scene.cpp'
        binary = Path(temporary) / 'scene'
        command = ['c++', '-std=c++20', '-O1', '-UNDEBUG',
                   '-I' + str(args.sdk_root / 'deps/N64ModernRuntime/N64Recomp/include'),
                   str(source), '-o', str(binary)]
        source.write_text(prefix + native + suffix)
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)
        # The original worker-start-only hook cannot initialize a repeated scene.
        original = native.replace(callback, '')
        start = original.index('    osStartThread_recomp(rdram, ctx);')
        original = original[:start] + callback + original[start:]
        source.write_text(prefix + original + suffix)
        subprocess.run(command, check=True)
        red = subprocess.run([str(binary)])
        if red.returncode != 14:
            raise RuntimeError('Worker-start-only initialization must reproduce missing repeated Training setup')
        print('RED original worker-start-only callback skips reused scenes (exit 14)')


if __name__ == '__main__':
    main()
