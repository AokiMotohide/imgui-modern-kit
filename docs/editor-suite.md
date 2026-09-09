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

Commit/Cancel intent survives overflow. Widgets retry pending terminal events on
subsequent calls even if the edited row is clipped. A pending Cancel cannot become
Commit, and a pending Commit no longer accepts value updates. Timeline reserves the
complete multi-clip batch before Begin, Update or termination; insufficient scratch
or event capacity is reported without silently dropping selected members. A locked
member rejects the whole linked/group move. Providers must return unique member IDs
and increment revision when a target disappears or its restrictions change.
Commit/Cancelの意図はoverflow後も保持し、編集行がclipされても次回widget呼出しで再送します。
Cancel待機はCommitへ変わらず、Commit待機は新しい提案値を受け付けません。Timelineは複数clipの
Begin・Update・終了に必要なevent容量をまとめて確認し、scratch不足時に一部対象だけを開始しません。
locked memberを含む移動は全体を拒否します。providerはmember IDを重複させず、対象消失・制限変更時に
revisionを進めてください。

`CanvasState`, `VisibleRange`, `ZoomAt`, `Fit`, `InPolygon`, `ResolveSnap` are reusable
math operations. `BeginCanvas`/`EndCanvas` pair around public DrawList content.
`CurveProvider` must return time-sorted keys per channel with neighboring keys for
interpolation. `PropertyProvider` and `AssetProvider` return only clipped, host-filtered
rows; `count` is the filtered count. UI performs no full-dataset search.
canvasとsnapは純粋計算として使用できます。Curve queryはchannel別時刻順と補間用隣接keyを
返します。Property/Assetの検索とfiltered countはホストが用意し、UIは可視行だけを要求します。

Current widgets: TimeRuler, Transport (including J/K/L), CurveEditor, PropertyGrid,
AssetBrowser, Splitter and StatusBar. CanvasSelection provides box/lasso selection
against a bounded point query. ResolveHandles implements Auto, Auto Clamped and
Vector tangents; MoveHandle preserves aligned tangents. Asset rename emits fixed-size
UTF-8 text events. Core multi-key transforms and full property-state actions remain
incomplete. Built-in toolbar labels currently use English.
時間軸・JKLを含むtransport・Curve・Property・Asset・splitter・statusを実装しています。
CanvasSelectionは範囲queryでbox/lasso選択を行います。自動接線とAligned handleの純粋計算、
Asset renameの固定長UTF-8 eventを提供します。複数key変換と全property状態の操作は未完成です。
組込toolbar文字列は現時点では英語です。

Curve rendering and handle positions use the same channel-local neighboring keys
as evaluation. Handle events store the side in `Value::offset` (-1 left, +1 right);
the host applies `MoveHandle` to the resolved starting key to preserve Aligned
constraints and convert a manually edited automatic tangent to Free.
Curveの描画・handle位置・評価は同じchannel内の隣接keyを使用します。Handle eventの
`Value::offset`は左-1・右+1です。ホストは開始keyの接線を解決してからMoveHandleを適用し、
Alignedの対向接線を維持し、自動接線の手動編集をFreeへ変更します。

Focused test: `ctest --test-dir build/windows-debug -C Debug -R imkit.editor_core --output-on-failure`.
The test checks time boundaries, exact event ticks above 2^53, cancellation, snap,
canvas transforms and curve interpolation. It is CPU evidence, not native UI acceptance.
focused testは時間境界・整数精度・cancel・snap・canvas・補間を検証し、native操作の証拠とは区別します。

## Video / 動画

`imkit::video` provides Timeline, Monitor, Waveform, LevelMeter, Histogram,
ScopeImage and color draft controls. EditClip, RollClips, SlideClip and SplitClip
are pure constrained edit operations. Timeline requests visible tracks and clips,
uses caller-owned drag storage for multiple clips, and emits independent events for
adjacent edits. Provider-selected members may include linked/group clips; locked
members must be marked by the host. Callbacks and returned views are non-owning.
VideoはTimeline・Monitor・波形・meter・scopeと色draft操作を公開します。編集計算は
純粋関数です。Timelineは可視track/clipを要求し、複数clipのdrag領域はホストが渡します。
linked/group対象はproviderが返し、locked trackのmemberもlockedと指定します。

Audio buckets use interleaved PCM and an explicit channel. Scope utilities operate
on CPU RGBA values without decoding, resampling, playback or color management.
Those services, timeline collision policy, undo, media loading and persistence remain
host responsibilities. Monitor flipY explicitly selects texture UV orientation.
PCMはinterleaved spanとchannelを指定します。scopeはCPU RGBAだけを集計し、decode・再生・
resample・色管理は行いません。衝突方針・Undo・media loading・保存もホスト責務です。

`ScopeBuffers` accepts optional red/green/blue waveform spans, each `width * 256`
bins. Supply all three or leave all empty. Histogram and waveform bin ranges are
clamped to [0,1]; nonfinite components become zero. The tinted `ScopeImage` overload
supports RGB parade rendering. Existing luma/vectorscope buffers remain required.
ScopeBuffersのRGB waveformは各width×256 binで、3本とも指定するかすべて空にします。
binは[0,1]へclampし、非有限成分は0として集計します。色指定のScopeImage overloadでRGB paradeを
描画できます。既存のluma/vectorscope領域は引き続き必須です。

