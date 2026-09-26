# 汎用ワークフロー部品

[English](workflow-components.md)

`imkit/workflow.h`（`imkit/imkit.h` から transitively 取得される）を include し、`imkit::imkit` にリンクします。画像・preview には `imkit/editor_canvas.h` を include し、`imkit::editor_core` にリンクします。これらの追加 API は patterns、Canvas、Selection、Splitter を再利用します。Editor Suite 全体の完成を意味しません。

## 所有権

ラベル、texture、候補 view、scratch はすべて借用です。ID は instance 内で非 zero かつ一意（通知のみ例外：同一 ID の最後の要素が優先）。状態・queue はホスト所有です。texture upload、worker、永続化、registry、Undo engine、新規依存、icon 資産は追加していません。Context 破棄前に scope を閉じてください。semantic 文字列はホストの frame 公開まで有効に保持してください。

イベント buffer は frame ごとにリセットします。満杯時は `overflow` をセットします。keyboard の tile resize は Begin/Update/Commit の 3 slot を必要とします。マウス取消は隣接する両方の寸法を復元し、終端イベントは buffer が満杯なら再送されます。Tile の寸法は ID 付きのホスト所有エントリです。通知の並べ替えは呼び出し側の index scratch を使い、結果は scratch 容量と要求数の小さい方までに制限します。

## API 一覧

`tests/workflow_api_compile.cpp` が compile/link の一覧で、source・relocated SDK consumer でも使用されます。既存の署名と native API 一覧は維持しています。

| API | 契約 |
|---|---|
| `StepNavigator`、`NavigationRail` | 借用 StepItem span、現在 ID、明示的な focus 状態；選択要求を返す |
| `WorkspaceTabs` | アイコンとラベルの導航；幅が狭い場合は combo に切替；選択要求を返す |
| `HierarchyGroupHeader`、`HierarchyRow` | ホスト所有の group 状態と安定した行 ID；選択・表示・ロック・任意の group 操作は要求；利用できない表示・ロック操作は省略可 |
| `BeginInspectorCard`、`EndInspectorCard` | 関連コントロールを高さ自動の境界付き card にまとめ；Begin/End は必ず対応 |
| `SettingToggleRow` | ラベル、説明、任意の無効理由；ホスト状態を変更せずに切替要求を返す |
| `FilterChip`、`SectionHeader` | 明示的な選択/開閉状態、native 実行 |
| `SelectNotifications` | 期限フィルタより先に最終重複が優先；優先度降順、同順は安定 |
| `NotificationCard(FeedbackView)`、`InlineAlert`、`PersistentBanner` | 意味色と dismiss 要求；banner は永続 |
| `ToastRegion(id, ...)` | ホスト queue/時刻/scratch、作業領域に制限されたスタック、focus 奪取なし |
| `EmptyState(id, StateView)`、`UnavailableState`、`RetryState` | 見出し、説明、任意の icon/action；要求を返す |
| `Progress(ProgressView, ...)` | Inline、child overlay、modal；負の fraction は未確定；cancel 要求 |
| `CircularProgress(CircularProgressView, ...)` | 借用の fraction/value/label；負値・非有限値の fraction は測定不能；閾値や状態を所有しない |
| `BeginCard`、`EndCard` | Begin が false を返しても必ず対応 |
| `MultiSelectionBar`、`HelpCallout`、`ValidationSummary` | ホストの count、command、問題；検証 engine を所有しない |
| `ResponsiveToolbar(..., ToolbarOptions)` | Overflow、icon ラベル、無効理由、native キーボード focus |
| `ResolveRightSidePanelLayout`、`RightSidePanelHandle` | 開閉・幅はホスト所有、コンパクト右端 toggle、mouse/keyboard resize；shortcut 要求はホスト範囲 |
| `ImageGeometryValid`、`FitImage`、`ClampImage` | texture の有無に依存しない数値画像 layout |
| `ResolveImagePlacement` | 純粋な Fit/Fill/Stretch の目的地と UV crop 解決；無効 geometry は拒否 |
| `PixelToNormalized`、`NormalizedToPixel` | 画像寸法で乗除算；無効寸法是 zero を返す |
| `BeginImageViewport`、`EndImageViewport` | 既存 Canvas、借用 texture、uniform zoom；必ず対応 |
| `ZoomToolbar` | Fit/fill/1:1、zoom と pan；表示状態のみを更新 |
| `DrawOverlay` | Point/polyline/rectangle/circle/label、選択/ホバー装飾、clipped |
| `PreviewTile`、`ResizableTileStrip` | 借用 preview/action、水平/垂直スクロール、最小範囲 |
| `RequestBuffer::Push`、`TileEventBuffer::Push` | 呼び出し側保存、明示的な overflow |

画像は左上が原点、x は右、y は下。Canvas 単位は画像ピクセルです。デスクトップ画面変換には既存の `ToScreen`/`FromScreen` を使います。Fit は全体、Fill は切り抜き、1:1 は画像ピクセルと ImGui 座標単位を 1:1 対応（必ずしも物理表示ピクセルとは一致しません）。Clamp は小さい画像を中央に、大きい画像は辺まで制限します。カーソル中心の zoom は端 clamp が必要になるまで保持します。resize は manual 以外の mode を再 fit します。矩形・lasso 選択は既存の `CanvasSelection` が対応します。

`ResolveImagePlacement` は利用可能領域の左上を原点とするローカル座標を使います。Fit は中央配置と全 UV、Fill は全表示領域と中央 crop UV、Stretch は両方の全域を使います。texture や renderer は所有せず、非正数・非有限・描画不能な geometry に対して `valid=false` を返します。

Theme、locale、semantic 公開は `ComponentOptions` で渡します。描画専用 overlay は対話的 object node を生みません；編集対象はホストが定義します。Step の矢印は disabled を飛ばし、native の Tab/Shift-Tab と実行は維持されます。Splitter は矢印による resize も対応します。Reduced Motion は既存の静止表示を使います。新しい transition は導入しません。

`ResolveRightSidePanelLayout` は本文、常時表示される右端 handle、任意 panel の幅を返します。その順で描画し、同じ利用可能寸法を `RightSidePanelHandle` に渡します。`N` キーや他のショートカットを `toggleRequested` に変換する範囲はホストが決めるため、無関係な editor や文字入力のショートカットを奪いません。閉じた状態でも handle は残り、panel は本文を覆うのではなく幅を縮めます。

## Gallery

**Generic Workspace**、**Feedback / States**、**Preview Tiles** を開きます。workspace は navigation、toolbar、画像、overlay、選択、通知、状態表示を組み合わせ、既存の手続き生成 texture を使います。`--verify-workflow --output <directory>` は公開 IO 確認と 36 のページ/テーマ組み合わせ、加えて狭幅日本語・disabled 例 2 枚を capture します。native GPU capture は native OS/IME やスクリーンリーダー試験とは別です。
