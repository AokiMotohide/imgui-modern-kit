# Changelog / 変更履歴

## Unreleased

No changes yet. / 変更はまだありません。

## 3.0.0 — 2026-09-13

### Node Editor

- Add independent `imkit::node_editor` snapshots and bounded edit-request buffers with typed IDs, host-controlled revision validation and atomic operations.
- Add pan/zoom, selection, link create/reconnect/delete, connection compatibility feedback, grouping/subgraphs, bookmarks, diagnostics, search, palette, minimap and detached or inline previews.
- Add deterministic align/distribute/arrange/snap helpers that do not require a Dear ImGui context.
- Add dynamic socket create/delete/rename/reorder/type/multiplicity/value/exposure/limit edits. Connected destructive changes are requests the host may reject; the library never silently removes application links.
- Add standard `PinRow`, inline color/float/vector editing, variable input groups, node appearance hooks, density/contrast/narrow/Japanese states and the native Material Graph companion.
- 独立`imkit::node_editor`、型付きID、revision検証、atomic request、pan／zoom、選択、link編集、互換feedback、group／subgraph、検索、palette、minimap、previewを追加。
- 動的socketの作成・削除・rename・並べ替え・型・多重度・値・公開・上限編集、標準`PinRow`とMaterial Graph companionを追加。接続を壊す操作はホストの明示判断を必要とします。

### Platform, rendering and accessibility

- Pin the supported ABI to Dear ImGui 1.93.0 WIP docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`.
- Add Windows x64/Arm64 and macOS arm64/x86_64 build/test/package gates plus a macOS Universal 2 package.
- Add the GLFW/Metal native Gallery host, explicitly constructed `imkit::preview_metal`, an NSAccessibility bridge and the macOS WindowFrame adapter.
- Add Windows Arm64 packaging and retain the Win32 accessibility and WindowFrame adapters.
- Split OpenGL-specific declarations into `<imkit/preview_opengl3.h>`; `preview.h` now contains renderer-neutral contracts.
- Replace the Win32-only action sink with platform-neutral `NativeActionSink`, including UTF-8 value actions.
- Remove Dear ImGui 1.88 compatibility targets and legacy WindowFrame ABI paths.
- Windows x64／Arm64、macOS arm64／x86_64／Universal 2、Metal Gallery／preview、NSAccessibility、macOS WindowFrameを追加。OpenGL headerを分離し、共通`NativeActionSink`へ移行、1.88互換targetを削除しました。

### Workflow, shell and editors

- Add circular determinate, complete and unavailable progress states with size/stroke options.
- Add a responsive, resizable right Inspector panel whose open state and width stay host-owned.
- Add optional Video Timeline external-drop preview callbacks for the exact candidate time range and a host toolbar layout contract above the timeline.
- Preserve host-owned documents, commands, Undo/Redo, scene/media state, persistence, workers and preview resources across Editor Core, Video and CG surfaces.
- 円形進捗、responsiveな右Inspector panel、Timeline外部dropの正確な候補範囲preview、host toolbar配置契約を追加しました。

### Gallery, documentation and distribution

- Add the dedicated Node Editor Gallery, direct launcher/focus behavior, native Metal Gallery and deterministic 960×540 documentation capture modes.
- Replace the small README GIF grid with large V3 Overview, Node Editor, Workflow/Circular Progress, Timeline and Theme/Comparison animations captured only from native backbuffers.
- Add the native `imkit-v3.0.0-showcase.mp4` release asset from the same frame sequences.
- Synchronize English/Japanese README, Getting Started, Gallery, Guide and Node Editor documentation with Migration, Architecture, Dependencies and Validation.
- Install the complete English/Japanese documentation, Gallery, Node Editor Gallery, notices and platform libraries in CPack packages.
- native backbuffer由来の大判GIF 5本とshowcase MP4、Node Editor Gallery、英日導入文書、全対応環境のCPack配布を追加しました。

### Breaking changes

- Consumers must use the exact pinned Dear ImGui ABI and rebuild all ImKit-dependent translation units.
- OpenGL preview consumers must include `<imkit/preview_opengl3.h>`.
- Native accessibility action handlers migrate from `Win32ActionSink` to `NativeActionSink`.
- Dear ImGui 1.88 legacy targets are no longer built or installed.
- 正確な移行手順は[docs/migration-v3.md](docs/migration-v3.md)を参照してください。

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
