# Build a settings screen

[日本語](build-settings-screen.ja.md) · [Documentation index](README.md) · [How ImKit works](how-it-works.md)

A settings screen in ImKit is a host-owned struct plus a row of labeled controls. ImKit gives you themed controls that read and write your state and report the result; your code decides what to do with the changes. This page walks the build from an empty window to a complete, save-able settings panel. The per-control reference is [Components](components.md).

![Gallery overview with themed panels](images/gallery-overview.gif)

## The core pattern

Three rules make a settings screen work:

1. **State lives in your struct.** A plain C++ struct holds every setting. The controls read/write it; ImKit never persists it.
2. **One control per labeled row.** `BeginSettingRow` draws the label in the first column and your control in the second. Each row is one setting, one control.
3. **Return a request, don't write.** The panel returns a "save" request when the user clicks Save. Your code validates, persists, and shows a notification.

## What you need

1. A Dear ImGui context and a window (your app or the Gallery).
2. A `Theme` to scope the panel.
3. CMake linking the `imkit::imkit` target.

## The host state

Keep every setting in one struct. This is the single source of truth; the controls are views onto it.

```cpp
#include <imkit/imkit.h>
#include <array>

struct DisplaySettings {
    bool enabled = true;          // toggle
    int quality = 1;              // 0..2, chosen by the combo/segmented
    char qualitySearch[64]{};     // search buffer (host-owned)
    float exposure = 0.0f;        // slider, in EV
    char name[128] = "Main display"; // text input
    bool showSavedNotice = false; // drives the save notification
};
```

Nothing here is owned by ImKit. The `qualitySearch` buffer is a search field for the combo; you must keep it alive and reset it as needed.

## The row pattern

The idiomatic building block is a setting row: label on the left, control on the right, drawn at the same vertical rhythm as the rest of the panel.

```cpp
// A boolean: no row needed, the Toggle is self-contained.
imkit::Toggle("Enabled", &settings.enabled, {&theme});

// A labeled control: wrap it in a row.
if (imkit::BeginSettingRow("exposure", "Exposure")) {
    imkit::DragFloatWithUnit("##exposure", &settings.exposure, "EV", 0.01f, -8.0f, 8.0f);
    imkit::EndSettingRow();   // only call it when BeginSettingRow returned true
}
```

- `BeginSettingRow(id, label, labelWidth)` lays out the label and a second column. The `id` is a stable ImGui ID.
- Put `##` in the control's id so it keeps a stable ImGui id inside the row.
- `EndSettingRow` closes the two-column layout. Call it only when `BeginSettingRow` was true.
- For a boolean you can skip the row entirely — `Toggle`/`Switch`/`IndeterminateCheckbox` are self-contained.

## The complete settings screen

This is the full panel, assembled from the building blocks above. Draw it inside your existing Dear ImGui frame, after `NewFrame()`.

```cpp
bool DrawDisplaySettings(DisplaySettings& settings,
                         const imkit::Theme& theme, double now) {
    imkit::ThemeScope themeScope(theme);      // panel uses this theme, then restores
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
        saveRequested = imkit::ActionButton("Save", imkit::ActionVariant::Primary,
                                            {}, {&theme});
        imkit::EndDisabled();

        if (settings.showSavedNotice &&
            imkit::NotificationCard({"saved", "Settings saved", imkit::StatusKind::Success, 0},
                                    now, &theme)) {
            settings.showSavedNotice = false;   // dismiss request -> host hides it
        }
    }
    imkit::End();                                // pairs with Begin even when it returned false
    return saveRequested;                       // the host persists when this is true
}
```

The same recipe scales: swap `SearchableCombo` for `Segmented` (a small fixed choice), use `InputScalarWithUnit`/`DragVector3WithUnit` for numbers, and add a `StatusBadge` to show current state. See [Components](components.md) for each control.

## Save, validate, and notify

The panel does **not** persist. It reports. Your code does three things when `DrawDisplaySettings` returns `true`:

```cpp
double now = ImGui::GetIO().Time;
if (DrawDisplaySettings(settings, theme, now)) {
    if (settings.name[0] == '\0') {
        // Already rejected by the button; guard against out-of-band edits.
        return;
    }
    PersistDisplaySettings(settings);      // your file/DB/network save
    settings.showSavedNotice = true;       // re-draws the notification next frame
}
```

- **Validate before acting.** `ValidationMessage` draws feedback; `BeginDisabled`/`EndDisabled` gate the Save button on the same condition. Keep the condition in one place.
- **Notify after the fact.** `NotificationCard` returns a dismiss request; when it returns true (user dismissed it) you clear `showSavedNotice`. A zero `expiresAt` means the card stays until you remove it.
- **Persist last.** Only after `PersistDisplaySettings` returns, set `showSavedNotice` so a failed save does not flash a success toast.

## How to run it

- The Gallery's settings area builds exactly this pattern. Build the `imkit_gallery` target and open the settings page to see it themed.
- In your own app, call `DrawDisplaySettings` from your frame between `NewFrame()` and `Render()`, and handle the returned request as shown.

## What just happened

- Your `DisplaySettings` struct owned every value; the controls were views onto it.
- Each setting was a labeled row with one control, drawn in the panel's rhythm.
- The panel returned a save request instead of writing a file.
- Your code validated, persisted, and showed a dismiss-able notification.

## The rules that keep it clean

- **One value, one owner.** Each setting lives in exactly one field; the control writes it. No hidden state.
- **One control per row.** Do not cram a label and two controls into one row.
- **Gate the action on the validation.** Disable Save while invalid; show the message next to the offending row.
- **Persist in the host.** Never let a control open a file or a network socket. The panel returns; you act.
- **Keep `Begin`/`End` and row pairs balanced.** `End` even when `Begin` returned false; `EndSettingRow` only when `BeginSettingRow` returned true.

## Next

- [Components and recipes (per-control reference)](components.md)
- [Build a node editor](build-node-editor.md)
- [Author a custom component](custom-component.md)
