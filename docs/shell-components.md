# Application shell components

ImKit provides host-owned building blocks for persistent application chrome:
`AppBar`, `WorkspaceHeader`, `InspectorSection`, `AdvancedSection`,
`BottomActionBar`, `DiagnosticsDrawer`, and `ThemePicker`.

These components borrow labels and command spans, render the current host state,
and return command or selection requests. They do not own documents, docking,
settings, undo history, workers, renderers, or an ImGui context.

```cpp
imkit::AppBarView view{
    "Product", "project.pmproj", "ALIGN", "Ready", false,
    imkit::FeedbackKind::Success, commands};
if (auto action = imkit::AppBar("project", view, toolbar, options))
    Dispatch(action);
```

`ThemePicker` returns a `ThemePreset` selection only. Persist the stable ID from
`ThemePresets()` in host settings and call `MakeTheme`/`ApplyTheme` outside a
frame. `imkit_copy_font_assets(target, destination)` stages the optional Inter
and Noto Sans JP files; the host still loads and owns its font atlas.
