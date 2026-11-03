---
title: "Workflow部品"
description: "imkit/workflow.h（imkit/imkit.hから取り込まれます）は、ツールの作業画面に使う部品をまとめています。ナビゲーション、まとまった設定、階層の行、フィードバック、進捗、プレビュータイルを備えた画像ビューアです。imkit::imkitをリンクします。"
---

`imkit/workflow.h`（`imkit/imkit.h`から取り込まれます）は、ツールの作業画面に使う部品をまとめています。ナビゲーション、まとまった設定、階層の行、フィードバック、進捗、プレビュータイルを備えた画像ビューアです。`imkit::imkit`をリンクします。画像ビューアは、エディタのキャンバス、選択、スプリッターを使うので、`imkit/editor_canvas.h`と`imkit::editor_core`も必要です。

![GalleryのWorkspace画面。タブ、階層、インスペクターカード、設定が1画面に並ぶ](/imgui-modern-kit/docs-images/v4-workspace-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-workspace.gif)

## 部品の一覧

どの部品も、要求を返します。渡した値を、部品が書き換えることはありません。

| 分類 | 部品 | 役割 |
| --- | --- | --- |
| ナビゲーション | `StepNavigator`、`NavigationRail` | 借用した`StepItem`のspanに対する、ステップバーと縦のレール。選択の要求を返します |
| | `WorkspaceTabs` | アイコンとラベルのタブ。幅が狭いと、コンボに縮みます |
| | `ChoiceGroup` | 見出し付きの枠に、ラジオ形式の選択肢を1〜3列で並べます。選ばれた安定IDを返します |
| 階層と設定 | `HierarchyGroupHeader`、`HierarchyRow` | シーングラフのような行。行の全幅で選択でき、長いラベルは省略され、`accentColor`で色の線を付けられます。選択、表示、ロック、グループの操作は要求です。使えない表示やロックの操作は、省略できます |
| | `BeginInspectorCard` / `EndInspectorCard` | 関連する部品を、高さが自動で決まる枠付きのカードにまとめます。必ず対にします |
| | `BeginCollapsibleInspectorCard` / `EndCollapsibleInspectorCard` | 開閉の状態をアプリが持つ同じカードです。Endは、Beginがtrueを返したときだけ呼びます。閉じた中身は描かれません |
| | `SettingToggleRow` | ラベル、説明、任意の無効の理由を持ち、トグルの要求を返します |
| | `FilterChip`、`SectionHeader` | 選択や開閉の状態を明示して渡し、標準の操作で切り替えます |
| フィードバック | `NotificationCard`、`InlineAlert`、`PersistentBanner` | 意味のある状態と、閉じる要求です。バナーは、アプリが外すまで残ります |
| | `SelectNotifications` | 期限切れを除き、優先度で並べます。重複は、最後のものが勝ちます |
| | `ToastRegion` | アプリが持つキューと時刻で動き、作業領域に収まるスタックを表示し、フォーカスを奪いません。色付きのカードは、[トースト](../toasts/)を参照してください |
| | `EmptyState`、`UnavailableState`、`RetryState` | 見出し、説明、任意のアイコンと操作を持ち、要求を返します |
| | `MultiSelectionBar`、`HelpCallout`、`ValidationSummary` | アプリが持つ件数、コマンド、問題を表示します。検証の仕組みはありません |
| 進捗 | `Progress`、`ProgressTrack`、`StageProgress`、`TaskProgressCard`、`CircularProgress` | [進捗](#進捗)を参照してください |
| レイアウト | `BeginCard` / `EndCard` | Beginがfalseを返しても、必ず対にします |
| | `ResponsiveToolbar` | あふれたときのメニュー、アイコンのラベル、無効の理由、標準のキーボードフォーカスに対応します |
| | `ResolveRightSidePanelLayout`、`RightSidePanelHandle` | 端のハンドルと、マウスやキーボードでの幅の変更を備えた、折りたためる側面パネルです |
| 画像ビューア | `BeginImageViewport` / `EndImageViewport` | 借用したテクスチャに対する、パンと一様なズームです。必ず対にします |
| | `ZoomToolbar`、`DrawOverlay` | Fit、Fill、1:1と、ズーム、パン。点、折れ線、矩形、円、ラベルのオーバーレイは、表示領域で切り取られます |
| | `ResolveImagePlacement`、`FitImage`、`ClampImage`、`ImageGeometryValid`、`PixelToNormalized`、`NormalizedToPixel` | テクスチャを必要としない、純粋な幾何計算です |
| | `PreviewTile`、`ResizableTileStrip` | 借用したプレビューと操作、縦横のスクロール、最小のサイズを備えます |

これらのAPIのコンパイルとリンクの一覧は、`tests/workflow_api_compile.cpp`です。ソースとSDKを再配置した利用側のビルドでも、これをビルドします。Dear ImGuiのエイリアスは、`docs/reference/api-inventory.json`に別に載せています。

## 所有と寿命

- ラベル、テクスチャ、候補のビュー、作業用のバッファーは、呼び出しの間だけ借用します。状態とキューは、アプリが持ちます。
- IDは、0でなく、インスタンスごとに一意でなければなりません。通知だけは例外で、重複したときは、最後のものが勝ちます。
- `ChoiceGroup`は、候補と任意の`IconAtlas`を借用します。返されたIDは要求で、現在の値を変えることはありません。
- ここにある部品は、テクスチャのアップロードも、ワーカーの起動もしません。保存、レジストリ、Undoの仕組みもありません。
- スコープは、Contextを破棄する前に、すべて終えます。アクセシビリティ用に公開した文字は、アプリがフレームを公開するまで有効に保ちます。
- イベントバッファーは、毎フレームリセットします。いっぱいになると、`overflow`が立ちます。キーボードでタイルの大きさを変えるには、Begin、Update、Commitの3つの空きが要ります。マウスで取り消すと、隣り合う両方の大きさが元に戻り、終了のイベントは、バッファーがいっぱいのときに、再送されます。
- タイルの大きさは、IDをキーにした、アプリが持つ項目です。通知の並べ替えは、渡された添字用の作業領域を使い、その容量と要求した最大数の小さいほうまでを返します。
- テーマ、ロケール、意味の公開には、`ComponentOptions`を渡します。描画だけのオーバーレイは、操作できるノードを作らないので、編集対象のオブジェクトは、アプリが記述します。ステップの矢印は、無効な項目を飛ばし、標準のTab、Shift+Tab、操作の確定は、そのまま使えます。動きを減らす設定のときは、遷移の代わりに、静止した表示を使います。

## 進捗

`ProgressTrackView`は、工程名、状態の文、割合（または`valueText`）、丸みのあるバー、補足の文の、共通の元になります。`Progress`と`TaskProgressCard`はこれを再利用するので、カードやトーストが、割合や状態の文を別に組み立てることはありません。

| 項目 | 動作 |
| --- | --- |
| `ProgressStatus` | `Queued`、`Running`、`Paused`、`Succeeded`、`Failed`、`Cancelled` |
| 不定 | 割合が負または非有限のとき。短い区間が動くのは、`Running`のときだけで、動きは、`Theme.motion.enabled`、`reducedMotion`、アプリの`AnimationState`で決まります |
| `Progress` | インライン、子ウィンドウのオーバーレイ、モーダルのいずれか。キャンセルの要求を返します |
| `StageProgress` | 渡した工程と現在のIDを表示するだけです。幅が狭いと、現在の工程と`n/total`を表示します |
| `TaskProgressCard` | 通常、またはコンパクト（`TaskProgressCardOptions::compact`）の概要です。押された`Command.id`を返し、0は要求なしです。文字列と操作のspanは、そのフレームの間だけ借用します |
| `CircularProgress` | 借用した割合、値、ラベル。割合が負または非有限なら、値なしです。しきい値はなく、状態も持ちません |

```cpp
imkit::ProgressTrackView progress;
progress.fraction = fraction;     // 負または非有限なら、不定
progress.status = imkit::ProgressStatus::Running;
progress.label = "Inspecting files";
progress.supplemental = "The host owns progress updates.";
imkit::ProgressTrack("import-progress", progress, {&theme, &animation});
```

![Galleryの進捗トラック、段階表示、タスクカード](/imgui-modern-kit/docs-images/v4-progress-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-progress.gif)

## 画像ビューア

画像の座標は、左上が原点で、xは右へ、yは下へ増えます。キャンバスの単位は画像のピクセルで、デスクトップの画面座標とは、`ToScreen`と`FromScreen`で変換します。

| モード | 結果 |
| --- | --- |
| Fit | 画像の全体が見える |
| Fill | 画像が表示領域を埋め、はみ出す部分が切り取られる |
| 1:1 | 画像の1ピクセルが、ImGuiの座標の1単位になる。物理的な1ピクセルとは限らない |

`ClampImage`は、小さい画像を中央に置き、大きい画像は、端の範囲に収めます。カーソルを基点にしたズームは、端での補正が必要になるまで保たれ、大きさを変えると、手動でないモードは、収め直されます。矩形選択と投げ縄選択は、`CanvasSelection`が行います。

`ResolveImagePlacement`は、使える領域の左上を原点とする、ローカルの座標で動きます。Fitは、中央に置いた出力先と、全体のUVを返し、Fillは、出力先の全体と、中央を切り取ったUVを返し、Stretchは、両方の領域の全体を使います。テクスチャもレンダラーも持たず、正でない、非有限、描画できない形状には、`valid = false`を返します。

## 右の側面パネル

`ResolveRightSidePanelLayout`は、内容、常に見えている端のハンドル、任意のパネルの、それぞれの幅を返します。この順に描き、同じ使える範囲を`RightSidePanelHandle`に渡します。`N`のようなキーを`toggleRequested`にするかは、アプリが決めます。そのため、ライブラリが、ほかのエディタのテキスト入力やショートカットを奪うことはありません。折りたたんでいる間も、ハンドルは見え、パネルは、内容を覆わずに、内容の幅を狭めます。

## Galleryで試す

**Workspace**、**Workspace shell**、**Progress and states**、**Preview tiles**の画面を開きます。**Workspace**では、WorkspaceTabs、階層の行、インスペクターカード、`SettingToggleRow`、`DragVector3WithUnit`、`ChoiceGroup`、`CompactActionRow`を組み合わせ、返った要求で、Galleryが持つ値を更新しています。同じ形は、既存のImGuiのフレームでも使えます。[3.2の宣言と寿命の規則](../../api/v3-2/)を参照してください。ソースは`examples/gallery/workflow_pages.cpp`です。

`--verify-workflow --output <ディレクトリ>`は、公開IOの確認を行い、36通りの画面とテーマの組み合わせと、日本語で幅が狭い例と無効の例の2つをキャプチャします。これらは、ネイティブのGPUキャプチャで、ネイティブのOS、IME、スクリーンリーダーのテストとは別のものです。
