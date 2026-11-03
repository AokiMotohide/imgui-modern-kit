#!/usr/bin/env python3
"""Copy maintained bilingual consumer docs into the Starlight routes."""

from __future__ import annotations

import json
import posixpath
import re
import shutil
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
CONTENT = ROOT / "website/src/content/docs"
PUBLIC_IMAGES = ROOT / "website/public/docs-images"
BASE = "/imgui-modern-kit/"
RAW_BASE = "https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/"

PAGES = {
    # Start here
    "getting-started": ("getting-started", "Getting started", "導入ガイド"),
    "how-it-works": ("guides/how-it-works", "How ImKit works", "仕組みと設計"),
    "guide": ("guides/user-guide", "User guide", "利用ガイド"),
    "gallery": ("guides/gallery", "Gallery guide", "Galleryガイド"),
    "examples-recipes": ("guides/examples", "Examples and recipes", "実例とレシピ"),
    # Tutorials
    "build-first-app": ("tutorials/first-app", "Build your first app", "最初のアプリを作る"),
    "build-settings-screen": ("tutorials/settings-screen", "Build a settings screen", "設定画面を作る"),
    "build-node-editor": ("tutorials/node-editor", "Build a node editor", "ノードエディタを作る"),
    "build-timeline": ("tutorials/timeline", "Build a timeline", "タイムラインを作る"),
    "custom-component": ("tutorials/custom-component", "Build a custom component", "カスタムコンポーネントを作る"),
    # Features
    "components": ("features/components", "Components", "基本コンポーネント"),
    "themes": ("features/themes", "Themes", "テーマ"),
    "make-your-own-look": ("features/make-your-own-look", "Make your own look", "自分の世界観を作る"),
    "design-system": ("features/design-system", "Design system", "デザインシステム"),
    "icons": ("features/icons", "Icons", "アイコン"),
    "toasts": ("features/toasts", "Toasts", "トースト"),
    "charts": ("features/charts", "Charts", "チャート"),
    "ptz": ("features/ptz", "PTZ and range sliders", "PTZ操作と範囲スライダー"),
    "workflow-components": ("features/workflow", "Workflow components", "Workflow部品"),
    "shell-components": ("features/shell", "Shell components", "シェル部品"),
    "node-editor": ("features/node-editor", "Node Editor", "ノードエディタ"),
    "editor-suite": ("features/editor-suite", "Editor Suite", "エディタスイート"),
    "timeline-editing": ("features/timeline", "Timeline editing", "タイムライン編集"),
    "show": ("features/show", "Show module", "Showモジュール"),
    "gallery-window-frame": ("features/window-frame", "Window frame", "ウィンドウフレーム"),
    # Design, platform
    "architecture": ("guide/architecture", "Architecture", "アーキテクチャ"),
    "dependencies": ("platform/dependencies", "Dependencies", "依存関係"),
    "troubleshooting": ("platform/troubleshooting", "Troubleshooting", "トラブルシューティング"),
    "migration-v3": ("platform/migration", "Migrate from v2", "v2からの移行"),
    # API
    "editor-api": ("api/editor-suite", "Editor API", "Editor API"),
    "widget-inventory": ("api/widgets", "Component inventory", "部品一覧"),
}

