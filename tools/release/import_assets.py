#!/usr/bin/env python3
"""ClayFighter 64 two-ROM portable installer. Work by djultra64."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
BASE_SHA = '357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07'
SC_SHA = '9ccebc317b7ee892be3bdc0d1c8ae46687995908ed51ebd58d6855e5e25abd7f'
ROM_SUFFIXES = ('.z64', '.v64', '.n64', '.c64', '.c')
ASSET_SHA = '26ccae86b452d4bacadf429e0b241751aeef58a41d676e7d6173974414dafd7d'
BIO_SHA = 'd7f85d9be2dd4614923c1c0af0e6b3b27170f02e715edbebca2d4e4e7627bae3'
REQUIRED = ('ClayFighter 64 is a fusion of ClayFighter 63 1/3 and Sculptor\'s Cut.\n'
            'Installation requires BOTH of your own USA ROMs:\n'
            '  1. ClayFighter 63 1/3 (USA)\n'
            '  2. ClayFighter: Sculptor\'s Cut (USA)\n'
            'Supported formats: .z64, .v64, .n64, .c64, .c, or ZIP containing exactly one ROM.\n'
            'No ROMs or extracted original game assets are included or downloaded.')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def portable_path(root, relative):
    root = Path(root).resolve()
    destination = root / relative
    if not destination.resolve().is_relative_to(root):
        raise ValueError('Imported files must remain inside the game folder.')
    return destination

def load_rom(path):
    path = Path(path)
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            entries = [i for i in archive.infolist() if not i.is_dir() and
                       Path(i.filename).suffix.lower() in ROM_SUFFIXES]
            if len(entries) != 1:
                raise ValueError('ZIP must contain exactly one supported ROM.')
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

def discover_roms(root):
    """Find supported ROMs in the installation and adjacent ROMs folders."""
    root = Path(root)
    directories = [root]
    for parent in (root, root.parent):
        try:
            directories.extend(p for p in parent.iterdir() if p.is_dir() and p.name.casefold() == 'roms')
        except OSError:
            continue
    found = {}
    for directory in directories:
        try:
            candidates = [p for p in directory.iterdir() if p.is_file() and p.suffix.lower() in (*ROM_SUFFIXES, '.zip')]
        except OSError:
            continue
        # Prefer likely filenames, but accept any file verified by its content.
        candidates.sort(key=lambda p: ('clay' not in p.stem.casefold(), p.name.casefold()))
        for path in candidates:
            try:
                digest = sha(load_rom(path))
            except (OSError, ValueError, zipfile.BadZipFile, RuntimeError):
                continue
            if digest in (BASE_SHA, SC_SHA):
                found.setdefault(digest, path)
            if len(found) == 2:
                return found
    return found


def linux_dialog(found):
    """Select missing ROMs with the desktop's native file picker."""
    zenity = shutil.which('zenity')
    kdialog = shutil.which('kdialog')
    if not zenity and not kdialog:
        return None
    paths = []
    for digest, label in ((BASE_SHA, 'ClayFighter 63 1/3 (USA)'),
                          (SC_SHA, "ClayFighter: Sculptor's Cut (USA)")):
        if digest in found:
            paths.append(found[digest])
            continue
        title = 'Select ' + label + ' ROM'
        if zenity:
            command = [zenity, '--file-selection', '--title=' + title,
                       '--file-filter=N64 ROMs and ZIP archives | *.z64 *.v64 *.n64 *.c64 *.c *.zip *.Z64 *.V64 *.N64 *.C64 *.C *.ZIP',
                       '--file-filter=All files | *']
        else:
            command = [kdialog, '--title', title, '--getopenfilename', '.',
                       'N64 ROMs and ZIP archives (*.z64 *.v64 *.n64 *.c64 *.c *.zip *.Z64 *.V64 *.N64 *.C64 *.C *.ZIP)']
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0 or not result.stdout.strip():
            return []
        paths.append(Path(result.stdout.rstrip('\r\n')))
    return paths


def windows_dialog(found):
    script = ROOT / 'tools/release/install_assets_gui.ps1'
    env = os.environ.copy()
    env['CLAY_BASE_ROM'] = str(found.get(BASE_SHA, ''))
    env['CLAY_SC_ROM'] = str(found.get(SC_SHA, ''))
    result = subprocess.run(['powershell.exe', '-NoProfile', '-STA', '-ExecutionPolicy', 'Bypass',
                             '-File', str(script)], env=env, capture_output=True, encoding='utf-8', check=True,
                            creationflags=0x08000000)
    return [Path(p) for p in json.loads(result.stdout)] if result.stdout.strip() else []


