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

PAGES = {
    "components": ("features/components", "Components", "コンポーネント"),
    "themes": ("features/themes", "Themes", "テーマ"),
    "workflow-components": ("features/workflow", "Workflow components", "Workflow部品"),
    "shell-components": ("features/shell", "Application shell components", "アプリケーションShell部品"),
    "editor-suite": ("features/editor-suite", "Editor Suite", "Editor Suite"),
    "timeline-editing": ("features/timeline", "Timeline editing", "Timeline編集"),
    "gallery-window-frame": ("features/window-frame", "WindowFrame", "WindowFrame"),
    "icons": ("features/icons", "Icons", "アイコン"),
    "examples-recipes": ("guides/examples", "Examples and recipes", "実例とレシピ"),
    "gallery": ("guides/gallery", "Gallery guide", "Galleryガイド"),
    "dependencies": ("platform/dependencies", "Dependencies", "依存関係"),
    "troubleshooting": ("platform/troubleshooting", "Troubleshooting", "トラブルシューティング"),
    "migration-v3": ("platform/migration", "Migrate from v2", "v2からの移行"),
    "architecture": ("guide/architecture", "Architecture", "アーキテクチャ"),
    "editor-api": ("api/editor-suite", "Editor API", "Editor API"),
    "widget-inventory": ("api/widgets", "Component inventory", "部品一覧"),
}

SOURCES = {'README': ('README.md', '目次.md'), 'getting-started': ('getting-started/getting-started.md', 'getting-started/導入ガイド.md'), 'how-it-works': ('getting-started/how-it-works.md', 'getting-started/仕組みと設計思想.md'), 'guide': ('getting-started/guide.md', 'getting-started/利用ガイド.md'), 'gallery': ('getting-started/gallery.md', 'getting-started/ギャラリーガイド.md'), 'examples-recipes': ('getting-started/examples-recipes.md', 'getting-started/実例とレシピ.md'), 'build-first-app': ('tutorials/build-first-app.md', 'tutorials/最初のアプリの作成.md'), 'build-settings-screen': ('tutorials/build-settings-screen.md', 'tutorials/設定画面の作成.md'), 'build-node-editor': ('tutorials/build-node-editor.md', 'tutorials/ノードエディタの作成.md'), 'build-timeline': ('tutorials/build-timeline.md', 'tutorials/タイムラインの作成.md'), 'custom-component': ('tutorials/custom-component.md', 'tutorials/カスタムコンポーネントの作成.md'), 'architecture': ('architecture/architecture.md', 'architecture/アーキテクチャ.md'), 'design-system': ('architecture/design-system.md', 'architecture/デザインシステム.md'), 'themes': ('architecture/themes.md', 'architecture/テーマ.md'), 'icons': ('architecture/icons.md', 'architecture/アイコン.md'), 'dependencies': ('architecture/dependencies.md', 'architecture/依存関係.md'), 'toasts': ('components/toasts.md', 'components/トースト.md'), 'components': ('components/components.md', 'components/基本コンポーネント.md'), 'node-editor': ('components/node-editor.md', 'components/ノードエディタ.md'), 'timeline-editing': ('components/timeline-editing.md', 'components/タイムライン編集.md'), 'editor-suite': ('components/editor-suite.md', 'components/エディタスイート.md'), 'workflow-components': ('components/workflow-components.md', 'components/ワークフローコンポーネント.md'), 'shell-components': ('components/shell-components.md', 'components/シェルコンポーネント.md'), 'gallery-window-frame': ('components/gallery-window-frame.md', 'components/ウィンドウフレーム.md'), 'api-coverage': ('reference/api-coverage.md', 'reference/公開API一覧.md'), 'editor-api': ('reference/editor-api.md', 'reference/エディタAPI.md'), 'widget-inventory': ('reference/widget-inventory.md', 'reference/ウィジェット一覧.md'), 'documentation-catalog': ('reference/documentation-catalog.md', 'reference/文書カタログ.md'), 'validation': ('reference/validation.md', 'reference/検証記録.md'), 'migration-v3': ('reference/migration-v3.md', 'reference/v3移行ガイド.md'), 'troubleshooting': ('reference/troubleshooting.md', 'reference/トラブルシューティング.md')}
SOURCES["v3.2-api"]=("reference/v3.2-api.md","reference/3.2追加API.md")
PAGES["v3.2-api"]=("api/v3-2", "ImKit 3.2 API", "ImKit 3.2 API")
PAGES["toasts"] = ("features/toasts", "Toast notifications", "トースト通知")

