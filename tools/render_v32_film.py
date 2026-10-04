#!/usr/bin/env python3
"""Render the ImKit 3.2 film: native UI footage + original motion graphics/music.
Requires Python 3, Pillow, NumPy, FFmpeg. Frames stream directly into FFmpeg.
Native inputs: tools/capture_v32_media.py. No intermediate full-film PNG sequence.
"""
from pathlib import Path
import argparse,math,subprocess,wave,json
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageFilter
ROOT=Path(__file__).resolve().parents[1]
W,H,FPS,SECONDS=1920,1080,60,60
BG=(22,35,41);GREEN=(169,231,203);BLUE=(144,185,255);GOLD=(246,200,121);WHITE=(240,247,244);MUTED=(162,180,181)
FONT=ROOT/'assets/fonts/Inter-Regular.ttf';BOLD=ROOT/'assets/fonts/Inter-SemiBold.ttf'
FONTS={}
def font(size,bold=False):
 key=(size,bold)
 if key not in FONTS:FONTS[key]=ImageFont.truetype(str(BOLD if bold else FONT),size)
 return FONTS[key]
def ease(v):v=max(0,min(1,v));return 1-(1-v)**3

def music(path):
 """Original 120 BPM D-minor synth composition, with deterministic percussion."""
 rate=48000;n=rate*SECONDS;audio=np.zeros((n,2),dtype=np.float32);rng=np.random.default_rng(320)
 def note(start,length,midi,gain,pan=0,kind='pluck'):
  i=int(start*rate);count=min(int(length*rate),n-i)
  if count<=0:return
  t=np.arange(count)/rate;freq=440*2**((midi-69)/12)
  env=(1-np.exp(-t*80))*np.exp(-t*(4 if kind=='pluck' else .65))*np.minimum(1,(length-t)*6)
  if kind=='bass':sound=(np.sin(2*np.pi*freq*t)+.22*np.sin(2*np.pi*freq*2*t))*env
  else:sound=(np.sin(2*np.pi*freq*t)+.30*np.sin(2*np.pi*freq*2*t)+.10*np.sin(2*np.pi*freq*3*t))*env
  audio[i:i+count,0]+=sound*gain*math.sqrt((1-pan)/2);audio[i:i+count,1]+=sound*gain*math.sqrt((1+pan)/2)
 chords=[(50,57,62,65),(46,53,58,62),(53,60,65,69),(48,55,60,64)]
 for bar in range(30):
  start=bar*2;chord=chords[(bar//2)%4]
  intensity=.40 if start<5 else .75 if start<25 else 1 if start<48 else .70 if start<55 else .35
  for j,midi in enumerate(chord):note(start,2.4,midi,.045*intensity,(-.6+j*.4),'pad')
  for eighth in range(8):
   when=start+eighth*.25
   note(when,.45,chord[(eighth*3+bar)%4]+12,.065*intensity,math.sin(eighth)*.45)
   if eighth%2==0:note(when,.35,chord[0]-12,.17*intensity,0,'bass')
   if when<4 or when>56:continue
   # Synthesized kick, snare and filtered-noise hat; no samples.
   length=.18 if eighth%2==0 else .06;i=int(when*rate);count=min(int(length*rate),n-i);t=np.arange(count)/rate
   hat=rng.standard_normal(count);hat=np.diff(hat,prepend=0)*np.exp(-t*65)*.012*intensity
   audio[i:i+count,0]+=hat;audio[i:i+count,1]+=hat
   if eighth%2==0:
    kick=np.sin(2*np.pi*(46*t+9*(1-np.exp(-t*25))))*np.exp(-t*25)*.27*intensity
    audio[i:i+count]+=kick[:,None]
   if eighth in (2,6):
    snare=(rng.standard_normal(count)*.035+np.sin(2*np.pi*190*t)*.05)*np.exp(-t*22)*intensity
    audio[i:i+count]+=snare[:,None]
 # Stereo delay adds movement without sudden gain jumps.
 delay=int(.375*rate);audio[delay:,0]+=audio[:-delay,1].copy()*.20
 audio[delay:,1]+=audio[:-delay,0].copy()*.16
 envelope=np.minimum(1,np.arange(n)/(rate*1.5))*np.minimum(1,(n-np.arange(n))/(rate*3))
 audio*=envelope[:,None];audio=np.tanh(audio*1.4);audio*=.78/max(.01,float(np.max(np.abs(audio))))
 with wave.open(str(path),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(rate);f.writeframes((audio*32767).astype('<i2').tobytes())

class Film:
 def __init__(self,inputs):
  self.sources={};self.cached={}
  for name in ['comparison','workspace','icons','themes','toasts','timeline','node-editor-final']:
   paths=sorted((inputs/name).glob('frame-*.png'))
   if not paths:raise FileNotFoundError(inputs/name)
   self.sources[name]=paths
  self.icons=[]
  for p in sorted((ROOT/'assets/icons/originals').rglob('*.png'))[::4][:64]:
   icon=Image.open(p).convert('RGBA');icon.thumbnail((70,70));self.icons.append(icon)
 def source(self,name,t,box=None,size=None):
  paths=self.sources[name];idx=min(len(paths)-1,max(0,int(t*(16 if name=="toasts" else 8))))
  key=(name,idx,box,size)
  if key not in self.cached:
   image=Image.open(paths[idx]).convert('RGB')
   if box:image=image.crop(box)
   if size:image=image.resize(size,Image.Resampling.LANCZOS)
   if len(self.cached)>140:self.cached.clear()
   self.cached[key]=image
  return self.cached[key]
 def text(self,draw,xy,text,size=46,color=WHITE,bold=False):draw.text(xy,text,font=font(size,bold),fill=color)
 def panel(self,img,content,x,y,radius=20):
  arrival=1-ease(self.local/.8);x+=int(arrival*90);y+=int(arrival*22)
  mask=Image.new('L',content.size);ImageDraw.Draw(mask).rounded_rectangle((0,0,content.width-1,content.height-1),radius,fill=255)
  img.paste(content,(int(x),int(y)),mask)
  ImageDraw.Draw(img).rounded_rectangle((int(x),int(y),int(x+content.width),int(y+content.height)),radius,outline=(72,97,99),width=2)
 def background(self,t):
  img=Image.new('RGB',(W,H),BG);d=ImageDraw.Draw(img)
  for x in range(-100,W+100,80):d.line((x+int(t*8)%80,0,x+int(t*8)%80,H),fill=(28,45,51),width=1)
  for y in range(0,H,80):d.line((0,y,W,y),fill=(28,45,51),width=1)
  # Slow orbital lines and travelling signals are continuous at 60 fps.
  for i in range(5):
   cx=1570+i*35;cy=400+i*48;r=180+i*32
   d.ellipse((cx-r,cy-r,cx+r,cy+r),outline=(33,58,62),width=2)
   a=t*.22+i*1.1;px=cx+math.cos(a)*r;py=cy+math.sin(a)*r
   d.ellipse((px-5,py-5,px+5,py+5),fill=GREEN if i%2==0 else BLUE)
  return img
 def header(self,d,kicker,title,t,entered):
  offset=int((1-ease((t-entered)/.7))*40)
  self.text(d,(96,46+offset),kicker,22,GREEN,True)
  self.text(d,(96,92+offset),title,64,WHITE,True)
 def frame(self,t):
  start=max(b for b in [0,5,16,25,30.5,38,48,51,53,55] if b<=t);self.local=t-start
  im=self.background(t);d=ImageDraw.Draw(im)
  if t<5:
   e=ease(t/1.1);x=96-int((1-e)*150)
   self.text(d,(x,265),'ImKit',196,GREEN,True)
   self.text(d,(96,502),'Modern UI.',72,WHITE,True)
   self.text(d,(96,590),'Familiar workflow.',72,WHITE,True)
   self.text(d,(98,752),'For your existing Dear ImGui app.',36,MUTED)
   self.text(d,(98,828),'C++20  /  3.2',25,GREEN,True)
   # A kinetic brand diagram unfolds into connected UI modules.
   for i,(label,c) in enumerate([('THEMES',GREEN),('CONTROLS',BLUE),('ICONS',GOLD)]):
    u=ease((t-.35-i*.25)/.8);xx=1110+int((1-u)*350);yy=240+i*178
    d.rounded_rectangle((xx,yy,xx+630,yy+130),20,fill=(30,51,57),outline=c,width=2)
    self.text(d,(xx+38,yy+40),label,36,c,True)
    if i:d.line((xx+540,yy-48,xx+540,yy),fill=c,width=3)
  elif t<16:
   self.header(d,'01 / MODERNIZE','Same calls. A different feel.',t,5)
   local=t-5
   left=self.source('comparison',local,(262,266,748,620),(1030,750))
   right=self.source('comparison',local,(759,266,1245,620),(1030,750))
   # Identical-size specimen crops and a smooth travelling mask.
   mixed=left.copy();wipe=int(1030*ease((local-2.0)/2.2))
   if wipe:mixed.paste(right.crop((0,0,wipe,750)),(0,0))
   self.panel(im,mixed,794,228)
   d=ImageDraw.Draw(im)
   self.text(d,(96,326),'Keep your',60,WHITE,True);self.text(d,(96,400),'ImGui:: calls.',60,GREEN,True)
   self.text(d,(96,538),'Add a theme.',40,WHITE)
   self.text(d,(96,600),'Keep your values.',40,WHITE)
   self.text(d,(96,720),'No new context.',30,MUTED)
   self.text(d,(96,766),'No new frame loop.',30,MUTED)
   self.text(d,(824,986),'THEME ONLY  /  native Gallery capture',22,MUTED)
  elif t<25:
   self.header(d,'02 / INTEGRATE','Keep your ImGui workflow.',t,16)
   d.rounded_rectangle((96,240,1824,874),24,fill=(13,26,32),outline=(72,97,99),width=2)
   lines=[('CMAKE',GREEN),('set(IMKIT_IMGUI_TARGET host_imgui)',WHITE),('add_subdirectory(external/imgui-modern-kit)',WHITE),('target_link_libraries(your_app PRIVATE imkit::imkit)',WHITE),('',WHITE),('C++ / BEFORE ImGui::NewFrame()',BLUE),('auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);',WHITE),('imkit::ApplyTheme(theme, 1.0f);',GREEN)]
   for i,(line,color) in enumerate(lines):
    if t-16>i*.11:self.text(d,(138,280+i*67),line,32 if i not in (0,5) else 22,color,i in (0,5,7))
   d.line((138,835,138+int(1500*ease((t-17)/1.2)),835),fill=GREEN,width=3)
   self.text(d,(98,927),'One target. One theme. Your application stays yours.',40,WHITE)
  elif t<38:
   local=t-25;self.header(d,'03 / COMPOSE','Useful controls. Familiar interactions.',t,25)
   if local<5.5:
    content=self.source('workspace',local,None,(1440,810));self.panel(im,content,384,224)
    self.text(d,(96,370),'Tabs.',44,GREEN,True);self.text(d,(96,444),'Hierarchy.',44,WHITE,True);self.text(d,(96,518),'Settings.',44,WHITE,True)
    self.text(d,(96,850),'NEW IN 3.2',23,GREEN,True)
   else:
    # Detail crops, never stretched; masks slide across inspector and action areas.
    content=self.source('workspace',local-5.5,(956,212,1265,539),(710,752));self.panel(im,content,1114,230)
    self.text(d,(96,320),'Three-axis inputs.',58,WHITE,True)
    self.text(d,(96,412),'Clear choices.',58,GREEN,True)
    self.text(d,(96,545),'Return a request.',38,WHITE)
    self.text(d,(96,602),'Update your app’s state.',38,WHITE)
    row=self.source('workspace',local-5.5,(517,515,945,620),(856,210));self.panel(im,row,96,734)
  elif t<48:
   local=t-38;self.header(d,'04 / PERSONALIZE','288 icons. 13 themes.',t,38)
   # Original PNG masters form a staggered beat-synchronised grid.
   for i,icon in enumerate(self.icons):
    row,col=divmod(i,8);targetx=96+col*104;targety=236+row*92
    phase=ease((local-i*.025)/.65);xx=int(targetx+(1-phase)*100);yy=int(targety+(1-phase)*50)
    pulse=1+.045*max(0,1-((local-i*.0625)% .5)/.12)
    size=int(70*pulse);tile=icon.resize((size,size),Image.Resampling.LANCZOS)
    d.rounded_rectangle((xx-8,yy-8,xx+78,yy+78),14,fill=(220,236,231),outline=GREEN,width=1)
    im.paste(tile,(xx+(70-size)//2,yy+(70-size)//2),tile)
   name='icons' if local<4 else 'themes'
   content=self.source(name,local if local<4 else local-4,None,(856,482));self.panel(im,content,976,265)
   d=ImageDraw.Draw(im);self.text(d,(976,794),'Search. Select. Make it yours.',35,WHITE,True)
   self.text(d,(976,868),'Light + dark  /  semantic colors',26,MUTED)
  elif t<55:
   local=t-48
   self.header(d,'05 / GROW','Small details. Bigger possibilities.',t,48)
   if local<3:
    content=self.source('toasts',local,(748,0,1280,430),(1000,808));self.panel(im,content,824,225)
    self.text(d,(96,374),'Loading → Success',51,GREEN,True)
    self.text(d,(96,472),'Host-owned notifications.',30,WHITE)
    self.text(d,(96,540),'Same ID. New state.',30,MUTED)
   else:
    name='timeline' if local<5 else 'node-editor-final'
    content=self.source(name,(local-3 if local<5 else local-5)*2,None,(1380,776));self.panel(im,content,444,224)
    self.text(d,(96,388),'Timeline' if local<5 else 'Node',44,GREEN,True)
    self.text(d,(96,456),'& editing' if local<5 else 'Editor',44,WHITE,True)
    self.text(d,(96,844),'YOUR DATA.',24,GREEN,True);self.text(d,(96,884),'YOUR HISTORY.',24,GREEN,True)
  else:
   e=ease((t-55)/.9);y=280+int((1-e)*120)
   self.text(d,(96,100),'IMKIT 3.2',28,GREEN,True)
   self.text(d,(96,y),'Modernize',114,WHITE,True)
   self.text(d,(96,y+132),'your next tool.',114,GREEN,True)
   self.text(d,(102,636),'Try the Gallery. Explore the docs.',44,WHITE)
   self.text(d,(102,737),'aokimotohide.github.io/imgui-modern-kit',34,MUTED)
   self.text(d,(102,846),'Open source  /  MIT  /  C++20',28,GREEN,True)
   d.rounded_rectangle((1430,312,1750,632),80,fill=(30,51,57),outline=GREEN,width=3)
   d.line((1490,382,1580,382,1580,472,1680,472),fill=GREEN,width=11)
   d.line((1490,562,1580,562,1580,472),fill=BLUE,width=11)
   for x,y,c in [(1490,382,GOLD),(1680,472,GREEN),(1490,562,BLUE)]:d.ellipse((x-16,y-16,x+16,y+16),fill=c)
  # Timeline progress is understated but makes the edit rhythm visible.
  d=ImageDraw.Draw(im);d.rectangle((0,H-5,int(W*t/SECONDS),H),fill=GREEN)
  # Short dip through the brand canvas at scene boundaries, eased at 60 fps.
  boundaries=[5,16,25,38,48,51,53,55]
  distance=min(abs(t-b) for b in boundaries)
  if distance<.16:im=Image.blend(im,Image.new('RGB',(W,H),BG),(1-distance/.16)*.7)
  return im

def main():
 p=argparse.ArgumentParser();p.add_argument('--inputs',type=Path,default=ROOT/'out/v3.2-native');p.add_argument('--output',type=Path,default=ROOT/'website/public/media/imkit-3.2-film.mp4');p.add_argument('--review-only',action='store_true');args=p.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);scratch=ROOT/'out/promo';scratch.mkdir(parents=True,exist_ok=True)
 film=Film(args.inputs)
 times=[1.5,4,7,10,14,18,23,27,33,39,43,49,52,54,56.5,59]
 review=Image.new('RGB',(960*4,580*4),BG)
 for i,t in enumerate(times):
  im=film.frame(t);im.resize((960,540),Image.Resampling.LANCZOS).save(scratch/f'review-{t:04.1f}.jpg',quality=94)
  review.paste(im.resize((960,540)),((i%4)*960,(i//4)*580));ImageDraw.Draw(review).text(((i%4)*960+16,(i//4)*580+544),f'{t:.1f}s',font=font(24),fill=WHITE)
 review.save(scratch/'contact-sheet.jpg',quality=94)
 film.frame(4).save(args.output.with_name('imkit-3.2-poster.jpg'),quality=96)
 if args.review_only:return
 wav=scratch/'original-score.wav';music(wav)
 cmd=['ffmpeg','-y','-hide_banner','-loglevel','warning','-f','rawvideo','-pix_fmt','rgb24','-s',f'{W}x{H}','-r',str(FPS),'-i','pipe:0','-i',str(wav),'-af','loudnorm=I=-16:TP=-1.5:LRA=8,volume=-1.03dB','-c:v','libx264','-preset','medium','-crf','20','-pix_fmt','yuv420p','-c:a','aac','-b:a','192k','-ar','48000','-movflags','+faststart','-t',str(SECONDS),str(args.output)]
 proc=subprocess.Popen(cmd,stdin=subprocess.PIPE)
 try:
  for frame in range(FPS*SECONDS):
   proc.stdin.write(film.frame(frame/FPS).tobytes())
   if frame%(FPS*5)==0:print(f'rendered {frame//FPS:02d}/{SECONDS}s',flush=True)
 finally:proc.stdin.close()
 if proc.wait():raise RuntimeError('FFmpeg failed')
 (scratch/'provenance.json').write_text(json.dumps({'duration':SECONDS,'fps':FPS,'size':[W,H],'music':'Original 120 BPM D minor composition; procedural oscillators and deterministic noise, seed 320; no samples','fonts':'Repository Inter (SIL OFL), rasterized only','visuals':'Native Gallery/Node Editor UI captures + original motion graphics; source scenes documented by capture.txt','script':'tools/render_v32_film.py','output_bytes':args.output.stat().st_size},indent=2)+'\n')
 print(args.output,flush=True)
if __name__=='__main__':main()
