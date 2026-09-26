# Changelog / 変更履歴

> Each release lists its changes in English (Added / Changed / Fixed / Documentation / Breaking changes), then a Japanese summary. Releases 2.2.0 and earlier keep their original form.
> 各リリースは英語（Added / Changed / Fixed / Documentation / Breaking changes）で記述し、その後に日本語サマリーを置きます。2.2.0 以前のリリースは元の形式のままです。

## Unreleased

**Added**
- `Slate` — the 13th named theme preset. Stable ID and display order are preserved, so existing consumers are unaffected.
- `imkit::DragVector3WithUnit` — per-axis drag with a shared unit label. Each axis keeps native `DragFloat` behavior and its own stable ID scope; the three fields stack vertically when the window is narrow.
- Workflow surface gains a generic work tab, a hierarchy listing, and a settings card.

**Changed**
- Hierarchy headings and rows now expose auxiliary operations as selectable requests; the host decides which are available.
- Contrast and visibility of the general dark theme and its controls were improved.

**Breaking changes** — none. No public API or ABI changes.

**日本語**
- **追加**: 13番目の名前付き theme preset `Slate`。安定IDと表示順は維持され、既存 consumer に影響ありません。
- **追加**: `imkit::DragVector3WithUnit`（単位付き三軸 drag）。各軸は標準 `DragFloat` の挙動と安定 ID 範囲を保持し、狭い幅では3欄を縦並びにします。
- **追加**: workflow surface に汎用作業タブ、階層一覧、設定カードを追加。
- **変更**: 階層の見出し・行が選択可能な補助操作要求を返すようになりました（利用可能か決めるのはホスト）。
- **変更**: 汎用ダークテーマと関連コントロールのコントラスト・視認性を向上。
- **破壊的変更**: なし（公開 API・ABI は不変）。

## 3.1.0 — 2026-09-26

**Documentation and onboarding**
- Bilingual documentation catalog: every public guide now maps to its audience, API contract, Gallery page, source file, and SDK package path.
- Module recipes for components, themes, workflow, Node Editor, Editor Suite, timeline editing, and WindowFrame — including where each call sits in the frame and the host-ownership boundary.
- Gallery Start screen: clearer task routes and a direct link to the recipe map; English and Japanese documentation and examples expanded.
- Automated checks for language pairs, headings, procedures, links, and Gallery routes; all 48 public documentation files are verified in release packages.
- **No public API or ABI changes.**

**日本語**
- **文書・導入**: 英日文書カタログを追加し、各ガイドの読者・API契約・Gallery画面・source・SDK package path を対応付けました。
- **文書・導入**: コンポーネント、theme、workflow、Node Editor、Editor Suite、timeline編集、WindowFrame の recipe を追加（frame 内の呼出し位置とホスト所有の境界を含む）。
- **文書・導入**: Gallery Start 画面に目的別の案内と recipe 一覧への導線を追加し、英日文書と実例を拡充しました。
- **検証**: 言語 pair・見出し・手順・link・Gallery route の自動検査を追加し、release package 内の公開文書 48 件を確認します。
- **公開 API・ABI に変更はありません。**

## 3.0.0 — 2026-09-13

**Added**
- Independent `imkit::node_editor`: host-controlled snapshot edits, revision validation, and atomic requests. Pan/zoom, selection, link create/reconnect/delete, compatibility feedback, grouping/subgraphs, bookmarks, diagnostics, search, palette, minimap, and detached or inline previews.
- Deterministic align/distribute/arrange/snap helpers that need no Dear ImGui context.
- Dynamic socket edits (create/delete/rename/reorder/type/multiplicity/value/exposure/limit). Destructive edits to a connected graph are requests the host may reject; the library never silently removes application links.
- Standard `PinRow`, inline color/float/vector editing, variable input groups, node appearance hooks, density/contrast/narrow/Japanese states, and the native Material Graph companion.

