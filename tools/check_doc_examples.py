#!/usr/bin/env python3
"""Compile the C++ examples in the public docs against the real ImKit headers.

Each ```cpp block is compiled (syntax only) as it is and wrapped in a function. Only errors that
concern ImKit or a missing member are reported, because the examples use host variables that are
not declared here. An error on a line that also uses an undeclared name is skipped, so this finds
renamed or removed APIs and wrong signatures; it is not a full proof that every example runs.

    python tools/check_doc_examples.py --imgui <pinned Dear ImGui checkout> [docs/tutorials/build-first-app.md ...]

With no files, every public page in tools/check_docs.py is checked. Needs a C++20 compiler (g++ or clang++).
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import check_docs  # noqa: E402

PRELUDE = """#include <imgui.h>
#include <imkit/imkit.h>
#include <imkit/video.h>
#include <imkit/cg.h>
#include <imkit/editor_core.h>
#include <imkit/editor_canvas.h>
#include <imkit/node_editor.h>
#include <imkit/show.h>
#include <imkit/preview.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>
"""
RELEVANT = re.compile(r"imkit|has no member named|is not a member of")
SYNTAX = re.compile(r"expected|stray|does not name a type|before token")
BLOCK = re.compile(r"^```(?:cpp|c\+\+)\n(.*?)^```", re.S | re.M)


def compile_source(compiler: str, imgui: Path, source: str, workdir: Path) -> tuple[list[str], list[str]]:
    unit = workdir / "example.cpp"
    unit.write_text(source, encoding="utf-8")
    run = subprocess.run(
        [compiler, "-std=c++20", "-fsyntax-only", "-fmax-errors=30", "-I", str(ROOT / "include"), "-I", str(imgui), str(unit)],
        capture_output=True, text=True, env={**os.environ, "LC_ALL": "C", "LANG": "C"})
    errors = [line for line in run.stderr.splitlines() if "error" in line]
    undeclared = set()
    for line in errors:
        match = re.match(r".*?:(\d+):", line)
        if match and "not declared in this scope" in line:
            undeclared.add(match.group(1))
    kept = []
    for line in errors:
        match = re.match(r".*?:(\d+):", line)
        if RELEVANT.search(line) and not (match and match.group(1) in undeclared):
            kept.append(line)
    return kept, errors


def check_page(compiler: str, imgui: Path, page: Path, workdir: Path) -> list[str]:
    text = page.read_text(encoding="utf-8")
    problems: list[str] = []
    for match in BLOCK.finditer(text):
        code = match.group(1)
        if "imkit" not in code:
            continue
        line = text[: match.start()].count("\n") + 2
        top_level = code.lstrip().startswith("#include") or re.search(r"^(struct|namespace|enum|class)\b", code, re.M) or \
            re.search(r"^\s*(void|bool|auto|static|[\w:<>]+)\s+\w+\s*\(.*\)\s*(const)?\s*\{", code, re.M)
        # Examples commonly use a `theme` and an `animation` that the application owns, so a wrapped example sees both.
        wrapped = PRELUDE + "static imkit::Theme theme;\nstatic imkit::AnimationState animation;\nvoid doc_fn() {\n" + code + "\n}\n"
        variants = [PRELUDE + code, wrapped]
        if not top_level:
            variants.reverse()
        best, best_key = [], None
        for source in variants:
            kept, errors = compile_source(compiler, imgui, source, workdir)
            key = (sum(1 for e in errors if SYNTAX.search(e)), len(errors))
            if best_key is None or key < best_key:
                best, best_key = kept, key
        if best:
            problems.append(f"{page.relative_to(ROOT)}:{line}: {best[0].split(': error:', 1)[-1].strip()[:200]}")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--imgui", type=Path, required=True, help="Dear ImGui checkout at the pinned commit")
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("pages", nargs="*", type=Path)
    args = parser.parse_args()
    pages = [ROOT / p if not p.is_absolute() else p for p in args.pages] or sorted(check_docs.PUBLIC_FILES)
    problems: list[str] = []
    with tempfile.TemporaryDirectory() as tmp:
        for page in pages:
            problems.extend(check_page(args.compiler, args.imgui, page, Path(tmp)))
    if problems:
        print("Documentation examples reference APIs that do not compile:", file=sys.stderr)
        print("\n".join(f"- {p}" for p in problems), file=sys.stderr)
        return 1
    print(f"Documentation examples compile against the public headers in {len(pages)} pages.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
