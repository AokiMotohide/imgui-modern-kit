# Editor Suite 2.0

[日本語](editor-suite.ja.md)

[Timeline editing](timeline-editing.md) は独立したフェード、cut transition、トラック管理、選択・クリップボード操作を説明します。

ImKit は、固定した Dear ImGui 1.93.0 WIP docking commit 上で再利用可能な C++20 編集部品を提供します。

## Modules

| Target | Header | 役割 |
|---|---|---|
| `imkit::imkit` | `imkit/imkit.h` | Native wrapper、Precision Layers theme、アイコン |
| `imkit::editor_core` | `imkit/editor_core.h` | Canvas、time、curve、property/asset コントロール |
| `imkit::video` | `imkit/video.h` | Timeline、Monitor、Audio、Color |
| `imkit::cg` | `imkit/cg.h`、`imkit/preview.h` | Viewport、階層、animation、UV、DrawList preview |
| `imkit::editor_suite` | 上記の header | Core + Video + CG（OpenGL は必須ではない） |
| `imkit::preview_opengl3` | `imkit/preview.h` | 任意の host-context レンダラ |

## Ownership and events

Context、backend、font、texture、元データ、provider、UI state、選択、Undo、保存、worker はホスト所有です。span と UTF-8 ラベルは各呼出し中のみ有効な非所有参照です。StableId は一意の非ゼロ uint64_t。時間は毎秒 705600000 の Tick（int64_t）と有理数 FrameRate を使い、負の pre-roll と 29.97/59.94 drop-frame timecode に対応します。時フィールドの解析は 2 桁を受け入れます。

Preview 部品は layout・overlay・入力 UI・request のみを所有します。画像／動画 texture、media の decode/capture、CG 描画と FBO resize、frame clock、device、native window、worker、保存はホスト所有です。Video Monitor は現在の frame で渡された texture と状態を表示するだけで、再生・seek は行いません。

連続編集は revision・元値・提案値付きの Begin/Update/Commit/Cancel を返します。preview は確定値と分け、確定または外部変更で revision を進めます。容量不足は overflow として通知し、終端の意図を保持して再送します。関連対象は batch 全体の容量と lock を確認します。即時 context 操作は型付き Commit を返し、移動範囲・表示設定はホストの UI state を更新します。

可視 provider が定常フレームの作業量を制限します。編集に必要な選択対象全体・隣接要素の query を実行します。並べ替え・絞り込みでも ID を安定させます。キー预设はすべてホスト所有 Command binding 経由です。canvas 操作は focus されているウィジェット内で行われます。event payload、query の完全性、scratch 容量、overload は [API 契約](editor-api.md) を参照してください。

## Editor controls

Timeline 選択は選択済みクリップのドラッグで選択集合を維持します。Shift で追加、Ctrl で切り替えます。Move/Duplicate は選択全体を timeline zero で制限し、間隔を変更しません。`TimelineState::memberDrags` には全同行対象の transaction 容量を用意し、`TimelineProvider::selected` は画面外・リンク対象と lock 状態も返してください。ホストの衝突判定は区間の厳密な重複を検証します：端同士の接触は有効です。

Timeline の `SelectAll` は `editing.box` に全 Tick・全 track 高さの範囲を渡します。画面外も含む編集可能 clip ID を重複なく返してください。選択容量不足では元の選択集合を保持します。clip key が未選択なら Delete・Duplicate は選択 clip 全体の Begin/Commit を返します。Duplicate は選択集合の総時間幅だけ後方にオフセットします。選択された clip key が優先され、clip 本体クリックや clip 全選択は key 選択を解除します。衝突方針と Undo はホストの責任です。

Gallery のショートカット popup はエディターが使う同じホスト所有 binding を編集し、修飾キー・割当解除・共有コードにも対応します。変更は Gallery セッション中に有効で、保存は利用アプリケーションの役割です。预设は出発点であり、対象製品のキー設定全体の完全複製ではありません。

Core は canvas の pan/zoom/fit、box/lasso 選択、time ruler と編集可能 range、marker 編集、transport、複数 key 曲线と Bezier handle、property state と配列の並べ替え、数値の copy/paste、検索・改名可能な asset の grid/list 表示を提供します。

