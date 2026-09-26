# ImKit アプリの作り方

[English](build-first-app.md) · [文書索引](README.md) · [ImKit の仕組み](how-it-works.md)

これは codelab です。読み終えれば、テーマ付き ImKit コントロールを描画する Dear ImGui ホストアプリの全体像が手に入ります。リポの最小ホスト `examples/consumer/main.cpp` をもとに、CPU 埋め込みリソースを使い、GPU・画像ローダ・ファイル検索なしで構成を理解できます。

![ネイティブ Gallery 概要](images/gallery-overview.gif)

## 作るもの

Dear ImGui ホストアプリが：

- Dear ImGui コンテキストを作成し、I/O を設定する。
- テーマ付き `Theme` 値を 1 つ作成し、適用する。
- チルドウィンドウの中でいくつかのテーマ付き ImKit コントロールを描画する。
- コントロールに**あなたのコード**が反応する（`Toggle` はあなたの bool を読み書き、ボタンの結果はあなたが扱う）。

すべてがホスト所有です。ImKit は描画と報告だけ。所有モデルは [ImKit の仕組み](how-it-works.md) を参照。

## 必要なもの

1. Dear ImGui 1.93.0 WIP（固定 rev）をビルドできる C++20 トールチェーン。
2. CMake 3.20 以上。
3. 固定 commit `367b2c24f399988ddafc0bb4628da0106bcc09be` の Dear ImGui ソース。
4. ImKit ソースまたは SDK パッケージを、`imkit::imkit` ターゲットとしてリンク。

この文書を読むには GPU は不要です。以下の例は CPU 単独の smoke ホストで、リアルな Gallery は GLFW/OpenGL や Metal でウィンドウを描画します。

## ステップ 1 — Dear ImGui コンテキストの作成

ImKit はコンテキストを作らない。あなたが一度和作成し、終了時に破棄します。

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};   // 実アプリではウィンドウバックエンドから
    io.DeltaTime   = 1.f / 60.f;   // 実アプリではウィンドウバックエンドから
    io.IniFilename = nullptr;      // ホストが全状態を所有。設定ファイルなし
    io.Fonts->AddFontDefault();    // 文字が描けるようデフォルトフォントを追加
    // ...
}
```

ポイント：

- `CreateContext()` は一度だけ（起動時）、`DestroyContext()` も一度だけ（終了時）。
- `io.IniFilename = nullptr` にすると ImKit は設定ファイルを読み書きしない。全状態はアプリが正解。
- `AddFontDefault()` は Dear ImGui に組み込みフォントを与える。実アプリでは自分でアップロード。

## ステップ 2 — テーマの作成と適用

テーマは単なる値です。`MakeTheme` はあなたが所有するコピーを返し、`ApplyTheme` が現在の Dear ImGui コンテキストにインストールします。

```cpp
    // アイコン: CPU データは埋め込み。実アプリは各 atlas をアップロード・バインドする。
    imkit::IconAtlas icons;
    for (int size : imkit::IconPixelSizes) {
        const auto atlas = imkit::GetIconAtlasPixels(size);
        // 実アプリ: atlas.rgba を GPU テクスチャへアップロードし
        //     icons.SetTexture(size, texture);
    }

    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    imkit::ApplyTheme(theme, 1.0f);   // 現在コンテキストへ、NewFrame 前に適用
```

- `ThemePreset::PrecisionDark` は名前付き preset の一つ。`imkit::ThemePresets()` の値なら何でも可。
- `ApplyTheme` はスケール係数を第 3 引数に取る。`1.0f` がデフォルト。
- ImKit に「現在のテーマ」のグローバル保持はありません。どのテーマが有効かで、適用タイミングはホストが決めます。

## ステップ 3 — 最初の ImKit コントロールを描画

`imkit::Begin`/`imkit::End` は Dear ImGui のチャイルドウィンドウ `Begin`/`End` を `imkit` ネームスペースへ再エクスポートしたもの。描画コールのグループを括ります。内部でテーマ付きコントロールを呼び出します。各コントロールはホスト所有値を読み、あなたが扱う結果を返します。

```cpp
    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);   // このブロック内だけテーマが有効
        if (imkit::Begin("Settings")) {   // チャイルドウィンドウが表示中なら true
            bool enabled = true;           // あなたの state — ImKit が所有しない
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
            imkit::Icon(icons, imkit::IconId::Settings);  // テクスチャ未バウンドで空白
        }
        imkit::End();
    }
    ImGui::Render();
