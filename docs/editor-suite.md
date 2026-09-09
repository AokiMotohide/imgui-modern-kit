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

`ComponentStack` consumes non-owning `ComponentView` rows with component/owner IDs,
host UTF-8 labels/descriptions and enabled/expanded/locked state. It returns Toggle
fields 0/1/2, owner-scoped Reorder offsets -1/+1, and Remove actions. The host owns
component data, execution order and semantics. Gallery connects Mesh renderer and
Wireframe override to both previews, preserves stack order, and copies components
with fresh IDs on object duplication. Model tests verify enabled rendering, ordered
overrides and renderer removal; the API fixture exercises the public widget.
ComponentStackはcomponent/owner ID、ホストのUTF-8名・説明、enabled/expanded/locked状態を持つ
非所有ComponentViewを受け取ります。Toggleのfield 0/1/2、owner付きReorderのoffset -1/+1、
Remove操作を返します。componentの保存・実行順・意味はホストが所有します。Galleryは
Mesh rendererとWireframe overrideを両previewへ接続し、stack順を保持します。object複製では
新しいIDでcomponentもコピーします。model検証で有効化・override順・renderer無効化の結果を確認し、
API fixtureで公開widgetを使用しています。

`ComponentStackOptions` supplies the owner, available `ComponentTypeView` IDs/labels
and owner lock state. Add component emits `ComponentAdd` targeting the owner, with
the selected type ID in proposed.parent. Gallery creates a fresh component ID for
Mesh renderer or Wireframe override. Owner lock prevents addition and mutation;
expansion remains a view operation. Public IO verifies type selection, while host
model checks verify construction and locked-owner rejection.
ComponentStackOptionsはowner、追加可能なComponentTypeViewのID・表示名、owner lockを渡します。
追加はownerを対象とし、proposed.parentに選択type IDを持つComponentAddを返します。Galleryは
Mesh rendererまたはWireframe overrideへ新しいcomponent IDを割り当てます。owner lockでは追加・変更を
拒否し、展開だけは表示操作として許可します。公開IOでtype選択、host modelで生成とlock拒否を確認しています。

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

Viewport Select mode starts box selection on empty canvas; Lasso selection switches
to the shared polygon-selection path. `selectionPoints` and
`selectionCanvas.selectionPath` are host scratch storage. Candidates are projected
origins from the supplied visible-object span, excluding hidden/unselectable/locked
objects. Scratch overflow preserves selection and reports overflow; revision changes
cancel the selection gesture. Gallery connects the result to its common object
selection. Focused public IO tests exercise both shapes and exclude an outside origin.
ViewportのSelect modeでは空白dragでbox選択し、Lasso selectionで共通polygon選択へ切り替えます。
selectionPointsとselectionCanvas.selectionPathはホストのscratch領域です。渡された可視object spanの
originを投影し、hidden・unselectable・locked対象を除外します。scratch不足は選択を維持して通知し、
revision変更は選択gestureを取り消します。Galleryは共通object selectionへ接続しています。
公開IOテストで両形状による選択と範囲外originの除外を確認しています。

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

### Viewport theme colors / ViewportのTheme色

`BeginViewport` uses `Theme::editor.canvas`, `grid`, and `axisX/Y/Z` for its background, grid, scene axes, and navigation gizmo. `TransformGizmo` uses the same axis tokens and `editor.gizmo` for screen-space handles. Plane fills preserve the token alpha, multiplied by 65/255; active handles also change outline thickness. Hosts can customize these tokens without changing global ImGui colors.

`BeginViewport`の背景・grid・scene軸・navigation gizmoは`Theme::editor.canvas`、`grid`、`axisX/Y/Z`を使用します。`TransformGizmo`も同じ軸tokenを使い、screen-space handleには`editor.gizmo`を使用します。平面の塗りはtokenのalphaに65/255を乗じ、active handleは輪郭の太さでも区別します。ホストはImGuiのグローバル色を変えずにこれらのtokenを指定できます。

### Continuous ring rotation / 回転リングの連続操作

Axis and screen rotation rings accumulate the signed angular difference between consecutive mouse samples. Crossing ±180 degrees or completing multiple turns no longer resets the gesture angle; snap and fine adjustment use this accumulated angle. `ViewportState::rotationMouse` and `rotationAngle` are gesture state owned by the host. Rotation events still contain the resulting Euler orientation, not a revolution count. As with sampled pointer input, an angular jump greater than 180 degrees between samples is interpreted as the shorter arc.

