# アプリケーションShell部品

ImKitは`AppBar`、`WorkspaceHeader`、`InspectorSection`、`AdvancedSection`、
`BottomActionBar`、`DiagnosticsDrawer`、`ThemePicker`を提供します。

各部品はlabelとcommandを借用し、ホスト状態を表示して操作要求だけを返します。
document、Dock配置、設定、Undo履歴、worker、renderer、ImGui Contextは所有しません。

`ThemePicker`の結果はホスト設定へ安定IDで保存し、frame外で`MakeTheme`と
`ApplyTheme`を呼びます。`imkit_copy_font_assets(target, destination)`は任意の
Inter＋Noto Sans JP資産を利用側へ配置しますが、font atlasの読込と寿命はホスト所有です。