```

- `{&theme}` は 1 番目のメンバーがテーマの `ComponentOptions`。上書きなしなら `{}`。
- `Toggle` はユーザー入力であなたの `bool` を書き替え、状態変化時に `true` を返す。
- `StatusBadge` は描画専用のステータス chip。
- `Icon` はグリフのレイアウト領域を確保。テキストチャを未バウンドだと CPU ホストでは意図的に空白。

## 完全なプログラム

組み立てると：

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};
    io.DeltaTime   = 1.f / 60.f;
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefault();

    imkit::IconAtlas icons;
    for (int size : imkit::IconPixelSizes) {
        const auto atlas = imkit::GetIconAtlasPixels(size);
        (void)atlas;   // 実アプリ: アップロードし icons.SetTexture(size, texture);
    }

    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    imkit::ApplyTheme(theme, 1.0f);

    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);
        if (imkit::Begin("Settings")) {
            bool enabled = true;
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
            imkit::Icon(icons, imkit::IconId::Settings);
            if (imkit::ActionButton("Save", imkit::ActionVariant::Primary, {}, {&theme})) {
                // "Save" の意味を決めるのはあなたのコード。ImKit は押下を報告するだけ。
            }
        }
        imkit::End();
    }
    ImGui::Render();

    ImGui::DestroyContext();
    return 0;
}
```

## 実行

1. 今日は ImKit をリアルなウィンドウで確認できる**Gallery**をビルドして実行してください。このガイドの全コントロールを描画します。

   ```powershell
   cmake --preset windows-debug
   cmake --build --preset windows-debug --target imkit_gallery --parallel
   ./build/windows-debug/catalog/Debug/imkit_gallery.exe
   ```

   macOS では `macos-universal` preset（Release、arm64/x86_64）を使用。[Gallery](gallery.md) を参照。

2. 上記のコードそのものは最小ホスト `examples/consumer/main.cpp` です。**CPU smoke アプリ**で、`DisplaySize`・`DeltaTime` を手動設定し、ウィンドウバックエンドなしで 1 フレームを描画します。コンテキスト + テーマ + コントロールのパターンが動く最小のホストです。

## 何が起きているか

- ホストが Dear ImGui コンテキストを作成し、すべて（コンテキスト、フォント、state）を所有した。
- ImKit がテーマ値を適用し、あなたのフレームの中でテーマ付きコントロールを描画した。
- 全コントロールはあなたの値を読み、結果を返した。自動的な動作はなかった。
- 何の永続化もなく、ImKit が所有するものもなかった。

## よくあるミス

- **`NewFrame` 後にテーマを適用。** `ApplyTheme` は `NewFrame` 前に、現在のコンテキストで実行。
- **`imkit::End()` を忘れる。** `Begin` に対応する `End` がなければチャイルドウィンドウが壊れる。
- **ImKit がデータを処理してくれると期待する。** `Toggle`/`ActionButton`/drag は値や要求を返すだけ。**あなたが**モデルに適用する。
- **`Theme` ポインターを保持する。** `Theme` と `ComponentOptions.theme` は非保持。渡す値の寿命はホスト所有。
- **`Begin`/`End` を他のウィンドウスコープと混ぜる。** チャイルドウィンドウは対応させ、誤って別の `Begin`/`End` の中に入れない。

## 次に読む

- [設定画面の作り方](build-settings-screen.md) — コンポーネントでフルなフォームを組む。
- [ノードエディターの作り方](build-node-editor.md) — graph snapshot を描画し、編集を集める。
- [タイムラインエディターの作り方](build-timeline.md) — フェード、トランジション、グループ移動。
- [カスタムコントロールの作成](custom-component.md) — 自分のテーマ付きコントロールを書く。
