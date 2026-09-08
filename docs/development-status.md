# Development status

## Implemented

- Static `imkit` library and `imkit::imkit` consumer target
- Host-provided Dear ImGui target contract and standalone pinned dependency path
- Direct standard delegation for Button, Checkbox, SliderFloat, InputText, Selectable, and ProgressBar
- Standard-versus-wrapper Gallery with independent state, reset, and official demo toggle
- Windowless context smoke test and external-target consumer fixture
- Architecture, dependency, license, and widget inventory documentation

## Verification

- Environment: Windows, Visual Studio Community 2026 18.9.2, MSVC 19.51.36256.0, CMake 4.3.0-rc1
- `cmake --preset windows-debug`: passed; pinned Dear ImGui and GLFW sources were fetched into `build/windows-debug`
- `cmake --build --preset windows-debug --parallel`: passed for `imkit`, `imkit_gallery`, `imkit_context_smoke`, and their development dependencies
- `imkit.context_smoke`: passed after preparing a font atlas independently for each host-selected context
- `imkit.consumer_build`: passed with `imkit/imkit.h` included directly; Gallery and tests disabled; no GLFW or OpenGL target created; no Dear ImGui source compiled by `imkit`
- Tests-only top-level configure (`IMKIT_BUILD_GALLERY=OFF`, `IMKIT_BUILD_TESTS=ON`): passed without resolving GLFW or OpenGL
- `imkit_gallery.exe`: launched successfully and exposed a top-level window titled `ImKit Development Gallery`

The available automation surface could not inspect native application content. Widget interaction and screenshot capture were not performed. Manual acceptance remains: compare both columns, edit all stateful widgets, use Reset, and toggle the official Dear ImGui demo.

## Not implemented

Modern design, complete standard-widget coverage, custom widgets, integration into an existing application, GitHub publication, packaging, installation, and automated releases have not started.
