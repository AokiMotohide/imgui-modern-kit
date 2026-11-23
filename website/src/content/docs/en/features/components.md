---
title: "Components"
description: "ImKit components are small compositions over public Dear ImGui behavior. Edited values, lifetime and persistence remain in the host."
---

ImKit components are small compositions over public Dear ImGui behavior. Edited values, lifetime and persistence remain in the host. The recipe below shows a complete frame-level settings panel; the smaller examples that follow highlight individual controls.

![Plain Dear ImGui and ImKit editing the same values side by side](/imgui-modern-kit/docs-images/v4-compare-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-compare.gif)

## Complete settings panel

Keep the values in application state and draw the panel during the host's existing Dear ImGui frame. The function returns a save request; it does not write a file or own a notification queue.

```cpp
#include <imkit/imkit.h>
#include <array>

struct DisplaySettings {
    bool enabled = true;
    int quality = 1;
    char qualitySearch[64]{};
    float exposure = 0.0f;
    char name[128] = "Main display";
    bool showSavedNotice = false;
};

bool DrawDisplaySettings(DisplaySettings& settings, const imkit::Theme& theme, double now) {
    imkit::ThemeScope themeScope(theme); // Same live context; destroyed before that context.
    bool saveRequested = false;
    if (imkit::Begin("Display settings")) {
        imkit::Toggle("Enabled", &settings.enabled, {&theme});

        if (imkit::BeginSettingRow("quality", "Quality")) {
            constexpr std::array<const char*, 3> labels{"Draft", "Balanced", "High"};
            imkit::SearchableCombo("##quality", &settings.quality, labels,
                                   settings.qualitySearch, sizeof(settings.qualitySearch));
            imkit::EndSettingRow();
        }

        if (imkit::BeginSettingRow("exposure", "Exposure")) {
            imkit::DragFloatWithUnit("##exposure", &settings.exposure, "EV", 0.01f, -8.0f, 8.0f);
            imkit::EndSettingRow();
        }

        imkit::InputTextWithHint("Name", "Required", settings.name, sizeof(settings.name));
        const bool invalidName = settings.name[0] == '\0';
        imkit::ValidationMessage("Enter a name before saving.", invalidName, imkit::ComponentOptions{&theme});
        imkit::BeginDisabled(invalidName);
        saveRequested = imkit::ActionButton("Save", imkit::ActionVariant::Primary, {}, {&theme});
        imkit::EndDisabled();

        if (settings.showSavedNotice &&
            imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0}, now, imkit::ComponentOptions{&theme})) {
            settings.showSavedNotice = false; // The host handles the dismiss request.
        }
    }
    imkit::End(); // Pair with Begin even when it returned false.
    return saveRequested; // The host persists settings and decides when to show a notice.
}
```

Call this from the host's frame after `NewFrame()` and before `Render()`. When it returns `true`, validate and persist `DisplaySettings` in the host, then set `showSavedNotice`. Keep `Theme`, font pointers, the Dear ImGui context and the panel state alive in the host.

## Primary action

```cpp
if (imkit::ActionButton("Apply", imkit::ActionVariant::Primary, {}, {&theme, &animation}))
    SaveSettings();
```

Set `ComponentOptions::callToAction` on the one action the user should press next: the button gets a trailing chevron, and a ring spreads once when it starts being drawn with the mark (not under reduced motion). Keep it to one button per region.

While the work a button started is running, set `ComponentOptions::progress` (0..1) and put the stage in the label; the button fills from its leading edge.

Use primary once per decision area. Secondary, ghost and destructive variants express intent without changing button activation semantics.

### Button roles, corners and widths

| Role | Variant | Use for |
|---|---|---|
| Main | `Primary` | The one action of a region. At most one per region |
| Ordinary | `Secondary` | Other actions |
| Choice | `Segmented` (not a button) | Picking one of several values; only the chosen segment is filled with the accent |
| Tool | `Ghost` | Small tools inside the content (undo, fit to view); frameless until hovered |
| Danger | `SubtleDestructive` (`Destructive` in a confirmation) | Deleting or stopping |

- Every button frame is painted by ImKit with smooth corners: the curve eases into the straight edge instead of meeting it with a circular arc. The radius is a quarter of the control height (`Radius::controlShare = .25`), so it keeps its proportion at every density. Only switch tracks and status badges stay round, because they are states, not things to press.
- A width that follows the label is never narrower than `ActionButtonMinimumWidth()` (four text heights).
- Give every button in a row or column of related actions the width `ActionRowWidth(labels)`, so they line up.
- An `IconActionButton` without a label is a square.

## Boolean and mixed state

```cpp
imkit::Toggle("Enabled", &enabled, {&theme, &animation});
imkit::IndeterminateCheckbox("Inherited", &state);
```

`Toggle` returns whether the value changed. `IndeterminateCheckbox` moves `Mixed` to `Checked` on activation; store its enum beside the rest of the host model.

## Search and validation

