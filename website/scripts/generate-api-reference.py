#!/usr/bin/env python3
"""Generate the signature and metadata indexes used by the website."""

from __future__ import annotations

import json
import posixpath
import re
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
CONTENT = ROOT / "website/src/content/docs"

NATIVE_CATEGORIES = {
    "Basic / Selection": "基本・選択",
    "Input / Media": "入力・メディア",
    "Numeric / Units": "数値・単位",
    "Hierarchy / Table": "階層・表",
    "Overlay / Layout": "オーバーレイ・配置",
    "Shared layout / helpers": "共通配置・補助",
}

NODE_GROUPS = [
    ("Style and connection", "スタイルと接続", [
        "MakeNodeStyle", "CanConnect", "ValidatePinEdit", "IsPinConnected",
    ]),
    ("Graph layout", "グラフの配置", [
        "AlignNodes", "DistributeNodes", "ArrangeNodes", "SnapNodesToGrid",
        "FitGroupToContents", "QueueLayout", "TraceNodes",
    ]),
    ("View and selection state", "表示と選択状態", [
        "CaptureView", "RestoreView", "RememberView", "NavigateHistory",
        "AddBookmark", "SetZoom", "FrameNodes", "IsSelected", "Select",
    ]),
    ("Editor and node drawing", "編集領域とnode描画", [
        "BeginEditor", "EndEditor", "BeginNode", "EndNode", "BeginPin", "EndPin",
        "PinRow", "PinAddRow", "GetPinPosition", "Link", "DrawLinks", "DrawNodes",
        "QueueCommand", "QueuePinEdits", "InsertNode",
    ]),
    ("Panels and previews", "パネルとプレビュー", [
        "NodePalette", "MiniMap", "Diagnostics", "LayoutToolbar", "NodeSearch",
        "NodeInspector", "ExposedProperties", "BackgroundImage", "Annotation",
        "Breadcrumbs", "Preview", "DrawDetachedPreviews",
    ]),
]


def write_pair(route: str, en_title: str, ja_title: str, en_body: str, ja_body: str) -> None:
    for locale, title, body in (("en", en_title, en_body), ("ja", ja_title, ja_body)):
        root = CONTENT / ("en" if locale == "en" else "")
        path = root / f"{route}.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            f"---\ntitle: {json.dumps(title, ensure_ascii=False)}\n---\n\n{body.strip()}\n",
            encoding="utf-8",
        )
        print(f"{path.relative_to(ROOT)}")


def local_href(current_route: str, target_route: str) -> str:
    relative = posixpath.relpath(f"/{target_route}/", f"/{current_route}/")
    return relative if relative.endswith("/") else f"{relative}/"


