#!/usr/bin/env python3
"""Create clean public Linux and Windows 1.0 archives. djultra64."""
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import zipfile

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'local/releases/1.0'
SDK=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK')

def copy(source,destination):
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,destination)

def common(platform):
    target=OUT/('clayfighter64-1.0-'+platform+'-x86_64')
    if target.exists():raise SystemExit(f'Refusing to overwrite staging directory: {target}')
    target.mkdir(parents=True)
    for name in ['README.md','LICENSE','NOTICE','CREDITS.md','docs/INSTALL.md','docs/BUILD.md','docs/VALIDATION_1.0.md',
                 'tools/release/import_assets.py','tools/release/launch.py','tools/port_assets.py','tools/sc_import_assets.py']:
        copy(ROOT/name,target/name)
    shutil.copytree(ROOT/'assets',target/'assets',ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copytree(ROOT/'config',target/'config')
    shutil.copytree(ROOT/'licenses',target/'licenses')
    copy(ROOT/'local/audio/title64.s16le',target/'local/audio/title64.s16le')
    return target

def build():
    OUT.mkdir(parents=True,exist_ok=True)
    linux=common('linux');windows=common('windows')
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
    copy(ROOT/'local/releases/windows/claytality64.exe',windows/'clayfighter64.exe')
    copy(ROOT/'local/font-decoder.exe',windows/'bin/font-decoder.exe')
    for source in (ROOT/'local/releases/windows').glob('*.dll'):copy(source,windows/source.name)
    with zipfile.ZipFile(ROOT/'local/python-3.13.9-embed-amd64.zip') as archive:
        archive.extractall(windows/'python')
    copy(windows/'python/LICENSE.txt',windows/'licenses/Python/LICENSE.txt')
    for name,script in [('Install Assets.cmd','import_assets.py'),('Play ClayFighter 64.cmd','launch.py')]:
        text='@echo off\r\ncd /d "%~dp0"\r\n"python\\python.exe" "tools\\release\\'+script+'" %*\r\nset "clay_exit=%ERRORLEVEL%"\r\nif not "%clay_exit%"=="0" pause\r\nexit /b %clay_exit%\r\n'
        (windows/name).write_bytes(text.encode())
    for target,platform in [(linux,'Linux x86-64'),(windows,'Windows x86-64')]:
        manifest={'project':'ClayFighter 64','version':'1.0','author':'djultra64','platform':platform,
                  'roms_included':False,'extracted_game_assets_included':False,'requires_both_usa_roms':True,
                  'files':{str(p.relative_to(target)):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
                           for p in sorted(target.rglob('*')) if p.is_file()}}
        (target/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    archive_staging(linux,windows)

def public_owner(info):
    info.uid=0;info.gid=0;info.uname="djultra64";info.gname=""
    return info

def archive_staging(linux,windows):
    with tarfile.open(OUT/(linux.name+'.tar.gz'),'w:gz') as archive:archive.add(linux,arcname=linux.name,filter=public_owner)
    with zipfile.ZipFile(OUT/(windows.name+'.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=6,strict_timestamps=False) as archive:
        for p in sorted(windows.rglob('*')):
            if p.is_file():archive.write(p,windows.name+'/'+str(p.relative_to(windows)))
    for p in OUT.iterdir():
        if p.is_file():print(p.name,p.stat().st_size,hashlib.sha256(p.read_bytes()).hexdigest())
if __name__=='__main__':build()
