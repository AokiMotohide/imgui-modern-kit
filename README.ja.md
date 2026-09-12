# imgui-modern-kit

[English](README.md) · [導入ガイド](docs/getting-started.ja.md) · [Gallery ガイド](docs/gallery.ja.md) · [変更履歴](CHANGELOG.md)

## ✨ 作る道具にも、仕事と同じ意図を宿す

**ImKit v2.1.0** はDear ImGui向けのC++20デザインレイヤーです。一貫した外観、再利用可能な操作部品、意味別テーマ、生成icon、任意のEditor surfaceを追加しながら、Context、renderer、データ、workflowの所有権はホストへ残します。

MITライセンス · 静的ライブラリ · Windows x64/MSVCで検証済み · Dear ImGui `v1.92.9b-docking` 基準

![native ImKit Galleryの案内](docs/images/gallery-overview.gif)

> 🪟 **まずGalleryを試してください。** [Windows x64 Gallery](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v2.1.0/imkit-2.1.0-gallery-windows-x64.zip)をdownloadし、展開後に`imkit_gallery.exe`を実行します。installerもアプリコードも不要です。

## 宣伝用mockupではなく、実際に並べて確かめる

移動・resize可能な **Compare** windowでは、直接Dear ImGuiを使った実例とImKitの実例を並べます。両列は*同じホスト所有値*を編集します。左は`StyleColorsDark`と公開Dear ImGui widget、右はImKit部品と選択中Themeを使います。

![Default Dear ImGuiとImKitのライブ比較](docs/images/gallery-comparison.gif)

これは描き直した画像ではなく、実GalleryのOpenGL backbufferから得たcaptureです。視覚構造と操作契約の継続性を示します。性能、native OS/IME、accessibilityのbenchmarkではありません。比較画面のために第三者UIコード・assetを複製、追加していません。

| Theme palette | Workflow feedback | Timeline編集 |
|---|---|---|
| ![Theme paletteの遷移](docs/images/gallery-themes.gif) | ![Workflow状態のfeedback](docs/images/gallery-workflow.gif) | ![Timeline操作](docs/images/gallery-timeline.gif) |

## 30秒で価値を確認する

ソースからGalleryをbuildします。

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

最初の画面から比較、component、theme、workflow、Frame Labへ案内します。各画面でホスト所有権の境界を見える化しているため、実例が知らないうちにframework依存へ変わりません。

既存Dear ImGuiホストへ追加します。

```cmake
# 対応するDear ImGui本体を含むhost_imguiを先に作成します。
set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
imkit::ApplyTheme(theme); // Context作成後、NewFrameより前

// ホストのframe内:
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled, {&theme});
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary, {}, {&theme});
}
imkit::End(); // Beginがfalseでも必要
```

## アプリらしさを決める部分は、アプリのまま残す

ImKitは次のものを作成・所有しません。

- Dear ImGui Context、backend、renderer、frame loop
- Font atlas、texture、GPU resource、platform window
- 編集値、scene/mediaデータ、Undo履歴、永続化、worker

ID、focus、keyboard navigation、callback、clipping、テキスト編集はDear ImGuiの標準動作を保ちます。ImKitはdesign/component libraryであり、renderer、アプリframework、Dear ImGui forkではありません。

## 必要なmoduleだけを選ぶ

| Target | 主な用途 | 依存境界 |
|---|---|---|
| `imkit::imkit` | Theme、native wrapper、control、icon、workflow pattern | 互換Dear ImGui target |
| `imkit::editor_core` | Canvas、選択、splitter、Editorデータ表示契約 | `imkit::imkit` |
| `imkit::video`、`imkit::cg`、`imkit::editor_suite` | 任意の高度なEditor実例 | `imkit::editor_core` |
| `imkit::preview_opengl3` | 明示的に構築するpreview helper | ホストが渡すOpenGL Context／関数表 |
| `imkit::window_frame_win32` / `imkit::window_frame_macos` | 任意の借用native window adapter | 該当platformのOS libraryだけ |

ソース導入を推奨します。Windows SDK archiveは検証済みcompiler／CRT／ImGui ABIの組合せ向けです。Galleryは開発用実行ファイルとしてGLFWとOpenGLを使いますが、`imkit::imkit`のconsumerへ追加しません。

## ✅ 検証済み範囲を明確にする

対応基準はDear ImGui `v1.92.9b-docking`、commit `b48d1afbe8ee8b238e2961dc363a949dd7304e23`、Windows x64/MSVCです。native Gallery captureはcommit済みコードから再生成できます。公開IO検証では共有状態の比較とGallery workflowを扱います。native OS/IME入力、支援技術、別platform、個別ホストアプリの受け入れは別作業です。

```powershell
cmake --build --preset windows-debug --target imkit_theme_test imkit_workflow_test --parallel
ctest --test-dir build/windows-debug -C Debug -R "imkit.(theme|workflow)" --output-on-failure
./build/windows-debug/catalog/Debug/imkit_gallery.exe --verify-comparison --output out/comparison
```

互換範囲を広げる前に、完全な[検証記録と制約](docs/validation.md)を確認してください。

## 📚 次に読む文書

| 目的 | 日本語 | English |
|---|---|---|
| 導入と最初のframe | [導入ガイド](docs/getting-started.ja.md) | [Getting started](docs/getting-started.md) |
| Gallery、操作、再生成可能GIF | [Gallery ガイド](docs/gallery.ja.md) | [Gallery guide](docs/gallery.md) |
| Theme、font、倍率 | [テーマ](docs/themes.ja.md) | [Themes](docs/themes.md) |
| 部品と実装recipe | [コンポーネント](docs/components.ja.md) | [Components](docs/components.md) |
| 設計とホスト所有権 | [Architecture](docs/architecture.md) | [Architecture](docs/architecture.md) |
| APIと問題解決 | [API coverage](docs/api-coverage.md) · [トラブルシューティング](docs/troubleshooting.ja.md) | [API coverage](docs/api-coverage.md) · [Troubleshooting](docs/troubleshooting.md) |

## ライセンスと出典

ImKitは[MITライセンス](LICENSE)です。Dear ImGui、GLFW、任意font assetにはそれぞれのライセンスが適用されます。版、hash、fontの出典、配布noticeは[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)に記録しています。v2.1のGallery比較とGIFには、第三者の画像・icon・font・コードassetを追加していません。
