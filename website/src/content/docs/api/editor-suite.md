---
title: "Editor API"
---

ImKit のエディタスイートは、Dear ImGui 標準の基本オーバーロード群とは独立した、プロフェッショナルな制作ツール（DCCツール、動画編集ソフト、シーケンサ等）向けの高機能UIコンポーネントAPIです。

- **タイムライン追加API**: `FadeCurve`, `FadeView`, `EvaluateFade`, `FadePicker`, `CutTransitionView`, `TimelineEditingProvider`, `TrackAction`, `ClipboardAction`, `TransitionShelf`。操作ジェスチャーやホスト所有権の詳細は [タイムライン編集](../../features/timeline/) を参照してください。
- **アウトライナー装飾**: `cg::ObjectView::icon` により行アイコンを任意に指定できます（`IconId::Count` で既定のアイコン表示）。
- **一括確定グループ**: `Event::operation` および `operationSize` により、一括確定・取り消しを行う編集トランザクション集合を表現します。
- **クリップボードコマンド**: `CopyClips`, `CutClips`, `PasteClips` をショートカットコマンドとしてバインド可能です。
- **統合要件**: すべてのターゲットは C++20 を採用し、ホストアプリケーションの Dear ImGui ターゲットを継承します。必要なモジュールのみをインクルード・リンクして使用してください。

| CMake ターゲット | 公開ヘッダー / 名前空間 | 主な提供 API |
|---|---|---|
| `imkit::editor_core` | `imkit/editor_core.h`<br>`imkit::editor` | `StableId`, `Tick`, `FrameRate`, `FrameToTick`, `TickToFrame`, `FormatTimecode`, `ParseTimecode`, `EventBuffer::PushBatch`, `EventBuffer`, `Transaction`, `Selection`, `ResolveSnap`, `CanvasState`, `BeginCanvas`, `CanvasSelection`, `TimeRuler`, `Transport`, `CurveEditor`, `ResolveHandles`, `MoveHandle`, `Evaluate`, `PropertyGrid`, `AssetBrowser`, `Splitter`, `StatusBar` |
| `imkit::video` | `imkit/video.h`<br>`imkit::video` | `TimelineProvider`, `TimelineState`, `Timeline`, `EditClip`, `RollClips`, `SlideClip`, `SplitClip`, `EditTransition`, `TransitionPicker`, `EvaluateEnvelope`, `Monitor`, `MonitorView`, `MonitorOptions`, `MonitorControls`, `MonitorLabels`, `BuildAudioBuckets`, `UpdateMeter`, `Waveform`, `WaveformProvider`, `WaveformQuery`, `WaveformView`, `WaveformPixel`, `DrawWaveform`, `PlacementMode`, `LevelMeter`, `AudioStrip`, `BuildScopes`, `Histogram`, `ScopeImage`, `ColorControls` |
| `imkit::cg` | `imkit/cg.h`<br>`imkit::cg` | `Project`, `TransformDelta`, `OrientationBasis`, `BeginViewport`, `ViewportObjects`, `TransformGizmo`, `PreviewTransform`, `Outliner`, `TransformUV`, `UVEditor`, `DopeSheet`, `AnimationStrips` |
| `imkit::cg` | `imkit/preview.h`<br>`imkit::preview` | `Vertex`, `Mesh`, `Triangle`, `Cube`, `Sphere`, `DrawListPreview` |
| `imkit::preview_opengl3` | `imkit/preview.h`<br>`imkit::preview` | `GLFunctions`, `OpenGL3Renderer::Init/Resize/Render/Pick/Shutdown/Texture` |
| `imkit::editor_suite` | 上記モジュールヘッダー | インターフェース集約ターゲット: `editor_core` + `video` + `cg`（`preview_opengl3` を除く） |

**モジュール依存関係**:
- `editor_core` → `imkit`
- `video` → `editor_core`
- `cg` → `editor_core`
- `preview_opengl3` → `cg`

