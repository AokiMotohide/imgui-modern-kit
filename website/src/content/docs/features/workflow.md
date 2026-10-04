---
title: "Workflow部品"
---

## 用途

アプリ側で管理する処理に進捗、空・利用不可状態、通知、navigation、responsive toolbarを加える部品です。

## Galleryの画面

![進捗と状態表示を含むWorkflow部品の画面。](../../../assets/captures/v3-workflow-progress-poster.png)

Workflow — 進捗と状態の表示 · [Galleryの操作映像を開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v3-workflow-progress.gif)

## 最小描画例

```cpp
#include <imkit/workflow.h>

void DrawImportProgress() {
    const imkit::CircularProgressView progress{
        0.42f, "42%", "素材を読み込み中"
    };
    imkit::CircularProgress("asset-import", progress);
}
```

**例の種別:** 公開headerの型を使う完結した関数例です。アプリのImGuiフレーム中に呼び、表示文字列は描画が終わるまで保持します。

## アプリへ組み込む

`imkit::imkit`をlinkします。進捗値、取消、通知queue、texture、保存はアプリ側で管理します。

## 範囲

状態を描画する部品です。worker実行、完了判定、通知の保存は行いません。

## 関連APIとガイド

- [Workflowガイド](./)
- [Application shellガイド](../shell/)

---

## Ownership / 所有権

Workflow部品は複数画面の制作ツールで再利用できるUIです。`<imkit/workflow.h>`をincludeし、`imkit::imkit`へlinkします。描画はホストが作成したDear ImGui frame内で行います。data、provider、ID、UI選択、command dispatch、保存、worker、image texture、zoom stateはホスト所有です。非所有string/spanはcallback/draw期間だけ有効にします。部品は永続化、Undo、navigation policy、media decode、background workerを実装しません。

## API inventory / API一覧

公開APIにはProgress、step navigation、responsive toolbar/workspace、data table、image viewport、toast、empty/loading/error stateなどがあります。frame内では`imkit::StepNavigator(id, items, current, hostOwnedState, layout, options)`のように呼びます。戻り値は選択要求で、current値を自動更新しません。部品一覧とoverload条件は[`include/imkit/workflow.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/workflow.h)、[`tests/workflow_api_compile.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/tests/workflow_api_compile.cpp)、英語版の[API inventory](../../en/features/workflow/)で確認してください。

## Gallery

**Generic Workspace**、**Feedback / States**、**Preview Tiles**を操作できます。画面sourceは[`workflow_pages.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/workflow_pages.cpp)、page選択は[`gallery.cpp`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/gallery/gallery.cpp)です。同一libraryの公開APIを描画するsampleであり、Gallery固有dataや状態管理はconsumer向けserviceではありません。各moduleの利用手順は[実例recipe](../../guides/examples/)を参照してください。
