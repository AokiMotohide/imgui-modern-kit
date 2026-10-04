#!/usr/bin/env python3
"""Continuous 60 Hz native GL capture -> lossless video -> operation GIFs.
Intermediate videos/events belong to this job in out/v3.2-native (regenerable).
Requires the canonical Debug targets and FFmpeg; no PNG frame directories.
"""
from pathlib import Path
import argparse,json,subprocess,shutil
ROOT=Path(__file__).resolve().parents[1]
ROUTES={'overview':'v3-overview','comparison':'v3-comparison','workspace':'v3-workspace','components':'v3-components','icons':'v3-icons','themes':'v3-theme-comparison','toasts':'v3-toasts','timeline':'v3-timeline','workflow':'v3-workflow-progress','preview-contract':'v3-preview-contract','icon-artwork':'v3-icon-artwork','node-editor-final':'v3-node-editor'}

def probe(path):
 info=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-show_format','-of','json',str(path)]))
 video=next(s for s in info['streams'] if s['codec_type']=='video')
 assert (video['width'],video['height'],video['r_frame_rate'])==(1920,1080,'60/1')
 return float(info['format']['duration'])

def gif(path,dest,seconds):
 # Keep every operation, accelerating longer takes as one continuous sequence.
 duration=6.0 if path.stem=='node-editor-final' else 8.0
 source_seconds=min(seconds,5.7) if path.stem=='node-editor-final' else seconds
 # Graph takes contain expensive moving CPU preview textures. A focused native
 # crop retains the sockets/header drag at 20 fps without a large empty canvas.
 crop='trim=duration=5.7,crop=1296:729:0:112,' if path.stem=='node-editor-final' else ''
 for colors in [128,96,64,48]:
  base=f'{crop}setpts={duration/source_seconds:.9f}*PTS,fps=20,scale=960:540:flags=lanczos'
  filters=f'[0:v]{base},split[a][b];[a]palettegen=max_colors={colors}:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle'
  subprocess.run(['ffmpeg','-y','-v','error','-i',str(path),'-filter_complex',filters,'-t',str(duration),'-loop','0',str(dest)],check=True)
  if dest.stat().st_size<=2*1024**2:return
 # Preserve the complete sequence and the 960x540 contract if a very dynamic
 # source still exceeds the web budget. Motion remains visible at 12 fps.
 base=f'{crop}setpts={duration/source_seconds:.9f}*PTS,fps=12,scale=960:540:flags=lanczos'
 filters=f'[0:v]{base},split[a][b];[a]palettegen=max_colors=64:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle'
 subprocess.run(['ffmpeg','-y','-v','error','-i',str(path),'-filter_complex',filters,'-t',str(duration),'-loop','0',str(dest)],check=True)
 if dest.stat().st_size>2*1024**2:raise RuntimeError(f'GIF exceeds 2 MiB: {dest}')
def main():
 p=argparse.ArgumentParser();p.add_argument('--gif-only',action='store_true');p.add_argument('--routes',nargs='+',choices=list(ROUTES),default=list(ROUTES));args=p.parse_args()
 free=shutil.disk_usage(ROOT).free;peak=20*1024**3
 if free<max(20*1024**3,peak+10*1024**3):raise RuntimeError('Insufficient capacity')
 binaries=ROOT/'build/windows-debug/catalog/Debug';output=ROOT/'out/v3.2-native';output.mkdir(parents=True,exist_ok=True);reports=[]
 for route in args.routes:
  name=ROUTES[route];movie=output/f'{route}.mkv'
  if not args.gif_only:
   exe=binaries/('imkit_node_editor_gallery.exe' if route=='node-editor-final' else 'imkit_gallery.exe');cmd=[str(exe),'--capture-motion']
   if route=='node-editor-final':cmd.append(str(output))
   else:cmd.extend([route,'--output',str(output)])
   cmd.extend(['--width','1920','--height','1080']);print(f'Capture {route}',flush=True);subprocess.run(cmd,cwd=ROOT,check=True)
  seconds=probe(movie);events=[json.loads(line) for line in Path(str(movie)+'.jsonl').read_text().splitlines()]
  frames=[e for e in events if 'time' in e];checks=[e for e in events if 'check' in e]
  assert frames and all(e['passed'] for e in checks) and abs(seconds-len(frames)/60)<.05
  assert any(e['down'] or e['middle'] or e['wheel'] for e in frames)
  dest=ROOT/'docs/images'/f'{name}.gif';gif(movie,dest,seconds)
  reports.append({'route':route,'seconds':seconds,'frames':len(frames),'checks':checks,'gif_bytes':dest.stat().st_size})
  print(f'{route}: {seconds:.2f}s, {len(frames)} native frames, {dest.stat().st_size} GIF bytes',flush=True)
 (output/'motion-validation.json').write_text(json.dumps(reports,indent=2)+'\n')
 print(f'free before={free}, after={shutil.disk_usage(ROOT).free}')
if __name__=='__main__':main()
