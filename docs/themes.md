# Themes and customization

[日本語](themes.ja.md)

## Named presets

`ThemePresets()` returns twelve entries in a stable display order. Each `ThemePresetInfo` contains an enum value, a stable lowercase ID for host persistence, an English display name and its Light/Dark scheme.

| Light | Dark |
|---|---|
| Precision Light, Warm Sand, Rose, Solar, High Contrast Light | Precision Dark, Graphite, Midnight, Ocean, Forest, Violet, High Contrast Dark |

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::Forest);
```

Persist the stable ID and resolve it without relying on enum order. Unknown and differently-cased IDs return `std::nullopt`, so the host can apply its own migration or fallback policy.

```cpp
const auto preset = imkit::ThemePresetFromId(savedPresetId)
    .value_or(imkit::ThemePreset::Graphite);
auto theme = imkit::MakeTheme(preset);
```

`MakePrecisionTheme(Light/Dark)` remains source-compatible and produces the same Precision Light/Dark values.

## Customization

`SetAccent` updates accent, focus, contrasting on-accent text and selection. Edit other `Theme::colors`, `metrics`, `motion` or `editor` fields explicitly when needed. Arbitrary edits are not automatically contrast-corrected.

Shipped presets validate normal text at 4.5:1 or better against canvas, surface, input and raised surfaces. Muted text validates at 3:1 or better. Accent and destructive foreground pairs validate at 4.5:1 or better.

## Ownership and persistence

`Theme` is a copyable host-owned value. ImKit has no current-theme registry and writes no files. Store a preset ID or a complete customized value in the host's own settings model, then reconstruct or reapply it explicitly.

For applications that consume a pinned Git submodule but also develop ImKit beside the host, expose a host-owned CMake cache path and pass either the submodule or that explicit path to `add_subdirectory`. Keep the override out of project files and release manifests; both routes must still set `IMKIT_IMGUI_TARGET` before adding ImKit.

Fonts in `FontSet` are non-owning. Load glyphs into the host atlas before the frame. ImKit does not discover OS fonts or provide IME callbacks.

## Scale and scope

`ApplyTheme(theme, scale)` derives a fresh style from unscaled metrics, so repeated calls do not compound dimensions. `ThemeScope` restores the previous style and font on destruction and supports nesting on the same live context.
