---
title: "Design-system header一覧"
---

このmapは[`docs/design-system-api.json`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/design-system-api.json)から生成しています。5つの公開headerと型のまとまり、契約ガイドへの対応を示すheader metadataです。**関数signatureの完全一覧ではありません。**

動作と契約は[design-systemガイド](../../features/design-system/)および各公開headerを確認してください。

| 公開header | metadataに記録された型 |
|---|---|
| [`include/imkit/theme.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/theme.h) | `ColorScheme`, `ContrastMode`, `Density`, `Easing`, `Typography`, `SpacingTokens`, `Radius`, `Stroke`, `Elevation`, `Opacity`, `StateColors`, `SemanticColors`, `Palette`, `Metrics`, `Motion`, `FontSet`, `EditorPalette`, `Theme`, `ThemeScope`, `AnimationState` |
| [`include/imkit/accessibility.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility.h) | `SemanticRole`, `SemanticAction`, `SemanticState`, `SemanticNode`, `AccessibilityTree`, `AccessibilitySink`, `ActionRequest`, `ActionQueue`, `AccessibilityFrame` |
| [`include/imkit/accessibility_win32.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/accessibility_win32.h) | `Win32ActionSink` |
| [`include/imkit/locale.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/locale.h) | `TextDirection`, `LocaleContext` |
| [`include/imkit/patterns.h`](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/include/imkit/patterns.h) | `Command`, `CommandPaletteState`, `SplitButtonResult`, `ToolbarState`, `DialogState`, `DialogResult`, `FormFieldInfo`, `AdaptiveSplitState`, `VisibleRange`, `ListProvider`, `DataColumn`, `DataAction`, `DataEvent`, `DataProvider`, `DataTableState` |
