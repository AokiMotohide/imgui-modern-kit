#!/usr/bin/env python3
"""Verify public v3.2.0 revised downloads and unchanged library identities.

Downloads are hashed in memory in 1 MiB chunks; no redundant local archives.
GitHub CLI provides metadata; public asset downloads never send credentials.
"""
from pathlib import Path
import hashlib,json,subprocess,urllib.request,time

ROOT=Path(__file__).resolve().parents[1]
RELEASE=ROOT/'out/release'
NAMES=['imkit-v3.2.0-showcase.mp4','imkit-v3.2.0-manifest.json','imkit-v3.2.0-validation-evidence.zip','SHA256SUMS']

def public_bytes(url):
 # Bypass stale CDN entries during explicit asset replacement verification.
 request=urllib.request.Request(url+'?verify='+str(time.time_ns()),headers={'User-Agent':'ImKit-release-verifier'})
 return urllib.request.urlopen(request,timeout=60)

def main():
 release=json.loads(subprocess.check_output(['gh','api','repos/AokiMotohide/imgui-modern-kit/releases/tags/v3.2.0']))
 assets={a['name']:a for a in release['assets']}
 manifest=json.loads((RELEASE/NAMES[1]).read_text(encoding='utf-8'))
 assert release['tag_name']=='v3.2.0' and manifest['source_commit']=='0e98e72bad1bba65f0e760debefb9a04f0168888'
 tag=subprocess.check_output(['gh','api','repos/AokiMotohide/imgui-modern-kit/git/ref/tags/v3.2.0'],text=True)
 tag=json.loads(tag)['object']
 while tag['type']=='tag':tag=json.loads(subprocess.check_output(['gh','api',tag['url']]))['object']
 assert tag['sha']==manifest['source_commit']
 for name,sha in manifest['media_revision']['stable_archives'].items():assert assets[name]['digest']=='sha256:'+sha
 reports=[]
 for name in NAMES:
  expected=hashlib.sha256((RELEASE/name).read_bytes()).hexdigest();h=hashlib.sha256();size=0
  with public_bytes(assets[name]['browser_download_url']) as response:
   for chunk in iter(lambda:response.read(1024*1024),b''):h.update(chunk);size+=len(chunk)
  assert h.hexdigest()==expected and size==assets[name]['size']
  assert assets[name]['digest']=='sha256:'+expected
  reports.append({'file':name,'bytes':size,'sha256':expected,'result':'PASS'})
 report={'production_commit':manifest['media_revision']['production_commit'],'source_commit':manifest['source_commit'],'stable_archives':6,'downloads':reports,'result':'PASS'}
 (ROOT/'out/promo/public-release-verification.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))

if __name__=='__main__':main()
