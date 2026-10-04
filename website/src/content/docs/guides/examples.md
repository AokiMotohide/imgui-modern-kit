---
title: "実例とレシピ"
---

Galleryを操作できる実例として使い、公開headerと実装へ進みます。実例はGallery host内で動作し、単独widgetや利用側アプリのdata/runtimeの代替ではありません。共通ルールは、module targetをlinkし、`ImGui::NewFrame()`から`ImGui::Render()`までの間に描画APIを呼び、ImGui Context、renderer、font、アプリ状態、保存、Undo、workerをホスト所有にすることです。

## Recipe

| moduleと用途 | include・CMake target | frame内の呼出し・状態所有者 | Gallery画面・source・次に読む文書 | 適用範囲と制約 |
|---|---|---|---|---|
| [Components](../../features/components/)：入力部品と設定行 | `<imkit/components.h>` · `imkit::imkit` | `imkit::ActionButton("Apply", imkit::ActionVariant::Primary);` 状態とIDはホスト所有です。 | **Components: Basic** · [`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp) · [Components recipe](../../features/components/) | Dear ImGui標準のitem/input契約を使います。viewとlabelは描画呼出し中だけ借用します。 |
| [Icon・Theme](../../features/themes/)：外観と記号の統一 | `<imkit/theme.h>`, `<imkit/icons.h>` · `imkit::imkit` | `imkit::MakeTheme(...)`でホスト所有Themeを作り、ホストのframe scopeで適用します。Theme寿命とfont atlasはホスト所有です。 | **Icons**、header内のAppearance · [`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp) · [Theme](../../features/themes/)、[Icon](../../features/icons/) | Theme選択と保存はホスト責務です。Icon atlasのuploadもホストが行います。 |
| [Workflow・Shell](../../features/workflow/)、[Shell component](../../features/shell/)：手順やアプリ枠を組み立てる | `<imkit/workflow.h>`, `<imkit/components.h>` · `imkit::imkit` | frame内で部品を描画し、返された要求をホストで処理します。provider、model data、UI状態、command dispatchはホスト所有です。 | **Generic Workspace**、**Feedback / States**、**Preview Tiles** · [`workflow_pages.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/workflow_pages.cpp)、[`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp) · [Workflow guide](../../features/workflow/) | UI部品を提供します。保存、background処理、navigation policy、アプリserviceは実装しません。 |
| [Node Editor](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/node-editor.ja.md)：graph snapshotを描画し編集要求を返す | `<imkit/node_editor.h>` · `imkit::node_editor` | snapshotとホスト所有request bufferを使い、frame内で`BeginEditor`、`DrawNodes`、`EndEditor`を呼びます。revision検証と要求適用はホストが行います。 | 別windowの **Node Editor Gallery** · [`examples/node_editor/main.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/node_editor/main.cpp) · [Node Editor統合](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/node-editor.ja.md) | graph保存、socket policy、評価、Undo、保存はホスト所有です。付属Material Graphは実例です。 |
| [Editor Suite](../../features/editor-suite/)：編集UIの再利用 | `<imkit/editor_suite.h>`または個別module header · `imkit::editor_suite` | frame内でホスト所有providerとevent bufferを使って描画します。変更適用、選択、media、Undo、clock、workerはホスト所有です。 | **Editor Core**、**Video**、**CG** · [`editor_workspaces.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/editor_workspaces.cpp) · [Editor Suite](../../features/editor-suite/)、[Editor API](../../api/editor-suite/) | Videoはmediaをdecode・再生せず、CGは汎用rendererではありません。容量・provider条件は各契約を確認します。 |
| [Timeline編集](../../features/timeline/)：fade、transition、複数clip移動 | `<imkit/video.h>` · `imkit::video`（または`imkit::editor_suite`） | ホスト所有`TimelineEditingProvider`を使ってVideoをframe内で描画し、eventをホストが検証・確定します。 | **Video** · [`editor_workspaces.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/editor_workspaces.cpp) · [Timeline編集](../../features/timeline/) | providerとevent storageは呼出し中有効にします。衝突判定、保存、Undoはホスト責務です。 |
| [Window frame](../../features/window-frame/)：Theme付きtitle areaとwindow操作要求 | `<imkit/window_frame.h>` · `imkit::imkit`。任意headerは`window_frame_win32.h`または`window_frame_macos.h` · 対応OS target | 明示style/content/stateを渡してframe内で描画し、返されたoperationをホストで実行します。native windowとevent loopはホスト所有です。 | **Frame Lab** · [`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp) · [WindowFrame guide](../../features/window-frame/) | core描画はnative windowを所有しません。Win32/Cocoa統合は任意targetです。OS入力は各OSでの受入が必要です。 |

## 最小の統合位置

次は呼出し位置の例であり、完成したアプリやbuild fileではありません。公開component APIだけを示しています。compile確認される実装例はGallery sourceを参照してください。

```cpp
// 対応Dear ImGui Contextとbackendはホストが初期化済み。
ImGui::NewFrame();
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary)) {
    host.ApplyPendingChanges();
}
ImGui::Render();
```

正確なoverload、default、ABI条件は[公開API対応表](../../api/native/)と[依存関係](../../platform/dependencies/)を参照してください。build可能なconsumer統合は[導入ガイド](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/getting-started.ja.md)と[`examples/consumer`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/consumer/CMakeLists.txt)を使います。consumer targetはcompile/link smoke、Galleryは操作できる学習例です。
