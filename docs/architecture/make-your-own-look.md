# Make your own look

[日本語](世界観を作る.md) · [Documentation index](../README.md) · [Themes](themes.md)

A look in ImKit is one value, `imkit::Theme`: colours, corners, outline, shadow, type sizes and an optional skin. Apply it once with `ApplyTheme` and both plain `ImGui::` widgets and ImKit components follow it. The five steps below go from shallow to deep. Start at the shallowest one that does what you need; each step builds on the one before.

## The five steps

| Step | You write | You get |
|---|---|---|
| 1 | `MakeTheme(ThemePreset::…)` | One of the 13 presets |
| 2 | `BrandPalette` with four colours, `SetCornerStyle` | A complete theme from your brand colours |
| 3 | A few `key = value` lines | Outline, shadow, density and any token, changed without rebuilding |
| 4 | Fields of `Theme` | Every token, directly |
| 5 | `Skin`, `DrawNineSlice`, `DrawBackdropPattern` | Your own drawing for frames and backgrounds |

## Step 2: four colours and a corner style

```cpp
imkit::BrandPalette brand;
brand.canvas  = ImVec4(1.00f, 0.91f, 0.66f, 1);   // window background
brand.surface = ImVec4(1.00f, 0.96f, 0.84f, 1);   // panels and cards
brand.text    = ImVec4(0.29f, 0.16f, 0.09f, 1);
brand.accent  = ImVec4(0.70f, 0.33f, 0.18f, 1);   // primary buttons, focus, selection
imkit::Theme theme = imkit::MakeBrandTheme(brand);
imkit::SetCornerStyle(theme, imkit::CornerStyle::Round);
imkit::ApplyTheme(theme, scale);                  // before ImGui::NewFrame()
```

`raised`, `border`, `textSecondary`, `success`, `warning` and `error` are optional; a colour left at alpha 0 is derived. Control states and selection follow from the four colours. Secondary text and the border are derived against every surface they sit on, so they keep their contrast. The light or dark scheme follows the canvas.

Check `ValidateContrast(theme)` after building a theme. A `false` is a warning: the theme still applies, and the host decides whether to show it. Typography, motion and density keep their defaults; change them on the returned value.

## Step 3: a theme file

`ParseTheme` reads `key = value` lines on top of a base theme. Only the keys that appear change, so a file can be a handful of lines.

```
# my.theme
preset = warm-sand
corner = round
stroke.border = 3
elevation.hard = true
elevation.offset = 0 3
semantic.shadow = #4a2a18
```

```cpp
const std::string text = ReadWholeFile("my.theme");     // the host reads the file
const imkit::ThemeParseResult result = imkit::ParseTheme(text, theme);
if (!result.warnings.empty()) Log(result.warnings);     // bad lines are skipped, never fatal
imkit::ApplyTheme(theme, scale);
```

Keys apply in this order: `preset` replaces the base (fonts and skin are kept), then the `brand.*` colours rebuild the colour tokens, then every other key from top to bottom. `corner = sharp | soft | round | pill` is a shortcut for `SetCornerStyle`. A `#` starts a comment line; colours are `#RRGGBB` or `#RRGGBBAA`. `brand.*` needs `brand.canvas`, `brand.surface`, `brand.text` and `brand.accent`; with fewer, none of them is applied and a warning is returned.

ImKit reads and writes no files and watches nothing. For development, reload the file when its timestamp changes and call `ParseTheme` on a fresh base theme. For release, embed the text as a string literal. `SerializeTheme(theme)` writes every token the parser understands, which is the quickest way to start a file.

## Outlines, corners and shadows

- **Outline.** A border of 2 design pixels or more (`theme.stroke.border`) is an outline: it is drawn in the full-strength border colour on windows, popups, frames and every button variant. Below 2 it stays a soft hairline, exactly as before. Dividers, tree lines and table lines never exceed 1.
- **Corners.** `SetCornerStyle` sets `radius.subtle`, `control` and `overlay` together: Sharp, Soft (the default), Round or Pill; Pill rounds a control to half its height at every density. Windows and images never use a radius larger than the overlay radius, so even a very large control radius does not turn them into capsules.
- **Shadow.** `elevation.hard` replaces the blurred shadow with a solid copy of the shape moved by `elevation.offset`, in `semantic.shadow`. `DrawShadow` draws it for your own shapes and `OverlayDecoration` uses it for popups. Buttons that receive the theme draw it behind the frame and move down by the offset while pressed.

