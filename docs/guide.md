# User guide

[日本語](guide.ja.md)

## Ownership and extension layers

`imkit::imkit` is a C++20 static target. The host owns one Dear ImGui implementation, all contexts, NewFrame/Render, backends, platform windows, fonts and edited data. ImKit only acts on the current live context, on its GUI thread. Begin/End contracts and native input semantics are unchanged.

The extension layers are `theme.h` (copyable semantic values), `native.h` (exact public overload sets), `widgets.h` (compatible wrappers with selection/tab decoration), and `components.h` (small compositions). There is no hidden registry, current-theme singleton, worker or plugin manager. To extend the design, add semantic tokens or an explicit component options value; keep edits in the host and use native widgets for input.

## Theme, fonts and scale

`MakePrecisionTheme(Light/Dark)` supplies the selected design. `ApplyTheme(theme, scale)` replaces style from unscaled tokens before NewFrame; applying twice does not multiply sizes. The default scale is 125%, and the supported host control range is 50% through 250%. It does not change `io.FontDefault`. `ThemeScope` is for a frame-local region, restores style and font on destruction, and supports nesting. Create it before Begin or before the widgets that need the theme. Destroy it on the same live context, before context destruction.

```cpp
auto theme = imkit::MakePrecisionTheme(imkit::ColorScheme::Light);
theme.fonts = {regularFont, emphasisFont}; // non-owning, loaded by the host
auto saved = theme;
// Within a host frame:
{
    imkit::ThemeScope scope(theme, 1.25f);
    if (imkit::Begin("Display")) {
        imkit::TextUnformatted("Settings / 設定");
    }
    imkit::End();
}
theme = saved; // host-owned preset, no implicit persistence
```

Base values: control height 28, gap 6, radius 4, body 14, heading 18 logical pixels. Native controls include current font height plus padding, so larger fonts increase control height. Inline buttons, menu rows, tables and multiline text follow their native layout rather than forcing every item to 28 px. Use the `scale` argument as the application scale; avoid also multiplying geometry outside ImKit. Monitor DPI remains the host's responsibility.

`SetAccent` updates accent, contrasting on-accent, focus and derived selection. Other colors are independent. Palette values include canvas, surface, input, raised, text, muted, border, destructive/on-destructive, success and warning. Arbitrary edits are not a blanket contrast guarantee.

The catalog uses Inter 4.1 first, then Noto Sans JP 2.004 Regular in merge mode after each Inter source. Latin U+0020–U+024F is excluded from the fallback. Japanese body and heading glyphs use Regular, with no synthesized bold. Fonts are optional host assets, not embedded library data. Strings are UTF-8; configure appropriate fonts and platform IME callbacks in your own host. No OS font discovery is performed.

## Components

| API | Contract |
|---|---|
| `ActionButton` | Primary/secondary/ghost/destructive; pass `ComponentOptions{&theme, &animation}` for explicit palette/motion |
| `IconActionButton` | A single icon plus short label, description/disabled reason and action variant |
| `IconButton` | Native arrow-button semantics with a tooltip label |
| `Toggle` / `Switch` | Native checkbox hit testing, editing and keyboard operation; drawn switch track |
| `IndeterminateCheckbox` | Mixed activates to checked; checked/unchecked toggle normally |
| `Segmented` | Host-selected stable index over a span of labels |
| `SearchableCombo` | Host-owned UTF-8 search buffer and selection; optional disabled-index span; ASCII case folding, exact non-ASCII matching |
| `InputScalarWithUnit` | Native scalar parsing and precision; unit is a separate label |
| `DragFloatWithUnit`, `InputVector3WithUnit` | Native editing; explicit unit and equal-width vector fields |
| `BeginSettingRow` / `EndSettingRow` | Two-column table; call End only if Begin returned true |
| `StatusBadge` | Text and marker shape supplement semantic status color |
| `NotificationCard` | Host owns expiry/removal; returns a dismiss request; zero expiry persists |
| `BeginToolbar` / `EndToolbar` | Child layout; End is always required |
| `ValidationMessage` | Draw-only invalid outline plus tooltip; call immediately after an input |
| `OverlayDecoration` | Optional current-window inset elevation, clipped inside the popup |

