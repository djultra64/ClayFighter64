#!/usr/bin/env python3
"""Isolated real original63 stage import/combat probe; author djultra64.

Only private GDB changes this test process. Original ROM, stable asset image,
launchers, generated CPU and normal executable remain untouched.
"""
from pathlib import Path
import argparse,hashlib,json,os,shutil,signal,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[1]
def number(x):return int(x,0)if isinstance(x,str)else int(x)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=int,required=True);p.add_argument('--baseline',action='store_true');p.add_argument('--image',type=Path,default=ROOT/'local/sc_stages/hybrid-image.z64');p.add_argument('--manifest',type=Path,default=ROOT/'local/sc_stages/hybrid-manifest.json');p.add_argument('--binary',type=Path,default=ROOT/'build/claytality64');p.add_argument('--seconds',type=float,default=75);p.add_argument('--input',type=Path);p.add_argument('--name');p.add_argument('--prepare-only',action='store_true');a=p.parse_args()
 assert a.stage in(0,5,11),'Only authorized bounded stage candidates0/5/11'
 base=(ROOT/'probe-output/base.z64').read_bytes();manifest=json.loads(a.manifest.read_text());data=a.image.read_bytes();stage=manifest['stages'][str(a.stage)]
 assert len(base)==0xc00000 and data[:len(base)]==base,'Experimental image must preserve EVERY original ROM byte'
 assert hashlib.sha256(base).hexdigest()==manifest['base_prefix_sha256'];assert hashlib.sha256(data).hexdigest()==manifest['image_sha256']
 patches=stage['ram_table_patches'];assert {number(x['address'])for x in patches}=={b+a.stage*4 for b in(0x800d92c0,0x800d9328,0x800d9390)}
 assert all(0xc00000<=number(x['value'])<len(data)for x in patches)
 name=a.name or f'sc-stage-{a.stage}-'+('original-control'if a.baseline else'experimental')
 out=ROOT/'analysis'/name;out.mkdir(parents=True,exist_ok=True)
 sha=hashlib.sha256(a.binary.read_bytes()).hexdigest();binary=ROOT/'local/sc_stages/runtime-binaries'/sha/'claytality64';binary.parent.mkdir(parents=True,exist_ok=True)
 if not binary.exists():shutil.copy2(a.binary,binary)
 source_hashes={str(a.binary.relative_to(ROOT)):sha,'probe-output/base.z64':hashlib.sha256(base).hexdigest(),'local/sc_roster/asset-image-roster.z64':hashlib.sha256((ROOT/'local/sc_roster/asset-image-roster.z64').read_bytes()).hexdigest(),'Jugar.sh':hashlib.sha256((ROOT/'Jugar.sh').read_bytes()).hexdigest()}
 scripted=a.input or out/'input.json'
 if not a.input:
  events=[{'at_ms':8000,'button':'start'},{'at_ms':12000,'button':'south'},{'at_ms':16000,'button':'south'},{'at_ms':18000,'button':'south'},{'at_ms':34000,'button':'right','duration_ms':3000},{'at_ms':38000,'button':'left','duration_ms':1600},{'at_ms':42000,'button':'up','duration_ms':250}]
  for t in range(44000,68000,550):events.append({'at_ms':t,'button':['south','east','west','north'][(t//550)%4],'duration_ms':220})
  scripted.write_text(json.dumps(events,indent=2)+'\n')
 script=r'''set pagination off
set confirm off
set breakpoint pending on
set debuginfod enabled off
catch throw
python
import gdb,json,struct,time,pathlib
IMAGE=pathlib.Path(IMAGE_PATH).read_bytes()
PATCHES=PATCH_DATA
TARGET=STAGE_ID
BASELINE=IS_BASELINE
OUT=pathlib.Path(OUTPUT_PATH)
rom_installed=False
pending_ram=None
forced=0
def emit(event,**values):
 row={'event':event,**values};print(json.dumps(row),flush=True)
 with(OUT/'gdb-events.jsonl').open('a')as f:f.write(json.dumps(row)+'\n')
def word(ram,addr):return struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(ram+(addr&0x1fffffff),4)))[0]
def put(ram,addr,value):gdb.selected_inferior().write_memory(ram+(addr&0x1fffffff),struct.pack('<I',value&0xffffffff))
def patch(ram):
 if BASELINE:return
 before={hex(x['address']):word(ram,x['address'])for x in PATCHES}
 for x in PATCHES:put(ram,x['address'],x['value'])
 assert all(word(ram,x['address'])==x['value']for x in PATCHES)
 emit('experimental_stage_tables',stage=TARGET,before=before,after={hex(x['address']):x['value']for x in PATCHES})
class TablesFinished(gdb.FinishBreakpoint):
 def __init__(self,frame,ram):super().__init__(frame,internal=True);self.ram=ram
 def stop(self):patch(self.ram);return False
class Bootstrap(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame();ram=int(frame.read_var('rdram'))
  if pending_ram==ram:TablesFinished(frame,ram)
  return False
class Scene(gdb.Breakpoint):
 def stop(self):
  global rom_installed,pending_ram,forced
  frame=gdb.newest_frame();ram=int(frame.read_var('rdram'));ctx=frame.read_var('ctx').dereference();config=int(ctx['r4'])&0xffffffff
  if word(ram,0x80106d58):return False
  assert 0x80000000<=config<=0x807fffE0,'Native real combat config bounds'
  if not BASELINE and not rom_installed:
   vector=gdb.parse_and_eval('clay_rom')['_M_impl'];start=int(vector['_M_start']);finish=int(vector['_M_finish']);capacity=int(vector['_M_end_of_storage'])-start
   assert finish-start==0xc00000,'Fresh original base cart required'
   assert len(IMAGE)<=capacity,'Experimental append exceeds actual host vector allocation; no inferior allocation permitted'
   # Preserve allocator ownership/capacity; only append private test bytes and
   # extend the existing vector logical length after an actual capacity check.
   gdb.selected_inferior().write_memory(start+0xc00000,IMAGE[0xc00000:])
   gdb.execute('set variable clay_rom._M_impl._M_finish = (unsigned char*)'+str(start+len(IMAGE)),to_string=True)
   emit('experimental_stage_cart_append',original_bytes=0xc00000,image_bytes=len(IMAGE),capacity=capacity)
   rom_installed=True
  pending_ram=ram
  # A87C resolves original REF aliases and maxima first. Patch ONLY the target
  # after it returns, so no other original stage inherits the test payload.
  if word(ram,0x800d9498):patch(ram)
  prior=word(ram,config+0x1c);put(ram,config+0x1c,TARGET);put(ram,0x80108cd4,TARGET)
  forced+=1;emit('test_only_stage_selection',stage=TARGET,previous=prior,combat_config=hex(config),scene=forced,baseline=BASELINE)
  return False
class Loader(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame();ram=int(f.read_var('rdram'));ctx=f.read_var('ctx').dereference();stage=int(ctx['r4'])&0xffffffff
  emit('original_stage_loader',stage=stage,definition_start=word(ram,0x800d92c0+stage*4),definition_end=word(ram,0x800d9328+stage*4),mesh_start=word(ram,0x800d9390+stage*4));return False
class Loaded(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame();ram=int(f.read_var('rdram'))
  emit('original_stage_combat_ready',stage=word(ram,0x80108cd4),p1=word(ram,0x801028a8),p2=word(ram,0x801028b0),attract=bool(word(ram,0x80106d58)));return False
Scene('func_8003C740',internal=True)
Bootstrap('func_8000A87C',internal=True)
Loader('func_8000B250',internal=True)
Loaded('clay::input::begin_combat_input',internal=True)
def exited(event):
 code=getattr(event,'exit_code',None);(OUT/'inferior-exit.json').write_text(json.dumps({'exit_code':code}))
gdb.events.exited.connect(exited)
end
run
thread apply all bt 24
quit
'''
 for key,value in {'IMAGE_PATH':repr(str(a.image.resolve())),'PATCH_DATA':repr([{'address':number(x['address']),'value':number(x['value'])}for x in patches]),'STAGE_ID':str(a.stage),'IS_BASELINE':repr(a.baseline),'OUTPUT_PATH':repr(str(out))}.items():script=script.replace(key,value)
 (out/'probe.gdb').write_text(script);(out/'gdb-events.jsonl').unlink(missing_ok=True);(out/'inferior-exit.json').unlink(missing_ok=True)
 if a.prepare_only:print(out/'probe.gdb');return
 with tempfile.TemporaryDirectory(prefix='clay-private-stage-')as tmp:
  tmp=Path(tmp);display=tmp/'display';display.touch();handle=display.open('wb');xlog=(out/'xvfb.log').open('w');x=subprocess.Popen(['Xvfb','-displayfd',str(handle.fileno()),'-screen','0','1280x720x24','-nolisten','tcp'],pass_fds=(handle.fileno(),),stdout=xlog,stderr=xlog);game=None
  try:
   until=time.monotonic()+10
   while not display.read_text().strip():
    if x.poll()is not None or time.monotonic()>until:raise RuntimeError('Private Xvfb failed')
    time.sleep(.05)
   env=os.environ.copy();env.update(DISPLAY=':'+display.read_text().strip(),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',CLAY_TRACE_GAME='1',CLAY_TRACE_INPUT='1')
   with(out/'game.log').open('w')as log:
    game=subprocess.Popen(['gdb','-q','-batch','-x',str(out/'probe.gdb'),'--args',str(binary),'--rom',str(ROOT/'probe-output/base.z64'),'--test-input',str(scripted),'--seconds',str(a.seconds),'--graphics-profile','original','--user-data',str(out/'prefs')],env=env,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    start=time.monotonic();points=[30,40,55,a.seconds-3];captures=[]
    while game.poll()is None:
     elapsed=time.monotonic()-start
     if elapsed>a.seconds+35:raise RuntimeError('Native stage probe timed out')
     if points and elapsed>=points[0]:
      point=points.pop(0);ids=subprocess.run(['xdotool','search','--onlyvisible','--name','Clay'],env=env,text=True,capture_output=True).stdout.split()
      if ids:
       image=out/f'frame-{point}.png';subprocess.run(['import','-window',ids[-1],str(image)],env=env,capture_output=True,check=True);captures.append(str(image))
     time.sleep(.1)
    game.wait(timeout=10)
   events=[]
   for path in(out/'game.log',out/'gdb-events.jsonl'):
    for line in path.read_text().splitlines():
     try:e=json.loads(line)
     except ValueError:continue
     if e.get('event')in('test_only_stage_selection','original_stage_loader','original_stage_combat_ready','experimental_stage_tables','experimental_stage_cart_append','guest_input','combat_loaded','combat_health','combat_finished','progress','validation_end','fatal'):events.append(e)
   exit_status=json.loads((out/'inferior-exit.json').read_text())if(out/'inferior-exit.json').exists()else{'exit_code':None}
   healthy=any(e.get('event')=='validation_end'and e['exit_code']==0 and not e['stalled']for e in events)
   combat=any(e.get('event')=='original_stage_combat_ready'and e['stage']==a.stage and not e['attract']for e in events)
   end_hashes={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest()for path in source_hashes}
   unchanged=end_hashes==source_hashes
   report={'author':'djultra64','scope':'original63 control'if a.baseline else stage['scope'],'stage':a.stage,'base_name':stage['base_name'],'sc_name':stage['sc_name'],'binary_sha256':sha,'experimental_image_sha256':manifest['image_sha256'],'original_sources_unchanged':unchanged,'actual_native_stage_combat':combat,'healthy_validation_end':healthy,'inferior_exit_code':exit_status['exit_code'],'gdb_exit_code':game.returncode,'captures':captures,'events':events,'passed':healthy and combat and unchanged and exit_status['exit_code']==0}
   (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('events','captures')},indent=2))
   if not report['passed']:raise SystemExit(1)
  finally:
   if game and game.poll()is None:os.killpg(game.pid,signal.SIGTERM);game.wait(timeout=10)
   x.terminate();x.wait(timeout=10);handle.close();xlog.close()
if __name__=='__main__':main()