def generate_native_api() -> None:
    entries = json.loads((DOCS / "api-inventory.json").read_text(encoding="utf-8"))
    included = [entry for entry in entries if entry.get("included") is True]
    revision_labels = {entry.get("version") for entry in included}
    if len(revision_labels) != 1:
        raise ValueError(f"Native API rows disagree on the ImGui revision: {sorted(revision_labels)}")
    revision_label = next(iter(revision_labels))
    revision_match = re.search(r"\((\d+)\)$", revision_label or "")
    native_header = (ROOT / "include/imkit/native.h").read_text(encoding="utf-8")
    header_match = re.search(r"IMGUI_VERSION_NUM\s*!=\s*(\d+)", native_header)
    if not revision_match or not header_match or revision_match.group(1) != header_match.group(1):
        raise ValueError("API inventory revision does not match include/imkit/native.h")
    revision_name = revision_label.rsplit(" (", 1)[0]
    categories: dict[str, list[dict[str, str]]] = defaultdict(list)
    for entry in included:
        categories[entry["category"]].append(entry)

    en = [
        f"This index is generated from [`docs/api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/api-inventory.json). It lists every entry marked `included: true` in that source: **{len(included)} signature rows** across the pinned Dear ImGui `{revision_name}` revision (`{revision_match.group(1)}`). Rows marked `included: false` are omitted, including host lifecycle and debug APIs such as `CreateContext`, `NewFrame`, and `Render`.",
        "",
        f"`<imkit/native.h>` imports these Dear ImGui overloads into `namespace imkit` with their original defaults and Begin/End contracts. These are declaration listings, not standalone headers or call examples. Search the page for a function name or use the section headings to find its category. The row count does not claim that every overload received a separate interactive test; see the [integration guide]({local_href('api/native', 'getting-started')}) and [components guide]({local_href('api/native', 'features/components')}) for use, and the pinned public header for the exact implementation contract.",
        "",
    ]
    ja = [
        f"この一覧は[`docs/api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/api-inventory.json)から生成しています。`included: true`の**{len(included)}宣言行**を掲載し、Dear ImGui `{revision_name}` revision（`{revision_match.group(1)}`）に固定しています。`included: false`のContextやフレーム管理、debug用関数（`CreateContext`、`NewFrame`、`Render`など）は含みません。",
        "",
        f"`<imkit/native.h>`はDear ImGuiのoverloadを元のdefault値とBegin/End契約のまま`namespace imkit`へ公開します。以下は宣言一覧であり、単独でincludeするheaderや呼出し例ではありません。関数名をサイト内検索するか、見出しから分類を探せます。件数は、全overloadを個別に操作検証したことを示しません。使い方は[導入ガイド]({local_href('api/native', 'getting-started')})と[コンポーネントガイド]({local_href('api/native', 'features/components')})、正確な契約は固定版public headerを確認してください。",
        "",
    ]

    category_order = list(dict.fromkeys(entry["category"] for entry in included))
    en.extend(["## Function categories", ""])
    ja.extend(["## 関数の分類", ""])
    category_anchors = {
        "Basic / Selection": "native-basic-selection",
        "Input / Media": "native-input-media",
        "Numeric / Units": "native-numeric-units",
        "Hierarchy / Table": "native-hierarchy-table",
        "Overlay / Layout": "native-overlay-layout",
        "Shared layout / helpers": "native-shared-layout-helpers",
    }
    for category in category_order:
        anchor = category_anchors.get(category, "native-" + re.sub(r"[^a-z0-9]+", "-", category.lower()).strip("-"))
        ja_category = NATIVE_CATEGORIES.get(category, category)
        en.extend([f"- [{category} ({len(categories[category])})](#{anchor})"])
        ja.extend([f"- [{ja_category}（{len(categories[category])}）](#{anchor})"])
    en.append("")
    ja.append("")
    for category in category_order:
        rows = categories[category]
        ja_category = NATIVE_CATEGORIES.get(category, category)
        anchor = category_anchors.get(category, "native-" + re.sub(r"[^a-z0-9]+", "-", category.lower()).strip("-"))
        en.extend([f'<span id="{anchor}"></span>', f"## {category} ({len(rows)})", "", "```cpp", *[row["signature"] for row in rows], "```", ""])
        ja.extend([f'<span id="{anchor}"></span>', f"## {ja_category}（{len(rows)}）", "", "```cpp", *[row["signature"] for row in rows], "```", ""])

    en.extend([
        "## Related pages", "",
        f"- [Components]({local_href('api/native', 'features/components')})",
        f"- [Themes]({local_href('api/native', 'features/themes')})",
        f"- [Pinned dependency contract]({local_href('api/native', 'platform/dependencies')})",
        "- [Public native header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/native.h)",
    ])
    ja.extend([
        "## 関連ページ", "",
        f"- [コンポーネント]({local_href('api/native', 'features/components')})",
        f"- [テーマ]({local_href('api/native', 'features/themes')})",
        f"- [固定依存関係]({local_href('api/native', 'platform/dependencies')})",
        "- [公開native header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/native.h)",
    ])
    write_pair("api/native", "Dear ImGui native API", "Dear ImGui native API", "\n".join(en), "\n".join(ja))


def generate_node_api() -> None:
    entries = json.loads((DOCS / "node-editor-api.json").read_text(encoding="utf-8"))
    by_name = {entry["name"].removeprefix("imkit::node_editor::"): entry for entry in entries}
    if len(by_name) != len(entries):
        raise ValueError("Node API inventory has duplicate names; group overload declarations explicitly")
    project = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    project_version = re.search(r"project\([^\n]*VERSION\s+([0-9.]+)", project)
    if not project_version:
        raise ValueError("Cannot read the current ImKit project version from CMakeLists.txt")
    imkit_version = project_version.group(1)
    grouped_names = {name for _, _, names in NODE_GROUPS for name in names}
    actual_names = set(by_name)
    if grouped_names != actual_names:
        missing = sorted(actual_names - grouped_names)
        extra = sorted(grouped_names - actual_names)
        raise ValueError(f"Node API grouping mismatch; missing={missing}, extra={extra}")

    en = [
        f"This generated index contains the **{len(entries)} declarations** in [`docs/node-editor-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/node-editor-api.json) for `<imkit/node_editor.h>`. The header defines the exact types and contract. It is a development API in v{imkit_version}; graph data, validation, undo and persistence remain with the host.",
        "",
        f"Signatures below are declaration listings, not self-contained code examples. For the `BeginEditor` call contract, ownership, return value, and paired-call rules, see the [BeginEditor reference]({local_href('api/node-editor', 'api/node-editor/begin-editor')}). The [Node Editor guide]({local_href('api/node-editor', 'features/node-editor')}) shows the drawing order.",
        "",
    ]
    ja = [
        f"この生成一覧は[`docs/node-editor-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/node-editor-api.json)にある`<imkit/node_editor.h>`の**{len(entries)}宣言**を掲載します。型と正確な契約はheaderを基準にしてください。v{imkit_version}の開発中APIであり、グラフデータ、検証、取り消し、保存はアプリ側が管理します。",
        "",
        f"以下は宣言一覧で、単独で使うコード例ではありません。`BeginEditor`の呼び出し、所有権、戻り値、対応する終了呼び出しは[BeginEditor詳細]({local_href('api/node-editor', 'api/node-editor/begin-editor')})を参照してください。[Node Editorガイド]({local_href('api/node-editor', 'features/node-editor')})に描画順を示しています。",
        "",
    ]

    for en_label, ja_label, names in NODE_GROUPS:
        en.extend([f"## {en_label}", "", "```cpp", *[by_name[name]["declaration"] for name in names], "```", ""])
        ja.extend([f"## {ja_label}", "", "```cpp", *[by_name[name]["declaration"] for name in names], "```", ""])

    en.extend([
        "## Related pages", "",
        f"- [Node Editor feature guide]({local_href('api/node-editor', 'features/node-editor')})",
        f"- [Getting started]({local_href('api/node-editor', 'getting-started')})",
        "- [Public header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/node_editor.h)",
    ])
    ja.extend([
        "## 関連ページ", "",
        f"- [Node Editorの機能ガイド]({local_href('api/node-editor', 'features/node-editor')})",
        f"- [導入ガイド]({local_href('api/node-editor', 'getting-started')})",
        "- [公開header](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/node_editor.h)",
    ])
    write_pair("api/node-editor", "Node Editor API", "Node Editor API一覧", "\n".join(en), "\n".join(ja))