Three-way ColorControls now provide hue/chroma disks plus independent neutral-level
sliders for Lift/Gamma/Gain, and Temperature/Tint/Exposure sliders. Disk edits
preserve the RGB mean; double-click centers chroma. The original overload edits a
host-owned draft. The event overload accepts immutable `ColorValues`, six explicit
`ColorPropertyIds`, host-owned `ColorState`, revision and `EventBuffer`; it emits
Begin/Update/Commit/Cancel without mutating source values. Zero property IDs disable
their controls. RGB properties use `Value::x/y/z`, scalar properties use x. Supply
UTF-8 `ColorLabels` for localization. These are grading controls, not a color-managed
image processing or playback engine.
ColorControlsはLift/Gamma/Gainそれぞれの色相・彩度diskと独立した平均level、Temperature/Tint/Exposureを
操作できます。disk編集はRGB平均を維持し、double-clickで彩度を中心へ戻します。既存overloadはホスト所有の
draftを編集します。event overloadは不変ColorValues・明示的な6つのproperty ID・ホスト所有state・revision・
event bufferを受け、元データを変更せずBegin/Update/Commit/Cancelを返します。IDが0の操作はdisabledです。
RGBはValueのx/y/z、scalarはxを使い、表示文字列はUTF-8のColorLabelsで指定できます。色管理・画像処理・再生
engineを提供するものではありません。GalleryのColorタブはeventを合成ホストの色設定へ適用します。

## CG / CG編集

`imkit::cg` provides camera projection, navigation, object-origin picking, axis gizmo,
Outliner restriction/reparent events, UV transforms, DopeSheet and generic animation
strip movement. `OrientationBasis` resolves world/local/view/parent/custom axes.
Host-supplied pivotPosition represents the median, bounds or cursor location.
Scene geometry, hierarchy validation, selection and animation runtime stay host-owned.
CGは投影・navigation・object origin選択・軸gizmo・Outlinerの制限/reparent event・UV変換・
Dope Sheet・animation strip移動を提供します。orientationは純粋関数で計算し、pivotPositionは
ホストが計算します。scene geometry・階層検証・選択・animation runtimeは所有しません。

## Preview / 簡易描画

`DrawListPreview` belongs to `imkit::cg`. It projects indexed non-owning mesh spans,
uses host triangle scratch, and depth-sorts triangles. It has no z-buffer: intersecting
or cyclic surfaces can be incorrect. Cube and Sphere fill host buffers. CPU normals
and GL shading use inverse-scale and rotation transformed normals for simple
world-space Lambert lighting. A zero scale axis contributes a zero normal component;
fully degenerate normals receive ambient lighting only.
DrawListPreviewはcgに含まれ、ホストのtriangle scratchでdepth sortします。z-bufferはなく、
交差面や循環する重なりは正確ではありません。Cube/Sphereはホストbufferへ生成します。
簡易Lambertは逆scaleと回転を適用したworld-space normalを使います。scaleが0の軸のnormal成分は0とし、
完全に退化したnormalは環境光だけで表示します。

`imkit::preview_opengl3` is separate from `editor_suite`. Include `<imkit/preview.h>`.
The host supplies a complete GLFunctions table and makes its OpenGL 3.3 context
current before Init, Resize, Render, Pick and Shutdown. The object owns only its FBO,
RGBA color, integer ID and depth textures, shaders, VAO and buffers. It never creates
a context or loads GL functions. Call Shutdown before context destruction; the
object destructor deliberately performs no GL calls. Copying is disabled.
preview_opengl3はeditor_suiteに含めません。ホストがGL関数表とcurrent contextを用意します。
renderer objectは自身のFBO・color/ID/depth texture・shader・VAO/bufferだけを所有します。
Context破棄前にShutdownが必須です。デストラクタはGL関数を呼ばず、copyは禁止します。

The current renderer changes GL bindings, viewport, depth, cull, blend, scissor and
polygon state, and ends with framebuffer/program/VAO bound to zero. The host must
re-establish its render state before subsequent passes. Pick returns the exact
64-bit StableId; zero means background. Resize invalidates the previous texture ID.
現在のrendererはGL stateを変更します。後続passのstateはホストが再設定してください。
終了時FBO/program/VAOは0です。Pickは64bit ID、背景は0を返します。Resize後はtexture IDを
取得し直してください。GL resource lifecycleは同じcurrent contextで管理します。

## Integration and remaining acceptance / 統合と未達項目

Gallery pages 7, 8 and 9 use the public modules for Editor Core, Video and CG.
The sample host applies commit events, increments revisions and synchronizes
object selection. Its data and edited labels are synthetic. The 100k dataset toggle
constructs 256 tracks, 100096 clips and 100000 keys. No media runtime is implied.
Gallery 7/8/9は公開moduleでCore/Video/CGを構成します。sample hostがcommit eventを適用して
revisionを進め、object選択を同期します。100k切替は256 track・100096 clip・100000 keyです。

The requested 1.0 suite is **not complete**. Remaining work includes fully integrated
box/lasso and multi-key workflows, per-track variable heights, complete track flags,
transition handles/picker, linked/group sample policy, audio envelope editing,
gizmo plane/screen handles and full pivot rotation,
navigation gizmo, hierarchy rename/reorder and stack inspector, UV edge/face/island
interaction, strip scale/repeat/blend editing, editor-specific icon expansion,
complete localization and all requested representative input checks. Provider search
controls exist, but the Gallery's sample providers do not yet apply every filter.
依頼された1.0 Suiteは**未完成**です。box/lassoと複数key操作の統合、可変track高と全flag、
transition編集/picker、linked/groupのsample処理、audio envelope、
gizmoのplane/screen handleとpivot回転、navigation gizmo、階層rename/reorderとstack inspector、
UVのedge/face/island操作、strip scale/repeat/blend、editor icon追加、完全な表示文字列差替え、
全代表操作の検証が残っています。Gallery providerでは全検索条件の適用も未完了です。

Version remains 0.2.0 until 1.0 acceptance is complete. No 1.0 tag or Release is created.
1.0の受入完了まではversionを0.2.0に保持し、1.0 tag/Releaseは作成しません。