SOURCES = {'README': ('README.md', '目次.md'), 'getting-started': ('getting-started/getting-started.md', 'getting-started/導入ガイド.md'), 'how-it-works': ('getting-started/how-it-works.md', 'getting-started/仕組みと設計思想.md'), 'guide': ('getting-started/guide.md', 'getting-started/利用ガイド.md'), 'gallery': ('getting-started/gallery.md', 'getting-started/ギャラリーガイド.md'), 'examples-recipes': ('getting-started/examples-recipes.md', 'getting-started/実例とレシピ.md'), 'build-first-app': ('tutorials/build-first-app.md', 'tutorials/最初のアプリの作成.md'), 'build-settings-screen': ('tutorials/build-settings-screen.md', 'tutorials/設定画面の作成.md'), 'build-node-editor': ('tutorials/build-node-editor.md', 'tutorials/ノードエディタの作成.md'), 'build-timeline': ('tutorials/build-timeline.md', 'tutorials/タイムラインの作成.md'), 'custom-component': ('tutorials/custom-component.md', 'tutorials/カスタムコンポーネントの作成.md'), 'architecture': ('architecture/architecture.md', 'architecture/アーキテクチャ.md'), 'design-system': ('architecture/design-system.md', 'architecture/デザインシステム.md'), 'themes': ('architecture/themes.md', 'architecture/テーマ.md'), 'icons': ('architecture/icons.md', 'architecture/アイコン.md'), 'dependencies': ('architecture/dependencies.md', 'architecture/依存関係.md'), 'toasts': ('components/toasts.md', 'components/トースト.md'), 'components': ('components/components.md', 'components/基本コンポーネント.md'), 'node-editor': ('components/node-editor.md', 'components/ノードエディタ.md'), 'timeline-editing': ('components/timeline-editing.md', 'components/タイムライン編集.md'), 'editor-suite': ('components/editor-suite.md', 'components/エディタスイート.md'), 'workflow-components': ('components/workflow-components.md', 'components/ワークフローコンポーネント.md'), 'shell-components': ('components/shell-components.md', 'components/シェルコンポーネント.md'), 'gallery-window-frame': ('components/gallery-window-frame.md', 'components/ウィンドウフレーム.md'), 'api-coverage': ('reference/api-coverage.md', 'reference/公開API一覧.md'), 'editor-api': ('reference/editor-api.md', 'reference/エディタAPI.md'), 'widget-inventory': ('reference/widget-inventory.md', 'reference/ウィジェット一覧.md'), 'documentation-catalog': ('reference/documentation-catalog.md', 'reference/文書カタログ.md'), 'validation': ('reference/validation.md', 'reference/検証記録.md'), 'migration-v3': ('reference/migration-v3.md', 'reference/v3移行ガイド.md'), 'troubleshooting': ('reference/troubleshooting.md', 'reference/トラブルシューティング.md')}
SOURCES["charts"]=("components/charts.md","components/チャート.md")
SOURCES["ptz"]=("components/ptz.md","components/PTZ操作と範囲スライダー.md")
SOURCES["show"]=("components/show.md","components/ショーモジュール.md")
SOURCES["make-your-own-look"]=("architecture/make-your-own-look.md","architecture/世界観を作る.md")
SOURCES["v3.2-api"]=("reference/v3.2-api.md","reference/3.2追加API.md")
PAGES["v3.2-api"]=("api/v3-2", "ImKit 3.2 API", "ImKit 3.2 API")
SOURCES["v3.3-api"]=("reference/v3.3-api.md","reference/3.3追加API.md")
PAGES["v3.3-api"]=("api/v3-3", "ImKit 3.3 API", "ImKit 3.3 API")
SOURCES["v4.0-api"]=("reference/v4.0-api.md","reference/4.0追加API.md")
PAGES["v4.0-api"]=("api/v4-0", "ImKit 4.0 API", "ImKit 4.0 API")
SOURCES["migration-v4"]=("reference/migration-v4.md","reference/v4移行ガイド.md")
PAGES["migration-v4"]=("platform/migration-v4", "Migrate from v3", "v3からの移行")

JSON_ROUTES = {
    "docs/reference/api-inventory.json": "api/native",
    "docs/reference/node-editor-api.json": "api/node-editor",
    "docs/reference/design-system-api.json": "api/design-system",
    "docs/reference/window-frame-api-inventory.json": "api/window-frame",
}

LANG_LINE = re.compile(r"^\s*\[(?:日本語|English)\]\([^)]*\)")
LINK = re.compile(r"(!?)\[([^\]]*)\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")


def docs_path_map() -> dict[str, tuple[str, str]]:
    result: dict[str, tuple[str, str]] = {}
    for slug, (route, _en_title, _ja_title) in PAGES.items():
        en,ja=SOURCES[slug]
        result[f"docs/{en}"]=(route,"en")
        result[f"docs/{ja}"]=(route,"ja")
    result["docs/reference/api-coverage.md"]=("api/native","en")
    result["docs/reference/公開API一覧.md"]=("api/native","ja")
    return result


DOC_ROUTES = docs_path_map()


def github_blob(path: str, anchor: str = "") -> str:
    return f"https://github.com/AokiMotohide/imgui-modern-kit/blob/main/{path}{anchor}"


def local_route(current: str, current_locale: str, target: str, target_locale: str, anchor: str) -> str:
    current_prefix = "en/" if current_locale == "en" else ""
    target_prefix = "en/" if target_locale == "en" else ""
    current_path = f"/{current_prefix}{current}/"
    target_path = f"/{target_prefix}{target}/"
    relative = posixpath.relpath(target_path, current_path)
    if not relative.endswith("/"):
        relative += "/"
    return relative + anchor


