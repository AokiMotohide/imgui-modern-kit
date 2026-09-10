# Components and recipes

[日本語](components.ja.md)

ImKit components are small compositions over public Dear ImGui behavior. Edited values, lifetime and persistence remain in the host.

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
