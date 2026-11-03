---
title: "Getting started"
description: "ImKit is a C++20 static library that you link into an application that already uses Dear ImGui."
---

ImKit is a C++20 static library that you link into an application that already uses Dear ImGui. It does not create a context, renderer, backend or frame loop. This page takes you from a clean project to the first themed frame.

## Requirements

| Item | Requirement |
| --- | --- |
| C++ | C++20 |
| Dear ImGui | `docking` branch, commit `367b2c24f399988ddafc0bb4628da0106bcc09be` (1.93.0 WIP) |
| Build system | CMake 3.20 or newer |
| Platforms | Windows x64 / Arm64 with MSVC, macOS 15 or later (arm64 / x86_64) with Apple Clang |

Your application supplies the Dear ImGui sources. See [Dependencies](../platform/dependencies/) for why the revision is pinned.

## Choose how to start

| Goal | Path |
| --- | --- |
| See what ImKit looks like | [Run the Gallery](#run-the-gallery) |
| Use it in a CMake project | [Add ImKit from source](#add-imkit-from-source) |
| Use the prebuilt libraries | [Use the SDK package](#use-the-sdk-package) |

## Run the Gallery

Download the [latest release package](https://github.com/AokiMotohide/imgui-modern-kit/releases/latest) for your platform and extract it.

- **Windows:** run `imkit_gallery.exe` and `imkit_node_editor_gallery.exe` in `bin/`.
- **macOS:** open the two `.app` bundles. Use the Universal 2 package if one archive must run on both Apple Silicon and Intel. The packages are unsigned and not notarized.

![The Gallery Start page: the same controls before and after an ImKit theme, then the whole Gallery changing theme](/imgui-modern-kit/docs-images/v4-start-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-start.gif)

The Gallery is built only from the public components that your application links, so its source in `examples/gallery/` is a working reference.

## Add ImKit from source

1. Build Dear ImGui as a CMake target in your project.
2. Set `IMKIT_IMGUI_TARGET` to that target's name.
3. Add ImKit with `add_subdirectory`.
4. Link `imkit::imkit` to your application.

```cmake
add_library(host_imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(host_imgui PUBLIC ${IMGUI_SOURCE_DIR})

set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_BUILD_GALLERY OFF)          # skip the samples and tests when embedding
set(IMKIT_BUILD_DESIGN_GALLERY OFF)
set(IMKIT_BUILD_TESTS OFF)
add_subdirectory(external/imgui-modern-kit)

target_link_libraries(your_app PRIVATE imkit::imkit)
imkit_copy_font_assets(your_app "assets/fonts")   # optional
```

`IMKIT_IMGUI_TARGET` must name an existing target. ImKit then downloads nothing and does not compile Dear ImGui. `imkit_copy_font_assets` only copies the optional Inter and Noto Sans JP files beside your executable. Loading them into the font atlas, and keeping the atlas and context alive, is your application's job.

### Optional modules

| Need | Link |
| --- | --- |
| Themes, controls, workflow, charts, PTZ, icons | `imkit::imkit` |
| Timeline, curves, inspector, preview monitors | `imkit::editor_suite` (or the narrower `imkit::video`, `imkit::cg`) |
| Node graph | `imkit::node_editor` |
| Show control | `imkit::show` |
| Custom title bar | `imkit::window_frame_win32` / `imkit::window_frame_macos` |
| OpenGL or Metal preview | `imkit::preview_opengl3` / `imkit::preview_metal` |

## Draw the first frame

Apply a theme on the live context before `NewFrame`, then draw inside a normal window.

```cpp
#include <imkit/imkit.h>

// Once, after the Dear ImGui context exists
imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
// theme.fonts = {regularFont, emphasisFont};   // optional, borrowed pointers

// Every frame
imkit::ApplyTheme(theme);          // before ImGui::NewFrame()
ImGui::NewFrame();

if (imkit::Begin("Settings")) {
    imkit::TextUnformatted("ImKit is running");
    if (imkit::ActionButton("Test", imkit::ActionVariant::Primary)) {
        // your code
    }
}
imkit::End();

ImGui::Render();
// your backend submits ImGui::GetDrawData()
```

- `ApplyTheme(theme, scale)` takes your application scale. The default is 125%, and the supported range is 50% to 250%.
- To give one window its own theme, create an `imkit::ThemeScope` before the widgets. Destroy it on the same context, before that context is destroyed.

## Use the SDK package

Use the installed SDK only when you have confirmed that the compiler, CRT, architecture, Dear ImGui revision and `imconfig.h` all match the package. Set the confirmation flag yourself.

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)
set(CMAKE_PREFIX_PATH "path/to/imkit-sdk" ${CMAKE_PREFIX_PATH})
set(IMKIT_SDK_ABI_CONFIRMED ON)
find_package(imkit 4.0 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

If any condition differs, build from source. The package does not contain Dear ImGui.

## Next steps

| Read | For |
| --- | --- |
| [How ImKit works](../guides/how-it-works/) | The frame model and who owns what |
| [Build your first app](../tutorials/first-app/) | A complete walkthrough |
| [Themes](../features/themes/) | Choosing and customizing a theme |
| [Components](../features/components/) | The controls and a full settings panel |
| [Gallery guide](../guides/gallery/) | Finding each component's source in the Gallery |
| [Troubleshooting](../platform/troubleshooting/) | Build, link and runtime errors |