Native ranges use `DragFloatRange2` / `DragIntRange2`; all scalar and vector overloads are in the [API table](api-coverage.md). Standard wrappers use current ImGuiStyle and do not require a theme argument. Appending DrawList decoration does not submit another item; `IsItem*` continues to refer to the widget. Groups such as unit inputs and segments expose native group status.

## Motion lifetime

`AnimationState` is host-owned with 256 fixed slots. Use a separate instance per live context and reset with your own context generation token when a context is destroyed or reused. It retains no context pointer. IDs follow the native ID stack. `Prune(frame)` discards unseen entries after 120 frames; capacity exhaustion replaces the oldest entry. A new entry starts at the current target to avoid unsolicited entry animations. `Reset` discards all transitions.

Native wrappers remain immediately responsive. Optional action outlines, switch movement and overlay decoration use 60/100 ms default transitions. Omitted state, disabled motion or zero duration produces the correct immediate state. Motion never delays the underlying edited value.

## Source integration

```cmake
add_library(host_imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(host_imgui PUBLIC ${IMGUI_SOURCE_DIR})
set(IMKIT_IMGUI_TARGET host_imgui)
add_subdirectory(external/imgui-modern-kit)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

Use the pinned 1.92.9b docking source. Historical 1.91.9 and 1.89 WIP (18814) configurations were inspected for compatibility: they lack current font/style/API contracts and are not supported by this release. Upgrade the host's ImGui deliberately before adopting ImKit; do not replace a host's fork implicitly. Other OS/toolchains are not verified.

## Installed SDK

The binary SDK is Windows x64, MSVC v145, `/MD` Release and `/MDd` Debug, with Dear ImGui 1.92.9b docking default ABI types. Match compile definitions and `imconfig.h` across all translation units. The library archive does **not** contain ImGui core. Header guards reject other ImGui version numbers; the explicit ABI confirmation below also requires you to check compiler, CRT, architecture and type settings. It is not automatic binary introspection.

```cmake
# Create the matching host_imgui target first.
set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_SDK_ABI_CONFIRMED ON) # only after checking the manifest and settings
find_package(imkit 2.2 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

Configure with `-DCMAKE_PREFIX_PATH=/path/to/sdk`. The package uses an imported dependency adapter to your target, with no original build paths. `examples/consumer` supports either a source checkout or the installed package. Use source integration when binary settings do not match.

## Build and evidence

See [validation](validation.md) for exact evidence and limits. Native OS/IME input and physical devices are separate from the public-IO integration runner. Each category capture comes from the actual catalog and shipped API. License records are in [third-party notices](../THIRD_PARTY_NOTICES.md).

## Editor Suite 2.0 workspaces

Open **Editor Core**, **Video Editor**, or **CG Editor** in the native Gallery. Use `--verify-editors --capture-editors --output out/editors` for public-IO checks and real backbuffer images, including 150% scale. Timeline has lower-right Fit/minus/log-zoom/plus controls; middle mouse pans/orbits, Ctrl+wheel zooms around the pointer, clip edges trim, Razor splits, and selected gizmo axis tips drag. UV and Graph tabs expose editable points. See [Editor Suite 2.0 verification and boundaries](editor-refresh.md).

## Design system foundations

See [API migration, provider ownership and current implementation boundaries](design-system.md).

## Generic workflow and image components

See [workflow API, ownership, coordinates and Gallery](workflow-components.md).
Use Generic Workspace, Feedback / States and Preview Tiles. Keep notification
time/queues, textures, selection and resize dimensions in host-owned state.
