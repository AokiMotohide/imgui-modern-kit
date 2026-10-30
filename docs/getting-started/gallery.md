# Gallery guide

[日本語](ギャラリーガイド.md) · [README](../../README.md)

The Gallery is the fastest way to see ImKit before you integrate it. It is a desktop app built on the same public library you link, so nothing on screen is a demo-only widget.

**How to read this page:** [Start here](#start-here-three-minutes) and [Build and run](#build-and-run) are for a first visit. The page map and the capture sections are reference.

## Start here (three minutes)

| Step | Do this | What you see |
| --- | --- | --- |
| 1 | Open **Start** and switch **Dear ImGui / ImKit** above the Display settings panel. | The same controls before and after a theme. Both edit the same values. |
| 2 | Under **Wear a world**, click a theme. | The whole Gallery re-themes. |
| 3 | Open **Theme Studio** (Look) and load `blueprint`, `pastel-pop` or `sticker`. | Colours, corners, outline and shadow change together. The text at the bottom is the theme file that reproduces the look. |
| 4 | Open **Before / After**. | Plain Dear ImGui and ImKit side by side, editing one set of host-owned values. |

The header has a page search (Enter opens the first match), **Appearance** (theme, dark, density, contrast, reduced motion, scale), **Compare** and **Node Editor**.

## Build and run

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

On macOS use the `macos-universal` preset (Release, arm64 and x86_64):

```bash
cmake --preset macos-universal
cmake --build --preset macos-universal --target imkit_gallery --parallel
# The app bundle is produced in build/macos-universal.
```

The Windows release archive holds `imkit_gallery.exe`, the `design-assets` directory, the license and third-party notices. It installs no service and writes no user configuration. `imkit_gallery --theme-file my.theme` applies a theme file at start-up and follows later edits to it ([Make your own look](../architecture/make-your-own-look.md)).

## Page map

Pages keep their numbers when the navigation changes. `--page N` and the capture scripts use them; the names are in `PageId` ([gallery.h](../../examples/gallery/gallery.h)).

| Group | Pages | Use it to |
| --- | --- | --- |
| **Overview** | Start · Before / After · Workspace | See what changes first. Workspace combines tabs, hierarchy actions, settings and choices; every request is applied by Gallery-owned state. |
| **Look** | Theme Studio · Skins · Tokens · Icons · Window frame | Build a look from four colours, replace component frames, browse 321 icons in seven sizes, try the window-frame presets. |
| **Controls** | Basic · Numbers and units · Text, colour, media · Trees and tables · Popups and layout · Composites · Rows and palette · Progress and states · Toasts | Find the control you need and copy its call. |
| **Data** | Charts · PTZ camera | Donut, bar, gauge and sparkline charts; a pan, tilt and zoom pad against a simulated camera. |
| **Workspaces** | Editor core · Video · CG · Workspace shell · Preview tiles | See timelines, inspectors, monitors and gizmos assembled from public components. |
| **Show** | Controls · Live · Rules · Schedule · Beat · Script · Surface · LED · Images | Operate-a-show components for media servers (`imkit::show`). |
| **Guidelines** | Patterns · Accessibility · Responsive | Check navigation patterns, the published semantic tree and narrow layouts. |

The [examples and recipes](examples-recipes.md) map each route to its header, CMake target, source file and what stays with the host.

## Capture the media

The film and the GIFs record the real Gallery through public Dear ImGui input: pointer moves, presses, drags, wheel and typed characters. A large cursor and click ring are drawn at the real input coordinates, and each take asserts its results (for example shared values, workspace requests, node links). This does not verify native OS input, IME, screen readers or physical DPI.

```powershell
& './tools/Invoke-SharedBuild.ps1' -Target imkit_gallery,imkit_node_editor_gallery
python tools/media/capture.py        # lossless 1080p60 takes in out/media/native, plus the GIFs
python tools/media/film.py --lang both
python tools/media/verify.py
```

Run from the repository root with FFmpeg on PATH. The film needs Python with Pillow and NumPy; none of this is an ImKit runtime dependency. Intermediate files stay in `out/media` and are not committed.

| Output | Contract |
| --- | --- |
| GIFs `docs/images/v4-*.gif` | 960×540, 20 fps (12 fps if needed), 6–8 s, at most 2 MiB. A longer take is sped up as one continuous sequence. |
| Films `website/public/media/imkit-4.0-film-{ja,en}.mp4` | 1920×1080, 60 fps, about three minutes, about −14 LUFS. Same footage and score in both languages; only the captions differ. |

## Provenance

Images and films are recorded from the Gallery or the Node Editor companion. They contain no stock footage, third-party product screens, icons, fonts or UI code. The only third-party media is the film's music, "Laserpack" by Kevin MacLeod ft. DarkNES, under CC BY 4.0 (see the notices). Dear ImGui, GLFW and optional fonts keep their licenses ([THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md)). See [validation](../reference/validation.md) for evidence and exclusions, and [window frame](../components/gallery-window-frame.md) for Frame Lab's platform boundary.
