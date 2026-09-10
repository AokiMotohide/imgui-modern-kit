# Changelog / 変更履歴

## 2.0.0

- Add host-owned application shell components: AppBar, WorkspaceHeader, InspectorSection, BottomActionBar, AdvancedSection, DiagnosticsDrawer and ThemePicker.
- Ship versioned Inter and Noto Sans JP assets with OFL notices, SHA-256 inventory and a CMake copy helper; font atlas and GPU lifetime remain host-owned.
- Extend source/SDK packaging, installed-consumer verification and Windows CI for the complete Editor Suite and shell surface.
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