外部のエディタ・プロット・ギズモライブラリや独自の GL ローダーへの暗黙の依存は一切ありません。ソースビルドとインストール版 SDK で同一のターゲット名がエクスポートされます。

---

## Providerとevent

```cpp
std::array<imkit::editor::Event, 128> eventStorage;
imkit::editor::EventBuffer events{eventStorage};
std::array<imkit::editor::StableId, 128> selectionStorage;
imkit::editor::Selection selection{selectionStorage};
imkit::video::TimelineState ui; // フレーム間をまたいでホスト側で保持

// 通常の Dear ImGui フレームループ内:
imkit::video::Timeline("timeline", provider, ui, selection, events, theme);
for (const auto &event : events.Events()) {
    // ホスト側でリビジョン確認、ドラフト反映、確定(Commit)、Undo記録を行う
    DispatchToHost(event);
}
events.Clear();
```

### イベントと状態管理の設計原則

- **ホストアプリケーションの所有権**: 上記の `provider`、`theme`、およびイベントディスパッチ処理はすべてホスト側が所有します。
- **高精度なタイムスタンプと識別ID**: `Value.first`, `last`, `offset` は整数タイムスタンプ（`Tick`）を正確に保持します。`parent` には 64-bit 非ゼロの `StableId` を保持します。
- **数値チャンネルの格納規則**: `x`, `y`, `z`, `w` フィールドは編集操作に応じて以下の数値をエンコードします：
  - **クリップ編集**: `{start, end, sourceIn, trackId, speed}`
  - **プロパティ編集**: `x`
  - **トランスフォーム変形**: `x / y / z`
  - **カーブキー編集**: `first`（Tick時刻）および `x`（値）
  - **カーブハンドル編集**: `x / y`（相対秒 / 相対値）
  - **親子付け替え（Reparent）**: `parent`
- **インライン名称変更**: 名前変更イベントは、元の文字列および提案文字列を終端ヌル文字込み 256 バイトの固定配列で返します。これを超える長大な文字列の編集はホスト側の専用ダイアログ等で処理します。
- **固定バッファ容量とオーバーフロー**: 選択項目やトランザクションの作業バッファはホストから容量明示で提供されます。1フレーム内の全フェーズを収容できる十分なバッファを確保してください。容量不足時は暗黙のヒープ確保を行わず、オーバーフローフラグを返します。
- **Gallery サンプルでの実装範囲**: Gallery での動作実証範囲は [エディタスイート](../../features/editor-suite/) に記載されているサブセットに準拠します。

---

## GL pass contract

### カメラナビゲーション API

| API | 提供契約 |
|---|---|
| `NavigateCamera(Camera&, Point orbitPixels, Point panPixels, double wheel, double viewportHeight)` | 描画領域のスケールに応じた純粋なカメラ回転（Orbit）、視点相対パン、および正投影ズーム操作。 |
| `AlignCamera(Camera&, Axis, bool negative = false)` | 6方向（X / Y / Z の正負方向）の直交正投影ビューへカメラを整列。 |
| `ViewportState::cameraView` | `BeginViewport` 呼び出し中のみ有効な、ホスト側カメラオブジェクトへの非所有参照。 |
| `ViewportState::navigationGizmo` | クリック可能なビュー軸ナビゲーションオーバーレイの表示/非表示を切り替え。 |

### カラーコレクション API

