#!/usr/bin/env python3
"""Private mixed-capture correlation against supplied title PCM, by djultra64.
Presence/timing evidence; does not transcribe or play the recording.
"""
from pathlib import Path
import argparse,json,wave,array,math,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('wav',type=Path);p.add_argument('log',type=Path);p.add_argument('--pcm',type=Path,default=Path(__file__).resolve().parents[1]/'local/audio/title64.s16le');p.add_argument('--output',type=Path);a=p.parse_args()
events=[]
for line in a.log.read_text(errors='replace').splitlines():
 try:e=json.loads(line)
 except ValueError:continue
 if e.get('event')in ['title_audio_replacement','title_audio_mix_start']:events.append(e)
with wave.open(str(a.wav))as w:assert w.getnchannels()==2 and w.getsampwidth()==2;hz=w.getframerate();out=array.array('h',w.readframes(w.getnframes()))
source=array.array('h',a.pcm.read_bytes())
if sys.byteorder!='little':source.byteswap();out.byteswap()
starts=[];gain=None
for e in events:
 if e['event']=='title_audio_replacement':gain=e['gain']
 else:assert gain is not None;starts.append((e['output_frame'],gain,e['frequency']))
assert starts,'No actual native title event reached host mixer';runs=[]
for start,gain,freq in starts:
 assert hz==freq;step=(32000<<32)//hz;phase=0;x=[]
 while phase>>32<len(source):
  i=phase>>32;s=source[i]
  if i+1<len(source):s+=int((source[i+1]-s)*(phase&0xffffffff)/0x100000000)
  x.append(int(s*gain*23170/(32767*32767)));phase+=step
 assert start+len(x)<=len(out)//2,'Capture ended before announcement finished'
 y=[(out[(start+i)*2]+out[(start+i)*2+1])/2 for i in range(len(x))]
 def fit(xs,ys):
  mx=sum(xs)/len(xs);my=sum(ys)/len(ys);xx=sum((v-mx)**2 for v in xs);yy=sum((v-my)**2 for v in ys);xy=sum((sx-mx)*(sy-my)for sx,sy in zip(xs,ys))
  return xy/xx if xx else 0,xy/math.sqrt(xx*yy)if xx*yy else 0
 coef,corr=fit(x,y)
 # Select100ms windows from source energy alone; silence in the provided recording
 # contains no identifiable voice and must not count background music as a mismatch.
 block=max(1,hz//10);blocks=[(i,math.sqrt(sum(v*v for v in x[i:i+block])/len(x[i:i+block])))for i in range(0,len(x),block)]
 peak=max(rms for _,rms in blocks);assert peak>0,'Provided source is silent'
 active=[j for i,rms in blocks if rms>=peak*.25 for j in range(i,min(i+block,len(x)))]
 assert len(active)>=hz//10,'Insufficient active supplied audio'
 active_x=[x[i]for i in active];active_y=[y[i]for i in active];active_coef,active_corr=fit(active_x,active_y)
 controls=[]
 for shift_ms in [-1000,-500,-100,100,500,1000]:
  shift=int(hz*shift_ms/1000)
  if start+shift+active[0]<0 or start+shift+active[-1]>=len(out)//2:continue
  wrong_y=[(out[(start+shift+i)*2]+out[(start+shift+i)*2+1])/2 for i in active];wrong_coef,wrong_corr=fit(active_x,wrong_y)
  controls.append({'offset_ms':shift_ms,'fit_coefficient':wrong_coef,'correlation':wrong_corr})
 run={'start_frame':start,'start_seconds':start/hz,'output_frames':len(x),'native_gain':gain,'provided_clip_fit_coefficient':coef,'correlation':corr,'active_source_seconds':len(active)/hz,'source_window_peak_rms':peak,'source_window_threshold_fraction':.25,'active_provided_clip_fit_coefficient':active_coef,'active_correlation':active_corr,'negative_offset_controls':controls,'residual_music_sfx_rms':math.sqrt(sum((sy-sx)**2 for sx,sy in zip(x,y))/len(x))};runs.append(run)
 assert .65<active_coef<1.35 and active_corr>.35,'Provided active clip not identified at exact native event offset'
 assert len(controls)>=2 and max(abs(control['correlation'])for control in controls)<active_corr*.5,'Wrong timing matches supplied audio; exact cue identification ambiguous'
occupied=set()
for run in runs:
 occupied.update(range(run['start_frame'],run['start_frame']+run['output_frames']))
nonvoice_energy=0;nonvoice_frames=0
for i in range(len(out)//2):
 if i not in occupied:nonvoice_energy+=out[i*2]**2+out[i*2+1]**2;nonvoice_frames+=1
result={'author':'djultra64','result':'PASS','capture':str(a.wav),'actual_native_title_events':len(runs),'runs':runs,'outside_voice_rms':math.sqrt(nonvoice_energy/max(1,nonvoice_frames*2)),'scope':'At actual logged original title cue PCM offset, supplied clip is detected in final additive stereo audio. Residual/background energy is diagnostic; this is not a transcription, physical playback or proof of every sound effect.'};print(json.dumps(result,indent=2))
if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
