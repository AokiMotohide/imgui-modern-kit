# Design system refresh

[日本語](design-system.ja.md)

This change adds cross-cutting foundations to Editor Suite 2.0. It is **not full acceptance of the comprehensive next-release specification**. The implementation and remaining work below are separate from previous Editor Suite verification.

## Design

| Reference | Decision |
|---|---|
| [Fluent tokens](https://fluent2.microsoft.design/design-tokens) | Explicit semantic tokens; deterministic style projection |
| [Qt Model/View](https://doc.qt.io/qt-6/model-view-programming.html) | Host provider owns data and visible index |
| [Radix accessibility](https://www.radix-ui.com/primitives/docs/overview/accessibility), [APG Dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/) | Native input widgets plus explicit composite focus state |
| [Qt internationalization](https://doc.qt.io/qt-6/internationalization.html) | Explicit host locale callbacks, no global locale |
| [UI Automation fragments](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/nn-uiautomationcore-irawelementproviderfragment) | Copied native snapshots, borrowed HWND and action callback |

## API and migration

`MakeTheme(scheme, contrast, density)` provides 13 presets. `SetDensity` changes 24/28/44px control geometry without changing typography. Edit `semantic`, `typography`, `radius`, `stroke`, `elevation`, `opacity`, then call `ResolveTheme`. The legacy `colors` and `metrics` remain rendering projections for existing editor code; do not edit both independently. `MakePrecisionTheme` remains available. Host font pointers are never loaded or freed by ImKit.

`motion.reducedMotion` disables component animation. `ValidateContrast` checks text at 4.5:1 and focus/borders at 3:1 against the declared surfaces and control states, including alpha compositing. It is not a proof for arbitrary custom colors, background images, or every composed widget state. `AnimationState` supports Linear, EaseOut and EaseInOut interpolation.

Pass a host-owned `AccessibilityFrame` through `ComponentOptions::accessibility`. Call `Begin(generation)` before drawing and `Publish(sink)` after drawing. Node strings and child arrays are borrowed for that publication only. Storage overflow is explicit. `Validate()` checks IDs, parents, label references and cycles; it is an optional diagnostic, not a per-frame large-tree requirement. `ActionQueue` has fixed host-provided storage and must be serialized by the host. Clear stale requests at a host-defined frame boundary.

Enable `IMKIT_BUILD_ACCESSIBILITY_WIN32` and link `imkit::accessibility_win32`. `CreateWin32Provider` copies the frame into a COM snapshot. The host initializes COM, handles `WM_GETOBJECT`, obtains `IRawElementProviderSimple` through `QueryInterface`, and passes it to `UiaReturnRawElementProvider`. Release caller-owned COM references. Native clients can retain snapshots; the action callback and user data must outlive them. Snapshot refresh, UIA events and application shutdown invalidation remain host integration work. Invoke, Toggle, Focus, fragment navigation and common properties are implemented; other UIA patterns are not.

`LocaleContext` accepts a language tag, layout direction, translation, and number and date/time callbacks. Breadcrumbs and data column ordering support pseudo RTL. This is not bidirectional text shaping or complete RTL layout. Coordinate-oriented editor canvases retain their direction.

## Components

`patterns.h` exposes CommandPalette, SearchField, Breadcrumbs, SplitButton, Toolbar/ResponsiveToolbar, Dialog/AlertDialog, Popover, Menu, FormField, Progress, Spinner, Skeleton, empty/loading/error states, ToastRegion, Pagination, AdaptiveSplitLayout, VirtualList, DataTable and TreeDataGrid. All edited data and queues remain host-owned. Draw functions return requests or synchronously invoke the provider's `apply`; they do not implement persistence or undo.

Providers supply a flattened visible index after sort/filter/tree expansion. `query` receives a clipped range; `id` and `cell` are called only in that range. Stable row IDs must be nonzero. Callbacks must keep returned text alive for the frame. Host operations can rebuild indexes on events; drawing itself must not scan all rows. Table selection events carry index ranges, so hosts can use interval selection rather than allocate per selected row. Column sizing and visibility use Dear ImGui tables.

## Remaining implementation

- Complete semantic emission for all existing wrappers, composite parent/label/action relationships, and all UIA patterns/events; full Gallery UIA integration.
- Exhaustive roving-focus verification including disabled items, Tabs close/focus restoration, complete Tree/DataGrid navigation and typeahead.
- Complete FormField label/validation semantics, indeterminate progress animation, responsive layout policies and menu/form RTL; host number/date formatting usage.
- Full component state matrices and distinct production pattern pages; complete long-label and narrow-width acceptance.
- Existing raster icon migration for optical issues, full per-size optical acceptance, installed/relocated SDK verification after this API change.

Native OS/IME, real screen readers and integration into another application are not verified. Public ImGui IO and native backbuffer captures do not establish those results.

## Recorded verification

| Check | Result |
|---|---|
| Debug API | Existing public API compile/link and new design API fixture passed |
| Theme | 12 combinations, 24/28/44px density, contrast and easing/reduced-motion tests passed |
| Semantic tree | Stable IDs, duplicate detection, parents, Grid/Row/Cell relationships and action queue passed |
| Win32 UIA | Optional target compiled; representative child/name/Invoke callback retrieval passed |
| Public keyboard IO | Space opens Dialog; Escape closes it and restores launcher focus |
| Data views | DataTable and TreeDataGrid at 100,000 rows: clipped query, steady ImGui and C++ allocations 0 |
| Icons | 280 IDs, seven raster levels, alpha/tint/atlas correspondence and C++ icon tests passed |
| External source consumer | Separate host ImGui target, MakeTheme and semantic publication compiled/ran |
| Release Gallery | Build passed; 100 native backbuffers in two resolutions |

Capture directories are `out/design-system-final-1280` and `out/design-system-final-1920`. They contain 96 combinations of four pages and 12 themes across two resolutions, plus Japanese 150% and pseudo RTL 200% at each resolution. Representative frames were visually inspected. These are captures, not proof of every keyboard path, state matrix, or every icon's optical quality. Generated captures are not committed.
