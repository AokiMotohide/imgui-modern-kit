#!/usr/bin/env python3
"""Inspect the final promo and native GIFs; keep measurements outside Git."""
from pathlib import Path
import subprocess,json,re,hashlib
from PIL import Image
from capture_v32_media import ROUTES,probe
ROOT=Path(__file__).resolve().parents[1]
def main():
 movie=ROOT/'website/public/media/imkit-3.2-film.mp4'
 info=json.loads(subprocess.check_output(['ffprobe','-v','error','-count_frames','-show_streams','-show_format','-of','json',str(movie)]))
 video=next(s for s in info['streams'] if s['codec_type']=='video');audio=next(s for s in info['streams'] if s['codec_type']=='audio')
 assert (video['width'],video['height'],video['r_frame_rate'],int(video['nb_read_frames']))==(1920,1080,'60/1',3600)
 assert abs(float(info['format']['duration'])-60)<.05 and audio['channels']==2 and audio['sample_rate']=='48000'
 assert movie.stat().st_size<40*1024**2
 result=subprocess.run(['ffmpeg','-hide_banner','-i',str(movie),'-af','loudnorm=I=-16:TP=-1.5:LRA=8:print_format=json','-f','null','NUL' if __import__('os').name=='nt' else '/dev/null'],capture_output=True,text=True,check=True)
 loudness=json.loads(re.search(r'\{\s*"input_i"[\s\S]*?\}',result.stderr).group())
 assert abs(float(loudness['input_i'])+16)<.5 and float(loudness['input_tp'])<=-1.5
 gifs=[]
 for path in sorted((ROOT/'docs/images').glob('v3-*.gif')):
  with Image.open(path) as image:
   assert image.size==(960,540) and path.stat().st_size<=2*1024**2
   durations=[]
   for i in range(image.n_frames):image.seek(i);durations.append(image.info.get('duration',0))
   seconds=sum(durations)/1000
   assert 6<=seconds<=8.05 and all(d==50 for d in durations)
   gifs.append({'file':path.name,'bytes':path.stat().st_size,'frames':image.n_frames,'seconds':seconds,'fps':20})
 native=[]
 for route in ROUTES:
  path=ROOT/'out/v3.2-native'/f'{route}.mkv';seconds=probe(path)
  events=[json.loads(line) for line in Path(str(path)+'.jsonl').read_text().splitlines()]
  frames=[e for e in events if 'time' in e];checks=[e for e in events if 'check' in e];actions=[e for e in events if 'action' in e]
  assert checks and all(e['passed'] for e in checks) and abs(seconds-len(frames)/60)<.05
  assert actions and any(e['down'] or e['middle'] or e['wheel'] for e in frames)
  assert all(e['frame']==i and abs(e['time']-i/60)<.0001 for i,e in enumerate(frames))
  native.append({'route':route,'seconds':seconds,'frames':len(frames),'checks':checks,'actions':actions,'event_sha256':hashlib.sha256(Path(str(path)+'.jsonl').read_bytes()).hexdigest()})
 assert len(gifs)==len(ROUTES)
 report={'film':{'bytes':movie.stat().st_size,'sha256':hashlib.sha256(movie.read_bytes()).hexdigest(),'seconds':60,'resolution':[1920,1080],'fps':60,'frames':3600,'video':video['codec_name'],'audio':audio['codec_name'],'channels':2,'loudness_lufs':float(loudness['input_i']),'true_peak_dbtp':float(loudness['input_tp']),'loudness_range_lu':float(loudness['input_lra'])},'gifs':gifs,'native_operations':native,'result':'PASS'}
 (ROOT/'out/v3.2-native/motion-validation.json').write_text(json.dumps(native,indent=2)+'\n')
 out=ROOT/'out/promo/media-validation.json';out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['film'],indent=2));print(f'PASS: {len(gifs)} native GIFs, all <= 2 MiB')
if __name__=='__main__':main()
