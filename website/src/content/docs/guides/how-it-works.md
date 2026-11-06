---
title: "仕組みと設計"
description: "ImKitは、Dear ImGuiの上に重ねて使うライブラリです。テーマ、部品、編集用のウィジェットを追加し、アプリ自身のDear ImGuiのフレームの中から呼び出します。正しく使うために知っておくことは、フレームがどう動くかと、だれが何を持つかの2点です。このページでは、その2点を説明します。"
---

ImKitは、Dear ImGuiの上に重ねて使うライブラリです。テーマ、部品、編集用のウィジェットを追加し、アプリ自身のDear ImGuiのフレームの中から呼び出します。正しく使うために知っておくことは、フレームがどう動くかと、だれが何を持つかの2点です。このページでは、その2点を説明します。正式な仕様は[アーキテクチャ](../../guide/architecture/)にあります。

## ImKitがすること、しないこと

| ImKitがすること | ImKitがしないこと |
| --- | --- |
| Dear ImGuiのウィジェットを、テーマ付きの版で包む（`imkit::imkit`） | Dear ImGuiのContext、バックエンド、レンダラー、フレームループの生成と所有 |
| 複合部品の提供。Workflow、グラフ、ノードエディタ、タイムライン、Showモジュール | メディアのデコードや再生、シミュレーションの実行 |
| 名前付きのテーマ、321個のアイコン、独自の見た目を作るためのトークン | データ、選択、Undo履歴、保存ファイルの保持 |

ImKitはDear ImGuiの仕組みを何も持たないため、すでにContextがあるアプリへ、そのアプリの作りを変えずに追加できます。

## Dear ImGuiの1フレーム

Dear ImGuiは即時モードのライブラリで、画面は毎フレーム作り直されます。

1. ループが`ImGui::NewFrame()`を呼びます。
2. アプリが、ウィジェットの関数を好きな順に呼びます。ImKitの関数も、その1つです。
3. アプリが`ImGui::Render()`を呼び、描画リストをバックエンドへ渡します。
4. 次のフレームは、空の状態から始まります。

ウィジェットのオブジェクトは、フレームをまたいで残りません。テキスト、選択、フラグはアプリのデータに置き、フレームごとにそれを読んで更新します。

## だれが何を持つか

![ImKitは描画と報告を担い、データ、状態、Dear ImGuiの準備はアプリが持つ](/imgui-modern-kit/docs-images/ownership-ja.svg)

| 対象 | 所有 | 補足 |
| --- | --- | --- |
| Dear ImGuiのContext、バックエンド、レンダラー、フレームループ | アプリ | ImKitは生成も破棄もしません |
| フォントとフォントアトラス | アプリ | `Theme::fonts`は借用ポインターです |
| 文書、シーン、グラフ、メディア、設定 | アプリ | ImKitはコピーも保持もしません |
| 選択、Undo/Redo、保存 | アプリ | |
| `Theme`の値と、適用するタイミング | アプリ | |
| 画面に表示される内容 | ImKit | 渡されたデータをもとに描きます |
| クリック、ドラッグ、接続の意味 | アプリ | ImKitは要求を返し、アプリが検証して適用します |

ImKitが返すものは、押されたボタン、ドラッグされたクリップ、接続の要求など、すべて「要求」です。意味を決めてデータを更新するのは、アプリのコードです。

## ImKitを使った1フレーム

Galleryの「Before / After」画面では、同じ値を、左は素のDear ImGui、右はImKitで編集しています。

![素のDear ImGuiとImKitが、同じ値を左右で編集する](/imgui-modern-kit/docs-images/v4-compare-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-compare.gif)

```cpp
#include <imkit/imkit.h>

// ループの外で1回
imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);

// アプリの状態
bool verbose = false;

// 毎フレーム
imkit::ApplyTheme(theme);          // ImGui::NewFrame() より前
ImGui::NewFrame();

if (imkit::Begin("Settings")) {
    imkit::Toggle("Verbose", &verbose);                        // アプリのboolを読み書きする
    if (imkit::ActionButton("Save", imkit::ActionVariant::Primary)) {
        SaveSettings(verbose);                                 // Saveの動作は、アプリが決める
    }
}
imkit::End();

ImGui::Render();
```

- `ApplyTheme`は、現在のContextのスタイルにテーマを書き込みます。`NewFrame`より前に呼びます。スケールのない値からスタイルを作り直すので、毎フレーム呼んでも、サイズは累積しません。
- ImKitのウィジェットは、アプリが持つ値（`&verbose`）を読み、結果（`ActionButton`の`bool`）を返します。
- `Begin`と`End`は、Dear ImGuiの規則に従います。`Begin`がfalseを返しても、`End`は必ず呼びます。

## まとめ

- ImKitは、アプリのDear ImGuiのフレームの中に描画し、Contextを生成も所有もしません。
- 毎フレーム、アプリのデータから描き直します。状態はすべてアプリが持ちます。
- ImKitは要求を返し、アプリのコードがそれを適用します。
- テーマは`NewFrame`より前に適用し、`Begin`と`End`を対にします。

## 次に読む

- [導入ガイド](../../getting-started/): プロジェクトに組み込みます。
- [最初のアプリを作る](../../tutorials/first-app/): 完全な手順を通して作ります。
- [アーキテクチャ](../../guide/architecture/): 所有の規則とモジュールの境界です。
