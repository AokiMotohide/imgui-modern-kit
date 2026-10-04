#!/usr/bin/env python3
"""Inspect the final promo and native GIFs; keep measurements outside Git."""
from pathlib import Path
import subprocess,json,re,hashlib
from PIL import Image
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
 assert abs(float(loudness['input_i'])+16)<.5 and float(loudness['input_tp'])<=-1
 gifs=[]
 for path in sorted((ROOT/'docs/images').glob('v3-*.gif')):
  with Image.open(path) as image:
   assert image.size==(960,540) and path.stat().st_size<=2*1024**2
   gifs.append({'file':path.name,'bytes':path.stat().st_size,'frames':image.n_frames})
 report={'film':{'bytes':movie.stat().st_size,'sha256':hashlib.sha256(movie.read_bytes()).hexdigest(),'seconds':60,'resolution':[1920,1080],'fps':60,'frames':3600,'video':video['codec_name'],'audio':audio['codec_name'],'channels':2,'loudness_lufs':float(loudness['input_i']),'true_peak_dbtp':float(loudness['input_tp']),'loudness_range_lu':float(loudness['input_lra'])},'gifs':gifs,'result':'PASS'}
 out=ROOT/'out/promo/media-validation.json';out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['film'],indent=2));print(f'PASS: {len(gifs)} native GIFs, all <= 2 MiB')
if __name__=='__main__':main()
