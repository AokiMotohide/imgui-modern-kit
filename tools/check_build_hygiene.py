#!/usr/bin/env python3
"""Check build-time hygiene rules across tests, gallery, internal headers, and CMake."""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

# Tests that intentionally verify the umbrella header itself.
ALLOWED_UMBRELLA_TESTS = {
    "api_compile.cpp",
    "context_smoke.cpp",
    "design_system.cpp",
}


def check_unit_tests(errors: list[str]) -> None:
    for p in sorted((ROOT / "tests").glob("*.cpp")):
        if p.name.endswith("_api_compile.cpp") or p.name in ALLOWED_UMBRELLA_TESTS:
            continue
        txt = p.read_text(encoding="utf-8")
        if re.search(r'^\s*#\s*include\s*[<"]imkit/(imkit|show)\.h[>"]', txt, re.M):
            errors.append(f"{p.relative_to(ROOT).as_posix()}: 単体テストでアンブレラヘッダー (<imkit/imkit.h> / <imkit/show.h>) を include しています")


def check_gallery_pages(errors: list[str]) -> None:
    for p in sorted((ROOT / "examples/gallery").glob("*_page*.cpp")):
        if p.name == "start_page.cpp":
            continue
        txt = p.read_text(encoding="utf-8")
        if re.search(r'^\s*#\s*include\s*"gallery\.h"', txt, re.M):
            errors.append(f"{p.relative_to(ROOT).as_posix()}: ページ実装が gallery.h を直接 include しています (gallery_page_host.h を使ってください)")


def check_internal_headers(errors: list[str]) -> None:
    headers = list((ROOT / "src").rglob("*internal.h"))
    for p in headers:
        txt = p.read_text(encoding="utf-8")
        # Catch large static arrays inside inline functions or headers
        for m in re.finditer(r"static\s+(?:const\s+)?(?:char|unsigned\s+char|int|float)\s+\w+\s*\[(\d+)\]", txt):
            size = int(m.group(1))
            if size >= 256:
                errors.append(f"{p.relative_to(ROOT).as_posix()}: 内部ヘッダーに大型 static 配列 ([{size}]) があります (.cpp へ移動してください)")


def check_cmake_rules(errors: list[str]) -> None:
    cmake_files = [ROOT / "CMakeLists.txt", ROOT / "cmake/imkitNodeEditor.cmake"]
    combined = "\n".join(p.read_text(encoding="utf-8") for p in cmake_files)
    for m in re.finditer(r"target_precompile_headers\(\s*(\w+)\s+", combined):
        target = m.group(1)
        if target.endswith("_api_compile"):
            errors.append(f"CMake: API compile ターゲット {target} に PCH を付けてはいけません")
    if re.search(r"add_dependencies\(\s*imkit_gallery\s+imkit_node_editor_gallery\s*\)", combined):
        errors.append("CMake: imkit_gallery から imkit_node_editor_gallery への強制依存が残っています")


def main() -> int:
    errors: list[str] = []
    check_unit_tests(errors)
    check_gallery_pages(errors)
    check_internal_headers(errors)
    check_cmake_rules(errors)
    if errors:
        for err in errors:
            print(f"FAIL: {err}", file=sys.stderr)
        return 1
    print("PASS: build hygiene checks (unit tests, gallery pages, internal headers, CMake)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