軸・screen回転リングは、隣接するマウス入力間の符号付き角度差を積算します。±180度の通過や複数周回で操作角度をリセットせず、snap・fineにも積算角度を使用します。`ViewportState::rotationMouse`と`rotationAngle`はホスト所有の操作状態です。回転イベントは周回数ではなく結果のEuler姿勢を返します。入力サンプル間で180度を超えて移動した場合は短い側の弧として解釈します。公開ImGui IOテストでscreenリングの810度までの連続操作を確認しています。

Gizmo gestures cancel the complete transform batch when a participating object becomes hidden, or when the pivot cannot be projected (behind the near plane or in a zero-size viewport). An insufficient event buffer retains the pending Cancel batch for retry. Ordinary movement outside the viewport rectangle remains projectable and does not cancel the gesture.

ギズモ操作は、参加対象の非表示化、またはpivotの投影不能（near planeより後方、Viewport寸法ゼロ）で変換バッチ全体をCancelします。event buffer不足時はCancel一式を保持して再送します。Viewport矩形の外への通常移動は投影可能なため取消しません。

Curve and UV context menus use `BoxSelect` and `LassoSelect` when the host supplies `CurveState::icons` or `UVState::icons`. The active mode has an explicit text indicator and selected background. Mode buttons are disabled during a drag or area selection. The atlas is non-owning; null retains the text checkbox. Gallery supplies its atlas to both editors.

Curve／UVのcontext menuは、ホストが`CurveState::icons`／`UVState::icons`を渡した場合に`BoxSelect`／`LassoSelect`を表示します。activeモードを文字と選択背景で示し、drag・領域選択中はモードボタンを無効にします。atlasは非所有で、nullでは従来の文字checkboxを使用します。Galleryは両部品へatlasを渡します。

The external Debug source consumer now compiles and runs the Component Stack type/owner options, Curve/UV host atlas fields, UV face query, and continuous gizmo state through the public editor targets. This is source-consumption evidence; it does not establish installed-package, Release, GPU, or native menu interaction coverage.

外部Debug source consumerで、Component Stackのtype／owner options、Curve／UVのホストatlas、UV face query、連続回転ギズモの状態を公開editor target経由でコンパイル・実行しました。これはsource導入の検証であり、install済みpackage、Release、GPU、native menu操作の検証ではありません。

### Timeline transition duration / Timelineのtransition長

Clip top-edge square handles edit the in/out transition durations independently, including creation from zero. `EditKind::TransitionDuration` carries in duration in `Value::first` and out duration in `last`, with Begin/Update/Commit/Cancel phases. The sum is constrained to the clip duration. Locked clips/tracks reject editing; revision changes, Escape, or removal from the visible query cancel. Preview uses the transaction proposal and Gallery applies committed durations. Public ImGui IO tests cover both handles and their typed commits. Transition type selection, adjacent-clip overlap semantics, and native capture remain separate unfinished work.

クリップ上端の四角いhandleで、開始側・終了側のtransition長を独立して編集し、ゼロからも追加できます。`EditKind::TransitionDuration`は`Value::first`に開始側の長さ、`last`に終了側の長さを格納し、Begin／Update／Commit／Cancelを返します。合計はclip duration以内に制限します。locked clip／trackは編集を拒否し、revision変更・Escape・可視queryからの消失で取り消します。previewは提案値を使用し、GalleryはCommitを適用します。両handleと型付きCommitを公開ImGui IOで確認しました。transition種類の選択、隣接clipのoverlap契約、native captureは引き続き未完了です。

`video::EditTransition(clip, end, durationDelta)` is the pure duration calculation used by Timeline. It preserves the opposite end, clamps the edited end inside the clip, and rejects locked clips or invalid original durations. Arithmetic clamps the delta before addition. Focused tests cover the duration bound and complete Cancel delivery after event-buffer shortage on track locking, revision changes, and Escape.

`video::EditTransition(clip, end, durationDelta)`はTimelineが使用する純粋な長さ計算です。反対側の長さを保持し、編集側をクリップ内に制限します。locked clipと不正な元の長さは拒否し、加算前にdeltaを制限します。長さの上限と、trackのロック・revision変更・Escapeによる取消しがbuffer不足後にも届くことをfocused testで確認しました。

