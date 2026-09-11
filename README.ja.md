# imgui-modern-kit

[English](README.md) · [導入ガイド](docs/getting-started.ja.md) · [テーマ](docs/themes.ja.md) · [コンポーネント](docs/components.ja.md) · [Gallery](docs/gallery.ja.md)

**Dear ImGuiに、モダンで一貫したネイティブデザインを追加します。** ImKitは、ホストアプリケーションの所有権を奪わず、C++ツールへ統一された外観、再利用可能な操作部品、意味別テーマ、生成アイコン、高度なEditor UIを提供します。

MITライセンス · C++20 · 静的ライブラリ · Dear ImGui固定基準

![ImKit Gallery：テーマ、コンポーネント、Editor実例](docs/images/gallery-overview.gif)

## ImKitを選ぶ理由

- **単なる色変更ではないデザイン**：階層化したsurface、明確な情報構造、コンパクトな寸法、意味別状態を標準部品と合成部品へ一貫して適用します。
- **Dear ImGuiの操作契約を維持**：ID、focus、navigation、callback、clipping、テキスト編集は標準動作のままです。rendererやframe lifecycleを置き換えません。
- **小さな部品から本格ツールまで拡張**：ボタン、設定行、validationから始め、必要に応じてアイコン、Timeline、Graph、3D workspace APIと組み合わせられます。

## 30秒で導入

対応基準は **Dear ImGui v1.92.9b-docking**、commit `b48d1afbe8ee8b238e2961dc363a949dd7304e23`です。ソース導入を推奨します。

```cmake
# 対応するDear ImGui本体を含むhost_imguiを先に作成
set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
imkit::ApplyTheme(theme); // Context作成後、NewFrameより前

// ホストのframe内
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled, {&theme});
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary, {}, {&theme});
}
imkit::End(); // Beginがfalseでも必要
```

Context、backend、font、renderer、frame lifecycle、ID、編集値、永続化はホストが所有します。ImKitはこれらを作成せず、worker threadも開始しません。

## 導入前に確認すること

ImKitは**静的C++20 UIライブラリ**であり、アプリケーションframeworkやDear ImGuiのforkではありません。ホストが作成したDear ImGui Contextへ、Themeとネイティブ部品、必要に応じてEditor向けmoduleを追加します。renderer、backend、データmodel、Undo/history、永続化、font、frame loopは引き続き利用側の責務です。

| 確認項目 | 現在の回答 |
|---|---|
| 対応基準 | Dear ImGui `v1.92.9b-docking`、`b48d1afbe8ee8b238e2961dc363a949dd7304e23`。検証済み環境はWindows x64/MSVCです。 |
| core依存 | 利用側が互換Dear ImGui targetを渡します。基本targetの`imkit`はGLFW、OpenGL、rendererを持ち込みません。 |
| 開発時だけの依存 | GLFWとOpenGL3はnative Galleryのbuildだけに使用し、通常のconsumerへlinkされません。 |
| 所有権とデータ | 現在Themeのglobal registry、保存済み設定、worker、アプリケーションデータを保持しません。`Theme`、font、編集値はホスト所有です。 |
| 互換を主張しない範囲 | 別Dear ImGui版・別OS、native OS/IME、支援技術、個別ホストアプリの受け入れは暗黙に検証済みとしません。 |

既存ツールへ予測可能に組み込めるよう、この境界を明示しています。Editor moduleを使う前に[設計と所有権](docs/architecture.md)を確認してください。

## 12種類の列挙可能なテーマ

`PrecisionLight`、`PrecisionDark`、`Graphite`、`Midnight`、`Ocean`、`Forest`、`WarmSand`、`Rose`、`Violet`、`Solar`、`HighContrastLight`、`HighContrastDark`を利用できます。安定したpreset IDはホスト側の保存に利用できます。アプリ固有の配色には`SetAccent`または`Theme`の直接編集を使えます。

```cpp
for (const auto &preset : imkit::ThemePresets()) {
    // preset.idはホスト側の保存に使える安定ID
    ShowThemeChoice(preset.displayName, preset.id);
}
```

コントラスト保証、font所有権、preset保存は[テーマとカスタマイズ](docs/themes.ja.md)を参照してください。

## ネイティブツール向けコンポーネント

重要度別action、switch、混在選択、segmented control、検索付き選択、単位入力、設定行、validation、badge、notification、toolbar、色変更可能なicon catalogを提供します。Theme適用中もDear ImGuiの標準overloadを利用できます。

常設アプリShellと、ホストが読み込む任意のInter／Noto Sans JP資産は[アプリケーションShell部品](docs/shell-components.ja.md)を参照してください。

Editor Suiteは同じ契約をTimeline、Curve、3D workspaceへ拡張する発展例です。更新中の詳細仕様をREADMEへ固定せず、[部品契約](docs/editor-suite.md)、[API](docs/editor-api.md)、[検証記録](docs/editor-validation.md)を正本とします。

