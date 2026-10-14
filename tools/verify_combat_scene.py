#!/usr/bin/env python3
"""Visible hardware combat verification, retaining its inferior afterwards. djultra64."""
import argparse,hashlib,json,os,re,shutil,signal,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--after',type=float,default=600);a=p.parse_args()
source=ROOT/'build/claytality64';digest=hashlib.sha256(source.read_bytes()).hexdigest();binary=ROOT/'local/debug-binaries'/digest/'claytality64';binary.parent.mkdir(parents=True,exist_ok=True)
if not binary.exists():shutil.copy2(source,binary)
text=(ROOT/'generated/guest/funcs_5.c').read_text().splitlines()
exit_line=next(n+1 for n,line in enumerate(text)if '0x8003DB00:' in line)+1
commands=ROOT/'analysis/combat-fixed-verification.gdb'
commands.write_text('set pagination off\ncatch throw\nbreak func_8003D940\ncommands\nsilent\nprintf "CLAY_SCENE_CLEANUP_ENTER\\n"\ncontinue\nend\nbreak '+str(ROOT/'generated/guest/funcs_5.c')+':'+str(exit_line)+'\ncommands\nsilent\nprintf "CLAY_ASYNC_WAIT_COMPLETED\\n"\ncontinue\nend\nrun\nthread apply all bt 24\ninfo inferiors\nsource '+str(ROOT/'tools/thread_snapshot.gdb')+'\ndetach\nquit\n')
logpath=ROOT/'analysis/combat-fixed-debug.log';started=time.monotonic()
with logpath.open('w')as log:
 gdb=subprocess.Popen(['gdb','-batch','-x',str(commands),'--args',str(binary),'--seconds','0'],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
 while gdb.poll()is None and time.monotonic()-started<a.after:time.sleep(1)
 if gdb.poll()is None:os.kill(gdb.pid,signal.SIGINT)
 try:gdb.wait(timeout=20)
 except subprocess.TimeoutExpired:
  os.kill(gdb.pid,signal.SIGTERM);gdb.wait(timeout=5)
content=logpath.read_text(errors='replace');progress=[]
for line in content.splitlines():
 if '"event":"progress"'in line:
  try:progress.append(json.loads(line))
  except ValueError:pass
match=re.search(r'\[Inferior \d+ \(process (\d+)\) detached\]',content)
result={'author':'djultra64','binary_sha256':digest,'elapsed_seconds':round(time.monotonic()-started,2),'gdb_exit':gdb.returncode,'cleanup_entries':content.count('CLAY_SCENE_CLEANUP_ENTER'),'async_wait_completions':content.count('CLAY_ASYNC_WAIT_COMPLETED'),'progress':progress,'detached_game_pid':int(match.group(1))if match else None,'exception_caught':'hit Catchpoint'in content}
(ROOT/'analysis/combat-fixed-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='progress'}))
