# Widget inventory

[日本語](ウィジェット一覧.md) · [Components](../components/components.md) · [API coverage](api-coverage.md)

**Precision Layers** is the selected implementation. The authoritative overload-level mapping is [api-coverage.md](api-coverage.md), backed by [api-inventory.json](api-inventory.json) and `tests/api_compile.cpp`.

## Core widget inventory

Dear ImGui standard widgets styled with the ImKit Precision Layers design system:

| Family | Method | Catalog |
|---|---|---|
| Named themes | 13 discoverable host-owned presets; stable IDs; no global selection or persistence | Theme presets |
| Text, links, buttons, checkbox/radio, combo/list, progress | Shared semantic style; native behavior | Basic / Selection |
| Drag, slider, scalar input; all types, vector lengths, ranges | Shared input/selection/focus tokens; native precision/parsing | Numeric / Units |
| Text input, hint, multiline, callbacks | Native input and IME contract; optional validation decoration | Input / Media |
| Color edit/picker/button, image variants, plots | Native rendering with theme surfaces/borders/accent | Input / Media |
| Tree, collapsing header, selectable, tabs, tables | Native behavior; selected tree/row/tab underline; table colors | Hierarchy / Table |
| Menu, tooltip, popup/context popup/modal | Raised surface; native stacks; optional inset elevation | Overlay / Layout |
| Window, child, layout, scrolling, docking | Theme style and host-owned layout | Overlay / Layout |
| Switch, mixed checkbox, segments, searchable combo | Native input composition; explicit host-owned values | Composites / Basic |
| Unit scalar/vector, setting row, badge, notification, toolbar | Small compositions with theme/font-derived dimensions | Composite controls / Numeric / Overlay |

> [!NOTE]
> Legacy column helpers are available, but Dear ImGui native tables are preferred for new implementations.

## Editor extension inventory

- Generic workflow additions are listed in [workflow-components.md](../components/workflow-components.md).
- The custom editor API is inventoried separately in [editor-api.md](editor-api.md). Its compile/link fixture is `tests/editor_api_compile.cpp`, also compiled against an external host ImGui target. The generated native inventory is unchanged.

## Component audit decisions

| Candidate | Decision | Reason |
|---|---|---|
| Step Navigator, Navigation Rail, filter chip | Add | Host state and request IDs |
| Notification, progress, empty/error, toolbar | Extend | Existing patterns and tokens |
| Image viewport, overlays | Extend | Shared Canvas and selection coordinates |
| Preview tile/strip | Add | Borrowed textures, existing Splitter |
| Card, collapsible section | Add | Theme surface and explicit open state |
| SettingRow, PropertyGrid, StatusBadge, Skeleton, divider, spacer | Reuse | Avoid aliases; reuse existing APIs |
| Sort/filter table headers, row actions | Reuse | DataTable/TreeDataGrid, native table APIs |
| Virtual list/grid | Reuse | VirtualList/AssetBrowser providers |
| Tree/list reorder, generic drag feedback | Defer | Native drag-drop available; move constraints belong to host |
| Multi-selection action bar | Add | Selection count and command view |
| Zoom toolbar | Add | Shared image state |
| Minimap/overview | Defer | Separate overview-content contract not introduced |
| Domain processing, device control, persistence | Exclude | Outside UI-library responsibility |
