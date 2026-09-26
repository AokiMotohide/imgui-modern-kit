# Layered Depth refinements

Historical comparison record. Generated `out/` images are local artifacts; see the [production catalog](development-status.md) for the adopted library and public screenshots.


Direction 04 is the selected basis. Three alternatives refine its density, elevation, typography, and state visibility for complex desktop editing. These are component proposals, not a replacement application layout or host integration.

## Common changes

- Neutral canvas and panel colors reduce surrounding color cast. Accent remains independently editable.
- Input surfaces sit below panels; popup/modal surfaces sit above them.
- Smaller radii and selective shadows reduce visual noise in adjacent panels.
- Numeric specimens use right-aligned values, explicit units, and X/Y/Z fields. These added numeric specimens are appearance samples; the interactive pane retains its existing scalar and range controls.
- Selected items use both a mark and a surface change. Keyboard focus remains a distinct outline.
- Three variants use the same text, data, control positions, font family, and default accent for comparison.

| Proposal | Height / gap / radius (px) | Body / heading (px) | Control / overlay motion (ms) | Intended emphasis |
|---|---|---|---|---|
| 01 Precision Layers | 28 / 6 / 4 | 14 / 18 | 60 / 100 | Dense numeric editing; restrained panel depth and underline tabs |
| 02 Balanced Layers | 34 / 9 / 6 | 15 / 21 | 100 / 150 | Reading/editing balance; selective shadow and soft selection |
| 03 Focus Layers | 40 / 12 / 8 | 16 / 23 | 100 / 180 | Larger targets, stronger borders, selection, and focus |

**01 Precision Layers is the selected production theme.** The other proposals remain comparison history. The production catalog uses the shipped `imkit` API; see [the user guide](guide.md).

## Change colors

Open **Colors** in the Gallery toolbar. Violet, Blue, and Teal offer quick accent presets. Each preset also updates its foreground, focus, and selected-surface colors. The color swatches open native Dear ImGui color editors.

Editable roles: accent, on-accent, canvas, surface, input surface, raised surface, text, muted text, border, selection, focus, destructive, and on-destructive. Light and dark retain separate overrides. Overrides are shared across the three proposals so geometry can be compared with the same colors. Manually edited color roles are independent; arbitrary colors are not automatically contrast-corrected.

**Reset** resets example values while preserving palette edits. **Reset palette** restores colors for the active light/dark mode. Palette edits last for the current process only; there is no persistence, export, or production Theme API. Switching between the original and refined collections clears overrides.

## Run and images

```powershell
./build/windows-debug/Debug/imkit_design_gallery.exe
./build/windows-debug/Debug/imkit_design_gallery.exe --capture --output out/design-refinements
```

The Gallery defaults to the three refinements; **Original 5** restores the first proposal collection. All original images remain in their original output folder.

| Proposal | Light | Dark |
|---|---|---|
| 01 Precision Layers | Light (`out/design-refinements/proposal-01-light.png`) | Dark (`out/design-refinements/proposal-01-dark.png`) |
| 02 Balanced Layers | Light (`out/design-refinements/proposal-02-light.png`) | Dark (`out/design-refinements/proposal-02-dark.png`) |
| 03 Focus Layers | Light (`out/design-refinements/proposal-03-light.png`) | Dark (`out/design-refinements/proposal-03-dark.png`) |

- Contact sheet (`out/design-refinements/contact-sheet.png`): three rows, light left, dark right.
- Color variants (`out/design-refinements/colors-contact-sheet.png`): proposal 02 dark; blue left, teal right.
- Palette editor (`out/design-refinements/palette-editor.png`).
- Six actual Modal/Combo frames: `overlay-01-light.png` through `overlay-03-dark.png` in the same folder.

Images are 1920 x 1440 actual Dear ImGui/OpenGL backbuffer frames, UI scale 1.0, fixed window content scale 1.5. Only the contact sheets are scaled composites. No image generation or web mockup is used.

## Delivery boundary

The refinement captures above predate Japanese support. The Japanese follow-up builds only `imkit_design_gallery` in Debug and captures its Japanese specimen in light/dark; no test suite, assertion runner, Context smoke test, or consumer fixture is required for this change. Host compatibility and native IME input are not evaluated.

The existing Inter 4.1 files and their complete OFL notice remain unchanged. Japanese fallback adds the unmodified, Japanese-subset Noto Sans JP Regular 2.004 and its complete OFL license; [provenance and hashes](../THIRD_PARTY_NOTICES.md) are recorded. No host application, public API, or library implementation is changed. This is experimental-only, without release, packaging, or production adoption.

## Japanese specimen

Select **日本語 / Japanese** in the toolbar. The specimen includes Japanese headings, instructions, an editable UTF-8 name, checkbox, enabled/disabled actions, mixed Latin numbers/units, hiragana, katakana, kanji, half-width kana, and punctuation. Select the same button again to return to the existing gallery.

The gallery host merges Noto Sans JP immediately after each Inter source (Regular and SemiBold). Latin U+0020–U+024F is excluded from the fallback; Inter has first priority for other overlapping glyphs. Dear ImGui 1.92's dynamic atlas rasterizes requested glyphs on demand. Both Japanese weights use Regular; font coverage is limited to the supplied font, not every Unicode character. Other hosts must configure their own fonts; `imkit` does not load fonts or search the OS.

```powershell
cmake --build build/windows-debug --config Debug --target imkit_design_gallery
./build/windows-debug/Debug/imkit_design_gallery.exe --capture-japanese --output out/design-japanese
```

This focused capture writes only Japanese light (`out/design-japanese/japanese-light.png`), Japanese dark (`out/design-japanese/japanese-dark.png`), and renderer metadata. It does not run the older proposal capture matrix or interaction verifier. These are real OpenGL backbuffer images, not proof of native IME operation.

Verified on 2026-09-09: the changed Debug target built successfully, and both 1920 × 1440 frames were captured on NVIDIA GeForce RTX 3090 Ti (OpenGL 4.6, Dear ImGui 1.92.9b). Visual inspection found no missing Japanese glyphs or clipped labels in the specimen. Interactive editing and native IME operation were not tested.
