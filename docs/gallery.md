# Gallery guide

[日本語](gallery.ja.md) · [README](../README.md)

The Windows-native Gallery is an onboarding application and a public-API specimen. It uses the same ImKit library that consumers link; it does not hide Gallery-only replacement widgets behind the demo.

## A route for a first visit

1. **Start** explains the host boundary and routes directly to a focused task.
2. **Compare** opens a movable, resizable Default Dear ImGui / ImKit window. Both columns mutate the same host-owned values, so a click in either column is visible in the other.
3. **Components, themes and icons** provide searchable native specimens, palette editing, and 284 preset outline icons in seven sizes.
4. **Workflow, timeline and Frame Lab** show optional compositional and editor-oriented surfaces without claiming that the host's scene, undo or renderer belongs to ImKit.

The Start screen routes to **Components: Basic** (page 0), **Icons** (page 6; change themes from Appearance in the header), **Generic Workspace** (page 15), and **Video** (page 8). The [examples and recipes map](examples-recipes.md) connects each route to its public header, CMake target, implementation source, ownership rules and next guide. The Start card identifiers are exercised by the Gallery verifier.

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

A normal launch opens the ImKit Gallery and the official Dear ImGui Demo Window as two movable,
resizable windows on the same canvas. They start side by side on a wide display and retain normal
Dear ImGui docking behavior. The Gallery uses the selected ImKit theme; the official demo keeps the
unmodified Dear ImGui style for direct comparison. Close or reopen the demo with the
**Dear ImGui Demo** checkbox in the Gallery header.

## Verification and reproducible captures

The Gallery runner uses public Dear ImGui IO and captures its OpenGL backbuffer. These routes produce deterministic documentation frames; they are not native OS, IME, screen-reader, or physical-DPI automation.

    $gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
    & $gallery --verify-comparison --output out/comparison
    $routes = @('overview', 'comparison', 'components', 'icons', 'themes', 'workflow', 'preview-contract', 'timeline')
    foreach ($route in $routes) {
        & $gallery --capture-demo $route --width 1440 --height 810 --output out/readme
    }
    $node = './build/windows-debug/catalog/Debug/imkit_node_editor_gallery.exe'
    & $node --capture-gif out/readme/node-editor --width 1440 --height 810

Each Gallery route writes native backbuffer frames to a directory under out/readme. Capture at 1440×810, then downsample to 960×540 for the README. The Node Editor mode writes 56 frames at the requested capture size. Encode all nine README images with the expected frame count for each route:

    $routes = @(
        @{name='overview'; file='v3-overview.gif'; frames=60; colors=128; dither='floyd-steinberg'},
        @{name='comparison'; file='v3-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='components'; file='v3-components.gif'; frames=52; colors=128; dither='floyd-steinberg'},
        @{name='icons'; file='v3-icons.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='themes'; file='v3-theme-comparison.gif'; frames=56; colors=128; dither='floyd-steinberg'},
        @{name='workflow'; file='v3-workflow-progress.gif'; frames=64; colors=96; dither='floyd-steinberg'},
        @{name='preview-contract'; file='v3-preview-contract.gif'; frames=50; colors=128; dither='floyd-steinberg'},
        @{name='timeline'; file='v3-timeline.gif'; frames=64; colors=128; dither='floyd-steinberg'},
        @{name='node-editor'; file='v3-node-editor.gif'; frames=56; colors=96; dither='none'}
    )
    foreach ($route in $routes) {
        python tools/build_readme_gif.py "out/readme/$($route.name)" "docs/images/$($route.file)" --expected-frames $route.frames --width 960 --height 540 --fps 8 --colors $route.colors --dither $route.dither
    }

The encoder requires a consistent 16:9 source, resizes it to 960×540, uses 8 fps, and rejects any GIF over 2 MiB. The Node Editor uses a 96-color palette without dithering to stay within that limit; the other routes use the values in the table. Keep the nine README animations within a combined 9 MiB budget. Pillow is only needed to regenerate documentation images; it is not linked or installed by ImKit.

## Provenance and distribution boundary

Checked-in `docs/images/v3-*.gif` files are generated from the Gallery or Node Editor companion. The release showcase MP4 is encoded from the same native frames. No stock image, third-party product screen, icon, font, UI implementation or media asset is included. Dear ImGui, GLFW and optional fonts retain their licenses and notices in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). See [validation](validation.md) for exact evidence and exclusions, and [public window frame](gallery-window-frame.md) for Frame Lab's platform-adapter boundary.
