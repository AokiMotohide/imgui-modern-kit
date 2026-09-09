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

For variable-height tracks, provide `TimelineProvider::layout` and `totalHeight`.
The callback receives the visible pixel interval and returns contiguous tracks plus
the first track's absolute top. Build an indexed prefix-height table in the host;
the Gallery uses binary search and rebuilds its table only after data/layout edits.
Use `TrackExtent` for both that table and drawing: expanded rows use at least 64 px,
collapsed rows 32 px. Without a layout callback the original uniform-row query is
retained. Track labels open a context menu with a height slider emitting TrackHeight
transactions. Collapse, visible, mute, solo, lock, record, source and target controls
emit explicit TrackControl selectors and are applied to Gallery track state.
可変track高にはlayout callbackとtotalHeightを指定します。callbackは可視pixel区間に対する連続trackと先頭の
絶対topを返します。ホストで高さの累積表を保持し、Galleryはデータ・layout編集後に更新した表を二分探索します。
累積表と描画の両方にTrackExtentを使い、展開時は最低64px・折り畳み時は32pxです。callbackを省略すると既存の
均一行queryを使用します。track名のcontext menuから高さを編集でき、TrackHeight transactionを返します。
展開・visible・mute・solo・lock・record・source・targetの操作は明示selectorを返し、Galleryがtrack状態へ適用します。

Audio buckets use interleaved PCM and an explicit channel. Scope utilities operate
on CPU RGBA values without decoding, resampling, playback or color management.
Those services, timeline collision policy, undo, media loading and persistence remain
host responsibilities. Monitor flipY explicitly selects texture UV orientation.
PCMはinterleaved spanとchannelを指定します。scopeはCPU RGBAだけを集計し、decode・再生・
resample・色管理は行いません。衝突方針・Undo・media loading・保存もホスト責務です。

`AudioStripView::gainId` and `panId` explicitly identify independent properties;
zero IDs disable those controls. AudioStrip provides a native vertical gain fader
and pan slider, emitting transactions through the host `PropertyState`. Mute/Solo/
Record emit Toggle events on the strip ID with a `TrackControl` selector in x and
the boolean value in y. Locked strips emit no edits. Labels are host-supplied UTF-8.
`ClipView::audioBuckets` is a non-owning min/max bucket view for actual clip waveform
drawing. The Gallery supplies PCM buckets and a channel selector, applies edits to
host mixer values and shared track flags, and shows synthetic stereo meter response
with pan, gain and mute/solo. Record is an arming flag, not a recorder implementation.
AudioStripのgain/panは独立した明示IDで識別し、IDが0ならdisabledになります。nativeの縦faderとpan sliderは
ホストのPropertyStateを使ってtransactionを返します。Mute/Solo/Recordはstrip IDへのToggle eventで、xが
TrackControl・yがboolです。locked時は編集eventを出しません。表示文字列はホスト指定UTF-8です。
ClipView::audioBucketsは非所有のmin/max bucketでclip波形へ接続します。GalleryはPCM bucket・channel選択・
ホストのmixer値と共有track flagへの適用・pan/gain/mute/soloに応じた合成stereo meterを提供します。
Recordはarming状態であり、録音engineは含みません。

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
Gallery computes the selection-origin median and world bounds (transformed cube
vertices, origins for non-mesh entries), and offers editable cursor coordinates.
The pivot stays fixed during a drag. DrawList and OpenGL previews apply both the
transform and its companion position transaction.
Galleryは選択originの平均とworld bounds（cubeの変換後頂点、非mesh項目はorigin）を計算し、
cursor座標は入力で変更できます。drag中はpivotを固定し、DrawList・OpenGL双方のpreviewへ
姿勢／scaleと位置のtransactionを反映します。
Scene geometry, hierarchy validation, selection and animation runtime stay host-owned.
CGは投影・navigation・object origin選択・軸gizmo・Outlinerの制限/reparent event・UV変換・
Dope Sheet・animation strip移動を提供します。orientationは純粋関数で計算し、pivotPositionは
ホストが計算します。scene geometry・階層検証・選択・animation runtimeは所有しません。

