#!/usr/bin/env python3
"""Check consumer-document language pairs and local Markdown links."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
PAIRS = [
    "README", "documentation-catalog", "getting-started", "guide", "gallery",
    "examples-recipes", "components", "themes", "troubleshooting", "migration-v3",
    "architecture", "dependencies", "workflow-components", "shell-components",
    "node-editor", "editor-suite", "editor-api", "timeline-editing",
    "gallery-window-frame", "icons", "design-system", "api-coverage",
    "widget-inventory", "validation",
]
PUBLIC_FILES = {ROOT / f"README{suffix}.md" for suffix in ("", ".ja")}
PUBLIC_FILES |= {DOCS / f"{name}{suffix}.md" for name in PAIRS for suffix in ("", ".ja")}
LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
HEADING_RE = re.compile(r"^\s{0,3}(#{1,6})\s+(.+?)\s*#*\s*$")
HTML_ID_RE = re.compile(r"\bid=[\"']([^\"']+)[\"']", re.IGNORECASE)


def strip_code(text: str) -> str:
    lines: list[str] = []
    fenced = False
    marker = ""
    for line in text.splitlines():
        match = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if match:
            token = match.group(1)
            if not fenced:
                fenced, marker = True, token[0]
            elif token[0] == marker:
                fenced = False
            lines.append("")
        else:
            lines.append("" if fenced else line)
    return "\n".join(lines)


def slug(value: str) -> str:
    value = re.sub(r"<[^>]+>", "", value).lower()
    value = re.sub(r"[^\w\- ]", "", value, flags=re.UNICODE)
    return re.sub(r"\s+", "-", value.strip())


def anchors(path: Path) -> set[str]:
    source = strip_code(path.read_text(encoding="utf-8"))
    result: set[str] = set()
    counts: dict[str, int] = {}
    for line in source.splitlines():
        heading = HEADING_RE.match(line)
        if heading:
            base = slug(heading.group(2))
            index = counts.get(base, 0)
            counts[base] = index + 1
            result.add(base if index == 0 else f"{base}-{index}")
        result.update(HTML_ID_RE.findall(line))
    return result


def check_links(path: Path) -> list[str]:
    failures: list[str] = []
    source = strip_code(path.read_text(encoding="utf-8"))
    for match in LINK_RE.finditer(source):
        raw = match.group(1).strip().split(maxsplit=1)[0].strip("<>")
        parsed = urlsplit(raw)
        if parsed.scheme or parsed.netloc or not parsed.path and not parsed.fragment:
            continue
        target = (path.parent / unquote(parsed.path)).resolve() if parsed.path else path.resolve()
        if not target.exists():
            failures.append(f"{path.relative_to(ROOT)}: missing link target {raw}")
            continue
        if parsed.fragment and target.is_file() and target.suffix.lower() == ".md":
            if unquote(parsed.fragment) not in anchors(target):
                failures.append(f"{path.relative_to(ROOT)}: missing anchor {raw}")
    return failures


def main() -> int:
    errors: list[str] = []
    for name in PAIRS:
        english = DOCS / f"{name}.md"
        japanese = DOCS / f"{name}.ja.md"
        for path in (english, japanese):
            if not path.is_file():
                errors.append(f"missing language pair member: {path.relative_to(ROOT)}")
        if english.is_file() and japanese.is_file():
            for page, counterpart in ((english, japanese.name), (japanese, english.name)):
                content = strip_code(page.read_text(encoding="utf-8"))
                if counterpart not in content[:500]:
                    errors.append(f"missing language switch near top: {page.relative_to(ROOT)} -> {counterpart}")
            en_headings = sum(1 for line in strip_code(english.read_text(encoding="utf-8")).splitlines()
                              if re.match(r"^\s{0,3}##\s", line))
            ja_headings = sum(1 for line in strip_code(japanese.read_text(encoding="utf-8")).splitlines()
                              if re.match(r"^\s{0,3}##\s", line))
            if en_headings != ja_headings:
                errors.append(f"section count differs: {english.relative_to(ROOT)} ({en_headings}) vs {japanese.relative_to(ROOT)} ({ja_headings})")
            en_steps = len(re.findall(r"^\s*\d+[.)]\s+", strip_code(english.read_text(encoding="utf-8")), re.MULTILINE))
            ja_steps = len(re.findall(r"^\s*\d+[.)]\s+", strip_code(japanese.read_text(encoding="utf-8")), re.MULTILINE))
            if en_steps != ja_steps:
                errors.append(f"numbered procedure count differs: {english.relative_to(ROOT)} ({en_steps}) vs {japanese.relative_to(ROOT)} ({ja_steps})")
    for path in sorted(PUBLIC_FILES):
        if not path.is_file():
            errors.append(f"missing public entry page: {path.relative_to(ROOT)}")
        else:
            errors.extend(check_links(path))
    gallery_source = (ROOT / "examples/gallery/gallery.cpp").read_text(encoding="utf-8")
    gallery_guides = [DOCS / "gallery.md", DOCS / "gallery.ja.md"]
    for page, action, title in (
        (0, "Open components", "Components: Basic"),
        (6, "Open themes and icons", "Icons"),
        (15, "Open workflow", "Generic Workspace"),
        (8, "Open timeline", "Video"),
    ):
        if f'StartCard(s, {page},' not in gallery_source or f'"{action}"' not in gallery_source:
            errors.append(f"Gallery Start route is missing page {page}: {action}")
        for guide in gallery_guides:
            if title not in guide.read_text(encoding="utf-8"):
                errors.append(f"{guide.relative_to(ROOT)} does not describe Gallery route {title}")
    if errors:
        print("Documentation check failed:", file=sys.stderr)
        print("\n".join(f"- {error}" for error in errors), file=sys.stderr)
        return 1
    print(f"Documentation check passed: {len(PAIRS)} English/Japanese pairs and local links in {len(PUBLIC_FILES)} public pages.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
