# Application shell components

[日本語](シェルコンポーネント.md)

ImKit provides host-owned building blocks for persistent application chrome:
`AppBar`, `WorkspaceHeader`, `InspectorSection`, `AdvancedSection`,
`BottomActionBar`, `DiagnosticsDrawer`, `ThemePicker`, and `ExternalControlIndicator`.

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
| `ExternalControlIndicator` | Glow frame around the viewport and a pill naming who is in control | Stop request, whether another frame is needed |

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

## External control indicator

`ExternalControlIndicator` shows that something other than the person at the screen is driving the application: an automation API, a remote operator or an agent. A glow frame around the viewport says so at any distance; a pill names who is in control, what it is doing, and offers Stop. The host owns the connection, the phase and what Stop does.

| Phase | Default look |
| --- | --- |
| `Off` | Nothing; a visible frame fades out in the last phase's look |
| `Connected` | A quiet, steady accent frame |
| `Active` | A breathing accent frame that ripples on `Pulse()` |
| `InputLocked` | A stronger, steady frame in the warning colour, a light wash and the locked text |

```cpp
static imkit::ExternalControlState remote;
static imkit::ExternalControlStyle look = imkit::MakeExternalControlStyle(theme);
imkit::ExternalControlView view;
view.phase = server.Locked()   ? imkit::ExternalControlPhase::InputLocked
           : server.Busy()     ? imkit::ExternalControlPhase::Active
           : server.Attached() ? imkit::ExternalControlPhase::Connected
                               : imkit::ExternalControlPhase::Off;
view.client = server.ClientName();
view.activity = server.CurrentCommand();
if (server.TakeNewCommand())
    remote.Pulse();
// After the windows it should cover:
const auto request = imkit::ExternalControlIndicator("remote", view, remote, ImGui::GetTime(), look, nullptr, {&theme});
if (request.stopRequested)
    server.Disconnect();
```

- `ExternalControlStyle` holds one `GlowFrameStyle` per phase (`connected`, `active`, `locked`), the pill position (`Hidden`, a corner, or the top or bottom centre), `pillMargin`, `lockedTint` and `pulseDot`. Every field can be changed after `MakeExternalControlStyle`.
- The frame draws on the viewport's foreground draw list. The pill is a small window, so Stop takes clicks and keyboard focus like any other button. Stop is reported only while the phase is not `Off`.
- `needsFrame` is true while the picture changes over time. The indicator does not hold back input; in `InputLocked`, the host blocks local input itself.
- The frame options are described in [Glow frames](../architecture/make-your-own-look.md#glow-frames).

## Themes and fonts

- `ThemePicker` returns a `ThemePreset` selection only. Persist the stable ID from
  `ThemePresets()` in host settings and call `MakeTheme`/`ApplyTheme` outside a
  frame to apply it.
- `imkit_copy_font_assets(target, destination)` stages the optional Inter
  and Noto Sans JP files; the host still loads and owns its font atlas.
