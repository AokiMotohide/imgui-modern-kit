# ImKit 3.0 migration

[日本語](v3移行ガイド.md) · [English](migration-v3.md)

1. Update the host to Dear ImGui `docking` commit
   `367b2c24f399988ddafc0bb4628da0106bcc09be`, including matching official backends
   and Test Engine sources.
2. Include `<imkit/preview_opengl3.h>` for `GLFunctions` and `OpenGL3Renderer`.
   `<imkit/preview.h>` now contains renderer-neutral types only. On macOS link
   `imkit::preview_metal` and pass borrowed `MTLDevice`/command buffers.
3. Replace `Win32ActionSink` with `NativeActionSink`. The callback returns `bool` and
   receives an optional UTF-8 value. Link the matching `accessibility_win32` or
   `accessibility_macos` target.
4. Remove `IMKIT_BUILD_WINDOW_FRAME_LEGACY_IMGUI` and legacy target references.
   All consumers use the normal v3 targets and the exact pinned Dear ImGui ABI.
5. Installed consumers set `IMKIT_SDK_ABI_CONFIRMED=ON` only after matching the
   recorded OS, architecture, compiler, ImGui revision and `imconfig` ABI.
6. Link `imkit::node_editor` explicitly when adopting the new Node Editor. Move
   graph data, revision checks, edit application, Undo, evaluation and persistence
   into the host; do not treat the companion Material Graph model as library state.

## Deprecations in 3.4

| Deprecated | Use instead |
|---|---|
| `StatusBadge(text, kind, const Theme*)` | `StatusBadge(text, kind, ComponentOptions{&theme})` |
| `NotificationCard(Notification, now, const Theme*)` | `NotificationCard(Notification, now, ComponentOptions{&theme})` or the `FeedbackView` overload |
| `ValidationMessage(message, invalid, const Theme*)` | `ValidationMessage(message, invalid, ComponentOptions{&theme})` |
| `EmptyState(message)` | `EmptyState(id, StateView, options)` |
| `Progress(id, fraction, label, options)` | `Progress(id, ProgressView, ProgressPresentation, ...)` or `ProgressTrack` |
| `ToastRegion(span<Notification>, now, options)` | `ToastViewport` |

The old spellings still compile and behave as before; they only warn. Define `IMKIT_DEPRECATED(message)` as nothing to silence the warnings. Removal is planned for the next major version. Other 3.4 changes that can affect hosts: `SemanticRole` gained `Slider` and `Meter`, `RangeSlider` publishes `Slider` nodes instead of `Group`, and `PtzState` no longer exposes `held` and `padValid`.
