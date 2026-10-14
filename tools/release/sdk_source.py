#!/usr/bin/env python3
"""Archive the exact prepared third-party sources and build recipes. djultra64."""
from pathlib import Path
import tarfile

ROOT=Path(__file__).resolve().parents[2]
SDK=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK')
OUT=ROOT/'local/releases/1.0/clayfighter64-1.0-sdk-source.tar.gz'

def clean(info):
    if any(part in ('.git','__pycache__') for part in Path(info.name).parts):return None
    info.uid=0;info.gid=0;info.uname='';info.gname=''
    return info

with tarfile.open(OUT,'w:gz') as archive:
    for source in ['N64ModernRuntime','N64Recomp','rt64','spimdisasm']:
        archive.add(SDK/'deps'/source,arcname='sdk/deps/'+source,filter=clean)
    archive.add(SDK/'RT64-SOURCE-PATCH.json',arcname='sdk/RT64-SOURCE-PATCH.json',filter=clean)
    archive.add(ROOT/'config/dependencies-1.0.json',arcname='sdk/dependencies-1.0.json',filter=clean)
    for filename in ['sdl2-compat-2.32.70.tar.gz','SDL3-3.4.12.tar.gz','dbus-1.16.2.tar.xz']:
        archive.add(ROOT/'local'/filename,arcname='sdk/shared-source/'+filename,filter=clean)
    for name in ['sdl2','sdl3','dbus']:
        source=Path('/home/linuxbrew/.linuxbrew/opt')/name
        archive.add(source/'INSTALL_RECEIPT.json',arcname='sdk/shared-build/'+name+'/INSTALL_RECEIPT.json',filter=clean)
        archive.add(source/'.brew',arcname='sdk/shared-build/'+name+'/recipes',filter=clean)
print(OUT,OUT.stat().st_size)
