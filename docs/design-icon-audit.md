# Icon decisions / アイコン対応表

Existing 238 IDs retain their order; new IDs are appended. / 既存238 IDの順序を維持し、末尾へ追加。

| Candidate / 候補 | Decision / 判断 | ID |
|---|---|---|
| Shortcut | Reuse / 再利用 | Keyboard |
| PopOut | Reuse / 再利用 | ExternalLink |
| Table | Reuse / 再利用 | Grid |
| Tree | Reuse / 再利用 | Hierarchy |
| Group | Reuse / 再利用 | Collection |
| SearchClear | Reuse / 再利用 | Close |
| OpenRecent | Reuse / 再利用 | History |
| Rename | Reuse / 再利用 | Edit |
| Retry | Reuse / 再利用 | Refresh |
| CommandPalette | New / 新規 | CommandPalette |
| Keyboard | New / 新規 | Keyboard |
| Mouse | New / 新規 | Mouse |
| Touch | New / 新規 | Touch |
| Accessibility | New / 新規 | Accessibility |
| ScreenReader | New / 新規 | ScreenReader |
| HighContrast | New / 新規 | HighContrast |
| ReduceMotion | New / 新規 | ReduceMotion |
| TextSize | New / 新規 | TextSize |
| RTL | New / 新規 | RTL |
| ChevronUp | New / 新規 | ChevronUp |
| ChevronDown | New / 新規 | ChevronDown |
| ChevronLeft | New / 新規 | ChevronLeft |
| ChevronRight | New / 新規 | ChevronRight |
| Dock | New / 新規 | Dock |
| Undock | New / 新規 | Undock |
| WindowMinimize | New / 新規 | WindowMinimize |
| WindowMaximize | New / 新規 | WindowMaximize |
| WindowRestore | New / 新規 | WindowRestore |
| Columns | New / 新規 | Columns |
| Rows | New / 新規 | Rows |
| ExpandAll | New / 新規 | ExpandAll |
| CollapseAll | New / 新規 | CollapseAll |
| Ungroup | New / 新規 | Ungroup |
| SortNeutral | New / 新規 | SortNeutral |
| FilterClear | New / 新規 | FilterClear |
| DragHandle | New / 新規 | DragHandle |
| Archive | New / 新規 | Archive |
| ArchiveRestore | New / 新規 | ArchiveRestore |
| FolderAdd | New / 新規 | FolderAdd |
| FolderMove | New / 新規 | FolderMove |
| Replace | New / 新規 | Replace |
| KeyCommand | New / 新規 | KeyCommand |
| KeyEnter | New / 新規 | KeyEnter |
| KeyEscape | New / 新規 | KeyEscape |
| KeyTab | New / 新規 | KeyTab |
| PanelLeftOpen | New / 新規 | PanelLeftOpen |
| PanelLeftClose | New / 新規 | PanelLeftClose |
| PanelRightOpen | New / 新規 | PanelRightOpen |
| PanelRightClose | New / 新規 | PanelRightClose |
| PanelBottomOpen | New / 新規 | PanelBottomOpen |
| PanelBottomClose | New / 新規 | PanelBottomClose |

New drawings are authored in `tools/design_icons.py` under the repository MIT license.
新規図形の正本は `tools/design_icons.py`。リポジトリMITライセンスを適用。

The 12px raster applies peak-alpha correction to faint downsampled strokes. Existing 16–64px assets retain their raster sources.
12pxのみ縮小で薄くなる線のalphaを補正。既存16〜64px資産は従来の画像正本を維持。

Full per-icon optical acceptance is not yet recorded; generated pixel checks are not a visual audit.
全件の光学的な受入記録は未完了。画素検査を目視監査の代わりに扱わない。
