# imgui-modern-kit

[English](README.md) · [導入ガイド](docs/getting-started.ja.md) · [テーマ](docs/themes.ja.md) · [コンポーネント](docs/components.ja.md) · [Gallery](docs/gallery.ja.md)

**Dear ImGuiに、モダンで一貫したネイティブデザインを追加します。** ImKitは、ホストアプリケーションの所有権を奪わず、C++ツールへ統一された外観、再利用可能な操作部品、意味別テーマ、生成アイコン、高度なEditor UIを提供します。

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

## 完全な12テーマ

`PrecisionLight`、`PrecisionDark`、`Graphite`、`Midnight`、`Ocean`、`Forest`、`WarmSand`、`Rose`、`Violet`、`Solar`、`HighContrastLight`、`HighContrastDark`を利用できます。各presetは標準部品とEditor用の意味別色を初期化します。アプリ固有の配色には`SetAccent`または`Theme`の直接編集を使えます。

```cpp
for (const auto &preset : imkit::ThemePresets()) {
    // preset.idはホスト側の保存に使える安定ID
    ShowThemeChoice(preset.displayName, preset.id);
}
```

コントラスト保証、font所有権、preset保存は[テーマとカスタマイズ](docs/themes.ja.md)を参照してください。

## ネイティブツール向けコンポーネント

重要度別action、switch、混在選択、segmented control、検索付き選択、単位入力、設定行、validation、badge、notification、toolbar、色変更可能なicon catalogを提供します。Theme適用中もDear ImGuiの標準overloadを利用できます。

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

## ドキュメント

| 目的 | 日本語 | English |
|---|---|---|
| 導入と最初のframe | [導入ガイド](docs/getting-started.ja.md) | [Getting started](docs/getting-started.md) |
| preset、accent、font、倍率 | [テーマ](docs/themes.ja.md) | [Themes](docs/themes.md) |
| 部品と実装recipe | [コンポーネント](docs/components.ja.md) | [Components](docs/components.md) |
| Galleryと実capture | [Gallery](docs/gallery.ja.md) | [Gallery](docs/gallery.md) |
| よくある問題 | [トラブルシューティング](docs/troubleshooting.ja.md) | [Troubleshooting](docs/troubleshooting.md) |
| 設計と責務 | [Architecture](docs/architecture.md) | [Architecture](docs/architecture.md) |
| overload単位の対応 | [API coverage](docs/api-coverage.md) | [API coverage](docs/api-coverage.md) |

## 対応範囲と開発状況

検証済み基準はWindows x64/MSVCと固定Dear ImGui docking版です。その他のOSやDear ImGui版を暗黙に互換とは扱いません。native OS/IMEと個別アプリへの組み込みは別の受け入れ確認です。[検証と制約](docs/validation.md)を参照してください。

安定したcore、theme、componentはこのREADME群で説明します。高度なEditor moduleは、ホスト所有データと型付きeventの契約を維持しながら更新を継続します。

## ライセンス

ImKitのコードはMITです。Dear ImGui・GLFWにはそれぞれのライセンス、Galleryで任意利用するInter・Noto Sans JPにはSIL OFL 1.1が適用されます。fontとiconの出典、固定hash、通知は[第三者通知](THIRD_PARTY_NOTICES.md)に記録しています。Dear ImGuiやデザイン参照元との提携を示すものではありません。
