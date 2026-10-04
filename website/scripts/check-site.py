#!/usr/bin/env python3
"""Validate built routes, local links, locale metadata, media, and generated API scope."""

from __future__ import annotations

import html
import json
import re
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
BASE = "/imgui-modern-kit/"


class PageParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.links: list[tuple[str, str]] = []
        self.ids: set[str] = set()
        self.lang: str | None = None
        self.main_count = 0
        self.images: list[dict[str, str]] = []
        self.pre_depth = 0
        self.current_code: list[str] | None = None
        self.code_blocks: list[str] = []
        self.feature_cards = 0

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = {key: value or "" for key, value in attrs}
        if self.pre_depth and "ec-line" in values.get("class", "").split() and self.current_code is not None:
            self.current_code.append("\n")
        if values.get("id"):
            self.ids.add(unquote(values["id"]))
        if tag == "html":
            self.lang = values.get("lang")
        if tag == "main":
            self.main_count += 1
        if tag == "a" and values.get("href"):
            self.links.append(("href", values["href"]))
        if tag in {"img", "script", "link", "source", "video"}:
            for attr in ("src", "href", "poster"):
                if values.get(attr):
                    self.links.append((attr, values[attr]))
        if tag == "img":
            self.images.append(values)
        if tag == "pre":
            self.pre_depth += 1
            self.current_code = []
        if tag == "article" and "feature-card" in values.get("class", "").split():
            self.feature_cards += 1

    def handle_endtag(self, tag: str) -> None:
        if tag == "pre" and self.pre_depth:
            self.pre_depth -= 1
            if self.current_code is not None:
                self.code_blocks.append("".join(self.current_code))
                self.current_code = None

    def handle_data(self, data: str) -> None:
        if self.current_code is not None and self.pre_depth:
            self.current_code.append(data)


def normalize_signature(signature: str) -> str:
    return re.sub(r"\s+", " ", signature).strip()


def target_for(page: Path, value: str) -> tuple[Path, str]:
    parsed = urlsplit(html.unescape(value))
    pathname = unquote(parsed.path)
    if not pathname:
        target = page
    elif pathname.startswith(BASE):
        target = DIST / pathname[len(BASE):]
    elif pathname.startswith("/"):
        raise ValueError(f"root path escapes configured base {BASE}: {value}")
    else:
        target = (page.parent / pathname).resolve()
        if not target.is_relative_to(DIST.resolve()):
            raise ValueError(f"relative path escapes dist: {value}")
    if target.is_dir():
        target = target / "index.html"
    elif not target.exists() and target.suffix == "":
        target = target / "index.html"
    return target, unquote(parsed.fragment)


def main() -> None:
    if not DIST.is_dir():
        raise SystemExit("website/dist is missing; run npm run build first")

    pages = sorted(DIST.rglob("*.html"))
    parsed_pages: dict[Path, PageParser] = {}
    errors: list[str] = []
    links_checked = 0

    for page in pages:
        parser = PageParser()
        parser.feed(page.read_text(encoding="utf-8"))
        parsed_pages[page] = parser
        relative = page.relative_to(DIST).as_posix()
        if relative.startswith("en/") and parser.lang != "en":
            errors.append(f"{relative}: expected lang=en, found {parser.lang!r}")
        if not relative.startswith("en/") and relative != "404.html" and parser.lang != "ja":
            errors.append(f"{relative}: expected lang=ja, found {parser.lang!r}")
        if parser.main_count != 1:
            errors.append(f"{relative}: expected one main landmark, found {parser.main_count}")
        for image in parser.images:
            if not image.get("alt", "").strip():
                errors.append(f"{relative}: image is missing alternative text")
            sources = " ".join((image.get("src", ""), image.get("srcset", "")))
            if re.search(r"\.gif(?:[?#\s,]|$)", sources, re.IGNORECASE):
                errors.append(f"{relative}: GIF is loaded directly in an img instead of a still poster/player")
        for attribute, value in parser.links:
            if not value or value.startswith(("http://", "https://", "mailto:", "tel:", "data:", "javascript:")):
                continue
            try:
                target, fragment = target_for(page, value)
            except ValueError as exc:
                errors.append(f"{relative}: {exc}")
                continue
            links_checked += 1
            if not target.is_file():
                errors.append(f"{relative}: missing local {attribute} target {value} -> {target}")
                continue
            if fragment and target.suffix == ".html":
                target_parser = parsed_pages.get(target)
                if target_parser is None:
                    target_parser = PageParser()
                    target_parser.feed(target.read_text(encoding="utf-8"))
                    parsed_pages[target] = target_parser
                if fragment not in target_parser.ids:
                    errors.append(f"{relative}: missing fragment {value} -> #{fragment}")

    native_entries = json.loads((ROOT.parent / "docs/reference/api-inventory.json").read_text(encoding="utf-8"))
    included = [entry for entry in native_entries if entry.get("included") is True]
    excluded = [entry for entry in native_entries if entry.get("included") is False]
    for locale_prefix in ("", "en/"):
        native_path = DIST / f"{locale_prefix}api/native/index.html"
        parser = parsed_pages.get(native_path)
        if parser is None:
            errors.append(f"missing API page {native_path.relative_to(DIST)}")
            continue
        code_lines = Counter(
            normalize_signature(line)
            for block in parser.code_blocks
            for line in block.splitlines()
            if line.strip()
        )
        expected = Counter(normalize_signature(entry["signature"]) for entry in included)
        missing = expected - code_lines
        leaked = Counter(normalize_signature(entry["signature"]) for entry in excluded) & code_lines
        if missing:
            errors.append(f"{locale_prefix or 'ja/'}api/native: {len(missing)} included signature rows missing; sample {sorted(missing)[:2]}")
        if leaked:
            errors.append(f"{locale_prefix or 'ja/'}api/native: excluded API signatures appear in listing; sample {sorted(leaked)[:2]}")

    node_entries = json.loads((ROOT.parent / "docs/reference/node-editor-api.json").read_text(encoding="utf-8"))
    for locale_prefix in ("", "en/"):
        node_path = DIST / f"{locale_prefix}api/node-editor/index.html"
        parser = parsed_pages.get(node_path)
        if parser is None:
            errors.append(f"missing Node API page {node_path.relative_to(DIST)}")
            continue
        code_lines = {
            normalize_signature(line)
            for block in parser.code_blocks
            for line in block.splitlines()
            if line.strip()
        }
        expected = {normalize_signature(entry["declaration"]) for entry in node_entries}
        if expected - code_lines:
            errors.append(f"{locale_prefix or 'ja/'}api/node-editor: generated declarations are incomplete")

    for catalog in (DIST / "features/index.html", DIST / "en/features/index.html"):
        parser = parsed_pages.get(catalog)
        if parser is None or parser.feature_cards != 6:
            errors.append(f"{catalog.relative_to(DIST)}: expected six feature cards")

    print(f"checked {len(pages)} HTML pages and {links_checked} local links")
    print(f"API inventory: {len(included)} included native rows, {len(excluded)} excluded, {len(node_entries)} Node declarations")
    if errors:
        print("site checks failed:")
        for error in errors:
            print(f"- {error}")
        raise SystemExit(1)
    print("all local routes/fragments, base paths, locales, media and API scopes passed")


if __name__ == "__main__":
    main()
