# 製品Gallery

[English](gallery.md)

native Galleryは初見向け案内と公開APIの実例を兼ねます。installed libraryから使えない代替部品をGallery内だけに実装しません。

## Navigation

- **Start**：デザイン概要、30秒導入、主要機能への入口。
- **Components**：action、数値、input/media、合成部品。
- **Patterns**：階層・データ、overlay・layoutの実例。
- **Themes & Icons**：完全な12preset、palette編集、検索可能なicon。
- **Editor Examples**：更新中のEditor Core、Video、3D workspace。

header検索はcomponent名、API名、用途keywordを対象にします。安定componentの各pageにはライブ操作、最小コードのCopy、所有権の注意を表示します。desktop幅未満ではsidebarをcompact selectorへ切り替えます。

## Buildと実行

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

通常起動では、ImKit GalleryとDear ImGui公式Demo Windowを同じキャンバス上の可動・リサイズ可能な
2つのウィンドウとして表示します。広い画面では左右に初期配置し、通常のDear ImGui docking操作を
維持します。Galleryには選択中のImKit themeを適用し、公式Demoは比較のためDear ImGui標準styleの
まま描画します。公式DemoはGallery headerの**Dear ImGui Demo**チェックから閉じる・再表示できます。

## 検証とcapture

```powershell
./build/windows-debug/catalog/Debug/imkit_gallery.exe --verify --output out/catalog
./build/windows-debug/catalog/Debug/imkit_gallery.exe --capture --output out/catalog
```

公開Dear ImGui IOと実OpenGL backbufferを使用します。native OS/IME automationではありません。
captureと検証modeは、従来の決定的な全キャンバスGallery配置を維持し、Dear ImGui公式Demo Windowを
表示しません。

## README GIFの再生成

```powershell
./build/windows-debug/catalog/Debug/imkit_gallery.exe --capture-readme --output out/readme
python tools/build_readme_gif.py out/readme/readme-frames docs/images/gallery-overview.gif
```

決定的なsequenceで960×540のnative frameを120枚取得し、10fps・12秒へ変換します。Pillowは文書生成だけに使い、consumerへlink・install・公開しません。commitするGIFは8MiB未満とします。

Windows Galleryには、OS非依存の公開window frame値型と任意Win32 adapterを編集する**Frame Lab**もあります。[公開ウィンドウ枠](gallery-window-frame.md)を参照してください。既存の自動captureは、枠専用検証を明示しない限りnative OS frameを維持します。
