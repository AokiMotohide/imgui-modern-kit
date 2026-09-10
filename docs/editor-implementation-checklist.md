# Editor Suite 1.0 acceptance / 実装確認表

Requirements are mapped to public code, host application and representative evidence.
各要求を公開実装・ホスト適用・代表検証へ対応付けます。詳細・測定条件・未実施区分は[検証結果](editor-validation.md)を参照してください。

| Requirement / 要求 | Public implementation / 実装 | Evidence / 根拠 | Status / 状態 |
|---|---|---|---|
| Stable IDs, ticks, timecode, transactions, overflow | editor_core.h / editor_core.cpp | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Canvas zoom/pan/fit/grid/rulers, box/lasso, visible queries | BeginCanvas, CanvasSelection | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Time ruler, ranges, marker, loop, auto-scroll, commands | TimeRuler, Transport, MakeBindings | Core public IO plus host verifier: range/marker transactions, property state, array reorder, copy/paste controls / 公開IO・ホスト適用。 | Implemented / 代表確認済み |
| Multi-key move/duplicate/snap and navigation | CurveEditor, DopeSheet | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Curve channels/interpolation/handles/extrapolation/ghost | Evaluate, ResolveHandles, MoveHandle, CurveEditor | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Property categories/mixed/reset/override/favorite/lock/animation | PropertyGrid | Core public IO plus host verifier: range/marker transactions, property state, array reorder, copy/paste controls / 公開IO・ホスト適用。 | Implemented / 代表確認済み |
| Asset grid/list/thumbnail/path/filter/tag/rename/status | AssetBrowser | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Toolbar/status/splitter/rename/context/tooltip | editor_core components | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Track roles/heights/collapse and all header controls | video::Timeline, TrackExtent, TrackLayout | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Clip presentation, move/duplicate/trim/split/ripple/roll/slip/slide | EditClip, RollClips, SlideClip, SplitClip, Timeline | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Snap targets/guide/magnet, scrolling/zoom/fit | Timeline | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Transition/caption/property keys | Timeline, Video workspace | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Media bin/monitors/transport/overlays/Inspector | Monitor, Transport, Video workspace | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| PCM buckets/envelope/meter/hold/pan/fader/track audio | Waveform, LevelMeter, AudioStrip, CPU utilities | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Histogram/luma/RGB/vector scopes, three-way wheels, color curves | ScopeImage, Histogram, BuildScopes, ColorControls, ApplyColorCurves, CurveEditor | tests/video.cpp, host related edits/color/envelope/caption/marker application; native Video/GPU captures / Video・ホスト・native確認。 | Implemented / 代表確認済み |
| Camera modes/shading/overlays/navigation/alignment | cg viewport and camera utilities | tests/cg.cpp, host selection/affine/animation/UV application and native CG light/dark/150% captures / CG・ホスト・native確認。 | Implemented / 代表確認済み |
| Click/box/lasso, restrictions and selection sync | cg viewport, Outliner | tests/cg.cpp, host selection/affine/animation/UV application and native CG light/dark/150% captures / CG・ホスト・native確認。 | Implemented / 代表確認済み |
| Gizmo axes/planes/screen, orientations/pivots/snap/multi-object | OrientationBasis, gizmo utilities | tests/cg.cpp, host selection/affine/animation/UV application and native CG light/dark/150% captures / CG・ホスト・native確認。 | Implemented / 代表確認済み |
| Hierarchy/restrictions/search/rename/reorder/reparent/link/duplicate | Outliner | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| Transform/component/category/array Inspector | PropertyGrid, CG workspace | Core public IO plus host verifier: range/marker transactions, property state, array reorder, copy/paste controls / 公開IO・ホスト適用。 | Implemented / 代表確認済み |
| Dope Sheet/Graph/animation strip operations | DopeSheet, CurveEditor, animation strips | tests/cg.cpp, host selection/affine/animation/UV application and native CG light/dark/150% captures / CG・ホスト・native確認。 | Implemented / 代表確認済み |
| UV coordinate modes, vertex/edge/face/island, selection/transform/overlays | UV editor and transform utilities | tests/cg.cpp, host selection/affine/animation/UV application and native CG light/dark/150% captures / CG・ホスト・native確認。 | Implemented / 代表確認済み |
| DrawList primitives/mesh/Lambert/depth sort | DrawListPreview, Cube, Sphere | tests/cg.cpp and native indexed-mesh/depth/64-bit pick/resize/lifecycle/normal checks / CPUと実GPU確認。 | Implemented / 代表確認済み |
| OpenGL lifecycle/mesh/depth/normal/resize/64-bit picking | preview:: renderer | tests/cg.cpp and native indexed-mesh/depth/64-bit pick/resize/lifecycle/normal checks / CPUと実GPU確認。 | Implemented / 代表確認済み |
| Existing 120 icons plus operation mapping and new originals | icons.h, catalog.json, build_icons.py | 206 originals, 1236 PNGs, 6 atlases; generation/ID/alpha tests and native 16px/150% review / 生成・ID・alpha・native確認。 | Implemented / 代表確認済み |
| Semantic tokens, native Core/Video/CG/Icon Gallery, Undo | Theme, editor_workspaces.cpp | Core/API/context fixtures, host verifier and native Core/Video/CG pages / CPU・公開IO・ホスト適用・native代表画面。 | Implemented / 代表確認済み |
| CPU/API/consumer/install/package/IO/GPU | tests, Gallery verifier | 7 Debug/Release fixtures, Debug external source consumer, installed Release SDK consumer / compile・link・実行合格。 | Implemented / 代表確認済み |
| 100k Release performance: pan/zoom/selection/clip/trim/key drag | Gallery performance fixture | Six operations: P95 1.6981–1.8727 ms, zero measured steady allocations / 6操作・定常割当0。docs/evidence/editor-performance.csv | Implemented / 代表確認済み |
| English/Japanese docs, 1.0 version, commits, origin/main | README, docs, CHANGELOG, CMake | 1.0.0 metadata, bilingual guides/API/contracts, scoped Japanese commits / 日英文書・version・関連commit。 | Implemented / 代表確認済み |

Native OS/IME and real-project integration were not performed; they are not implied by these results.
native OS/IMEと実project統合は未実施であり、本結果から合格を推定しません。
