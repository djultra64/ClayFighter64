#!/usr/bin/env python3
"""Stage versioned public Linux/Windows archives without claiming validation. djultra64."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import zipfile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.release.public_files import check_public_file, release_version, source_files, write_checksums, private_path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'local/releases/1.0'
SDK=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK')

def copy(source,destination):
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,destination)

def common(platform, version='1.0', validation=None):
    version=release_version(version)
    target=OUT/('clayfighter64-'+version+'-'+platform+'-x86_64')
    if target.exists():raise SystemExit(f'Refusing to overwrite staging directory: {target}')
    target.mkdir(parents=True)
    for name in ['README.md','LICENSE','NOTICE','CREDITS.md','docs/INSTALL.md','docs/BUILD.md',
                 'tools/release/import_assets.py','tools/release/launch.py','tools/port_assets.py','tools/sc_import_assets.py']:
        copy(ROOT/name,target/name)
    for name in ['docs/TRAINING.md','docs/PACKAGING.md','docs/VALIDATION_1.0.md']:
        if (ROOT/name).is_file():copy(ROOT/name,target/name)
    if validation:
        copy(validation,target/'docs/releases'/Path(validation).name)
    shutil.copytree(ROOT/'assets',target/'assets',symlinks=True,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copytree(ROOT/'config',target/'config',symlinks=True)
    shutil.copytree(ROOT/'licenses',target/'licenses',symlinks=True)
    copy(ROOT/'local/audio/title64.s16le',target/'local/audio/title64.s16le')
    return target

def write_manifest(target, platform, version):
    files={}
    for path in sorted(target.rglob('*')):
        if path.is_symlink():raise ValueError(f'Symlink excluded from public package: {path}')
        if path.is_file() and path.name!='manifest.json':
            relative=path.relative_to(target)
            # Only the authored replacement announcement is allowed under local/.
            if relative.parts[0]=='local' and relative.as_posix()!='local/audio/title64.s16le':
                raise ValueError(f'Private state excluded from public package: {relative}')
            if private_path(relative) and relative.parts[0]!='local':
                raise ValueError(f'Private path excluded from public package: {relative}')
            check_public_file(path,relative)
            files[relative.as_posix()]={'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    manifest={'project':'ClayFighter 64','version':release_version(version),'author':'djultra64','platform':platform,
              'roms_included':False,'extracted_game_assets_included':False,'requires_both_usa_roms':True,
              'portable_settings':'local/settings','portable_cache':'local/cache','files':files}
    (target/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

def source_archive(version):
    name='clayfighter64-'+release_version(version)+'-source'
    destination=OUT/(name+'.tar.gz')
    if destination.exists():raise ValueError(f'Refusing to overwrite source archive: {destination}')
    with tarfile.open(destination,'w:gz') as archive:
        for source in source_files(ROOT):
            archive.add(source,arcname=name+'/'+source.relative_to(ROOT).as_posix(),filter=public_owner)
    return destination

def build(version='1.0', validation=None, *, linux_only=False):
    version=release_version(version)
    OUT.mkdir(parents=True,exist_ok=True)
    linux=common('linux',version,validation)
    windows=None if linux_only else common('windows',version,validation)
    copy(ROOT/'build/claytality64',linux/'clayfighter64')
    copy(ROOT/'generated/release-decoder/font-decoder',linux/'bin/font-decoder')
    for name in ['libSDL2-2.0.so.0','libSDL3.so.0','libdbus-1.so.3']:
        copy(Path('/home/linuxbrew/.linuxbrew/lib')/name,linux/'lib'/name)
    for name in ['libdxcompiler.so','libdxil.so']:
        copy(SDK/'deps/rt64/src/contrib/dxc/lib/x64'/name,linux/'lib'/name)
    for name,script in [('Install Assets.sh','import_assets.py'),('Play ClayFighter 64.sh','launch.py')]:
        path=linux/name
        path.write_text('#!/usr/bin/env bash\nset -euo pipefail\nclay_release_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"\nexport LD_LIBRARY_PATH="$clay_release_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec python3 "$clay_release_dir/tools/release/'+script+'" "$@"\n')
        path.chmod(0o755)
    if windows:prepare_windows(windows)
    for target,platform in [(linux,'Linux x86-64'),(windows,'Windows x86-64')]:
        if target:write_manifest(target,platform,version)
    source_archive(version)
    archive_staging(linux,windows)

def prepare_windows(windows):
    copy(ROOT/'local/releases/windows/claytality64.exe',windows/'clayfighter64.exe')
    copy(ROOT/'local/font-decoder.exe',windows/'bin/font-decoder.exe')
    for source in (ROOT/'local/releases/windows').glob('*.dll'):copy(source,windows/source.name)
    with zipfile.ZipFile(ROOT/'local/python-3.13.9-embed-amd64.zip') as archive:
        archive.extractall(windows/'python')
    copy(windows/'python/LICENSE.txt',windows/'licenses/Python/LICENSE.txt')
    for name,script in [('Install Assets.cmd','import_assets.py'),('Play ClayFighter 64.cmd','launch.py')]:
        text='@echo off\r\ncd /d "%~dp0"\r\n"python\\python.exe" "tools\\release\\'+script+'" %*\r\nset "clay_exit=%ERRORLEVEL%"\r\nif not "%clay_exit%"=="0" pause\r\nexit /b %clay_exit%\r\n'
        (windows/name).write_bytes(text.encode())

def public_owner(info):
    info.uid=0;info.gid=0;info.uname="djultra64";info.gname=""
    return info

def archive_staging(linux,windows):
    with tarfile.open(OUT/(linux.name+'.tar.gz'),'w:gz') as archive:archive.add(linux,arcname=linux.name,filter=public_owner)
    if windows:
        with zipfile.ZipFile(OUT/(windows.name+'.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=6,strict_timestamps=False) as archive:
            for p in sorted(windows.rglob('*')):
                if p.is_file():archive.write(p,windows.name+'/'+p.relative_to(windows).as_posix())
    write_checksums(OUT)
    for p in OUT.iterdir():
        if p.is_file():print(p.name,p.stat().st_size,hashlib.sha256(p.read_bytes()).hexdigest())
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version',default='1.0',type=release_version)
    parser.add_argument('--output',type=Path,help='New staging/archive directory; default local/releases/VERSION.')
    parser.add_argument('--sdk-root',type=Path,default=SDK)
    parser.add_argument('--validation',type=Path,help='Include this completed validation document; no implicit claims.')
    parser.add_argument('--linux-only',action='store_true')
    args=parser.parse_args()
    OUT=(args.output or ROOT/'local/releases'/args.version).resolve();SDK=args.sdk_root.resolve()
    if not OUT.is_relative_to(ROOT):parser.error('Release staging must remain inside the game folder.')
    build(args.version,args.validation,linux_only=args.linux_only)
