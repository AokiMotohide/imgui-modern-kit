#!/usr/bin/env python3
"""Assemble and verify ImKit release metadata around CI-produced CPack archives.

This tool does not build or publish. Run it from the committed release commit.
Pass --packages-dir after downloading and extracting the five GitHub Actions
artifacts. The native showcase must already exist in out/release.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "out/release"
EXPECTED_PACKAGES = {
    "windows-x64": re.compile(r"imgui-modern-kit-.*-Windows-x64\.zip$", re.I),
    "windows-arm64": re.compile(r"imgui-modern-kit-.*-Windows-arm64\.zip$", re.I),
    "macos-arm64": re.compile(r"imgui-modern-kit-.*-Darwin-arm64\.zip$", re.I),
    "macos-x86_64": re.compile(r"imgui-modern-kit-.*-Darwin-x86_64\.zip$", re.I),
    "macos-universal2": re.compile(r"imgui-modern-kit-.*-Darwin-universal2\.zip$", re.I),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def one_member(names: set[str], suffix: str) -> bool:
    return any(name.replace("\\", "/").endswith(suffix) for name in names)


def verify_package(key: str, archive: Path) -> dict:
    with zipfile.ZipFile(archive) as package:
        names = {name.rstrip("/") for name in package.namelist()}
    common = [
        "README.md",
        "README.ja.md",
        "LICENSE",
        "THIRD_PARTY_NOTICES.md",
        "docs/README.md",
        "docs/README.ja.md",
        "docs/components.md",
        "docs/components.ja.md",
        "docs/getting-started.md",
        "docs/getting-started.ja.md",
        "docs/node-editor.md",
        "docs/node-editor.ja.md",
    ]
    missing = [name for name in common if not one_member(names, name)]
    if key.startswith("windows"):
        required = [
            "lib/imkit.lib",
            "lib/imkit_node_editor.lib",
            "bin/imkit_gallery.exe",
            "bin/imkit_node_editor_gallery.exe",
        ]
    else:
        required = [
            "lib/libimkit.a",
            "lib/libimkit_node_editor.a",
            "imkit_gallery.app/Contents/MacOS/imkit_gallery",
            "imkit_node_editor_gallery.app/Contents/MacOS/imkit_node_editor_gallery",
        ]
    missing.extend(name for name in required if not one_member(names, name))
    if missing:
        raise RuntimeError(f"{archive.name} is incomplete: {', '.join(missing)}")
    return {
        "file": archive.name,
        "sha256": sha256(archive),
        "entries": len(names),
        "verified": common + required,
    }


def find_packages(directory: Path) -> dict[str, Path]:
    archives = list(directory.rglob("*.zip"))
    found: dict[str, Path] = {}
    for key, pattern in EXPECTED_PACKAGES.items():
        matches = [path for path in archives if pattern.search(path.name)]
        if len(matches) != 1:
            raise RuntimeError(f"expected one {key} CPack archive, found {len(matches)}")
        found[key] = matches[0]
    return found


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--packages-dir", type=Path)
    args = parser.parse_args()
    version_match = re.search(
        r"project\(imgui-modern-kit VERSION ([\d.]+)", (ROOT / "CMakeLists.txt").read_text()
    )
    if not version_match:
        raise RuntimeError("project version was not found")
    version = version_match.group(1)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    dirty = subprocess.check_output(
        ["git", "status", "--porcelain", "--untracked-files=no"], cwd=ROOT, text=True
    ).strip()
    if dirty:
        raise RuntimeError("commit the intended source tree before assembling the release")

    OUT.mkdir(parents=True, exist_ok=True)
    showcase = OUT / f"imkit-v{version}-showcase.mp4"
    if not showcase.is_file():
        raise RuntimeError(f"missing native showcase: {showcase}")

    package_records = {}
    package_files: list[Path] = []
    if args.packages_dir:
        package_dir = args.packages_dir.resolve()
        for key, source in find_packages(package_dir).items():
            destination = OUT / source.name
            if source.resolve() != destination.resolve():
                shutil.copy2(source, destination)
            package_records[key] = verify_package(key, destination)
            package_files.append(destination)

    manifest = {
        "version": version,
        "tag": f"v{version}",
        "source_commit": commit,
        "language": "C++20",
        "license": "MIT",
        "dear_imgui": {
            "version": "1.93.0 WIP docking",
            "version_num": 19297,
            "commit": "367b2c24f399988ddafc0bb4628da0106bcc09be",
            "configuration": "matching imconfig.h and ABI required; host supplies core",
        },
        "platforms": [
            "Windows x64",
            "Windows Arm64",
            "macOS arm64",
            "macOS x86_64",
            "macOS Universal 2",
        ],
        "modules": [
            "imkit",
            "node_editor",
            "editor_core",
            "video",
            "cg",
            "editor_suite",
            "preview_opengl3",
            "preview_metal",
            "window_frame_win32",
            "window_frame_macos",
            "accessibility_win32",
            "accessibility_macos",
        ],
        "host_ownership": [
            "Dear ImGui context and backends",
            "renderer and platform windows",
            "font atlas and textures",
            "application graph, scene and media data",
            "Undo, persistence and workers",
        ],
        "documentation_media": {
            "gif_size": "960x540",
            "gif_count": 5,
            "showcase": showcase.name,
            "source": "native Gallery and Node Editor companion backbuffers only",
        },
        "signing": {
            "macos": "unsigned and not notarized when CI release credentials are unavailable"
        },
        "validation_boundary": (
            "CI build/test/package and native automated smoke do not establish physical input, "
            "native IME, real screen-reader, mixed-DPI, external-host or notarization acceptance"
        ),
        "packages": package_records,
    }
    manifest_path = OUT / f"imkit-v{version}-manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    source = OUT / f"imkit-v{version}-source.zip"
    subprocess.run(
        ["git", "archive", "--format=zip", f"--prefix=imgui-modern-kit-{version}/",
         f"--output={source}", commit],
        cwd=ROOT,
        check=True,
    )

    evidence = OUT / f"imkit-v{version}-validation-evidence.zip"
    with zipfile.ZipFile(evidence, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(ROOT / "docs/validation.md", "validation.md")
        archive.write(manifest_path, manifest_path.name)
        for gif in sorted((ROOT / "docs/images").glob("v3-*.gif")):
            archive.write(gif, f"images/{gif.name}")
        capture_sources = {
            "overview": ROOT / "out/v3-native/overview/capture.txt",
            "node-editor": ROOT / "out/v3-native/node-editor-final/capture.txt",
            "workflow-progress": ROOT / "out/v3-native/workflow/capture.txt",
            "timeline": ROOT / "out/v3-native/timeline/capture.txt",
            "theme-comparison": ROOT / "out/v3-native/themes/capture.txt",
        }
        for name, metadata in capture_sources.items():
            if not metadata.is_file():
                raise RuntimeError(f"missing capture metadata: {metadata}")
            archive.write(metadata, f"captures/{name}.txt")

    artifacts = sorted(
        {source, evidence, manifest_path, showcase, *package_files}, key=lambda path: path.name.lower()
    )
    checksums = "".join(f"{sha256(path)}  {path.name}\n" for path in artifacts)
    (OUT / "SHA256SUMS").write_text(checksums, encoding="ascii")
    print(checksums, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
