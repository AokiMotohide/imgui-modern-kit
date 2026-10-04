---
title: "Editor Suite"
---

## 用途

アプリが持つデータとresourceに、編集用のCanvas、Timeline、Video、CG部品を加えるときに使います。

## Galleryの画面

![ImKitのTimeline編集画面。](../../../assets/captures/v3-timeline-poster.png)

Editor Suite — Timelineと編集画面 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-timeline.gif)

## 最小描画例

```cpp
#include <imkit/video.h>
#include <imkit/editor_core.h>

void DrawTimeline(const imkit::video::TimelineProvider& provider,
                  imkit::video::TimelineState& state,
                  imkit::editor::Selection& selection,
                  imkit::editor::EventBuffer& events,
                  const imkit::Theme& theme) {
    imkit::video::Timeline("timeline", provider, state, selection, events, theme);
}
```

**例の種別:** アプリの状態を引数に取る関数例です。provider、選択、event buffer、Themeはアプリ側で用意し、eventは検証してから適用します。

## アプリへ組み込む

`imkit::editor_suite`または`imkit::video` targetをlinkし、providerの参照先は呼び出し中に有効に保ちます。

## 範囲

編集画面はmediaのdecode・再生、任意の3D描画、Undo、project file読み込みを担当しません。

## 関連APIとガイド

- [Editor API契約](../../api/editor-suite/)
- [Timeline編集ガイド](../timeline/)

---

ImKit のエディタスイートは、固定リビジョンの Dear ImGui 1.93.0 WIP docking 上で動作する、プロフェッショナルな制作ツール（DCCツール、動画編集ソフト、シーケンサ等）向けの高機能UIコンポーネント群です。

クリップのフェードやトランジション等の個別操作仕様は [タイムライン編集](../timeline/) を参照してください。

---

## 提供モジュール一覧 (Modules)

| CMake ターゲット | 公開ヘッダー | 主な役割とコンポーネント |
|---|---|---|
| `imkit::imkit` | `<imkit/imkit.h>` | 基本ウィジェット、Precision Layers テーマ、アイコン |
| `imkit::editor_core` | `<imkit/editor_core.h>` | 無限キャンバス、タイムルーラー、カーブエディタ、プロパティインスペクタ |
| `imkit::video` | `<imkit/video.h>` | マルチトラックタイムライン、モニタオーバーレイ、オーディオミキサー、カラーカーブ |
| `imkit::cg` | `<imkit/cg.h>`<br>`<imkit/preview.h>` | 3Dビューポートギズモ、シーングラフ階層、アニメーションドープシート、UVエディタ |
| `imkit::editor_suite` | 上記すべて | Core + Video + CG の全機能を包括（OpenGL は任意） |
| `imkit::preview_opengl3` | `<imkit/preview.h>` | ホスト側の OpenGL 3.3 コンテキストを借用した GPU オフスクリーンプレビュー |

---

## 所有モデルとイベント処理 (Ownership and events)

- **ホストがすべてを所有**: Dear ImGui コンテキスト、バックエンド、フォント、テクスチャ、プロジェクト生データ、Provider、UI状態、選択セット、Undo履歴、ワーカースレッドはすべてホストアプリケーションが所有します。
- **時間の単位（Tick）**: 時間は 1秒あたり 705,600,000 Tick（`int64_t`）の有理数 `FrameRate` で高精度に管理され、29.97 / 59.94 fps のドロップフレームタイムコードや負のプリロールに対応します。
- **プレビューコンポーネントの責務**: レイアウト計算、オーバーレイ描画、入力検出、操作リクエストの返却のみを担当します。動画のデコード・再生、3Dメッシュの評価、ファイル保存などは行いません。

詳細な型定義やイベント署名は [エディタAPI](../../api/editor-suite/) を参照してください。

---

## 主なUI機能 (Editor controls)

### 1. Editor Core
- 無限キャンバス（パン、ズーム、矩形/投げなわ選択）。
- タイムルーラー、マーカー編集、トランスポートバー。
- ベジェハンドル付きマルチキーフレームカーブエディタ。
- アセットブラウザ（グリッド/リスト表示、インラインリネーム、検索）。

### 2. Video Editor
- マルチトラックタイムライン（可変高トラック、リップル削除、スリップ/スライド編集）。
- ビデオモニタ（ホスト提供のテクスチャ表示、アスペクト比固定、グリッドオーバーレイ）。
- カラーコレクション（3-wayカラーホイール、RGBカーブエディタ、スコープ表示）。

![Native Video](/imgui-modern-kit/docs-images/editor-video-1.0.png)

### 3. CG Editor
- 3Dビューポートナビゲーション（Orbit、Pan、Zoom、Frustumカリング）。
- マルチオブジェクト変形トランスフォームギズモ（移動、回転、非均等スケール、シアー）。
- シーングラフ階層（ドラッグ＆ドロップによる親子付け替え・並べ替え）。
- UVエディタ（頂点/エッジ/面/アイランド選択、UDIMタイリング対応）。

![Native CG](/imgui-modern-kit/docs-images/editor-cg-1.0.png)

---

## プレビューのライフサイクル (Preview lifecycle)

- `preview_opengl3` などのオフスクリーンレンダラーを使用する場合、ホスト側で有効なOpenGLコンテキストを確立した上で初期化および破棄を行います。
- コンテキスト破棄前には必ず `Shutdown()` を呼び出し、GPUテクスチャリソースを安全に解放してください。

---

## Native Gallery での検証 (Native Gallery)

Gallery アプリケーションの **Editor Core**、**Video Editor**、**CG Editor** の各画面で、これらのエディタスイートの総合的な動作を検証できます：

```powershell
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

---

## 適用範囲と責務境界 (Scope)

エディタスイートは汎用UIフレームワークであり、特定のメディアフォーマット（MP4/H.264/ProRes等）のコーデックや3Dシーングラフの評価エンジンは含まれません。これらはすべてホスト側のビジネスロジックとして統合してください。
過去の内部検証データや操作設計は [検証記録](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/archive/editor-validation.md) および [Editor 2.0 リフレッシュ](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/archive/editor-refresh.md) に保管されています。
