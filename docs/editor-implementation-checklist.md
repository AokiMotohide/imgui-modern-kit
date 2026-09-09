# Editor 1.0 implementation and acceptance / 実装確認表

This table tracks the supplied 1.0 requirements against code, host application and
acceptance evidence. Partial implementation is not acceptance. Rows remain open
until the named operations and their direct checks are complete.
添付の1.0要求を実装・ホスト適用・検証へ対応付けます。部分実装を合格にせず、
各操作と直接検証が揃うまで未完了として管理します。

| Requirement / 要求 | Public implementation / 実装 | Host and evidence / 適用・検証 | Status / 状態 |
|---|---|---|---|
| Stable IDs, ticks, timecode, transactions, overflow | editor_core.h / editor_core.cpp | tests/editor_core.cpp; termination and disappearance audit pending | Partial / 部分 |
| Canvas zoom/pan/fit/grid/rulers, box/lasso, visible queries | BeginCanvas, CanvasSelection | editor_workspaces.cpp; full selection integration pending | Partial / 部分 |
| Time ruler, ranges, marker, loop, auto-scroll, commands | TimeRuler, Transport, MakeBindings | Complete binding routing and range interactions pending | Partial / 部分 |
| Multi-key move/duplicate/snap and navigation | CurveEditor, DopeSheet | Multi-key host application and IO checks pending | Partial / 部分 |
| Curve channels/interpolation/handles/extrapolation/ghost | Evaluate, ResolveHandles, MoveHandle, CurveEditor | tests/editor_core.cpp; neighbor-aware rendering correction in progress | Partial / 部分 |
| Property categories/mixed/reset/override/favorite/lock/animation | PropertyGrid | Full state actions and Inspector application pending | Partial / 部分 |
| Asset grid/list/thumbnail/path/filter/tag/rename/status | AssetBrowser | Clickable path and complete host filtering pending | Partial / 部分 |
| Toolbar/status/splitter/rename/context/tooltip | editor_core components | Localization and icon integration pending | Partial / 部分 |
| Track roles/heights/collapse and all header controls | video::Timeline | Variable heights and all flag application pending | Partial / 部分 |
| Clip presentation, move/duplicate/trim/split/ripple/roll/slip/slide | EditClip, RollClips, SlideClip, SplitClip, Timeline | tests/video.cpp; full linked/group/locked multi-edit and IO acceptance pending | Partial / 部分 |
| Snap targets/guide/magnet, scrolling/zoom/fit | Timeline | Complete targets and representative gesture checks pending | Partial / 部分 |
| Transition/caption/property keys | Timeline, Video workspace | Handles, picker, inline edits and host application pending | Open / 未完了 |
| Media bin/monitors/transport/overlays/Inspector | Monitor, Transport, Video workspace | Selection routing, overlay controls and metadata presets pending | Partial / 部分 |
| PCM buckets/envelope/meter/hold/pan/fader/track audio | Waveform, LevelMeter, CPU utilities | tests/video.cpp; envelope and complete audio controls pending | Partial / 部分 |
| Histogram/luma/RGB/vector scopes, three-way wheels, color curves | ScopeImage, Histogram, BuildScopes, ColorControls | RGB bins and three wheel public-IO transactions tested; Gallery host application connected; full color curve controls pending | Partial / 部分 |
| Camera modes/shading/overlays/navigation/alignment | cg viewport and camera utilities | Orthographic range, navigation gizmo and full display routing pending | Partial / 部分 |
| Click/box/lasso, restrictions and selection sync | cg viewport, Outliner | Origin picking exists; complete selection workflows pending | Partial / 部分 |
| Gizmo axes/planes/screen, orientations/pivots/snap/multi-object | OrientationBasis, gizmo utilities | Plane/screen handles and pivot transforms pending | Partial / 部分 |
| Hierarchy/restrictions/search/rename/reorder/reparent/link/duplicate | Outliner | Host hierarchy validation/application and IO acceptance pending | Partial / 部分 |
| Transform/component/category/array Inspector | PropertyGrid, CG workspace | Full stack and array operations pending | Partial / 部分 |
| Dope Sheet/Graph/animation strip operations | DopeSheet, CurveEditor, animation strips | Multiple key and strip scale/repeat/blend/reorder pending | Partial / 部分 |
| UV coordinate modes, vertex/edge/face/island, selection/transform/overlays | UV editor and transform utilities | Complete selection units and transforms pending | Partial / 部分 |
| DrawList primitives/mesh/Lambert/depth sort | DrawListPreview, Cube, Sphere | Normal transformation and camera/light display pending; no z-buffer | Partial / 部分 |
| OpenGL lifecycle/mesh/depth/normal/resize/64-bit picking | preview:: renderer | Existing GPU verifier; transformed normal acceptance pending | Partial / 部分 |
| Existing 120 icons plus operation mapping and new originals | icons.h, catalog.json, build_icons.py | Dynamic atlas, operation mapping, individual originals and integration pending | Open / 未完了 |
| Semantic tokens, native Core/Video/CG/Icon Gallery, Undo | Theme, editor_workspaces.cpp | Full data application, filtering, Undo and visual acceptance pending | Partial / 部分 |
| CPU/API/consumer/install/package/IO/GPU | tests, Gallery verifier | Existing focused fixtures; changed contract checks and final acceptance pending | Partial / 部分 |
| 100k Release performance: pan/zoom/selection/clip/trim/key drag | Gallery performance fixture | Prior pan-only evidence; other operations pending | Partial / 部分 |
| English/Japanese docs, 1.0 version, commits, origin/main | README, docs, CHANGELOG, CMake | Version remains 0.2.0 until all required acceptance completes | Open / 未完了 |

Native OS/IME, media runtime and real project integration are separate evidence
categories. No such acceptance is inferred from public ImGui IO or GPU captures.
native OS/IME・media runtime・実project統合は公開IO・GPU captureと区別します。
