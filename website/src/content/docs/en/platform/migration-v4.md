---
title: "Migrate from v3"
---

**Who needs this:** every application that links ImKit 3.x. Source changes are small. Rebuilding is mandatory, and the default look changes unless you set the old values yourself.

## Four steps

| Step | What to do | Why |
| --- | --- | --- |
| 1 | Rebuild your application and **every** ImKit static library with the 4.0 headers. Ask for `find_package(imkit 4.0 CONFIG REQUIRED)`. | Several public structs grew. A 3.x object next to a 4.0 structure reads the wrong layout. The Dear ImGui revision is unchanged. |
| 2 | Look at the app once with the new defaults. Keep them, or restore the old tokens below. | Radii, shadows, motion and control height changed. |
| 3 | Replace deprecated spellings when the compiler warns. | They still compile in 4.0 but are marked `[[deprecated]]`. |
| 4 | If you call `imkit::Combo`, `BeginCombo`, `RadioButton` or the `Slider*` family, retest them. | They are now ImKit wrappers, not forwards to `ImGui::`. |

## Default tokens that changed

| Token | 3.3.0 | 4.0.0 |
| --- | --- | --- |
| `Radius` subtle / control / overlay | 2 / 4 / 6 | 4 / 8 / 12 |
| `Elevation` raised / overlay | 2 / 5 | 4 / 14, plus `offset` and `hard` |
| `Opacity::disabled` | 1 | 0.55 |
| `Metrics::controlHeight` (Comfortable) | 28 | 30 |
| `Metrics::horizontalPadding` (Comfortable) | 10 | 12 |
| `Motion` control / overlay seconds | 0.06 / 0.10 | 0.14 / 0.20 |

To keep the 3.3 look, set the old values after you build the theme. `ResolveTheme` recomputes the metrics from the density, so assign them last:

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
theme.radius = {2, 4, 6};
theme.elevation.raised = 2;
theme.elevation.overlay = 5;
theme.opacity.disabled = 1.f;
theme.motion.controlSeconds = 0.06f;
theme.motion.overlaySeconds = 0.10f;
theme.metrics.controlHeight = 28;
theme.metrics.horizontalPadding = 10;
imkit::ApplyTheme(theme, 1.0f);
```

The coloured presets (Graphite, Midnight, Ocean, Forest, Violet, Warm Sand, Rose, Solar) are now built from four brand colours, so their derived colours differ slightly from 3.x. Precision Light and Precision Dark keep their own palettes.

## Deprecated spellings

| Deprecated | Use instead |
| --- | --- |
| `StatusBadge(text, kind, const Theme*)` | `StatusBadge(text, kind, ComponentOptions{&theme})` |
| `NotificationCard(n, now, const Theme*)` | `NotificationCard(n, now, ComponentOptions{&theme})` |
| `ValidationMessage(message, invalid, const Theme*)` | `ValidationMessage(message, invalid, ComponentOptions{&theme})` |
| `EmptyState(message)` | `EmptyState(id, StateView, options)` |
| `Progress(id, fraction, label)` | `Progress(id, ProgressView, ProgressPresentation, ...)` or `ProgressTrack` |
| `ToastRegion(span<Notification>)` | `ToastViewport` |

Define `IMKIT_DEPRECATED(message)` as nothing to silence the warnings while you migrate.

## Other behaviour changes

- `SemanticRole` gains `Slider` and `Meter`. `RangeSlider` publishes `Slider` nodes instead of `Group` to assistive technology.
- Chevron combos do not mark the item as edited; use the return value as before.
- `PtzState` keeps only `speed` public.
- `IMKIT_VERSION` now reports `4.0.0`. It stayed at `3.0.0` through 3.1 to 3.3.

## Check it worked

- The build reports no mismatch between object and header versions, and `find_package(imkit 4.0)` resolves.
- Open the app next to the Gallery (`imkit_gallery`) and compare **Before / After** with your own screens.
- Run your own tests with the deprecation warnings on, then turn them into fixes one at a time.
