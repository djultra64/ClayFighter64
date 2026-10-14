#!/usr/bin/env python3
"""Private stage 3.10/4.10 conformance research. Author: djultra64.

No ROM mutation or gameplay adapter: 4.10 rail and exit semantics have no
proved lossless mapping to the original engine, and are explicitly rejected.
"""
from pathlib import Path
import argparse, hashlib, json, math, struct
ROOT = Path(__file__).resolve().parents[1]

def u32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise ValueError('word outside definition')
    return struct.unpack_from('>I', data, offset)[0]

def span(data, offset, length):
    if offset < 72 or length < 0 or offset + length > len(data):
        raise ValueError('section outside declared definition')
    return data[offset:offset + length]

def parse(payload):
    if len(payload) < 72 or payload[:4] not in (b'\x03\x0a\x01\x00', b'\x04\x0a\x01\x00'):
        raise ValueError('unsupported stage definition schema')
    size = u32(payload, 4)
    if size < 72 or size > len(payload):
        raise ValueError('invalid declared size')
    data = payload[:size]
    version = data[0]
    stride = 68 if version == 3 else 96
    count, offset = u32(data, 56), u32(data, 60)
    span(data, offset, count * stride)
    objects, object_offset = u32(data, 64), u32(data, 68)
    object_data = span(data, object_offset, objects * 60)
    cameras = []
    for index in range(count):
        p = offset + index * stride
        first = struct.unpack_from('>7f', data, p)
        if not all(math.isfinite(f) for f in first):
            raise ValueError('nonfinite camera/spawn field')
        tables = []
        for field, record in zip(([28,36,44,52,60] if version == 3 else [28,64,72,80,88]), [36,36,24,52 if version == 3 else 56,None]):
            n, start = u32(data,p+field), u32(data,p+field+4)
            tables.append({'count':n,'offset':start,'record_bytes':record})
        for table in tables:
            if table['record_bytes'] is not None:
                table['data'] = span(data,table['offset'],table['count']*table['record_bytes'])
        # Final table has variable-size native cinematic records. Bound its raw
        # bytes by the next camera's first section, or by the object table.
        tail_end = u32(data,offset+(index+1)*stride+32) if index+1<count else object_offset
        tail = tables[-1]
        tail['data'] = span(data,tail['offset'],tail_end-tail['offset'])
        rail = None
        if version == 4:
            n = data[p+52]
            if data[p+53:p+56] != bytes(3):
                raise ValueError('unknown packed rail flags')
            start, mode = u32(data,p+56),u32(data,p+60)
            raw = span(data,start,n*20)
            points = [struct.unpack_from('>5f',raw,i*20) for i in range(n)]
            if not all(math.isfinite(v) for point in points for v in point):
                raise ValueError('nonfinite rail')
            errors = []
            for i,point in enumerate(points[:-1]):
                nxt=points[i+1];length=math.hypot(nxt[0]-point[0],nxt[1]-point[1])
                if length:
                    errors.append(max(abs((nxt[0]-point[0])/length-point[3]),abs((nxt[1]-point[1])/length-point[4]),abs((nxt[2]-point[2])-length)))
            rail={'count':n,'offset':start,'mode':mode,'points':points,'maximum_geometric_error':max(errors,default=0),'extra_floats':struct.unpack_from('>4f',data,p+36)}
        cameras.append({'first7':first,'tables':tables,'rail':rail})
    return {'version':version,'size':size,'camera_count':count,'camera_offset':offset,'camera_stride':stride,'cameras':cameras,'object_count':objects,'object_offset':object_offset,'objects':object_data,'data':data}

def require_original_engine_definition(payload):
    """Identity only: never relabel SC data or silently discard its semantics."""
    parsed=parse(payload)
    if parsed['version'] != 3:
        raise ValueError('4.10 unsupported: nonempty new rail, changed spawn/exit/cinematic semantics require explicit native-compatible adaptation')
    return parsed['data']

