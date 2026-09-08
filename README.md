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

