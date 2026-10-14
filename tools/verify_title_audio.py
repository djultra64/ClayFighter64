#!/usr/bin/env python3
"""Original title cue metadata/PCM/DSP fixture by djultra64; private audio only."""
from pathlib import Path
import argparse,re,subprocess,tempfile,os,json,struct
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args();rom=r/'probe-output/base.z64';pcm=r/'local/audio/title64.s16le';assert pcm.is_file();pcm_bytes=pcm.read_bytes();assert pcm_bytes and not len(pcm_bytes)%2;source_frames=len(pcm_bytes)//2;assert all(rom.read_bytes()[pc+0xc00:pc+0xc04].hex()=='0c026dad'for pc in [0x65d74,0x66488])
f={}
for path in(r/'generated/guest').glob('funcs_*.c'):
 for n in ['800C1A04','800C1910','800C1520']:
  m=re.search(r'RECOMP_FUNC void func_'+n+r'\([^\n]*\).*?;\}',path.read_text(),re.S)
  if m:f[n]=m[0]
assert len(f)==3
with tempfile.TemporaryDirectory(prefix='clay-title-audio-')as t:
 t=Path(t);guest=t/'guest.cpp';guest.write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C"{\n'+'\n'.join(f.values())+'\n}\n');flags=[a.cxx,'-std=c++20','-O1','-UNDEBUG','-I'+str(r/'host'),'-I'+str(r/'generated/guest'),'-I'+str(a.sdk_root/'deps/N64ModernRuntime/N64Recomp/include')]
 subprocess.run(flags+[str(r/'tests/title_audio.cpp'),str(r/'host/title_audio.cpp'),'-o',str(t/'dsp')],check=True);subprocess.run([str(t/'dsp'),str(t/'fixture.pcm')],check=True)
 subprocess.run(flags+[str(r/'tests/title_audio_native.cpp'),str(r/'host/title_audio.cpp'),str(guest),'-o',str(t/'native')],check=True);capture=r/'analysis/title-audio-private-fixture.wav';env=os.environ.copy();env.update(CLAY_CAPTURE_AUDIO=str(capture),CLAY_TRACE_AUDIO='1');subprocess.run([str(t/'native'),str(rom),str(pcm)],check=True,env=env);wave=capture.read_bytes();assert wave[:4]==b'RIFF' and len(wave)==44+(source_frames+32000)*4 and struct.unpack_from('<I',wave,24)[0]==32000 and struct.unpack_from('<I',wave,40)[0]==len(wave)-44
 print(f'PASS private32kHz/stereo16 WAV capture header and complete{source_frames}sourceframes+1secondtail; no physical sound output')