## Text roles and fonts

`PushTextRole(theme, TextRole::Heading)` pushes the size from `theme.typography` and, for headings and titles, the font from `theme.fonts.display` (then `emphasis`, then `regular`). Pair it with `PopTextRole()`. ImKit does not load fonts: put the glyphs into your atlas and pass the pointers in `theme.fonts`.

## Step 5: skins and backdrops

A `Skin` replaces the frame and background of ImKit components with your own drawing. The label, icon, focus ring, accessibility, tooltip, input and return value stay with the library.

```cpp
static bool DrawFrame(const imkit::SkinFrame& frame, void* user) {
    if (frame.part != imkit::SkinPart::Button)
        return false;                                  // anything else keeps the default look
    frame.draw->AddRectFilled(frame.min, frame.max, fill, frame.rounding);
    frame.draw->AddRect(frame.min, frame.max, ink, frame.rounding, 0, 3.f);
    return true;
}
imkit::Skin skin{&DrawFrame, nullptr};
theme.skin = &skin;                                    // non-owning, like theme.fonts
imkit::ActionButton("Go", imkit::ActionVariant::Primary, {}, {&theme});
```

`SkinFrame` carries the rectangle, rounding, part (`Button`, `Badge`, `Card`), variant or status, the interaction state and an eased `hover` value. `DrawNineSlice` stretches a picture you loaded over a rectangle while keeping its corners; `NineSliceQuads` gives the same split without drawing. `DrawBackdropPattern` fills a rectangle with `Dots`, `Stripes` or `Rays`. Neither helper loads or owns a texture.

## What follows the theme on its own

`ApplyTheme` writes `ImGuiStyle`, so colours, corners and outlines reach plain `ImGui::` widgets and ImKit components called without options. Skins, the hard shadow on buttons and text roles need the theme itself: pass it as `ComponentOptions::theme` (`imkit::ComponentOptions{&theme}`) or to the function. ImKit keeps no global current theme. A host that calls components in many places usually wraps them once.

## Theme file keys

Colours are `#RRGGBB[AA]`, numbers are plain decimals, switches are `true` or `false`.

| Keys | Value |
|---|---|
| `preset` | A preset ID such as `warm-sand` |
| `corner` | `sharp`, `soft`, `round` or `pill` |
| `scheme`, `contrast`, `density` | `light` or `dark`; `standard` or `high-contrast`; `compact`, `comfortable` or `touch` |
| `brand.canvas`, `brand.surface`, `brand.text`, `brand.accent` | Required together; colours |
| `brand.raised`, `brand.border`, `brand.text-secondary`, `brand.success`, `brand.warning`, `brand.error` | Optional colours |
| `semantic.canvas`, `surface`, `surface-raised`, `overlay`, `text`, `text-secondary`, `text-disabled`, `border`, `accent`, `on-accent`, `success`, `warning`, `error`, `shadow` | Colours |
| `semantic.control.rest`, `hover`, `pressed`, `focused`, `selected`, `disabled` | Colours |
| `typography.caption`, `body`, `label`, `heading`, `title`, `monospace` | Sizes, 1 to 200 |
| `radius.subtle`, `control`, `overlay` | 0 to 1000 |
| `stroke.border`, `stroke.focus` | 0 to 32 |
| `elevation.surface`, `raised`, `overlay` | 0 to 128 |
| `elevation.offset` | Two numbers, −64 to 64, such as `0 3` |
| `elevation.hard` | Switch |
| `opacity.disabled`, `opacity.scrim` | 0 to 1 |
| `motion.control-seconds`, `motion.overlay-seconds` | 0 to 10 |
| `motion.enabled`, `motion.reduced` | Switch |
| `motion.easing` | `linear`, `ease-out` or `ease-in-out` |

Unknown keys and values out of range are skipped with a warning. Key names are stable: they are never renamed or reused.

## Limits

- Skins and the button shadow reach only components that receive the theme, as described above.
- `editor` colours (clips, axes, markers) are not part of the theme file; `MakeBrandTheme` leaves the ones it does not derive at their Precision values.
- Fonts, textures, files and the lifetime of a skin belong to the host.
- A backdrop is redrawn every frame. For a large, fine halftone, bake a texture and draw it as an image.