**Platform, rendering and accessibility**
- The supported ABI is pinned to Dear ImGui 1.93.0 WIP docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`.
- Build/test/package gates for Windows x64/Arm64 and macOS arm64/x86_64, plus a macOS Universal 2 package.
- GLFW/Metal native Gallery host, explicitly constructed `imkit::preview_metal`, an NSAccessibility bridge, and the macOS WindowFrame adapter.
- OpenGL-specific declarations moved to `<imkit/preview_opengl3.h>`; `preview.h` now carries renderer-neutral contracts.
- Platform-neutral `NativeActionSink` (with UTF-8 value actions) replaces the Win32-only action sink.

**Workflow, shell and editors**
- Circular determinate/complete/unavailable progress states with size and stroke options.
- Responsive, resizable right Inspector panel; open state and width stay host-owned.
- Optional Video Timeline external-drop preview callbacks (exact candidate time range) and a host toolbar layout contract above the timeline.
- Host ownership of documents, commands, Undo/Redo, scene/media state, persistence, workers, and preview resources is preserved across Editor Core, Video, and CG.

**Gallery, documentation and distribution**
- Dedicated Node Editor Gallery, direct launcher/focus behavior, native Metal Gallery, and deterministic 960×540 capture modes.
- The README GIF grid is replaced by larger V3 Overview, Node Editor, Workflow/Circular Progress, Timeline, and Theme/Comparison animations, captured only from native backbuffers.
- New release asset `imkit-v3.0.0-showcase.mp4` from the same frame sequences.
- CPack packages now include complete English/Japanese documentation, the Gallery, the Node Editor Gallery, notices, and platform libraries.

**Removed**
- Dear ImGui 1.88 compatibility targets and legacy WindowFrame ABI paths are no longer built or installed.

**Breaking changes**
- Consumers must use the exact pinned Dear ImGui ABI and rebuild all ImKit-dependent translation units.
- OpenGL preview consumers must include `<imkit/preview_opengl3.h>`.
- Native accessibility handlers migrate from `Win32ActionSink` to `NativeActionSink`.
- See the [v3 migration](docs/migration-v3.md) for the exact upgrade steps.

**日本語**
- **追加**: 独立 `imkit::node_editor`（型付きID、revision検証、atomic request、pan/zoom、選択、link編集、group/subgraph、検索、palette、minimap、preview）。
- **追加**: Dear ImGui context 不要の determinstic align/distribute/arrange/snap、動的 socket 編集（作成・削除・rename・並べ替え・型・多重度・値・公開・上限）、標準 `PinRow`、Material Graph companion。
- **プラットフォーム・レンダリング・accessibility**: Dear ImGui 1.93.0 WIP docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` に ABI を固定。Windows x64/Arm64・macOS arm64/x86_64・Universal 2、Metal Gallery/preview、NSAccessibility、macOS WindowFrame を追加。OpenGL 宣言を `<imkit/preview_opengl3.h>` に分離、共通 `NativeActionSink` へ移行。
- **workflow・shell・editor**: 円形進捗、responsive な右 Inspector panel、Timeline 外部 drop の正確な候補範囲 preview、host toolbar 配置契約。
- **gallery・文書・配布**: Node Editor Gallery、Metal Gallery、960×540 capture、大判 GIF 5本＋showcase MP4、全対応環境の CPack 配布。
- **削除**: Dear ImGui 1.88 互換 target と旧 WindowFrame ABI path を非ビルド・非インストールに。
- **破壊的変更**: 正確にピン定された Dear ImGui ABI と、全 ImKit 依存 TU の再ビルドが必要。OpenGL preview は `<imkit/preview_opengl3.h>` が必要。accessibility は `Win32ActionSink` → `NativeActionSink`。詳細は[移行](docs/migration-v3.md)。

## 2.2.0 — 2026-09-12

- Integrate the guided Gallery comparison, responsive workflow controls, Timeline interaction and Preview placement/Monitor contracts into one release line.
- Add Fit/Fill/Stretch preview placement, cropped and flipped Monitor presentation, five preview states and Gallery coverage for representative source and viewport aspects.
- Refresh the English/Japanese release entry points and native Gallery GIFs; the overview now introduces the route before showing the live comparison.
- 案内付きGallery比較、responsive workflow操作、Timeline操作、Preview配置／Monitor契約を1つのリリース系列へ統合。
- Fit／Fill／Stretch配置、crop／flip対応Monitor、5つのpreview状態、代表的な素材／viewport aspectのGallery確認を追加。
- 英日READMEとnative Gallery GIFを更新し、概要GIFは最初に利用導線を示してからライブ比較へ進む構成に変更。

## 2.1.0 — 2026-09-12

