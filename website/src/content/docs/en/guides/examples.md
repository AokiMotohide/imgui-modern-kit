---
title: "Examples and recipes"
description: "Find the area you want to build, then follow the header, the CMake target, and the Gallery page whose source you can read."
---

Find the area you want to build, then follow the header, the CMake target, and the Gallery page whose source you can read. The Gallery runs the same public components that your application links, so its source is a working reference. It is not a replacement for your application's data and runtime.

Three rules apply to every area:

1. Link the module target, and call the draw functions between `ImGui::NewFrame()` and `ImGui::Render()`.
2. Your application owns the Dear ImGui context, renderer, fonts, state, saving, Undo and workers.
3. Labels, spans and views are borrowed for the call. Results come back as return values or requests.

## Recipes

| I want to build | Header and target | Gallery page and source | Read |
| --- | --- | --- | --- |
| Settings rows, buttons, inputs | `<imkit/components.h>` · `imkit::imkit` | **Basic controls**, **Rows and palette** · `examples/gallery/gallery.cpp` | [Components](../../features/components/) |
| A theme, brand colours, a custom look | `<imkit/theme.h>`, `<imkit/skin.h>` · `imkit::imkit` | **Theme Studio**, **Skins** · `examples/gallery/look_pages.cpp` | [Themes](../../features/themes/), [Make your own look](../../features/make-your-own-look/) |
| Icons | `<imkit/icons.h>` · `imkit::imkit` | **Icons** · `examples/gallery/gallery.cpp` | [Icons](../../features/icons/) |
| Charts: donut, bar, gauge, sparkline | `<imkit/charts.h>` · `imkit::imkit` | **Charts** · `examples/gallery/charts_page.cpp` | [Charts](../../features/charts/) |
| PTZ camera control, range slider | `<imkit/ptz.h>`, `<imkit/range_slider.h>` · `imkit::imkit` | **PTZ camera**, **Numbers and units** · `examples/gallery/ptz_page.cpp` | [PTZ and range sliders](../../features/ptz/) |
| Steps, tabs, hierarchy, progress, empty states | `<imkit/workflow.h>` · `imkit::imkit` | **Workspace**, **Progress and states**, **Workspace shell**, **Preview tiles** · `examples/gallery/workflow_pages.cpp` | [Workflow components](../../features/workflow/), [Shell components](../../features/shell/) |
| Toast notifications | `<imkit/toast.h>` · `imkit::imkit` | **Toasts** · `examples/gallery/gallery.cpp` | [Toasts](../../features/toasts/) |
| A node graph | `<imkit/node_editor.h>` · `imkit::node_editor` | **Node Editor Gallery** · `examples/node_editor/main.cpp` | [Build a node editor](../../tutorials/node-editor/), [Node Editor](../../features/node-editor/) |
| A timeline, curves, an inspector, previews | `<imkit/video.h>`, `<imkit/cg.h>` · `imkit::editor_suite` (or `imkit::video`, `imkit::cg`) | **Editor Core**, **Video**, **CG** · `examples/gallery/editor_workspaces.cpp` | [Build a timeline](../../tutorials/timeline/), [Editor Suite](../../features/editor-suite/) |
| A live-show control surface, cue list, LED tools | `<imkit/show.h>` · `imkit::show` | **Show** group: **Live**, **Surface**, **LED** and more · `examples/gallery/show_*_page.cpp` | [Show module](../../features/show/) |
| A themed title bar | `<imkit/window_frame.h>` · `imkit::imkit`, optionally `imkit::window_frame_win32` or `imkit::window_frame_macos` | **Window frame** · `examples/gallery/gallery.cpp` | [WindowFrame](../../features/window-frame/) |

## Two short shapes

Place a control in your frame:

```cpp
// Your application has already created the Dear ImGui context and backend.
ImGui::NewFrame();
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary)) {
    // Handle the request with your own state.
}
ImGui::Render();
```

Draw a chart from values you already hold:

```cpp
const std::array<imkit::ChartDatum, 3> usage{{{"Render", 42}, {"Cache", 24}, {"Other", 6}}};
imkit::DonutChartView view;
view.segments = usage;
view.value = "72 GB";            // borrowed for the frame
view.caption = "Used";
const int clicked = imkit::DonutChart("storage", view, {}, {&theme, &animation});
if (clicked >= 0) { /* your code decides what the click means */ }
```

## Where to go next

- Exact overloads and defaults: [Public API coverage](../../api/native/).
- Build and ABI requirements: [Dependencies](../../platform/dependencies/).
- A buildable integration: [Getting started](../../getting-started/) and [`examples/consumer`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/examples/consumer/CMakeLists.txt). The consumer target is a compile and link smoke test, and the Gallery is the interactive example.
