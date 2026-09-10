# Widget inventory / 部品一覧

**Precision Layers** is the selected implementation. The authoritative overload-level mapping is [api-coverage.md](api-coverage.md), backed by [api-inventory.json](api-inventory.json) and `tests/api_compile.cpp`.

採用実装はPrecision Layersです。標準GUI部品と組み合わせに必要な公開補助をoverload単位で対応表に記載しています。分類は「共通Themeと標準描画」「公開DrawList装飾」「標準部品の合成」「非視覚補助」「対象外」です。

| Family / 部品群 | Method / 実装 | Catalog |
|---|---|---|
| Named themes / 名前付きTheme | 12 discoverable host-owned presets; stable IDs; no global selection or persistence | Theme presets |
| Text, links, buttons, checkbox/radio, combo/list, progress | Shared semantic style; native behavior | Basic / Selection |
| Drag, slider, scalar input; all types, vector lengths, ranges | Shared input/selection/focus tokens; native precision/parsing | Numeric / Units |
| Text input, hint, multiline, callbacks | Native input and IME contract; optional validation decoration | Input / Media |
| Color edit/picker/button, image variants, plots | Native rendering with theme surfaces/borders/accent | Input / Media |
| Tree, collapsing header, selectable, tabs, tables | Native behavior; selected tree/row/tab underline; table colors | Hierarchy / Table |
| Menu, tooltip, popup/context popup/modal | Raised surface; native stacks; optional inset elevation | Overlay / Layout |
| Window, child, layout, scrolling, docking | Theme style and host-owned layout | Overlay / Layout |
| ID, focus, keyboard, item status, clipping, drag/drop | Transparent public helpers; contextual visual effects | Shared |
| Switch, mixed checkbox, segments, searchable combo | Native input composition; explicit host-owned values | Composites / Basic |
| Unit scalar/vector, setting row, badge, notification, toolbar | Small compositions with theme/font-derived dimensions | Composite controls / Numeric / Overlay |

Legacy column helpers are available but tables are preferred. Context/frame lifecycle, renderer/platform functions, debug/demo windows, allocation, logging and ini persistence remain host responsibilities. Obsolete declarations and private implementation APIs are not wrapped. The base design still applies when native widgets are mixed into an ImKit-themed window.

旧column補助も公開しますが、新規コードはtableを推奨します。Context/frame、renderer/platform、debug/demo、allocator、logging、ini永続化はホスト責務です。obsolete/private APIをラップしません。ImKitのTheme適用中に標準ImGui部品を混在させても共通styleは適用されます。

## Editor extension inventory

Generic workflow additions are listed in [workflow-components.md](workflow-components.md).
汎用ワークフローの追加APIは上記一覧を参照してください。

The custom editor API is inventoried separately in [editor-api.md](editor-api.md). Its compile/link fixture is `tests/editor_api_compile.cpp`, also compiled against an external host ImGui target. The generated 365-overload native inventory is unchanged.

## Generic component audit / 汎用部品監査

| Candidate / 候補 | Decision / 分類 | Reason / 理由 |
|---|---|---|
| Step Navigator, Navigation Rail, filter chip | Add / 追加 | Host state and request IDs / ホスト状態と要求ID |
| Notification, progress, empty/error, toolbar | Extend / 拡張 | Existing patterns and tokens / 既存patternsとtoken |
| Image viewport, overlays | Extend / 拡張 | Shared Canvas and selection coordinates / Canvas座標を共用 |
| Preview tile/strip | Add / 追加 | Borrowed textures, existing Splitter / 借用textureとSplitter |
| Card, collapsible section | Add / 追加 | Theme surface and explicit open state / Themeと明示状態 |
| SettingRow, PropertyGrid, StatusBadge, Skeleton, divider, spacer | Reuse / 既存APIで充足 | Avoid aliases / 別名APIを増やさない |
| Sort/filter table headers, row actions | Reuse / 既存APIで充足 | DataTable/TreeDataGrid, native table APIs |
| Virtual list/grid | Reuse / 既存APIで充足 | VirtualList/AssetBrowser providers |
| Tree/list reorder, generic drag feedback | Defer / 将来保留 | Native drag-drop available; move constraints belong to host / 移動契約はホスト次第 |
| Multi-selection action bar | Add / 追加 | Selection count and command view / 件数と候補のみ |
| Inspector/property groups | Reuse / 既存APIで充足 | PropertyGrid, SettingRow, SectionHeader |
| Range, marquee/lasso feedback | Reuse / 既存APIで充足 | Selection, CanvasSelection |
| Zoom toolbar | Add / 追加 | Shared image state / 画像状態を共用 |
| Minimap/overview | Defer / 将来保留 | Separate overview-content contract not introduced / 別content契約を増やさない |
| Undo/redo state, shortcut hints | Reuse / 既存APIで充足 | Command disabled state and shortcut; host owns history |
| Help callout, validation summary | Add / 追加 | Borrowed explanations/issues, no validation engine |
| Confirmation/destructive guard | Reuse / 既存APIで充足 | AlertDialog, Destructive ActionButton; host applies accepted request |
| Recent/favorite/pin | Reuse / 既存APIで充足 | Existing icons, commands, AssetBrowser; host owns lists |
| Two/three-pane workspace | Reuse / 既存APIで充足 | AdaptiveSplitLayout, Splitter, native tables |
| Domain processing, device control, persistence | Exclude / 対象外 | Outside UI-library responsibility / UIライブラリの責務外 |