### Transition type picker / transition種類の選択

`video::TransitionPicker` edits independent in/out `TransitionKind` values (None, Dissolve, Fade, Crossfade). Timeline opens it on clip right-click; the host applies `TransitionType` events (`first`/`last` are in/out kind). Each choice emits an atomic Begin/Commit pair or reports buffer overflow without a partial edit. Locked tracks/clips disable the picker. D/F/X badges identify the chosen type. Choosing None retains the saved duration for later re-enabling. These are UI and host-model semantics; media blending remains host-owned.

`video::TransitionPicker`は開始側・終了側の`TransitionKind`（None／Dissolve／Fade／Crossfade）を独立して変更します。Timelineのclip右クリックで開き、ホストは`TransitionType`イベントの`first`／`last`を開始側／終了側の種類として適用します。選択ごとにBegin／Commitの一組を返し、buffer不足時は部分送信せずoverflowを通知します。locked track／clipでは無効です。D／F／Xのbadgeで種類を示し、Noneへ変更しても再有効化用に長さを保持します。これはUIとホストデータの契約であり、メディア合成はホストの責任です。

Transition picker public-IO tests now cover choosing Fade on the in side and Crossfade on the out side, preservation of the opposite type, atomic rejection with a one-event buffer, and locked-clip input suppression. Native menu appearance and host playback are not covered by these tests.

transition pickerの公開IOテストで、開始側のFade・終了側のCrossfade選択、反対側の種類の保持、1イベントbufferでの部分送信拒否、locked clipへの入力抑止を確認しました。native menuの表示とホスト再生はこのテストの検証範囲に含みません。

Gallery provides a host-owned transition history example: right-click the query/commit status text for Undo transition or Redo transition. It records only the changed clip ID and transition values, clears the redo branch after a new edit, and clears history on dataset replacement. Replay rejects removed/locked targets, diverged values, or durations invalidated by another edit, and advances the host revision. Headless host-model verification covers duration apply/Undo/Redo and type apply/Undo. This history currently covers transition edits only; general editor Undo integration remains unfinished.

Galleryにはホスト所有のtransition履歴例を追加しました。query／commit状態テキストを右クリックするとUndo transition／Redo transitionを選べます。変更clipのIDとtransition値だけを記録し、新規編集でRedo側を破棄、dataset切替で履歴を消去します。対象消失・ロック・値の不一致・別編集により長さが不正になる復元を拒否し、復元時にホストrevisionを進めます。headless host model検証で長さの適用／Undo／Redoと種類の適用／Undoを確認しました。現時点の履歴対象はtransition編集のみで、Editor全体のUndo統合は未完了です。

Caption double-click now starts a host-visible Rename transaction and focuses the native InputText widget. Updates carry both original and proposed UTF-8 text; Enter commits, Escape restores the original text in Cancel. Revision changes, locked targets, and removal from the visible query terminate the transaction, with terminal events retained on buffer shortage. Oversized original labels are rejected instead of silently truncated. Public IO tests verify UTF-8 Enter and Escape; native OS/IME composition has not been tested.

captionのdouble-clickはホストへRenameのBeginを返し、標準InputTextへfocusを移します。Updateは元のUTF-8文字列と提案文字列を含み、Enterで確定、Escapeでは元の文字列をCancelへ戻します。revision変更・対象ロック・可視queryからの消失で終了し、buffer不足時は終了イベントを保持します。長すぎる元labelは黙って切り詰めず拒否します。公開IOでUTF-8のEnter／Escapeを確認しました。native OS／IME変換は未検証です。

`Transaction::Cancel` restores both `proposed` and `proposedText` from their original values before attempting delivery. Revision-driven cancellation and retries after buffer shortage therefore have the same text contract as explicit Escape. Core, caption, and Outliner focused regressions pass.

`Transaction::Cancel`は送信前に`proposed`と`proposedText`を元の値へ戻します。revision変更による取消しとbuffer不足後の再送も、明示的なEscapeと同じ文字列契約になります。Core・caption・Outlinerの直接回帰テストが合格しています。

Transition handles and hit regions follow font scale. Clips with nonzero transitions reserve a separate label band for type badges; audio waveform uses the space below the clip label. Gallery includes a nonzero Dissolve/Fade example. Native backbuffer captures are in `out/transition-native-fit/` (local evidence, not distributed); 100% light and 150% dark were inspected while correcting handle size and label/waveform collisions. This does not verify the picker popup or native OS input.

