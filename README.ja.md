# ImKit

[English](README.md) · [文書一覧](docs/README.ja.md) · [導入ガイド](docs/getting-started.ja.md) · [Gallery](docs/gallery.ja.md) · [v3 移行](docs/migration-v3.ja.md) · [Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)

ImKit は Dear ImGui 向けの C++20 静的ライブラリです。Dear ImGui のウィジェットに統一した、テーマ化できる見た目をつけ、少量の合成コントロール（テーマ付きボタン、トグル、検索コンボ、ワークフローパネル、ノードエディターなど）を足します。Dear ImGui コンテキスト、レンダラ、フレームループ、アプリケーション状態を所有することはありません。それらはあなたのアプリに残ります。

MIT ライセンス · Dear ImGui 1.93.0 WIP (docking) · Windows x64/Arm64 · macOS arm64/x86_64 / Universal 2

## まず試す

[v3.1.0 のリリースパッケージ](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.1.0)をダウンロードして展開し、ネイティブ Gallery を実行します。

- Windows: `bin/imkit_gallery.exe`
- macOS: `imkit_gallery.app`

専用の Node Editor Gallery も同梱しています。Gallery はアプリがリンクする同じライブラリをライブカタログとして開いたもので、デモ専用の代替ウィジェットは中に入っていない、と申えます。

自分でビルドするなら:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

## 既存の Dear ImGui アプリに追加する

既存の Dear ImGui target を指定して、1 つの import target をリンクします。

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)  # 対応する既存 Dear ImGui target
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

フレームループでは `NewFrame` 前にテーマを適用し、`Begin`/`End` の中で描画します。

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
imkit::ApplyTheme(theme, 1.25f);  // ImGui::NewFrame() より前

// NewFrame()/Render() 内:
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled);
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
}
imkit::End();  // Begin と必ず対にする
```

コンテキスト、backend、レンダラ、font atlas、値、フレームループはそのままです。ImKit は現在の生存中 Context だけに対して動きます。

## タarget ごとに何を追加するか

| Target | 追加されるもの | あなたに残るもの |
|---|---|---|
| `imkit::imkit` | テーマ、コントロール、アイコン、ワークフロー・シェル部品 | Context、フレームループ、値、フォント |
| `imkit::node_editor` | canvas、ソケット、リンク、layout、minimap、検索、編集要求 | graph model、検証、履歴、評価 |
| `imkit::editor_core` | canvas、選択、共通のエディター契約 | document、command |
| `imkit::video` / `imkit::cg` / `imkit::editor_suite` | Timeline、Inspector、階層、curve、編集 surface | media/scene data、Undo、永続化 |
| `imkit::preview_opengl3` / `imkit::preview_metal` | 明示的に生成する preview レンダラ | GL Context / device、command buffer、texture の寿命 |
| プラットフォーム `window_frame` / `accessibility` target | ネイティブ frame と semantic ブリッジ | native window、公開された semantic tree |

Gallery は、自分の実行ファイル用に GLFW とレンダラ backend をリンクするだけです。`imkit::imkit` をリンクしても、あなたのアプリには加わらないです。

## 所有境界が設計である

ImKit は Dear ImGui コンテキスト、backend、レンダラ、プラットフォーム window、font atlas、texture、編集する data、Undo 履歴、永続化、worker を生成・所有しません。公開 Dear ImGui の ID、focus、navigation、callback、clipping、テキスト編集は通常動作を保ちます。これにより ImKit は、競合するフレームワークではなく、あなたのツールの中に入る library になります。

## 画面が示しているもの

以下の各アニメーションはネイティブ Gallery からの実キャプチャ（960×540）で、再描画や別製品の写真ではありません。[ショーケース（MP4）全編](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v3.1.0/imkit-v3.1.0-showcase.mp4)。

### 概要

![ネイティブ Gallery の概要](docs/images/v3-overview.gif)

Start 画面は主な経路を列挙し recipe マップへリンクしてから、live 比較、components、workflow、Timeline の実例へ開きます。

### Node Editor

![動的ソケットとリンクを持つ Node Editor](docs/images/v3-node-editor.gif)

型付き接続、動的ソケット、inline 値、pan/zoom、minimap、検索、layout、preview はすべてアプリデータから独立しています。同梱の companion は Material Graph の mock で、レンダリングと評価はホスト側の仕事です。

### ワークフローと進捗

![ワークフロー feedback と円形 progress](docs/images/v3-workflow-progress.gif)

framework を採用せずに、filter、notification、step navigation、dialog、state、サイドパネル、確定または未確定の progress を組み合わせられます。

### Timeline

![Timeline 操作と Undo](docs/images/v3-timeline.gif)

任意のエディター module は、拡張可能な Timeline、外部 drop preview、host toolbar、Inspector、canvas、gizmo、curve、階層、ホスト所有の Undo/Redo を扱います。

### テーマ

![Theme と Default Dear ImGui の比較](docs/images/v3-theme-comparison.gif)

13 個の名前付き preset に semantic color、density、contrast mode を加え、周囲の Dear ImGui style と動作を保ちつつ一貫して適用できます。

## 互換性

v3 の ABI は Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`（1.93.0 WIP）に固定しています。CI は Windows x64/Arm64 と macOS arm64/x86_64、macOS Universal 2 package を build・test・package し、macOS arm64 gate で Metal Gallery の smoke 実行を行います。署名 secret が無い場合の package は未署名・未 notarize です。