def differences(a,b,stride):
    if len(a)!=len(b):
        raise ValueError('different record array length')
    return [{'record':i//stride,'offset':hex(i%stride),'base_hex':a[i:i+4].hex(),'sc_hex':b[i:i+4].hex(),'base_float':struct.unpack('>f',a[i:i+4])[0],'sc_float':struct.unpack('>f',b[i:i+4])[0]} for i in range(0,len(a),4) if a[i:i+4]!=b[i:i+4]]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=ROOT/'analysis/sc_stage_definition.json');args=parser.parse_args()
    catalog=json.loads((ROOT/'analysis/sc_stage_feasibility.json').read_text())
    roms={k:(ROOT/f'probe-output/{k}.z64').read_bytes() for k in ['base','sc']}
    out=ROOT/'local/sc_stages/definitions';out.mkdir(parents=True,exist_ok=True);(out/'.gitignore').write_text('*\n')
    stages=[];rejected=0
    for index in range(26):
        parsed={}
        for kind in roms:
            a,b=[int(v,16) for v in catalog['images'][kind]['stages'][index]['definition_range']]
            payload=roms[kind][a:b];parsed[kind]=parse(payload)
            (out/f'{kind}-{index:02d}.bin').write_bytes(parsed[kind]['data'])
        base,sc=parsed['base'],parsed['sc']
        assert require_original_engine_definition(base['data'])==base['data']
        try:require_original_engine_definition(sc['data'])
        except ValueError:rejected+=1
        else:raise AssertionError('unadapted SC definition accepted')
        assert base['object_count']==sc['object_count']
        cameras=[]
        for a,b in zip(base['cameras'],sc['cameras']):
            tables=[]
            for slot,(x,y) in enumerate(zip(a['tables'],b['tables'])):
                tables.append({'slot':slot,'base_count':x['count'],'sc_count':y['count'],'base_record_bytes':x['record_bytes'],'sc_record_bytes':y['record_bytes'],'base_bytes':len(x['data']),'sc_bytes':len(y['data']),'identical':x['data']==y['data'],'base_sha256':hashlib.sha256(x['data']).hexdigest(),'sc_sha256':hashlib.sha256(y['data']).hexdigest()})
            cameras.append({'base_first7':a['first7'],'sc_first7':b['first7'],'tables':tables,'sc_rail':b['rail']})
        stages.append({'id':index,'base_name':catalog['images']['base']['stages'][index]['name'],'sc_name':catalog['images']['sc']['stages'][index]['name'],'cameras':cameras,'object_count':base['object_count'],'object_record_bytes':60,'object_differences':differences(base['objects'],sc['objects'],60)})
    # Rejection guards operate on genuine fixture copies, not synthetic schemas.
    sample=(out/'sc-00.bin').read_bytes()
    for mutation in ['truncated','wrong_version','bad_camera_offset','bad_object_count','nonfinite_spawn']:
        d=bytearray(sample)
        if mutation=='truncated':d=d[:100]
        elif mutation=='wrong_version':d[0]=5
        elif mutation=='bad_camera_offset':struct.pack_into('>I',d,60,0xfffffff0)
        elif mutation=='bad_object_count':struct.pack_into('>I',d,64,0xffffffff)
        else:struct.pack_into('>I',d,72,0x7f800000)
        try:parse(d)
        except ValueError:pass
        else:raise AssertionError('invalid fixture accepted: '+mutation)
    report={'author':'djultra64','status':'PASS structural conformance and strict rejection; no lossless 4.10 adapter or arena gameplay claim','native_contract':{'version_gate':'80085998..800859D4: exact3.10 else originalfatal B9154','camera_stride':'80085A04..80085A18: index*68','spawn_fields':'80085A5C..80085B00: camera+0C/+10/+14/+18','movement_bounds':'800903B0/903FC/90418/90650: camera+20 offset,+1C count,record36','object_loader':'8000B2B8..8000B640: definition+40/+44,count*60; float+28 copiedat8000B4F4..8000B518'},'layout':{'base_camera':68,'sc_camera':96,'base_exit_record':52,'sc_exit_record':56,'shared_segment_record':36,'shared_secondary_record':24,'object_record':60,'last_table':'variable native cinematic records; no constant104-byte assumption'},'tests':{'original_identity':26,'unadapted_sc_rejected':rejected,'invalid_fixture_rejected':5},'stages':stages,'recommendation':'For bounded SC visual variants retain entire original3.10 definition (cameras, collision, exits, cinematic points, objects), then validate SC mesh geometry/material index compatibility. Stage5 needs special geometry delta checks. Do not import changed SC rails or reinterpret exits as52bytes.'}
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps({'original_identity':26,'unadapted_sc_rejected':rejected,'invalid_fixture_rejected':5,'changed_object_stages':[s['id'] for s in stages if s['object_differences']],'report':str(args.output)}))
if __name__=='__main__':main()
