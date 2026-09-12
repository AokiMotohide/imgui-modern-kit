# Gallery guide

[日本語](gallery.ja.md) · [README](../README.md)

The Windows-native Gallery is an onboarding application and a public-API specimen. It uses the same ImKit library that consumers link; it does not hide Gallery-only replacement widgets behind the demo.

## A route for a first visit

1. **Start** explains the host boundary and routes directly to a focused task.
2. **Compare** opens a movable, resizable Default Dear ImGui / ImKit window. Both columns mutate the same host-owned values, so a click in either column is visible in the other.
3. **Components, themes and icons** provide searchable native specimens, palette editing and generated icons.
4. **Workflow, timeline and Frame Lab** show optional compositional and editor-oriented surfaces without claiming that the host's scene, undo or renderer belongs to ImKit.

The comparison's baseline is only public Dear ImGui API (`StyleColorsDark` plus direct widgets). It is a visual and interaction-contract example, not a claim about performance, OS input or accessibility. Its ImKit column borrows the host-owned `Theme`, scale, state and animation values; no global registry or Gallery-only third-party asset is introduced.

## Build and run

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

The released Windows archive contains `imkit_gallery.exe`, the required `design-assets` directory, this project's license and third-party notices. It does not install a service, create a user configuration or add a runtime dependency to an ImKit consumer.

## Verification and reproducible captures

The Gallery runner uses public Dear ImGui IO and its real OpenGL backbuffer. It is appropriate for deterministic widget contracts and visual documentation; it is **not** native OS/IME, screen-reader or physical-DPI automation.

```powershell
$gallery = './build/windows-debug/catalog/Debug/imkit_gallery.exe'
& $gallery --verify-comparison --output out/comparison
& $gallery --capture-readme --width 960 --height 540 --output out/readme
foreach ($demo in 'comparison', 'themes', 'workflow', 'timeline') {
    & $gallery --capture-demo $demo --width 960 --height 540 --output out/gifs
}
```

`--verify-comparison` checks that Default and ImKit controls update shared host-owned state, that their temporary styles restore after a frame, and that close/reopen removes and restores the submitted controls.

`--capture-readme` emits 120 frames. Each `--capture-demo` route emits 80 frames. The sequences show Start / Comparison / Components, shared-value edits, palette and preset transitions, workflow feedback, and timeline interaction. All frames are native `960×540` backbuffers.

```powershell
python tools/build_readme_gif.py out/readme/readme-frames docs/images/gallery-overview.gif
foreach ($demo in 'comparison', 'themes', 'workflow', 'timeline') {
    python tools/build_readme_gif.py (Join-Path out/gifs $demo) (Join-Path docs/images ("gallery-$demo.gif")) --expected-frames 80
}
```

The encoder rejects wrong frame counts or dimensions and GIFs over 8 MiB. Pillow is a documentation-only tool: it is neither linked nor installed by ImKit.

## Provenance and distribution boundary

Checked-in `docs/images/gallery-*.gif` files are generated from the Gallery. No stock image, third-party icon, font, UI implementation or media asset was added for the v2.1 comparison and GIFs. Dear ImGui, GLFW and optional fonts retain their existing licenses and notices in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). See [validation](validation.md) for exact evidence and exclusions, and [public window frame](gallery-window-frame.md) for Frame Lab's platform-adapter boundary.