自動テストは build、focused test、public-IO / GPU の smoke 確認までです。物理の pointer/keyboard 入力、native IME、実の screen reader、mixed-DPI、外部ホストの受入までは対象外です。それらの根拠を頼む前に [検証](docs/validation.md) と [依存関係](docs/dependencies.md) を読んでもらいたい。

## 文書

| 目的 | English | 日本語 |
|---|---|---|
| 目的別に文書を見る | [一覧](docs/README.md) | [文書一覧](docs/README.ja.md) |
| 各ページの読者・API・Gallery 経路 | [カタログ](docs/documentation-catalog.md) | [文書カタログ](docs/documentation-catalog.ja.md) |
| module 別 recipe と frame 内位置 | [実例とrecipe](docs/examples-recipes.md) | [実例とrecipe](docs/examples-recipes.ja.md) |
| 導入と最初の frame | [Getting started](docs/getting-started.md) | [導入ガイド](docs/getting-started.ja.md) |
| native Gallery とキャプチャ | [Gallery](docs/gallery.md) | [Galleryガイド](docs/gallery.ja.md) |
| コンポーネントとrecipe | [User guide](docs/guide.md) | [ガイド](docs/guide.ja.md) |
| Node Editor 統合 | [Node Editor](docs/node-editor.md) | [Node Editor](docs/node-editor.ja.md) |
| v3 の破壊的変更 | [Migration](docs/migration-v3.md) | [v3 移行](docs/migration-v3.ja.md) |
| 所有権とパッケージング | [Architecture](docs/architecture.md) · [Dependencies](docs/dependencies.md) | [設計](docs/architecture.ja.md) · [依存関係](docs/dependencies.ja.md) |
| 確認済み／未確認範囲 | [Validation](docs/validation.md) | [検証](docs/validation.ja.md) |

v3 の追加と移行点は [CHANGELOG.md](CHANGELOG.md) にまとめてあります。

## ライセンスと出典

ImKit は [Dear ImGui](https://github.com/ocornut/imgui) および Omar Cornut 氏と contributor が築いた明快さ・移植性・immediate-mode の思想への敬意を込めて開発しています。Dear ImGui の fork や代替ではなく、その優れた動作と所有境界を保ちながら機能を重ねる、独立した拡張 layer です。

ImKit は [MIT ライセンス](LICENSE) です。Dear ImGui、GLFW、任意の font 資産にはそれぞれの license が当てはまります。正確な revision、hash、notice は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) にあります。GIF とリリース MP4 は、ネイティブな ImKit Gallery / companion の backbuffer だけを使っています。
