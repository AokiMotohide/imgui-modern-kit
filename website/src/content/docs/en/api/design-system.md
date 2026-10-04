---
title: "Design-system header map"
---

This map is generated from [`docs/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/design-system-api.json). It records 5 public-header/type groups and the related contract page. It is header metadata, **not a complete function-signature inventory**.

Use the [design-system guide](../../features/design-system/) and each public header for behavior and declarations.

| Public header | Type groups recorded in the metadata |
|---|---|
| [`include/imkit/theme.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/theme.h) | `ColorScheme`, `ContrastMode`, `Density`, `Easing`, `Typography`, `SpacingTokens`, `Radius`, `Stroke`, `Elevation`, `Opacity`, `StateColors`, `SemanticColors`, `Palette`, `Metrics`, `Motion`, `FontSet`, `EditorPalette`, `Theme`, `ThemeScope`, `AnimationState` |
| [`include/imkit/accessibility.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility.h) | `SemanticRole`, `SemanticAction`, `SemanticState`, `SemanticNode`, `AccessibilityTree`, `AccessibilitySink`, `ActionRequest`, `ActionQueue`, `AccessibilityFrame` |
| [`include/imkit/accessibility_win32.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility_win32.h) | `Win32ActionSink` |
| [`include/imkit/locale.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/locale.h) | `TextDirection`, `LocaleContext` |
| [`include/imkit/patterns.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/patterns.h) | `Command`, `CommandPaletteState`, `SplitButtonResult`, `ToolbarState`, `DialogState`, `DialogResult`, `FormFieldInfo`, `AdaptiveSplitState`, `VisibleRange`, `ListProvider`, `DataColumn`, `DataAction`, `DataEvent`, `DataProvider`, `DataTableState` |
