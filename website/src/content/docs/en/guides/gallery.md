---
title: "Gallery guide"
---

The Windows-native Gallery is an onboarding application and a public-API specimen. It uses the same ImKit library that consumers link; it does not hide Gallery-only replacement widgets behind the demo.

## A route for a first visit

1. **Start** explains the host boundary and routes directly to a focused task.
2. **Compare** is an independent page with Theme only and Components modes. The first uses identical `ImGui::` calls. Both share application values, font and scale, with scoped restoration of default style metrics and colors.
3. **Components, themes and icons** provide searchable native specimens, palette editing, and all 288 runtime icon presets in seven sizes. The repository also contains 238 transparent PNG artwork masters across 16 image-backed categories; README shows these at a larger scale alongside the full runtime catalogue.
4. **Workflow, timeline and Frame Lab** show optional compositional and editor-oriented surfaces without claiming that the host's scene, undo or renderer belongs to ImKit.

The Start screen routes to **Components: Basic** (page 0), **Icons** (page 6; change themes from Appearance in the header), **Generic Workspace** (page 15; includes a responsive ChoiceGroup), and **Video** (page 8). The [examples and recipes map](../examples/) connects each route to its public header, CMake target, implementation source, ownership rules and next guide. The Start card identifiers are exercised by the Gallery verifier.

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

The revised media records continuous 1920×1080 native OpenGL backbuffers at 60 Hz. Public Dear ImGui IO supplies the pointer movement, button holds, drag, wheel and character input. A large cursor and click ring are drawn by the capture host at the real input coordinates. Assertions check shared comparison values, workspace requests and node link/move results. This does not verify native OS input, IME, screen readers or physical DPI.

```powershell
& './tools/Invoke-SharedBuild.ps1' -Target imkit_gallery,imkit_node_editor_gallery
python tools/capture_v32_media.py
python tools/render_v32_film.py --review-only
python tools/render_v32_film.py
python tools/check_v32_media.py
```

Run from the repository root. FFmpeg must be on PATH; Python needs Pillow and NumPy for the film. These are development tools, not ImKit runtime dependencies. Lossless videos, input events and checks remain in `out/v3.2-native`; film audio and review frames remain in `out/promo`. Native frames stream into FFmpeg, rather than accumulating a PNG sequence.

The twelve finished GIFs are 960×540, normally 20 fps, six to eight seconds and no more than 2 MiB each. Longer takes are accelerated continuously; the Node Editor GIF focuses on socket connection and header movement, while the film also shows pan and zoom. Encoding can lower the palette or frame rate to meet the size limit. Only finished media is committed.

The English 60-second film combines these operations with original kinetic typography and a synthesized 128 BPM electro-house score. The scripts reproduce the composition and score without stock footage, sampled music or narration. Review the actual playback and operation results as well as media metadata; a file-format check is not a visual-quality review.

## Provenance and distribution boundary

Checked-in `docs/images/v3-*.gif` files are generated from the Gallery or Node Editor companion. The release showcase MP4 uses the same continuous native videos. No stock image, third-party product screen, icon, font, UI implementation or media asset is included. Dear ImGui, GLFW and optional fonts retain their licenses and notices in [THIRD_PARTY_NOTICES.md](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/THIRD_PARTY_NOTICES.md). See [validation](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/validation.md) for exact evidence and exclusions, and [public window frame](../../features/window-frame/) for Frame Lab's platform-adapter boundary.

## ImKit 3.2 layout

Compare (21) and New in 3.2 (22) are independent pages. Wide navigation collapses to a section selector. The sample composition is drawn by the Gallery host with the public DrawList; it does not imply a renderer or scene model owned by ImKit. The main captures use a reproducible 1.0 font DPI factor; physical monitor DPI is not an acceptance result.
