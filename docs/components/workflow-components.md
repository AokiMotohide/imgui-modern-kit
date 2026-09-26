# Generic workflow components

[日本語](ワークフローコンポーネント.md)

Include `imkit/workflow.h` (also included by `imkit/imkit.h`) and link `imkit::imkit`. For images and previews, include `imkit/editor_canvas.h` and link `imkit::editor_core`. These additive APIs reuse patterns, Canvas, Selection and Splitter. They do not establish completion of the whole Editor Suite.

## Ownership

Labels, textures, candidate views and scratch buffers are borrowed. IDs must be nonzero and unique per instance, except notifications: the last duplicate wins. State and queues belong to the host. No texture upload, worker, persistence, registry, undo engine, dependency or icon asset is added. End every scope before destroying its context. Semantic text must remain valid through host frame publication.

Reset event buffers each frame. Full buffers set `overflow`. Keyboard tile resizing requires three slots for Begin/Update/Commit. Mouse cancellation restores both adjacent sizes; terminal events are retried if the buffer is full. Tile sizes are host-owned, ID-keyed entries. Notification sorting uses caller index scratch and limits results to the smaller of scratch capacity and requested maximum.

## API inventory

`tests/workflow_api_compile.cpp` is the compile/link inventory, also built by source and relocated SDK consumers. Existing signatures and the native API inventory are retained.

| API | Contract |
|---|---|
| `StepNavigator`, `NavigationRail` | Borrowed StepItem span, current ID, explicit focus state; returns a selection request |
| `WorkspaceTabs` | Icon and label navigation; collapses to a combo when space is narrow; returns a selection request |
| `HierarchyGroupHeader`, `HierarchyRow` | Host-owned group state and stable row IDs; selection, visibility, lock and optional group action are requests; unavailable visibility or lock actions may be omitted |
| `BeginInspectorCard`, `EndInspectorCard` | Groups related controls in an auto-height bordered card; always pair Begin/End |
| `SettingToggleRow` | Label, description and optional disabled reason; returns a toggle request without changing host state |
| `FilterChip`, `SectionHeader` | Explicit selection/open state, native activation |
| `SelectNotifications` | Last duplicate wins before expiry filtering; priority descending, stable ties |
| `NotificationCard(FeedbackView)`, `InlineAlert`, `PersistentBanner` | Semantic status and dismiss request; banner persists |
| `ToastRegion(id, ...)` | Host queue/time/scratch, work-area-clamped stack, no focus stealing |
| `EmptyState(id, StateView)`, `UnavailableState`, `RetryState` | Heading, description, optional icon/action; returns a request |
| `Progress(ProgressView, ...)` | Inline, child overlay, modal; negative fraction means indeterminate; cancel request |
| `CircularProgress(CircularProgressView, ...)` | Borrowed fraction/value/label; negative or non-finite fraction means unavailable; no threshold or state ownership |
| `BeginCard`, `EndCard` | Always paired, even when Begin returns false |
| `MultiSelectionBar`, `HelpCallout`, `ValidationSummary` | Host count, commands or issues; no validation engine |
| `ResponsiveToolbar(..., ToolbarOptions)` | Overflow, icon labels, disabled reasons, native keyboard focus |
| `ResolveRightSidePanelLayout`, `RightSidePanelHandle` | Host-owned open/width state, compact edge toggle and mouse/keyboard resize; host scopes shortcut requests |
| `ImageGeometryValid`, `FitImage`, `ClampImage` | Numeric image layout independent of texture availability |
| `ResolveImagePlacement` | Pure Fit/Fill/Stretch destination and UV crop resolver; invalid geometry is rejected |
| `PixelToNormalized`, `NormalizedToPixel` | Divide/multiply by image dimensions; invalid dimensions return zero |
| `BeginImageViewport`, `EndImageViewport` | Existing Canvas, borrowed texture, uniform zoom; always paired |
| `ZoomToolbar` | Fit/fill/1:1, zoom and pan; updates display state |
| `DrawOverlay` | Point/polyline/rectangle/circle/label, selected/hovered decoration, clipped |
| `PreviewTile`, `ResizableTileStrip` | Borrowed previews/actions, horizontal/vertical scroll, minimum extents |
| `RequestBuffer::Push`, `TileEventBuffer::Push` | Caller storage, explicit overflow |

Image coordinates start at top left, x right and y down; Canvas units are image pixels. Use the existing `ToScreen`/`FromScreen` for desktop-screen conversion. Fit shows the whole image, Fill crops, and 1:1 maps one image pixel to one ImGui coordinate unit (not necessarily one physical display pixel). Clamp centers small images and limits large images to their edges. Cursor-anchored zoom is preserved until edge clamping is needed. Resizing refits non-manual modes. `CanvasSelection` handles marquee and lasso selection.

`ResolveImagePlacement` uses local coordinates whose origin is the available region's top-left. Fit returns a centered destination with full UVs, Fill returns the full destination with centered crop UVs, and Stretch uses both complete regions. It owns no texture or renderer and returns `valid=false` for non-positive, non-finite or non-drawable geometry.

Pass `ComponentOptions` for theme, locale and semantic publication. Draw-only overlays do not create interactive object nodes; the host describes edited objects. Step arrows skip disabled items; native Tab/Shift-Tab and activation remain intact. Splitter additionally supports arrow-key resizing. Reduced motion uses existing static indicators; no new transitions are introduced.

`ResolveRightSidePanelLayout` returns widths for content, the always-visible edge handle and the optional panel. Draw those regions in that order and pass the same available extent to `RightSidePanelHandle`. The host decides whether an `N` key or another shortcut becomes `toggleRequested`; this prevents the library from stealing text input or shortcuts from unrelated editors. The handle remains visible while collapsed, and the panel reduces content width instead of covering it.

## Gallery

Open **Generic Workspace**, **Feedback / States**, or **Preview Tiles**. The workspace combines navigation, toolbar, image, overlays, selection, feedback and status. It uses the existing procedural texture. `--verify-workflow --output <directory>` runs public IO checks and captures 36 page/theme combinations plus two narrow Japanese/disabled examples. Native GPU captures are distinct from native OS/IME and screen-reader tests.