def rewrite_destination(destination: str, source_locale: str, current_route: str, source: Path) -> str:
    if destination.startswith(("https://", "http://", "mailto:", "tel:", "data:", "#", "/")):
        return destination

    path, marker, fragment = destination.partition("#")
    anchor = f"#{fragment}" if marker else ""
    normalized = posixpath.normpath(posixpath.join(source.parent.relative_to(ROOT).as_posix(), unquote(path)))

    if normalized.startswith("docs/images/"):
        image_name = normalized.removeprefix("docs/images/")
        source = ROOT / normalized
        if image_name.endswith(".gif"):
            # GIFs stay in the repository; the site links to them instead of storing a second copy.
            return f"{RAW_BASE}{image_name}{anchor}"
        if source.is_file():
            PUBLIC_IMAGES.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, PUBLIC_IMAGES / image_name)
            return f"{BASE}docs-images/{image_name}{anchor}"

    if normalized.startswith("docs/diagrams/"):
        diagram = ROOT / normalized
        if diagram.is_file():
            PUBLIC_IMAGES.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(diagram, PUBLIC_IMAGES / diagram.name)
            return f"{BASE}docs-images/{diagram.name}{anchor}"

    if normalized in DOC_ROUTES:
        target, target_locale = DOC_ROUTES[normalized]
        return local_route(current_route, source_locale, target, target_locale, anchor)

    if normalized in JSON_ROUTES:
        target_locale = "en" if source_locale == "en" else "ja"
        return local_route(current_route, source_locale, JSON_ROUTES[normalized], target_locale, anchor)

    return github_blob(normalized, anchor)


def prepare_body(source: Path, locale: str, current_route: str) -> str:
    lines = source.read_text(encoding="utf-8").splitlines()
    if lines and lines[0].startswith("# "):
        lines.pop(0)
    while lines and not lines[0].strip():
        lines.pop(0)
    if lines and LANG_LINE.match(lines[0]):
        lines.pop(0)
    text = "\n".join(lines).strip()

    def substitute(match: re.Match[str]) -> str:
        image, label, destination = match.groups()
        url = rewrite_destination(destination, locale, current_route, source)
        return f"{image}[{label}]({url})"

    text=LINK.sub(substitute, text)
    def native_image(match):
        destination,alt=match.groups()
        url=rewrite_destination(destination,locale,current_route,source)
        if destination.endswith(".gif"):
            stem=Path(destination).stem
            poster=ROOT/"website/src/assets/captures"/(stem+"-poster.png")
            PUBLIC_IMAGES.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(poster,PUBLIC_IMAGES/poster.name)
            return f"![{alt}]({BASE}docs-images/{poster.name})\n\n[{'Open native GIF' if locale=='en' else '操作GIFを開く'}]({url})"
        return f'![{alt}]({url})'
    text=re.sub(r'<img src="([^"]+)" alt="([^"]+)"[^>]*>',native_image,text)
    return text.strip()


def summarize(body: str, limit: int = 150) -> str:
    """First prose paragraph of a page, as plain text, for the page description."""
    for block in re.split(r"\n\s*\n", body):
        text = block.strip()
        if not text or text.startswith(("#", "<", "|", "```", "!", "-", "*", ">", "[!")):
            continue
        text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
        text = re.sub(r"[`*_]", "", text)
        text = " ".join(text.split())
        if len(text) > limit:
            cut = max(text.rfind("。", 0, limit), text.rfind(". ", 0, limit))
            text = text[: cut + 1] if cut > 40 else text[:limit].rstrip() + "…"
        return text
    return ""


def write_page(slug: str, route: str, locale: str, title: str) -> None:
    source = DOCS / SOURCES[slug][0 if locale=="en" else 1]
    if not source.is_file():
        raise FileNotFoundError(source)
    output_root = CONTENT / ("en" if locale == "en" else "")
    output = output_root / f"{route}.md"
    output.parent.mkdir(parents=True, exist_ok=True)
    body = prepare_body(source, locale, route)
    description = summarize(body)
    front = f"---\ntitle: {json.dumps(title, ensure_ascii=False)}\n"
    if description:
        front += f"description: {json.dumps(description, ensure_ascii=False)}\n"
    output.write_text(f"{front}---\n\n{body}\n", encoding="utf-8")
    print(f"{source.relative_to(ROOT)} -> {output.relative_to(ROOT)}")


def main() -> None:
    for slug, (route, en_title, ja_title) in PAGES.items():
        write_page(slug, route, "en", en_title)
        write_page(slug, route, "ja", ja_title)


if __name__ == "__main__":
    main()
