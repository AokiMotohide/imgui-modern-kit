# Design system

[日本語](デザインシステム.md) · [Themes](themes.md) · [Documentation catalog](../reference/documentation-catalog.md)

The design system gives creative tools one set of semantic tokens, an accessibility publication path, and host-driven data views on top of Dear ImGui. The host owns data, state and lifetime. ImKit draws and returns requests.

Status: the foundations below are implemented and verified as recorded. The full next-release specification is not accepted; [Remaining work](#remaining-work) lists the gaps, and none of this replaces the earlier Editor Suite verification.

## Design decisions

| Reference | Decision |
|---|---|
| [Fluent tokens](https://fluent2.microsoft.design/design-tokens) | Explicit semantic tokens; deterministic style projection |
| [Qt Model/View](https://doc.qt.io/qt-6/model-view-programming.html) | The host provider owns data and the visible index |
| [Radix accessibility](https://www.radix-ui.com/primitives/docs/overview/accessibility), [APG Dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/) | Native input widgets plus explicit composite focus state |
| [Qt internationalization](https://doc.qt.io/qt-6/internationalization.html) | Explicit host locale callbacks, no global locale |
| [UI Automation fragments](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/nn-uiautomationcore-irawelementproviderfragment) | Copied native snapshots, borrowed HWND and action callback |

## Theme and tokens

```cpp
#include <imkit/theme.h>

// Build a theme from a scheme, a contrast mode and a density.
imkit::Theme theme = imkit::MakeTheme(imkit::ColorScheme::Dark,
                                      imkit::ContrastMode::Standard,
                                      imkit::Density::Comfortable);

theme.semantic.accent = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); // change a token
imkit::ResolveTheme(theme);                             // rebuild the rendering projections
```

- `MakeTheme(ThemePreset)` builds one of the 13 presets. `MakeTheme(scheme, contrast, density)` builds a base theme. `MakePrecisionTheme` remains available.
- `SetDensity` switches `Compact`, `Comfortable` and `Touch` (24, 30 and 44 px controls) without changing typography.
- Edit `semantic`, `typography`, `radius`, `stroke`, `elevation` and `opacity`, then call `ResolveTheme`. The legacy `colors` and `metrics` are rendering projections for existing editor code; do not edit both sets independently.
- Host font pointers are never loaded or freed by ImKit.

| Semantic token | Role |
|---|---|
| `canvas` | Backmost application background |
| `surface`, `surfaceRaised`, `overlay` | Panels, raised cards, and popups or dialogs |
| `text`, `textSecondary`, `textDisabled` | Body, supporting and disabled text |
| `border` | Borders and dividers |
| `accent`, `onAccent` | Primary action and selection, and the text drawn on it |
| `success`, `warning`, `error` | Status colors |
| `control` | Per-state control colors: rest, hover, pressed, focused, selected, disabled |
| `shadow` | Shadow color |

**Contrast and motion**

- In standard contrast, the projected Dear ImGui border color is a softened mix of `semantic.border` and the surface, so frame outlines can fall below 3:1. High contrast keeps the full-strength border.
- `ValidateContrast` checks text at 4.5:1 and focus rings and borders at 3:1 against the declared surfaces and control states, including alpha compositing, using the full `semantic.border`. It does not prove arbitrary custom colors, background images or every composed widget state.
- `motion.reducedMotion` disables component animation. `AnimationState` interpolates with Linear, EaseOut or EaseInOut.

## Accessibility

Pass a host-owned `AccessibilityFrame` through `ComponentOptions::accessibility`.

1. Call `Begin(generation)` before drawing.
2. Draw the components.
3. Call `Publish(sink)` after drawing.

- Node strings and child arrays are borrowed for that publication only. Storage overflow is reported explicitly.
- `Validate()` checks IDs, parents, label references and cycles. It is an optional diagnostic, not a per-frame requirement for large trees.
- `ActionQueue` uses fixed host-provided storage, and the host must serialize access. Clear stale requests at a host-defined frame boundary.

**Windows UI Automation.** Enable `IMKIT_BUILD_ACCESSIBILITY_WIN32` and link `imkit::accessibility_win32`.

- `CreateWin32Provider` copies the frame into a COM snapshot.
- The host initializes COM, handles `WM_GETOBJECT`, obtains `IRawElementProviderSimple` through `QueryInterface`, passes it to `UiaReturnRawElementProvider`, and releases the caller-owned COM references.
- Native clients can retain snapshots, so the action callback and its user data must outlive them.
- Snapshot refresh, UIA events and invalidation at application shutdown remain host integration work.
- Implemented: Invoke, Toggle, Focus, fragment navigation and common properties. Other UIA patterns are not.

## Localization

`LocaleContext` accepts a language tag, a layout direction, a translation callback and number and date/time callbacks. There is no global locale.

- Breadcrumbs and data column ordering support pseudo RTL. This is not bidirectional text shaping or a complete RTL layout.
- Coordinate-oriented editor canvases keep their direction.

## Components and data views

`patterns.h` exposes CommandPalette, SearchField, Breadcrumbs, SplitButton, Toolbar and ResponsiveToolbar, Dialog and AlertDialog, Popover, Menu, FormField, Progress, Spinner, Skeleton, empty, loading and error states, ToastRegion, Pagination, AdaptiveSplitLayout, VirtualList, DataTable and TreeDataGrid.

- All edited data and queues stay host-owned. Draw functions return requests or synchronously invoke the provider's `apply`. They implement neither persistence nor undo.
- Providers supply a flattened visible index after sort, filter and tree expansion.
- `query` receives a clipped range. `id` and `cell` are called only inside that range, and stable row IDs must be nonzero.
- Callbacks keep returned text alive for the frame.
- Hosts rebuild indexes on events. Drawing itself must not scan all rows.
- Table selection events carry index ranges, so hosts can use interval selection instead of allocating per selected row.
- Column sizing and visibility use Dear ImGui tables.

## Remaining work

- Complete semantic emission for all existing wrappers, composite parent, label and action relationships, and all UIA patterns and events; full Gallery UIA integration.
- Exhaustive roving-focus verification, including disabled items, Tabs close and focus restoration, complete Tree and DataGrid navigation and typeahead.
- Complete FormField label and validation semantics, indeterminate progress animation, responsive layout policies, menu and form RTL, and host number and date formatting usage.
- Full component state matrices and distinct production pattern pages; complete long-label and narrow-width acceptance.
- Optical migration of the existing raster icons, per-size optical acceptance, and installed or relocated SDK verification after this API change.

Native OS/IME, real screen readers and integration into another application are not verified. Public ImGui IO and native backbuffer captures do not establish those results.

## Recorded verification

Recorded at the Editor Suite 2.0 foundation revision. Counts, such as 280 icon IDs and the 28 px Comfortable height, are those of that revision.

| Check | Result |
|---|---|
| Debug API | The existing public API and the new design API fixture compiled, linked and passed |
| Theme | 12 combinations, 24/28/44 px density, contrast, and easing and reduced-motion tests passed |
| Semantic tree | Stable IDs, duplicate detection, parents, Grid/Row/Cell relationships and the action queue passed |
| Win32 UIA | The optional target compiled; representative child, name and Invoke callback retrieval passed |
| Public keyboard IO | Space opens a Dialog; Escape closes it and restores launcher focus |
| Data views | DataTable and TreeDataGrid at 100,000 rows: clipped query, steady-state ImGui and C++ allocations 0 |
| Icons | 280 IDs, seven raster levels, alpha, tint and atlas correspondence, and C++ icon tests passed |
| External source consumer | A separate host ImGui target compiled and ran MakeTheme and semantic publication |
| Release Gallery | Build passed; 100 native backbuffers in two resolutions |

Capture directories are `out/design-system-final-1280` and `out/design-system-final-1920`. They hold 96 combinations of four pages and 12 themes across two resolutions, plus Japanese 150% and pseudo RTL 200% at each resolution. Representative frames were inspected visually. The captures are not proof of every keyboard path, state matrix or icon's optical quality, and they are not committed.
