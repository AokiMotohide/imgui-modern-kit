# 設計

[English](architecture.md)

## 配置

ワークフロー部品は patterns の拡張です。画像・preview 部品は Editor Core に属し、Canvas・Selection・Splitter を再利用します。基本の `imkit` は Editor Core に依存しません。公開契約は [汎用ワークフロー部品](workflow-components.ja.md) を参照してください。

横断的なデザインシステムの追加、明示的な semantic frame、locale、provider 契約は [デザインシステム刷新](design-system.ja.md) に記載されています。`MakeTheme`／`ResolveTheme` を使う場合、従来 palette/metrics は新 semantic token の描画用導出値として扱われます。

Precision Layers は semantic なデザイン、native な動作、ホスト state を分離します。

## プラットフォーム境界

core は platform・renderer 非依存を維持します。公式 Dear ImGui backend をコンパイルするのは Gallery host のみ（Windows は GLFW/OpenGL3、macOS は GLFW/Metal）です。任意の `preview_opengl3`・`preview_metal` target は明示的な off-screen GPU リソースのみを所有し、Context・device・command buffer・submission はホスト所有です。`accessibility_win32`・`accessibility_macos` アダプタは公開された semantic snapshot をコピーし、共通の `NativeActionSink` 経由で操作要求を返します。

| レイヤー | 責務 |
|---|---|
| `version.h` | 対応版を明示する guard。暗黙の跨版 ABI 保証なし |
| `theme.h`、`theme.cpp` | 名前付き preset の列挙、コピー可能な palette/metrics/fonts/motion、決定的な style 導出、入れ子 RAII |
| `native.h` | ホストの公開ヘッダーから import した正確な overload 群 |
| `widgets.h`、`widgets.cpp` | 既存6関数、pointer Selectable、公開 DrawList による選択/tree/tab マーク |
| `components.h`、`components.cpp` | 小さな native 合成。theme/animation は任意・明示的に渡す |
| `node_editor.h`、`node_editor.cpp`、`node_layout.cpp` | graph snapshot、容量有限な request、決定的な layout。graph 保存・評価は持たない |
| Gallery host | Context、fonts、GLFW/OpenGL、画像キャプチャ、代表入力 |

`imkit` は自身の実装だけをコンパイルします。Dear ImGui をコンパイルし、backend をリンクし、Context を初期化し、font を探し、設定を永続化し、worker を生み出したりしません。公開ラッパーは native の Begin/End、focus、callback、disabled、clipping、ID の契約を維持します。装飾は別 item を提出せず、複合部品は native group を使います。

## Theme と state

グローバルな Theme registry はありません。`ThemePresets()` は不変の列挙 metadata、`MakeTheme()` はコピー可能でホスト所有の値を返します。どちらにも現在の選択は保持されません。`FontSet` は非所有参照を含む。`AnimationState` はホストが明示的に寿命を管理する固定容量の保存です。scope はその Context が破棄される前に終了してください。静的 SDK は、利用者が先に作成した ImGui target に import interface adapter 経由で結合します。

`window_frame.h` も同じ境界を守ります。style、寸法、feature、content、state は明示的な値です。文字列/span は 1 回の描画中だけ借用されます。core は配置の計算、current Dear ImGui context への描画、型付き request の返却だけを行い、native window や platform 入力を所有しません。`window_frame_win32`・`window_frame_macos` は `HWND` または Cocoa window を借用する別 target で、対応 platform だけで build・export・install され、`imkit::imkit` に Windows/Cocoa 依存を加えません。

## 拡張方針

overload を追加するのは、固定の公開签名と比較し、default と戻り値の语义を維持したときだけです。`tools/generate_api.py` で API 対応表と compile/link fixture を再生成してください。対応していない新しい Dear ImGui 版への対応には、単なる version guard の緩和ではなく、adapter/style への意識的なレビューが必要です。描画寸法は theme・文字サイズから導出し、state は容量を制限し、native 編集を維持してください。ホスト固有のデータや service をこの library には持ち込みません。

## Editor module の所有権

Node Editor は独立した任意 target です。1 frame の間 graph snapshot を借用し、容量有限な編集 request を返します。動的 socket policy、互換判定、revision 受理、model 変更、preview 計算、Undo、永続化はホスト所有です。Material Graph companion は GUI 統合のサンプルであり、renderer や shader system ではありません。

Editor Core、Video、CG は非所有の provider view を消費し、固定バッファの event を生成します。編集された scene/media データ、選択、Undo、worker、Context は所有しません。明示的に生成した任意の OpenGL3 preview object は、自身の graphics リソースのみを所有します。Context と GL 関数表はホストから来ます。詳細は [Editor Suite](editor-suite.ja.md) を参照してください。

CG の変換は任意方向の非均等な scale に対して rotation、scale、上三角の shear を保持します。Scale イベントはアフィン成分を明示的に持ち、ホストは scale とまとめて適用します。両 preview 経路は同じ完全な線形変換と逆転置 normal を使います。
