# ImKit

[日本語](README.ja.md) · [Getting started](docs/getting-started.md) · [Gallery](docs/gallery.md) · [Node Editor](docs/node-editor.md) · [v3 migration](docs/migration-v3.md) · [Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)

**Build modern native tools faster.** ImKit v3.0.0 is a C++20 static UI library for Dear ImGui. It adds a coherent theme system, production controls, workflow and editor surfaces, a host-owned Node Editor, generated icons, native accessibility adapters, and optional OpenGL/Metal preview helpers—without taking ownership of your application.

MIT licensed · Windows x64/Arm64 · macOS arm64/x86_64/Universal 2 · Dear ImGui 1.93.0 WIP docking

## Try it in 30 seconds

Download the package for your machine from [ImKit v3.0.0](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.0.0), extract it, and run `bin/imkit_gallery.exe` or `imkit_gallery.app`. The dedicated Node Editor Gallery is included.

Or build the native Gallery:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

Add ImKit to an existing Dear ImGui application with one target and one theme scope:

```cmake
set(IMKIT_IMGUI_TARGET host_imgui) # your matching Dear ImGui target
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);

// Inside your existing Dear ImGui frame:
imkit::ThemeScope themeScope(theme);
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled);
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
}
imkit::End(); // always pair with Begin
```

Your context, backends, renderer, font atlas, values and frame loop stay exactly where they are.

## See what v3 can do

Each animation is a 960×540 capture of the real native Gallery or Node Editor companion backbuffer—not a redraw or a third-party product recording. [Watch the complete native showcase (MP4)](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v3.0.0/imkit-v3.0.0-showcase.mp4).

### V3 overview

![ImKit v3 native Gallery overview](docs/images/v3-overview.gif)

Start from a guided home page, open the live Default Dear ImGui/ImKit comparison, and move directly into components and editor workflows.

### Host-owned Node Editor

![ImKit v3 Node Editor with dynamic sockets, links, inline values, previews and minimap](docs/images/v3-node-editor.gif)

Typed connections, dynamic sockets, inline values, pan/zoom, minimap, search, layout, previews, grouping and edit requests are independent of application data. The companion is a Material Graph GUI mock; rendering and evaluation remain host responsibilities.

### Workflow and circular progress

![ImKit workflow feedback and animated circular progress](docs/images/v3-workflow-progress.gif)

Compose filters, notifications, step navigation, dialogs, states, responsive side panels and determinate/unavailable progress without adopting an application framework.

### Timeline interaction

![ImKit timeline interaction and undo](docs/images/v3-timeline.gif)

Optional editor modules cover scalable timeline, external-drop previews, host toolbars, Inspector, canvas, gizmo, curves, hierarchy and host-owned Undo/Redo contracts.

### Themes and live comparison

![ImKit themes and live Default Dear ImGui comparison](docs/images/v3-theme-comparison.gif)

Twelve stable theme presets, semantic colors, density and contrast modes apply consistently while preserving surrounding Dear ImGui style and behavior.

## Choose only the modules you need

| CMake target | Adds | Host still owns |
|---|---|---|
| `imkit::imkit` | themes, controls, icons, workflow and shell components | context, frame loop, values, fonts |
| `imkit::node_editor` | canvas, sockets, links, layout, minimap, search and requests | graph model, validation, history, evaluation |
| `imkit::editor_core` | canvas, selection and shared editor contracts | documents and commands |
| `imkit::video`, `imkit::cg`, `imkit::editor_suite` | timeline, Inspector, hierarchy, curves and editing surfaces | media/scene data, Undo, persistence |
| `imkit::preview_opengl3` | explicitly constructed OpenGL preview renderer | GL context and function table |
| `imkit::preview_metal` | explicitly constructed Metal preview renderer | device, command buffer and texture lifetime |
| platform WindowFrame/accessibility targets | native frame and semantic bridges | native window and published semantic tree |

The Gallery depends on GLFW and a renderer backend only as a development executable. Linking `imkit::imkit` does not add them to your application.

## The ownership boundary is the feature

ImKit never creates or owns the Dear ImGui context, backend, renderer, platform windows, font atlas, textures, edited data, Undo history, persistence or workers. Public Dear ImGui IDs, focus, navigation, callbacks, clipping and text editing remain intact. This keeps ImKit reusable across tools instead of turning it into a competing framework.

## Compatibility and validation

The v3 ABI baseline is Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` (1.93.0 WIP). CI builds, tests and packages Windows x64/Arm64 and macOS arm64/x86_64, and builds a Universal 2 package. The macOS arm64 gate launches the Metal Gallery smoke path. Packages are unsigned and not notarized when signing secrets are unavailable.

Automated tests do not establish physical pointer/keyboard, native IME, real screen-reader, mixed-DPI, external-host or notarization acceptance. See [validation scope](docs/validation.md) and [dependencies](docs/dependencies.md) before expanding those claims.

## Documentation

| Need | English | 日本語 |
|---|---|---|
| Browse all documentation by task | [Documentation index](docs/README.md) | [文書一覧](docs/README.ja.md) |
| Install and first frame | [Getting started](docs/getting-started.md) | [導入ガイド](docs/getting-started.ja.md) |
| Native Gallery and capture | [Gallery](docs/gallery.md) | [Galleryガイド](docs/gallery.ja.md) |
| Components and recipes | [Guide](docs/guide.md) | [ガイド](docs/guide.ja.md) |
| Node Editor integration | [Node Editor](docs/node-editor.md) | [Node Editor](docs/node-editor.ja.md) |
| v3 breaking changes | [Migration](docs/migration-v3.md) | [Migration](docs/migration-v3.md) |
| Ownership and packaging | [Architecture](docs/architecture.md) · [Dependencies](docs/dependencies.md) | same canonical documents |
| Verified and unverified scope | [Validation](docs/validation.md) | [Validation](docs/validation.md) |

See [CHANGELOG.md](CHANGELOG.md) for every v3 addition and migration point.

## License and provenance

ImKit is developed with deep respect for [Dear ImGui](https://github.com/ocornut/imgui), and for the clarity, portability and immediate-mode philosophy established by Omar Cornut and its contributors. ImKit is an independent extension layer—not a fork or replacement—and aims to preserve the behavior and ownership boundaries that make Dear ImGui effective.

ImKit is [MIT licensed](LICENSE). Dear ImGui, GLFW and optional font assets retain their own licenses; exact revisions, hashes and distribution notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Repository GIFs and the release MP4 contain only native ImKit Gallery/companion backbuffers.
