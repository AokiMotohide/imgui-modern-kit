# Getting started

[日本語](getting-started.ja.md)

ImKit is a C++20 static library layered over one Dear ImGui implementation supplied by the host. It does not create a context, renderer, backend or frame loop.

## Requirements

- Dear ImGui v1.92.9b-docking at `b48d1afbe8ee8b238e2961dc363a949dd7304e23`
- CMake 3.20 or newer
- Verified binary environment: Windows x64, MSVC v145

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
find_package(imkit 2.2 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

Use source integration whenever the binary conditions differ. The archive does not contain Dear ImGui core.

## Next steps

- Choose and customize a [theme](themes.md).
- Copy practical [component recipes](components.md).
- Explore the native [Gallery](gallery.md).
- Diagnose configuration failures with [Troubleshooting](troubleshooting.md).
