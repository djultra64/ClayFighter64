#!/usr/bin/env python3
"""Generate the original NTSC-U ClayFighter CPU functions. Author: djultra64."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM_OFFSET = 0x1000
VRAM = 0x80000400
TEXT_END = 0xD3520
LOAD_END = 0xF9980
EXPECTED_SHA = '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'


def write_changed(path, contents):
    if not path.exists() or path.read_text() != contents:
        path.write_text(contents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    sys.path.insert(0, str(sdk / 'deps/spimdisasm'))
    for packages in (sdk / 'analysis-venv/lib').glob('python*/site-packages'):
        sys.path.insert(0, str(packages))
    from spimdisasm import common, mips
    common.GlobalConfig.COMPILER = common.Compiler.IDO
    common.GlobalConfig.QUIET = True
    image = (ROOT / 'probe-output/base.z64').read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_SHA:
        raise ValueError('Unsupported ROM revision')
    unlocks = json.loads((ROOT / 'config/secret_unlocks.json').read_text())
    sc_hooks = json.loads((ROOT / 'config/sc_native_hooks.json').read_text())['hooks']
    roster_recipe=json.loads((ROOT/'config/sc_roster_hooks.json').read_text())
    roster_hooks=roster_recipe['verified_hooks']+[roster_recipe['selector_allocation']]
    for hook in roster_hooks:
        offset=int(hook['instruction_vram'],16)-VRAM+ROM_OFFSET
        assert image[offset:offset+4].hex()==hook['expected_rom_word']
    for hook in sc_hooks:
        offset = int(hook['instruction_vram'], 16) - VRAM + ROM_OFFSET
        assert image[offset:offset+4].hex() == hook['expected_rom_word']
    ctx = common.Context()
    ctx.changeGlobalSegmentRanges(0, len(image), 0x80000000, 0x80800000)
    matches = json.loads((ROOT / 'analysis/sdk_matches.json').read_text())
    named = {}
    for record in matches:
        if record['confidence'] == 'high' and len(record['addresses']) == 1:
            address = int(record['addresses'][0], 16)
            if address in named and named[address] != record['name']:
                raise ValueError(f'Ambiguous SDK mapping: {address:08X}')
            named[address] = record['name']
    native_sources = [sdk / f'deps/N64ModernRuntime/librecomp/src/{name}.cpp'
                      for name in ('ultra_translation', 'cont', 'vi')]
    native_sources += list((ROOT / 'host').glob('*.cpp'))
    supported = set()
    for source in native_sources:
        clean = re.sub(r'//[^\n]*', '', source.read_text())
        supported.update(re.findall(r'void\s+(\w+)_recomp\s*\(', clean))
    for address in [VRAM, 0x80000450, *named]:
        sym = ctx.globalSegment.addFunction(address,
                         vromAddress=address - VRAM + ROM_OFFSET)
        sym.name = f'func_{address:08X}'
        sym.isUserDeclared = True
    text = mips.sections.SectionText(ctx, ROM_OFFSET, TEXT_END, VRAM,
                                    'main', image, 0, None)
    text.analyze()
    functions = []
    bindings = {}
    for function in text.symbolList:
        address = function.vram
        name = f'func_{address:08X}'
        if address in named and named[address] in supported:
            name = named[address] + '_recomp'
            bindings[name] = dict(address=address, original=named[address])
        else:
            privileged = any(((w >> 26) == 0x10 and
                              ((w >> 21) & 31) not in (0, 4)) or
                             ((w >> 26) == 0x10 and ((w >> 21) & 31) == 4 and
                              ((w >> 11) & 31) != 12) or
                             (i.readsRs() and ((w >> 21) & 31) in (26, 27)) or
                             (i.readsRt() and ((w >> 16) & 31) in (26, 27)) or
                             i.getDestinationGpr() in (26, 27)
                             for i in function.instructions for w in [i.getRaw()])
            if privileged or (address in named and named[address].startswith(('os', '__os'))):
                name = f'cf_unimplemented_{address:08X}'
                bindings[name] = dict(address=address,
                                     original=named.get(address, 'unknown_privileged'))
        functions.append(dict(name=name, vram=address, size=function.sizew*4))
    assert any(f['vram'] == 0x80000450 for f in functions)
    assert all(ROM_OFFSET <= f['vram'] - VRAM + ROM_OFFSET < TEXT_END for f in functions)
    out = ROOT / 'generated'
    guest = out / 'guest'
    stage = out / 'cpu-stage'
    guest.mkdir(parents=True, exist_ok=True)
    stage.mkdir(exist_ok=True)
    lines = ['[[section]]', 'name = "main"', f'rom = 0x{ROM_OFFSET:X}',
             f'vram = 0x{VRAM:X}', f'size = 0x{LOAD_END - ROM_OFFSET:X}',
             'functions = [']
    lines += [f' {{ name = "{f["name"]}", vram = 0x{f["vram"]:X}, size = 0x{f["size"]:X} }},'
              for f in functions]
    (out / 'symbols.toml').write_text('\n'.join(lines + [']']) + '\n')
    (out / 'functions.json').write_text(json.dumps(functions, indent=2) + '\n')
    (guest / 'bindings.json').write_text(json.dumps(bindings, indent=2) + '\n')
    (out / 'cpu.toml').write_text('[input]\n'
        'symbols_file_path = "symbols.toml"\n'
        f'rom_file_path = {json.dumps(str(ROOT / "probe-output/base.z64"))}\n'
        'output_func_path = "cpu-stage"\nfunctions_per_output_file = 30\n'
        'uses_mips3_float_mode = false\n[patches]\nignored = '
        + json.dumps(sorted(bindings)) + '\n')
    # A larger previous translation can leave surplus files after SDK binding
    # changes; never compile stale guest bodies alongside the new headers.
    for stale in stage.glob('funcs_*.c'):
        stale.unlink()
    result = subprocess.run([str(sdk / 'toolchain/n64recomp-build/N64Recomp'),
                             str(out / 'cpu.toml')], capture_output=True, text=True)
    (out / 'recompile.log').write_text(result.stdout + result.stderr)
    if result.returncode:
        print((result.stdout + result.stderr)[-6000:])
        raise SystemExit(result.returncode)
    # C++ forbids jumping over initialized locals; hoist recompiler jump temps.
    body_pattern = re.compile(r'RECOMP_FUNC void [^(]+\([^)]*\) \{.*?^;\}', re.M | re.S)
    temporary = re.compile(r'^(\s*)gpr (jr_addend_[0-9A-F]{8}) = ([^;]+);$', re.M)
    def hoist(match):
        body = match.group(0)
        names = list(dict.fromkeys(m[2] for m in temporary.finditer(body)))
        body = temporary.sub(lambda m: f'{m[1]}{m[2]} = {m[3]};', body)
        at = body.index('\n') + 1
        return body[:at] + ''.join(f'    gpr {name} = 0;\n' for name in names) + body[at:]
    def optional_sc_hooks(match):
        body = match.group(0)
        name = re.match(r'RECOMP_FUNC void ([^(]+)\(', body)[1]
        for hook in sc_hooks:
            if hook['function'] != name:
                continue
            if 'old_statement' in hook:
                assert body.count(hook['old_statement']) == 1
                body = body.replace(hook['old_statement'], hook['new_statement'])
            elif name == 'func_80001008':
                assert body.count('    return;') == 1
                body = body.replace('    return;', '    clay_sc_register_model(rdram, ctx->r2);\n    return;')
            else:
                call = {'func_80001AA8': 'clay_sc_unregister_model(rdram, ctx->r4);',
                        'func_8003C740': 'clay_sc_demo_combat_selection(rdram, ctx->r4);',
                        'func_8005D334': 'clay_sc_demo_selection(rdram);'}[name]
                at = body.index('\n') + 1
                body = body[:at] + '    ' + call + '\n' + body[at:]
        return body
    def mainmenu_hooks(match):
        body=match.group(0)
        name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        if name=='func_80066B6C':
            assert image[0x6776C:0x67770].hex()=='3c0e800e'
            at=body.index('\n')+1
            body=body[:at]+'    clay::main_menu::prepare(rdram);\n'+body[at:]
        if name in ('func_80066DC0','func_8006B014'):
            # Retarget only the original array-base construction. The helper
            # returns original tables for every non-main menu and transition.
            pattern=r'(    ctx->r(\d+) = ADD32\(ctx->r\d+, -(?:0X51FC|0X4DD4|0X4EA8)\);)'
            body,count=re.subn(pattern,lambda m:m[1]+f'\n    ctx->r{m[2]} = S32(clay::main_menu::table(rdram,uint32_t(ctx->r{m[2]})));',body)
            assert count>=3,(name,count)
        if name=='func_80066DC0':
            assert image[0x67A48:0x67A4C].hex()=='0c016c7a'
            assert body.count('    clay_native_options_border(rdram,ctx);')==1
            body=body.replace('    clay_native_options_border(rdram,ctx);','    clay::main_menu::draw_border(rdram,ctx,&clay_native_options_border);')
        if name=='func_80067980':
            offset=0x8006798C-VRAM+ROM_OFFSET;assert image[offset:offset+4].hex()=='0c02832a'
            call='    func_800A0CA8(rdram, ctx);';assert body.count(call)==1
            body=body.replace(call,call+'\n    clay::main_menu::observe_input(rdram);')
            assert image[0x68DD4:0x68DD8].hex()=='3c0c800e'
            assert body.count('L_800681D4:')==1
            body=body.replace('L_800681D4:','L_800681D4:\n    if(clay::main_menu::confirm_exit(rdram))goto L_800684F4;')
            body=body.replace('    return;','    clay::main_menu::restore_selection(rdram);\n    return;')
        return body
    def native_prompt_hooks(match):
        body = match.group(0)
        name = re.match(r'RECOMP_FUNC void ([^(]+)\(', body)[1]
        prompts = {'func_80071738': (0x72338, '27bdffd8', 'prepare_native_prompt_measure'),
                   'func_8007191C': (0x7251C, '27bdffb8', 'prepare_native_prompt_draw')}
        if name in prompts:
            offset, expected, helper = prompts[name]
            assert image[offset:offset+4].hex() == expected
            at = body.index('\n') + 1
            body = body[:at] + f'    clay::input::{helper}(rdram, ctx);\n' + body[at:]
        cleanup={'func_80072C4C':(0x7384C,'27bdffd8'),'func_8007145C':(0x7205C,'27bdffe8')}
        if name in cleanup:
            offset,expected=cleanup[name];assert image[offset:offset+4].hex()==expected
            at=body.index('\n')+1;body=body[:at]+'    clay::input::restore_native_prompt_font(rdram,ctx->r4);\n'+body[at:]
        return body
    def input_mode_hooks(match):
        body=match.group(0)
        name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        if name=='func_8006FBD0':
            for address,expected in [(0x8006FEEC,'0c018db9'),(0x800705DC,'0c00f1c3'),(0x80070F80,'0c01b6b8')]:
                offset=address-VRAM+ROM_OFFSET;assert image[offset:offset+4].hex()==expected
            for old,new in [('func_800636E4','clay::training::select_fighters'),('func_8003C70C','clay::training::run_match')]:
                call=f'    {old}(rdram, ctx);'
                assert body.count(call)==1
                body=body.replace(call,f'    {new}(rdram,ctx,&{old});')
            call='    func_8006DAE0(rdram, ctx);'
            assert body.count(call)==1
            body=body.replace(call,call+'\n    clay::training::stage_panel_finished(rdram);\n    if(clay::training::take_stage_back()){clay::training::return_to_opponent_selection(rdram);goto L_8006FEE0;}')
            return body
        if name=='func_8006DAE0':
            offset=0x8006DB14-VRAM+ROM_OFFSET;assert image[offset:offset+4].hex()=='0c01b3ef'
            call='    func_8006CFBC(rdram, ctx);'
            assert body.count(call)==1
            return body.replace(call,call+'\n    clay::training::stage_panel_started(rdram);')
        if name=='func_8006D094':
            offset=0x8006D094-VRAM+ROM_OFFSET;assert image[offset:offset+4].hex()=='27bdff98'
            return body.replace('    return;','    clay::training::stage_panel_draw(rdram,ctx);\n    return;')
        if name=='func_8006D77C':
            assert image[0x8006D77C-VRAM+ROM_OFFSET:0x8006D77C-VRAM+ROM_OFFSET+4].hex()=='27bdffd8'
            at=body.index('\n')+1;body=body[:at]+'    clay::training::stage_panel_tick(rdram);\n'+body[at:]
            call='    func_8006D544(rdram, ctx);'
            assert body.count(call)==1
            return body.replace(call,'    clay::training::stage_input(rdram,ctx,&func_8006D544);')
        if name=='func_8006D544':
            assert image[0x8006D54C-VRAM+ROM_OFFSET:0x8006D54C-VRAM+ROM_OFFSET+4].hex()=='0c02832a'
            call='    func_800A0CA8(rdram, ctx);'
            assert body.count(call)==1
            return body.replace(call,call+'\n    clay::training::stage_input_polled(rdram);')
        if name=='func_80052DC4':
            address=0x800545F4;offset=address-VRAM+ROM_OFFSET
            assert image[offset:offset+16].hex()=='3c1880108f1828c044809000c7100034'
            at=body.index('    // 0x800545F4:')
            return body[:at]+'    if(clay::training::snapshot().active) goto L_80054890;\n'+body[at:]
        if name=='func_8003D940':
            call='    func_80059B68(rdram, ctx);'
            assert body.count(call)==1
            body=body.replace(call,'    if(!clay::training::draw_pause(rdram,ctx)) func_80059B68(rdram,ctx);')
            marker='    // 0x8003E9C0:'
            start=body.index(marker);at=body.index('    func_800588A4(rdram, ctx);',start)+len('    func_800588A4(rdram, ctx);')
            return body[:at]+'\n    clay::training::draw_hud(rdram,ctx);'+body[at:]
        if name=='func_8005FC68':
            offset=0x8005FC70-VRAM+ROM_OFFSET;assert image[offset:offset+4].hex()=='0c02832a'
            call='    func_800A0CA8(rdram, ctx);'
            assert body.count(call)==1
            return body.replace(call,call+'\n    clay::training::selection_tick(rdram);')
        if name=='func_80088B88':
            assert image[0x80088B88-VRAM+ROM_OFFSET:0x80088B88-VRAM+ROM_OFFSET+4].hex()=='27bdffe8'
            at=body.index('\n')+1
            return body[:at]+'    clay::training::prepare_input(rdram,ctx);\n'+body[at:]
        if name=='func_8007C338':
            address=0x8007C830;offset=address-VRAM+ROM_OFFSET
            assert image[offset:offset+4].hex()=='ad6e02c0'
            marker='    // 0x8007C830:';at=body.index(marker)
            return body[:at]+'    ctx->r14 = S32(clay::training::damage_applied(rdram,uint32_t(ctx->r11),uint32_t(ctx->r8),int32_t(ctx->r14),MEM_W(0x8C,ctx->r29)==0));\n'+body[at:]
        hooks={'func_8003C740':('27bdffc0',None,'clay::input::end_combat_input();'),
               'func_8003ED2C':('27bdff70','clay::input::update_combat_input(rdram);','clay::input::update_combat_input(rdram);'),
               'func_80087158':('27bdff98','clay::training::observe_world(rdram,uint32_t(ctx->r4),ctx);',None)}
        if name not in hooks:return body
        expected,enter,leave=hooks[name];address=int(name[-8:],16)
        assert image[address-VRAM+ROM_OFFSET:address-VRAM+ROM_OFFSET+4].hex()==expected
        if enter:
            at=body.index('\n')+1;body=body[:at]+'    '+enter+'\n'+body[at:]
        if leave:body=body.replace('    return;','    '+leave+'\n    return;')
        if name=='func_8003C740':
            assert image[0x3E218:0x3E21C].hex()=='15c0001d'
            marker='    // 0x8003D618: bne';assert body.count(marker)==1;at=body.index(marker)
            body=body[:at]+'    clay::input::begin_combat_input(rdram);\n'+body[at:]
        if name=='func_8003ED2C':
            assert body.count('L_8003F0C0:')==1
            body=body.replace('L_8003F0C0:','L_8003F0C0:\n    if(clay::training::pause_tick(rdram,ctx)) goto L_8003F6AC;')
            assert image[0x3F944:0x3F948].hex()=='3c048010'
            assert body.count('L_8003ED44:')==1
            body=body.replace('L_8003ED44:','L_8003ED44:\n    clay::input::update_combat_input(rdram);')
        return body
    def branding_hooks(match):
        body=match.group(0)
        name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        calls={'func_8005CAC0':[0x8005D250],
               'func_80064F78':[0x8006516C,0x8006551C]}.get(name,[])
        for address in calls:
            assert struct.unpack_from('>I',image,address-VRAM+ROM_OFFSET)[0]==0x0C01708D
            marker=f'    // 0x{address:08X}: jal';start=body.index(marker)
            at=body.index('    func_8005C234(rdram, ctx);',start)
            body=body[:at]+body[at:].replace('    func_8005C234(rdram, ctx);','    clay_logo64_draw(rdram, ctx);',1)
        return body
    def selector_preview_hooks(match):
        body=match.group(0)
        if not body.startswith('RECOMP_FUNC void func_8005E37C('):return body
        for address,right,sprite in [(0x8005F4FC,False,False),(0x8005F8C0,True,False),(0x8005F6B0,False,True),(0x8005FA78,True,True)]:
            function='func_800A6520' if sprite else 'func_8005DB24'
            offset=address-VRAM+ROM_OFFSET;expected=0x0C000000|((int(function[-8:],16)>>2)&0x03FFFFFF)
            assert struct.unpack_from('>I',image,offset)[0]==expected
            marker=f'    // 0x{address:08X}: jal';start=body.index(marker);end=body.index('    '+function+'(rdram, ctx);',start)
            needle='    '+function+'(rdram, ctx);';replacement=f'    clay::selector::draw_preview(rdram,ctx,{str(right).lower()},{str(sprite).lower()},&{function});'
            body=body[:end]+body[end:].replace(needle,replacement,1)
        for address,right in [(0x8005E448,'MEM_W(0x40,ctx->r29)>=2'),(0x8005E584,'MEM_W(0x40,ctx->r29)>=2'),(0x8005E5C8,'MEM_W(0x40,ctx->r29)>=2'),(0x8005E9AC,'false'),(0x8005EB24,'true')]:
            name_call=address in (0x8005E9AC,0x8005EB24)
            function='func_800A6520' if name_call else 'func_800B09DC'
            expected=0x0C000000|((int(function[-8:],16)>>2)&0x03FFFFFF)
            assert struct.unpack_from('>I',image,address-VRAM+ROM_OFFSET)[0]==expected
            marker=f'    // 0x{address:08X}: jal';start=body.index(marker)
            at=body.index('    '+function+'(rdram, ctx);',start)
            helper='draw_name' if name_call else 'translate_panel' if address in (0x8005E584,0x8005E5C8) else 'translate_mechanism'
            if helper=='translate_panel':right='unsigned(MEM_W(0x40,ctx->r29))'
            needle='    '+function+'(rdram, ctx);'
            body=body[:at]+body[at:].replace(needle,f'    clay::selector::{helper}(rdram,ctx,{right},&{function});',1)
        return body
    def port_options_hooks(match):
        body=match.group(0)
        name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        if name=='func_80066DC0':
            assert image[0x679C0:0x679C4].hex()=='27bdff50'
            assert struct.unpack_from('>I',image,0x80066E48-VRAM+ROM_OFFSET)[0]==0x0C016C7A
            at=body.index('    func_8005B1E8(rdram, ctx);',body.index('    // 0x80066E48: jal'))
            body=body[:at]+body[at:].replace('    func_8005B1E8(rdram, ctx);','    clay_native_options_border(rdram,ctx);',1)
            at=body.index('    // 0x80066E68:');body=body[:at]+'    if(clay::port::draw_native_options(rdram,ctx))goto L_8006796C;\n'+body[at:]
        for pc in [0x8006FBE4,0x800705F0,0x80070AA8,0x80070C20]:
            marker=f'    // 0x{pc:08X}: jal'
            if marker in body:
                at=body.index('    func_8009DB18(rdram, ctx);',body.index(marker))
                body=body[:at]+'    ctx->r4=0x3000; ctx->r6=0x1000; // Native Options pool for full two-column font.\n'+body[at:]
        if name=='func_800B3220':
            assert struct.unpack_from('>I',image,0x800B32AC-VRAM+ROM_OFFSET)[0]==0x0C02CD31
            at=body.index('    // 0x800B32B4:');body=body[:at]+'    clay::extend_native_main_heap(rdram,ctx);\n'+body[at:]
        if name=='func_80067980':
            assert image[0x68594:0x68598].hex()=='3c0e8012'
            marker='    // 0x80067994:';at=body.index(marker)
            body=body[:at]+'    if(clay::port::update_native_options(rdram,ctx))goto L_8006B004;\n'+body[at:]
        if name=='func_8006B014':
            assert image[0x6BC14:0x6BC18].hex()=='27bdffa8'
            at=body.index('\n')+1;body=body[:at]+'    clay::port::prepare_native_options_labels(rdram);\n'+body[at:]
        if name=='func_8006D094':
            for pc in [0x8006D208,0x8006D27C]:
                assert struct.unpack_from('>I',image,pc-VRAM+ROM_OFFSET)[0]==0x0C0176C9
                at=body.index('    func_8005DB24(rdram, ctx);',body.index(f'    // 0x{pc:08X}: jal'))
                body=body[:at]+body[at:].replace('    func_8005DB24(rdram, ctx);','    clay::ui::draw_vs_actor(rdram,ctx,&func_8005DB24);',1)
        return body
    def roster_native_hooks(match):
        body=match.group(0);name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        for hook in roster_hooks:
            if hook['function']!=name:continue
            address=int(hook['instruction_vram'],16)
            pattern=re.compile(r'(    // 0x'+f'{address:08X}'+r':[^\n]*\n)(    [^\n]+\n)')
            body,count=pattern.subn(lambda m:m[1]+'    '+hook['new_statement']+'\n',body)
            assert count>=1,(name,hex(address))
        if name=='func_8005DDCC':
            assert body.count('    return;')==1
            body=body.replace('    return;','    clay_sc_roster_install(rdram,ctx);\n    return;')
        if name=='func_800AFFE0':
            at=body.index('\n')+1;body=body[:at]+'    const gpr portrait_source=ctx->r4;\n'+body[at:]
            body=body.replace('    return;','    clay_sc_roster_augment_portraits(rdram,ctx,portrait_source);\n    return;')
        if name=='func_8005D484':
            assert image[0x5E084:0x5E088].hex()=='28810011'
            at=body.index('\n')+1;body=body[:at]+'    if(clay_sc_roster_extra_allowed(ctx->r4)){\n        ctx->r2=ctx->r4;\n        return;\n    }\n'+body[at:]
        if name=='func_8005E37C':
            assert image[0x5F288:0x5F28C].hex()=='0c028c30'
            start=body.index('    // 0x8005E688: jal');at=body.index('    func_800A30C0(rdram, ctx);',start)
            body=body[:at]+body[at:].replace('    func_800A30C0(rdram, ctx);','    clay_sc_roster_draw_grid(rdram,ctx);',1)
        if name=='func_8005E2CC':
            assert image[0x5EECC:0x5EED0].hex()=='3c0e800e'
            at=body.index('\n')+1;body=body[:at]+'    clay_sc_roster_release_grid(rdram,ctx);\n'+body[at:]
        return body
    def actor_interpolation_hooks(match):
        body=match.group(0);name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        if name in ('func_8009DB18','func_8003C740'):
            pc=int(name[-8:],16);expected='27bdffe0' if name=='func_8009DB18' else '27bdffc0'
            assert image[pc-VRAM+ROM_OFFSET:pc-VRAM+ROM_OFFSET+4].hex()==expected
            call='clay::reserve_actor_interpolation_pool(ctx);' if name=='func_8009DB18' else 'clay::reset_actor_interpolation();'
            at=body.index('\n')+1;body=body[:at]+'    '+call+'\n'+body[at:]
        if name=='func_80052DC4':
            address=0x800545F4;offset=address-VRAM+ROM_OFFSET
            assert image[offset:offset+16].hex()=='3c1880108f1828c044809000c7100034'
            at=body.index('    // 0x800545F4:')
            return body[:at]+'    if(clay::training::snapshot().active) goto L_80054890;\n'+body[at:]
        if name=='func_8003D940':
            assert image[0x3EE0C:0x3EE10].hex()=='3c048011'
            at=body.index('    // 0x8003E20C:');body=body[:at]+'    clay::begin_actor_interpolation_frame(rdram);\n'+body[at:]
        if name=='func_800438D8':
            assert image[0x4551C:0x45520].hex()=='0c02a1f1'
            at=body.index('    // 0x8004491C:');at=body.index('    func_800A87C4(rdram, ctx);',at)
            body=body[:at]+'    clay::begin_actor_piece(rdram,ctx);\n'+body[at:]
            assert image[0x45524:0x45528].hex()=='8faa00fc'
            at=body.index('    // 0x80044924:');body=body[:at]+'    clay::end_actor_piece(rdram);\n'+body[at:]
        return body
    def title_audio_hooks(match):
        body=match.group(0)
        if body.startswith('RECOMP_FUNC void func_8009B6B4('):
            at=body.index('\n')+1
            body=body[:at]+'    clay::audio::trace_native_sound(rdram,ctx);\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_800657DC('):
            for pc,wrapper in [(0x80065D74,'play_title_announcement'),(0x80066488,'finish_title_announcement')]:
                off=pc-VRAM+ROM_OFFSET
                assert image[off:off+4].hex()=='0c026dad'
                at=body.index('    // '+hex(pc).upper().replace('0X','0x')+': jal')
                end=body.index('    func_8009B6B4(rdram, ctx);',at)
                body=body[:end]+body[end:].replace('    func_8009B6B4(rdram, ctx);','    clay::audio::'+wrapper+'(rdram,ctx,&func_8009B6B4);',1)
        return body
    def native_bio_hooks(match):
        body=match.group(0)
        if body.startswith('RECOMP_FUNC void func_8006CC38('):
            off=0x8006CC38-VRAM+ROM_OFFSET
            assert image[off:off+4].hex()=='27bdffd8'
            at=body.index('\n')+1
            body=body[:at]+'    clay::bios::install(rdram, ctx);\n'+body[at:]
        return body
    def visual_bug_hooks(match):
        body=match.group(0);name=re.match(r'RECOMP_FUNC void ([^(]+)\(',body)[1]
        if name in ('func_800A87C4','func_800A6520'):
            pc=int(name[-8:],16);assert image[pc-VRAM+ROM_OFFSET:pc-VRAM+ROM_OFFSET+4].hex().startswith('27bd')
            at=body.index('\n')+1;body=body[:at]+'    clay::begin_sprite_slices(rdram,ctx);\n'+body[at:]
            label,offset=('L_800A94C4:',0x18c) if name=='func_800A87C4' else ('L_800A75E8:',0x154)
            assert body.count(label)==1
            body=body.replace(label,label+'\n    clay::limit_sprite_slice_rows(rdram,ctx,'+str(offset)+');')
            body=body.replace('    return;','    clay::end_sprite_slices(rdram);\n    return;')
        if name=='func_800A45BC':
            at=body.index('\n')+1;body=body[:at]+'    const uint32_t full_scissor_begin=clay::ui::display_list_cursor(rdram);\n'+body[at:]
            body=body.replace('    return;','    clay::ui::fix_scissor(rdram,full_scissor_begin);\n    return;')
        if name=='func_800396C4':
            at=body.index('\n')+1;body=body[:at]+'    const uint32_t wide_effect=uint32_t(ctx->r4),wide_vertices=clay::ui::vertex_cursor(rdram);\n'+body[at:]
            body=body.replace('    return;','    clay::ui::extend_circle_mask(rdram,wide_effect,wide_vertices);\n    return;')
            for pc in ('0x80039B04','0x80039B7C'):
                at=body.index('    // '+pc+': jal');end=body.index('    func_800A4BF0(rdram, ctx);',at)
                body=body[:end]+body[end:].replace('    func_800A4BF0(rdram, ctx);','    clay::ui::draw_transition_band(rdram,ctx,&func_800A4BF0);',1)
        if name=='func_8005E37C':
            at=body.index('    clay_sc_roster_draw_grid(rdram,ctx);')
            body=body[:at]+'    clay::ui::extend_selector_background_mesh(rdram,uint32_t(ctx->r5));\n'+body[at:]
        for function,pc in [('func_80059B68','0x80059CF0'),('func_800396C4','0x8003983C')]:
            if name!=function:continue
            at=body.index('    // '+pc+': jal');end=body.index('    func_800A4D9C(rdram, ctx);',at)
            body=body[:end]+body[end:].replace('    func_800A4D9C(rdram, ctx);','    clay::ui::draw_fullscreen_fill(rdram,ctx,&func_800A4D9C);',1)
        return body
    for path in stage.glob('funcs_*.c'):
        contents = body_pattern.sub(hoist, path.read_text())
        contents = body_pattern.sub(optional_sc_hooks, contents)
        contents = body_pattern.sub(native_prompt_hooks, contents)
        contents = body_pattern.sub(input_mode_hooks, contents)
        contents = body_pattern.sub(selector_preview_hooks, contents)
        contents = body_pattern.sub(branding_hooks, contents)
        contents = body_pattern.sub(port_options_hooks, contents)
        contents = body_pattern.sub(mainmenu_hooks, contents)
        contents = body_pattern.sub(roster_native_hooks, contents)
        contents = body_pattern.sub(actor_interpolation_hooks, contents)
        contents = body_pattern.sub(visual_bug_hooks, contents)
        contents = body_pattern.sub(title_audio_hooks, contents)
        contents = body_pattern.sub(native_bio_hooks, contents)
        unlock = unlocks['native_initializer_patch']
        if unlocks['enabled_by_default'] and ('RECOMP_FUNC void ' + unlock['function'] + '(') in contents:
            offset = int(unlock['instruction_rom_offset'], 16)
            assert image[offset:offset+4].hex() == unlock['expected_instruction']
            needle = unlock['expected_generated_statement']
            assert contents.count(needle) == 1
            contents = contents.replace(needle, unlock['replacement_generated_statement'])
        if 'RECOMP_FUNC void func_8009B514(' in contents:
            # Original stop/restart-sequence waits for two audio ticks. N64
            # interrupt preemption must be made cooperative on the host.
            for address, expected in ((0x8009B618, '3c08800e'),
                                      (0x8009B674, '3c18800e')):
                offset = address - VRAM + ROM_OFFSET
                assert image[offset:offset+4].hex() == expected
                label = f'L_{address:08X}:'
                assert contents.count(label) == 1
                contents = contents.replace(label, label + '\n    yield_self_1ms(rdram);')
        if 'RECOMP_FUNC void func_8009C660(' in contents:
            # The game's audio queue reads AI_LEN directly, bypassing libultra.
            # Replace that exact hardware-register load with the host queue.
            assert image[0x9D2C4:0x9D2C8].hex() == '8d6c0004'
            needle = '    ctx->r12 = MEM_W(ctx->r11, 0X4);'
            assert contents.count(needle) == 1
            contents = contents.replace(needle, '    ctx->r12 = S32(clay_ai_length());')
        if 'RECOMP_FUNC void func_8009AB5C(' in contents:
            for address, expected in ((0x8009B20C, '3c0a800e'),
                                      (0x8009B268, '3c18800e'),
                                      (0x8009B2C4, '3c08800e')):
                offset = address - VRAM + ROM_OFFSET
                assert image[offset:offset+4].hex() == expected
                label = f'L_{address:08X}:'
                assert contents.count(label) == 1
                contents = contents.replace(label, label + '\n    yield_self_1ms(rdram);')
        if 'RECOMP_FUNC void func_8003D940(' in contents:
            # Stage/scene cleanup waits until queued VI framebuffers are
            # presented. On N64 the VI interrupt preempts this predicate loop;
            # the cooperative host must let the original VI worker run.
            # Native regression uses original predicate, VI worker and SDK
            # scheduler with the captured pending states [2, 2, 3].
            assert image[0x3E6EC:0x3E6F0].hex() == '0c026285'
            label = 'L_8003DAEC:'
            assert contents.count(label) == 1
            contents = contents.replace(label, label + '\n    yield_self_1ms(rdram);')
        path.write_text(contents)
    for path in stage.iterdir():
        if path.is_file():
            write_changed(guest / path.name, path.read_text())
    for stale in guest.glob('funcs_*.c'):
        if not (stage / stale.name).is_file():
            stale.unlink()
    write_changed(guest / 'dispatch.inc', '\n'.join(
        f'case 0x{f["vram"]:08X}u: return {f["name"]};' for f in functions) + '\n')
    write_changed(guest / 'bindings.h', '#pragma once\n#include "recomp.h"\n' + '\n'.join(
        f'extern "C" void {name}(uint8_t*, recomp_context*);' for name in bindings) + '\n')
    write_changed(out / 'fallbacks.cpp', '#include "host.h"\nextern "C" {\n' +
        '\n'.join(f'void {name}(uint8_t*, recomp_context*) '
                  f'{{clay_fatal("{item["original"]}",0x{item["address"]:08X});}}'
                  for name, item in bindings.items()
                  if name.startswith('cf_unimplemented_')) + '\n}\n')
    print(f'{len(functions)} CPU functions; {len(bindings)} native runtime bindings')


if __name__ == '__main__':
    main()