| シグネチャ | 提供契約 |
|---|---|
| `ScopeBuffers::redWaveform/greenWaveform/blueWaveform` | 任意の `width * 256` 要素のRGBビン配列（3チャンネル一括指定のみ有効）。 |
| `ScopeImage(..., const Theme&, ImVec4 tint)` | テクスチャ所有権を持たない、CPU計算ベースのスコープ描画。 |
| `Rgba ApplyColorCurves(Rgba, const ColorCurveSet&)` | 時刻順にソートされた非所有RGBキースパンを評価。正規化入力は `0..TicksPerSecond` にマッピングされ、空チャンネルは恒等変換、出力はクランプされ、アルファ値は保持。 |
| `bool ColorControls(const char*, ColorValues&)` | ホスト側のドラフト値を直接編集する既存互換オーバーロード。 |
| `void ColorControls(const char*, const ColorValues&, const ColorPropertyIds&, uint64_t, ColorState&, EventBuffer&, const ColorLabels& = {})` | 不変のソースデータと型付きRGB/スカラーのトランザクションを生成する安全なオーバーロード（6個の独立プロパティID）。 |

`ColorControls` の両オーバーロードは、ドラッグ操作可能な3-wayカラーホイールとレベルスライダーを描画します。`ColorState` はホスト側のオーバーレイ描画や自動操作テスト用に各ホイールの中心座標と半径を記録します。色空間変換パイプライン自体はホスト側の責務です。

Gallery のカラー画面では、補間とハンドル調整が可能な共通の `CurveEditor` を使用してホスト所有のRGBキーを編集します。確定（Commit）時に合成テスト画像とスコープビンを再計算し、取り消し（Cancel）では既存バッファをそのまま保持します。

### オーディオ API

| API | 提供契約 |
|---|---|
| `TrackControl` | イベント `x` に安定したトグル項目セレクタ、`y` に真偽値フラグを格納。 |
| `AudioStripView::gainId, panId` | 明示的なプロパティID（0 は無効状態、算術的なID推論は行わない）。 |
| `AudioStripView::locked`, ラベルポインタ | ホスト側のロック状態フラグおよび借用UTF-8ラベル文字列。 |
| `AudioStrip(...)` | ゲインフェーダー・パン調整トランザクション、およびミュート/ソロ/録音イベントの発行。 |
| `ClipView::audioBuckets` | クリップ内部に描画される PCM min/max 波形バケットへの非所有参照。 |

### トラックレイアウト API

| API | 提供契約 |
|---|---|
| `float TrackExtent(const TrackView&)` | 展開・折り畳み状態を考慮した実際のトラック描画高さを算出。 |
| `TrackLayout { tracks, top }` | 連続した可視行スパンと最初の可視行の絶対ピクセルオフセット。 |
| `TimelineProvider::layout`, `totalHeight` | 任意のインデックス付き可変高さクエリ（均一高さトラックも完全サポート）。 |
| `TimelineState::heightDrag` | トラック高さ変更中のホスト側ドラッグ状態。 |
| `EditKind::TrackHeight` | 提案イベント（`x` に変更前後の高さ、`target` にトラックIDを格納）。 |

### OpenGL 3.3 プレビューのライフサイクル

1. 現在有効な OpenGL 3.3 コンテキストから全関数ポインタを取得し、`GLFunctions` に設定します。
2. `OpenGL3Renderer` を初期化し、非所有メッシュビューを描画します。
3. `Texture()` で取得したテクスチャIDを Dear ImGui UI に渡します（FBO描画のため上下反転UVを使用）。
4. `Pick(x, y)` は左上原点の座標を受け取り、64-bit の識別IDを返します（背景は0）。
5. コンテキスト破棄前には必ず同一の OpenGL コンテキスト上で `Shutdown()` を呼び出してください。
6. パイプライン状態の復元（ブレンド、シザー、深度テスト等）およびスレッド同期はホスト側の責務です。実装例は `examples/gallery/main.cpp` を参照してください。

### コンパイル検証フィクスチャ

公開フィクスチャ `tests/editor_api_compile.cpp` は全公開ターゲットをリンクします。外部プロジェクトでもこのフィクスチャをビルドすることで、API宣言の整合性を確認できます。

### トランスポート操作