## Galleryを実行

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

製品型Galleryには、初見向けHome、用途検索、ライブ操作、コピー可能なコード、全theme preset、Editor発展例があります。[Galleryの使い方とcapture](docs/gallery.ja.md)を参照してください。

## 導入方法

- **ソース導入**：推奨。`add_subdirectory`より前に対応するhost ImGui targetを定義します。
- **Installed SDK**：Windows x64/MSVC v145向け。compiler、CRT、ImGui ABI、`imconfig.h`を一致させます。
- **ライブラリ単体**：consumerへGLFW、OpenGL、font、capture依存を追加しません。

詳細は[導入ガイド](docs/getting-started.ja.md)、構成・ABIエラーは[トラブルシューティング](docs/troubleshooting.ja.md)を参照してください。

## 必要なmoduleだけを選ぶ

| Target | 用途 | 依存先 |
|---|---|---|
| `imkit::imkit` | Theme、native wrapper、component、icon、accessibility metadata、workflow pattern | 利用側のDear ImGui target |
| `imkit::editor_core` | Canvas、選択、splitter、Editorのデータ表示契約 | `imkit::imkit` |
| `imkit::video`、`imkit::cg`、`imkit::editor_suite` | 任意のVideo/CG Editor実例と型付きホストevent | `imkit::editor_core` |
| `imkit::preview_opengl3` | 明示的に構築するOpenGL3 preview helper | `imkit::cg`、ホストが渡すContextとGL関数表 |
| `imkit::window_frame_win32` / `imkit::window_frame_macos` | 任意の借用native window adapter | `imkit::imkit`、該当platformのOS libraryだけ |

高度なmoduleは実用的な参照実装ですが、scene、media、選択、Undo、GPU Contextを所有しません。正確な制約は[Editor契約](docs/editor-suite.md)を参照してください。

OS非依存のframe値型、Frame Lab、platform adapter境界は[公開ウィンドウ枠](docs/gallery-window-frame.md)に記載しています。presetと編集済みStyleはホスト所有で、基本ライブラリにWindows／Cocoa依存は入りません。

## 取得した内容を確認する

READMEのGallery画像は手描きmockupではなく、native backbufferから生成しています。生成script、依存版、asset hashはリポジトリに記録しています。検証済みWindows基準でDebugライブラリと公開契約の主要checkを再現する手順は次のとおりです。

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_theme_test imkit_workflow_test --parallel
ctest --test-dir build/windows-debug -C Debug -R "imkit.(theme|workflow)" --output-on-failure
```

確認内容と明示的な対象外は[検証記録](docs/validation.md)を正本とします。buildやtestが成功しても、記載した互換範囲が自動的に広がるわけではありません。

## ドキュメント

| 目的 | 日本語 | English |
|---|---|---|
| 導入と最初のframe | [導入ガイド](docs/getting-started.ja.md) | [Getting started](docs/getting-started.md) |
| preset、accent、font、倍率 | [テーマ](docs/themes.ja.md) | [Themes](docs/themes.md) |
| 部品と実装recipe | [コンポーネント](docs/components.ja.md) | [Components](docs/components.md) |
| アプリShellとfont資産 | [Shell部品](docs/shell-components.ja.md) | [Shell components](docs/shell-components.md) |
| Galleryと実capture | [Gallery](docs/gallery.ja.md) | [Gallery](docs/gallery.md) |
| よくある問題 | [トラブルシューティング](docs/troubleshooting.ja.md) | [Troubleshooting](docs/troubleshooting.md) |
| 設計と責務 | [Architecture](docs/architecture.md) | [Architecture](docs/architecture.md) |
| overload単位の対応 | [API coverage](docs/api-coverage.md) | [API coverage](docs/api-coverage.md) |

## 対応範囲と開発状況

検証済み基準はWindows x64/MSVCと固定Dear ImGui docking版です。その他のOSやDear ImGui版を暗黙に互換とは扱いません。native OS/IMEと個別アプリへの組み込みは別の受け入れ確認です。[検証と制約](docs/validation.md)を参照してください。

安定したcore、theme、componentはこのREADME群で説明します。高度なEditor moduleは、ホスト所有データと型付きeventの契約を維持しながら更新を継続します。

## ライセンス

ImKitのコードは[MIT](LICENSE)です。Dear ImGuiとGallery専用依存には個別のライセンスがあります。任意fontの出典、生成iconのprovenance、固定revision、file hashは[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)に記録しています。参照したdesign projectとの提携・推奨関係は主張しません。

GroupedStepNavigatorは選択可能な大分類と全工程を上下に表示し、任意の分類アクセントと現在・完了・警告を別の形で示します。IconToolbarは既定のアイコンのみ表示に加え、短いラベルを残した折り返し表示を選べます。IconActionButtonは単独操作をアイコン、短いラベル、操作種別、説明で表示します。いずれも状態とテクスチャを借用し、操作要求を返します。