transition handleとhit領域をfont scaleへ追従させました。長さゼロ以外のtransitionがあるclipでは種類badgeとclip名の帯を分け、audio waveformはclip名より下へ収めます。GalleryにDissolve／Fadeの非ゼロ長例を追加しました。native backbuffer captureは`out/transition-native-fit/`（ローカル検証用、非配布）にあり、100% lightと150% darkを見ながらhandle寸法とlabel／waveformの重なりを修正しました。picker popupとnative OS入力の検証ではありません。

Clip key diamonds support time dragging with `Keyframe` transactions: target is the key ID, `first` is clip-local time, `parent` is the clip ID, and `x` preserves the value. Time is constrained to the clip and optionally snapped to frames. `TimelineState::keySelection` is an optional non-owning key selection separate from clip selection; Gallery connects its shared key selection and applies commits to the key model. Locked targets, revision changes, Escape, and disappearance cancel. Public IO verifies begin, preview without host mutation, and typed commit for one key. Multi-key clip editing and native interaction coverage remain unfinished.

clip内のdiamondをドラッグして時刻を編集できます。`Keyframe` transactionのtargetはkey ID、`first`はclip内の時刻、`parent`はclip IDで、`x`には元の値を保持します。時刻をclip内に制限し、任意でframeへsnapします。`TimelineState::keySelection`はclip選択とは独立した非所有参照で、Galleryは共通key選択とCommit適用へ接続します。対象ロック・revision変更・Escape・対象消失で取り消します。単一keyのBegin、ホストを変更しないpreview、型付きCommitを公開IOで確認しました。clip内の複数key編集とnative操作確認は未完了です。

Clip-local key dragging now includes selected keys from the same clip. Host-owned `TimelineState::keyCompanions` stores companion transactions. Movement and frame snap use one shared delta constrained by all selected endpoints, preserving spacing. Begin/Update/terminal batches preflight capacity; missing or locked members cancel the batch. Public IO tests verify two-key movement, boundary clamping, and complete Commit retry after a one-event buffer shortage. Each visible key registers an InvisibleButton so dragging cannot also move its ImGui window. Cross-clip key movement is not implemented.

同じclip内の選択keyをまとめてドラッグできます。ホスト所有の`TimelineState::keyCompanions`に関連transactionを保持し、移動とframe snapは全選択keyの端点で制約した共通deltaを使うため間隔を保ちます。Begin／Update／終了batchは容量を事前確認し、対象消失・ロックで全体を取り消します。公開IOで2key移動・端点制約・1イベントbuffer不足後のCommit一括再送を確認しました。可視keyをInvisibleButtonへ登録し、ドラッグがImGuiウィンドウ移動へ伝わる不具合も修正しました。clipをまたぐkey移動は未実装です。

Transition handles also use public InvisibleButton hit regions. Drag tests now assert that the timeline window origin remains fixed. Cursor restoration submits a zero-size Dummy to satisfy ImGui layout bookkeeping. The Video test executable reports MSVC assertions to stderr instead of opening a blocking dialog; the focused test passes.

transition handleも公開InvisibleButtonのhit領域を使用し、ドラッグ中にTimelineウィンドウの原点が動かないことをテストしました。cursor復元後に寸法ゼロのDummyを登録し、ImGuiのlayout契約も満たします。Videoテスト実行ファイルはMSVC assertionを停止ダイアログではなくstderrへ出力する設定とし、focused testが合格しました。

The focused Timeline uses host `Duplicate`/`Delete` bindings for selected keys in the last edited clip. Duplicate offsets the whole selection by one frame, reduced uniformly at the clip boundary; Delete removes selected keys. Locked members reject the whole command, and event capacity is reserved for all Begin/Commit pairs. Text input and active gestures suppress these commands. Gallery rebuilds its clip-key span from the owning channel and time range, so copied keys are not hidden by the old six-key sample limit. Public IO tests cover remapped keys, lock rejection, and capacity rejection; the Gallery host-model verifier passes.

