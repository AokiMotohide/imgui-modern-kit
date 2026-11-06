# Application shell components

[日本語](シェルコンポーネント.md)

ImKit provides host-owned building blocks for persistent application chrome:
`AppBar`, `WorkspaceHeader`, `InspectorSection`, `AdvancedSection`,
`BottomActionBar`, `DiagnosticsDrawer`, and `ThemePicker`.

## Overview

These components borrow labels and command spans, render the current host state,
and return command or selection requests. They do not own documents, docking,
settings, undo history, workers, renderers, or an ImGui context. All state management
and lifecycle decisions remain on the host side.

## Shell component inventory

| Component | Primary purpose | Return value / Behavior |
|---|---|---|
| `AppBar` | Top-level header bar (app name, project name, badge, status, commands) | Clicked command request |
| `WorkspaceHeader` | Workspace switching tabs or view-control tabs | Selected tab request |
| `InspectorSection` | Collapsible group frame inside a property inspector | Keeps fold state / renders content |
| `AdvancedSection` | Collapsible toggle section for advanced settings | Fold/unfold toggle request |
| `BottomActionBar` | Bottom-aligned status row and action buttons (Commit/Cancel/Progress) | Triggered action |
| `DiagnosticsDrawer` | Bottom drawer displaying logs, warnings, and diagnostic messages | Open/close request, renders logs |
| `ThemePicker` | UI widget for selecting available theme presets | Selected theme preset |

## Code example

```cpp
#include <imkit/imkit.h>
#include <vector>

// Draw the AppBar and receive the pressed action
void RenderAppHeader(MyAppState& state, imkit::ToolbarState& toolbar) {
    const std::vector<imkit::Command> commands = {
        {1, "Save", "Ctrl+S"},
        {2, "Export", "", !state.hasSelection},   // StableId, label, shortcut, disabled
    };

    imkit::AppBarView view;
    view.product = "MyStudio";          // Application name
    view.document = "scene_01.proj";    // Project name
    view.context = "READY";             // Mode or badge
    view.status = "All changes saved";               // Status text
    view.dirty = false;                 // Unsaved changes
    view.statusKind = imkit::FeedbackKind::Success;
    view.actions = commands;

    if (const auto action = imkit::AppBar("main_app_bar", view, toolbar)) {   // StableId of the pressed action, or 0
        state.Run(action);              // your code runs the action
    }
}
```

## Themes and fonts

- `ThemePicker` returns a `ThemePreset` selection only. Persist the stable ID from
  `ThemePresets()` in host settings and call `MakeTheme`/`ApplyTheme` outside a
  frame to apply it.
- `imkit_copy_font_assets(target, destination)` stages the optional Inter
  and Noto Sans JP files; the host still loads and owns its font atlas.
