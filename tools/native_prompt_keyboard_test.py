#!/usr/bin/env python3
"""Private real SDL/X11 keyboard proof of native Options keycaps; djultra64."""
from pathlib import Path
import argparse,json,os,shutil,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,default=ROOT/'build/claytality64');p.add_argument('--name',default='keyboard-native-prompts');p.add_argument('--reload',action='store_true');p.add_argument('--display',action='store_true');p.add_argument('--game-display',action='store_true');p.add_argument('--no-create-prefs',action='store_true');a=p.parse_args();out=ROOT/'analysis'/(a.name+('-reload'if a.reload else''));out.mkdir(parents=True,exist_ok=True)
 exe=ROOT/'local'/('clay-keyboard-'+a.name);shutil.copy2(a.exe,exe)
 if not a.no_create_prefs:(ROOT/'local'/a.name/'prefs').mkdir(parents=True,exist_ok=True)
 actions=[(1.5,'Return'),(3,'Return'),(8,'z'),(10.5,'Down'),(12,'z'),(17.5,'Down'),(18.1,'Down'),(18.7,'Down'),(19.3,'Down'),(19.9,'Down'),(20.5,'Down'),(22.5,'z'),(28,'z'),(29,'q'),(31,'x')]
 captures=[16.5,27.5,30.5,33];seconds=36
 if a.reload:actions=[item for item in actions if item[0]<28];captures=[16.5,29,33]
 if a.display:actions=[item for item in actions if item[0]<22]+[(21.1,'Down'),(23,'z'),(28.5,'z'),(29.2,'Right'),(30,'z'),(32,'x')];captures=[16.5,27.5,30.7,34]
 if a.game_display:actions=[item for item in actions if item[0]<22]+[(21.1,'Down'),(23,'z'),(30,'Down'),(31,'Down'),(32,'Down'),(33,'z'),(35,'z'),(37,'x'),(39,'x')];captures=[16.5,28,34,36,38,40];seconds=43
 with tempfile.TemporaryDirectory(prefix='clay-private-keyboard-')as temp:
  number=Path(temp)/'display';fd=number.open('wb');xlog=(out/'xvfb.log').open('w');server=subprocess.Popen(['Xvfb','-displayfd',str(fd.fileno()),'-screen','0','1920x1080x24','-nolisten','tcp'],pass_fds=(fd.fileno(),),stdout=xlog,stderr=xlog);game=None
  try:
   deadline=time.monotonic()+10
   while not number.read_text().strip():
    if server.poll()is not None or time.monotonic()>deadline:raise RuntimeError('Private display failed')
    time.sleep(.05)
   env=os.environ.copy();env.update(DISPLAY=':'+number.read_text().strip(),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',CLAY_TRACE_INPUT='1',CLAY_TRACE_GAME='1',VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json')
   log=(out/'game.log').open('w');start=time.monotonic();game=subprocess.Popen([str(exe),'--rom',str(ROOT/'probe-output/base.z64'),'--user-data',str((ROOT/'local'/a.name/'prefs').resolve()),'--seconds',str(seconds)],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT);window=None;issued=[]
   while game.poll()is None:
    elapsed=time.monotonic()-start
    if elapsed>seconds+25:raise RuntimeError('Native keyboard test failed to terminate')
    if window is None:
     ids=subprocess.run(['xdotool','search','--pid',str(game.pid)],env=env,capture_output=True,text=True).stdout.split()
     if ids:window=ids[-1];subprocess.run(['xdotool','windowfocus',window],env=env,check=True)
    if window and actions and elapsed>=actions[0][0]:
     point,key=actions.pop(0);subprocess.run(['xdotool','keydown',key],env=env,check=True);time.sleep(.16);subprocess.run(['xdotool','keyup',key],env=env,check=True);issued.append({'at_seconds':point,'key':key,'backend':'XTest->SDL keyboard state'})
    if window and captures and elapsed>=captures[0]:
     point=captures.pop(0);subprocess.run(['import','-window',window,str(out/f'frame-{point:g}.png')],env=env,check=True)
    time.sleep(.04)
   log.close();events=[]
   for line in(out/'game.log').read_text().splitlines():
    try:e=json.loads(line)
    except ValueError:continue
    if e.get('event')in('native_options_state','native_options_draw','native_options_change','guest_input','validation_end','progress','input_binding','fatal'):events.append(e)
   ending=next((e for e in reversed(events)if e['event']=='validation_end'),None);healthy=ending and not ending['stalled']and ending['exit_code']==0
   report={'author':'djultra64','private_display':True,'real_keyboard_events':issued,'exit_code':game.returncode,'liveness_verified':bool(healthy),'events':events};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'exit_code':game.returncode,'liveness_verified':bool(healthy),'options_draw':[e for e in events if e['event']=='native_options_draw']}));return game.returncode or(0 if healthy else 73)
  finally:
   if game and game.poll()is None:game.terminate();game.wait(timeout=10)
   server.terminate();server.wait(timeout=10);fd.close();xlog.close()
if __name__=='__main__':raise SystemExit(main())
