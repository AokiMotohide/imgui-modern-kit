#!/usr/bin/env python3
"""Record native v3.2 Gallery specimens and publish only finished GIF assets.
Run after building imkit_gallery in the canonical shared Debug output.
Intermediate backbuffers belong to this media job in out/v3.2-native (regenerable).
"""
from pathlib import Path
import argparse,subprocess,sys,shutil
ROOT=Path(__file__).resolve().parents[1]
ROUTES={'overview':'v3-overview','comparison':'v3-comparison','workspace':'v3-workspace','components':'v3-components','icons':'v3-icons','themes':'v3-theme-comparison','toasts':'v3-toasts','timeline':'v3-timeline','workflow':'v3-workflow-progress','preview-contract':'v3-preview-contract','icon-artwork':'v3-icon-artwork'}
def main():
 p=argparse.ArgumentParser();p.add_argument('--gif-only',action='store_true');args=p.parse_args()
 free=shutil.disk_usage(ROOT).free;peak=2*1024**3
 if free<max(20*1024**3,peak+10*1024**3):raise RuntimeError('Insufficient capacity')
 binaries=ROOT/'build/windows-debug/catalog/Debug';output=ROOT/'out/v3.2-native'
 for route,name in ROUTES.items():
  if not args.gif_only:subprocess.run([str(binaries/'imkit_gallery.exe'),'--capture-demo',route,'--width','1280','--height','720','--output',str(output)],cwd=ROOT,check=True)
  count=len(list((output/route).glob('frame-*.png')))
  subprocess.run([sys.executable,str(ROOT/'tools/build_readme_gif.py'),str(output/route),str(ROOT/'docs/images'/f'{name}.gif'),'--expected-frames',str(count),'--colors','96'],check=True)
 if not args.gif_only:subprocess.run([str(binaries/'imkit_node_editor_gallery.exe'),'--capture-gif',str(output/'node-editor-final'),'--width','1280','--height','720'],cwd=ROOT,check=True)
 count=len(list((output/'node-editor-final').glob('frame-*.png')))
 subprocess.run([sys.executable,str(ROOT/'tools/build_readme_gif.py'),str(output/'node-editor-final'),str(ROOT/'docs/images/v3-node-editor.gif'),'--expected-frames',str(count),'--colors','128','--dither','none'],check=True)
 print(f'free before={free}, after={shutil.disk_usage(ROOT).free}')
if __name__=='__main__':main()
