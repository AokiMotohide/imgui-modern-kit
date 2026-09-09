# Editor Suite development / Editor Suite開発

The editor modules are being implemented against the host-owned provider/event contract.
This document records the current implementation, not a 1.0 acceptance claim.
Editor moduleはホスト所有のprovider/event契約で実装中です。本書は現状を記録し、1.0完成を宣言しません。

## Editor Core

Link `imkit::editor_core` and include `<imkit/editor_core.h>`. The host owns every
provider, span, revision, selection buffer, UI state, event buffer and undo history.
All spans remain valid throughout the widget call. IDs must be unique and nonzero.
`imkit::editor_core`と`<imkit/editor_core.h>`を使用します。provider、span、revision、
選択buffer、UI state、event buffer、Undoはすべてホスト所有です。spanは呼出し中有効、IDは一意かつ非ゼロとします。

`TicksPerSecond = 705600000`, `FrameRate`, frame/tick conversion, strict timecode parsing
and 29.97/59.94 drop-frame formatting support negative pre-roll. Invalid rate returns
zero from conversion functions and false from timecode functions. Tick rounding is to
the nearest integer. Timecode parsing currently accepts two-digit hours.
時間正本は毎秒705600000 tickです。負のpre-roll、通常timecode、29.97/59.94 drop-frameに
対応します。不正rateは変換で0、timecodeでfalseを返します。tickは最近接整数へ丸めます。
解析する時フィールドは現時点では2桁です。

`Transaction` emits Begin/Update/Commit/Cancel with original and proposed values.
Apply previews separately from committed data. Keep the revision unchanged during the
gesture; increment it only on an accepted commit or external edit. Revision mismatch
cancels the gesture. A full `EventBuffer` sets `overflow`; the host must reserve enough
room or retry pending transaction completion with a cleared buffer. No event silently
overwrites another. `Value` stores times/IDs separately from floating point values.
Transactionは元値と提案値を返します。previewは確定データと分離し、gesture中のrevisionは
保持します。確定または外部変更でrevisionを進めると継続dragをcancelします。buffer不足は
overflowで通知し、ホストが容量確保またはbufferを空にして終端eventを再試行します。

`CanvasState`, `VisibleRange`, `ZoomAt`, `Fit`, `InPolygon`, `ResolveSnap` are reusable
math operations. `BeginCanvas`/`EndCanvas` pair around public DrawList content.
`CurveProvider` must return time-sorted keys per channel with neighboring keys for
interpolation. `PropertyProvider` and `AssetProvider` return only clipped, host-filtered
rows; `count` is the filtered count. UI performs no full-dataset search.
canvasとsnapは純粋計算として使用できます。Curve queryはchannel別時刻順と補間用隣接keyを
返します。Property/Assetの検索とfiltered countはホストが用意し、UIは可視行だけを要求します。

Current widgets: TimeRuler, Transport, CurveEditor (key/handle drag), PropertyGrid
(numeric edit/reset/key request), AssetBrowser (grid/list/selection/asset drag payload),
Splitter, StatusBar. Property/asset labels are host UTF-8 strings; built-in toolbar
labels currently use English. Curve handle modes are represented in views; automatic
tangent derivation is not yet implemented. Box/lasso gesture UI, multi-key transforms,
asset rename and additional property flags still require implementation.
現時点では時間軸・transport・key/handle drag・数値property編集/reset/key要求・asset選択と
payload・splitter・status表示を実装しています。自動接線、box/lasso gesture、複数key変換、
asset rename、追加property flag操作は未実装です。組込toolbar文字列は現時点で英語です。

Focused test: `ctest --test-dir build/windows-debug -C Debug -R imkit.editor_core --output-on-failure`.
The test checks time boundaries, exact event ticks above 2^53, cancellation, snap,
canvas transforms and curve interpolation. It is CPU evidence, not native UI acceptance.
focused testは時間境界・整数精度・cancel・snap・canvas・補間を検証し、native操作の証拠とは区別します。
