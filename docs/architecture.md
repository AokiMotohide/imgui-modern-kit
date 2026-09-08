# Architecture

## Boundary

`imkit` is a static C++20 library containing public declarations and direct calls to six public Dear ImGui widget functions. Its public link to the target named by `IMKIT_IMGUI_TARGET` supplies both the Dear ImGui headers exposed by `imkit` and the implementation selected by the host. The library does not compile Dear ImGui sources and does not link GLFW, OpenGL, a renderer backend, or an operating-system API.

The host creates, selects, and destroys every `ImGuiContext`. It also starts and ends frames and owns all values passed to widgets. `imkit` has no initialization or shutdown API and stores no mutable global or per-context state.

## Standalone development

When this repository is the top-level project and no external target is named, `cmake/Dependencies.cmake` fetches the pinned Dear ImGui source and creates the development-only `imkit_bundled_imgui` target. This target remains separate from `imkit`.

`imkit_gallery` is a host application. It owns GLFW, the OpenGL context, the Dear ImGui context, backend lifecycle, frame loop, and demo values. Its backend adapter target is the only ImKit-owned target that links GLFW and OpenGL. Tests own their contexts in the same manner but create no operating-system window.

When added as a subdirectory, Gallery and tests default to off and `IMKIT_IMGUI_TARGET` is mandatory. This path neither downloads dependencies nor modifies the host's Dear ImGui configuration.

## Deferred design

Themes, modern styling, animation state, composite controls, internal widget rendering, and any state-management class are deferred until their concrete requirements are known. They are not represented by placeholder abstractions in this foundation.

