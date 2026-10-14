#!/usr/bin/env python3
"""Run native Vulkan and real scripted SDL input on a private Linux desktop. djultra64."""
import argparse,json,os,subprocess,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--name',default='native-hidden');p.add_argument('--seconds',type=float,default=35);p.add_argument('--input',type=Path,required=True);p.add_argument('--capture',type=float,nargs='*',default=[]);p.add_argument('game_args',nargs=argparse.REMAINDER);a=p.parse_args()
 out=ROOT/'analysis'/a.name;out.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix='clay-private-display-') as temp:
  path=Path(temp)/'display';displayfile=path.open('wb');xlog=(out/'xvfb.log').open('w');server=subprocess.Popen(['Xvfb','-displayfd',str(displayfile.fileno()),'-screen','0','1920x1080x24','-nolisten','tcp'],pass_fds=(displayfile.fileno(),),stdout=xlog,stderr=xlog)
  game=None
  try:
   deadline=time.monotonic()+10
   while not path.read_text().strip():
    if server.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Private display did not start')
    time.sleep(.05)
   env=os.environ.copy();env.update(DISPLAY=':'+path.read_text().strip(),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',CLAY_TRACE_INPUT='1',CLAY_TRACE_GAME='1')
   # Xvfb lacks DRI3 hardware presentation; use software only for private visual checks.
   icd='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json'
   if Path(icd).exists():env['VK_ICD_FILENAMES']=icd
   args=a.game_args[1:] if a.game_args[:1]==['--'] else a.game_args
   logfile=(out/'game.log').open('w');start=time.monotonic();game=subprocess.Popen([str(ROOT/'build/claytality64'),'--rom',str(ROOT/'probe-output/base.z64'),'--test-input',str(a.input.resolve()),'--seconds',str(a.seconds),*args],cwd=ROOT,env=env,stdout=logfile,stderr=subprocess.STDOUT)
   captures=sorted(a.capture)
   while game.poll() is None:
    elapsed=time.monotonic()-start
    if elapsed>a.seconds+30:raise RuntimeError('Native game failed to terminate after bounded hidden validation')
    if captures and elapsed>=captures[0]:
     point=captures.pop(0);search=subprocess.run(['xdotool','search','--pid',str(game.pid)],env=env,capture_output=True,text=True)
     ids=search.stdout.split()
     if ids:subprocess.run(['import','-window',ids[-1],str(out/f'frame-{point:g}.png')],env=env,capture_output=True)
    time.sleep(.1)
   logfile.close();events=[]
   for line in (out/'game.log').read_text().splitlines():
    try:e=json.loads(line)
    except ValueError:continue
    if e.get('event') in ('rt64_device_ready','graphics_requested','graphics_effective','combat_loaded','combat_input_mode','experimental_sc_model','experimental_sc_roster_portraits','progress','guest_input','validation_physical_input','validation_end','fatal'):events.append(e)
   progress=[e for e in events if e['event']=='progress'];advancing=len(progress)>=2 and progress[-1]['gfx']>progress[-2]['gfx'] and progress[-1]['polls']>progress[-2]['polls'];end=next((e for e in reversed(events) if e['event']=='validation_end'),None);healthy=end is not None and not end['stalled'] and end['exit_code']==0;code=game.returncode if game.returncode!=0 or a.seconds<10 or (advancing and healthy) else 73
   report={'author':'djultra64','progress_advancing':advancing,'native_liveness_verified':healthy,'exit_code':code,'elapsed_seconds':time.monotonic()-start,'private_display':True,'input_script':str(a.input),'events':events};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
   print(json.dumps({'name':a.name,'exit_code':code,'seconds':round(report['elapsed_seconds'],1),'combat_loads':[e for e in events if e['event']=='combat_loaded'],'gfx_progress':[e for e in events if e['event']=='progress'][-2:]}));return code
  finally:
   if game and game.poll() is None:game.terminate();game.wait(timeout=10)
   server.terminate();server.wait(timeout=10);displayfile.close();xlog.close()
if __name__=='__main__':raise SystemExit(main())
