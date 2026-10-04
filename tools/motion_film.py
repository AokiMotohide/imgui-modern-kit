"""ImKit motion film revision: actual continuous UI, kinetic type, original music.
Only the final MP4/poster/provenance are published; raw frames stream through pipes.
"""
from pathlib import Path
import argparse,json,math,subprocess,wave,hashlib,re
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageFilter
ROOT=Path(__file__).resolve().parents[1]
W,H,FPS,SECONDS=1920,1080,60,60
BG=(16,26,32);MINT=(169,231,203);BLUE=(144,185,255);GOLD=(246,200,121);WHITE=(242,250,246);MUTED=(163,184,185)
BEAT=60/128
FONTS={}
SHOTS=[0,1.6,3.5,5,7.5,10.5,14,16.8,20,22.6,25.2,28,30.5,33,35,37.5,39,41.5,44,47,49,51,53,55,57.5]
THEME_BEATS=[39,40.40625,41.34375,42.28125,43.21875,44]

def font(size,bold=False):
 key=(int(size),bold)
 if key not in FONTS:FONTS[key]=ImageFont.truetype(str(ROOT/'assets/fonts'/('Inter-SemiBold.ttf' if bold else 'Inter-Regular.ttf')),key[0])
 return FONTS[key]
def ease(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def mix(a,b,u):return tuple(x+(y-x)*u for x,y in zip(a,b))
def text(im,xy,s,size=48,color=WHITE,bold=False):ImageDraw.Draw(im).text(tuple(map(int,xy)),s,font=font(size,bold),fill=color)

class Clip:
 def __init__(self,path):
  self.path=path;self.proc=None;self.index=-1;self.image=None
  info=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_format','-of','json',str(path)]));self.duration=float(info['format']['duration'])
 def close(self):
  if self.proc:
   self.proc.stdout.close();self.proc.terminate();self.proc.wait();self.proc=None
 def frame(self,t):
  target=int(t*FPS+1e-6)
  if t<0 or t>=self.duration:raise ValueError(f'Source too short: {self.path.name}, requested {t}, available {self.duration}')
  if self.image is not None and target==self.index:return self.image
  if self.proc is None or target<self.index or target-self.index>120:
   self.close();self.proc=subprocess.Popen(['ffmpeg','-v','error','-threads','2','-ss',f'{target/FPS:.9f}','-i',str(self.path),'-f','rawvideo','-pix_fmt','rgb24','pipe:1'],stdout=subprocess.PIPE);self.index=target-1
  while self.index<target:
   raw=self.proc.stdout.read(W*H*3)
   if len(raw)!=W*H*3:raise RuntimeError(f'Missing native frame: {self.path} {target}')
   self.image=Image.frombytes('RGB',(W,H),raw);self.index+=1
  return self.image

class Film:
 def __init__(self,inputs):
  self.clips={name:Clip(inputs/f'{name}.mkv') for name in ['comparison','workspace','icons','themes','toasts','timeline','node-editor-final']}
  self.events={name:[json.loads(line) for line in Path(str(c.path)+'.jsonl').read_text().splitlines() if '"action"' in line] for name,c in self.clips.items()}
  rows=[json.loads(line) for line in Path(str(self.clips['themes'].path)+'.jsonl').read_text().splitlines()]
  self.theme_times=[0]+[e['frame']/FPS for e in rows if e.get('check')=='theme selection']+[self.clips['themes'].duration-1/FPS]
  if len(self.theme_times)!=len(THEME_BEATS):raise ValueError('Recapture four verified theme selections before rendering')
  self.icons=[]
  for p in sorted((ROOT/'assets/icons/originals').rglob('*.png'))[::3][:96]:
   icon=Image.open(p).convert('RGBA');icon.thumbnail((72,72));self.icons.append(icon)
   # Repository icons include light/dark artwork. Use one readable monochrome
   # raster treatment in this original motion-graphics layer, preserving alpha.
   ink=Image.new('RGBA',icon.size,(16,32,35));ink.putalpha(icon.getchannel('A'));self.icons[-1]=ink
  # Layer details are cached once, not thousands of full-resolution frames.
  self.noise=Image.new('RGB',(W,H),BG)
  d=ImageDraw.Draw(self.noise)
  for y in range(H):
   k=max(0,1-math.hypot((y-420)/1500,.4));d.line((0,y,W,y),fill=(int(16+5*k),int(26+12*k),int(32+13*k)))
 def close(self):
  for c in self.clips.values():c.close()
 def native(self,name,t,crop,size):return self.clips[name].frame(t).crop(tuple(map(int,crop))).resize(tuple(map(int,size)),Image.Resampling.LANCZOS)
 def card(self,im,content,xy,r=24,tilt=0):
  x,y=map(int,xy);w,h=content.size
  if tilt:
   # A brief perspective move on entry, returning to a square readable viewport.
   shift=int(tilt*90);content=content.transform((w,h),Image.Transform.QUAD,(0,shift,0,h-shift,w,h,w,0),Image.Resampling.BICUBIC)
  d=ImageDraw.Draw(im);d.rounded_rectangle((x-2,y+12,x+w+2,y+h+18),r+3,fill=(8,17,22))
  mask=Image.new('L',(w,h));ImageDraw.Draw(mask).rounded_rectangle((0,0,w-1,h-1),r,fill=255);im.paste(content,(x,y),mask)
  ImageDraw.Draw(im).rounded_rectangle((x,y,x+w,y+h),r,outline=(74,102,105),width=2)
 def background(self,t):
  im=self.noise.copy();d=ImageDraw.Draw(im)
  for i in range(9):
   x=int((i*293+t*40)%2200)-160;y=int(160+math.sin(t*.45+i)*320+i*80)
   d.line((x,y,x+260,y-130),fill=(29,49,57),width=1)
  pulse=max(0,1-(t%BEAT)/.16)
  d.ellipse((1460-int(pulse*12),-230,2220,530+int(pulse*12)),outline=(39,71,76),width=3)
  return im
 def heading(self,im,kicker,title,t,small=None):
  offset=int(32*(1-ease(t/.5)))
  text(im,(88,56+offset),kicker,21,MINT,True);text(im,(86,96+offset),title,62,WHITE,True)
  if small:text(im,(90,185),small,25,MUTED)
 def icon_cloud(self,im,t,area=(0,0,W,H),collapse=False):
  x,y,w,h=area;d=ImageDraw.Draw(im)
  for i,icon in enumerate(self.icons):
   row,col=divmod(i,8);phase=ease((t-i*.012)/.65)
   xx=x+(col+.5)*w/8;yy=y+(row+.5)*h/12
   if collapse:
    u=ease(t/1.9);xx=xx*(1-u)+1550*u;yy=yy*(1-u)+520*u
   else:
    xx+=(1-phase)*math.cos(i*2.4)*360;yy+=(1-phase)*math.sin(i*2.4)*220
    yy+=math.sin(t*2+i*.7)*2;xx+=math.cos(t*1.6+i*.5)*4
   n=int(38+6*phase);tile=icon.resize((n,n),Image.Resampling.LANCZOS)
   if collapse:n=max(2,int(n*(1-ease((t-.6)/1.5))));tile=icon.resize((n,n))
   if n>4:
    d.rounded_rectangle((xx-n*.64,yy-n*.64,xx+n*.64,yy+n*.64),12,fill=(221,239,229))
    im.paste(tile,(int(xx-n/2),int(yy-n/2)),tile)
 def frame(self,t):
  im=self.background(t);start=max(s for s in SHOTS if s<=t);local=t-start
  if t<5:
   # The actual shared slider reacts on both sides in the first seconds.
   content=self.native('comparison',t,(270,265,1918,590),(1740,343))
   self.card(im,content,(90,586-int(20*ease(t/.8))),tilt=.08*(1-ease(t/.7)))
   d=ImageDraw.Draw(im);sweep=int(1740*ease((t-1.8)/1.2))
   if 0<sweep<1740:d.line((90+sweep,557,90+sweep,918),fill=MINT,width=5)
   text(im,(96,50),'DEAR IMGUI → IMKIT',25,MINT,True)
   for i,word in enumerate(['Your UI.','Upgraded.']):
    u=ease((t-i*.22)/.7);text(im,(96+int(180*(1-u)),125+i*145),word,130,MINT if i else WHITE,True)
   text(im,(102,443),'Modern UI. Familiar workflow.',40,WHITE)
   text(im,(112,940),'SAME ImGui:: CALLS',25,MINT,True);text(im,(1108,940),'A MODERN THEME',25,MINT,True)
   if t>3.5:
    u=ease((t-3.5)/.55);text(im,(W-510+int(200*(1-u)),80),'ImKit 3.2',64,MINT,True)
  elif t<14:
   self.heading(im,'01 / MODERNIZE','Same calls. Same workflow.',local,'Keep your application values, context and frame loop.')
   if t<10.5:
    native=self.native('comparison',t,(270,265,1918,590),(1740,343));self.card(im,native,(90,420))
    text(im,(112,790),'DEAR IMGUI',27,MUTED,True);text(im,(1104,790),'IMKIT THEME',27,MINT,True)
    label='Edit once. Both sides respond.' if t<7.5 else 'Your familiar inputs, modern styling.'
    text(im,(96,898),label,51,WHITE,True)
   else:
    # Matched enlargement of the modern specimen, maintaining its aspect ratio.
    u=ease(local/.55);size=(int(1220+100*u),int((1220+100*u)*325/824))
    content=self.native('comparison',t,(1090,265,1914,590),size);self.card(im,content,(520,345),tilt=.05*(1-u))
    text(im,(96,386),'Add a',65,WHITE,True);text(im,(96,463),'theme.',65,MINT,True)
    text(im,(96,856),'Keep the ImGui:: calls you already know.',47,WHITE,True)
  elif t<20:
   self.heading(im,'02 / INTEGRATE','Keep your ImGui workflow.',local)
   cpp=t>=16.8;lines=(['auto theme = imkit::MakeTheme(','    imkit::ThemePreset::Forest);','imkit::ApplyTheme(theme, 1.0f);'] if cpp else ['set(IMKIT_IMGUI_TARGET host_imgui)','add_subdirectory(external/imgui-modern-kit)','target_link_libraries(your_app PRIVATE imkit::imkit)'])
   d=ImageDraw.Draw(im);d.rounded_rectangle((88,316,1816,716),28,fill=(10,21,28),outline=MINT,width=2)
   text(im,(128,348),'C++ · BEFORE ImGui::NewFrame()' if cpp else 'CMAKE · USE YOUR EXISTING IMGUI TARGET',23,MINT,True)
   for i,line in enumerate(lines):
    u=ease((local-i*.12)/.35);text(im,(128+int((1-u)*80),423+i*78),line,37,MINT if i==2 else WHITE,i==2)
   text(im,(96,832),'One theme. Your existing app.',58,WHITE,True)
   # A graph grows from the code, establishing the transition into components.
   d=ImageDraw.Draw(im)
   for i,label in enumerate(['YOUR APP','DEAR IMGUI','IMKIT']):
    x=100+i*560;u=ease((t-14-i*.25)/.6);d.line((x,986,x+int(440*u),986),fill=BLUE if i==1 else MINT,width=4)
    text(im,(x,922),label,22,MUTED,True)
  elif t<35:
   native_t=(t-20)*1.1
   titles=['Tabs that organize.','Select. Inspect.','Edit with familiar inputs.','See changes as you drag.','Choose your source.','Apply. Keep ownership.']
   k=min(5,int((t-20)/2.6));self.heading(im,'03 / BUILD YOUR TOOL',titles[k],local)
   # Camera is driven by the real cursor, bounded to keep context and results visible.
   crop=(265,155,1918,855)
   if 25.2<=t<30.5:
    u=ease((t-25.2)/.8)*(1-ease((t-29.8)/.7));crop=mix(crop,(680,205,1918,780),u)
   content=self.native('workspace',native_t,crop,(1740,int(1740*(crop[3]-crop[1])/(crop[2]-crop[0]))))
   y=258;self.card(im,content,(90,y),tilt=.06*(1-ease(local/.4)))
  elif t<39:
   self.heading(im,'04 / MAKE IT YOURS','288 icons. Ready to use.',local)
   self.icon_cloud(im,t-35,(65,280,680,700))
   content=self.native('icons',(t-35)*2,(265,154,1915,825),(1100,447));self.card(im,content,(760,355))
   text(im,(760,876),'Search. Select. Use the same ImGui loop.',32,WHITE,True)
  elif t<44:
   self.heading(im,'04 / MAKE IT YOURS','13 themes. One application.',local)
   # Actual selection frames are retimed to four 128 BPM accents, never held.
   source_t=float(np.interp(t,THEME_BEATS,self.theme_times))
   content=self.native('themes',source_t,(260,40,1918,840),(1575,760));self.card(im,content,(255,264),tilt=.08*(1-ease(local/.5)))
   d=ImageDraw.Draw(im)
   swatches=[MINT,BLUE,GOLD,(204,171,245),(243,154,176),(90,160,179),(159,210,132),(182,198,224),(50,78,109),(212,222,210),(214,146,104),(115,163,201),(184,193,191)]
   for i,c in enumerate(swatches):
    y=287+(i//2)*103+int(math.sin(t*3+i)*4);x=73+(i%2)*77
    d.rounded_rectangle((x,y,x+62,y+72),16,fill=c)
  elif t<47:
   self.heading(im,'05 / FEEDBACK','Loading → Success.',local,'Useful feedback. Host-owned state.')
   content=self.native('toasts',(t-44)*2,(265,0,1918,710),(1740,747));self.card(im,content,(90,290))
  elif t<51:
   self.heading(im,'06 / SCALE UP','Scrub. Edit. Play.',local,'Timeline controls for your application data.')
   content=self.native('timeline',(t-47)*1.8,(260,150,1918,1030),(1480,785));self.card(im,content,(350,260),tilt=.06*(1-ease(local/.5)))
   text(im,(85,474),'TIME',43,MINT,True);text(im,(85,530),'LINE',43,WHITE,True)
  elif t<55:
   self.heading(im,'06 / SCALE UP','Connect your ideas.',local,'Node Editor · real socket drag, node movement and canvas navigation')
   content=self.native('node-editor-final',(t-51)*2.24,(0,112,1590,1050),(1440,849));self.card(im,content,(380,228),tilt=.06*(1-ease(local/.5)))
   text(im,(84,452),'NODE',39,MINT,True);text(im,(84,510),'EDITOR',32,WHITE,True)
  else:
   if t<57.5:self.icon_cloud(im,t-55,(850,200,1000,720),collapse=True)
   d=ImageDraw.Draw(im)
   u=ease((t-55)/.8);text(im,(96,80),'IMKIT 3.2',27,MINT,True)
   text(im,(96-int((1-u)*100),242),'Modernize',108,WHITE,True)
   text(im,(96,370+int((1-u)*60)),'your next tool.',108,MINT,True)
   text(im,(103,635),'Try the Gallery. Explore the docs.',44,WHITE)
   text(im,(103,731),'aokimotohide.github.io/imgui-modern-kit',32,MUTED)
   text(im,(103,846),'OPEN SOURCE  /  MIT  /  C++20',25,MINT,True)
   if t>56.7:
    a=ease((t-56.7)/.8);d.rounded_rectangle((1390,330,1730,670),70,fill=(29,51,56),outline=MINT,width=3)
    for i in range(3):
     x=1440+i*90;y=510+math.sin(t*3+i)*25*(1-ease((t-58)/1))
     d.ellipse((x,y,x+36,y+36),fill=[MINT,BLUE,GOLD][i]);
     if i:d.line((x-54,y+18,x,y+18),fill=MINT,width=4)
    text(im,(1420,391),'ImKit',65,MINT,True)
  # Moving wipes connect layouts without long dips or blank slides.
  if start not in (0,14,16.8,55,57.5) and local<.32:
   d=ImageDraw.Draw(im);x=int(W*ease(local/.32));d.polygon([(x-130,-10),(x+70,-10),(x-30,H+10),(x-230,H+10)],fill=MINT)
  return im

def music(path,film):
 """Original 128 BPM electro-house. Synthesized instruments/SFX, no samples."""
 sr=48000;n=sr*SECONDS;t=np.arange(n)/sr;audio=np.zeros((n,2),np.float32);rng=np.random.default_rng(330)
 beat=BEAT;bar=4*beat;progression=[50,46,53,48]
 # Pumping is controlled by the beat phase; bass transient remains clear.
 pump=.28+.72*(1-np.exp(-(t%beat)*14))
 def add(start,length,sound,gain=.1,pan=0):
  i=round(start*sr);count=min(len(sound),n-i)
  if i<0 or count<=0:return
  audio[i:i+count,0]+=sound[:count]*gain*math.sqrt((1-pan)/2);audio[i:i+count,1]+=sound[:count]*gain*math.sqrt((1+pan)/2)
 def synth(start,length,midi,gain,kind='lead',pan=0):
  q=np.arange(round(length*sr))/sr;f=440*2**((midi-69)/12);phase=2*np.pi*f*q
  if kind=='bass':s=np.sin(phase)+.24*np.sin(phase*2);env=(1-np.exp(-q*180))*np.exp(-q*8)
  elif kind=='pad':s=sum(np.sin(phase*(1+v)) for v in [-.003,0,.003])/3;env=np.minimum(1,q/.08)*np.minimum(1,(length-q)/.15)
  else:s=sum(np.sin(phase*h)/h**1.5 for h in range(1,7));env=(1-np.exp(-q*140))*np.exp(-q*5)
  if kind!='bass':s*=pump[round(start*sr):round(start*sr)+len(s)]
  add(start,length,s*env,gain,pan)
 for b in range(math.ceil(SECONDS/bar)):
  when=b*bar;root=progression[(b//2)%4];intensity=.6 if when<5 else .8 if when<20 else 1 if when<44 else .92 if when<55 else .65
  for j,note in enumerate([root,root+7,root+12,root+15]):
   length=min(bar,SECONDS-when)
   if length>0:synth(when,length,note,.055*intensity,'pad',(-.75+j*.5))
  for k in range(8):
   at=when+k*beat/2
   if at+.4>=SECONDS:continue
   synth(at,.38,root-12,.27*intensity,'bass')
   melody=[12,19,15,22,12,24,19,15][(k+b)%8]
   if at>3:synth(at,.44,root+melody,.095*intensity,'lead',math.sin(k*2)*.55)
 for k in range(math.ceil(SECONDS/beat)):
  at=k*beat
  if at>58:continue
  if 14<=at<19.3 and k%2:continue  # Readable code break, then a full component drop.
  if 44<=at<46.7 and k%2:continue  # Short feedback break before the editor section.
  intensity=.55 if at<5 else .85 if at<20 else 1
  q=np.arange(int(sr*.28))/sr
  kick=np.sin(2*np.pi*(48*q+3.5*(1-np.exp(-q*42))))*np.exp(-q*19)
  add(at,.28,kick,.67*intensity)
  if k%2:
   clap=rng.standard_normal(len(q));clap=np.diff(clap,prepend=0)*np.exp(-q*33)
   add(at,.28,clap,.09*intensity)
  for off in [.25,.5,.75]:
   qh=np.arange(int(sr*.065))/sr;hat=np.diff(rng.standard_normal(len(qh)),prepend=0)*np.exp(-qh*70)
   add(at+off*beat,.065,hat,.035*intensity,(-.4 if off==.25 else .4))
 # Arrangement lifts and sync accents at the actual edit points.
 for at in [4.5,19.1,34.1,43.1,50.2,54.1]:
  q=np.arange(int(sr*.8))/sr;riser=np.diff(rng.standard_normal(len(q)),prepend=0)*np.linspace(0,1,len(q))**2
  add(at,.8,riser,.035)
 for at in SHOTS[1:]:
  q=np.arange(int(sr*.16))/sr;whoosh=np.diff(rng.standard_normal(len(q)),prepend=0)*np.sin(np.pi*q/.16)**2
  add(at-.08,.16,whoosh,.025,-.3)
 # Clicks come from recorded button events, mapped through the actual clip speed.
 mapping=[('comparison',0,14,1),('workspace',20,35,1.1),('icons',35,39,2),('themes',39,44,2),('toasts',44,47,2),('timeline',47,51,1.8),('node-editor-final',51,55,2.24)]
 for name,start,end,speed in mapping:
  rows=[json.loads(line) for line in Path(str(film.clips[name].path)+'.jsonl').read_text().splitlines() if '"time"' in line];down=False
  for e in rows:
   if e['down'] and not down:
    at=float(np.interp(e['time'],film.theme_times,THEME_BEATS)) if name=='themes' else start+e['time']/speed
    if at<end:
     q=np.arange(int(sr*.035))/sr;click=np.sin(2*np.pi*1800*q)*np.exp(-q*130);add(at,.035,click,.095)
   down=e['down']
 delay=int(beat*.75*sr);audio[delay:,0]+=audio[:-delay,1].copy()*.13;audio[delay:,1]+=audio[:-delay,0].copy()*.11
 envelope=np.minimum(1,t/.15)*np.minimum(1,(60-t)/1.25);audio=np.tanh(audio*.9)*envelope[:,None]
 audio*=.86/max(.01,float(np.abs(audio).max()))
 with wave.open(str(path),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(sr);f.writeframes((audio*32767).astype('<i2').tobytes())

def audio_filter(wav):
 # Measure the complete score first. Leave headroom for AAC reconstruction.
 measurement=subprocess.run(['ffmpeg','-hide_banner','-i',str(wav),'-af','loudnorm=I=-16:TP=-2.5:LRA=8:print_format=json','-f','null','NUL' if __import__('os').name=='nt' else '/dev/null'],capture_output=True,text=True,check=True)
 stats=json.loads(re.search(r'\{\s*"input_i"[\s\S]*?\}',measurement.stderr).group())
 return 'loudnorm=I=-16:TP=-2.5:LRA=8:linear=false:'+':'.join(f'{target}={stats[source]}' for target,source in [('measured_I','input_i'),('measured_TP','input_tp'),('measured_LRA','input_lra'),('measured_thresh','input_thresh'),('offset','target_offset')])

def main():
 p=argparse.ArgumentParser();p.add_argument('--inputs',type=Path,default=ROOT/'out/v3.2-native');p.add_argument('--output',type=Path,default=ROOT/'website/public/media/imkit-3.2-film.mp4');p.add_argument('--review-only',action='store_true');p.add_argument('--seconds',type=float,default=60);args=p.parse_args()
 scratch=ROOT/'out/promo';scratch.mkdir(parents=True,exist_ok=True);film=Film(args.inputs)
 try:
  if args.review_only:
   times=[.6,2.4,4.4,6,9,12,15.4,18.5,21,23.5,26.5,29,31.5,34,36.5,38,40,43,45.5,48,50,52,54,58]
   sheet=Image.new('RGB',(1920,6*300),BG)
   for i,t in enumerate(times):
    frame=film.frame(t);frame.save(scratch/f'review-{t:04.1f}.jpg',quality=95)
    sheet.paste(frame.resize((480,270)),((i%4)*480,(i//4)*300));text(sheet,((i%4)*480+12,(i//4)*300+273),f'{t:.1f}s',20,MINT)
   sheet.save(scratch/'contact-sheet.jpg',quality=95);return
  wav=scratch/'original-score.wav';music(wav,film);args.output.parent.mkdir(parents=True,exist_ok=True)
  cmd=['ffmpeg','-y','-hide_banner','-loglevel','warning','-f','rawvideo','-pix_fmt','rgb24','-s',f'{W}x{H}','-r',str(FPS),'-i','pipe:0','-i',str(wav),'-af',audio_filter(wav),'-c:v','libx264','-preset','medium','-crf','20','-pix_fmt','yuv420p','-c:a','aac','-b:a','192k','-ar','48000','-movflags','+faststart','-t',str(args.seconds),str(args.output)]
  proc=subprocess.Popen(cmd,stdin=subprocess.PIPE)
  try:
   for i in range(round(FPS*args.seconds)):
    proc.stdin.write(film.frame(i/FPS).tobytes())
    if i%(FPS*5)==0:print(f'Rendered {i//FPS}/60s',flush=True)
  finally:proc.stdin.close()
  if proc.wait():raise RuntimeError('Film encoder failed')
  if args.seconds==60:
   film.frame(4.4).save(args.output.with_name('imkit-3.2-poster.jpg'),quality=96)
   provenance={'version':'3.2.0','license':'MIT; repository font/icon notices retained','revision':2,'duration':60,'fps':60,'resolution':[W,H],'shots':len(SHOTS),'music':'Original 128 BPM D-minor electro-house; synthesized instruments and event-synchronized effects; seed 330; no samples or narration','fonts':'Repository Inter (SIL OFL), rasterized only','visuals':'Continuous native OpenGL backbuffer; real public Dear ImGui IO and large synchronized cursor; original typography and compositing','scripts':['tools/capture_v32_media.py','tools/render_v32_film.py','tools/motion_film.py'],'validate':'tools/check_v32_media.py','sha256':hashlib.sha256(args.output.read_bytes()).hexdigest()}
   provenance['icons']='Repository MIT icons rasterized with a consistent dark ink for the original graphics layer'
   (args.output.parent/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');(scratch/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
 finally:film.close()
if __name__=='__main__':main()