def windows_message(message, error=False):
    env = os.environ.copy()
    env['CLAY_INSTALL_MESSAGE'] = message
    env['CLAY_INSTALL_ICON'] = 'Error' if error else 'Information'
    subprocess.run(['powershell.exe', '-NoProfile', '-STA', '-Command',
                    'Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.MessageBox]::Show($env:CLAY_INSTALL_MESSAGE, "ClayFighter 64", "OK", $env:CLAY_INSTALL_ICON) | Out-Null'],
                   env=env, check=False, creationflags=0x08000000)

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
    work_root = portable_path(root, 'local/import-work')
    work_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='clayfighter64-import-', dir=work_root) as work:
        work = Path(work)
        (work/'base.z64').write_bytes(base)
        (work/'packet.bin').write_bytes(base[0xB68EEC:0xB68EEC+0x2B9B])
        subprocess.run([str(decoder), str(work/'base.z64'), str(work/'packet.bin'), str(work/'font.bin')], check=True, timeout=30, creationflags=0x08000000 if os.name == 'nt' else 0)
        bios = build_bios(base, donor, (work/'font.bin').read_bytes())
    if sha(bios) != BIO_SHA: raise ValueError('Extracted biography verification failed.')
    outputs = {'probe-output/base.z64': base,
               'local/sc_roster/asset-image-roster.z64': image,
               'local/sc_bios/bios.bin': bios}
    for relative, data in outputs.items():
        destination = portable_path(root, relative)
        destination.parent.mkdir(parents=True, exist_ok=True)
        temp = portable_path(root, relative + '.importing')
        temp.write_bytes(data)
        temp.replace(destination)
    manifest = {'project':'ClayFighter 64', 'asset_schema':1, 'author':'djultra64',
                'files': {name:sha(data) for name,data in outputs.items()}}
    portable_path(root, 'local/import-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    create_shortcut(root)
    print('Both USA ROMs verified. Characters, portraits and biographies extracted locally.\nInstallation complete. You can now run Play ClayFighter 64.')

def create_shortcut(root):
    """Create a local launcher with the supplied square artwork."""
    try:
        if os.name == 'nt':
            portable_path(root, 'ClayFighter 64.lnk')
            env = os.environ.copy(); env['CLAY_RELEASE_ROOT'] = str(root)
            script = "$r=$env:CLAY_RELEASE_ROOT;$s=(New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $r 'ClayFighter 64.lnk'));$s.TargetPath=(Join-Path $r 'Play ClayFighter 64.vbs');$s.WorkingDirectory=$r;$s.IconLocation=(Join-Path $r 'assets\\branding\\clayfighter64.ico');$s.Save()"
            subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-Command',script], env=env, check=True, timeout=15, creationflags=0x08000000)
        else:
            # Desktop-entry escaping is separate from shell quoting.
            escaped = str(root/'Play ClayFighter 64.sh').replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')
            desktop = portable_path(root, 'ClayFighter 64.desktop')
            desktop.write_text('[Desktop Entry]\nType=Application\nName=ClayFighter 64\nExec="'+escaped+'"\nIcon='+str(root/'assets/branding/clayfighter64-icon.png')+'\nTerminal=true\nCategories=Game;\n')
            desktop.chmod(0o755)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
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
            print('Local assets already verified. Run Play ClayFighter 64 to start.')
            if os.name == 'nt': windows_message('Local assets are already installed and verified.')
            return 0
        try:
            found = discover_roms(args.root)
            if len(found) == 2:
                paths = [found[BASE_SHA], found[SC_SHA]]
            elif os.name == 'nt':
                paths = windows_dialog(found)
            else:
                paths = linux_dialog(found) if (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')) else None
                if paths is None:
                    paths = []
                    for digest, label in ((BASE_SHA, 'Path to ClayFighter 63 1/3 USA ROM: '),
                                          (SC_SHA, "Path to Sculptor's Cut USA ROM: ")):
                        paths.append(found[digest] if digest in found else Path(input(label).strip().strip('"')))
        except (OSError, ValueError, subprocess.SubprocessError) as error:
            print(f'Installation failed: {error}', file=sys.stderr); return 1
        except (EOFError, KeyboardInterrupt):
            print('\nInstallation cancelled. BOTH ROMs are required.', file=sys.stderr); return 1
        if not paths:
            print('Installation cancelled.', file=sys.stderr); return 1
    try:
        if len(paths) != 2: raise ValueError('Provide exactly two ROM paths: 63 1/3 USA AND Sculptor\'s Cut USA.')
        install(args.root.resolve(), paths)
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.SubprocessError) as error:
        print(f'Installation failed: {error}', file=sys.stderr)
        if os.name == 'nt' and not args.roms: windows_message(f'Installation failed: {error}', True)
        return 1
    if os.name == 'nt' and not args.roms: windows_message('Installation complete. You can now run Play ClayFighter 64.')
    return 0

if __name__ == '__main__': sys.exit(main())
