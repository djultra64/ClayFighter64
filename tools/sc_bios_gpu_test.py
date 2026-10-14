#!/usr/bin/env python3
"""Private original-engine biography GPU fixture; test-only GDB scene setup, djultra64."""
from pathlib import Path
import argparse,json,os,subprocess,tempfile,time
R=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--id',type=int,required=True);p.add_argument('--seconds',type=float,default=65);a=p.parse_args();assert a.id in (0,6,11,12,16)
 out=R/'analysis'/f'sc-bios-gpu-{a.id}';out.mkdir(parents=True,exist_ok=True)
 marker=out/'scene-start.txt';marker.unlink(missing_ok=True)
 gdbscript='''set pagination off
set confirm off
set breakpoint pending on
python
import gdb,struct,time
class BioScene(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame();ram=int(frame.read_var('rdram'));ctx=frame.read_var('ctx').dereference();inf=gdb.selected_inferior()
  def word(address):return struct.unpack('<I',bytes(inf.read_memory(ram+(address&0x1fffffff),4)))[0]
  def put(address,value):inf.write_memory(ram+(address&0x1fffffff),struct.pack('<I',value&0xffffffff))
  config=word(0x800ddd40);actor_config=int(ctx['r4'])&0xffffffff
  if not (0x80000000<=config<0x80800000 and 0x80000000<=actor_config<0x80800000):raise RuntimeError('Invalid real native scene configuration')
  put(actor_config,TARGET);put(config+0x14,TARGET);put(config+0xd0,0)
  put(0x80106d68,1);put(0x80106d6c,0x8006cc38);put(0x80106d70,0x8006c6c0)
  print('TEST_ONLY_ORIGINAL_BIO_SCENE',TARGET,hex(config),hex(actor_config));open(MARKER,'w').write(str(time.time()))
  self.enabled=False;return False
BioScene('func_8003C740')
end
run
'''.replace('TARGET',str(a.id)).replace('MARKER',repr(str(marker)))
 (out/'scene.gdb').write_text(gdbscript)
 with tempfile.TemporaryDirectory(prefix='clay-bio-private-') as d:
  d=Path(d);display=d/'display';display.touch();handle=display.open('wb');serverlog=(out/'xvfb.log').open('w');server=subprocess.Popen(['Xvfb','-displayfd',str(handle.fileno()),'-screen','0','1920x1080x24','-nolisten','tcp'],pass_fds=(handle.fileno(),),stdout=serverlog,stderr=serverlog)
  game=None
  try:
   deadline=time.monotonic()+10
   while not display.read_text().strip():
    if time.monotonic()>deadline:raise RuntimeError('Private Xvfb startup timeout')
    time.sleep(.05)
   env=os.environ.copy();env.update(DISPLAY=':'+display.read_text().strip(),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',CLAY_TRACE_GAME='1',CLAY_TRACE_BIOS='1',VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json')
   log=(out/'game.log').open('w');start=time.monotonic();game=subprocess.Popen(['gdb','-q','-batch','-x',str(out/'scene.gdb'),'--args',str(R/'local/sc_bios_gpu/clayf64-bios-validation'),'--rom',str(R/'probe-output/base.z64'),'--sc-assets',str(R/'local/sc_roster/asset-image-roster.z64'),'--test-input',str(R/'local/test-empty-input.json'),'--user-data',str(out/'prefs'),'--graphics-profile','default','--seconds',str(a.seconds)],cwd=R,env=env,stdout=log,stderr=subprocess.STDOUT)
   points=[3,8,15,24];captured=[]
   while game.poll() is None:
    if time.monotonic()-start>a.seconds+30:raise RuntimeError('Private biography test timed out')
    if marker.exists() and points and time.time()-float(marker.read_text())>=points[0]:
     point=points.pop(0);ids=subprocess.run(['xdotool','search','--name','ClayFighter 64'],env=env,text=True,capture_output=True).stdout.split()
     if not ids:ids=subprocess.run(['xdotool','search','--name','ClayTality'],env=env,text=True,capture_output=True).stdout.split()
     if ids:
      image=out/f'frame-scene-{point}.png';subprocess.run(['import','-window',ids[-1],str(image)],env=env,check=True);captured.append(str(image))
    time.sleep(.1)
   log.close();events=[]
   for line in (out/'game.log').read_text().splitlines():
    try:event=json.loads(line)
    except ValueError:continue
    if event.get('event') in ('native_bio_table','experimental_sc_model','combat_loaded','progress','validation_end','fatal'):events.append(event)
   end=next((e for e in reversed(events) if e.get('event')=='validation_end'),None);bios=[e for e in events if e.get('event')=='native_bio_table']
   result={'author':'djultra64','scope':'Test-only private GDB chooses real original special-scene callbacks before original3C740 actor/stage constructor; no product hook or userprocess','character':a.id,'private_display':True,'test_only_forced_scene':True,'native_bio_events':bios,'validation_end':end,'captures':captured,'events':events,'gdb_exit_code':game.returncode,'elapsed_seconds':time.monotonic()-start}
   (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('character','native_bio_events','validation_end','captures','gdb_exit_code')}));return 0 if end and not end['stalled'] and bios else 73
  finally:
   if game and game.poll() is None:game.terminate();game.wait(timeout=10)
   server.terminate();server.wait(timeout=10);handle.close();serverlog.close()
if __name__=='__main__':raise SystemExit(main())
