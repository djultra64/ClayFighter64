#!/usr/bin/env python3
"""Decode an explicitly provided private announcement to build-time PCM; djultra64.
No runtime codecs, voice generation, normalization, trimming or time stretching.
"""
from pathlib import Path
import argparse,subprocess,hashlib,json,array,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'local/audio/title64.s16le');a=p.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(a.source),'-ac','1','-ar','32000','-f','s16le','-y',str(a.output)],check=True)
b=a.output.read_bytes();assert b and len(b)%2==0 and len(b)<=32000*20*2;samples=array.array('h',b)
if sys.byteorder!='little':samples.byteswap()
record={'author':'djultra64','private_user_provided_audio':True,'source_name':a.source.name,'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'format':'signed16 littleendian mono32000Hz','frames':len(samples),'decoded_duration':len(samples)/32000,'bytes':len(b),'pcm_sha256':hashlib.sha256(b).hexdigest(),'peak':max(abs(i)for i in samples),'conversion':'ffmpeg decode/resample only; no voice synthesis, cloning, gain normalization, trims or time stretch','scope':'Original intro title voice pair bank0/instrument0/sounds0and4 at80065D74/80066488'};a.output.with_suffix('.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