def generate_metadata_indexes() -> None:
    design = json.loads((DOCS / "design-system-api.json").read_text(encoding="utf-8"))
    en = [
        f"This map is generated from [`docs/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/design-system-api.json). It records {len(design)} public-header/type groups and the related contract page. It is header metadata, **not a complete function-signature inventory**.",
        "",
        f"Use the [design-system guide]({local_href('api/design-system', 'features/design-system')}) and each public header for behavior and declarations.",
        "",
        "| Public header | Type groups recorded in the metadata |",
        "|---|---|",
    ]
    ja = [
        f"このmapは[`docs/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/design-system-api.json)から生成しています。{len(design)}つの公開headerと型のまとまり、契約ガイドへの対応を示すheader metadataです。**関数signatureの完全一覧ではありません。**",
        "",
        f"動作と契約は[design-systemガイド]({local_href('api/design-system', 'features/design-system')})および各公開headerを確認してください。",
        "",
        "| 公開header | metadataに記録された型 |",
        "|---|---|",
    ]
    for entry in design:
        types = ", ".join(f"`{name}`" for name in entry["types"])
        en.append(f"| [`{entry['header']}`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/{entry['header'].removeprefix('include/')}) | {types} |")
        ja.append(f"| [`{entry['header']}`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/{entry['header'].removeprefix('include/')}) | {types} |")
    write_pair("api/design-system", "Design-system header map", "Design-system header一覧", "\n".join(en), "\n".join(ja))

    window = json.loads((DOCS / "window-frame-api-inventory.json").read_text(encoding="utf-8"))
    en = [
        "This map reflects the named API entries in [`docs/window-frame-api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/window-frame-api-inventory.json). The file records type and function names, not signatures; use the public header as the contract source.",
        "",
        f"Header: [`<{window['header']}>`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/{window['header']})",
        "",
        "## Types", "",
        *[f"- `{name}`" for name in window["types"]],
        "",
        "## Functions", "",
        *[f"- `{name}`" for name in window["functions"]],
        "",
        "## Optional platform targets", "",
    ]
    ja = [
        "このmapは[`docs/window-frame-api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/window-frame-api-inventory.json)に記録された名前を示します。型名と関数名のみの一覧でsignatureは含みません。契約は公開headerを基準にしてください。",
        "",
        f"Header: [`<{window['header']}>`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/{window['header']})",
        "",
        "## 型", "",
        *[f"- `{name}`" for name in window["types"]],
        "",
        "## 関数", "",
        *[f"- `{name}`" for name in window["functions"]],
        "",
        "## 任意のplatform target", "",
    ]
    for platform, target in window["optional_targets"].items():
        en.append(f"- **{platform}:** `{target['target']}` (`<{target['header']}>`, `{target['type']}`)")
        ja.append(f"- **{platform}:** `{target['target']}` (`<{target['header']}>`, `{target['type']}`)")
    en.extend(["", f"See the [WindowFrame guide]({local_href('api/window-frame', 'features/window-frame')})."])
    ja.extend(["", f"[WindowFrameガイド]({local_href('api/window-frame', 'features/window-frame')})も参照してください。"])
    write_pair("api/window-frame", "WindowFrame API map", "WindowFrame API map", "\n".join(en), "\n".join(ja))


def main() -> None:
    generate_native_api()
    generate_node_api()
    generate_metadata_indexes()


if __name__ == "__main__":
    main()
