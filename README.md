# imgui-modern-kit

This repository is the development foundation for a Dear ImGui extension library. The current widgets delegate directly to standard Dear ImGui; modern visual design is not implemented.

## Requirements

- Windows
- Visual Studio 2026 with the Desktop development with C++ workload
- CMake 3.20 or newer (`windows-debug` uses a CMake version that supports the Visual Studio 18 2026 generator)
- Git and network access for the first standalone dependency fetch

## Standalone build and Gallery

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --parallel
ctest --test-dir build/windows-debug -C Debug --output-on-failure
./build/windows-debug/Debug/imkit_gallery.exe
```

The standalone configuration downloads pinned Dear ImGui and GLFW revisions into `build/`. It does not install them globally.

## Use with a host-owned Dear ImGui target

Create the Dear ImGui target before adding this repository. Library-only subdirectory use performs no downloads and does not require GLFW or OpenGL.

```cmake
add_library(host_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(host_imgui PUBLIC ${imgui_SOURCE_DIR})

set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(path/to/imgui-modern-kit)
target_link_libraries(my_app PRIVATE imkit::imkit)
```

To build the Gallery against a host target, its public include paths must expose the matching official `backends/` directory, and the target must provide `ImGui::ShowDemoWindow` (normally by compiling `imgui_demo.cpp`).

## Current API

```cpp
#include <imkit/imkit.h>

bool enabled = false;
float amount = 0.5F;
char name[64] = "Sample text";

imkit::Button("Button");
imkit::Checkbox("Enabled", &enabled);
imkit::SliderFloat("Amount", &amount, 0.0F, 1.0F);
imkit::InputText("Name", name, sizeof(name));
imkit::Selectable("Item", false);
imkit::ProgressBar(amount);
```

The host owns the current Dear ImGui context, frame lifecycle, and all edited values.

## Not implemented

Modern styling, custom drawing, custom widgets, wrappers for the complete Dear ImGui API, host-application integration, installation, packaging, and release automation are outside this foundation stage.

---

# imgui-modern-kit（日本語）

このリポジトリは、Dear ImGui拡張ライブラリを開発するための基盤です。現在の部品は標準Dear ImGuiへ直接処理を委譲しており、モダンな外観の設計・実装はまだ行っていません。

## 必要な開発環境

- Windows
- 「C++によるデスクトップ開発」ワークロードを含むVisual Studio 2026
- CMake 3.20以降（`windows-debug`プリセットにはVisual Studio 18 2026ジェネレーターを扱えるCMakeが必要）
- スタンドアロン構成で依存を初回取得するためのGitとネットワーク接続

## スタンドアロンのビルドとGallery起動

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --parallel
ctest --test-dir build/windows-debug -C Debug --output-on-failure
./build/windows-debug/Debug/imkit_gallery.exe
```

スタンドアロン構成では、固定したDear ImGuiとGLFWのリビジョンを`build/`内へ取得します。システム全体へのインストールは行いません。

## ホスト所有のDear ImGuiターゲットを使用する

このリポジトリを追加する前に、ホスト側でDear ImGuiターゲットを作成してください。ライブラリだけをサブディレクトリとして利用する場合、依存のダウンロードは行わず、GLFWとOpenGLも要求しません。

```cmake
add_library(host_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(host_imgui PUBLIC ${imgui_SOURCE_DIR})

set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(path/to/imgui-modern-kit)
target_link_libraries(my_app PRIVATE imkit::imkit)
```

ホスト側のDear ImGuiターゲットを使ってGalleryをビルドする場合、公開includeパスから対応する公式`backends/`ディレクトリを参照でき、ターゲットが`ImGui::ShowDemoWindow`を提供している必要があります。通常は`imgui_demo.cpp`をコンパイルして提供します。

## 現在のAPI

```cpp
#include <imkit/imkit.h>

bool enabled = false;
float amount = 0.5F;
char name[64] = "Sample text";

imkit::Button("Button");
imkit::Checkbox("Enabled", &enabled);
imkit::SliderFloat("Amount", &amount, 0.0F, 1.0F);
imkit::InputText("Name", name, sizeof(name));
imkit::Selectable("Item", false);
imkit::ProgressBar(amount);
```

現在のDear ImGui Context、フレームのライフサイクル、編集対象の値はすべてホストが所有します。

## 未実装の範囲

モダンなスタイル、独自描画、独自部品、Dear ImGui API全体のラッパー、ホストアプリへの組み込み、インストール、パッケージ化、自動リリースは、この基盤段階の対象外です。
