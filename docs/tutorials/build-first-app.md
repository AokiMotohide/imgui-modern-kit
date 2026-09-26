# Build your first ImKit app

[日本語](最初のアプリの作成.md) · [Documentation index](../README.md) · [How ImKit works](../getting-started/how-it-works.md)

This is a codelab. By the end, you will have the full shape of a Dear ImGui host application that draws themed ImKit controls. It is modelled on the repo's minimal host, `examples/consumer/main.cpp`, and uses CPU-embedded resources so it needs no GPU, image loader, or file lookup to *understand* how things fit together.

![Native Gallery overview](../images/gallery-overview.gif)

## What you'll build

A Dear ImGui host application that:

- Creates a Dear ImGui context and sets up its I/O.
- Builds one themed `Theme` value and applies it.
- Draws a few themed ImKit controls inside a child window.
- Lets **your** code react to the controls (a `Toggle` reads/writes your bool; a button result is a value you handle).

Everything is host-owned. ImKit only draws and reports — see [How ImKit works](../getting-started/how-it-works.md) for the ownership model.

## What you need

1. A C++20 toolchain that builds Dear ImGui 1.93.0 WIP (the pinned revision).
2. CMake 3.20 or newer.
3. Dear ImGui source at pinned commit `367b2c24f399988ddafc0bb4628da0106bcc09be`.
4. ImKit source or the SDK package, linked as the `imkit::imkit` target.

You do not need a GPU to read this. The example below is a CPU-only smoke host; the real Gallery uses GLFW/OpenGL or Metal to draw a window.

## Step 1 — Create the Dear ImGui context

ImKit never creates the context. You do, exactly once, and destroy it at shutdown.

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};   // real app: supplied by your windowing backend
    io.DeltaTime   = 1.f / 60.f;   // real app: supplied by your windowing backend
    io.IniFilename = nullptr;      // host owns all state; no settings file
    io.Fonts->AddFontDefault();    // give Dear ImGui a font so text renders
    // ...
}
```

Key points:

- `CreateContext()` is called once, at startup. `DestroyContext()` once, at shutdown.
- `io.IniFilename = nullptr` means ImKit will not read or write any settings file. Your application is the source of truth for all state.
- `AddFontDefault()` gives Dear ImGui a built-in font. In a real app you upload your own.

## Step 2 — Build and apply a theme

A theme is a plain value. `MakeTheme` returns a copy that you own, and `ApplyTheme` installs it on the current Dear ImGui context.

```cpp
    // Icons: CPU data is embedded. A real app uploads each atlas and binds it.
    imkit::IconAtlas icons;
    for (int size : imkit::IconPixelSizes) {
        const auto atlas = imkit::GetIconAtlasPixels(size);
        // Real app: upload atlas.rgba to a GPU texture, then:
        //     icons.SetTexture(size, texture);
    }

    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    imkit::ApplyTheme(theme, 1.0f);   // applied to the current context, before NewFrame
```

- `ThemePreset::PrecisionDark` is one of the named presets; any value from `imkit::ThemePresets()` works.
- `ApplyTheme` takes a scale factor (the third position). `1.0f` is the default.
- ImKit holds no global "current theme". The host decides which theme is active and when.

## Step 3 — Draw your first ImKit controls

`imkit::Begin`/`imkit::End` are Dear ImGui's child-window `Begin`/`End`, re-exported into the `imkit` namespace. They bracket a group of drawing calls. Inside, call themed controls; each reads host-owned values and returns results you handle.

```cpp
    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);   // theme is active only inside this block
        if (imkit::Begin("Settings")) {   // true while the child window is visible
            bool enabled = true;           // YOUR state — not owned by ImKit
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
            imkit::Icon(icons, imkit::IconId::Settings);  // empty until you bind a texture
        }
        imkit::End();
    }
    ImGui::Render();
```

- `{&theme}` is a `ComponentOptions` whose first member is the theme. Pass `{}` when you have no overrides.
- `Toggle` writes your `bool` on the user's input and returns `true` when the state changed.
- `StatusBadge` is a draw-only status chip.
- `Icon` reserves layout space for a glyph; with no bound texture it is intentionally blank in this CPU host.

## The full program

Putting the pieces together:

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {640, 480};
    io.DeltaTime   = 1.f / 60.f;
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefault();

    imkit::IconAtlas icons;
    for (int size : imkit::IconPixelSizes) {
        const auto atlas = imkit::GetIconAtlasPixels(size);
        (void)atlas;   // real app: upload and icons.SetTexture(size, texture);
    }

    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    imkit::ApplyTheme(theme, 1.0f);

    ImGui::NewFrame();
    {
        imkit::ThemeScope scope(theme);
        if (imkit::Begin("Settings")) {
            bool enabled = true;
            imkit::Toggle("Enabled", &enabled, {&theme});
            imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
            imkit::Icon(icons, imkit::IconId::Settings);
            if (imkit::ActionButton("Save", imkit::ActionVariant::Primary, {}, {&theme})) {
                // Your code decides what "Save" means. ImKit only reported the press.
            }
        }
        imkit::End();
    }
    ImGui::Render();

    ImGui::DestroyContext();
    return 0;
}
```

## Run it

1. To see ImKit in a real window today, build and run the **Gallery** — it renders every control in this guide:

   ```powershell
   cmake --preset windows-debug
   cmake --build --preset windows-debug --target imkit_gallery --parallel
   ./build/windows-debug/catalog/Debug/imkit_gallery.exe
   ```

   On macOS, use the `macos-universal` preset (Release, arm64/x86_64). See [Gallery](../getting-started/gallery.md).

2. The exact code above is the minimal host `examples/consumer/main.cpp`. It is a **CPU smoke app**: it sets `DisplaySize` and `DeltaTime` by hand and renders one frame directly, with no windowing backend. It is the smallest possible host that proves the context + theme + controls pattern compiles and runs.

## What just happened

- Your host created the Dear ImGui context and owns everything (context, fonts, state).
- ImKit applied a theme value and drew themed controls *inside* your frame.
- Every control read your values and returned results; nothing was acted on automatically.
- Nothing was persisted and nothing was owned by ImKit.

## Common mistakes

- **Applying the theme after `NewFrame`.** `ApplyTheme` must run on the current context *before* `NewFrame`.
- **Forgetting `imkit::End()`.** A `Begin` without a matching `End` breaks the child window.
- **Expecting ImKit to act on data.** A `Toggle`/`ActionButton`/drag returns a value or request. **You** apply it to your model.
- **Retaining `Theme` pointers.** `Theme` and `ComponentOptions.theme` are non-retained; the host owns the lifetime of any value it passes.
- **Mixing `Begin`/`End` with other window scopes.** Keep the child window balanced and do not nest it inside another `Begin`/`End` incorrectly.

## Next

- [Build a settings screen](build-settings-screen.md) — compose a full form from components.
- [Build a node editor](build-node-editor.md) — render a graph snapshot and collect edits.
- [Build a timeline editor](build-timeline.md) — fades, transitions, and grouped moves.
- [Author a custom component](custom-component.md) — write your own themed control.
