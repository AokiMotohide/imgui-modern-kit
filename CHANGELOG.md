# Changelog / 変更履歴

> Published release entries present English first, followed by equivalent Japanese notes. Unreleased remains a work-in-progress list.

## Unreleased / 未リリース

**Modern refresh.** New default tokens: radius 4/8/12 (subtle, control, overlay), control height 30 in Comfortable density, softer hairline borders, tonal buttons, a real outer overlay shadow and 0.14 s control motion. Segmented controls glide a raised thumb, chips and badges are pills, selected rows get a leading accent bar, scrollbars are slim pills, and sliders use pill grabs. Progress tracks, `ProgressBar`, circular progress, toasts, alerts, empty/error states, pagination, search and split buttons were redrawn. Node editor, histogram, meters, waveform and colour wheel received matching visuals. **Added:** `imkit/charts.h` with `DonutChart`, `BarChart`, `Gauge`, `Sparkline` and `ChartColor`. Public signatures are unchanged; `Theme::radius`, `Theme::elevation`, `Theme::opacity::disabled` and `Theme::motion` defaults changed, so hosts that relied on the old values should set them explicitly. The Gallery uses full-width pages with card layouts and gains a Charts page. A second pass compacted `imkit::Checkbox`, the new `imkit::RadioButton` wrappers and `imkit::Toggle`, restyled toolbars, tabs, step navigators, tile dividers and the video timeline (grouped tool segments, ruler band and playhead head, rounded clips with readable text on every tone, chevron track headers, dashed snap guides); timeline hit rectangles are unchanged apart from a narrower track-header chevron. A third pass added `imkit/ptz.h` (`PtzControl`, `PtzPadVector`) and `imkit/range_slider.h` (`RangeSlider`, `RangeSliderInt`), fill-style `SliderFloat`/`SliderInt`/`VSliderFloat` and two-to-four component sliders, chevron `BeginCombo`/`Combo` wrappers (they do not mark the item edited), a redrawn curve editor grid with axis labels and per-channel colours, quieter transport buttons, dope sheet and strip styling, histogram marks, placeholder media tiles, and a rebuilt Toasts page; the Gallery gains a PTZ Camera page. A fourth pass adds the optional `imkit::show` target (`imkit/show.h`) for media servers: performance controls (`Fader`, `Knob`, `XYPad`, `PadGrid`, `MeterBar`, `TimecodeDisplay`, `HoldButton`), live components (`CueList`, `GoBar`, `ClipLauncher`, `Mixer`, `EffectRack`, `ModulatorEditor`, `MappingTable`, `DeviceHealth`), a no-code `RuleBuilder` with pure `EvaluateRule`/`FormatRuleSentence`/`ValidateRule`, scheduling (`Calendar`, `ResourceTimeline`, `RecurrenceEditor`, `CronEditor`, pickers, `WeekPainter`, solar times and conflict detection), beat components (`BeatClock`, `Deck`, waveforms, hot cues, `DjChannelStrip`, `StepSequencer`, `Spectrum`) and a script paste panel with diagnostics, parameter panel and table import. Continuous controls report `ValueEdit`. `SemanticRole` gains `Slider` and `Meter` (`RangeSlider` now publishes `Slider` nodes instead of `Group`). `PtzState` keeps only `speed` public. The `const Theme*` overloads of `StatusBadge`, `NotificationCard` and `ValidationMessage`, and the older `EmptyState(message)`, `Progress(id, fraction, label)` and `ToastRegion(span<Notification>)`, are deprecated in favour of the `ComponentOptions` and newer spellings; define `IMKIT_DEPRECATED(message)` as nothing to silence the warnings. Gallery pages 25-30 demonstrate the new module. A fifth pass redesigns the live controls: `GoBar` becomes a console deck (ARM key, a hero GO key with a Space hint, pause, hold-to-confirm Stop all and Panic keys and a Next card with the cue number and time left), `CueList` gains a header row, row separators, indent guides and clearer running/standby rows, `ClipLauncher` shows picture thumbnails and can fit its columns at a fixed aspect, and `PadView` gains `image`/`uv0`/`uv1`. New in `imkit::show`: `ConsoleButton`, `ScribbleStrip` and `MasterBar`; `image.h` with `ImageButton`, `ImageCropEditor` and `FitImage` (any texture becomes a button; end users can drop, choose, clear and reposition pictures); `surface.h` with `ControlSurface`, `SurfacePalette` and geometry helpers (a grid canvas where operators place faders, knobs, keys, pictures and displays, with an edit mode for selecting, moving, resizing, adding, duplicating and deleting); `led.h` with `LedColorPicker`, `GradientEditor`, `PixelPreview`, `BudgetBar`, `ColorOrderPicker`, `TestPatternBar`, `PortStrip` and the pure `KelvinToRgb`, `RgbToRgbw`, `MixLightColor`, `SampleGradient` and `EstimateLedCurrent`; `address.h` with `AddressMap`, `ChannelMonitor`, `FindOverlaps`, `FirstFreeRange` and `PixelsPerUniverse`. 32 LED and live-surface icons bring the catalogue to 320 (21 categories). Public additions are at the end of existing structs; `HoldButtonOptions` gains `key` and `hint`, `GoBarView` gains `nextNumber` and `totalSeconds`, `CueListOptions` gains `showHeader`. Gallery pages 31-33 show the surface, LED parts and picture buttons.

