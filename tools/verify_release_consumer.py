#!/usr/bin/env python3
"""Compile and run the existing host-ImGui consumer against the produced SDK ZIP."""
from pathlib import Path
import argparse,subprocess,zipfile,json,re
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--generator',required=True);p.add_argument('--arch',default='');args=p.parse_args()
 build=args.build_dir.resolve();archives=list(build.glob('imgui-modern-kit-*.zip'))
 if len(archives)!=1:raise RuntimeError(f'Expected one SDK archive, found {len(archives)}')
 imgui=build/'_deps/imkit_imgui_source-src'
 pinned=re.search(r'set\(IMKIT_IMGUI_COMMIT "([0-9a-f]+)"\)',(ROOT/'cmake/Dependencies.cmake').read_text(encoding='utf-8')).group(1)
 actual=subprocess.check_output(['git','-C',str(imgui),'rev-parse','HEAD'],text=True).strip()
 if actual!=pinned:raise RuntimeError('Consumer Dear ImGui revision differs from SDK baseline')
 subprocess.run(['git','-C',str(imgui),'diff','--exit-code','HEAD','--','imconfig.h'],check=True)
 stage=build/'package-sdk';stage.mkdir(exist_ok=True)
 with zipfile.ZipFile(archives[0]) as z:
  for member in z.infolist():
   target=(stage/member.filename).resolve()
   if not target.is_relative_to(stage.resolve()):raise RuntimeError('Unsafe ZIP path')
  z.extractall(stage)
 configs=list(stage.rglob('imkitConfig.cmake'))
 if len(configs)!=1:raise RuntimeError('Missing/ambiguous installed CMake package')
 consumer=build/'package-consumer'
 # Use CMake-compatible paths; SDK and host use the same CI compiler/configuration.
 cmd=['cmake','-S',(ROOT/'tests/consumer').as_posix(),'-B',consumer.as_posix(),'-G',args.generator,f'-DIMKIT_SOURCE_DIR={ROOT.as_posix()}',f'-DIMGUI_SOURCE_DIR={imgui.as_posix()}',f'-DIMKIT_PACKAGE_DIR={configs[0].parent.as_posix()}','-DIMKIT_SDK_ABI_CONFIRMED=ON']
 if args.generator.startswith('Visual Studio'):cmd+=['-A',args.arch]
 elif args.arch:cmd+=[f'-DCMAKE_OSX_ARCHITECTURES={args.arch}','-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0']
 subprocess.run(cmd,check=True)
 subprocess.run(['cmake','--build',str(consumer),'--config','Release','--parallel','2'],check=True)
 executable=consumer/'Release'/('imkit_consumer.exe' if args.generator.startswith('Visual Studio') else 'imkit_consumer')
 subprocess.run([str(executable)],check=True)
 (build/'sdk-consumer-evidence.json').write_text(json.dumps({'archive':archives[0].name,'generator':args.generator,'arch':args.arch,'configuration':'Release','imgui_commit':actual,'abi_confirmation':'same CI compiler/configuration, pinned Dear ImGui and unmodified imconfig.h','source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'result':'compile/link all consumer fixtures and run imkit_consumer: PASS','boundary':'public API host fixture; no native OS/IME/GPU or physical acceptance'},indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
