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

The custom editor API is inventoried separately in [editor-api.md](editor-api.md). Its compile/link fixture is `tests/editor_api_compile.cpp`, also compiled against an external host ImGui target. The generated 365-overload native inventory is unchanged.
