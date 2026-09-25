# Workflow components / Workflow部品

[English](workflow-components.md) · [Shell components](shell-components.ja.md) · [実例recipe](examples-recipes.ja.md)

## Ownership / 所有権

Workflow部品は複数画面の制作ツールで再利用できるUIです。`<imkit/workflow.h>`をincludeし、`imkit::imkit`へlinkします。描画はホストが作成したDear ImGui frame内で行います。data、provider、ID、UI選択、command dispatch、保存、worker、image texture、zoom stateはホスト所有です。非所有string/spanはcallback/draw期間だけ有効にします。部品は永続化、Undo、navigation policy、media decode、background workerを実装しません。

## API inventory / API一覧

公開APIにはProgress、step navigation、responsive toolbar/workspace、data table、image viewport、toast、empty/loading/error stateなどがあります。frame内では`imkit::StepNavigator(id, items, current, hostOwnedState, layout, options)`のように呼びます。戻り値は選択要求で、current値を自動更新しません。部品一覧とoverload条件は[`include/imkit/workflow.h`](../include/imkit/workflow.h)、[`tests/workflow_api_compile.cpp`](../tests/workflow_api_compile.cpp)、英語版の[API inventory](workflow-components.md)で確認してください。

## Gallery

**Generic Workspace**、**Feedback / States**、**Preview Tiles**を操作できます。画面sourceは[`workflow_pages.cpp`](../examples/gallery/workflow_pages.cpp)、page選択は[`gallery.cpp`](../examples/gallery/gallery.cpp)です。同一libraryの公開APIを描画するsampleであり、Gallery固有dataや状態管理はconsumer向けserviceではありません。各moduleの利用手順は[実例recipe](examples-recipes.ja.md)を参照してください。
