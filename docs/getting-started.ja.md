# 導入ガイド

[English](getting-started.md)

ImKitは、ホストが用意した1つのDear ImGui実装へ重ねるC++20静的ライブラリです。Context、renderer、backend、frame loopは作成しません。

## 必要環境

- Dear ImGui v1.92.9b-docking、`b48d1afbe8ee8b238e2961dc363a949dd7304e23`
- CMake 3.20以降
- binary検証環境：Windows x64、MSVC v145

## ソース導入

Dear ImGui targetを先に作り、include directoryを公開してから、ImKitへtarget名を渡します。

```cmake
add_library(host_imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(host_imgui PUBLIC ${IMGUI_SOURCE_DIR})

set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
imkit_copy_font_assets(your_app "assets/fonts")
```

copy helperは任意のInter＋Noto Sans JP資産を配置するだけです。atlasへの読込、
font pointer、Contextの寿命は利用側が所有します。

埋め込み時に不要な開発targetは`add_subdirectory`より前に無効化します。

```cmake
set(IMKIT_BUILD_GALLERY OFF)
set(IMKIT_BUILD_DESIGN_GALLERY OFF)
set(IMKIT_BUILD_TESTS OFF)
```

## 最初のframe

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
theme.fonts = {regularFont, emphasisFont}; // 非所有参照
imkit::ApplyTheme(theme, applicationScale); // NewFrameより前

ImGui::NewFrame();
if (imkit::Begin("Settings")) {
    imkit::TextUnformatted("Ready");
}
imkit::End();
ImGui::Render();
```

`ApplyTheme`は現在の生存中Contextへ適用します。frame内では`ThemeScope`も使えますが、作成時と同じContext上でContext破棄より先に破棄してください。

## インストール済みSDK

対応するhost ImGui targetを先に作り、SDK prefixを指定します。compiler、CRT、architecture、ImGui revision、`imconfig.h`を確認した後だけABI一致を明示します。

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_SDK_ABI_CONFIRMED ON)
find_package(imkit 2.1 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

binary条件が異なる場合はソース導入を使います。archiveにDear ImGui本体は含まれません。

## 次に読む文書

- [テーマ](themes.ja.md)を選び、配色を調整する。
- [コンポーネントrecipe](components.ja.md)を利用する。
- native [Gallery](gallery.ja.md)で操作する。
- 構成エラーは[トラブルシューティング](troubleshooting.ja.md)で確認する。
