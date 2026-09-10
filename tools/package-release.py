"""Package a committed source tree and an already validated staged SDK.

Run after tools/stage-sdk.ps1 and the relocated Debug/Release consumer checks.
This tool neither builds nor publishes. Archives contain no build caches.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
out = root / "out/release"
stage = out / "sdk"
version = re.search(r"project\(imgui-modern-kit VERSION ([\d.]+)",
                    (root / "CMakeLists.txt").read_text()).group(1)
commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
if subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True).strip():
    raise RuntimeError("Commit the intended source tree before packaging")
libraries = (
    "imkit.lib", "imkitd.lib",
    "imkit_editor_core.lib", "imkit_editor_cored.lib",
    "imkit_video.lib", "imkit_videod.lib",
    "imkit_cg.lib", "imkit_cgd.lib",
    "imkit_preview_opengl3.lib", "imkit_preview_opengl3d.lib",
)
for name in libraries:
    if not (stage / "lib" / name).is_file():
        raise RuntimeError("Stage and validate both SDK configurations first")

manifest = {
    "version": version,
    "source_commit": commit,
    "dear_imgui": {"version": "1.92.9b docking", "version_num": 19291,
                   "commit": "b48d1afbe8ee8b238e2961dc363a949dd7304e23",
                   "configuration": "default imconfig.h ABI types; host supplies core"},
    "sdk": {"platform": "Windows", "architecture": "x64", "language": "C++20",
            "compiler": "MSVC 19.51.36256", "toolset": "v145",
            "windows_sdk": "10.0.26100.0", "Debug": "/MDd; imkitd.lib",
            "Release": "/MD; imkit.lib",
            "debug_metadata": "Embedded CodeView; source/object paths normalized. Executable sections and relocations unchanged."},
    "modules": ["imkit", "editor_core", "video", "cg", "editor_suite", "preview_opengl3"],
    "shell_components": ["AppBar", "WorkspaceHeader", "InspectorSection", "AdvancedSection",
                         "BottomActionBar", "DiagnosticsDrawer", "ThemePicker"],
    "font_assets": {"directory": "share/imkit/fonts", "manifest": "share/imkit/fonts/manifest.json",
                    "ownership": "host-loaded and host-owned"},
    "validation": {"api_overloads": 365, "catalog_categories": 6,
                   "input": "public Dear ImGui IO events; native OS/IME not tested",
                   "consumer": "relocated SDK Debug and Release compile/link/run",
                   "gpu": "actual OpenGL backbuffer; light/dark and representative 1.5 scale"},
    "integration": "Source build against the pinned host ImGui is the recommended route. Match all ABI/CRT settings before using the SDK."
}
manifest_bytes = (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
manifest_file = out / "manifest.json"
manifest_file.write_bytes(manifest_bytes)

source = out / f"imkit-{version}-source.zip"
source_prefix = f"imkit-{version}-source/"
subprocess.run(["git", "archive", "--format=zip", f"--prefix={source_prefix}",
                "-o", str(source), commit], cwd=root, check=True)
with zipfile.ZipFile(source, "a", zipfile.ZIP_DEFLATED) as archive:
    archive.writestr(source_prefix + "manifest.json", manifest_bytes)

sdk = out / f"imkit-{version}-windows-x64-msvc-v145.zip"
with zipfile.ZipFile(sdk, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(stage.rglob("*")):
        if path.is_file():
            archive.write(path, f"imkit-{version}-sdk/" + path.relative_to(stage).as_posix())
    archive.writestr(f"imkit-{version}-sdk/manifest.json", manifest_bytes)

evidence = out / f"imkit-{version}-evidence.zip"
with zipfile.ZipFile(evidence, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted((root / "out/catalog").glob("*")):
        if path.suffix in (".png", ".txt"):
            archive.write(path, "catalog/" + path.name)
    archive.writestr("manifest.json", manifest_bytes)

artifacts = [source, sdk, evidence, manifest_file]
checksums = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + p.name + "\n" for p in artifacts)
(out / "SHA256SUMS").write_text(checksums, encoding="ascii")
print(checksums, end="")
