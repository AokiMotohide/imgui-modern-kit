# Changelog / 変更履歴

> Published release entries present English first, followed by equivalent Japanese notes. Unreleased remains a work-in-progress list.

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

### Added
- A bilingual documentation catalog maps each public guide to its audience, API contract, Gallery page, source file, and SDK package path.
- Module recipes cover Components, Themes, Workflow, Node Editor, Editor Suite, Timeline, and WindowFrame, including frame placement and host-owned responsibilities.
- Gallery Start adds task-focused routes and a direct path to the recipe map.

### Compatibility
- No public API or ABI changes. The Dear ImGui ABI baseline remains the one introduced in v3.0.0.

### 日本語
#### 追加
- 英日文書カタログを追加し、各ガイドの対象読者、API契約、Gallery画面、実装source、SDK内の配置先を対応付けました。
- Components、Theme、Workflow、Node Editor、Editor Suite、Timeline、WindowFrameの実装例を追加しました。フレーム内で呼ぶ位置と、ホスト側に残る責務も確認できます。
- GalleryのStart画面に目的別の案内を設け、実装例一覧へ直接進めるようにしました。

#### 互換性
- 公開API・ABIに変更はありません。Dear ImGuiのABI基準はv3.0.0から変わっていません。

## 3.0.0 — 2026-09-13

### Added
- A standalone imkit::node_editor target adds a host-controlled graph canvas, typed connections, dynamic sockets, inline values, minimap, search, previews, and revision-checked edit requests.
- Context-free helpers arrange, align, distribute, and snap nodes. The host retains the graph model, validation, evaluation, undo, and persistence.
- Workflow and Editor Suite additions include circular progress, a resizable Inspector, Timeline drop previews, and a host toolbar layout contract.
- Native Metal Gallery and preview support, macOS WindowFrame and accessibility bridges, and Windows/macOS platform packages.

### Fixed
- Invalid Timeline drop candidates are rejected before commit. Node Editor collapse and minimap interaction issues were corrected, and macOS WindowFrame coordinate and bundle-resource handling was fixed.

### Breaking changes and migration
- The supported ABI is pinned to Dear ImGui 1.93.0 WIP docking commit 367b2c24f399988ddafc0bb4628da0106bcc09be. Rebuild all ImKit-dependent translation units against that revision.
- OpenGL preview declarations moved to the separate preview_opengl3.h header. Win32ActionSink was replaced by NativeActionSink.
- Dear ImGui 1.88 compatibility targets and legacy WindowFrame ABI paths were removed. Follow the v3 migration guide before upgrading.

### 日本語
#### 追加
- 独立したimkit::node_editor targetを追加しました。ホストが管理するgraph canvas上で、型付き接続、動的socket、値の直接編集、minimap、検索、preview、revisionを検証する編集要求を扱えます。
- Dear ImGui Contextに依存しない、nodeの配置・整列・分配・snap機能を追加しました。graph model、検証、評価、Undo、保存はホスト側で管理します。
- WorkflowとEditor Suiteに、円形進捗、幅を変更できるInspector、Timelineのdrop候補preview、ホストtoolbarの配置契約を追加しました。
- Metal対応Gallery／preview、macOS WindowFrame／accessibility bridge、Windows／macOSの配布packageを追加しました。

#### 修正
- Timelineで無効なdrop候補を確定できないようにしました。Node Editorの折りたたみ時の異常終了とminimap操作を修正し、macOS WindowFrameの座標変換とbundle内の資産配置も修正しました。

#### 破壊的変更と移行
- 対応ABIをDear ImGui 1.93.0 WIP docking commit 367b2c24f399988ddafc0bb4628da0106bcc09beに固定しました。全ImKit依存translation unitをこのrevisionに合わせて再buildしてください。
- OpenGL previewの宣言をpreview_opengl3.hへ分離しました。Win32ActionSinkはNativeActionSinkへ置き換わります。
- Dear ImGui 1.88互換targetと旧WindowFrame ABI pathを削除しました。更新前にv3移行ガイドを確認してください。

## 2.2.0 — 2026-09-12

### Added
- Preview placement now supports Fit, Fill, and Stretch. Monitor presentation also supports crop and vertical flip.
- Five explicit preview states—Ready, Loading, Empty, Offline, and Error—make unavailable or incomplete media visible to the user.
- Gallery examples cover representative source and viewport aspect ratios alongside the live comparison, Workflow, and Timeline surfaces.

### 日本語
#### 追加
- Previewの配置方法にFit、Fill、Stretchを追加しました。Monitorではcropと上下反転も指定できます。
- Ready、Loading、Empty、Offline、Errorの5状態を追加し、素材の準備中や利用できない状態を画面上で示せるようにしました。
- 代表的なsource／viewportのアスペクト比を使ったGallery実例を追加し、ライブ比較、Workflow、Timelineの画面と合わせて確認できます。

## 2.1.0 — 2026-09-12

