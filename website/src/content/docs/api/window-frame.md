---
title: "WindowFrame API map"
---

このmapは[`docs/window-frame-api-inventory.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/window-frame-api-inventory.json)に記録された名前を示します。型名と関数名のみの一覧でsignatureは含みません。契約は公開headerを基準にしてください。

Header: [`<imkit/window_frame.h>`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/window_frame.h)

## 型

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

## 関数

- `MakeWindowFrameStyle`
- `LayoutWindowFrame`
- `DrawWindowFrame`
- `ElideWindowFrameTitle`
- `ValidateWindowFrameContrast`

## 任意のplatform target

- **Windows:** `imkit::window_frame_win32` (`<imkit/window_frame_win32.h>`, `WindowFrameWin32Adapter`)
- **macOS:** `imkit::window_frame_macos` (`<imkit/window_frame_macos.h>`, `WindowFrameMacOSAdapter`)

[WindowFrameガイド](../../features/window-frame/)も参照してください。