- **アトラスアイコン対応**: `Transport(state, bindings, const IconAtlas*)` は再生・一時停止・停止・コマ送りボタンにホスト所有のアイコンテクスチャを使用します（null 指定時はテキスト文字ボタンを使用）。
- **J/K/L キーボードショートカット**: `PlayReverse` / `Pause` / `PlayForward` コマンドに対応：
  - `K` キーで現在位置を一時停止。
  - `Stop` でイン点へ巻き戻し。
  - コマンドバインディングが空の場合はキーボード再生を無効化。
  - ループ再生を含む全コマンドはホスト側で自由にリマップ可能。

### タイムライン吸着とナビゲーション

- `ResolveTimelineSnap` は移動中のクリップ両端を評価し、移動対象IDと無効な吸着種別を除外してスナップ位置を決定します。
  - `snapping`: 吸着全体のマスター切り替え。
  - `magnet`: プロバイダー候補への吸着有効化。
  - `snapToFrame`: 画面上 8 ピクセル以内でのフレーム境界吸着。
  - `SnapResult.tick`: 主端点吸着位置、`candidate.tick`: ガイド表示位置。
- **再生ヘッド追従（FollowPlayhead）**: Off、Smooth、Page の3モードを提供：
  - `Smooth`: 進行方向の前後に画面幅の 10% の余白を保持。
  - `Page`: 画面幅単位でページ切り替え（逆再生対応）。
  - `TimelineState.autoScroll` で挙動を選択。
- **タイムルーラー（TimeRuler）**: 横ズーム倍率に応じたフレーム単位の目盛りを自動描画。
- **全体表示（Fit）**: 左右に 24 論理ピクセルのパディングを付加して全コンテンツを表示（縦スクロール位置は維持）。
- **ズームバーとオプション**: `TimelineState::options` で対数ズームバーの範囲や表示ツールを制御。

### プロパティ操作と CG インスペクタ

- **コンテキスト操作**: プロパティがロック中でも「お気に入り」「ロック」「オーバーライド」のコンテキストメニューを表示可能。
- **キーフレーム操作イベント**: `EditKind::PropertyKey` によりカーブキーとは独立してプロパティ値をキーフレーム化。
- **アセットブレッドクラム**: `Navigate` イベントを発行し、階層ナビゲーションをホスト側で処理。
- **CG インスペクタ**: 移動・回転（ラジアン）・スケールの全9成分を編集可能。リセット時は移動/回転を 0、スケールを 1 に復帰。
- **複数選択時の Mixed 表示**: 値が異なる複数オブジェクトの選択時は `Mixed` を表示。ロックされたプロパティは編集を拒否。

### カーブエディタ

- **可視範囲クエリ**: チャンネル/時刻インデックスに基づき、可視キーおよび接線計算用の隣接2キーを取得。
- **補間とハンドルメニュー**: キーの右クリックから `KeyInterpolation` / `KeyHandleMode` を変更。
- **アクティブチャンネル**: クリックされたキーのチャンネルへ自動切り替え。`ghostOtherChannels` 有効時は非アクティブチャンネルを薄い破線で表示。
- **全区間評価**: `CurveProvider.sample` コールバックにより、外挿モード（Constant / Linear / Repeat）を含む正確なカーブ評価を実行。
- **キーの複製とスナップ**: Altドラッグで新しい `StableId` を割り当てた `Duplicate` イベントを発行。`snapToFrame` でフレーム境界へ吸着。

### アニメーションストリップと UV エディタ

- **ストリップ設定**: `EditKind::StripSettings` で範囲、チャンネル、スケール/リピート/ブレンド、ミュート/ロック設定を返却。
- **UV 選択モード**: 頂点、エッジ、面、アイランドの各モードで矩形・投げなわ選択をサポート。
- **座標規約**: 内部幾何情報は正規化 UV 単位を維持。UDIM 表示は `1001 + u + 10v`（列 0..9）の標準規約に準拠。
- **トポロジー表示**: `UVEdge` によりエッジ表示を頂点提案座標へ接続。`UVFace` はポリゴン内外判定と重なり（overlap）のアウトライン表示を支援。

