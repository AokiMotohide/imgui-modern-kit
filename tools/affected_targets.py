#!/usr/bin/env python3
"""Compute minimal build targets, CTest regex, and CompileOnly files from changed paths."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

# Direct mapping for source files whose test target name differs from the file stem.
SRC_TO_TESTS: dict[str, list[str]] = {
    "src/accessibility.cpp": ["imkit_design_system_test"],
    "src/component_internal.cpp": ["imkit_design_system_test"],
    "src/components.cpp": ["imkit_design_system_test"],
    "src/draw_internal.cpp": ["imkit_theme_test", "imkit_decor_test"],
    "src/easing.cpp": ["imkit_motion_test", "imkit_motion_curve_api_compile"],
    "src/editor_canvas.cpp": ["imkit_editor_core_test", "imkit_workflow_test"],
    "src/icons_catalog.inc": ["imkit_icons_test"],
    "src/icons_data.cpp": ["imkit_icons_test"],
    "src/motion_curve.cpp": ["imkit_motion_test", "imkit_motion_curve_api_compile"],
    "src/node_editor_panels.cpp": ["imkit_node_editor_test"],
    "src/node_layout.cpp": ["imkit_node_editor_test"],
    "src/patterns.cpp": ["imkit_design_system_test"],
    "src/preview.cpp": ["imkit_cg_test"],
    "src/preview_opengl3.cpp": ["imkit_editor_api_compile"],
    "src/range_slider.cpp": ["imkit_design_system_test"],
    "src/shell.cpp": ["imkit_workflow_test"],
    "src/theme_text.cpp": ["imkit_theme_test"],
    "src/video_monitors.cpp": ["imkit_video_test"],
    "src/widgets.cpp": ["imkit_part_style_test", "imkit_design_system_test"],
    "src/widgets_containers.cpp": ["imkit_design_system_test"],
    "src/widgets_controls.cpp": ["imkit_design_system_test"],
    "src/show/address.cpp": ["imkit_show_led_test"],
    "src/show/common.cpp": ["imkit_show_controls_test"],
    "src/show/led_color.cpp": ["imkit_show_led_test"],
    "src/show/led_view.cpp": ["imkit_show_led_test"],
    "src/show/rules_ui.cpp": ["imkit_show_rules_test"],
    "src/show/schedule_ui.cpp": ["imkit_show_schedule_test"],
}

DOC_EXTENSIONS = {".md", ".mdx", ".svg", ".png", ".gif", ".jpg", ".txt"}


def parse_cmake_tests() -> tuple[dict[str, list[str]], dict[str, str], dict[str, list[str]]]:
    """Return (exe_to_sources, exe_to_ctest, source_to_exes) from CMake files."""
    cmake_texts = [
        (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"),
        (ROOT / "cmake/imkitNodeEditor.cmake").read_text(encoding="utf-8"),
    ]
    combined = "\n".join(cmake_texts)
    exe_to_sources: dict[str, list[str]] = {}
    for m in re.finditer(r"add_executable\(\s*(\w+)\s+([^)]+)\)", combined, re.S):
        name = m.group(1)
        tokens = [t.strip().strip('"') for t in m.group(2).split() if not t.strip().startswith("${")]
        exe_to_sources[name] = [t for t in tokens if t.endswith((".cpp", ".mm"))]

    exe_to_ctest: dict[str, str] = {}
    for m in re.finditer(r"add_test\(\s*NAME\s+([\w.]+)\s+COMMAND\s+(\w+)", combined):
        exe_to_ctest[m.group(2)] = m.group(1)

    source_to_exes: dict[str, list[str]] = {}
    for exe, srcs in exe_to_sources.items():
        for src in srcs:
            source_to_exes.setdefault(src, []).append(exe)
    return exe_to_sources, exe_to_ctest, source_to_exes


def build_reverse_include_graph() -> dict[str, set[str]]:
    """Map each header path (repo-relative) to all files that directly or transitively include it."""
    all_files: dict[str, list[str]] = {}
    for folder in ("include", "src", "tests", "examples"):
        base = ROOT / folder
        if not base.is_dir():
            continue
        for p in base.rglob("*"):
            if p.suffix in {".h", ".hpp", ".cpp", ".mm", ".inc", ".inl"}:
                rel = p.relative_to(ROOT).as_posix()
                txt = p.read_text(encoding="utf-8", errors="ignore")
                incs = [m[1] for m in re.findall(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', txt, re.M)]
                all_files[rel] = incs

    direct_includers: dict[str, set[str]] = {}
    for rel, incs in all_files.items():
        rel_dir = pathlib.PurePosixPath(rel).parent
        for inc in incs:
            candidates = [
                f"include/{inc}",
                (rel_dir / inc).as_posix(),
                f"src/{inc}",
            ]
            for cand in candidates:
                norm = pathlib.PurePosixPath(cand).as_posix()
                # collapse .. segments cleanly
                parts: list[str] = []
                for part in norm.split("/"):
                    if part == "..":
                        if parts:
                            parts.pop()
                    elif part and part != ".":
                        parts.append(part)
                resolved = "/".join(parts)
                if resolved in all_files:
                    direct_includers.setdefault(resolved, set()).add(rel)
                    break

    def transitive_includers(header: str) -> set[str]:
        seen: set[str] = set()
        stack = [header]
        while stack:
            cur = stack.pop()
            for nxt in direct_includers.get(cur, ()):
                if nxt not in seen:
                    seen.add(nxt)
                    stack.append(nxt)
        return seen

    return {h: transitive_includers(h) for h in all_files}


def normalize_path(raw: str) -> str:
    p = pathlib.Path(raw)
    if p.is_absolute():
        try:
            return p.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            return raw.replace("\\", "/")
    return raw.replace("\\", "/").lstrip("./")


def is_doc_only(rel: str) -> bool:
    if rel.startswith(("docs/", "website/")) or rel in {"AGENTS.md", "CLAUDE.md", "README.md", "README.ja.md", "CHANGELOG.md", "LICENSE", "THIRD_PARTY_NOTICES.md"}:
        return True
    return pathlib.PurePosixPath(rel).suffix.lower() in DOC_EXTENSIONS


def ctest_regex_from_targets(targets: list[str], exe_to_ctest: dict[str, str]) -> str:
    names: list[str] = []
    for t in targets:
        if t in exe_to_ctest:
            names.append(exe_to_ctest[t])
    if not names:
        return ""
    suffixes = [re.escape(n[len("imkit.") :]) if n.startswith("imkit.") else re.escape(n) for n in sorted(set(names))]
    if all(n.startswith("imkit.") for n in names):
        return "^imkit\\.(" + "|".join(suffixes) + ")$"
    return "^(" + "|".join(re.escape(n) for n in sorted(set(names))) + ")$"


def analyze(paths: list[str]) -> dict[str, object]:
    exe_to_sources, exe_to_ctest, source_to_exes = parse_cmake_tests()
    rev_graph = build_reverse_include_graph()

    targets: set[str] = set()
    compile_only: set[str] = set()
    configure_required = False
    docs_check = False
    non_doc_seen = False
    notes: list[str] = []

    for raw in paths:
        rel = normalize_path(raw)
        if is_doc_only(rel):
            docs_check = True
            continue
        non_doc_seen = True

        if rel in {"CMakeLists.txt", "CMakePresets.json"} or rel.startswith("cmake/"):
            configure_required = True
            if rel == "cmake/imkitNodeEditor.cmake":
                targets.update(["imkit_node_editor_test", "imkit_node_editor_api_compile"])
            continue

        if rel.startswith("tools/"):
            if rel == "tools/build_icons.py":
                targets.add("imkit_icons_test")
            elif rel == "tools/generate_api.py":
                targets.add("imkit_api_compile")
            else:
                notes.append(f"ツール変更 ({rel}): 該当ツール単体を実行して確認してください")
            continue

        if rel.endswith((".cpp", ".mm")):
            compile_only.add(rel)

        # Tests directly edited
        if rel.startswith("tests/"):
            for exe in source_to_exes.get(rel, []):
                targets.add(exe)
            if rel.startswith("tests/consumer/"):
                notes.append("tests/consumer/ 変更時は ctest で imkit.consumer_build を確認してください")
            continue

        # Gallery / examples
        if rel.startswith("examples/gallery/"):
            if rel == "examples/gallery/allocation_probe.cpp":
                targets.update(["imkit_workflow_test", "imkit_design_system_test", "imkit_video_test", "imkit_gallery"])
            else:
                targets.add("imkit_gallery")
            continue
        if rel.startswith("examples/node_editor/"):
            targets.add("imkit_node_editor_gallery")
            continue
        if rel.startswith("examples/design_gallery/"):
            targets.add("imkit_gallery")
            continue

        # Library src/
        if rel.startswith("src/"):
            if rel in SRC_TO_TESTS:
                for t in SRC_TO_TESTS[rel]:
                    if t in exe_to_sources:
                        targets.add(t)
                continue
            p = pathlib.PurePosixPath(rel)
            if p.suffix in {".h", ".hpp", ".inc", ".inl"}:
                includers = rev_graph.get(rel, set())
                matched = False
                for inc in sorted(includers):
                    if inc in SRC_TO_TESTS:
                        targets.update(SRC_TO_TESTS[inc])
                        matched = True
                    elif inc.startswith("src/show/") and inc.endswith(".cpp"):
                        cand = f"imkit_show_{pathlib.PurePosixPath(inc).stem}_test"
                        if cand in exe_to_sources:
                            targets.add(cand)
                            matched = True
                    elif inc.startswith("src/") and inc.endswith(".cpp"):
                        cand = f"imkit_{pathlib.PurePosixPath(inc).stem}_test"
                        if cand in exe_to_sources:
                            targets.add(cand)
                            matched = True
                    elif inc.startswith("tests/"):
                        for exe in source_to_exes.get(inc, []):
                            if not exe.endswith("_api_compile"):
                                targets.add(exe)
                                matched = True
                if not matched:
                    targets.add("imkit_api_compile")
                continue

            # Regular .cpp in src/ or src/show/
            if rel.startswith("src/show/"):
                cand = f"imkit_show_{p.stem}_test"
                targets.add(cand if cand in exe_to_sources else "imkit_show_controls_test")
            else:
                cand = f"imkit_{p.stem}_test"
                if cand in exe_to_sources:
                    targets.add(cand)
                else:
                    targets.add("imkit_api_compile")
            continue

        # Public headers in include/imkit/
        if rel.startswith("include/imkit/"):
            includers = rev_graph.get(rel, set())
            unit_tests: list[str] = []
            api_compiles: list[str] = []
            for inc in sorted(includers):
                if inc.startswith("tests/"):
                    for exe in source_to_exes.get(inc, []):
                        if exe.endswith("_api_compile"):
                            api_compiles.append(exe)
                        else:
                            unit_tests.append(exe)
            if rel.startswith("include/imkit/show/") or rel == "include/imkit/show.h":
                targets.add("imkit_show_api_compile")
                if rel != "include/imkit/show.h":
                    stem = pathlib.PurePosixPath(rel).stem
                    cand = f"imkit_show_{stem}_test"
                    if cand in exe_to_sources:
                        targets.add(cand)
                targets.update(unit_tests)
            elif rel == "include/imkit/node_editor.h":
                targets.update(["imkit_node_editor_test", "imkit_node_editor_api_compile"])
            elif rel in {"include/imkit/imkit.h", "include/imkit/native.h"}:
                targets.add("imkit_api_compile")
            else:
                targets.update(unit_tests)
                targets.update(api_compiles)
                if "imkit_api_compile" in exe_to_sources:
                    targets.add("imkit_api_compile")
                # Also map stem directly if a dedicated test exists
                stem = pathlib.PurePosixPath(rel).stem
                for cand in (f"imkit_{stem}_test", f"imkit_{stem}_api_compile"):
                    if cand in exe_to_sources:
                        targets.add(cand)
            continue

    sorted_targets = sorted(targets)
    test_regex = ctest_regex_from_targets(sorted_targets, exe_to_ctest)
    build_required = bool(sorted_targets or configure_required)
    if non_doc_seen and not build_required and not notes:
        notes.append("該当ターゲットが見つかりませんでした。必要なら手動で -Target を指定してください")

    return {
        "paths": [normalize_path(p) for p in paths],
        "build_required": build_required,
        "configure_required": configure_required,
        "docs_check": docs_check,
        "targets": sorted_targets,
        "test_regex": test_regex,
        "compile_only": sorted(compile_only),
        "notes": notes,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="Changed file paths relative to repository root")
    parser.add_argument("--git-changed", action="store_true", help="Read changed files from git status (use only in solo tree)")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    args = parser.parse_args()

    paths = list(args.paths)
    if args.git_changed:
        out = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
        for line in out.splitlines():
            if len(line) > 3:
                entry = line[3:].split(" -> ")[-1].strip()
                if entry:
                    paths.append(entry)

    if not paths:
        print("使い方: python tools/affected_targets.py <変更パス...> [--json] [--git-changed]", file=sys.stderr)
        return 2

    result = analyze(paths)
    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0

    if not result["build_required"]:
        print("ビルド不要（文書・非コード変更のみ）")
        if result["docs_check"]:
            print("推奨確認: python tools/check_docs.py")
        for note in result["notes"]:
            print("注意:", note)
        return 0

    if result["configure_required"]:
        print("再構成が必要: & '.\\tools\\Invoke-SharedBuild.ps1' -Configure")
    if result["compile_only"]:
        print("単体コンパイル:", " ".join(result["compile_only"]))
    if result["targets"]:
        tgt_arg = ",".join(result["targets"])
        print("推奨ターゲット:", tgt_arg)
        print(f"ビルド: & '.\\tools\\Invoke-SharedBuild.ps1' -Target {tgt_arg}")
    if result["test_regex"]:
        print(f"テスト: & '.\\tools\\Invoke-SharedBuild.ps1' -Tests -TestRegex '{result['test_regex']}'")
    if result["docs_check"]:
        print("文書確認: python tools/check_docs.py")
    for note in result["notes"]:
        print("注意:", note)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
