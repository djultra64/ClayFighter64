#!/usr/bin/env python3
"""Shared public archive policy; authored by djultra64."""
from pathlib import Path
import hashlib
import re

PRIVATE_PARTS = {'analysis', 'local', 'probe-output', 'generated', 'build', '.git', '__pycache__', '.pytest_cache'}
ROM_SUFFIXES = {'.z64', '.v64', '.n64', '.rom'}
ROM_HEADERS = {bytes.fromhex(x) for x in ('80371240', '37804012', '40123780')}
SOURCE_DIRS = ('host', 'include', 'tools', 'tests', 'config', 'assets', 'licenses', 'docs')
SOURCE_FILES = ('CMakeLists.txt', 'README.md', 'LICENSE', 'NOTICE', 'CREDITS.md', '.gitignore')
PRIVATE_SUFFIXES = {'.bin', '.decoded', '.pcm', '.wav', '.s16le', '.exe', '.dll', '.o', '.a', '.so', '.lib', '.pdb', '.pyc', '.log', '.zip', '.gz', '.xz'}


def private_path(relative):
    parts=Path(relative).parts
    # Authentic notices retain upstream paths, including zstd/build/LICENSE.
    blocked=PRIVATE_PARTS-({'build'} if parts and parts[0]=='licenses' else set())
    return any(part in blocked for part in parts)


def release_version(value):
    if not re.fullmatch(r'[0-9]+\.[0-9]+(?:\.[0-9]+)?(?:-[a-zA-Z0-9.-]+)?', value):
        raise ValueError('Release version must look like 1.0 or 2.0-rc1.')
    return value


def check_public_file(path, relative, *, source=False):
    path, relative = Path(path), Path(relative)
    if path.is_symlink():
        raise ValueError(f'Public archives cannot contain symlinks: {relative}')
    if path.suffix.lower() in ROM_SUFFIXES:
        raise ValueError(f'ROM excluded from public archive: {relative}')
    if source and private_path(relative):
        raise ValueError(f'Private path excluded from source archive: {relative}')
    with path.open('rb') as stream:
        if stream.read(4) in ROM_HEADERS:
            raise ValueError(f'ROM header found in public archive: {relative}')


def source_files(root):
    """Only authored source, public assets, recipes and authentic notices."""
    root = Path(root)
    for name in SOURCE_FILES:
        path = root / name
        if path.is_file():
            check_public_file(path, name, source=True)
            yield path
    for name in SOURCE_DIRS:
        for path in sorted((root / name).rglob('*')):
            relative = path.relative_to(root)
            # Private development observations and fixtures are not corresponding source.
            if private_path(relative):
                continue
            if relative.parts[:2] == ('docs', 'superpowers'):
                continue
            if relative.as_posix() == 'tools/windows_remote.py':
                continue
            if relative.parts[0] not in {'assets','licenses'} and path.suffix.lower() in PRIVATE_SUFFIXES:
                continue
            if path.is_symlink():
                raise ValueError(f'Public archives cannot contain symlinks: {relative}')
            if path.is_file():
                check_public_file(path, relative, source=True)
                yield path


def write_checksums(output):
    """Refresh sums after packaging either binary/authored or dependency sources."""
    output=Path(output)
    archives=sorted(path for path in output.iterdir()
                    if path.is_file() and (path.name.endswith('.tar.gz') or path.suffix=='.zip'))
    lines=[]
    for path in archives:
        with path.open('rb') as stream:
            digest=hashlib.file_digest(stream,'sha256').hexdigest()
        lines.append(digest+'  '+path.name+'\n')
    (output/'SHA256SUMS').write_text(''.join(lines))