### Added
- Gallery Start guides new users to a movable, resizable comparison between Default Dear ImGui and ImKit. Both sides update the same host-owned values.
- Reproducible native capture routes cover the overview, comparison, theme palettes, Workflow feedback, and Timeline editing.
- A Windows x64 Gallery archive includes the required design assets, runtime dependency inventory, license, and third-party notices.

### 日本語
#### 追加
- GalleryのStart画面から、Default Dear ImGuiとImKitを並べた比較を開けます。windowは移動・resizeでき、両側から同じホスト所有値を更新します。
- 概要、比較、Theme palette、Workflowのfeedback、Timeline編集を、再生成できるnative capture routeとして追加しました。
- 必要なdesign asset、runtime dependency一覧、license、第三者noticeを含むWindows x64 Gallery archiveを用意しました。

## 2.0.0

### Added
- Host-owned application shell components: AppBar, WorkspaceHeader, InspectorSection, BottomActionBar, AdvancedSection, DiagnosticsDrawer, and ThemePicker.
- The Editor Suite expands with timeline overview and source placement, host-owned undo/redo, waveform and pixel-envelope providers, property curves, and batch reservations.
- Twelve named themes have stable IDs; ThemePresetFromId supports host-owned preset persistence. The native Gallery opens on Components and provides searchable examples.
- Versioned Inter and Noto Sans JP font assets include OFL notices, hashes, and a CMake copy helper. Font atlas and GPU lifetime remain host-owned.
- Source and Windows SDK packages add install/consumer verification for the expanded library.

### Fixed
- Multi-clip timeline edits validate the full selection before commit, preventing partial application when a clip or selection limit is reached.
- Clip and transition hit regions now follow Dear ImGui layout and scale. Editing a deselected keyframe is rejected, and Inspector reset behavior is corrected.

### 日本語
#### 追加
- AppBar、WorkspaceHeader、InspectorSection、BottomActionBar、AdvancedSection、DiagnosticsDrawer、ThemePickerなど、ホスト側で構成するアプリシェル部品を追加しました。
- Editor Suiteを拡張し、Timeline overview、素材配置、ホスト所有のUndo／Redo、waveformとpixel envelopeのprovider、property curve、batch reservationを追加しました。
- 安定ID付きの名前付きThemeを12種類用意しました。ThemePresetFromIdでホスト側に保存したpresetを復元できます。GalleryはComponents画面から開き、実例を検索できます。
- InterとNoto Sans JPのversion固定font資産に、OFL notice、hash、CMake配置helperを追加しました。font atlasとGPU資源の寿命はホスト側で管理します。
- 拡張したlibraryに合わせてsource／Windows SDK packageと、install後のconsumer確認を整えました。

#### 修正
- 複数clipのTimeline編集はcommit前に選択範囲全体を検証し、容量や選択queryの不足による部分適用を防ぐようにしました。
- clipとtransitionの入力領域をDear ImGuiのlayoutと倍率に合わせました。選択解除済みkeyframeの編集開始を拒否し、Inspectorのreset動作も修正しました。

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

### Added
- Precision Layers applies light/dark semantic palettes, scale, and font scopes across the public GUI families.
- The release tracks 365 public GUI and helper overloads with signature and link coverage while preserving the original six wrappers.
- Composite controls include selection marks, action variants, switches, mixed selection, segments, searchable selection, units, settings rows, badges, notifications, and toolbars.
- An optional six-category live catalog includes an editable palette, Japanese glyph fallback, public-IO verification, and native GPU captures.

### SDK scope
- C++20 static library targeting Dear ImGui 1.92.9b docking commit b48d1afbe8ee8b238e2961dc363a949dd7304e23.
- The binary SDK is Windows x64 / MSVC v145, with Debug and Release CRT variants. ImGui core is not embedded; use a matching host target.
- The source archive is the recommended route for other platforms, compilers, or ABI configurations. Changing only the version guard does not port the ABI.

### 日本語
#### 追加
- 公開GUI部品群にPrecision Layersを適用し、light／darkの意味別palette、倍率、font scopeを追加しました。
- 公開GUI／helperの365 overloadをsignatureとlinkの両面から追跡し、従来の6 wrapperを維持しました。
- 選択mark、action variant、switch、mixed selection、segment、検索可能な選択欄、単位表示、設定行、badge、notification、toolbarを追加しました。
- 任意で起動できる6カテゴリのlive catalogに、編集可能なpalette、日本語glyph fallback、公開IOによる操作確認、native GPU captureを用意しました。

#### SDKの対象
- Dear ImGui 1.92.9b docking commit b48d1afbe8ee8b238e2961dc363a949dd7304e23を対象とするC++20静的libraryです。
- binary SDKはWindows x64／MSVC v145向けで、Debug／ReleaseのCRT構成を収録します。ImGui coreは含まず、ホスト側の対応targetを使用します。
- ほかのOS、compiler、ABI構成ではsource archiveから導入してください。version guardだけを変更してもABI移植にはなりません。

## 0.1.0

Initial six-wrapper development foundation / 6関数の開発基盤。
