# ノードエディター（開発中API）

独立した `imkit::node_editor` targetを追加しました。`imkit::editor_suite`からも利用できます。
公開ヘッダーは `<imkit/node_editor.h>`。対応環境は既存と同じC++20 / Dear ImGui 1.92.9b dockingです。

独自実装であり、imgui-node-editorを取得・リンクしません。同ライブラリはAPIと操作の参考です。
第三者のコードや画像を追加していません。独立デモだけがインストール済みWindowsのSegoe UIを
任意に読み込みます。フォントの再配布やライブラリ側での読み込みは行いません。

## 基本的な利用

ホスト所有の `EditorState`、非所有の `GraphView`、操作出力用の `RequestBuffer` を渡します。
`BeginEditor → DrawLinks → BeginNode/EndNode → NodePalette → EndEditor` の順で描画します。
ノード本体には通常のImGuiウィジェットを置けます。`DrawNodes` を使えば、折り畳み・低倍率時の
詳細省略を含めた標準の描画ループを使えます。直接描画する場合は `collapsed` と `detailZoom` を
見て本体の描画を省略してください。[最小コード例](node-editor.md#first-frame)

`BeginEditor` の戻り値が非表示でも `EndEditor` は必要です。`BeginNode` はtrueのときだけ
`EndNode` と対応させます。`BeginPin` と `EndPin` も明示的なframeを受け取り、グローバルな
current-editorは使いません。frameのコピーやノードscopeの入れ子は行わないでください。

Node/Pin/Link/GraphのIDは別型の64bit値です。0は予約値で、同一グラフ内のIDは一意にします。
座標・寸法はグラフ単位。ポートのoffsetは指定辺に沿った距離です。グラフのspan、文字列、
style参照はサイドパネル・独立プレビューも含む描画中に有効な状態を保ち、callbackから
再確保しないでください。複数ビュー・ContextではEditorStateを分けます。

`EditorState::Reserve` は任意の事前確保であり、ノード数の上限ではありません。
大きな操作バッファは `std::vector<EditRequest>` などでヒープに置いてください。

## 整列API

以下はImGui Contextなしでも呼べます。配置結果をまとめて適用し、Undo一回分にできます。

| API | 機能 |
|---|---|
| `AlignNodes` | 左・中央・右・上・中央・下。選択範囲、基準ノード、指定矩形を基準にする |
| `DistributeNodes` | 水平・垂直方向の中心間隔、または辺の間の余白を均等化する |
| `ArrangeNodes` | 接続関係に沿って左右／上下に配置する。循環部分も決定的な順序で扱う |
| `SnapNodesToGrid` | 指定グリッドへ位置を合わせる |
| `FitGroupToContents` | 内容と余白に合うグループ枠を返す |
| `FrameNodes` | 表示位置・倍率だけを変更する。空の選択は全体表示 |
| `TraceNodes` | 循環に対応した上流／下流のノード一覧を返す |

`AlignNodes` 等は変更された位置だけを返します。容量不足、不正な寸法、重複ID、消失ID、
包含関係の不正では、出力に一部だけを書き込みません。グループの移動ではロックされていない
子孫の相対位置を維持します。親子を同時に選択しても二重移動しません。

ロックされた部分は動かさず、等間隔配置では固定された区間の境界として扱います。
両端ノードは保持します。範囲がノード幅の合計より狭ければ余白が負になる場合があります。
自動配置は親ごとに計算し、固定された兄弟との重なりを避けます。一般的な制約ソルバーや
最適配線探索ではありません。明示的な呼び出し以外で既存の配置を変えません。

## 編集とホスト側の責務

出力は `operationSize` 件を一つの操作として扱い、graphとrevisionを確認して一括適用／拒否します。
連続操作はBegin/Update/Commit/Cancelです。Begin/Updateは下書き通知として扱い、永続モデルと
revisionはCommitで変更してください。ドラッグ中の下書き表示はエディター自身が行います。
Cancelは下書きを戻します。終端通知がバッファに入らなかった場合は保持し、ホストが出力を
消費・拡大した後に再送します。単発操作の容量不足はfalseまたはoverflowとして確認できます。

Undo履歴、クリップボードの内容、ファイル保存、評価処理、worker、texture、Context、backendは
ホスト側の責務です。`CaptureView` / `RestoreView` は保存可能な値を扱い、形式を強制しません。

接続の方向、存在、ロック、重複、接続数はUIで確認します。その後 `canConnect` が型変換、
循環、同じノードへの接続などを判断します。callbackがない場合、非0の型IDは一致が必要で、
0は型未指定です。循環は禁止しません。出力を分岐させる場合は `multiple=true` にしてください。

`parent` は表示上の包含、`childGraph` は階層グラフです。ホストはEnterGraph要求を受けて
描画するグラフを切り替え、エディターはグラフごとの表示位置と選択を保持します。
公開ポートの `internal` は内部ポートとの対応です。グループ化・解除・公開・テンプレートは
ホストに対する要求であり、独自のモデルへの移行を強制しません。

`InsertNode` は適合する入出力を探し、既存の配線一本を二本にする要求を一件で返します。
その要求のfrom/toは挿入ノードの入力／出力、linkは元の配線です。コピー・切り取り・貼り付け・
複製・グループ化は選択ノードを対象とします。ReorderPinのindexはノード内の0始まりの順序、
HidePinは1が非表示／0が表示です。ExposePropertyはtypeにプロパティID、indexに公開1／非公開0を
渡します。Renameは終端を除き127 UTF-8 byteまでの文字列を要求自身が保持します。

独立デモには、通常のホストモデルを使った追加・複製・削除、Undo/Redo、グループとサブグラフの
境界、解除、独立コピーとしてのテンプレート配置を実装しています。

## プレビュー・外観・操作

全ノードでプロパティ下部に `Preview` を配置できます。texture参照またはサイズと対話可否を
受け取るcallbackを渡します。callbackは領域内に描画し、対話可否を尊重してください。
画像・波形・文字・表・3D描画結果などの生成はホストが行います。

未提供、更新待ち、古い結果、エラーは別状態です。折り畳み、リサイズ、出力切替、拡大、固定、
二出力の比較を扱います。画面外のノードも含め、対象ごとに `DrawDetachedPreviews` を呼ぶことで
独立ウィンドウを維持します。textureはホストがDrawDataを消費するまで有効にしてください。

PreviewDemandはUIピクセルでのサイズと可視性を通知します。そのframeに通知がない出力は
要求されていません。描画解像度、DPI換算、更新頻度、スケジューリングはホストが決めます。
ノードの寸法は明示値なので、中に置くUIが収まる大きさを指定します。

`MakeNodeStyle` はThemeの意味色・密度・角丸・線幅・motionを継承します。エディター全体、
個別ノードのstyle、ポートの色・形状・辺、個別配線の色・線種・太さを変更できます。
本体には標準ImGui入力を使えます。BackgroundImage/Annotationはホスト提供の背景装飾です。
リンクの流れ表示は実行エンジンではありません。

NodeInspector / ExposedProperties / NodeSearch / Diagnostics / LayoutToolbar / Breadcrumbsは、
ホストのサイドパネルにも配置できます。標準操作はヘッダードラッグ、中ボタンパン、wheel拡縮、
Fで表示合わせ、Tabで追加、Ctrl選択切替、Shift追加選択、任意の投げ縄、Altドラッグで配線切断、
Alt＋入力ポートドラッグで付け替えです。キー割当は変更可能です。文字入力・popupが優先され、
readOnlyではグラフ編集だけを抑止します。実行・debug操作はホストが能力を指定した場合に表示します。

## デモと確認範囲

`imkit_node_editor_gallery` をビルドし、`catalog/Debug` 内の実行ファイルを起動します。
`--smoke [出力PNGの絶対パス]` でホスト編集・Undo・サブグラフ解除とnative描画5frameを確認し、
必要なら実backbufferを保存します。captureはbuild配下に置き、製品画像として自動追加しません。
既存Galleryの実装ファイルは変更していません。

直接テストは `imkit.node_editor`、公開ヘッダーとlinkは `imkit.node_editor_api_compile` です。
[API一覧](node-editor-api.json)は `python tools/generate_node_api.py` で独立に再生成できます。
既存APIの生成物を巻き込まずに更新します。

デザイン段階のAPIとして、固定の性能合格条件や製品間の網羅的互換保証は設定していません。
native OS/IME、実機DPI、支援技術、Release性能、install済みSDKの配布確認は、これらの
開発用テストで確認したことにはなりません。[English](node-editor.md)