- Add a guided Gallery Start screen and a movable, resizable live comparison between direct Default Dear ImGui and ImKit controls. The two columns share host-owned state and the comparison preserves the surrounding ImGui style after each frame.
- Add reproducible native GIF routes for the overview, comparison, theme palettes, workflow feedback and timeline editing. The encoder verifies frame count, dimensions and size limits.
- Publish a Windows x64 Gallery archive with its required design assets, runtime dependency inventory, license and third-party notices.
- Refresh English and Japanese README/Gallery guidance around trying the native demo first, host ownership, compatibility limits and asset provenance.
- GalleryのStart画面と、直接Dear ImGui／ImKitを並べる移動・resize可能な比較windowを追加。両列はホスト所有状態を共有し、比較後も周囲のImGui styleを復元します。
- 概要、比較、Theme palette、workflow feedback、timeline操作のnative GIFを再生成可能にしました。encoderはframe数、寸法、容量上限を検証します。
- 必要なdesign asset、runtime dependency一覧、license、第三者noticeを含むWindows x64 Gallery archiveを公開します。
- native demoを最初に試す導線、ホスト所有権、互換範囲、asset出典を中心に英日README／Gallery文書を更新しました。

## 2.0.0

- Add host-owned application shell components: AppBar, WorkspaceHeader, InspectorSection, BottomActionBar, AdvancedSection, DiagnosticsDrawer and ThemePicker.
- Ship versioned Inter and Noto Sans JP assets with OFL notices, SHA-256 inventory and a CMake copy helper; font atlas and GPU lifetime remain host-owned.
- Extend source/SDK packaging, installed-consumer verification and Windows CI for the complete Editor Suite and shell surface.
- Add `ThemePresetFromId()` for host-owned stable preset persistence and document pinned-submodule/local-source development.
- 安定preset IDの復元APIと、固定submodule／ローカルソースを切り替える並行開発手順を追加。
- Open the native Gallery on the Components page instead of an empty initial canvas.
- native Galleryの初期表示を空画面ではなくComponents pageへ修正。
- Add twelve discoverable named themes with stable IDs, complete editor palettes and checked contrast thresholds.
- Redesign the native Gallery around guided onboarding, searchable navigation, live examples and copyable code.
- Refresh the English/Japanese onboarding documentation and add a reproducible native README animation.
- Translate arrows, shared gizmo preview and immutable drag projection.
- Source-time stereo waveform providers and pixel envelopes.
- Editor workspace layout, timeline overview, source placement and host Undo/Redo.
- Inspector property curves, batch reservation and consumer migration documentation.
- ホスト所有のアプリshell部品、Inter／Noto Sans JP font資産、CMake配置helper、完全なSDK／install後consumer検証を追加。
- 12種類の名前付きTheme、製品型Gallery、英日導入文書、実backbufferから再生成できるREADME GIFを追加。
- 移動矢印・Gizmo追従・素材時間波形・編集画面・ホスト履歴を更新。利用側の再ビルドが必要。


## 1.0.0 — 2026-09-10

- Complete Editor Core, Video, CG and optional OpenGL3 preview with host-owned data and typed edits.
- Connect related timeline edits, transitions/captions/audio/color curves, hierarchy/Inspector, gizmos, animation and UV workflows in the native Gallery.
- Preserve 120 IconIds and add 86 originals: 206 icons, 1236 PNG variants, six atlases.
- Validate Debug/Release, public IO, native GPU and 100k interaction performance; document source/SDK consumption and GL lifetime.
- Core・Video・CG・任意OpenGL3 previewとホスト所有の型付き編集を完成。
- Timeline関連編集・transition/caption/audio/color curve、階層/Inspector・gizmo・animation・UVをnative Galleryへ接続。
- 既存120 IconIdを保持して86原画を追加。合計206アイコン・1236 PNG・6 atlas。
- Debug/Release・公開IO・実GPU・10万件操作性能を検証し、導入・所有権・GL寿命を文書化。

## 0.2.0 — Precision Layers

- Adopt Precision Layers across public GUI families, with light/dark semantic palettes and explicit scale/font scopes.
- Expose 365 current public overloads with signature/link coverage; preserve six original wrappers.
- Add selected row/tree/tab marks and composable actions, switches, mixed selection, search, units, settings, badges, notifications and toolbars.
- Add a six-category live catalog, Japanese fallback, public-IO verification and real GPU captures.
- Provide CMake install/export, source and Windows x64 Debug/Release SDK distribution and bilingual documentation.

- 公開GUI部品群にPrecision Layersを適用。light/dark、意味別配色、倍率とfont scopeを提供。
- 365の公開overloadを対応表とcompile/link fixtureで追跡。既存6関数を維持。
- 選択・tree・tabのマークと各種合成部品を追加。
- 実APIによる6カテゴリのカタログ、日本語フォールバック、公開IO検証、実GPU画像を追加。
- CMake install/export、ソース・Windows x64 Debug/Release SDK、日英文書を整備。

## 0.1.0

Initial six-wrapper development foundation / 6関数の開発基盤。
