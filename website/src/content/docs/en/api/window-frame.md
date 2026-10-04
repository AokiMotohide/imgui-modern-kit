---
title: "WindowFrame API map"
---

This map reflects the named API entries in [`docs/window-frame-api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/window-frame-api-inventory.json). The file records type and function names, not signatures; use the public header as the contract source.

Header: [`<imkit/window_frame.h>`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/window_frame.h)

## Types

- `WindowFramePreset`
- `WindowFrameStyle`
- `WindowFrameMetrics`
- `WindowFrameFeatures`
- `WindowFrameContent`
- `WindowFrameState`
- `WindowFrameLayout`
- `WindowFrameResult`
- `WindowFrameOperation`
- `WindowFrameEventType`
- `WindowFrameEvent`
- `WindowFrameRect`
- `WindowFrameContrast`

## Functions

- `MakeWindowFrameStyle`
- `LayoutWindowFrame`
- `DrawWindowFrame`
- `ElideWindowFrameTitle`
- `ValidateWindowFrameContrast`

## Optional platform targets

- **Windows:** `imkit::window_frame_win32` (`<imkit/window_frame_win32.h>`, `WindowFrameWin32Adapter`)
- **macOS:** `imkit::window_frame_macos` (`<imkit/window_frame_macos.h>`, `WindowFrameMacOSAdapter`)

See the [WindowFrame guide](../../features/window-frame/).