**日本語.** 既定のトークンを更新しました。角丸は4/8/12（小・コントロール・オーバーレイ）、Comfortableの高さは30、枠線は控えめな細線、ボタンは色調塗り、オーバーレイには外側の影、コントロールの動きは0.14秒です。セグメントは浮き上がるサムが滑り、チップとバッジはピル形、選択行は先頭にアクセントバー、スクロールバーは細いピル、スライダーのつまみもピル形です。進捗トラック、`ProgressBar`、円形進捗、トースト、通知、空・エラー状態、ページ送り、検索欄、分割ボタンを描き直しました。ノードエディタ、ヒストグラム、メーター、波形、色相環も合わせています。`imkit/charts.h`に`DonutChart`・`BarChart`・`Gauge`・`Sparkline`・`ChartColor`を追加しました。公開シグネチャは変わりませんが、`Theme::radius`・`Theme::elevation`・`Theme::opacity::disabled`・`Theme::motion`の既定値が変わっています。従来値に依存するホストは明示してください。Galleryは全幅のカード配置になり、Chartsページを追加しました。第2弾として、`imkit::Checkbox`・新しい`imkit::RadioButton`・`imkit::Toggle`を小型化し、ツールバー、タブ、ステップ表示、タイルの区切り、映像タイムライン（工具のセグメント化、ルーラーの帯と再生ヘッドの頭、どの色でも文字が読める角丸クリップ、シェブロンのトラック見出し、点線のスナップガイド）を描き直しました。タイムラインの当たり判定は、トラック見出しのシェブロンを狭めた点を除いて変えていません。 第3弾として、`imkit/ptz.h`（`PtzControl`、`PtzPadVector`）と`imkit/range_slider.h`（`RangeSlider`、`RangeSliderInt`）を追加し、`SliderFloat`・`SliderInt`・`VSliderFloat`と2〜4成分スライダーを塗り表示にし、`BeginCombo`/`Combo`をシェブロン表示のラッパーにしました（項目を編集済みとして記録しません）。曲線エディタの格子・軸ラベル・チャンネル色、再生ボタン列、ドープシートとストリップ、ヒストグラムの目盛り、メディアの仮タイル、Toastsページも刷新し、GalleryにPTZ Cameraページを追加しました。第4弾として、メディアサーバー向けの任意ターゲット`imkit::show`（`imkit/show.h`）を追加しました。操作部品（`Fader`・`Knob`・`XYPad`・`PadGrid`・`MeterBar`・`TimecodeDisplay`・`HoldButton`）、ライブ部品（`CueList`・`GoBar`・`ClipLauncher`・`Mixer`・`EffectRack`・`ModulatorEditor`・`MappingTable`・`DeviceHealth`）、コードなしの`RuleBuilder`（純関数の`EvaluateRule`・`FormatRuleSentence`・`ValidateRule`付き）、スケジュール（`Calendar`・`ResourceTimeline`・`RecurrenceEditor`・`CronEditor`・各ピッカー・`WeekPainter`・日の出日没・競合検出）、ビート部品（`BeatClock`・`Deck`・波形・ホットキュー・`DjChannelStrip`・`StepSequencer`・`Spectrum`）、診断・パラメータパネル・表の取込を備えたスクリプト貼り付けパネルです。連続値の部品は`ValueEdit`を返します。`SemanticRole`に`Slider`と`Meter`を追加し、`RangeSlider`は`Group`ではなく`Slider`ノードを公開します。`PtzState`の公開フィールドは`speed`だけになりました。`StatusBadge`・`NotificationCard`・`ValidationMessage`の`const Theme*`版と、旧世代の`EmptyState(message)`・`Progress(id, fraction, label)`・`ToastRegion(span<Notification>)`は、`ComponentOptions`版と新しい綴りへ移行するため非推奨にしました。警告を消すには`IMKIT_DEPRECATED(message)`を空に定義します。Galleryのページ25〜30が新モジュールを見せます。第5弾でライブ操作を作り直しました。`GoBar`はコンソールのデッキ（ARMキー、Spaceのヒント付きの大きなGOキー、一時停止、長押し確認のStop allとPanic、キュー番号と残り時間を載せたNextカード）になり、`CueList`は見出し行、行の区切り、インデントガイド、見やすい実行中/待機行を備え、`ClipLauncher`は画像サムネイルを表示して列を固定の縦横比で幅に収められます。`PadView`に`image`・`uv0`・`uv1`を追加しました。`imkit::show`の新部品は、`ConsoleButton`・`ScribbleStrip`・`MasterBar`、`image.h`の`ImageButton`・`ImageCropEditor`・`FitImage`（任意のテクスチャをボタンにし、エンドユーザーが画像をドロップ・選択・削除・位置調整できます）、`surface.h`の`ControlSurface`・`SurfacePalette`と幾何の補助（フェーダー・ノブ・キー・画像・表示を自由に置くグリッドのキャンバスで、編集モードでは選択・移動・サイズ変更・追加・複製・削除ができます）、`led.h`の`LedColorPicker`・`GradientEditor`・`PixelPreview`・`BudgetBar`・`ColorOrderPicker`・`TestPatternBar`・`PortStrip`と純関数の`KelvinToRgb`・`RgbToRgbw`・`MixLightColor`・`SampleGradient`・`EstimateLedCurrent`、`address.h`の`AddressMap`・`ChannelMonitor`・`FindOverlaps`・`FirstFreeRange`・`PixelsPerUniverse`です。LEDとライブ操作のアイコン32個を加え、カタログは320個（21カテゴリ）になりました。公開構造体への追加は末尾のみです。`HoldButtonOptions`に`key`と`hint`、`GoBarView`に`nextNumber`と`totalSeconds`、`CueListOptions`に`showHeader`を追加しました。Galleryのページ31〜33がサーフェス、LED部品、画像ボタンを見せます。

