# Gallery guide

[日本語](ギャラリーガイド.md) · [README](../../README.md)

The Windows-native Gallery is an onboarding application and a public-API specimen. It uses the same ImKit library that consumers link; it does not hide Gallery-only replacement widgets behind the demo.

## A route for a first visit

1. **Start** explains the host boundary and routes directly to a focused task.
2. **Compare** is an independent page with Theme only and Components modes. The first uses identical `ImGui::` calls. Both share application values, font and scale, with scoped restoration of default style metrics and colors.
3. **Components, themes and icons** provide searchable native specimens, palette editing, and all 288 runtime icon presets in seven sizes. The repository also contains 238 transparent PNG artwork masters across 16 image-backed categories; README shows these at a larger scale alongside the full runtime catalogue.
4. **Workflow, timeline and Frame Lab** show optional compositional and editor-oriented surfaces without claiming that the host's scene, undo or renderer belongs to ImKit.

The Start screen routes to **Components: Basic** (page 0), **Icons** (page 6; change themes from Appearance in the header), **Generic Workspace** (page 15; includes a responsive ChoiceGroup), and **Video** (page 8). The [examples and recipes map](examples-recipes.md) connects each route to its public header, CMake target, implementation source, ownership rules and next guide. The Start card identifiers are exercised by the Gallery verifier.

The comparison's baseline is only public Dear ImGui API (`StyleColorsDark` plus direct widgets). It is a visual and interaction-contract example, not a claim about performance, OS input or accessibility. Its ImKit column borrows the host-owned `Theme`, scale, state and animation values; no global registry or Gallery-only third-party asset is introduced.

## Build and run

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

On macOS use the `macos-universal` preset (Release, arm64/x86_64, Universal 2):

```bash
cmake --preset macos-universal
cmake --build --preset macos-universal --target imkit_gallery --parallel
# The app bundle is produced in the build/macos-universal output directory.
```

The released Windows archive contains `imkit_gallery.exe`, the required `design-assets` directory, this project's license and third-party notices. It does not install a service, create a user configuration or add a runtime dependency to an ImKit consumer.

A normal launch opens a focused Gallery. Wide windows show category navigation; compact windows use the section selector. **Compare** (page 21) and **New in 3.2** (page 22) are separate pages. The latter combines tabs, hierarchy actions, a preview, settings, three-axis input, choices and a status/action row. Each request is applied to Gallery-owned state.

## Verification and reproducible captures

The Gallery runner uses public Dear ImGui IO and captures its OpenGL backbuffer. These routes produce deterministic documentation frames; they are not native OS, IME, screen-reader, or physical-DPI automation.

    $gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
    & $gallery --verify-comparison --output out/comparison
    & $gallery --verify-toasts --width 1440 --height 810 --output out/readme/toasts
    $routes = @('overview', 'comparison', 'components', 'icons', 'icon-artwork', 'themes', 'workflow', 'preview-contract', 'timeline', 'workspace', 'toasts')
    foreach ($route in $routes) {
        & $gallery --capture-demo $route --width 1440 --height 810 --output out/readme
    }
    $node = './build/windows-debug/catalog/Debug/imkit_node_editor_gallery.exe'
    & $node --capture-gif out/readme/node-editor --width 1440 --height 810

Each Gallery route writes native backbuffer frames to a directory under out/readme. Run the icon-artwork route from the repository root so it can read the checked-in PNG masters. The Gallery decodes those files with Windows Imaging Component, renders the artwork as tiles, and captures the OpenGL backbuffer. Capture at 1440×810, then downsample to 960×540 for the README. The Node Editor mode writes 56 frames at the requested capture size. Encode the eleven README animations with the expected frame count for each route:

    $routes = @(
        @{name='overview'; file='v3-overview.gif'; frames=60; colors=128; dither='floyd-steinberg'},
        @{name='comparison'; file='v3-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='components'; file='v3-components.gif'; frames=52; colors=128; dither='floyd-steinberg'},
        @{name='icons'; file='v3-icons.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='icon-artwork'; file='v3-icon-artwork.gif'; frames=60; colors=128; dither='floyd-steinberg'},
        @{name='themes'; file='v3-theme-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='workflow'; file='v3-workflow-progress.gif'; frames=64; colors=96; dither='floyd-steinberg'},
        @{name='preview-contract'; file='v3-preview-contract.gif'; frames=50; colors=128; dither='floyd-steinberg'},
        @{name='timeline'; file='v3-timeline.gif'; frames=64; colors=128; dither='floyd-steinberg'},
        @{name='node-editor'; file='v3-node-editor.gif'; frames=56; colors=96; dither='none'}
    )
    foreach ($route in $routes) {
        python tools/build_readme_gif.py "out/readme/$($route.name)" "docs/images/$($route.file)" --expected-frames $route.frames --width 960 --height 540 --fps 8 --colors $route.colors --dither $route.dither
    }
    python tools/build_readme_gif.py out/readme/toasts docs/images/v3-toasts.gif --pattern 'toasts-[0-5]-*.png' --expected-frames 12 --width 960 --height 540 --fps 6 --colors 96 --dither none --disposal 2

The encoder requires a consistent 16:9 source, resizes it to 960×540, and rejects any GIF over 2 MiB. The Node Editor and Toasts routes use a 96-color palette without dithering; the others use the values in the table. Keep all eleven README animations within a combined 9 MiB budget. Pillow is only needed to regenerate documentation images; it is not linked or installed by ImKit.

## Provenance and distribution boundary

Checked-in `docs/images/v3-*.gif` files are generated from the Gallery or Node Editor companion. The release showcase MP4 is encoded from the same native frames. No stock image, third-party product screen, icon, font, UI implementation or media asset is included. Dear ImGui, GLFW and optional fonts retain their licenses and notices in [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md). See [validation](../reference/validation.md) for exact evidence and exclusions, and [public window frame](../components/gallery-window-frame.md) for Frame Lab's platform-adapter boundary.

## ImKit 3.2 layout

Compare (21) and New in 3.2 (22) are independent pages. Wide navigation collapses to a section selector. The sample composition is drawn by the Gallery host with the public DrawList; it does not imply a renderer or scene model owned by ImKit. The main captures use a reproducible 1.0 font DPI factor; physical monitor DPI is not an acceptance result.
