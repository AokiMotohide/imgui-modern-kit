# User guide

[日本語](利用ガイド.md) · [Documentation index](../README.md) · [Getting started](getting-started.md)

How ImKit fits into a Dear ImGui app: who owns what, how to apply a theme, which controls you get, and how to link it. Read the first three sections once; use the rest as reference.

## Who owns what

`imkit::imkit` is a C++20 static target. Your application owns the Dear ImGui implementation, every context, `NewFrame` and `Render`, the backends, platform windows, fonts and the edited data. ImKit acts on the current live context, on its GUI thread, and leaves the Begin/End contracts and native input semantics unchanged.

| Header | Role |
| --- | --- |
| `theme.h` | Copyable semantic values: presets, palette, metrics, fonts, motion |
| `native.h` | Exact overload sets imported from the public Dear ImGui header |
| `widgets.h` | Compatible wrappers with selection and tab decoration |
| `components.h` | Small compositions such as buttons, toggles, badges and segments |

There is no hidden registry, current-theme singleton, worker or plugin manager. To extend the design, add semantic tokens or an explicit options value. Keep edits in the host and use native widgets for input.

## Apply a theme

Apply a theme once per frame, before `NewFrame()`:

```cpp
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
theme.fonts = {regularFont, emphasisFont}; // non-owning, loaded by the host

imkit::ApplyTheme(theme, 1.25f);           // before ImGui::NewFrame()
ImGui::NewFrame();
```

`ApplyTheme` rebuilds the style from unscaled tokens, so calling it every frame never compounds the sizes. The default scale is 125%, and the supported range is 50% to 250%. It does not change `io.FontDefault`.

To give one window or dialog its own theme, use `ThemeScope`:

```cpp
{
    imkit::ThemeScope scope(lightTheme, 1.0f); // restores style and font when it ends
    if (imkit::Begin("Preview")) {
        imkit::TextUnformatted("Drawn with the light theme");
    }
    imkit::End();
}
```

Create the scope before `Begin`, or before the widgets that need the theme. Scopes nest. Destroy each one on the same live context, before the context is destroyed.

### Sizes, accent and fonts

| Topic | Rule |
| --- | --- |
| Base values (logical pixels) | Control height 30, gap 6, radius 8 for controls and 12 for cards and overlays, body 14, heading 18 |
| Layout | Native controls add the font height and padding, so a larger font makes taller controls. Inline buttons, menu rows, tables and multiline text keep their native layout instead of being forced to 30 px. |
| Scale | Pass your application scale as the `scale` argument and do not multiply geometry again outside ImKit. Monitor DPI stays the host's job. |
| Accent | `SetAccent` updates the accent, the contrasting on-accent colour, focus and the derived selection. Other colours are independent. Arbitrary edits are not a blanket contrast guarantee. |
| Fonts | The Gallery uses Inter 4.1, then Noto Sans JP 2.004 Regular merged after each Inter source with Latin U+0020–U+024F excluded from the fallback. Japanese text uses Regular with no synthesized bold. Fonts are optional host assets, not embedded in the library. |
| Text input | Strings are UTF-8. Configure fonts and the platform IME callbacks in your host; ImKit does no OS font discovery. |

## Components

| API | Contract |
| --- | --- |
| `ActionButton` | Primary, secondary, ghost, destructive; pass `ComponentOptions{&theme, &animation}` for explicit palette and motion |
| `IconActionButton` | One icon plus a short label, a description or disabled reason, and an action variant |
| `IconButton` | Native arrow-button semantics with a tooltip label |
| `Toggle` / `Switch` | Native checkbox hit testing, editing and keyboard operation; drawn switch track |
| `IndeterminateCheckbox` | Mixed activates to checked; checked and unchecked toggle as usual |
| `Segmented` | A host-selected stable index over a span of labels |
| `SearchableCombo` | Host-owned UTF-8 search buffer and selection, optional disabled indexes, ASCII case folding, exact non-ASCII matching |
| `InputScalarWithUnit` | Native scalar parsing and precision; the unit is a separate label |
| `DragFloatWithUnit`, `InputVector3WithUnit`, `DragVector3WithUnit` | Native editing with explicit units and labeled X/Y/Z fields that stack at narrow widths |
| `BeginSettingRow` / `EndSettingRow` | Two-column table; call End only if Begin returned true |
| `StatusBadge` | Text and marker shape supplement the status colour |
| `NotificationCard` | The host owns expiry and removal; returns a dismiss request; zero expiry persists |
| `BeginToolbar` / `EndToolbar` | Child layout; End is always required |
| `ValidationMessage` | Draw-only invalid outline plus tooltip; call it right after an input |
| `OverlayDecoration` | Optional inset elevation for the current popup, clipped inside it |

Native ranges use `DragFloatRange2` and `DragIntRange2`. All scalar and vector overloads are in the [API table](../reference/api-coverage.md). Standard wrappers use the current `ImGuiStyle` and need no theme argument. Decoration appended to the DrawList submits no extra item, so `IsItem*` keeps referring to the widget, and grouped controls such as unit inputs and segments expose native group status.

## Motion lifetime

- `AnimationState` is host-owned, with 256 fixed slots. Use one instance per live context, and reset it with your own generation token when a context is destroyed or reused. It keeps no context pointer.
- IDs follow the native ID stack. `Prune(frame)` drops entries unseen for 120 frames. When the slots run out, the oldest entry is replaced. A new entry starts at its target, so nothing animates unasked. `Reset` drops all transitions.
- Native wrappers respond immediately. Action outlines, switch movement and overlay decoration use the theme's motion (default 140 ms for controls, 200 ms for overlays). With no state, motion disabled or zero duration, the correct state shows at once. Motion never delays the edited value.

## Link ImKit

From source, point ImKit at the Dear ImGui target your application already has:

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

Use Dear ImGui docking commit `367b2c24f399988ddafc0bb4628da0106bcc09be` exactly, in every participating target. Earlier font, style and API contracts are not supported. Upgrade the host on purpose.

From an installed SDK, create the matching `host_imgui` first:

```cmake
set(IMKIT_IMGUI_TARGET host_imgui)
set(IMKIT_SDK_ABI_CONFIRMED ON) # only after checking the manifest and settings
find_package(imkit 4.0 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE imkit::imkit)
```

Configure with `-DCMAKE_PREFIX_PATH=/path/to/sdk`. The package uses an imported adapter to your target and carries no original build paths.

- Packages exist for Windows x64, Windows Arm64, macOS arm64, macOS x86_64 and macOS Universal 2. Each manifest records the OS, architecture, compiler and Dear ImGui commit.
- Match compile definitions and `imconfig.h` across all translation units. The library archive does **not** contain Dear ImGui core.
- Header guards reject other Dear ImGui version numbers. The explicit ABI confirmation also asks you to check the manifest and type settings; it is not automatic binary introspection.
- Use source integration when binary settings do not match. `examples/consumer` supports either form.

## Where to go next

| You want | Read |
| --- | --- |
| The Gallery and what each page shows | [Gallery guide](gallery.md) |
| Semantic tokens, locale and provider contracts | [Design system](../architecture/design-system.md) |
| Wizards, inspectors, preview tiles, toasts | [Workflow components](../components/workflow-components.md) |
| Timelines, curves, 3D views | [Editor Suite](../components/editor-suite.md) |
| What 4.0 added | [4.0 additions](../reference/v4.0-api.md) |
| Exact evidence and limits | [Validation](../reference/validation.md), [third-party notices](../../THIRD_PARTY_NOTICES.md) |
