#!/usr/bin/env python3
"""Private native selector Back failure capture; djultra64."""
from pathlib import Path
import os,subprocess,time,tempfile,signal,hashlib,shutil,json,argparse
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--name',default='selector-back-diag');parser.add_argument('--input',default='local/selector-back/input.json');parser.add_argument('--after',type=float,default=45);parser.add_argument('--keyboard',action='store_true');parser.add_argument('--native',action='store_true');parser.add_argument('--trace-reset',action='store_true');args=parser.parse_args();out=ROOT/'analysis'/args.name;out.mkdir(exist_ok=True,parents=True)
source=ROOT/'build/claytality64';sha=hashlib.sha256(source.read_bytes()).hexdigest();binary=ROOT/'local/selector-back'/sha/'claytality64';binary.parent.mkdir(exist_ok=True,parents=True)
if not binary.exists():shutil.copy2(source,binary)
(out/'binary.sha256').write_text(sha+'\n')
snapshot=out/'snapshot.gdb';snapshot.write_text((ROOT/'tools/thread_snapshot.gdb').read_text().replace('analysis/debug-rdram.bin',str(out/'rdram.bin')))
with tempfile.TemporaryDirectory(prefix='clay-selector-back-')as temporary:
 path=Path(temporary)/'display';display=path.open('wb');xlog=(out/'xvfb.log').open('w');x=subprocess.Popen(['Xvfb','-displayfd',str(display.fileno()),'-screen','0','1920x1080x24','-nolisten','tcp'],pass_fds=(display.fileno(),),stdout=xlog,stderr=xlog)
 game=None
 try:
  while not path.read_text().strip():
   if x.poll()is not None:raise RuntimeError('PrivateXvfb failed')
   time.sleep(.05)
  env=os.environ.copy();env.update(DISPLAY=':'+path.read_text().strip(),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',CLAY_TRACE_INPUT='1',CLAY_TRACE_GAME='1',CLAY_TRACE_INPUT_SDL='1')
  commands=['gdb','-q','-batch','-ex','set pagination off','-ex','set debuginfod enabled off','-ex','catch throw','-ex','run','-ex','thread apply all bt 32','-ex','info registers','-x',str(snapshot),'--args',str(binary),'--rom','probe-output/base.z64','--sc-assets','local/sc_roster/asset-image-roster.z64','--test-input',args.input,'--seconds',str(args.after+20),'--user-data',str(ROOT/'local/selector-back'/('prefs-'+args.name))]
  if args.trace_reset:
   trace=out/'trace-reset.gdb';trace.write_text('break clay_sc_roster_install\ncommands\nsilent\nprintf "CLAY_SELECTOR_INSTALL source=%x copy=%x\\n", grid_source, grid_copy\ncontinue\nend\n')
   commands[commands.index('-ex')+2:commands.index('-ex')+2]=['-x',str(trace)]
  if args.keyboard:
   at=commands.index('--test-input');del commands[at:at+2]
  keymap={'start':'Return','south':'z','east':'x','right':'Right','left':'Left','up':'Up','down':'Down'}
  keys=[(e['at_ms'],e.get('key',keymap.get(e.get('button'))),e.get('duration_ms',200)) for e in json.loads((ROOT/args.input).read_text()) if e.get('player',1)==1]
  if args.native:
   commands=commands[commands.index('--args')+1:];commands[commands.index('--seconds')+1]=str(args.after)
  with(out/'gdb.log').open('w')as log:
   game=subprocess.Popen(commands,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);start=time.monotonic();shots=[20,26,35,args.after-2]
   while game.poll()is None and time.monotonic()-start<args.after+(10 if args.native else 0):
    elapsed=time.monotonic()-start
    if args.keyboard and keys and elapsed*1000>=keys[0][0]:
     moment,key,duration=keys.pop(0);ids=subprocess.run(['xdotool','search','--onlyvisible','--name','Clay'],env=env,capture_output=True,text=True).stdout.split()
     if ids:
      subprocess.run(['xdotool','windowfocus','--sync',ids[-1]],env=env,check=True);subprocess.run(['xdotool','keydown',key],env=env,check=True);time.sleep(duration/1000);subprocess.run(['xdotool','keyup',key],env=env,check=True)
      print(json.dumps({'keyboard':key,'scheduled_ms':moment,'private_display':env['DISPLAY']}),flush=True)
    if shots and elapsed>=shots[0]:
     t=shots.pop(0);ids=subprocess.run(['xdotool','search','--onlyvisible','--name','Clay'],env=env,capture_output=True,text=True).stdout.split()
     if ids:subprocess.run(['import','-window',ids[-1],str(out/f'frame-{t}.png')],env=env,capture_output=True)
    time.sleep(.1)
   if game.poll()is None:os.kill(game.pid,signal.SIGINT)
   game.wait(timeout=15)
   events=[]
   for line in(out/'gdb.log').read_text().splitlines():
    try:event=json.loads(line)
    except ValueError:continue
    if event.get('event')in('guest_input','progress','validation_end','fatal'):events.append(event)
   result={'log':str(out/'gdb.log'),'process_exit':game.returncode,'binary_sha256':sha,'real_keyboard':args.keyboard,'parent_gdb':not args.native,'events':events}
   (out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='events'}))
 finally:
  if game and game.poll()is None:os.killpg(game.pid,signal.SIGTERM);game.wait(timeout=10)
  x.terminate();x.wait(timeout=10);display.close();xlog.close()