Gallery builds Outliner rows from parent StableIds, recalculating depth and child
indicators after reparenting. Collapsed branches hide descendants. Search retains
matching rows and their ancestors, revealing matches inside collapsed branches.
Expand/Collapse hierarchy context actions update the host subtree. Reparent rejects
cycles, nonexistent parents and locked source/destination objects. Focused host-model
checks cover row construction, collapse, search ancestry and changed parent depth.
Galleryはparent StableIdからOutliner行を構築し、reparent後の深さ・子の有無を再計算します。
閉じた枝の子孫は非表示になり、検索中は一致行と祖先を表示して閉じた枝の一致も見せます。
階層全体の展開・折り畳みmenuはホストの子孫状態へ反映します。reparentはcycle・存在しない親・
lockedな移動元／移動先を拒否します。行構築・折り畳み・検索の祖先保持・親変更後の深さを
ホストmodel検証で確認しています。

Outliner context actions emit `Reorder` with the current parent and offset -1/+1
for sibling movement, or `Reparent` with parent 0 for Move to root. Gallery keeps
StableId order separate from object storage, swaps only siblings, and rejects locked
neighbors. Host-model tests confirm reordered rows and unchanged object identities.
OutlinerのMove up/downは現在のparentとoffset -1/+1を持つReorderを返し、Move to rootは
parent 0のReparentを返します。GalleryはStableIdの表示順をobject保存位置と別に管理し、
同じ親の行だけを交換します。lockedな隣接行は越えません。ホストmodel検証で行順変更と
object ID・保存位置の維持を確認しています。

Outliner Duplicate emits a host action for the selected row. Gallery appends an
object with a new StableId, nine separately allocated property IDs, copied transform
and restrictions, the same parent, and its own editable name. Mesh entries are
included in both preview paths. Object/property storage grows with duplication while
existing IDs and order references remain stable. Host-model checks prove independent
transform editing and property IDs. Hierarchy subtree duplication is not established by this single-object operation.
OutlinerのDuplicateは対象行のホスト操作を返します。Galleryは新しいStableId、個別に割り当てた
9個のproperty ID、コピーしたtransform・restriction、同じparent、独立した名前を持つobjectを追加します。
meshは両previewへ含めます。保存領域は複製に応じて増え、既存IDと表示順参照を維持します。
ホストmodel検証でproperty IDの独立性と元objectに影響しない移動を確認しています。
この単一object操作では子孫階層の複製は検証していません。

`ObjectView::geometry` is an optional opaque host geometry ID. Outliner offers Linked
duplicate when it is nonzero (`Duplicate`, proposed.offset=1). Ordinary duplication
copies Gallery vertex/index storage; linked duplication shares its geometry ID while
keeping an independent object transform and property IDs. `LinkGeometry` uses
proposed.parent as the source object ID; zero requests a private copy of current data.
Gallery applies Link geometry to active and Make geometry single user. Model checks
cover shared preview data, independent-copy isolation, unlink and relink. The library
does not allocate, clone, or own geometry itself.
ObjectView::geometryは任意のホストgeometry IDです。0以外の行にLinked duplicateを表示し、
Duplicateのproposed.offset=1で返します。通常複製ではGalleryの頂点／indexをコピーし、linkedでは
geometry IDを共有します。objectのtransformとproperty IDは個別です。LinkGeometryは
proposed.parentに参照元object IDを入れ、0では現在のデータを独立コピーします。
Galleryはactive objectとのlinkとsingle user化を適用し、共有preview・独立コピーの分離・unlink・
relinkをmodelで確認しています。ライブラリ自体はgeometryを確保・複製・所有しません。

