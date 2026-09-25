# ImKit

[English](README.md) · [文書目次](docs/README.ja.md) · [文書カタログ](docs/documentation-catalog.ja.md) · [実例とrecipe](docs/examples-recipes.ja.md) · [導入ガイド](docs/getting-started.ja.md) · [Gallery](docs/gallery.ja.md) · [Node Editor](docs/node-editor.ja.md) · [v3移行](docs/migration-v3.ja.md) · [Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)

**モダンなnative制作ツールを、より速く。** ImKit v3.1.0はDear ImGui向けのC++20静的UIライブラリです。Theme、制作向けcontrol、workflow／editor surface、ホスト所有のNode Editor、生成icon、native accessibility adapter、任意のOpenGL／Metal previewを、既存アプリケーションの所有権を保って利用できます。今回の更新では、目的から選べる英日文書カタログ、module別recipe、Galleryの学習案内を整えました。

MIT License · Windows x64/Arm64 · macOS arm64/x86_64/Universal 2 · Dear ImGui 1.93.0 WIP docking

## 30秒で試す

[ImKit v3.1.0](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.1.0)から環境に合うpackageを取得して展開し、`bin/imkit_gallery.exe`または`imkit_gallery.app`を実行します。専用のNode Editor Galleryも同梱します。

sourceからnative Galleryを起動する場合は次の3行です。

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

既存Dear ImGuiアプリケーションへの追加は、1 targetと1 theme scopeで始められます。

```cmake
set(IMKIT_IMGUI_TARGET host_imgui) # 互換する既存Dear ImGui target
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);

// 既存Dear ImGui frame内
imkit::ThemeScope themeScope(theme);
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled);
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
}
imkit::End(); // Beginの戻り値に関係なく必ず対にする
```

Context、backend、renderer、font atlas、値、frame loopは既存アプリケーション側に残ります。

## v3でできること

Galleryから実例を探し、対応するAPIと統合方法をすぐ確認できるようになりました。[文書カタログ](docs/documentation-catalog.ja.md)は読者と目的から読むページを案内し、[実例とrecipe](docs/examples-recipes.ja.md)はmodule、CMake target、frame内の呼出し、Gallery画面、source、制約を結び付けます。Start画面にもrecipe一覧への案内があります。

以下はすべて、実際のnative GalleryまたはNode Editor companionのbackbufferを960×540で記録したGIFです。再描画したmockや第三者製品の映像ではありません。[native showcase全編（MP4）](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v3.1.0/imkit-v3.1.0-showcase.mp4)も公開しています。

### V3 Overview

![ImKit v3 native Gallery overview](docs/images/v3-overview.gif)

Start画面の4つの目的別routeとrecipe案内から、比較、component、workflow、Timelineの実例へ進めます。

### ホスト所有のNode Editor

![動的socket、link、inline値、preview、minimapを備えたImKit v3 Node Editor](docs/images/v3-node-editor.gif)

型付き接続、動的socket、inline値、pan／zoom、minimap、検索、layout、preview、groupと編集requestをアプリケーションdataから独立して提供します。companionのMaterial GraphはGUI mockであり、renderingと評価はホスト責務です。

### Workflowと円形進捗

![ImKit workflow feedbackと状態遷移する円形進捗](docs/images/v3-workflow-progress.gif)

filter、notification、step navigation、dialog、state、responsiveな右panel、確定／未測定progressをapplication frameworkなしで構成できます。

### Timeline操作

![ImKit Timeline操作とUndo](docs/images/v3-timeline.gif)

任意editor moduleは、拡張可能なTimeline、外部drop preview、host toolbar、Inspector、canvas、gizmo、curve、hierarchy、ホスト所有Undo／Redo契約を提供します。

### Themeとlive比較

![ImKit ThemeとDefault Dear ImGuiのlive比較](docs/images/v3-theme-comparison.gif)

安定IDを持つ12 Theme、semantic color、density、contrastを一貫して適用し、周囲のDear ImGui styleと操作契約を維持します。

## 必要なmoduleだけ選ぶ

