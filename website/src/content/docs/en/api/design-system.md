---
title: "Design-system header map"
---

This map is generated from [`docs/reference/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/design-system-api.json). It records 8 public-header/type groups and the related contract page. It is header metadata, **not a complete function-signature inventory**.

Use the [design-system guide](../../features/design-system/) and each public header for behavior and declarations.

| Public header | Type groups recorded in the metadata |
|---|---|
| [`include/imkit/theme.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/theme.h) | `ColorScheme`, `ContrastMode`, `Density`, `Easing`, `Typography`, `SpacingTokens`, `Radius`, `Stroke`, `Elevation`, `Opacity`, `StateColors`, `SemanticColors`, `Palette`, `Metrics`, `Motion`, `FontSet`, `EditorPalette`, `Theme`, `ThemeScope`, `AnimationState`, `ThemePreset` |
| [`include/imkit/accessibility.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility.h) | `SemanticRole`, `SemanticAction`, `SemanticState`, `SemanticNode`, `AccessibilityTree`, `AccessibilitySink`, `ActionRequest`, `ActionQueue`, `AccessibilityFrame` |
| [`include/imkit/accessibility_win32.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility_win32.h) |  |
| [`include/imkit/locale.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/locale.h) | `TextDirection`, `LocaleContext` |
| [`include/imkit/components.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/components.h) | `ActionVariant`, `CheckState`, `StatusKind`, `CompactActionRowRequest`, `ComponentOptions`, `CompactActionRowOptions`, `Notification` |
| [`include/imkit/patterns.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/patterns.h) | `Command`, `CommandPaletteState`, `SplitButtonResult`, `ToolbarState`, `DialogState`, `DialogResult`, `FormFieldInfo`, `AdaptiveSplitState`, `VisibleRange`, `ListProvider`, `DataColumn`, `DataAction`, `DataEvent`, `DataProvider`, `DataTableState` |
| [`include/imkit/workflow.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/workflow.h) | `WorkspaceTab`, `ChoiceItem`, `ChoiceGroupOptions`, `HierarchyRowView`, `HierarchyRowAction` |
| [`include/imkit/toast.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/toast.h) | `ToastView`, `ToastPosition`, `ToastPhase`, `ToastViewportState`, `ToastViewportOptions`, `ToastEvent`, `ToastEventBuffer` |
