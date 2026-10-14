#!/usr/bin/env python3
"""ClayFighter 64 1.0 two-ROM installer and asset extractor. Work by djultra64."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
BASE_SHA = '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'
SC_SHA = '9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f'
ASSET_SHA = '26ccae86b452d4bacadf429e0b241751aeef58a41d676e7d6173974414dafd7d'
BIO_SHA = 'd7f85d9be2dd4614923c1c0af0e6b3b27170f02e715edbebca2d4e4e7627bae3'
REQUIRED = ('ClayFighter 64 is a fusion of ClayFighter 63 1/3 and Sculptor\'s Cut.\n'
            'Installation requires BOTH of your own USA ROMs:\n'
            '  1. ClayFighter 63 1/3 (USA)\n'
            '  2. ClayFighter: Sculptor\'s Cut (USA)\n'
            'Supported formats: .z64, .v64, .n64, or ZIP containing exactly one ROM.\n'
            'No ROMs or extracted original game assets are included or downloaded.')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def load_rom(path):
    path = Path(path)
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            entries = [i for i in archive.infolist() if not i.is_dir() and
                       Path(i.filename).suffix.lower() in ('.z64', '.v64', '.n64')]
            if len(entries) != 1:
                raise ValueError('ZIP must contain exactly one ROM (.z64, .v64, .n64).')
            if entries[0].file_size > 32 * 1024 * 1024:
                raise ValueError('ROM is too large for a supported USA revision.')
            data = archive.read(entries[0])
    else:
        if path.stat().st_size > 32 * 1024 * 1024:
            raise ValueError('ROM is too large for a supported USA revision.')
        data = path.read_bytes()
    if len(data) < 4 or len(data) % 4:
        raise ValueError('Invalid N64 ROM size or header.')
    if data[:4] == bytes.fromhex('37804012'):
        data = bytes(v for pair in zip(data[1::2], data[0::2]) for v in pair)
    elif data[:4] == bytes.fromhex('40123780'):
        data = b''.join(data[i:i+4][::-1] for i in range(0, len(data), 4))
    if data[:4] != bytes.fromhex('80371240'):
        raise ValueError('Invalid N64 ROM header; select your own supported USA ROM.')
    return data

def build_bios(base, donor, font):
    u32 = lambda o: struct.unpack_from('>I', font, o)[0]
    chars = font[u32(0)-12:].split(b'\\', 1)[0].decode('ascii')
    width = {c: struct.unpack_from('>H', font, u32(i*8)-12+6)[0]
             for i, c in enumerate(chars, 1)}
    width[' '] = sum(width.values()) // len(width)
    advance = lambda text: sum(width[c.upper()] for c in text)
    if abs(struct.unpack_from('>f', base, 0xF2E14)[0] + .7) >= 1e-6:
        raise ValueError('Unsupported native biography scale.')
    entries = []
    for ident, offset in [(6,0xCA730), (11,0xCA0F0), (12,0xC9F68), (16,0xC9900)]:
        text = donor[offset:donor.index(0, offset)].decode('ascii')
        lines = []
        for paragraph in re.split(r'\r\n\s*\r\n', text.strip()):
            if lines: lines.append('')
            line = ''
            for word in paragraph.split():
                candidate = (line + ' ' + word).strip()
                if len(candidate) > 23 or advance(candidate) > 252:
                    if not line: raise ValueError('Biography word exceeds native line width.')
                    lines.append(line); line = word
                else: line = candidate
            if line: lines.append(line)
        if ' '.join(text.split()) != ' '.join(' '.join(lines).split()):
            raise ValueError('Biography extraction changed the word sequence.')
        payload = b''.join(s.encode('ascii') + b'\0' for s in lines)
        entries.append(struct.pack('<III', ident, len(lines), len(payload)) + payload)
    return b'CFB1' + b''.join(entries)

def install(root, paths):
    images = {}
    for path in paths:
        data = load_rom(path)
        digest = sha(data)
        if digest not in (BASE_SHA, SC_SHA):
            raise ValueError(f'Unsupported ROM: {Path(path).name}. Expected the exact USA revision; patched ROMs are not supported.')
        images[digest] = data
    if BASE_SHA not in images or SC_SHA not in images:
        raise ValueError('Both different USA ROMs are required: ClayFighter 63 1/3 AND Sculptor\'s Cut.')
    base, donor = images[BASE_SHA], images[SC_SHA]
    from tools.port_assets import build
    image, _ = build(base, donor)
    if sha(image) != ASSET_SHA: raise ValueError('Extracted asset verification failed.')
    decoder = root / 'bin' / ('font-decoder.exe' if os.name == 'nt' else 'font-decoder')
    with tempfile.TemporaryDirectory(prefix='clayfighter64-import-') as work:
        work = Path(work)
        (work/'base.z64').write_bytes(base)
        (work/'packet.bin').write_bytes(base[0xB68EEC:0xB68EEC+0x2B9B])
        subprocess.run([str(decoder), str(work/'base.z64'), str(work/'packet.bin'), str(work/'font.bin')], check=True, timeout=30)
        bios = build_bios(base, donor, (work/'font.bin').read_bytes())
    if sha(bios) != BIO_SHA: raise ValueError('Extracted biography verification failed.')
    outputs = {'probe-output/base.z64': base,
               'local/sc_roster/asset-image-roster.z64': image,
               'local/sc_bios/bios.bin': bios}
    for relative, data in outputs.items():
        destination = root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        temp = destination.with_name(destination.name + '.importing')
        temp.write_bytes(data)
        temp.replace(destination)
    manifest = {'project':'ClayFighter 64', 'version':'1.0', 'author':'djultra64',
                'files': {name:sha(data) for name,data in outputs.items()}}
    (root/'local/import-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    create_shortcut(root)
    print('Both USA ROMs verified. Characters, portraits and biographies extracted locally.\nInstallation complete. You can now run Play ClayFighter 64.')

def create_shortcut(root):
    """Create a local launcher with the supplied square artwork."""
    try:
        if os.name == 'nt':
            env = os.environ.copy(); env['CLAY_RELEASE_ROOT'] = str(root)
            script = "$r=$env:CLAY_RELEASE_ROOT;$s=(New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $r 'ClayFighter 64.lnk'));$s.TargetPath=(Join-Path $r 'Play ClayFighter 64.cmd');$s.WorkingDirectory=$r;$s.IconLocation=(Join-Path $r 'assets\\branding\\clayfighter64.ico');$s.Save()"
            subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-Command',script], env=env, check=True, timeout=15)
        else:
            # Desktop-entry escaping is separate from shell quoting.
            escaped = str(root/'Play ClayFighter 64.sh').replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')
            desktop = root/'ClayFighter 64.desktop'
            desktop.write_text('[Desktop Entry]\nType=Application\nName=ClayFighter 64\nExec="'+escaped+'"\nIcon='+str(root/'assets/branding/clayfighter64-icon.png')+'\nTerminal=true\nCategories=Game;\n')
            desktop.chmod(0o755)
    except (OSError, subprocess.SubprocessError) as error:
        print(f'Local shortcut unavailable: {error}. Use the Play launcher directly.')


def installed(root):
    expected = {'probe-output/base.z64':BASE_SHA,
                'local/sc_roster/asset-image-roster.z64':ASSET_SHA,
                'local/sc_bios/bios.bin':BIO_SHA}
    return all((root/name).is_file() and sha((root/name).read_bytes()) == digest for name,digest in expected.items())

def main():
    parser = argparse.ArgumentParser(description=REQUIRED, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('roms', nargs='*', type=Path, help='Paths to BOTH supported USA ROMs (either order).')
    parser.add_argument('--root', type=Path, default=ROOT, help=argparse.SUPPRESS)
    parser.add_argument('--check', action='store_true', help='Verify previously imported local data.')
    args = parser.parse_args()
    if args.check: return 0 if installed(args.root) else 1
    print(REQUIRED + '\n')
    paths = args.roms
    if not paths:
        if installed(args.root):
            print('Local assets already verified. Run Play ClayFighter 64 to start.'); return 0
        try:
            paths = [Path(input(label).strip().strip('"')) for label in
                     ['Path to ClayFighter 63 1/3 USA ROM: ', "Path to Sculptor's Cut USA ROM: "]]
        except (EOFError, KeyboardInterrupt):
            print('\nInstallation cancelled. BOTH ROMs are required.', file=sys.stderr); return 1
    try:
        if len(paths) != 2: raise ValueError('Provide exactly two ROM paths: 63 1/3 USA AND Sculptor\'s Cut USA.')
        install(args.root.resolve(), paths)
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.SubprocessError) as error:
        print(f'Installation failed: {error}', file=sys.stderr); return 1
    return 0

if __name__ == '__main__': sys.exit(main())
