# Components and recipes

[日本語](components.ja.md)

ImKit components are small compositions over public Dear ImGui behavior. Edited values, lifetime and persistence remain in the host. The recipe below shows a complete frame-level settings panel; the smaller examples that follow highlight individual controls.

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
        imkit::ValidationMessage("Enter a name before saving.", invalidName, &theme);
        imkit::BeginDisabled(invalidName);
        saveRequested = imkit::ActionButton("Save", imkit::ActionVariant::Primary, {}, {&theme});
        imkit::EndDisabled();

        if (settings.showSavedNotice &&
            imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0}, now, &theme)) {
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

Use primary once per decision area. Secondary, ghost and destructive variants express intent without changing button activation semantics.

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
imkit::ValidationMessage("A name is required", name[0] == 0, &theme);
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

## Status and notification

```cpp
imkit::StatusBadge("Ready", imkit::StatusKind::Success, &theme);
if (imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0}, now, &theme))
    dismissSavedNotice = true;
```

Notification expiry and removal belong to the host. More overload-level details are in [API coverage](api-coverage.md).

`NotificationCard` returns a dismiss request. A zero `expiresAt` means it remains visible until the host removes it; a nonzero expiration is compared with the `now` value supplied by the host.
