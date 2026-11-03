---
title: "シェル部品"
description: "ImKitは、ツールや制作環境の外枠（クローム）を構成するためのホスト主導型ビルディングブロックを提供します。"
---

ImKitは、ツールや制作環境の外枠（クローム）を構成するためのホスト主導型ビルディングブロックを提供します。
`AppBar`、`WorkspaceHeader`、`InspectorSection`、`AdvancedSection`、`BottomActionBar`、`DiagnosticsDrawer`、`ThemePicker` などのシェル部品が含まれます。

## 概要

各シェルコンポーネントは、テキストやコマンドの配列を借用（参照）してホストの現在状態を描画し、ユーザー操作が発生したときに操作リクエスト（アクション）を返します。

本ライブラリは、ドキュメント管理、Docking配置、設定の保存、Undo/Redo履歴、バックグラウンドワーカー、レンダラー、ImGui Contextなどを所有しません。すべての状態管理とライフサイクルはホスト側に委ねられています。

## シェルコンポーネント一覧

| コンポーネント | 主な用途 | 返却値 / 動作 |
|---|---|---|
| `AppBar` | 最上部のヘッダーバー（アプリ名、プロジェクト名、状態、コマンド一覧） | クリックされたコマンドのリクエスト |
| `WorkspaceHeader` | ワークスペース切り替えやビュー操作タブ | 選択されたタブのリクエスト |
| `InspectorSection` | プロパティインスペクター内の折りたたみ可能なグループ枠 | 開閉状態の維持・コンテンツ描画 |
| `AdvancedSection` | 詳細設定用のトグルセクション（通常は折りたたみ） | 開閉トグル要求 |
| `BottomActionBar` | 最下部のステータス行・実行ボタン群（OK/キャンセル/進行状況など） | 押下されたアクション |
| `DiagnosticsDrawer` | ログや警告、エラー情報を表示する下部ドロワー | 開閉要求・ログ描画 |
| `ThemePicker` | テーマプリセットを選択するUI | 選択されたテーマプリセット |

## コード例

```cpp
#include <imkit/imkit.h>
#include <vector>

// AppBarを描き、押された操作を受け取る
void RenderAppHeader(MyAppState& state, imkit::ToolbarState& toolbar) {
    const std::vector<imkit::Command> commands = {
        {1, "保存", "Ctrl+S"},
        {2, "エクスポート", "", !state.hasSelection},   // StableId, label, shortcut, disabled
    };

    imkit::AppBarView view;
    view.product = "MyStudio";          // アプリ名
    view.document = "scene_01.proj";    // プロジェクト名
    view.context = "READY";             // モード・バッジ
    view.status = "変更なし";               // 状態の文
    view.dirty = false;                 // 未保存の変更
    view.statusKind = imkit::FeedbackKind::Success;
    view.actions = commands;

    if (const auto action = imkit::AppBar("main_app_bar", view, toolbar)) {   // 押された操作のStableId。0なら何も押されていない
        state.Run(action);              // アプリが、その操作を実行する
    }
}
```

## テーマとフォント

- `ThemePicker` は単に選択された `ThemePreset` を返します。ホスト側は設定ファイル等にプリセットの安定IDを記録し、フレーム処理の外側で `MakeTheme` / `ApplyTheme` を呼び出して反映します。
- `imkit_copy_font_assets(target, destination)` を使用すると、バンドルされた Inter や Noto Sans JP フォントファイルをホスト環境へコピーできます。フォントアトラスのビルドとメモリ管理はホスト側が行います。