focusを持つTimelineは、最後にkey編集したclip内の選択keyにホストのDuplicate／Delete bindingを適用します。複製は全選択keyを1frameずらし、clip端では共通offsetを縮めます。Deleteは選択keyを削除します。locked対象があれば全体を拒否し、全Begin／Commit対の容量を事前確保します。文字入力中・操作中は実行しません。Galleryは所有channelと時間範囲からclip-key spanを再構築し、旧6key固定の上限で複製結果が隠れないようにしました。キー割当変更・ロック・容量不足を公開IOで確認し、Gallery host model検証も合格しました。

Timeline Hand mode registers a public InvisibleButton across the time canvas, captures its left drag, and scrolls time horizontally and tracks vertically. Clip click-selection and caption editing are suppressed in this mode. Public IO verifies horizontal pan without window movement, clip selection changes, or edit events.

TimelineのHand modeは時間軸領域に公開InvisibleButtonを登録し、左dragを受けて横方向は時間、縦方向はtrackをスクロールします。このmodeではclipのclick選択とcaption編集を抑止します。公開IOで、ウィンドウ移動・clip選択変更・編集イベントを起こさず横panできることを確認しました。

Clip bodies register a clipped public InvisibleButton after their key and transition controls. Move/trim gestures consequently own their mouse input without moving the parent window. Insufficient clip-selection scratch reports overflow and prevents an edit from starting. The direct public-IO regression covers both rejection and a normal stationary-window move/Commit.

clip本体はkey・transition controlの後に、可視範囲で切り取った公開InvisibleButtonを登録します。移動・trimはマウス入力を保持し、親ウィンドウへ移動を伝えません。clip選択scratch不足時はoverflowを通知して編集開始を防ぎます。公開IOの直接回帰で容量不足による拒否と、親ウィンドウを動かさない移動／Commitを確認しました。

Core canvas/grid/ruler/marker and Timeline track-header, video/audio/caption fill, snap guide, missing/offline/proxy/locked indicators now consume their `Theme::editor` semantic colors. Clip fill multiplies the host alpha by the selection-state opacity instead of replacing it. Core and Video direct regressions pass; this change has no new native capture.

Coreのcanvas／grid／ruler／markerと、Timelineのtrack header・video／audio／caption塗り・snap guide・missing／offline／proxy／locked表示に`Theme::editor`の意味色を適用しました。clip塗りはホストのalphaを置換せず、選択状態の不透明度を乗じます。Core／Videoの直接回帰テストが合格しています。今回の変更後のnative captureは未実施です。

### Clip volume envelope / clipのvolume envelope

`ClipView::envelope` borrows time-sorted `EnvelopePoint` values with stable IDs, clip-local ticks, gain, and lock state. Timeline draws connected gain points in the waveform band; dragging changes time within adjacent points and gain within 0–2. `AudioEnvelope` transactions carry point ID, clip ID in `parent`, tick in `first`, and gain in `x`. Revision changes, Escape, locks, and disappearance cancel; terminal delivery is retained on shortage. `EvaluateEnvelope` provides linear gain interpolation with constant endpoint extension and unity for an empty envelope. Gallery owns a three-point example and applies Commit values. Pure interpolation and public-IO drag/Commit tests pass. Right-click an audio clip to insert a point at the cursor time with interpolated gain; right-click a point to remove it. `offset` identifies edit (0), insert (1, target=clip), or remove (2, target=point). Gallery owns per-clip point vectors and refreshes borrowed spans after mutation. Public-IO insertion/removal tests pass. Playback gain application and native envelope capture remain unverified or unfinished.

`ClipView::envelope`は、StableId・clip内tick・gain・lockを持つ時刻順の`EnvelopePoint`を非所有spanで受けます。Timelineはwaveform帯へ点と線を描き、dragで隣接点間の時刻と0～2のgainを編集します。`AudioEnvelope`はpoint IDをtarget、clip IDを`parent`、tickを`first`、gainを`x`へ格納します。revision変更・Escape・ロック・対象消失で取り消し、容量不足時は終了イベントを保持します。`EvaluateEnvelope`は線形補間、範囲外の端点値、空envelopeでgain 1を返します。Galleryは3点の例を所有しCommitを適用します。純粋補間と公開IOのdrag／Commitテストが合格しました。audio clipの右クリックでカーソル時刻に補間gainの点を追加し、点の右クリックで削除できます。`offset`は編集0、追加1（target=clip）、削除2（target=point）を表します。Galleryはclipごとの点vectorを所有し、変更後に非所有spanを更新します。追加・削除の公開IOテストも合格しました。再生gain適用とnative envelope captureは未検証または未完了です。
