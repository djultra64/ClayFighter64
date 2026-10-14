#!/usr/bin/env python3
"""Create an optional RT64 observer copy without changing graphics behavior.
ClayTality 64 diagnostics by djultra64. Original RT64 attribution is retained.
"""
import argparse
import hashlib
from pathlib import Path
EXPECTED = '6e53030e8899c42347b51e67879fcb9679ad487ac067c192612c432d85bca4a4'
SITE = '        bool uploadTiles = false;'
OBSERVER = r'''
        if (std::getenv("CLAY_TRACE_INTERPOLATION")) {
            unsigned mapped=0, moving=0, vertexVelocity=0, perspective=0, orthographic=0, rectangles=0;
            auto matrixDiffers=[](const interop::float4x4& a,const interop::float4x4& b){
                float af[16],bf[16];static_assert(sizeof(a)==sizeof(af));
                std::memcpy(af,&a,sizeof(af));std::memcpy(bf,&b,sizeof(bf));
                for(unsigned c=0;c<16;c++)if(std::isfinite(af[c])&&std::isfinite(bf[c])&&std::abs(af[c]-bf[c])>1e-5f)return true;
                return false;
            };
            for(auto w:curFrame.workloads){
                const auto& d=workloads[w].drawData;
                if(w<curFrame.frameMap.workloads.size())for(const auto& t:curFrame.frameMap.workloads[w].transforms)mapped+=t.mapped;
                for(size_t t=0;t<std::min(d.worldTransforms.size(),d.lerpWorldTransforms.size());t++)moving+=matrixDiffers(d.worldTransforms[t],d.lerpWorldTransforms[t]);
                for(float v:d.velFloats)vertexVelocity+=std::isfinite(v)&&std::abs(v)>1e-5f;
                const auto& workload=workloads[w];
                for(unsigned f=0;f<workload.fbPairCount;f++)for(unsigned p=0;p<workload.fbPairs[f].projectionCount;p++){
                    const auto type=workload.fbPairs[f].projections[p].type;
                    perspective+=type==Projection::Type::Perspective;orthographic+=type==Projection::Type::Orthographic;rectangles+=type==Projection::Type::Rectangle;
                }
            }
            fprintf(stderr,"{\"event\":\"rt64_interpolation_observed\",\"weight\":%.6f,\"target_rate\":%u,\"aspect_scale\":%.6f,\"mapped_world_transforms\":%u,\"world_transforms_between_frames\":%u,\"nonzero_vertex_velocities\":%u,\"perspective\":%u,\"orthographic\":%u,\"rectangles\":%u}\n",curFrameWeight,workloadConfig.targetRate,workloadConfig.aspectRatioScale,mapped,moving,vertexVelocity,perspective,orthographic,rectangles);
        }
'''
def prepare(rt64, output):
    source=Path(rt64)/'src/hle/rt64_workload_queue.cpp'
    raw=source.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EXPECTED:
        raise ValueError('RT64 observer source fingerprint changed')
    text=raw.decode()
    if text.count(SITE)!=1:raise ValueError('RT64 interpolation observer site is not unique')
    text='#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n#include <cmath>\n'+text.replace(SITE,OBSERVER+SITE)
    output=Path(output);output.parent.mkdir(parents=True,exist_ok=True)
    if not output.exists() or output.read_text()!=text:output.write_text(text)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('rt64',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.rt64,a.output)