FEATURE_LEADS = {
    "components": {
        "capture": "v3-overview",
        "alt_en": "Native ImKit Gallery capture showing the integrated toolkit interface.",
        "alt_ja": "ImKit Galleryに表示されたUI部品の画面。",
        "caption_en": "Components — settings, selection and feedback",
        "caption_ja": "Components — 設定、選択、状態表示",
        "use_en": "Use the components module when a creative tool needs consistent settings rows, buttons, inputs and feedback while keeping its existing Dear ImGui context and application values.",
        "use_ja": "設定行、ボタン、入力欄、状態表示を揃えたいときに使います。Dear ImGuiのContextとアプリの値はそのままアプリ側で管理します。",
        "code": '''#include <imkit/imkit.h>

void DrawApplyButton() {
    if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary)) {
        // The application validates and applies its own document state.
    }
}''',
        "kind_en": "Function excerpt for an existing application frame. Call it between `NewFrame` and `Render`, then handle the returned activation in the application.",
        "kind_ja": "既存アプリのフレーム内で呼ぶ関数例です。`NewFrame`から`Render`までの間に呼び、戻り値の操作はアプリ側で処理します。",
        "integration_en": "Link `imkit::imkit`. Edited values and stable IDs stay in the application; use the returned boolean as an action request.",
        "integration_ja": "`imkit::imkit`をlinkします。編集値とIDはアプリ側で保持し、戻り値を操作要求として扱います。",
        "scope_en": "These are immediate-mode controls, not an application model, persistence layer or command dispatcher.",
        "scope_ja": "即時モードの入力部品です。アプリのデータモデル、保存機能、command dispatcherは提供しません。",
        "links": [("Native API index", "api/native"), ("Component inventory", "api/widgets")],
        "links_ja": [("Native API一覧", "api/native"), ("部品一覧", "api/widgets")],
    },
    "themes": {
        "capture": "v3-theme-comparison",
        "alt_en": "Native comparison of ImKit theme presets in the Gallery.",
        "alt_ja": "Gallery内でImKitのテーマを比較する画面。",
        "caption_en": "Themes — light and dark palettes",
        "caption_ja": "Themes — ライト・ダークの配色",
        "use_en": "Start with a named preset, then adjust semantic colors and metrics for the host application. Theme values are copied and remain under application ownership.",
        "use_ja": "テーマpresetを選び、必要に応じて色や寸法を調整します。Themeは値のcopyで、所有権はアプリ側にあります。",
        "code": '''#include <imkit/theme.h>

imkit::Theme MakeForestTheme() {
    auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);
    imkit::SetAccent(theme, {0.20f, 0.62f, 0.46f, 1.0f});
    return theme;
}''',
        "kind_en": "Complete function example. Keep the returned Theme value with the application, then apply it to a live context before `NewFrame`.",
        "kind_ja": "完結した関数例です。返されたTheme値はアプリ側で保持し、有効なContext上で`NewFrame`より前に適用します。",
        "integration_en": "Call `ApplyTheme(theme, scale)` on the active ImGui context before starting the frame. The host loads fonts into its own atlas and controls their lifetime.",
        "integration_ja": "戻り値はアプリ側で保持し、有効なContext上で`NewFrame`より前に`ApplyTheme(theme, scale)`を呼びます。フォントはアプリ側のatlasへ読み込み、描画で参照する間有効に保ちます。atlasとContextの寿命もアプリ側が管理します。",
        "scope_en": "Theme applies styling values; it does not create contexts, load OS fonts or guarantee contrast for arbitrary custom edits.",
        "scope_ja": "Themeは外観の値を適用します。Context生成やOS font読み込みは行わず、任意の色変更でcontrastを保証しません。",
        "links": [("Design-system API map", "api/design-system"), ("Native API index", "api/native")],
        "links_ja": [("Design-system header一覧", "api/design-system"), ("Native API一覧", "api/native")],
    },
    "workflow-components": {
        "capture": "v3-workflow-progress",
        "alt_en": "Native ImKit workflow view showing progress and status feedback.",
        "alt_ja": "進捗と状態表示を含むWorkflow部品の画面。",
        "caption_en": "Workflow — progress and status feedback",
        "caption_ja": "Workflow — 進捗と状態の表示",
        "use_en": "Use workflow components to show progress, empty or unavailable states, notifications, navigation and responsive toolbars in a host-owned workflow.",
        "use_ja": "アプリ側で管理する処理に進捗、空・利用不可状態、通知、navigation、responsive toolbarを加える部品です。",
        "code": '''#include <imkit/workflow.h>

void DrawImportProgress() {
    const imkit::CircularProgressView progress{
        0.42f, "42%", "素材を読み込み中"
    };
    imkit::CircularProgress("asset-import", progress);
}''',
        "kind_en": "Complete function example using public header types. Call it during the application's ImGui frame and keep the borrowed text valid for the draw call.",
        "kind_ja": "公開headerの型を使う完結した関数例です。アプリのImGuiフレーム中に呼び、表示文字列は描画が終わるまで保持します。",
        "integration_en": "Link `imkit::imkit`. Progress values and cancellation, notification queues, textures and persistence remain application state.",
        "integration_ja": "`imkit::imkit`をlinkします。進捗値、取消、通知queue、texture、保存はアプリ側で管理します。",
        "scope_en": "The library draws the requested state; it does not run workers, decide completion or persist notifications.",
        "scope_ja": "状態を描画する部品です。worker実行、完了判定、通知の保存は行いません。",
        "links": [("Workflow guide", "features/workflow"), ("Application shell guide", "features/shell")],
        "links_ja": [("Workflowガイド", "features/workflow"), ("Application shellガイド", "features/shell")],
    },
    "editor-suite": {
        "capture": "v3-timeline",
        "alt_en": "Native Gallery capture of the ImKit timeline editing surface.",
        "alt_ja": "ImKitのTimeline編集画面。",
        "caption_en": "Editor Suite — timeline and editing surfaces",
        "caption_ja": "Editor Suite — Timelineと編集画面",
        "use_en": "Choose Editor Core, Video or CG surfaces when a production tool needs editing controls over data and resources already owned by its application.",
        "use_ja": "アプリが持つデータとresourceに、編集用のCanvas、Timeline、Video、CG部品を加えるときに使います。",
        "code": '''#include <imkit/video.h>
#include <imkit/editor_core.h>

void DrawTimeline(const imkit::video::TimelineProvider& provider,
                  imkit::video::TimelineState& state,
                  imkit::editor::Selection& selection,
                  imkit::editor::EventBuffer& events,
                  const imkit::Theme& theme) {
    imkit::video::Timeline("timeline", provider, state, selection, events, theme);
}''',
        "kind_en": "Host-state function excerpt. The application supplies provider, selection, event storage and Theme, then validates and applies events.",
        "kind_ja": "アプリの状態を引数に取る関数例です。provider、選択、event buffer、Themeはアプリ側で用意し、eventは検証してから適用します。",
        "integration_en": "Link `imkit::editor_suite` or the narrower `imkit::video` target and keep every provider span valid during the call.",
        "integration_ja": "`imkit::editor_suite`または`imkit::video` targetをlinkし、providerの参照先は呼び出し中に有効に保ちます。",
        "scope_en": "The editing surfaces do not decode or play media, render arbitrary 3D scenes, own undo or load project files.",
        "scope_ja": "編集画面はmediaのdecode・再生、任意の3D描画、Undo、project file読み込みを担当しません。",
        "links": [("Editor API contracts", "api/editor-suite"), ("Timeline editing guide", "features/timeline")],
        "links_ja": [("Editor API契約", "api/editor-suite"), ("Timeline編集ガイド", "features/timeline")],
    },
    "timeline-editing": {
        "capture": "v3-timeline",
        "alt_en": "Native ImKit timeline capture showing clips, tracks and editing controls.",
        "alt_ja": "clip、track、編集操作を表示するTimeline画面。",
        "caption_en": "Timeline — clip, track and edit controls",
        "caption_ja": "Timeline — clip、track、編集操作",
        "use_en": "Use Video Timeline when the host needs to present tracks, clips, keyframes and editing gestures over its own media model.",
        "use_ja": "アプリ側のmedia modelに、track、clip、keyframe、編集操作を表示する場合に使います。",
        "code": '''#include <imkit/video.h>
#include <imkit/editor_core.h>

void DrawTimeline(const imkit::video::TimelineProvider& provider,
                  imkit::video::TimelineState& state,
                  imkit::editor::Selection& selection,
                  imkit::editor::EventBuffer& events,
                  const imkit::Theme& theme) {
    imkit::video::Timeline("timeline", provider, state, selection, events, theme);
}''',
        "kind_en": "Host-state function excerpt. The host owns provider data, the event buffer and all edit application.",
        "kind_ja": "アプリの状態を引数に取る関数例です。provider dataとevent buffer、編集反映はアプリ側が所有します。",
        "integration_en": "Handle complete Begin/Update/Commit/Cancel event batches and validate revision and collisions before changing the model.",
        "integration_ja": "Begin/Update/Commit/Cancel eventをまとめて処理し、model変更前にrevisionとcollisionを検証します。",
        "scope_en": "Timeline is an editing UI, not a playback engine, media decoder or project persistence system.",
        "scope_ja": "Timelineは編集UIです。再生engine、media decoder、project保存機能ではありません。",
        "links": [("Timeline interaction contract", "features/timeline"), ("Editor API reference", "api/editor-suite")],
        "links_ja": [("Timeline操作契約", "features/timeline"), ("Editor APIリファレンス", "api/editor-suite")],
    },
    "gallery-window-frame": {
        "capture": "v3-overview",
        "alt_en": "Native Gallery overview with its application shell and content regions.",
        "alt_ja": "アプリケーションshellとcontent領域を含むGallery画面。",
        "caption_en": "Gallery overview — application shell and content",
        "caption_ja": "Galleryの全体画面 — application shellとcontent",
        "use_en": "WindowFrame is for drawing a themed title area and returning typed window operations while the application keeps its native window and event loop.",
        "use_ja": "WindowFrameはテーマ付きのtitle areaを描画し、型付きwindow操作を返します。native windowとevent loopはアプリ側が管理します。",
        "code": '''#include <imkit/window_frame.h>

imkit::WindowFrameResult DrawTitleArea(const imkit::Theme& theme,
                                       const imkit::WindowFrameContent& content,
                                       float widthPixels) {
    const auto style = imkit::MakeWindowFrameStyle(
        imkit::WindowFramePreset::Workspace, theme);
    const auto layout = imkit::LayoutWindowFrame(widthPixels, style);
    return imkit::DrawWindowFrame(style, content, layout);
}''',
        "kind_en": "Complete function example. The host supplies the current Theme, borrowed content and window width, then applies the returned operation.",
        "kind_ja": "完結した関数例です。現在のTheme、借用content、window幅をアプリ側で渡し、戻り値の操作を適用します。",
        "integration_en": "The optional Win32 and macOS targets adapt native title-bar interaction; core drawing has no native-window ownership.",
        "integration_ja": "任意のWin32/macOS targetでnative title barの操作を連携します。基本描画APIはnative windowを所有しません。",
        "scope_en": "Icon atlases and GPU textures are also application resources; the library does not create platform windows or upload textures.",
        "scope_ja": "Icon atlasとGPU textureもアプリ側のresourceです。window生成やtexture uploadは行いません。",
        "links": [("WindowFrame API map", "api/window-frame"), ("Icons guide", "features/icons")],
        "links_ja": [("WindowFrame API map", "api/window-frame"), ("Iconガイド", "features/icons")],
    },
    "icons": {
        "capture": "gallery-icons",
        "alt_en": "Native Gallery capture displaying ImKit's icon atlas and icon buttons.",
        "alt_ja": "ImKitのicon atlasとicon buttonを表示するGallery画面。",
        "caption_en": "Icons — atlas and labeled actions",
        "caption_ja": "Icons — atlasとlabel付き操作",
        "use_en": "Use the bundled icon catalog for common tool actions while keeping atlas upload and GPU resources with the renderer.",
        "use_ja": "編集toolの操作に同梱icon catalogを使い、atlas uploadとGPU resourceはrenderer側で管理します。",
        "code": '''#include <imkit/icons.h>

bool DrawSaveAction(imkit::IconAtlas& icons) {
    return imkit::IconButton("save", icons, imkit::IconId::Save, "Save document");
}''',
        "kind_en": "Complete function example. The host binds atlas textures before drawing and handles the returned action.",
        "kind_ja": "完結した関数例です。描画前にatlas textureを登録し、戻り値の操作はアプリ側で処理します。",
        "integration_en": "Upload one of the seven supported atlas sizes through the host renderer, bind it to `IconAtlas` and release GPU resources after drawing completes.",
        "integration_ja": "7種類からatlas sizeを選んでアプリのrendererでuploadし、`IconAtlas`へ登録します。描画完了後にGPU resourceを解放します。",
        "scope_en": "Icons do not add a renderer, image decoder, file lookup or operating-system accessibility bridge.",
        "scope_ja": "Iconsはrenderer、image decoder、file検索、OS accessibility bridgeを追加しません。",
        "links": [("WindowFrame guide", "features/window-frame"), ("Public icon header", "https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/icons.h")],
        "links_ja": [("WindowFrameガイド", "features/window-frame"), ("公開icons header", "https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/icons.h")],
    },
}

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