| CMake target | 追加するもの | ホストに残るもの |
|---|---|---|
| `imkit::imkit` | Theme、control、icon、workflow、shell component | Context、frame loop、値、font |
| `imkit::node_editor` | canvas、socket、link、layout、minimap、検索、request | graph model、validation、history、評価 |
| `imkit::editor_core` | canvas、selection、editor共通契約 | documentとcommand |
| `imkit::video`、`imkit::cg`、`imkit::editor_suite` | Timeline、Inspector、hierarchy、curve、編集surface | media／scene data、Undo、保存 |
| `imkit::preview_opengl3` | 明示生成するOpenGL preview renderer | GL Contextとfunction table |
| `imkit::preview_metal` | 明示生成するMetal preview renderer | device、command buffer、texture寿命 |
| platform別WindowFrame／accessibility target | native frameとsemantic bridge | native windowと公開semantic tree |

Galleryは開発用実行ファイルとしてのみGLFWとrenderer backendを使います。`imkit::imkit`をlinkしても利用アプリケーションへ追加されません。

## 所有権の境界が機能である

ImKitはDear ImGui Context、backend、renderer、platform window、font atlas、texture、編集data、Undo履歴、永続化、workerを作成・所有しません。Dear ImGuiのID、focus、navigation、callback、clipping、text editingも維持します。この境界により、ImKitは別frameworkではなく複数の制作ツールで再利用できるlibraryになります。

## 互換性と検証範囲

v3のABI基準はDear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be`（1.93.0 WIP）です。CIはWindows x64/Arm64とmacOS arm64/x86_64をbuild・test・packageし、Universal 2 packageもbuildします。macOS arm64 gateではMetal Gallery smokeを実行します。署名secretがない場合、macOS packageは未署名・未notarizeです。

自動testは物理pointer／keyboard、native IME、実screen reader、mixed-DPI、外部host統合、notarizationの受け入れを証明しません。主張を拡張する前に[検証範囲](docs/validation.md)と[依存関係](docs/dependencies.md)を確認してください。

## 文書

| 目的 | 日本語 | English |
|---|---|---|
| 目的から文書を探す | [文書一覧](docs/README.ja.md) | [Documentation index](docs/README.md) |
| API・Gallery・sourceと文書の対応を見る | [文書カタログ](docs/documentation-catalog.ja.md) | [Documentation catalog](docs/documentation-catalog.md) |
| module別の実例と統合位置を調べる | [実例とrecipe](docs/examples-recipes.ja.md) | [Examples and recipes](docs/examples-recipes.md) |
| 導入と最初のframe | [導入ガイド](docs/getting-started.ja.md) | [Getting started](docs/getting-started.md) |
| native Galleryとcapture | [Gallery](docs/gallery.ja.md) | [Gallery](docs/gallery.md) |
| componentとrecipe | [ガイド](docs/guide.ja.md) | [Guide](docs/guide.md) |
| Node Editor統合 | [Node Editor](docs/node-editor.ja.md) | [Node Editor](docs/node-editor.md) |
| v3破壊的変更 | [Migration](docs/migration-v3.md) | [Migration](docs/migration-v3.md) |
| 所有権とpackage | [Architecture](docs/architecture.md) · [Dependencies](docs/dependencies.md) | 同じ正本文書 |
| 検証済み／未検証範囲 | [Validation](docs/validation.md) | [Validation](docs/validation.md) |

v3の全追加機能と移行点は[CHANGELOG.md](CHANGELOG.md)に記録しています。

## Licenseと出典

ImKitは、Omar Cornut氏とcontributorの皆様が築いてきた[Dear ImGui](https://github.com/ocornut/imgui)の明快さ、移植性、immediate-modeの思想に深い敬意を持って開発しています。Dear ImGuiのforkや代替ではなく、その優れた動作と所有権の境界を維持しながら機能を重ねる独立した拡張layerです。

ImKitは[MIT License](LICENSE)です。Dear ImGui、GLFW、任意font資産には各licenseが適用されます。正確なrevision、hash、配布noticeは[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)にあります。repository GIFとrelease MP4にはnative ImKit Gallery／companionのbackbufferだけを使用しています。