---

## TimelineおよびMonitorの拡張API

### タイムライン & モニタ拡張 API 一覧

| API | ホスト提供契約 |
|---|---|
| `ClipView::keyChannel`, `keyDefaultValue` | 明示的なキー挿入先チャンネルおよび空チャンネル時の初期値。 |
| `ClipView::envelope`, `EnvelopePoint` | `StableId`、ゲイン値、ロック状態を持つ、ローカル時刻順の非所有エンベロープ点列。 |
| `EvaluateEnvelope(points, tick)` | 線形ゲイン補間評価（範囲外は端点保持、空データ時は 1.0 を返却）。 |
| `EditTransition(clip, end, delta)` | 他端を固定しクリップ長内に収まるようにトランジション長を伸縮する純粋計算。 |
| `TransitionPicker(id, clip, revision, events, trackLocked)` | トランジション種別変更のアトミックな Begin/Commit イベントを発行。 |
| `MonitorOptions::transformBounds`, `anchor` | テクスチャの反転とは独立した、正規化表示矩形およびアンカー点。 |
| `MonitorOptions::metadataPreset` | メタデータ表示モード（`Off` / `Clip` / `Details`）。 |
| `MonitorOptions::clipName`, `markerComment`, `metadata` | 描画呼び出し中のみ有効な借用 UTF-8 文字列（オーバーレイ領域内に自動クリップ）。 |
| `EditorPalette::effectClip`, `adjustmentClip`, `groupClip` | トラックの役割に応じたセマンティックカラー。 |

### タイムライン対話操作の詳細

- **キーフレーム挿入**: `AddKey` は `target=channel`, `parent=clip`, `first=クリップ内tick`, `x=値` の `KeyInsert` イベントを発行。前後キー移動は `Navigate` イベントを発行。
- **オーディオエンベロープイベント**: `AudioEnvelope` イベントを発行（編集/削除時は `target=point`、追加時は `target=clip`、`offset`: 0=編集, 1=追加, 2=削除）。
- **トラックラベル**: `video::TrackLabels` によりトラックボタンや各種ツールチップの UTF-8 文字列を非所有で設定。
- **編集開始の事前検証**: `TimelineProvider::canBeginEdit(user, clip, kind)` により、操作開始前にホスト側で編集可否を判定可能（false 返却時は容量不足扱いとせず操作を安全に抑止）。
- **外部ドラッグ＆ドロップ**: `TimelineProvider::externalDrops` で複数のペイロードルートを受け入れ、プレビュー判定と1回のドロップ確定を処理。
- **編集ツールショートカット**: `ToolSelect`, `ToolRazor`, `ToolRipple`, `ToolRoll`, `ToolSlip`, `ToolSlide`, `ToolHand`（既定で V/C/B/N/Y/U/H キーに割り当て）。
- **クリップのリンクとグループ化**: コンテキストメニューからクリップのリンク・グループ解除、または複数クリップの一括リンク・グループ作成が可能（`EditKind::Link`）。

### CG ビューポートとオーバーレイ機能

- **シアーとアフィン変形**: `cg::Transform::shear` は上三角行列の xy, xz, yz 成分を保持。非均等スケール操作時も QR 分解により完全なアフィン変換を維持。
- **法線ベクトル表示**: `preview::DrawMeshNormals` により頂点法線および面法線をビューポート上にオーバーレイ描画。
- **ビューポートラベル**: `cg::ViewportState::labels` でツール名や投影モード等の UTF-8 文字列を多言語設定可能。
- **カメラ枠と安全枠**: `ViewportState` によりセンタリングされたカメラ枠、タイトル安全枠（5% / 10% インセット）、枠外暗転（パスポルトゥ）を描画。
- **メッシュ選択輪郭**: `preview::DrawMeshOutline` により選択メッシュのシルエット境界線をオーバーレイ表示。
- **カメラ・ライトのプリミティブ**: 視錐台（`CameraPrimitive`）および点光源八面体（`LightPrimitive`）のヘルパーメッシュをホストバッファへ書き込み可能。
- **関連クリップのリップル・ロール・スライド**: リンク・グループ化された関連クリップを包括して共通差分で一括変形。
- **カメラ姿勢の同期**: `CameraFromTransform` によりローカル +Z 前方 / +Y 上方のトランスフォームからカメラ姿勢を算出。
- **アウトライナーの行種別**: `ObjectView::kind` により Object, Collection, Mesh, Camera, Light, Component, Modifier 行を識別・フィルタリング。

