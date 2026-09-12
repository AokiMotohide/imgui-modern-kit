# Product Gallery

[日本語](gallery.ja.md)

The native Gallery is both an onboarding tool and a public-API specimen. It does not contain replacement components that are unavailable from the installed library.

## Navigation

- **Start:** design overview, 30-second integration and direct routes.
- **Components:** actions, numeric controls, input/media and composed controls.
- **Patterns:** hierarchy/data and overlay/layout examples.
- **Themes & Icons:** twelve complete presets, palette editing and searchable icons.
- **Editor Examples:** evolving Editor Core, Video and 3D workspaces.

The header search matches component names, API names and use-case keywords. Every stable component page includes a live specimen, a minimal copy action and an ownership reminder. The sidebar becomes a compact selector below desktop width.

## Build and run

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target imkit_gallery --parallel
./build/windows-debug/catalog/Debug/imkit_gallery.exe
```

A normal launch opens the ImKit Gallery and the official Dear ImGui Demo Window as two movable,
resizable windows on the same canvas. They start side by side on a wide display and retain normal
Dear ImGui docking behavior. The Gallery uses the selected ImKit theme; the official demo keeps the
unmodified Dear ImGui style for direct comparison. Close or reopen the demo with the
**Dear ImGui Demo** checkbox in the Gallery header.

## Verification and captures

```powershell
./build/windows-debug/catalog/Debug/imkit_gallery.exe --verify --output out/catalog
./build/windows-debug/catalog/Debug/imkit_gallery.exe --capture --output out/catalog
```

These use public Dear ImGui IO and real OpenGL backbuffers. They are not native OS/IME automation.
Capture and verification modes retain the deterministic full-canvas Gallery layout and do not open
the Dear ImGui Demo Window.

## Rebuild the README GIF

```powershell
./build/windows-debug/catalog/Debug/imkit_gallery.exe --capture-readme --output out/readme
python tools/build_readme_gif.py out/readme/readme-frames docs/images/gallery-overview.gif
```

The deterministic sequence captures 120 native 960×540 frames and encodes twelve seconds at 10 fps. Pillow is a documentation-tool dependency only; it is not linked, installed or exposed to consumers. The checked-in GIF must stay below 8 MiB.

The Windows Gallery also includes **Frame Lab** for the public cross-platform window-frame values and optional Win32 adapter. See [Public window frame](gallery-window-frame.md). Existing automated captures retain the native OS frame unless frame verification is explicitly requested.
