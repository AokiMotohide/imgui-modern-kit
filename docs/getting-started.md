# Getting started

[日本語](getting-started.ja.md)

ImKit is a C++20 static library layered over one Dear ImGui implementation supplied by the host. It does not create a context, renderer, backend or frame loop.

## Requirements

- Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` (1.93.0 WIP)
- CMake 3.20 or newer
- Package matrix: Windows x64/Arm64 with MSVC, macOS 15+ arm64/x86_64 with Apple Clang

## Fastest evaluation

Download the matching [v3.1.0 package](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.1.0), extract it, then run the Gallery and Node Editor Gallery in `bin/` on Windows or the two `.app` bundles on macOS. Universal 2 is the default macOS choice when one archive must run on both CPU families.

## Source integration

Create the Dear ImGui target first, expose its include directory, then name it before adding ImKit.

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

The copy helper stages the optional Inter and Noto Sans JP files only. The host
must load them into its own atlas and retain font/context lifetime ownership.

Configure optional ImKit targets before `add_subdirectory` when it is embedded:

```cmake
set(IMKIT_BUILD_GALLERY OFF)
set(IMKIT_BUILD_DESIGN_GALLERY OFF)
set(IMKIT_BUILD_TESTS OFF)
```

## First frame

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
theme.fonts = {regularFont, emphasisFont}; // non-owning
imkit::ApplyTheme(theme, applicationScale); // before NewFrame

ImGui::NewFrame();
if (imkit::Begin("Settings")) {
    imkit::TextUnformatted("Ready");
}
imkit::End();
ImGui::Render();
```

Call `ApplyTheme` on the current live context. A `ThemeScope` may be used inside a frame and must be destroyed on that same context before context destruction.

## Installed SDK

Create the matching host ImGui target first, configure with the SDK prefix, and explicitly confirm the documented ABI after checking compiler, CRT, architecture, ImGui revision and `imconfig.h`.

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_SDK_ABI_CONFIRMED ON)
find_package(imkit 3.0 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

Use source integration whenever the binary conditions differ. The archive does not contain Dear ImGui core.

Link `imkit::node_editor` only when the host needs the independent Node Editor. Its graph snapshot, edit validation, evaluation, Undo and persistence remain host-owned.

## Next steps

- Choose and customize a [theme](themes.md).
- Copy practical [component recipes](components.md).
- Explore the native [Gallery](gallery.md).
- Integrate the host-owned [Node Editor](node-editor.md).
- Diagnose configuration failures with [Troubleshooting](troubleshooting.md).
