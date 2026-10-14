#!/usr/bin/env python3
"""Read actual private native GPU frames during unforced attract playback; djultra64."""
from pathlib import Path
import argparse,ctypes as C,json,os,shutil,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[1]
class XImage(C.Structure):
 _fields_=[('width',C.c_int),('height',C.c_int),('xoffset',C.c_int),('format',C.c_int),('data',C.c_void_p),('byte_order',C.c_int),('bitmap_unit',C.c_int),('bitmap_bit_order',C.c_int),('bitmap_pad',C.c_int),('depth',C.c_int),('bytes_per_line',C.c_int),('bits_per_pixel',C.c_int),('red_mask',C.c_ulong),('green_mask',C.c_ulong),('blue_mask',C.c_ulong),('obdata',C.c_void_p),('functions',C.c_void_p*6)]
def main():
 from PIL import Image
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--seconds',type=float,default=180);p.add_argument('--name',default='normal-attract-white');p.add_argument('--exe',type=Path,default=ROOT/'build/claytality64');a=p.parse_args();out=ROOT/'analysis'/a.name;out.mkdir(parents=True,exist_ok=True);exe=ROOT/'local'/('clay-'+a.name);shutil.copy2(a.exe,exe)
 x=C.CDLL('libX11.so.6');x.XOpenDisplay.argtypes=[C.c_char_p];x.XOpenDisplay.restype=C.c_void_p;x.XGetImage.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_int,C.c_uint,C.c_uint,C.c_ulong,C.c_int];x.XGetImage.restype=C.POINTER(XImage);x.XGetPixel.argtypes=[C.POINTER(XImage),C.c_int,C.c_int];x.XGetPixel.restype=C.c_ulong;x.XDestroyImage.argtypes=[C.POINTER(XImage)];x.XCloseDisplay.argtypes=[C.c_void_p]
 def rgb(image,px,py):
  value=x.XGetPixel(image,px,py);return[(value>>16)&255,(value>>8)&255,value&255]
 with tempfile.TemporaryDirectory(prefix='clay-private-attract-')as temp:
  number=Path(temp)/'display';fd=number.open('wb');xlog=(out/'xvfb.log').open('w');server=subprocess.Popen(['Xvfb','-displayfd',str(fd.fileno()),'-screen','0','1920x1080x24','-nolisten','tcp'],pass_fds=(fd.fileno(),),stdout=xlog,stderr=xlog);game=None;display=None
  try:
   deadline=time.monotonic()+10
   while not number.read_text().strip():
    if server.poll()is not None or time.monotonic()>deadline:raise RuntimeError('Private display failed')
    time.sleep(.05)
   address=':'+number.read_text().strip();display=x.XOpenDisplay(address.encode());assert display
   env=os.environ.copy();env.update(DISPLAY=address,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',CLAY_TRACE_UI='1',CLAY_TRACE_GAME='1',VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json')
   log=(out/'game.log').open('w');started=time.monotonic();game=subprocess.Popen([str(exe),'--rom',str(ROOT/'probe-output/base.z64'),'--user-data',str((ROOT/'local'/a.name/'prefs').resolve()),'--seconds',str(a.seconds)],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT);window=None;frames=0;white=[];partial=[];last_capture=0;size=(1280,720)
   while game.poll()is None:
    elapsed=time.monotonic()-started
    if elapsed>a.seconds+30:raise RuntimeError('Native attract process exceeded bounded time')
    if window is None:
     ids=subprocess.run(['xdotool','search','--pid',str(game.pid)],env=env,capture_output=True,text=True).stdout.split()
     if ids:
      window=int(ids[-1]);geometry=subprocess.run(['xdotool','getwindowgeometry','--shell',str(window)],env=env,capture_output=True,text=True).stdout;values=dict(line.split('=',1)for line in geometry.splitlines()if'='in line);size=(int(values['WIDTH']),int(values['HEIGHT']))
    if window and elapsed>5:
     w,h=size;image=x.XGetImage(display,window,0,0,w,h,C.c_ulong(-1).value,2)
     if image:
      frames+=1;points=[(1,1),(w-2,1),(1,h-2),(w-2,h-2),(w//2,h//2)];colors=[rgb(image,*point)for point in points];bright=all(min(color)>=245 for color in colors)
      center=min(colors[-1])>=245;clipped=center and any(max(color)<150 for color in colors[:4])
      if (bright or clipped)and elapsed-last_capture>.12:
       grid=[rgb(image,int((i+.5)*w/32),int((j+.5)*h/18))for j in range(18)for i in range(32)];fraction=sum(min(color)>=245 for color in grid)/len(grid)
       if (bright and fraction>.99)or(clipped and fraction>.4):
        kind='white-full'if bright else'white-clipped';record={'seconds':round(elapsed,3),'corners_rgb':colors[:4],'center_rgb':colors[-1],'grid_white_fraction':fraction,'framebuffer_size':size,'source':'Single atomicXGetImage of actual native Vulkan presentation; noinput/RAM/testeffect injection'}
        filename=f'{kind}-{elapsed:.3f}.png';record['capture']=filename
        if image.contents.bits_per_pixel==32 and image.contents.red_mask==0xff0000:Image.frombytes('RGB',size,C.string_at(image.contents.data,image.contents.bytes_per_line*h),'raw','BGRX',image.contents.bytes_per_line).save(out/filename)
        (white if bright else partial).append(record);last_capture=elapsed;(out/'live-white.json').write_text(json.dumps({'white':white,'partial':partial},indent=2)+'\n')
      x.XDestroyImage(image)
    time.sleep(.04)
   log.close();events=[]
   for line in(out/'game.log').read_text().splitlines():
    try:e=json.loads(line)
    except ValueError:continue
    if e.get('event')in('progress','validation_end','combat_loaded','native_ui_coverage','fatal'):events.append(e)
   ending=next((e for e in reversed(events)if e['event']=='validation_end'),None);healthy=bool(ending and not ending['stalled']and ending['exit_code']==0);report={'author':'djultra64','private_display':True,'unforced_empty_input':True,'sampled_actual_gpu_frames':frames,'white_frames':white,'partial_white_frames':partial,'native_liveness_verified':healthy,'exit_code':game.returncode,'events':events};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'white_frames':len(white),'clipped_frames':len(partial),'sampled_gpu_frames':frames,'healthy':healthy,'exit_code':game.returncode}));return game.returncode or(0 if healthy else 73)
  finally:
   if game and game.poll()is None:game.terminate();game.wait(timeout=10)
   if display:x.XCloseDisplay(display)
   server.terminate();server.wait(timeout=10);fd.close();xlog.close()
if __name__=='__main__':raise SystemExit(main())
