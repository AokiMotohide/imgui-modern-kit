# ImKit

**Build polished native tools with Dear ImGui.**

ImKit is a C++20 static library that adds consistent themes and reusable controls to an existing Dear ImGui application. Start with the native Gallery, then integrate only the modules your tool needs.

[日本語](README.ja.md) · [Download the latest release](https://github.com/AokiMotohide/imgui-modern-kit/releases/latest) · [Documentation](docs/README.md) · [Releases](https://github.com/AokiMotohide/imgui-modern-kit/releases)

<img src="docs/images/v3-overview.gif" alt="The ImKit Gallery routes from its start page into live examples" width="960">

## Try the Gallery

Download the [v3.1.0 package for your platform](https://github.com/AokiMotohide/imgui-modern-kit/releases/tag/v3.1.0), extract it, and run:

- Windows: bin/imkit_gallery.exe
- macOS: imkit_gallery.app

The package also includes a dedicated Node Editor Gallery. The Gallery uses the same library and public components that an application links; its examples are not demo-only replacements.

To build the Gallery from source on Windows:

    cmake --preset windows-debug
    cmake --build --preset windows-debug --target imkit_gallery --parallel
    ./build/windows-debug/catalog/Debug/imkit_gallery.exe

## Add ImKit to an existing app

Point ImKit at the Dear ImGui target your application already uses:

    set(IMKIT_IMGUI_TARGET host_imgui)
    add_subdirectory(external/imgui-modern-kit)
    target_link_libraries(your_app PRIVATE imkit::imkit)

Apply a theme before starting the ImGui frame, then draw controls inside a normal window:

    #include <imkit/imkit.h>

    auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
    imkit::ApplyTheme(theme, 1.0f); // before ImGui::NewFrame()

    if (imkit::Begin("Display")) {
        static bool enabled = true;
        imkit::Toggle("Enabled", &enabled);
        imkit::ActionButton("Apply", imkit::ActionVariant::Primary);
    }
    imkit::End();

Your application keeps its Dear ImGui context, backends, renderer, fonts, data, undo history, persistence, and workers. ImKit draws into the current context and provides reusable UI.

## What you can build

- **Consistent controls and themes** — buttons, toggles, selection states, search, icons, and semantic palettes.
- **Workflow surfaces** — task navigation, notifications, progress, and host-composed application shells.
- **Editor building blocks** — timelines, inspectors, curves, hierarchy, preview monitors, and an optional Node Editor.
- **Native integration points** — optional OpenGL or Metal preview paths, WindowFrame adapters, and accessibility bridges.

Choose the smallest CMake target that provides the components you need. The [module recipes](docs/examples-recipes.md) show the target, public header, Gallery page, and frame placement for each area.

## See the components in action

### Gallery overview

<img src="docs/images/v3-overview.gif" alt="Gallery start page and routes to live examples" width="960">

Start with a task, open its live specimen, and follow the route to the integration guide.

### Components

<img src="docs/images/v3-components.gif" alt="ImKit buttons, toggles, selection controls, and input components" width="960">

Explore common controls with enabled, toggled, and mixed-selection states.

### Preset icon catalogue · 284 icons

**284 preset outline icons · 7 pixel sizes · 1,988 icon-size combinations.** Browse the tile catalogue, search by name or category, and use atlas sizes from 12 to 64 px.

<img src="docs/images/v3-icons.gif" alt="A tile catalogue of ImKit's 284 preset icons, changing selection and category" width="960">

The Gallery shows the complete catalogue as a searchable tile grid and previews the selected icon with its matching C++ call.

### Live comparison

<img src="docs/images/v3-comparison.gif" alt="Default Dear ImGui and ImKit controls updating shared values" width="960">

Compare the default Dear ImGui widgets with ImKit while both sides update the same host-owned values.

### Preview placement and states

<img src="docs/images/v3-preview-contract.gif" alt="Fit, Fill, and Stretch across Ready, Loading, Empty, Offline, and Error preview states" width="960">

See how Fit, Fill, and Stretch present Ready, Loading, Empty, Offline, and Error states.

### Workflow and progress

<img src="docs/images/v3-workflow-progress.gif" alt="Workflow navigation, feedback controls, and progress" width="960">

Compose task navigation, notifications, dialogs, and progress without handing application state to the library.

### Timeline

<img src="docs/images/v3-timeline.gif" alt="Timeline editing interaction and undo" width="960">

Inspect timeline controls and editing feedback while the host retains the timeline data and history.

### Node Editor

<img src="docs/images/v3-node-editor.gif" alt="Node Editor with links, dynamic sockets, minimap, and previews" width="960">

Use the optional graph canvas and edit requests with your own graph model, validation, evaluation, and undo.

### Themes

<img src="docs/images/v3-theme-comparison.gif" alt="ImKit themes and comparison with default Dear ImGui" width="960">

Compare named themes and palette choices in the native Gallery.

## Compatibility

The v3 ABI targets Dear ImGui 1.93.0 WIP docking commit 367b2c24f399988ddafc0bb4628da0106bcc09be. Release packages are available for Windows x64 and Arm64, macOS arm64 and x86_64, and macOS Universal 2.

The macOS packages are unsigned and not notarized. Automated checks do not establish physical input, native IME, screen-reader, mixed-DPI, or third-party host acceptance. See [validation scope](docs/validation.md) and [dependencies](docs/dependencies.md).

## Documentation

- [Documentation index](docs/README.md)
- [Getting started](docs/getting-started.md)
- [Components and usage](docs/guide.md)
- [Module examples and recipes](docs/examples-recipes.md)
- [Gallery guide](docs/gallery.md)
- [Node Editor integration](docs/node-editor.md)
- [v3 migration guide](docs/migration-v3.md)
- [Architecture and ownership](docs/architecture.md)
- [Changelog](CHANGELOG.md)

日本語の文書は[日本語README](README.ja.md)から参照できます。

## License

ImKit is MIT licensed. Dear ImGui, GLFW, and optional font assets retain their own licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for revisions, hashes, and notices.