```cpp
imkit::SearchableCombo("Quality", &quality, labels, search, sizeof(search), disabled);
imkit::InputTextWithHint("Name", "Required", name, sizeof(name));
imkit::ValidationMessage("A name is required", name[0] == 0, imkit::ComponentOptions{&theme});
```

Search buffers and selected indexes are host-owned. Validation draws feedback but does not reject or store values.

## Settings and units

```cpp
if (imkit::BeginSettingRow("exposure", "Exposure")) {
    imkit::DragFloatWithUnit("##value", &exposure, "EV", .01f, -8.f, 8.f);
    imkit::EndSettingRow();
}
```

Call `EndSettingRow` only when `BeginSettingRow` returns true. Native scalar parsing and precision are preserved.

For a three-axis drag value, use `DragVector3WithUnit("Offset", values, "cm", .1f, -10.f, 10.f)`. Each axis remains a native `DragFloat`; the label scopes its stable IDs and narrow widths stack the fields.

For a labeled inspector field, use `AxisVector3Field("position", "Position", values, "m", .01f)`. It keeps X/Y/Z colors and native drag or direct numeric input, stacking the axes when space is limited. Hosts that need edit lifecycle events can receive activation, commit, and cancellation through the final `AxisVector3Interaction*` argument. `CompactIconActionButton` pairs an atlas icon with a visible label and a subtle role accent for compact tool groups; the host owns the atlas and action state.

`SliderFloatWithInput("Gain", &gain, 0.f, 1.f, "%.2f", 0.01f, &interaction)` puts a slider and a number field on one row; both edit the same value, clamped to the range and snapped to `step` when it is above zero. Below about 160 px the number field wraps under the slider. `SliderInputInteraction` reports `activated`, `changed` (also while dragging), `committed` (slider released or number entry confirmed after a change) and `canceled` (Esc), so a host can send progress and the final value separately.

`TransferCurveEditor("falloff", series, state, options, &interaction)` (`imkit/transfer_curve.h`) edits cubic Bezier transfer curves on a 0..1 frame. Each `TransferCurveSeries` holds a host-owned `std::span<TransferCurveNode>` (absolute `pos`, `in`, `out`), a color, a dashed flag and an optional display mapping (`toDisplay` / `fromDisplay` / `user`, for example y^(1/gamma)); dragging converts back with `fromDisplay`. `TransferCurveState` holds the selected series and knot (-1 for none); only the selected knot shows its handles. Drag a knot (its handles follow) or a handle; x stays between the neighbouring knots and the first and last knot keep their x. Click near another curve to select that series. With keyboard focus the arrow keys move the knot by 0.01 (Shift: 0.04) and Esc clears the knot. `TransferCurveInteraction` reports `activated`, `changed`, `committed`, `canceled` (Esc during a drag ends it; values stay as dragged, so restore the host snapshot), `seriesChanged` and `nodeChanged`. `TransferCurveOptions::monotone` keeps every curve non-decreasing after each edit; `TransferCurveClampMonotone(nodes)` does the same on demand.

## Status and notification

```cpp
imkit::StatusBadge("Ready", imkit::StatusKind::Success, imkit::ComponentOptions{&theme});
if (imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0}, now, imkit::ComponentOptions{&theme}))
    dismissSavedNotice = true;
```

Notification expiry and removal belong to the host. More overload-level details are in [API coverage](../../api/native/).

## Compact status and actions

```cpp
imkit::CompactActionRowOptions row;
row.statusText = streaming ? "Streaming" : "Stopped";
row.statusKind = streaming ? imkit::StatusKind::Success : imkit::StatusKind::Neutral;
row.primaryLabel = streaming ? "Stop" : "Connect";
row.primaryVariant = streaming ? imkit::ActionVariant::SubtleDestructive
                               : imkit::ActionVariant::Primary;
row.settingsLabel = "Settings";

switch (imkit::CompactActionRow("camera-1", row)) {
case imkit::CompactActionRowRequest::Primary: requestConnectionChange = true; break;
case imkit::CompactActionRowRequest::Settings: openSettings = true; break;
case imkit::CompactActionRowRequest::None: break;
}
```

`CompactActionRow` draws host-provided status and actions, then returns a request. Connection state, command execution, and settings window lifetime stay with the host. When the available width is too narrow, it wraps the status and actions onto multiple lines.
Each action can be disabled independently; its disabled reason appears in that button's tooltip. Use `SubtleDestructive` for reversible risky actions such as stopping a stream.

`NotificationCard` returns a dismiss request. A zero `expiresAt` means it remains visible until the host removes it; a nonzero expiration is compared with the `now` value supplied by the host.

## Try it in the Gallery

Open **Workspace** in the Gallery to see WorkspaceTabs, hierarchy rows, inspector cards, `SettingToggleRow`, `DragVector3WithUnit`, `ChoiceGroup` and `CompactActionRow` used together. The returned requests update Gallery-owned values, and the same pattern works in an existing ImGui frame. The parts are described in [Workflow components](../workflow/), and the declarations and lifetime rules are in the [3.2 additions](../../api/v3-2/).
