---
title: "Design-system header一覧"
---

このmapは[`docs/reference/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/design-system-api.json)から生成しています。8つの公開headerと型のまとまり、契約ガイドへの対応を示すheader metadataです。**関数signatureの完全一覧ではありません。**

動作と契約は[design-systemガイド](../../features/design-system/)および各公開headerを確認してください。

| 公開header | metadataに記録された型 |
|---|---|
| [`include/imkit/theme.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/theme.h) | `ColorScheme`, `ContrastMode`, `Density`, `Easing`, `Typography`, `SpacingTokens`, `Radius`, `Stroke`, `Elevation`, `Opacity`, `StateColors`, `SemanticColors`, `Palette`, `Metrics`, `Motion`, `FontSet`, `EditorPalette`, `Theme`, `ThemeScope`, `AnimationState`, `ThemePreset` |
| [`include/imkit/accessibility.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility.h) | `SemanticRole`, `SemanticAction`, `SemanticState`, `SemanticNode`, `AccessibilityTree`, `AccessibilitySink`, `ActionRequest`, `ActionQueue`, `AccessibilityFrame` |
| [`include/imkit/accessibility_win32.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility_win32.h) |  |
| [`include/imkit/locale.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/locale.h) | `TextDirection`, `LocaleContext` |
| [`include/imkit/components.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/components.h) | `ActionVariant`, `CheckState`, `StatusKind`, `CompactActionRowRequest`, `ComponentOptions`, `CompactActionRowOptions`, `Notification` |
| [`include/imkit/patterns.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/patterns.h) | `Command`, `CommandPaletteState`, `SplitButtonResult`, `ToolbarState`, `DialogState`, `DialogResult`, `FormFieldInfo`, `AdaptiveSplitState`, `VisibleRange`, `ListProvider`, `DataColumn`, `DataAction`, `DataEvent`, `DataProvider`, `DataTableState` |
| [`include/imkit/workflow.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/workflow.h) | `WorkspaceTab`, `ChoiceItem`, `ChoiceGroupOptions`, `HierarchyRowView`, `HierarchyRowAction` |
| [`include/imkit/toast.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/toast.h) | `ToastView`, `ToastPosition`, `ToastPhase`, `ToastViewportState`, `ToastViewportOptions`, `ToastEvent`, `ToastEventBuffer` |
