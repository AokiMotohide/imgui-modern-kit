#!/usr/bin/env python3
"""Revise only v3.2.0 media/evidence/checksums in the established out/release.

Run after committing the reviewed production files. Does not upload or retag.
Original SDK/source digests and the release source/CI identities must match.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,re,shutil,subprocess,zipfile

ROOT=Path(__file__).resolve().parents[1]
RELEASE=ROOT/'out/release'
TAG_COMMIT='0e98e72bad1bba65f0e760debefb9a04f0168888'
NAMES=['imkit-v3.2.0-showcase.mp4','imkit-v3.2.0-manifest.json','imkit-v3.2.0-validation-evidence.zip','SHA256SUMS']

def digest(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()

def main():
 p=argparse.ArgumentParser();p.add_argument('--production-commit',required=True);args=p.parse_args()
 if not re.fullmatch('[0-9a-f]{40}',args.production_commit):raise ValueError('Full production commit required')
 subprocess.run(['git','cat-file','-e',args.production_commit+'^{commit}'],cwd=ROOT,check=True)
 if subprocess.check_output(['git','rev-parse','v3.2.0^{commit}'],cwd=ROOT,text=True).strip()!=TAG_COMMIT:raise ValueError('Original release tag changed')
 free=shutil.disk_usage(RELEASE).free
 if free<30*1024**3:raise RuntimeError('Reserve 30 GiB before assembly')
 backup=ROOT/'out/promo/release-original';backup.mkdir(parents=True,exist_ok=True)
 for name in NAMES:
  if not (backup/name).exists():shutil.copy2(RELEASE/name,backup/name)
 original=json.loads((backup/NAMES[1]).read_text(encoding='utf-8'))
 if original['source_commit']!=TAG_COMMIT:raise ValueError('Unexpected source identity')
 sums=dict((line.split('  ',1)[1],line.split('  ',1)[0]) for line in (backup/'SHA256SUMS').read_text().splitlines() if line)
 stable={name:sha for name,sha in sums.items() if name not in NAMES}
 for name,sha in stable.items():
  if digest(RELEASE/name)!=sha:raise ValueError(f'Stable archive changed: {name}')
 measured=json.loads((ROOT/'out/promo/media-validation.json').read_text())
 film=ROOT/'website/public/media/imkit-3.2-film.mp4'
 if measured['result']!='PASS' or digest(film)!=measured['film']['sha256']:raise ValueError('Run the media inspector on the final film')
 shutil.copy2(film,RELEASE/NAMES[0])
 manifest=dict(original);manifest['media_validation']=measured
 manifest['media_revision']={'revision':2,'production_commit':args.production_commit,'updated_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'film_file':NAMES[0],'film_sha256':measured['film']['sha256'],'original_film_sha256':original['media_validation']['film']['sha256'],'evidence_file':NAMES[2],'stable_archives':stable,'scope':'Presentation media and development capture tools only; original library tag/source/SDK/CI identities retained','provenance':json.loads((ROOT/'website/public/media/provenance.json').read_text())}
 manifest['documentation_media']={**original['documentation_media'],'gif_fps':20,'gif_duration_seconds':'6–8','source':'Continuous native public-IO captures with original motion graphics and 128 BPM electro-house'}
 manifest_bytes=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf-8')
 (RELEASE/NAMES[1]).write_bytes(manifest_bytes)
 temp=RELEASE/'media-revision-evidence.tmp'
 with zipfile.ZipFile(backup/NAMES[2]) as old,zipfile.ZipFile(temp,'w',zipfile.ZIP_DEFLATED) as new:
  # Preserve original evidence entries; distinguish every new revision entry.
  for entry in old.infolist():new.writestr(entry,old.read(entry.filename))
  new.writestr('media-revision/manifest.json',manifest_bytes)
  for source,arc in [('out/promo/media-validation.json','media-validation.json'),('out/promo/review.md','review.md'),('website/public/media/provenance.json','provenance.json'),('docs/reference/validation.md','validation.md'),('docs/reference/検証記録.md','validation-ja.md')]:new.write(ROOT/source,'media-revision/'+arc)
  for path in sorted((ROOT/'out/promo').glob('review-*.jpg')):new.write(path,'media-revision/stills/'+path.name)
  for path in sorted((ROOT/'out/v3.2-native').glob('*.mkv.jsonl')):new.write(path,'media-revision/events/'+path.name)
  for path in sorted((ROOT/'docs/images').glob('v3-*.gif')):new.write(path,'media-revision/images/'+path.name)
  for name in ['capture_v32_media.py','render_v32_film.py','motion_film.py','check_v32_media.py','revise_v32_release_media.py']:new.write(ROOT/'tools'/name,'media-revision/tools/'+name)
 temp.replace(RELEASE/NAMES[2])
 (RELEASE/'SHA256SUMS').write_text(''.join(f'{digest(RELEASE/name)}  {name}\n' for name in sorted(sums)),encoding='utf-8')
 for name,sha in stable.items():assert digest(RELEASE/name)==sha
 print(json.dumps({'updated':NAMES,'stable_archives':len(stable),'production_commit':args.production_commit,'source_commit':manifest['source_commit'],'free_before':free,'free_after':shutil.disk_usage(RELEASE).free},indent=2))

if __name__=='__main__':main()