def feature_intro(slug: str, route: str, locale: str) -> str:
    lead = FEATURE_LEADS.get(slug)
    if not lead:
        return ""
    is_en = locale == "en"
    capture = lead["capture"]
    alt = lead["alt_en" if is_en else "alt_ja"]
    caption = lead["caption_en" if is_en else "caption_ja"]
    use = lead["use_en" if is_en else "use_ja"]
    example_type = lead["kind_en" if is_en else "kind_ja"]
    integration = lead["integration_en" if is_en else "integration_ja"]
    scope = lead["scope_en" if is_en else "scope_ja"]
    link_key = "links" if is_en else "links_ja"
    link_lines = []
    for label, target in lead[link_key]:
        href = target if target.startswith("https://") else local_route(route, locale, target, locale, "")
        link_lines.append(f"- [{label}]({href})")

    gif = f"https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/{capture}.gif"
    capture_link = "Open the native Gallery animation" if is_en else "Galleryの操作映像を開く"
    use_heading = "## Use this when" if is_en else "## 用途"
    capture_heading = "## Gallery capture" if is_en else "## Galleryの画面"
    example_heading = "## Minimum drawing example" if is_en else "## 最小描画例"
    integration_heading = "## Integrate with the application" if is_en else "## アプリへ組み込む"
    scope_heading = "## Scope" if is_en else "## 範囲"
    links_heading = "## Related API and guides" if is_en else "## 関連APIとガイド"
    asset_prefix = "../../../../assets" if is_en else "../../../assets"

    return "\n\n".join([
        f"{use_heading}\n\n{use}",
        f"{capture_heading}\n\n![{alt}]({asset_prefix}/captures/{capture}-poster.png)\n\n{caption} · [{capture_link}]({gif})",
        f"{example_heading}\n\n```cpp\n{lead['code']}\n```\n\n**Example type:** {example_type}" if is_en else f"{example_heading}\n\n```cpp\n{lead['code']}\n```\n\n**例の種別:** {example_type}",
        f"{integration_heading}\n\n{integration}",
        f"{scope_heading}\n\n{scope}",
        f"{links_heading}\n\n" + "\n".join(link_lines),
        "---",
    ])


