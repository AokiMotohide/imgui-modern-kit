# Examples and recipes

[日本語](examples-recipes.ja.md) · [Documentation catalog](documentation-catalog.md)

Use the Gallery as a live specimen, then follow the public header and implementation linked below. Its examples run inside the Gallery host; they are not standalone widgets or a replacement for a host application's data and runtime. The shared rules are simple: link the module target, call draw APIs between `ImGui::NewFrame()` and `ImGui::Render()`, and keep the ImGui context, renderer, fonts, application state, persistence, undo and workers in the host.

## Recipes

| Module and purpose | Include and CMake target | Frame call and owned state | Gallery page, source, and next reading | Scope and constraints |
|---|---|---|---|---|
| [Components](components.md): controls and settings rows | `<imkit/components.h>` · `imkit::imkit` | `imkit::ActionButton("Apply", imkit::ActionVariant::Primary);` State and IDs are host-owned. | **Components: Basic** · [`gallery.cpp`](../examples/gallery/gallery.cpp) · [Components recipes](components.md) | Uses native ImGui item/input contracts. Views and labels are borrowed for the draw call. |
| [Icons and theme](themes.md): consistent appearance and symbols | `<imkit/theme.h>`, `<imkit/icons.h>` · `imkit::imkit` | Build a host-owned theme with `imkit::MakeTheme(...)`; apply it in the host's frame scope. The host owns theme lifetime and font atlas. | **Icons**, Appearance in the header · [`gallery.cpp`](../examples/gallery/gallery.cpp) · [Themes](themes.md), [Icons](icons.md) | Theme selection and persistence remain host-owned; icon atlas upload is the host's responsibility. |
| [Workflow and shell](workflow-components.md), [shell components](shell-components.md): guide multi-step tools and application chrome | `<imkit/workflow.h>`, `<imkit/components.h>` · `imkit::imkit` | Draw the selected component in the frame and handle returned requests in the host. Providers, model data, UI state and command dispatch belong to the host. | **Generic Workspace**, **Feedback / States**, **Preview Tiles** · [`workflow_pages.cpp`](../examples/gallery/workflow_pages.cpp), [`gallery.cpp`](../examples/gallery/gallery.cpp) · [Workflow guide](workflow-components.md) | These are UI building blocks. They do not implement persistence, background work, navigation policy or application services. |
| [Node Editor](node-editor.md): render graph snapshots and collect edits | `<imkit/node_editor.h>` · `imkit::node_editor` | Use `BeginEditor`, `DrawNodes`, and `EndEditor` during the frame with a current snapshot and caller-owned request buffer. The host validates revisions and applies requests. | Separate **Node Editor Gallery** · [`examples/node_editor/main.cpp`](../examples/node_editor/main.cpp) · [Node Editor integration](node-editor.md) | Graph storage, socket policy, evaluation, undo and persistence stay in the host. The companion material graph is a specimen. |
| [Editor Suite](editor-suite.md): reusable editing surfaces | `<imkit/editor_suite.h>` or a narrower module header · `imkit::editor_suite` | Draw with host-owned providers and event buffers inside the frame. The host applies edits and owns selection, media, undo, clocks and workers. | **Editor Core**, **Video**, **CG** · [`editor_workspaces.cpp`](../examples/gallery/editor_workspaces.cpp) · [Editor Suite](editor-suite.md), [Editor API](editor-api.md) | Video does not decode or play media; CG does not provide a general renderer. See module contracts for capacity and provider rules. |
| [Timeline editing](timeline-editing.md): fades, transitions and grouped moves | `<imkit/video.h>` · `imkit::video` (or `imkit::editor_suite`) | Call Video drawing in the frame with a host-owned `TimelineEditingProvider`; validate and commit its events in the host. | **Video** · [`editor_workspaces.cpp`](../examples/gallery/editor_workspaces.cpp) · [Timeline editing](timeline-editing.md) | Provider and event storage must remain valid for the call. Collision policy, persistence and undo are host-owned. |
| [Window frame](gallery-window-frame.md): draw a themed title area and return window operations | `<imkit/window_frame.h>` · `imkit::imkit`; optional `<imkit/window_frame_win32.h>` or `<imkit/window_frame_macos.h>` · matching OS target | Draw with explicit style/content/state in the frame; execute the returned operation in the host. The host owns the native window and event loop. | **Frame Lab** · [`gallery.cpp`](../examples/gallery/gallery.cpp) · [WindowFrame guide](gallery-window-frame.md) | Core drawing does not own a native window. Win32/Cocoa integration is an optional platform target; platform input behavior needs native acceptance. |

## A small integration shape

The following is a placement sketch, not a complete application or build file. It uses the public `ActionButton` call; see the Gallery source for a complete compile-checked implementation.

```cpp
// The host has already created the matching Dear ImGui context and backends.
ImGui::NewFrame();
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary)) {
    // Handle the request using host-owned application state.
}
ImGui::Render();
```

For exact overloads, defaults and ABI requirements use [Public API coverage](api-coverage.md) and [Dependencies](dependencies.md). For buildable consumer integration use [Getting started](getting-started.md) and [`examples/consumer`](../examples/consumer/CMakeLists.txt). The consumer target is a compile/link smoke example; the Gallery is the interactive learning example.