Timeline は右下に Fit、縮小、対数ズームバー、拡大を配置します。Ctrl+wheel はポインタ位置を固定してズームし、中ドラッグまたは Hand で pan します。overview range は可視区間の移動と両端の zoom を行います。ホストは既定動作を維持しつつ `TimelineState::options` で表示 tool と zoom 範囲を制限できます。

`TimelineProvider::externalDrops` は複数のホスト定義 ImGui payload 型を受け入れます。payload のメモリは callback 中だけ非所有参照で、delivery flag が preview と 1 回の確定 drop を区別します。`drawClipOverlay` は clip 内へアプリケーション固有の装飾を追加し、hit test や編集所有権を Timeline から移しません。任意の route preview は正確な候補 range、track 種別、非所有 label を返します。この場合 Timeline は行全体ではなく clip 形状の候補を描画し、確定時の計画規則も同じを使う責任はホストに残ります。

Video は可変高 role track、制限・source/target コントロール、関連 clip の移動・trim、split/ripple/roll/slip/slide/ripple-delete、snap target、transition の overlap/type/duration、caption、property key を提供します。Monitor overlay はホスト texture を使います。PCM bucket、envelope/mixer コントロール、CPU RGB/luma/vector scope は再生 engine と分離されています。three-way wheel と RGB CurveEditor 編集は合成ホストデータに適用し、ApplyColorCurves は alpha を保って RGB を評価します。

CG は camera/navigation/shading/overlay、制限付き階層選択と reparent/reorder、方向と pivot を反映する複数 object gizmo（明示的なアフィン scale/shear）、component/modifier row、graph/dope-sheet/strip コントロール、UV の vertex/edge/face/island 選択と変換（normalized/pixel/UDIM 座標、pin/seam overlay）を提供します。

## Preview lifecycle

DrawList preview は非所有 indexed mesh span をホストの triangle scratch へ投影し、depth sort します。Cube、Sphere、CameraPrimitive、LightPrimitive はホスト buffer に書き込みます。z-buffer は持たず、交差・循環する面や隠れた outline 辺は近似になります。両 renderer はアフィンの逆転置 normal 変換を使います。

OpenGL3 object は FBO/color/ID/depth texture、shader、VAO、buffer を所有します。ホストが Init/Resize/Render/Pick/Shutdown 用の GLFunctions と現在の OpenGL 3.3 context を渡します。その context を破棄する前に Shutdown を呼び出してください。デストラクタは GL を呼びません。Resize は texture ID を置き換え、Pick は完全な uint64_t ID（背景は 0）を返します。描画は GL binding、viewport、depth/cull/blend/scissor/polygon state を変更し、framebuffer/program/VAO が 0 になって終了します。後続 pass の状態はホストが復元します。

## Native Gallery

ページ 7/8/9 は公開 Core/Video/CG API を使い、サンプルホストへ編集を適用します。context menu は補助操作を公開し、range の端点と timeline track 名にも専用の menu があります。Color/Inspector 欄は狭い場合スクロールします。100k 切替は 256 track、100096 clip、100000 key を生成します。transition 履歴 menu はホスト Undo/Redo の例を示します。アイコンはホストが upload した atlas で、284 個（120 安定 ID ＋ 追加 164）です。

`imkit_gallery.exe --list-monitors` で画面を列挙し、`--monitor N` で capture を含む native window の配置先を指定できます。DisplayLink・複数 adapter デスクトップで便利で、システム表示設定は変更しません。

![Native Video](images/editor-video-1.0.png)
![Native CG](images/editor-cg-1.0.png)

## Scope

独立した開発用ノード module は [Node editor](node-editor.md) で説明しています。

これは編集 UI suite であり、media decoder/player、resampler、色管理 engine、UV unwrapper、IK/simulation/animation runtime、PBR/shadow renderer、ファイル形式 loader ではありません。native OS/IME と実プロジェクト統合は、公開 ImGui IO や GPU テストから推定しません。[検証](editor-validation.md) を参照してください。

[Editor 2.0 の移行と操作設計](editor-refresh.md) を参照してください。
