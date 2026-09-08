# Design proposals

Historical comparison record. Generated `out/` images are local artifacts; see the [production catalog](development-status.md) for the adopted library and public screenshots.


Five experimental Dear ImGui themes are available in the separate `imkit_design_gallery` executable. Public library functions and the foundation Gallery are unchanged. No proposal is selected for production.

The original direction 04 was selected for refinement. The Gallery now opens with [three Layered Depth refinements](design-refinements.md); use **Original 5** to return to this first comparison set. The initial five images below remain preserved.

## Run

```powershell
cmake --preset windows-debug -DIMKIT_BUILD_DESIGN_GALLERY=ON
cmake --build --preset windows-debug --target imkit_design_gallery --parallel
./build/windows-debug/Debug/imkit_design_gallery.exe
```

Use 01-05, Light/Dark, Comparison/Interactive, and Reset in the top bar. Comparison displays labelled fixed state samples; they do not claim simultaneous physical hover or focus. Interactive provides actual input, selection, and overlays. Windows smaller than the comparison canvas can scroll. The host owns all contexts, frames, edited values, and animation state.

The option defaults OFF, including external consumer use. The Design Gallery uses Windows WIC for PNG output and the existing GLFW/OpenGL backend. It has no install or production theme API.

## Proposals

Dimensions are unscaled pixels. Transition durations are control/overlay milliseconds. All proposals use Inter 4.1 Regular and SemiBold; icons are original DrawList geometry.

| Proposal | Intent | Height / gap / radius | Body / heading | Motion | Strength | Consideration |
|---|---|---|---|---|---|---|
| 01 Compact Outline | Neutral surfaces, fine boundaries, blue accent | 28 / 6 / 3 | 14 / 18 | 80 / 100 | Compact controls | Small targets |
| 02 Soft Surface | Warm neutrals, soft rounded surfaces, teal accent | 36 / 10 / 10 | 15 / 21 | 140 / 180 | Approachable hierarchy | Subtle surface separation |
| 03 Clear Contrast | Strong boundaries and double focus outlines | 34 / 8 / 2 | 15 / 20 | 40 / 80 | State legibility | Strong visual presence |
| 04 Layered Depth | Cool surfaces, violet accent, layered shadows | 38 / 12 / 8 | 15 / 22 | 120 / 200 | Overlay hierarchy | More depth and movement |
| 05 Spacious Type | Warm neutrals, larger type, quiet separators | 42 / 14 / 6 | 17 / 26 | 160 / 220 | Reading comfort | Lower control density |

Panel positions and item slots are fixed for fair comparison. Differences in control size, typography, padding, edge treatment, and animation expose the density choices within these slots. Not every interactive native widget duplicates the static specimen geometry pixel for pixel; public ImGui APIs retain their native editing and navigation behavior. This is a design proposal gallery, not a completed replacement widget library.

## Benchmark principles and provenance

- [shadcn/ui](https://ui.shadcn.com/docs/theming): semantic surface/foreground pairs and action roles.
- [Base UI](https://base-ui.com/react/overview/about): composable parts and separation of behavior from rendering.
- [Radix Themes](https://www.radix-ui.com/themes/docs/theme/overview): variants and shared spacing, radius, typography, and shadow values.
- [React Aria](https://react-aria.adobe.com/Button): separate hovered, pressed, focused, keyboard-focused, and disabled states.
- [Mantine](https://mantine.dev/theming/theme-object/): consistent theme values across component families.

These are conceptual references, not copied screens or source. Full asset provenance, hashes, and official benchmark license links are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Only the two Inter font files are newly incorporated third-party assets; the complete OFL text accompanies them.

## Images

All source images are real OpenGL backbuffer frames, 1920 x 1440, UI scale 1.0. The capture machine used a fixed 1.5 x 1.5 window content scale; PNG metadata is 96 DPI. Images are read as BGRA, vertically oriented for PNG storage, and encoded losslessly by WIC. No generated imagery or web mockup is used.

```powershell
./build/windows-debug/Debug/imkit_design_gallery.exe --original --capture --output out/design-proposals
```

The capture command opens a hidden GLFW/OpenGL window, switches all themes through public ImGui input events, captures ten comparison frames and ten actual Modal/Combo frames, and makes a five-row/two-column contact sheet. It does not invoke the optional assertion runner.

| Proposal | Light | Dark |
|---|---|---|
| 01 | Light (`out/design-proposals/proposal-01-light.png`) | Dark (`out/design-proposals/proposal-01-dark.png`) |
| 02 | Light (`out/design-proposals/proposal-02-light.png`) | Dark (`out/design-proposals/proposal-02-dark.png`) |
| 03 | Light (`out/design-proposals/proposal-03-light.png`) | Dark (`out/design-proposals/proposal-03-dark.png`) |
| 04 | Light (`out/design-proposals/proposal-04-light.png`) | Dark (`out/design-proposals/proposal-04-dark.png`) |
| 05 | Light (`out/design-proposals/proposal-05-light.png`) | Dark (`out/design-proposals/proposal-05-dark.png`) |

Contact sheet (`out/design-proposals/contact-sheet.png`). Additional actual overlay images use `overlay-01-light.png` through `overlay-05-dark.png` in the same folder. Generated files remain ignored by Git.

## Delivery evidence and limits

- Final Design Gallery Debug build: succeeded.
- Final capture: succeeded on NVIDIA GeForce RTX 3090 Ti, OpenGL 4.6; ten comparison PNGs, ten actual overlay PNGs, and a contact sheet.
- Earlier implementation: 400 public-input assertions passed, covering theme changes, controls, modal isolation, keyboard activation, and animation progression.
- The user subsequently requested appearance-first delivery without more tests. The final Slider/Combo/overlay drawing adjustments were built and captured; the 400-assertion result does not certify those final adjustments. No further assertion run, Context smoke test, or consumer fixture was run after that request.
- Native OS manual interaction was not performed. Real GPU capture is separate evidence from desktop manual acceptance.
- No image generation was used. No production adoption, complete public-API wrappers, host integration, commit, push, release, or packaging was performed.

The next step is the user's selection of 1-5 or a combination of density, palette, and treatment across proposals. Implementation stops at this selection boundary.