Outliner starts inline rename from its context menu or a double click. The public
InputText widget edits the UTF-8 draft; Enter commits and Escape cancels. Rename
events preserve original/proposed text and starting revision; terminal overflow is
retried. Locked objects cannot start rename or drag. Gallery applies committed names
to its host object labels. Public-IO tests cover context activation, Japanese UTF-8
input, Commit and Cancel; this does not constitute native IME verification.
Outlinerは右クリックmenuまたはダブルクリックからinline renameを開始します。公開InputTextで
UTF-8 draftを編集し、Enterで確定、Escapeで取消します。eventには元名・提案名・開始revisionを保持し、
終了buffer不足では再試行します。locked objectはrename・dragを開始しません。Galleryは確定名を
ホストobjectへ適用します。公開IOでmenu開始・日本語UTF-8入力・確定・取消を検証しています。
native IMEの検証には含めません。

`TransformAroundPivot` returns a complete TRS value with the object's position rotated
or scaled about a supplied pivot. Its orthonormal basis determines constrained offset
movement. Rotation composes the existing orientation rather than adding Euler angles.
The gizmo pairs rotation/scale with a position transaction when an external pivot
changes the object offset. Begin, Update and terminal batches preflight event capacity;
a short terminal buffer retains both transactions for retry. Gallery OpenGL preview
applies the companion position. TRS does not represent shear from arbitrary affine transforms.
TransformAroundPivotは指定pivotの周りに位置を回転・拡大縮小したTRSを返します。
直交正規basisで変位を拘束し、回転はEuler角の加算ではなく既存姿勢との合成で計算します。
gizmoは外部pivotで位置が変わる回転・scaleに位置transactionを併用します。
Begin・Update・終了eventは必要容量を事前確認し、終了buffer不足では両transactionを再試行まで保持します。
GalleryのOpenGL previewは位置側の提案も反映します。
TRSは任意のアフィン変換によるshearを表現しません。

`ViewportState::selectedObjects` accepts the complete host selection, including
objects outside the viewport. `companions` supplies host-owned `TransformCompanion`
storage for every additional object. Storage remains stable until the gesture ends.
All selected objects use the same constrained delta and shared pivot, or individual
origins in Individual mode. Locked targets reject the batch. Disappearing companion
IDs cancel it. Capacity is checked before each batch; terminal overflow retains the
whole selection for retry. Gallery supplies its selection and previews companions.
selectedObjectsは画面外も含む完全なホスト選択を受け取り、companionsは追加objectごとの
ホスト所有TransformCompanion領域を受け取ります。領域はgesture終了まで維持します。
全対象に共通変位・共通pivotを適用し、Individualでは各originを使います。locked対象があれば
開始を拒否し、補助対象IDの消失では一括取消します。batch容量を事前確認し、終了buffer不足時は
全対象を保持して再試行します。Galleryは選択と補助previewを接続しています。

`TransformGizmo` draws translation axes, XY/YZ/ZX plane handles and a screen handle;
Scale adds square axis tips, independent plane scaling and uniform screen scaling.
Rotate uses projected axis rings and an outer view-normal ring. Unified displays
translation tips, scale squares and rotation rings together; the chosen operation
remains fixed through the transaction. Plane movement solves the two projected
basis directions independently, and ring movement uses angular displacement.
Shift applies fine control and snap quantizes the resulting delta. Source transforms
remain host-owned; preview and commit use typed Translate/Rotate/Scale events.
Arbitrary oriented nonuniform scale still needs its TRS/shear behavior completed.
TransformGizmoは移動軸、XY/YZ/ZX平面、screen handleを描画します。Scaleは四角い軸端、
平面内の独立した拡大縮小、screenでの一様拡大縮小を提供します。Rotateは投影した軸ringと
画面法線方向の外周ringを使います。Unifiedは移動・scale・回転handleを同時表示し、
選んだ操作をtransaction終了まで維持します。平面操作は投影した2基底から個別に変位を求め、
ringは角度差で回転します。Shiftはfine、snapは変位の量子化へ反映します。
元transformはホスト所有のまま、Translate/Rotate/Scaleのpreview・commit eventを返します。
任意orientationの非一様scaleについてはTRS/shearの扱いが未完了です。

