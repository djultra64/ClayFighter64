#!/usr/bin/env python3
"""Verify diagnostic audio cannot write outside its installation. djultra64."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--cxx',default='c++')
    args=parser.parse_args()
    work=ROOT/'local/tests/audio-capture-paths';work.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='capture ',dir=work) as temporary:
        temporary=Path(temporary);game=temporary/'game';game.mkdir();outside=temporary/'outside';outside.mkdir()
        binary=temporary/'capture-test'
        subprocess.run([args.cxx,'-std=c++20','-O1','-I'+str(ROOT/'host'),
                        '-I'+str(args.sdk_root/'deps/N64ModernRuntime/N64Recomp/include'),
                        str(ROOT/'tests/title_audio_capture.cpp'),str(ROOT/'host/title_audio.cpp'),
                        '-o',str(binary)],check=True)
        for capture in [outside/'absolute.wav',Path('../outside/traversal.wav')]:
            result=subprocess.run([str(binary)],cwd=game,env={**os.environ,'CLAY_CAPTURE_AUDIO':str(capture)},capture_output=True,text=True,check=True)
            assert 'must remain inside the game folder' in result.stderr,result.stderr
            assert not list(outside.iterdir())
        (game/'escape').symlink_to(outside,target_is_directory=True)
        result=subprocess.run([str(binary)],cwd=game,env={**os.environ,'CLAY_CAPTURE_AUDIO':'escape/symlink.wav'},capture_output=True,text=True,check=True)
        assert 'must remain inside the game folder' in result.stderr and not list(outside.iterdir())
        for capture in [game/'absolute.wav',Path('relative.wav')]:
            subprocess.run([str(binary)],cwd=game,env={**os.environ,'CLAY_CAPTURE_AUDIO':str(capture)},check=True)
            path=capture if capture.is_absolute() else game/capture
            data=path.read_bytes();assert data[:4]==b'RIFF' and len(data)==52
    print('PASS: audio captures stay local; absolute escape, traversal and symlink escape refused.')


if __name__=='__main__':main()
