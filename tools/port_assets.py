#!/usr/bin/env python3
"""Prepare locally supplied model and portrait assets for the original63 port. djultra64."""
import hashlib
import json
from pathlib import Path
import struct
from tools.sc_import_assets import build as build_models
PORTRAITS={6:0xF6EEA0,11:0xF818D0,12:0xF865D0,16:0xF95D00}
EXPECTED='26ccae86b452d4bacadf429e0b241751aeef58a41d676e7d6173974414dafd7d'
def build(base,donor):
 model,manifest=build_models(base,donor);image=bytearray(model);portraits=[]
 for ident,source in PORTRAITS.items():
  if struct.unpack_from('>I',donor,0xABBF0+ident*4)[0]!=source:raise ValueError('Unexpected portrait table')
  version,count,length=struct.unpack_from('>III',donor,source)
  if version!=0x01000100 or count!=2 or source+length>len(donor):raise ValueError('Unsupported portrait bank')
  begin=len(image);image.extend(donor[source:source+length]);image.extend(bytes(-len(image)%16))
  portraits.append({'id':ident,'source':source,'begin':begin,'end':len(image),'sha256':hashlib.sha256(donor[source:source+length]).hexdigest()})
 digest=hashlib.sha256(image).hexdigest()
 if digest!=EXPECTED:raise ValueError('Unsupported port asset image')
 manifest.update({'asset_image_sha256':digest,'size':len(image),'portraits':portraits,'model_prefix_sha256':hashlib.sha256(model).hexdigest()})
 return bytes(image),manifest

def prepare(root,base,donor):
 image,manifest=build(base,donor);path=Path(root)/'analysis/sc_assets/port-assets.z64';path.parent.mkdir(parents=True,exist_ok=True)
 if not path.exists() or path.read_bytes()!=image:path.write_bytes(image)
 path.with_suffix('.manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 return path
