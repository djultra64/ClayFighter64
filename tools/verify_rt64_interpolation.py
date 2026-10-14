#!/usr/bin/env python3
"""Hidden actual RT64 matching and original shader fractional expression proof; djultra64.
No window, swapchain, display-setting change or user screenshot is created.
"""
from pathlib import Path
import argparse,subprocess,tempfile,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sdk-root',type=Path,default=Path('/tmp/Blast Corps source review b48mc3gh/independent SDK'));p.add_argument('--cxx',default='c++');a=p.parse_args()
rt=a.sdk_root/'deps/rt64';src=rt/'src'
include=[src,src/'contrib',src/'contrib/hlslpp/include',src/'contrib/json',src/'contrib/plume',src/'contrib/imgui',src/'contrib/plume/contrib/Vulkan-Headers/include',src/'contrib/plume/contrib/VulkanMemoryAllocator/include',src/'contrib/plume/contrib/volk',src/'contrib/mupen64plus-core/src/api']
include += [Path('/home/linuxbrew/.linuxbrew/include'),Path('/home/linuxbrew/.linuxbrew/include/SDL2'),Path('/home/linuxbrew/.linuxbrew/opt/xorgproto/include')]
sources=['hle/rt64_game_frame.cpp','hle/rt64_rigid_body.cpp','common/rt64_math.cpp','hle/rt64_framebuffer_storage.cpp','hle/rt64_framebuffer_changes.cpp','preset/rt64_preset_scene.cpp','preset/rt64_preset.cpp','shared/rt64_hlsl_json.cpp']
with tempfile.TemporaryDirectory(prefix='clay-rt64-interpolation-') as temp:
 work=Path(temp)
 world=next(line.strip() for line in (src/'shaders/RSPWorldCS.hlsl').read_text().splitlines() if 'const float4 worldPos =' in line)
 process=next(line.strip() for line in (src/'shaders/RSPProcessCS.hlsl').read_text().splitlines() if 'float4 tfPos =' in line)
 header='// Original RT64 shader statements (MIT); dependency notices preserved.\n#pragma once\n#include "common/rt64_math.h"\n'
 for name,line,returned in [('world',world,'worldPos'),('process',process,'tfPos')]:
  header+='inline hlslpp::float4 clay_rt64_'+name+'_sample(hlslpp::float3 pos,hlslpp::float3 vel,float weight){using namespace hlslpp;struct {float curFrameWeight;}gConstants{weight};float4x4 worldMats[]={float4x4::identity()};unsigned transformIndex=0;float4x4 worldMat=float4x4::identity(),viewProjMat=float4x4::identity();'+line+'return '+returned+';}\n'
 (work/'clay_rt64_shader_probe.h').write_text(header)
 binary=work/'test';command=[a.cxx,'-std=c++20','-O1','-w','-UNDEBUG','-ffunction-sections','-fdata-sections','-DHLSL_CPU','-DFFX_GCC','-Wl,--gc-sections','-I'+str(work)]+['-I'+str(i) for i in include]+[str(root/'tests/rt64_interpolation.cpp')]+[str(src/s) for s in sources]+['-o',str(binary)]
 subprocess.run(command,check=True);result=subprocess.run([str(binary)],text=True,capture_output=True);print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
 evidence={'author':'djultra64','result':'PASS','actual_dependency_routines':['RT64::GameFrame::buildTransformIdMap','RT64::GameFrame::matchTransform','RT64::RigidBody update methods'],'shader_sources':['RSPWorldCS.hlsl','RSPProcessCS.hlsl'],'fractional_weights':[0,0.5,1],'simulation_rate':60,'sample_rate':120,'velocity':[8,4,0],'midpoint':[14,12,0],'negative_guards':['vertexSKIP leavesvelocitieszero','cutID doesnotmatcholdgroup'],'scope':'ActualCPU RT64workload matching and exact originalshader currentpositionexpressions using HLSL_CPU math; noGPU dispatch/presentation/monitorrefreshclaim','stdout':result.stdout}
 (root/'analysis/rt64_interpolation_evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
