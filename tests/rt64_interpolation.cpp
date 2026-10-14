// Hidden actual RT64 frame/vertex matching proof by djultra64.
// RT64 source/shader copyright and MIT notices remain in the dependency.
#include "hle/rt64_game_frame.h"
#include "hle/rt64_workload.h"
#include "clay_rt64_shader_probe.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 RT64::Workload previous{},current{};RT64::GameFrame frame{};RT64::GameFrameMap::WorkloadMap map{};RT64::ModifiedBuffers changed{};
 for(auto*w:{&previous,&current}){auto&d=w->drawData;d.worldTransforms.push_back(hlslpp::float4x4::identity());d.transformGroups.resize(1);d.transformGroups[0].matrixId=0xcf000001;d.transformGroups[0].ordering=G_EX_ORDER_LINEAR;d.transformGroups[0].vertexInterpolation=G_EX_COMPONENT_INTERPOLATE;d.worldTransformGroups.push_back(0);d.worldTransformVertexIndices.push_back(0);d.worldIndices.resize(4);d.posFloats.resize(12);d.velFloats.resize(12);}
 for(unsigned v=0;v<4;v++){previous.drawData.posFloats[v*3]=v%2?20:10;previous.drawData.posFloats[v*3+1]=v<2?10:30;current.drawData.posFloats[v*3]=previous.drawData.posFloats[v*3]+8;current.drawData.posFloats[v*3+1]=previous.drawData.posFloats[v*3+1]+4;}
 std::multimap<uint32_t,uint32_t> old_ids,new_ids;std::vector<uint32_t> ignored;
 frame.buildTransformIdMap(previous,old_ids,ignored);frame.buildTransformIdMap(current,new_ids,ignored);assert(old_ids.size()==1&&new_ids.size()==1&&old_ids.begin()->first==new_ids.begin()->first);
 map.transforms.resize(1);map.prevTransformsMapped.resize(1);frame.matchTransform(current,previous,map,nullptr,new_ids.begin()->second,old_ids.begin()->second,changed);
 assert(changed.positionVelocity&&map.transforms[0].mapped&&map.prevTransformsMapped[0]);
 for(unsigned v=0;v<4;v++){assert(current.drawData.velFloats[v*3]==8);assert(current.drawData.velFloats[v*3+1]==4);}
 // Execute RT64's exact current-frame expressions from BOTH original shaders,
 // compiled using their own HLSL_CPU math types, at120Hz positions between60Hz ticks.
 for(float weight:{0.0f,0.5f,1.0f})for(unsigned v=0;v<4;v++){
  hlslpp::float3 p(current.drawData.posFloats[v*3],current.drawData.posFloats[v*3+1],0),vel(current.drawData.velFloats[v*3],current.drawData.velFloats[v*3+1],0);
  auto world=clay_rt64_world_sample(p,vel,weight),process=clay_rt64_process_sample(p,vel,weight);
  assert(world[0]==previous.drawData.posFloats[v*3]+8*weight);assert(world[1]==previous.drawData.posFloats[v*3+1]+4*weight);assert(world[0]==process[0]&&world[1]==process[1]);
 }
 // Original SKIP path produces no position velocity: extra presentations alone
 // would show the same simulation pose. A newcutID is excluded from ID matching.
 current.drawData.transformGroups[0].vertexInterpolation=G_EX_COMPONENT_SKIP;std::fill(current.drawData.velFloats.begin(),current.drawData.velFloats.end(),0);changed={};frame.matchTransform(current,previous,map,nullptr,0,0,changed);assert(!changed.positionVelocity&&current.drawData.velFloats[0]==0);
 current.drawData.transformGroups[0].matrixId=0xcf000002;frame.buildTransformIdMap(current,new_ids,ignored);assert(old_ids.find(new_ids.begin()->first)==old_ids.end());
 puts("PASS: actualRT64 workload ID matching + matchTransform creates8/4 vertexvelocity; exactRSPWorldCS/RSPProcessCS expressions at weights0/.5/1 yield midpoint14/12 for120Hz samples of60Hz motion; SKIP andcutIDs remainexcluded");
}