def rewrite_destination(destination: str, source_locale: str, current_route: str, source: Path) -> str:
    if destination.startswith(("https://", "http://", "mailto:", "tel:", "data:", "#", "/")):
        return destination

    path, marker, fragment = destination.partition("#")
    anchor = f"#{fragment}" if marker else ""
    normalized = posixpath.normpath(posixpath.join(source.parent.relative_to(ROOT).as_posix(), unquote(path)))

    if normalized.startswith("docs/images/"):
        image_name = normalized.removeprefix("docs/images/")
        source = ROOT / normalized
        if source.is_file():
            PUBLIC_IMAGES.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, PUBLIC_IMAGES / image_name)
            return f"{BASE}docs-images/{image_name}{anchor}"

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


def write_page(slug: str, route: str, locale: str, title: str) -> None:
    source = DOCS / SOURCES[slug][0 if locale=="en" else 1]
    if not source.is_file():
        raise FileNotFoundError(source)
    output_root = CONTENT / ("en" if locale == "en" else "")
    output = output_root / f"{route}.md"
    output.parent.mkdir(parents=True, exist_ok=True)
    body = prepare_body(source, locale, route)
    intro = feature_intro(slug, route, locale)
    if intro:
        body = f"{intro}\n\n{body}"
    output.write_text(f"---\ntitle: {json.dumps(title, ensure_ascii=False)}\n---\n\n{body}\n", encoding="utf-8")
    print(f"{source.relative_to(ROOT)} -> {output.relative_to(ROOT)}")


def main() -> None:
    for slug, (route, en_title, ja_title) in PAGES.items():
        write_page(slug, route, "en", en_title)
        write_page(slug, route, "ja", ja_title)


if __name__ == "__main__":
    main()