The focused CG public-IO test covers all three translation/scale planes, screen
translation/scaling/rotation, axis rotation rings and distinct Unified operations.
The native Gallery GPU verifier also passes the existing X-gizmo preview/commit
path after these changes. A 150% native capture checks translation-handle visibility;
it does not establish native OS/IME input coverage or visual coverage of every tool.
CGの公開IOテストで移動・scaleの3平面、screen移動・scale・回転、軸回転ring、
Unifiedの操作種別を確認しています。GalleryのGPU検証でも既存のX軸preview/commit経路が成功しました。
150%のnative captureでは移動handleの表示を確認しています。native OS/IME入力や全toolの外観確認を
実施した根拠には含めません。

`NavigateCamera` applies screen-pixel orbit/pan and wheel zoom. Pan uses the view
basis and projected world units per pixel; orthographic zoom updates visible height,
perspective zoom updates distance. `AlignCamera` provides all six axis views.
The viewport navigation gizmo calls this operation and consumes clicks within its
overlay. Optional `ViewportState::cameraView` points to a host-owned camera, copied
when Camera view is selected; free navigation leaves it unchanged. Gallery shading
selects Wireframe/Solid on both DrawList and OpenGL meshes. Material/Rendered modes
remain outside this preview implementation and are not exposed as selectable modes.
NavigateCameraはpixel単位のorbit/panとwheel zoomを処理します。panはview基底と画面倍率に従い、
orthographic zoomは表示高、perspective zoomは距離を変更します。AlignCameraは6軸方向を指定できます。
Viewport右上のnavigation gizmoが同じ操作を呼び、overlayへのクリックはscene選択へ流しません。
任意のcameraViewはホスト所有cameraへの非所有参照で、Camera表示時に反映し、自由navigationでは変更しません。
GalleryのWireframe/Solid切替はDrawListとOpenGL双方のmeshへ反映します。Material/RenderedはPreviewの対象外で、
選択可能なモードとして表示しません。

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
box/lasso and multi-key workflows,
transition handles/picker, linked/group sample policy, audio envelope editing,
full pivot rotation and multi-object transforms,
hierarchy rename/reorder and stack inspector, UV edge/face/island
interaction, strip scale/repeat/blend editing, editor-specific icon expansion,
complete localization and all requested representative input checks. Provider search
controls exist, but the Gallery's sample providers do not yet apply every filter.
依頼された1.0 Suiteは**未完成**です。box/lassoと複数key操作の統合、
transition編集/picker、linked/groupのsample処理、audio envelope、
gizmoのpivot回転と複数object変換、階層rename/reorderとstack inspector、
UVのedge/face/island操作、strip scale/repeat/blend、editor icon追加、完全な表示文字列差替え、
全代表操作の検証が残っています。Gallery providerでは全検索条件の適用も未完了です。

Version remains 0.2.0 until 1.0 acceptance is complete. No 1.0 tag or Release is created.
1.0の受入完了まではversionを0.2.0に保持し、1.0 tag/Releaseは作成しません。

Dope Sheet preserves an existing multi-key selection on drag. Provide `CurveProvider::selected`
and `CurveState::companionDrags` for the complete selection, including offscreen keys.
Drag moves time only; Alt-drag duplicates, and the context menu enables timing scale
around the earliest selected key or frame snap. Values remain unchanged. The preview
uses proposed ticks without mutating host data. Commit and Cancel require room for the
whole batch; insufficient event capacity retains all transactions for retry.

Dope Sheetは既存の複数キー選択をドラッグ時に維持します。画面外を含む選択全体を
`CurveProvider::selected`で返し、`CurveState::companionDrags`を確保してください。
ドラッグは時間だけを移動し、Altドラッグは複製します。右クリックメニューで、
最初の選択キーを基準とした時間scaleとフレームsnapを設定できます。値は維持します。
ホストデータを変更せず提案tickをプレビューし、Commit／Cancelは全対象を一括で返します。
イベント容量不足時は全transactionを保持して再試行します。
