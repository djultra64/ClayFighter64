#!/usr/bin/env python3
"""Regression: quiet supplied voice plus long silence/music; reject wrong cue/missing voice. djultra64."""
from pathlib import Path
import tempfile,subprocess,random,array,wave,json,sys
R=Path(__file__).resolve().parents[1];rng=random.Random(64);hz=32000;frames=hz*11;start=hz
source=array.array('h',(rng.randrange(-1500,1501) if i<hz or 9*hz<=i<10*hz else 0 for i in range(frames)))
background=[rng.randrange(-2200,2201)for _ in range(hz*14)]
with tempfile.TemporaryDirectory(prefix='clay-audio-correlation-')as tmp:
 t=Path(tmp);pcm=t/'source.pcm';pcm.write_bytes(source.tobytes())
 def capture(path,voice):
  stereo=array.array('h')
  for i,n in enumerate(background):
   v=int(source[i-start]*23170/32767) if voice and start<=i<start+frames else 0;stereo.extend([n+v,n+v])
  with wave.open(str(path),'wb')as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(hz);w.writeframes(stereo.tobytes())
 capture(t/'mixed.wav',True);capture(t/'missing.wav',False)
 def check(wav,offset,expected):
  log=t/'events.log';log.write_text(json.dumps({'event':'title_audio_replacement','gain':32767})+'\n'+json.dumps({'event':'title_audio_mix_start','frequency':hz,'output_frame':offset})+'\n')
  result=subprocess.run([sys.executable,str(R/'tools/analyze_title_audio_capture.py'),str(wav),str(log),'--pcm',str(pcm)],capture_output=True,text=True)
  assert (result.returncode==0)==expected,result.stdout+result.stderr
  if expected:
   data=json.loads(result.stdout);assert data['runs'][0]['correlation']<.35;assert data['runs'][0]['active_correlation']>.35
 check(t/'mixed.wav',start,True);check(t/'mixed.wav',start+hz//2,False);check(t/'missing.wav',start,False)
print('PASS source-only active windows detect quiet mixed voice with long silence; wrong cue and absent voice fail')