### モニタ表示コンポーネント

- **モニタ操作パネル**: `MonitorControls` によりセーフエリア、ガイド、タイムコード、変形枠、メタデータの表示設定をホスト所有で編集。
- **Monitor 表示オーバーロード**: `Monitor(id, MonitorView, ...)` により、ホストが更新する画像ビューとプレビュー状態を表示：
  - `Fit`: 元画像のアスペクト比を維持して全体表示。
  - `Fill`: 中央基準でクロップして領域全体を満たす。
  - `Stretch`: 自由なアスペクト比で引き伸ばし表示。
  - 読み込み中、空状態、オフライン、エラー等の状態を共通コンポーネントで統一表示。
- **責務の明確な分離**: ImKit はレイアウト計算、DrawList 描画、オーバーレイ合成、入力検出を担当し、テクスチャ生成、動画デコード、レンダリング、ファイル保存はすべてホスト側が所有します。

---

## 範囲・マーカー・配列コントロール

### タイムルーラーと範囲操作

- **表示リソースの借用**: `TimeState::rulerLabels` および `icons` を通じてホストのリソースを借用。
- **区間指定**: `TimeRuler` は作業領域（Work range）およびイン/アウト区間を描画。端点ドラッグで範囲を変更し、Escape キーで開始前の範囲に復帰可能。
- **コンテキスト操作**: 右クリックメニューから秒単位での数値編集や範囲クリアが可能。

### マーカー操作

- **マーカージェスチャー**: マーカーのドラッグ移動時に明示的なマーカー ID を持つ `Begin / Update / Commit / Cancel` イベントを発行。
- **追加と削除**: 右クリックからの削除は `Remove` イベントを発行。新規追加は `target=0` を指定してホスト側での新規 `StableId` 採番を要求。

### プロパティ配列の操作

- **要素の並べ替え**: `PropertyView::arrayElement` および `PropertyProvider::neighbor` により、ロックされていない配列要素の上下並べ替えをサポート。
- **イベント情報**: 並べ替え操作は `proposed.parent` に移動先の兄弟要素 ID を格納して発行。
- **表示とラベル**: 複数選択時の `Mixed` 表示はテキストとして直接レンダリング（数値書式文字列として誤解釈しない安全設計）。

### リップル削除（Ripple Delete）

- `RippleDeletePosition` は時刻順で重複しない削除クリップから残存クリップの移動先を正確に算出（区間の重複や Tick オーバーフローを安全に防止）。
- タイムラインは関連クリップを一括してリップル削除するイベントバッチを発行。ホスト側で後続クリップのロック状態を確認した上で移動を適用。

### 字幕とコンポーネント階層

- **字幕トラック**: コンテキストメニューからの追加時に `CaptionInsert` を発行。再生位置以降の直近の空き区間に初期字幕を自動生成。
- **コンポーネントアイコン**: `ComponentView::icon` および `ComponentTypeView::icon` により、行アイコンをホストのアトラス画像で自由に区別可能。
- **クリップボード連携**: プロパティ値のコンテキストメニューからコピー＆ペーストを標準サポート。

アーキテクチャの背景や設計思想については [エディタ 2.0 リフレッシュ設計記録](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/archive/editor-refresh.md) を参照してください。
