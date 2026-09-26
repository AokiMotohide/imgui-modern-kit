# Gallery guide

[日本語](gallery.ja.md) · [README](../README.md)

The Windows-native Gallery is an onboarding application and a public-API specimen. It uses the same ImKit library that consumers link; it does not hide Gallery-only replacement widgets behind the demo.

## A route for a first visit

1. **Start** explains the host boundary and routes directly to a focused task.
2. **Compare** opens a movable, resizable Default Dear ImGui / ImKit window. Both columns mutate the same host-owned values, so a click in either column is visible in the other.
3. **Components, themes and icons** provide searchable native specimens, palette editing and generated icons.
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

The Gallery runner uses public Dear ImGui IO and its real OpenGL backbuffer. It is appropriate for deterministic widget contracts and visual documentation; it is **not** native OS/IME, screen-reader or physical-DPI automation.

```powershell
$gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
& $gallery --verify-comparison --output out/comparison
& $gallery --capture-readme --width 960 --height 540 --output out/readme
foreach ($demo in 'comparison', 'themes', 'icons', 'workflow', 'timeline') {
    & $gallery --capture-demo $demo --width 960 --height 540 --output out/gifs
}
$node = './build/windows-debug/catalog/Debug/imkit_node_editor_gallery.exe'
& $node --capture-gif out/gifs/node-editor
```

`--verify-comparison` checks that Default and ImKit controls update shared host-owned state, that their temporary styles restore after a frame, and that close/reopen removes and restores the submitted controls.
These use public Dear ImGui IO and real OpenGL backbuffers. They are not native OS/IME automation.
Capture and verification modes retain the deterministic full-canvas Gallery layout and do not open
the Dear ImGui Demo Window.

`--capture-readme` emits 120 frames. The v3 README routes emit 120 workflow/progress frames, 80 timeline frames, 120 theme/comparison frames and 80 Node Editor frames. The Node Editor capture uses host-owned deterministic state to show zoom, pan, a dynamic socket, a new connection, inline values, previews and the minimap. All frames are native `960×540` backbuffers.

```powershell
python tools/build_readme_gif.py out/readme/readme-frames docs/images/v3-overview.gif
python tools/build_readme_gif.py out/gifs/node-editor docs/images/v3-node-editor.gif --expected-frames 80
python tools/build_readme_gif.py out/gifs/workflow docs/images/v3-workflow-progress.gif
python tools/build_readme_gif.py out/gifs/timeline docs/images/v3-timeline.gif --expected-frames 80
python tools/build_readme_gif.py out/gifs/themes docs/images/v3-theme-comparison.gif
```

The encoder rejects wrong frame counts or dimensions and GIFs over 8 MiB. Pillow is a documentation-only tool: it is neither linked nor installed by ImKit.

## Provenance and distribution boundary

Checked-in `docs/images/v3-*.gif` files are generated from the Gallery or Node Editor companion. The release showcase MP4 is encoded from the same native frames. No stock image, third-party product screen, icon, font, UI implementation or media asset is included. Dear ImGui, GLFW and optional fonts retain their licenses and notices in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). See [validation](validation.md) for exact evidence and exclusions, and [public window frame](gallery-window-frame.md) for Frame Lab's platform-adapter boundary.
