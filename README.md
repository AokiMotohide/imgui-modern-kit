# ImKit

[日本語](README.ja.md) · [Documentation index](docs/README.md) · [Getting started](docs/getting-started.md) · [Gallery](docs/gallery.md) · [v3 migration](docs/migration-v3.md) · [Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)

ImKit is a C++20 static library for Dear ImGui. It gives your Dear ImGui widgets a consistent, themeable look and adds a small set of composite controls (themed buttons, toggles, search combos, workflow panels, a node editor and more). It does not own your Dear ImGui context, renderer, frame loop or application state — those stay in your app.

MIT license · Dear ImGui 1.93.0 WIP (docking) · Windows x64/Arm64 · macOS arm64/x86_64 / Universal 2

## Try it first

[Download the v3.1.0 release package](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.1.0), extract it, and run the native Gallery:

- Windows: `bin/imkit_gallery.exe`
- macOS: `imkit_gallery.app`

A dedicated Node Editor Gallery is included as well. The Gallery is the same library your app links against, opened as a live catalog — there are no demo-only replacement widgets hiding inside.

Or build it yourself:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

## Adding it to an existing Dear ImGui app

Point ImKit at your existing Dear ImGui target, then link one imported target:

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)  # your matching Dear ImGui target
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

In your frame loop, apply a theme before `NewFrame` and draw within a `Begin`/`End`:

```cpp
#include <imkit/imkit.h>

auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
imkit::ApplyTheme(theme, 1.25f);  // before ImGui::NewFrame()

// inside NewFrame()/Render():
if (imkit::Begin("Display")) {
    static bool enabled = true;
    imkit::Toggle("Enabled", &enabled);
    imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
}
imkit::End();  // always paired with Begin
```

Your context, backends, renderer, font atlas, values and frame loop are unchanged. ImKit acts on the current live context only.

## What's in each target

| Target | Adds | You keep |
|---|---|---|
| `imkit::imkit` | themes, controls, icons, workflow and shell components | context, frame loop, values, fonts |
| `imkit::node_editor` | canvas, sockets, links, layout, minimap, search, edit requests | graph model, validation, history, evaluation |
| `imkit::editor_core` | canvas, selection, shared editor contracts | documents, commands |
| `imkit::video` / `imkit::cg` / `imkit::editor_suite` | timeline, Inspector, hierarchy, curves, editing surfaces | media/scene data, Undo, persistence |
| `imkit::preview_opengl3` / `imkit::preview_metal` | explicitly constructed preview renderer | GL context / device, command buffer, texture lifetimes |
| platform `window_frame` / `accessibility` targets | native frame and semantic bridges | native window, published semantic tree |

The Gallery links GLFW and a renderer backend only for its own executable. Linking `imkit::imkit` does not add them to your app.

## The ownership boundary is the design

ImKit never creates or owns a Dear ImGui context, backend, renderer, platform window, font atlas, texture, edited data, undo history, persistence or workers. Public Dear ImGui IDs, focus, navigation, callbacks, clipping and text editing keep their normal behavior. That is what lets ImKit sit inside your tool instead of competing with it.

## What the screenshots show

Each animation below is a real capture from the native Gallery (960×540) — not a redraw or a recording of another product. [Full showcase (MP4)](https://github.com/AokiMotohide/imgui-modern-kit/releases/download/v3.1.0/imkit-v3.1.0-showcase.mp4).

### Overview

![Native Gallery overview](docs/images/v3-overview.gif)

The Start screen lists the main routes and links to the recipe map, then opens the live comparison, components, workflow and Timeline specimens.

### Node Editor

![Node editor with dynamic sockets and links](docs/images/v3-node-editor.gif)

Typed connections, dynamic sockets, inline values, pan/zoom, minimap, search, layout and previews are all independent of your application data. The bundled companion is a Material Graph mock; rendering and evaluation stay with the host.

### Workflow and progress

![Workflow feedback and circular progress](docs/images/v3-workflow-progress.gif)

Compose filters, notifications, step navigation, dialogs, states, side panels and determinate or indeterminate progress without adopting a framework.

### Timeline

![Timeline interaction and undo](docs/images/v3-timeline.gif)

Optional editor modules cover a scalable timeline, external-drop previews, host toolbars, Inspector, canvas, gizmos, curves, hierarchy and host-owned Undo/Redo.

### Themes

![Themes against Default Dear ImGui](docs/images/v3-theme-comparison.gif)

Thirteen named presets plus semantic colors, density and contrast modes, applied consistently while preserving the surrounding Dear ImGui style and behavior.

## Compatibility

The v3 ABI is pinned to Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` (1.93.0 WIP). CI builds, tests and packages Windows x64/Arm64 and macOS arm64/x86_64, plus a macOS Universal 2 package; the macOS arm64 gate runs the Metal Gallery smoke path. Packages are unsigned and un-notarized when signing secrets are unavailable.

Automated tests cover build, focused tests and public-IO / GPU smoke checks. They do **not** cover physical pointer/keyboard input, native IME, real screen readers, mixed-DPI or external-host acceptance. Read [validation](docs/validation.md) and [dependencies](docs/dependencies.md) before relying on those.

## Documentation

| Need | English | 日本語 |
|---|---|---|
| Browse all docs by task | [Index](docs/README.md) | [文書一覧](docs/README.ja.md) |
| Audience, API, Gallery route per page | [Catalog](docs/documentation-catalog.md) | [文書カタログ](docs/documentation-catalog.ja.md) |
| Module recipes and frame placement | [Examples and recipes](docs/examples-recipes.md) | [実例とrecipe](docs/examples-recipes.ja.md) |
| Install and first frame | [Getting started](docs/getting-started.md) | [導入ガイド](docs/getting-started.ja.md) |
| Native Gallery and captures | [Gallery](docs/gallery.md) | [Galleryガイド](docs/gallery.ja.md) |
| Components and recipes | [User guide](docs/guide.md) | [ガイド](docs/guide.ja.md) |
| Node Editor integration | [Node Editor](docs/node-editor.md) | [Node Editor](docs/node-editor.ja.md) |
| v3 breaking changes | [Migration](docs/migration-v3.md) | [v3 移行](docs/migration-v3.ja.md) |
| Ownership and packaging | [Architecture](docs/architecture.md) · [Dependencies](docs/dependencies.md) | [設計](docs/architecture.ja.md) · [依存関係](docs/dependencies.ja.md) |
| Verified and unverified scope | [Validation](docs/validation.md) | [検証](docs/validation.ja.md) |

See [CHANGELOG.md](CHANGELOG.md) for every v3 addition and migration point.

## License and provenance

ImKit is developed with respect for [Dear ImGui](https://github.com/ocornut/imgui) and for the clarity, portability and immediate-mode approach established by Omar Cornut and its contributors. ImKit is an independent extension layer — not a fork or a replacement — and it aims to preserve the behavior and ownership boundaries that make Dear ImGui effective.

ImKit is [MIT licensed](LICENSE). Dear ImGui, GLFW and the optional font assets keep their own licenses; exact revisions, hashes and notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The GIFs and the release MP4 contain only native ImKit Gallery / companion backbuffers.
