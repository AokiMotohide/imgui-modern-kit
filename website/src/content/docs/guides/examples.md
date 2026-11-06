---
title: "実例とレシピ"
description: "作りたいものの分野を探し、ヘッダー、CMakeのターゲット、ソースを読めるGalleryの画面をたどります。Galleryは、アプリがリンクするのと同じ公開部品で動いているので、そのソースは、動く参照実装になります。アプリ自身のデータや実行環境の代わりにはなりません。"
---

作りたいものの分野を探し、ヘッダー、CMakeのターゲット、ソースを読めるGalleryの画面をたどります。Galleryは、アプリがリンクするのと同じ公開部品で動いているので、そのソースは、動く参照実装になります。アプリ自身のデータや実行環境の代わりにはなりません。

どの分野にも、次の3つの規則が当てはまります。

1. モジュールのターゲットをリンクし、描画関数は、`ImGui::NewFrame()`と`ImGui::Render()`の間で呼びます。
2. Dear ImGuiのContext、レンダラー、フォント、状態、保存、Undo、ワーカーは、アプリが持ちます。
3. ラベル、span、ビューは、呼び出しの間だけ借用されます。結果は、戻り値か要求として返ります。

## レシピ

| 作りたいもの | ヘッダーとターゲット | Galleryの画面とソース | 読む文書 |
| --- | --- | --- | --- |
| 設定の行、ボタン、入力 | `<imkit/components.h>` · `imkit::imkit` | **Basic controls**、**Rows and palette** · `examples/gallery/gallery.cpp` | [基本コンポーネント](../../features/components/) |
| テーマ、ブランド色、独自の見た目 | `<imkit/theme.h>`、`<imkit/skin.h>` · `imkit::imkit` | **Theme Studio**、**Skins** · `examples/gallery/look_pages.cpp` | [テーマ](../../features/themes/)、[自分の世界観を作る](../../features/make-your-own-look/) |
| アイコン | `<imkit/icons.h>` · `imkit::imkit` | **Icons** · `examples/gallery/gallery.cpp` | [アイコン](../../features/icons/) |
| グラフ: ドーナツ、棒、ゲージ、スパークライン | `<imkit/charts.h>` · `imkit::imkit` | **Charts** · `examples/gallery/charts_page.cpp` | [チャート](../../features/charts/) |
| PTZカメラの操作、範囲スライダー | `<imkit/ptz.h>`、`<imkit/range_slider.h>` · `imkit::imkit` | **PTZ camera**、**Numbers and units** · `examples/gallery/ptz_page.cpp` | [PTZ操作と範囲スライダー](../../features/ptz/) |
| ステップ、タブ、階層、進捗、空の状態 | `<imkit/workflow.h>` · `imkit::imkit` | **Workspace**、**Progress and states**、**Workspace shell**、**Preview tiles** · `examples/gallery/workflow_pages.cpp` | [Workflow部品](../../features/workflow/)、[シェル部品](../../features/shell/) |
| トースト通知 | `<imkit/toast.h>` · `imkit::imkit` | **Toasts** · `examples/gallery/gallery.cpp` | [トースト](../../features/toasts/) |
| ノードグラフ | `<imkit/node_editor.h>` · `imkit::node_editor` | **Node Editor Gallery** · `examples/node_editor/main.cpp` | [ノードエディタを作る](../../tutorials/node-editor/)、[ノードエディタ](../../features/node-editor/) |
| タイムライン、カーブ、インスペクター、プレビュー | `<imkit/video.h>`、`<imkit/cg.h>` · `imkit::editor_suite`（または`imkit::video`、`imkit::cg`） | **Editor Core**、**Video**、**CG** · `examples/gallery/editor_workspaces.cpp` | [タイムラインを作る](../../tutorials/timeline/)、[エディタスイート](../../features/editor-suite/) |
| ライブショーの操作卓、キューリスト、LEDの道具 | `<imkit/show.h>` · `imkit::show` | **Show**グループ: **Live**、**Surface**、**LED**など · `examples/gallery/show_*_page.cpp` | [Showモジュール](../../features/show/) |
| テーマ付きのタイトルバー | `<imkit/window_frame.h>` · `imkit::imkit`。任意で`imkit::window_frame_win32`か`imkit::window_frame_macos` | **Window frame** · `examples/gallery/gallery.cpp` | [ウィンドウフレーム](../../features/window-frame/) |

## 短い例を2つ

フレームの中に部品を置く例です。

```cpp
// Dear ImGuiのContextとバックエンドは、アプリがすでに作ってある。
ImGui::NewFrame();
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary)) {
    // 返った要求を、アプリ自身の状態で処理する。
}
ImGui::Render();
```

すでに持っている値から、グラフを描く例です。

```cpp
const std::array<imkit::ChartDatum, 3> usage{{{"Render", 42}, {"Cache", 24}, {"Other", 6}}};
imkit::DonutChartView view;
view.segments = usage;
view.value = "72 GB";            // そのフレームの間だけ借用される
view.caption = "Used";
const int clicked = imkit::DonutChart("storage", view, {}, {&theme, &animation});
if (clicked >= 0) { /* クリックの意味は、アプリのコードが決める */ }
```

## 次に読む

- オーバーロードと既定値の正確な一覧: [公開API一覧](../../api/native/)。
- ビルドとABIの要件: [依存関係](../../platform/dependencies/)。
- ビルドできる組み込み例: [導入ガイド](../../getting-started/)と[`examples/consumer`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/consumer/CMakeLists.txt)。consumerのターゲットは、コンパイルとリンクの動作確認用で、対話的な例は、Galleryです。
