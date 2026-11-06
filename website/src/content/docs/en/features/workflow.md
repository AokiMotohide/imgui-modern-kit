---
title: "Workflow components"
description: "imkit/workflow.h (included by imkit/imkit.h) holds the parts of a tool's workflow screens: navigation, grouped settings, hierarchy rows, feedback, pro…"
---

`imkit/workflow.h` (included by `imkit/imkit.h`) holds the parts of a tool's workflow screens: navigation, grouped settings, hierarchy rows, feedback, progress, and an image viewer with preview tiles. Link `imkit::imkit`. The image viewer also needs `imkit/editor_canvas.h` and `imkit::editor_core`, because it reuses the editor canvas, selection and splitter.

![The Gallery Workspace page: tabs, hierarchy, inspector cards and settings on one screen](/imgui-modern-kit/docs-images/v4-workspace-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-workspace.gif)

## Components

Every control returns a request. It never changes the value you pass in.

| Group | Components | What they do |
| --- | --- | --- |
| Navigation | `StepNavigator`, `NavigationRail` | Step bar and vertical rail over a borrowed `StepItem` span; return a selection request |
| | `WorkspaceTabs` | Icon and label tabs; collapse to a combo when narrow |
| | `ChoiceGroup` | Bordered heading with one to three columns of radio choices; returns the chosen stable ID |
| Hierarchy and settings | `HierarchyGroupHeader`, `HierarchyRow` | Scene-graph style rows with full-width selection, ellipsized labels and an optional `accentColor` stripe. Selection, visibility, lock and group actions are requests. An unavailable visibility or lock action can be omitted |
| | `BeginInspectorCard` / `EndInspectorCard` | Groups related controls in an auto-height bordered card. Always pair them |
| | `BeginCollapsibleInspectorCard` / `EndCollapsibleInspectorCard` | The same card with a host-owned open state. Call End only when Begin returns true. Closed content is not drawn |
| | `SettingToggleRow` | Label, description and optional disabled reason; returns a toggle request |
| | `FilterChip`, `SectionHeader` | Explicit selection or open state with native activation |
| Feedback | `NotificationCard`, `InlineAlert`, `PersistentBanner` | Semantic status with a dismiss request. A banner stays until you remove it |
| | `SelectNotifications` | Filters expired items and sorts by priority, newest duplicate wins |
| | `ToastRegion` | Host-owned queue and time; a stack clamped to the work area that never takes focus. For coloured cards see [Toasts](../toasts/) |
| | `EmptyState`, `UnavailableState`, `RetryState` | Heading, description, optional icon and action; return a request |
| | `MultiSelectionBar`, `HelpCallout`, `ValidationSummary` | Show a host count, commands or issues. There is no validation engine |
| Progress | `Progress`, `ProgressTrack`, `StageProgress`, `TaskProgressCard`, `CircularProgress` | See [Progress](#progress) |
| Layout | `BeginCard` / `EndCard` | Always pair them, even when Begin returns false |
| | `ResponsiveToolbar` | Overflow menu, icon labels, disabled reasons and native keyboard focus |
| | `ResolveRightSidePanelLayout`, `RightSidePanelHandle` | A collapsible side panel with an edge handle and mouse or keyboard resizing |
| Image viewer | `BeginImageViewport` / `EndImageViewport` | Pan and uniform zoom over a borrowed texture. Always pair them |
| | `ZoomToolbar`, `DrawOverlay` | Fit, fill and 1:1 with zoom and pan. Point, polyline, rectangle, circle and label overlays, clipped to the view |
| | `ResolveImagePlacement`, `FitImage`, `ClampImage`, `ImageGeometryValid`, `PixelToNormalized`, `NormalizedToPixel` | Pure geometry that does not need a texture |
| | `PreviewTile`, `ResizableTileStrip` | Borrowed previews and actions, scrolling in either direction, minimum sizes |

`tests/workflow_api_compile.cpp` is the compile and link inventory for these APIs. It is also built by the source and relocated-SDK consumers. `docs/reference/api-inventory.json` lists the Dear ImGui aliases separately.

## Ownership and lifetime

- Labels, textures, candidate views and scratch buffers are borrowed for the call. State and queues belong to your application.
- IDs must be nonzero and unique per instance. The exception is notifications, where the last duplicate wins.
- `ChoiceGroup` borrows its items and an optional `IconAtlas`. The returned ID is a request and never changes your current value.
- Nothing here uploads textures or starts workers. There is no persistence, registry or undo engine.
- End every scope before you destroy its context. Text published for accessibility must stay valid until your frame is published.
- Reset event buffers every frame. A full buffer sets `overflow`. Resizing a tile from the keyboard needs three slots for Begin, Update and Commit. Cancelling with the mouse restores both neighbouring sizes, and terminal events are retried when the buffer is full.
- Tile sizes are host-owned entries keyed by ID. Notification sorting uses your index scratch and returns at most the smaller of its capacity and the requested maximum.
- Pass `ComponentOptions` for the theme, locale and semantic publication. Draw-only overlays create no interactive nodes, so you describe the edited objects yourself. Step arrows skip disabled items, and native Tab, Shift+Tab and activation keep working. With reduced motion, static indicators replace transitions.

## Progress

`ProgressTrackView` is the shared source for the stage label, status text, percentage (or your `valueText`), the rounded bar and supplemental text. `Progress` and `TaskProgressCard` reuse it, so no card or toast builds a second percentage or status line.

| Item | Behaviour |
| --- | --- |
| `ProgressStatus` | `Queued`, `Running`, `Paused`, `Succeeded`, `Failed`, `Cancelled` |
| Indeterminate | A negative or non-finite fraction. Only a `Running` track moves a short segment, and `Theme.motion.enabled`, `reducedMotion` and your `AnimationState` control the motion |
| `Progress` | Inline, child overlay or modal, with a cancel request |
| `StageProgress` | Displays the stages and the current ID you pass. When narrow, it shows the current stage and `n/total` |
| `TaskProgressCard` | Regular or compact summary (`TaskProgressCardOptions::compact`). Returns the clicked `Command.id`, and 0 means no request. Strings and the operation span are borrowed for the frame |
| `CircularProgress` | Borrowed fraction, value and label. A negative or non-finite fraction means unavailable. It has no threshold and owns no state |

```cpp
imkit::ProgressTrackView progress;
progress.fraction = fraction;     // negative or non-finite means indeterminate
progress.status = imkit::ProgressStatus::Running;
progress.label = "Inspecting files";
progress.supplemental = "The host owns progress updates.";
imkit::ProgressTrack("import-progress", progress, {&theme, &animation});
```

![Progress tracks, stages and task cards in the Gallery](/imgui-modern-kit/docs-images/v4-progress-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-progress.gif)

## Image viewer

Image coordinates start at the top left, with x to the right and y down. Canvas units are image pixels, and `ToScreen` and `FromScreen` convert to desktop-screen coordinates.

| Mode | Result |
| --- | --- |
| Fit | The whole image is visible |
| Fill | The image fills the view and is cropped |
| 1:1 | One image pixel is one ImGui coordinate unit, not necessarily one physical pixel |

`ClampImage` centers a small image and keeps a large one within its edges. Cursor-anchored zoom is kept until edge clamping is needed, and resizing refits the modes that are not manual. `CanvasSelection` handles marquee and lasso selection.

`ResolveImagePlacement` works in local coordinates with the origin at the top left of the available region. Fit returns a centered destination with full UVs, Fill returns the full destination with centered crop UVs, and Stretch uses both whole regions. It owns no texture or renderer, and returns `valid = false` for non-positive, non-finite or non-drawable geometry.

## Right side panel

`ResolveRightSidePanelLayout` returns widths for the content, the always-visible edge handle and the optional panel. Draw them in that order, and pass the same available extent to `RightSidePanelHandle`. You decide whether a key such as `N` becomes `toggleRequested`, so the library never steals text input or shortcuts from other editors. The handle stays visible while collapsed, and the panel narrows the content instead of covering it.

## Try it in the Gallery

Open **Workspace**, **Workspace shell**, **Progress and states** or **Preview tiles**. **Workspace** combines WorkspaceTabs, hierarchy rows, inspector cards, `SettingToggleRow`, `DragVector3WithUnit`, `ChoiceGroup` and `CompactActionRow`, and updates Gallery-owned values from the returned requests. The same pattern works in an existing ImGui frame. See the [3.2 declarations and lifetime rules](../../api/v3-2/). The source is `examples/gallery/workflow_pages.cpp`.

`--verify-workflow --output <directory>` runs public IO checks and captures 36 page and theme combinations plus two narrow Japanese and disabled examples. These are native GPU captures. They are separate from native OS, IME and screen-reader tests.
