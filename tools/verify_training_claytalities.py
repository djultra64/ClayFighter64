#!/usr/bin/env python3
"""Derive private native Claytality smoke inputs; execution success requires runtime events. djultra64."""
import argparse
import json
from pathlib import Path
import struct
import tempfile
from verify_training_moves import ROOT, hybrid_definitions, records

# Authored control mapping for the verified default native permutation and host bindings.
# Directions remain rival-relative; generated scripts are specific to the requested side.
PHYSICAL = {0x1:'up',0x2:'down',0x10:'lb',0x20:'rb',0x40:'west',0x80:'back',
            0x100:'north',0x200:'south',0x400:'rt',0x800:'east'}


def first_finisher(data):
    count, pos, _ = records(data)
    for index in range(count):
        length = struct.unpack_from('>I', data, pos)[0]
        motions, motion_offset, variants, pattern_offset = struct.unpack_from('>HHHH', data, pos+40)
        if data[pos+6] == 3:
            for variant in range(variants):
                trigger, animation = struct.unpack_from('>HH', data, pos+pattern_offset+variant*36)
                if trigger:
                    minimum, maximum = struct.unpack_from('>hh', data, pos+26)
                    hold_mask, hold_ticks = struct.unpack_from('>HH', data, pos+36)
                    return {'record':index,'variant':variant,'animation':animation,
                            'motion':list(struct.unpack_from('>'+str(motions)+'H',data,pos+motion_offset)),
                            'trigger':trigger,'minimum_edge_gap':None if minimum == -1 else minimum,
                            'maximum_edge_gap':None if maximum == -1 else maximum,
                            'hold_mask':hold_mask,'hold_ticks':hold_ticks,
                            'postures':list(data[pos+19:pos+19+data[pos+18]])}
        pos += length
    return None


def buttons(mask, right_side):
    mapping = dict(PHYSICAL)
    mapping.update({0x4:'left' if right_side else 'right',0x8:'right' if right_side else 'left'})
    bits = mask & 0x7fff
    if bits & ~0xfff:
        raise ValueError('Unverified normalized command bits')
    selected = [bit for bit in sorted(mapping) if bit & bits]
    if mask & 0x8000:
        selected = selected[:1]  # Native ANY-bit recognizer.
    result = [mapping[bit] for bit in selected]
    if 'rt' in result:
        raise ValueError('Selected command needs a virtual trigger-axis event, unavailable in InputScript')
    return result


def script(command, args):
    result = []
    at = args.start_ms
    if command['hold_mask']:
        raise ValueError('Charged first finisher requires a separately verified timebase')
    tokens = command['motion'] + [command['trigger']]
    for index, token in enumerate(tokens):
        duration = args.trigger_ms if index == len(tokens)-1 else args.step_ms
        for button in buttons(token,args.right_side):
            result.append({'phase':'combat','at_ms':at,'duration_ms':duration,'player':1,'button':button})
        at += duration + args.release_ms
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'))
    parser.add_argument('--fighter',type=int,default=0)
    parser.add_argument('--all',action='store_true')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--prefix',type=Path,help='Existing SDL navigation script to prepend')
    parser.add_argument('--right-side',action='store_true')
    parser.add_argument('--start-ms',type=int,default=6000)
    parser.add_argument('--step-ms',type=int,default=100)
    parser.add_argument('--release-ms',type=int,default=34)
    parser.add_argument('--trigger-ms',type=int,default=100)
    args = parser.parse_args()
    if min(args.start_ms,args.step_ms,args.release_ms,args.trigger_ms)<0 or not args.step_ms or not args.trigger_ms:
        parser.error('Invalid input timings')
    destination = args.output or ROOT/'local/validation_claytalities'
    # Commands are derived privately; keep resulting scripts outside published source/assets.
    resolved = destination.resolve()
    if not any(resolved.is_relative_to(ROOT/part) for part in ('local','analysis')):
        parser.error('Private output must be inside this installation under local/ or analysis/')
    prefix = json.loads(args.prefix.read_text()) if args.prefix else []
    if not isinstance(prefix,list):raise ValueError('Navigation prefix must be an SDL script array')
    with tempfile.TemporaryDirectory(prefix='clay-private-finishers-') as directory:
        matched = False
        for ident,path in hybrid_definitions(Path(directory),args.sdk_root):
            if not args.all and ident != args.fighter:continue
            matched = True
            command = first_finisher(path.read_bytes())
            if command is None:
                print(json.dumps({'fighter':ident,'native_claytalities':False}));continue
            events = prefix + script(command,args)
            output = destination/f'fighter-{ident:02}.json' if args.all or destination.suffix!='.json' else destination
            output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(json.dumps(events,indent=2)+'\n')
            print(json.dumps({'fighter':ident,'script':str(output),'command':command,
                              'status':'Derived native command; runtime success unverified'}))
        if not matched:raise ValueError('Fighter is absent from the integrated 16-slot roster')


if __name__=='__main__':main()
