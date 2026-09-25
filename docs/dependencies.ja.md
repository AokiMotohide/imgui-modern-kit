# Dependencies / 依存関係

[English](dependencies.md) · [文書カタログ](documentation-catalog.ja.md)

## 固定依存

| 依存 | version・revision | 用途 |
|---|---|---|
| Dear ImGui | 1.93.0 WIP docking · `367b2c24f399988ddafc0bb4628da0106bcc09be` | 公開widget APIと型。利用hostのABI一致が必要 |
| GLFW | 3.5.1 · `d9d6f0f1f967807ffade6598ea9a631ebaf37a56` | Galleryのwindow、input、OpenGL Context |

第三者license全文はrepositoryの`THIRD_PARTY_NOTICES.md`にあります。

## 解決方法

- Galleryを有効にしたstandalone buildは、`IMKIT_IMGUI_TARGET`未指定時に固定revisionを取得します。Gallery無効時はGLFWを取得しません。
- subdirectoryとして利用する場合、既に存在するDear ImGui targetを`IMKIT_IMGUI_TARGET`へ指定します。この方式では依存をdownloadしません。
- ImKitはDear ImGui本体を再compileしません。includeとlink要件は指定target経由でconsumerへ渡します。
- Windows GalleryはCMake `FindOpenGL`を使います。macOS Galleryはsystem Metal、MetalKit、Cocoa frameworkを使います。

CMakeの最低対応versionは3.20です。`imkit` 3.1.0のconsumerは上記固定docking ABIを使います。別revision、`imconfig.h`、compiler/architectureの組合せは同一ABIと推定せず、明示確認が必要です。

Inter 4.1とNoto Sans JP 2.004は任意のhost assetです。出典、hash、OFLは`THIRD_PARTY_NOTICES.md`に記録されています。`imkit_copy_font_assets`はtarget隣へ配置しますが、fontをatlasにloadし寿命を持つのはホストです。

CPackはWindows x64/Arm64、macOS arm64/x86_64/Universal 2向けにlibrary、Gallery、Node Editor Gallery、英日文書、license、noticeを収録します。PillowとFFmpegは文書media作成用で、libraryへlink/install/再配布されません。
