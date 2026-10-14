#!/usr/bin/env python3
"""Private natural title/attract/logo reload diagnostic, djultra64."""
from pathlib import Path
import os,subprocess
R=Path(__file__).resolve().parents[1]
original_run=subprocess.run
def bounded_capture(args,*pos,**kw):
 if args and str(args[0]) in ('import','xdotool'):kw.setdefault('timeout',3)
 try:return original_run(args,*pos,**kw)
 except subprocess.TimeoutExpired:return subprocess.CompletedProcess(args,124,stdout='',stderr='Private capture timeout')
os.environ['CLAY_TRACE_LOGO']='1'
subprocess.run=bounded_capture
source=(R/'tools/headless_test.py').read_text().replace("ROOT/'build/claytality64'","ROOT/'local/logo64/clayf64-logo-loop'")
source=source.replace('captures=sorted(a.capture)','captures=sorted(a.capture);return_offset=0;return_captured=False')
needle='    if captures and elapsed>=captures[0]:'
insertion='''    if not return_captured:
     with (out/'game.log').open() as logo_log:
      logo_log.seek(return_offset);new_lines=logo_log.readlines();return_offset=logo_log.tell()
     for line in new_lines:
      try:event=json.loads(line)
      except ValueError:continue
      if event.get('event')!='native_logo64' or event.get('reason')!='cache':continue
      if event.get('prefix','00030000')[-2:]=='00' and event.get('flags','01010100')[-2:]=='00':continue
      points=[round(elapsed+i*.3,3) for i in range(16)]
      event['capture_seconds']=points
      (out/'native-return-event.json').write_text(json.dumps(event,indent=2)+'\\n')
      captures=sorted(captures+points);return_captured=True;break
'''
assert needle in source;source=source.replace(needle,insertion+needle)
exec(compile(source,str(R/'tools/headless_test.py'),'exec'),{'__name__':'__main__','__file__':str(R/'tools/headless_test.py')})