## 3.3.0 — 2026-10-23

Shared progress tracks, stage lists, task progress cards and richer loading toasts make long-running work easier to follow. `ProgressTrack`, `StageProgress` and `TaskProgressCard` take borrowed views; the host owns values and handles operation IDs. `ToastView` adds `stage`, `progressText` and `status`. Recompile applications and static libraries with matching 3.3 headers; do not mix 3.2 objects with 3.3 structures. Dependencies and the Dear ImGui baseline are unchanged. See [the 3.3 guide](docs/reference/v3.3-api.md).

共通の進捗トラック、工程一覧、操作付きタスクカード、進捗情報を持つトーストを追加しました。`ProgressTrack`・`StageProgress`・`TaskProgressCard`は表示用の参照を受け取り、値の管理と操作IDの処理はホストが行います。`ToastView`に`stage`・`progressText`・`status`を追加したため、アプリと静的ライブラリを同じ3.3ヘッダーで再コンパイルしてください。3.2のobjectと3.3の構造体は混在させないでください。依存とDear ImGuiの基準revisionは変更していません。[3.3追加API](docs/reference/3.3追加API.md)を参照してください。

## Media revision 2 / 映像改訂2 — 2026-10-05

The 60-second English film now uses continuous native operation footage, a large synchronized cursor, kinetic typography, close-ups, connected transitions and an original 128 BPM electro-house score. All twelve principal GIFs were recaptured at 960×540, 20 fps, 6–8 seconds and under 2 MiB each. README and Pages point to the revised film; Pages uses a content hash in the video URL. The v3.2.0 library tag, source archive and five SDK archives retain their original identities.

英語60秒動画を連続したnative操作映像から作り直しました。大きなカーソル、動く文字、部品への寄り、場面をつなぐ切り替え、オリジナルの128 BPMエレクトロハウスを組み合わせています。主要GIF12本も撮り直し、960×540、20 fps、6〜8秒、各2 MiB以下にそろえました。READMEとPagesから改訂版を案内し、Pagesの動画URLには内容ハッシュを付けます。v3.2.0のライブラリタグ、sourceアーカイブ、5構成のSDKアーカイブの識別情報は維持します。

## 3.2.0 — 2026-10-04

**Added:** WorkspaceTabs, hierarchy visibility/lock/group action requests, inspector cards, SettingToggleRow, DragVector3WithUnit, titled responsive ChoiceGroup, CompactActionRow, ToastViewport, SubtleDestructive action style, Slate (13th theme), and USB/Network/Smartphone/IndustrialCamera icons (288 total).

**Gallery and documentation:** category navigation, independent Theme only / Components comparison, host-applied 3.2 workspace, refreshed native captures, bilingual public guides and declarations, and a 60-second English motion graphics film with original electronic music. Pages now includes toast documentation and the 3.2 API reference.

**Compatibility:** `WindowFrameContent::iconTexture` extends the structure. Recompile all consumers and ImKit static libraries against 3.2; do not mix 3.1 object files. The pinned Dear ImGui revision remains unchanged. No new library dependency is introduced.

**日本語**

作業タブ、階層の可視・ロック・見出し操作要求、設定カード、設定行、単位付き三軸入力、見出し付き選択、状態と操作の行、ToastViewport、SubtleDestructive、13番目のテーマSlateを追加しました。USB、Network、Smartphone、IndustrialCameraを含む288アイコンを利用できます。

Galleryのカテゴリ案内、独立した2モードの比較、要求をアプリが反映する3.2作業画面を整えました。実キャプチャGIF、英日ガイドとAPI宣言、オリジナルBGM付きの約60秒英語モーショングラフィックス動画を公開します。Pagesにもトーストと3.2 API参照を含めます。

`WindowFrameContent::iconTexture` の追加により構造体が拡張されています。アプリとImKit静的ライブラリを3.2のヘッダーで再コンパイルしてください。3.1のobjectは混在させないでください。Dear ImGuiの対応revisionは変更せず、新しいライブラリ依存も追加していません。

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
