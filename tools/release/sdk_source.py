#!/usr/bin/env python3
"""Archive prepared third-party sources and recipes for an explicit version. djultra64."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tarfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.release.public_files import ROM_SUFFIXES, release_version, write_checksums
SDK=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK')


def clean(info):
    # Upstream analysis/ packages (spimdisasm) and build/cmake directories contain required source recipes (zstd,
    # for example), so exclude CMake outputs rather than all directories named build.
    if any(part in ('.git','__pycache__','CMakeFiles') for part in Path(info.name).parts):
        return None
    if Path(info.name).name=='CMakeCache.txt':return None
    if info.issym() or info.islnk() or Path(info.name).suffix.lower() in ROM_SUFFIXES:
        return None
    info.uid=0;info.gid=0;info.uname='';info.gname=''
    return info


def shared_sources(record):
    filenames=['sdl2-compat-2.32.70.tar.gz','SDL3-3.4.12.tar.gz','dbus-1.16.2.tar.xz']
    windows_sdl=record.get('windows_shared',{}).get('SDL2')
    if windows_sdl:
        source=record.get('windows_shared_source',{}).get('SDL2',{})
        filename='SDL2-'+windows_sdl+'.tar.gz'
        if source.get('archive')!=filename or not source.get('sha256'):
            raise ValueError('Record the exact Windows SDL2 source archive and SHA-256 before packaging.')
        filenames.append(filename)
    recorded=record.get('verification',{}).get('selected_source_and_installer_archives',{})
    result=[]
    for filename in filenames:
        path=ROOT/'local'/filename
        expected=recorded.get(filename,{}).get('sha256')
        if filename.startswith('SDL2-'):
            expected=record['windows_shared_source']['SDL2']['sha256']
        if not path.is_file():raise ValueError(f'Missing matching dependency source: {filename}')
        if expected and hashlib.sha256(path.read_bytes()).hexdigest()!=expected:
            raise ValueError(f'Dependency source SHA-256 mismatch: {filename}')
        result.append(path)
    return result


def build(version='1.0',sdk=SDK,output=None,dependencies=None):
    version=release_version(version)
    output=Path(output or ROOT/'local/releases'/version/('clayfighter64-'+version+'-sdk-source.tar.gz')).resolve()
    if not output.is_relative_to(ROOT):
        raise ValueError('SDK staging must remain inside the game folder.')
    dependencies=Path(dependencies or ROOT/'config'/('dependencies-'+version+'.json'))
    if not dependencies.is_file():
        raise ValueError(f'Record exact dependencies for this version before packaging: {dependencies}')
    sources=shared_sources(json.loads(dependencies.read_text()))
    if output.exists():
        raise ValueError(f'Refusing to overwrite SDK archive: {output}')
    output.parent.mkdir(parents=True,exist_ok=True)
    with tarfile.open(output,'w:gz') as archive:
        for source in ['N64ModernRuntime','N64Recomp','rt64','spimdisasm']:
            archive.add(Path(sdk)/'deps'/source,arcname='sdk/deps/'+source,filter=clean)
        archive.add(Path(sdk)/'RT64-SOURCE-PATCH.json',arcname='sdk/RT64-SOURCE-PATCH.json',filter=clean)
        archive.add(dependencies,arcname='sdk/dependencies-'+version+'.json',filter=clean)
        # Prepared SDK inputs include prebuilt DXC; retain its authentic notice
        # and the complete release dependency notice inventory beside sources.
        archive.add(ROOT/'licenses',arcname='sdk/licenses',filter=clean)
        for source in sources:
            archive.add(source,arcname='sdk/shared-source/'+source.name,filter=clean)
        for name in ['sdl2','sdl3','dbus']:
            source=Path('/home/linuxbrew/.linuxbrew/opt')/name
            archive.add(source/'INSTALL_RECEIPT.json',arcname='sdk/shared-build/'+name+'/INSTALL_RECEIPT.json',filter=clean)
            archive.add(source/'.brew',arcname='sdk/shared-build/'+name+'/recipes',filter=clean)
    write_checksums(output.parent)
    return output


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version',default='1.0',type=release_version)
    parser.add_argument('--sdk-root',type=Path,default=SDK)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--dependencies',type=Path,help='Exact dependency record for the binaries being distributed.')
    args=parser.parse_args()
    archive=build(args.version,args.sdk_root,args.output,args.dependencies)
    print(archive,archive.stat().st_size)
