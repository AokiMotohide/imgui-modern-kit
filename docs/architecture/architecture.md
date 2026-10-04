# Architecture

[日本語](アーキテクチャ.md) · [Documentation index](../README.md)

## Layout

The workflow components extend patterns. Image and preview components belong to Editor Core and reuse Canvas/Selection/Splitter. The base `imkit` does not depend on Editor Core. See [workflow-components.md](../components/workflow-components.md) for the public contract.

The cross-cutting design-system additions, explicit semantic frame, locale and provider contracts are documented in [design-system.md](design-system.md). Legacy palette/metrics are rendering projections of the new semantic tokens when using `MakeTheme`/`ResolveTheme`.

Precision Layers separates semantic design, native behavior and host state.

## Platform boundary

The core remains platform and renderer neutral. Official Dear ImGui backends are compiled only by the Gallery host: GLFW/OpenGL3 on Windows and GLFW/Metal on macOS. The optional `preview_opengl3` and `preview_metal` targets own only their explicit off-screen GPU resources; the host owns contexts, devices, command buffers and submission. The `accessibility_win32` and `accessibility_macos` adapters copy a published semantic snapshot and return actions through the same `NativeActionSink`.

| Layer | Responsibility |
|---|---|
| `version.h` | Explicit baseline guard; no silent cross-version ABI claim |
| `theme.h`, `theme.cpp` | Enumerated named presets, copyable palette/metrics/fonts/motion, deterministic style derivation, nested RAII |
| `native.h` | Exact overload sets imported from the host's public header |
| `widgets.h`, `widgets.cpp` | Original six compatible functions, pointer Selectable, selection/tree/tab markers via public DrawList |
| `components.h`, `components.cpp` | Small native compositions; optional theme/animation passed explicitly |
| `node_editor.h`, `node_editor.cpp`, `node_layout.cpp` | Graph snapshots, bounded requests and deterministic layout; no graph storage or evaluation |
| Catalog host | Context, fonts, GLFW/OpenGL, image capture and representative inputs |

`imkit` compiles only its own implementation. It does not compile Dear ImGui, link a backend, initialize a context, discover fonts, persist settings or spawn workers. Public wrappers preserve the native Begin/End, focus, callback, disabled, clipping and ID contracts. Decoration submits no replacement item; compound controls use native groups.

## Theme and state

There is no global theme registry. `ThemePresets()` is immutable discovery metadata and `MakeTheme()` returns a copyable host-owned value; neither stores a current selection. `FontSet` contains non-owning references, and `AnimationState` is fixed-capacity storage whose lifecycle the host controls explicitly. A scope must end before its context is destroyed. The static SDK binds to the consumer's already-created ImGui target through an imported interface adapter.

`window_frame.h` follows the same boundary: style, metrics, features, content and state are explicit values. Content strings and spans are borrowed for one draw call. The core computes layout, draws through the current Dear ImGui context and returns a typed request; it owns no native window or platform input. `window_frame_win32` and `window_frame_macos` are separately built and exported adapters that borrow an `HWND` or Cocoa window. They are installed only on their matching platform and add no Windows/Cocoa dependency to `imkit::imkit`.

## Extension policy

Add overloads only after comparing the pinned public signature and preserving defaults and return semantics. Regenerate the API inventory and compile/link fixture with `tools/generate_api.py`. Supporting an unsupported new Dear ImGui version requires a deliberate adapter/style review, not just relaxing the version guard. Keep rendering dimensions derived from theme and font size, keep state bounded, and leave native editing intact. Do not introduce host-specific data or services into this library.

## Editor module ownership

The Node Editor is an independent optional target. It borrows a graph snapshot for one frame and emits bounded edit requests. Dynamic socket policy, compatibility decisions, revision acceptance, model mutation, preview computation, Undo and persistence remain host-owned. The Material Graph companion is a GUI integration specimen, not a renderer or shader system.

Editor Core, Video and CG consume non-owning provider views and emit fixed-buffer events. They do not own edited scene/media data, selection, Undo, workers or context. The explicitly constructed optional OpenGL3 preview object owns its own graphics resources only; its context and GL function table originate from the host. See [Editor Suite contracts](../components/editor-suite.md).

CG transforms retain rotation, scale and upper-triangular shear for oriented non-uniform scaling. Scale events carry the affine terms explicitly; the host applies them atomically with scale. Both preview paths use the same full linear map and inverse-transpose normals.
