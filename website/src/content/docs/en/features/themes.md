---
title: "Themes"
description: "ImKit has thirteen named presets and a set of functions to adjust or build your own."
---

ImKit has thirteen named presets and a set of functions to adjust or build your own. A `Theme` is a plain value that your application owns, and `ApplyTheme` writes it into the current Dear ImGui context.

![Picking a theme in the Gallery changes the colours of every component](/imgui-modern-kit/docs-images/v4-themes-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-themes.gif)

## Built-in presets

`ThemePresets()` returns the presets in a stable display order. Each `ThemePresetInfo` holds the enum value, a stable lowercase ID for saving (for example `"precision-dark"`), an English display name and the Light or Dark scheme.

| Scheme | Preset | Character |
| --- | --- | --- |
| Dark | `PrecisionDark` | The standard dark theme: neutral grey with a blue accent |
| | `Graphite` | Near-black grey with a mint accent |
| | `Midnight` | Deep navy with a lavender-blue accent |
| | `Ocean` | Dark teal with a cyan accent |
| | `Forest` | Deep green with a green accent |
| | `Violet` | Dark purple with a purple accent |
| | `Slate` | Blue-grey canvas and surfaces |
| | `HighContrastDark` | Raised contrast for legibility |
| Light | `PrecisionLight` | The standard light theme: white and pale grey |
| | `WarmSand` | Sand beige with a brown accent |
| | `Rose` | Pale pink with a rose accent |
| | `Solar` | Warm cream with an olive-gold accent |
| | `HighContrastLight` | Raised contrast for legibility |

`PrecisionLight`, `PrecisionDark`, `Slate` and the two high-contrast presets have hand-tuned palettes. The other eight are built by `MakeBrandTheme` from four colours: canvas, surface, text and accent. They also show what an application can do with the same function (see [Make your own look](../make-your-own-look/)).

## Choose and restore a theme

Save the stable ID, not the enum value, and resolve it again with `ThemePresetFromId`. An unknown or differently cased ID returns `std::nullopt`, so you choose the fallback.

```cpp
#include <imkit/theme.h>

// From a preset
imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::Forest);

// From a saved ID, with a fallback
const auto preset = imkit::ThemePresetFromId(savedPresetId)
    .value_or(imkit::ThemePreset::PrecisionDark);
imkit::Theme current = imkit::MakeTheme(preset);
```

`MakePrecisionTheme(Light/Dark)` is kept for source compatibility and gives the same Precision values.

## Customize

```cpp
// Change the accent. Buttons, focus rings and selection follow.
imkit::SetAccent(theme, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
```

| Field | Role |
| --- | --- |
| `semantic`, `typography`, `radius`, `stroke`, `elevation`, `opacity` | The source of truth. Edit these, then call `ResolveTheme(theme)`. |
| `colors`, `metrics` | Projections that `ResolveTheme` and `SetDensity` rebuild. Direct edits are overwritten. |
| `layout` | Optional size overrides: control height, spacing, padding, and the ratios for Checkbox, Toggle, badge and dialog. Applied after the density. A value you leave unset keeps the density, so a theme that sets nothing draws as before. |
| `motion`, `editor` | Edit explicitly when you need to. |

`SetAccent` updates the accent, focus colour, contrasting text on the accent and the selection. Arbitrary edits are not contrast-corrected automatically.

The shipped presets keep normal text at 4.5:1 or better against canvas, surface, input and raised surfaces, muted text at 3:1 or better, and accent and destructive foreground pairs at 4.5:1 or better. Check your own theme with `ValidateContrast`.

## What `ApplyTheme` does

`ApplyTheme` replaces the whole `ImGuiStyle` of the current context, except `Alpha` and `FontScaleDpi`. Make your adjustments in the `Theme`, not in `ImGuiStyle`, because the next `ApplyTheme` overwrites style edits you made directly.

| Topic | Rule |
| --- | --- |
| Scale | `ApplyTheme(theme, scale)` rebuilds the style from unscaled values, so repeated calls never compound sizes. The range is 50% to 250% and the default is 125% (`ThemeScaleMinimum`, `ThemeScaleDefault`, `ThemeScaleMaximum`). Monitor DPI is your application's job. |
| Scope | `ThemeScope` applies a theme for a block and restores the previous style and font when it ends. Scopes nest on the same live context. |
| Fonts | `FontSet` holds borrowed pointers. Load glyphs into your atlas before the frame. ImKit does not find OS fonts or provide IME callbacks. |
| Ownership | `Theme` is a copyable value. ImKit has no current-theme registry and writes no files. Store a preset ID, or a complete customized value, in your own settings, and apply it again when needed. |
