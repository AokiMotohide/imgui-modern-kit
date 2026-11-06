---
title: "最初のアプリを作る"
description: "このチュートリアルでは、ImKitの部品を描く最小のプログラムを作ります。トグル、状態バッジ、ボタンを1つのウィンドウに並べ、1フレームをCPUだけで実行します。ウィンドウ、GPU、画像ローダーは要りません。"
---

このチュートリアルでは、ImKitの部品を描く最小のプログラムを作ります。トグル、状態バッジ、ボタンを1つのウィンドウに並べ、1フレームをCPUだけで実行します。ウィンドウ、GPU、画像ローダーは要りません。終わるころには、Dear ImGuiのContext、テーマ、アプリ自身の状態が、それぞれどこにあるかが分かります。

![GalleryのStart画面。同じ部品がImKitのテーマで変わる](/imgui-modern-kit/docs-images/v4-start-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-start.gif)

## 始める前に

- C++20のツールチェーンと、CMake 3.20以降。
- 固定コミット `367b2c24f399988ddafc0bb4628da0106bcc09be` のDear ImGuiを、CMakeのターゲットとしてビルドしたもの（[導入ガイド](../../getting-started/)を参照）。
- `imkit::imkit`としてリンクしたImKit。

## 手順1: Contextを作る

ImKitは、Dear ImGuiのContextを作りません。アプリが、起動時に1回作り、終了時に破棄します。

```cpp
#include <cstdio>
#include <imgui.h>
#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = {640, 480};    // 実際のアプリでは、ウィンドウから受け取る
    io.DeltaTime = 1.0f / 60.0f;    // これも、フレームタイマーから受け取る
    io.IniFilename = nullptr;       // 設定ファイルを使わない
    io.Fonts->AddFontDefault();

    // ここではレンダラーのバックエンドが動かないので、フォントアトラスを自分で構築する。
    // 実際のバックエンドは、これを代わりに行う。
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
```

`io.IniFilename = nullptr`を指定すると、Dear ImGuiは`imgui.ini`を読み書きしません。

## 手順2: テーマを作る

テーマは、ただの値です。`MakeTheme`はコピーを返し、それをアプリが持ちます。ImKitは、「現在のテーマ」をグローバルには持ちません。

```cpp
    imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
```

![Galleryでテーマを選ぶと、すべての部品の配色が変わる](/imgui-modern-kit/docs-images/v4-themes-poster.png)

[操作GIFを開く](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-themes.gif)

`ThemePreset::Ocean`は、内蔵のプリセットの1つです。一覧と、独自のテーマの作り方は、[テーマ](../../features/themes/)を参照してください。

## 手順3: 自分の状態を持つ

部品が編集する値は、アプリのものです。

```cpp
    bool notifications = true;
    int counter = 0;
```

## 手順4: フレームを描く

有効なContext上で、`NewFrame`より前にテーマを適用し、`NewFrame`と`Render`の間で描きます。

```cpp
    imkit::ApplyTheme(theme);       // ImGui::NewFrame() より前
    ImGui::NewFrame();

    ImGui::SetNextWindowPos({20, 20});
    ImGui::SetNextWindowSize({320, 200});
    if (imkit::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize)) {
        imkit::Toggle("Notifications", &notifications);
        imkit::StatusBadge(notifications ? "Active" : "Off",
                           notifications ? imkit::StatusKind::Success : imkit::StatusKind::Neutral);
        if (imkit::ActionButton("Add one", imkit::ActionVariant::Primary)) {
            ++counter;              // ボタンが押されたフレームで実行される
        }
        ImGui::SameLine();
        ImGui::Text("Count: %d", counter);
    }
    imkit::End();

    ImGui::Render();
```

| 呼び出し | 動作 |
| --- | --- |
| `imkit::Toggle` | アプリの`bool`を読み書きし、変わったフレームでtrueを返す |
| `imkit::StatusBadge` | 状態のチップを描く。戻り値はない |
| `imkit::ActionButton` | 押されたフレームでtrueを返す。何をするかは、アプリのコードが決める |
| `imkit::Begin` / `imkit::End` | Dear ImGuiの`Begin` / `End`。`Begin`がfalseを返しても、`End`は必ず呼ぶ |

## 手順5: 結果を読み、後始末をする

```cpp
    const ImDrawData* draw = ImGui::GetDrawData();
    std::printf("draw lists: %d, vertices: %d, indices: %d\n",
                draw->CmdListsCount, draw->TotalVtxCount, draw->TotalIdxCount);

    ImGui::DestroyContext();
    return 0;
}
```

ビルドして実行します。頂点数とインデックス数が0でなければ、フレームは描画されています。

```text
draw lists: 1, vertices: 418, indices: 1281
```

数値は、既定のフォントによって変わります。このプログラムにはバックエンドがないので、画面には何も出ず、ボタンも押せません。Context、テーマ、部品が連携して動くことの確認です。

## ウィンドウに表示する

1フレームだけの処理を、アプリのループに置き換え、バックエンドに`ImGui::GetDrawData()`を描画させます。Galleryは、GLFWとOpenGL3またはMetalで、これと同じことをしています。`examples/gallery/main.cpp`を読んでください。ビルドと実行の方法は、[Galleryガイド](../../guides/gallery/)にあります。

## よくある間違い

| 間違い | 結果 | 対処 |
| --- | --- | --- |
| `NewFrame`のあとに`ApplyTheme`を呼ぶ | 現在のフレームは、前のスタイルで描かれる | `ImGui::NewFrame()`より前に呼ぶ |
| `imkit::End()`を呼ばない | ウィンドウのスコープが開いたままになる | `Begin`1回につき、`End`を1回呼ぶ |
| ImKitがデータを変えると思い込む | ポインターで渡した値のほかは、何も変わらない | 返された結果を、アプリのモデルに反映する |
| `ThemeScope`が生きているうちにContextを破棄する | スコープのデストラクターでアサートする | `DestroyContext()`の前に、スコープを終える |

## 次に読む

- [設定画面を作る](../settings-screen/): ImKitの部品でフォームを作ります。
- [ノードエディタを作る](../node-editor/): グラフを描き、編集の要求を受け取ります。
- [タイムラインを作る](../timeline/): トラック、クリップ、編集操作です。
- [カスタムコンポーネントを作る](../custom-component/): 独自のテーマ付きコントロールです。
