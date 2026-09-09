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


### Clip key insertion and navigation / clip keyの追加と前後移動

Timeline accepts host bindings for `AddKey`, `PreviousKey`, and `NextKey` in its focused canvas. A sole selected clip takes priority; otherwise the last edited key owner is used. `ClipView::keyChannel` explicitly names the insertion channel, including an empty channel; `keyDefaultValue` is used when no keys exist. Insertion uses clip-local playhead time and the existing curve evaluation, rejects duplicate times and locked clips/tracks, and emits an atomic `KeyInsert` Begin/Commit pair (target=channel, parent=clip). Previous/next navigation emits `Navigate` with the destination timeline tick in `first` and clip ID in `parent`. Gallery applies insertion and navigation to host data. Remapped-key IO tests cover navigation, insertion, duplicate time, locked track, and buffer shortage. Dedicated icon controls remain pending.

Timelineのcanvasにfocusがあるとき、ホストbindingの`AddKey`・`PreviousKey`・`NextKey`を処理します。単独選択clipを優先し、それ以外は最後にkeyを編集したclipを使用します。`ClipView::keyChannel`は空channelを含む追加先の明示ID、`keyDefaultValue`はkeyがない場合の値です。追加にはplayheadのclip内時刻と既存curveの評価値を使用し、同時刻の重複とclip／trackのlockを拒否します。`KeyInsert`のBegin／Commitを一括で返し、targetはchannel、parentはclipです。前後移動は`Navigate`の`first`へ移動先timeline tick、`parent`へclip IDを格納します。Galleryは追加と移動をホストデータへ適用します。再割り当てキーによる移動・追加・同時刻重複・locked track・容量不足のIOテストを追加しました。専用アイコン操作は未接続です。


Timeline clips show duration timecode and speed when the title row has enough width. The title and metadata use separate clipped bounds; thumbnails start below the title. Proxy and edge handles retain their own margin. Hovering a clip exposes its full title, duration, speed, linked/group IDs, and media/lock status even when the clip is too short for inline metadata. Native 150% dark inspection is recorded in `out/clip-metadata-fit/page-8-dark-150.png` (local evidence).

Timelineのclipは名前行に幅がある場合、durationのtimecodeとspeedを表示します。名前と情報欄を別々の領域でclipし、thumbnailは名前の下から描画します。proxyと端のhandleの余白を確保しています。短いclipでもhover時に名前全文・duration・speed・linked／group ID・media／lock状態を確認できます。150% darkのnative確認結果は`out/clip-metadata-fit/page-8-dark-150.png`にあります（ローカル検証物）。


Gallery preserves each clip's explicit `keyChannel` when rebuilding key indices, including empty channels, and refreshes borrowed key spans for every referencing clip after host storage changes. The initial first-six-key override has been removed: each span contains only that channel's keys in the clip-local duration. Host-model tests cover channel exhaustion, reinsertion, and multiple clips referencing one channel.

Galleryはkey索引の再構築で、空channelを含む各clipの明示`keyChannel`を維持します。ホストの格納領域変更後は、参照する全clipの非所有key spanを更新します。初期表示を先頭6 keyで上書きする処理を除き、そのchannelのclip内durationに含まれるkeyを渡します。channel内全削除後のID維持・再追加・複数clipからの参照をホストモデル検証で確認しました。


Gallery's six-track cycle now contains Video, Audio, Caption, Effect, Adjustment, and Group rather than repeating the first three roles. Effect/adjustment/group clip ranges use the same public move/trim and track controls. `EditorPalette::effectClip`, `adjustmentClip`, and `groupClip` are distinct semantic fill tokens used by Timeline; original palette fields remain in their existing order. These track kinds do not add an effect processor or nested sequence playback engine.

Galleryの6 track構成をVideo・Audio・Caption・Effect・Adjustment・Groupへ揃えました。effect／adjustment／groupのclip範囲は共通の公開move／trimとtrack操作を使用します。Timelineの塗りに`EditorPalette::effectClip`・`adjustmentClip`・`groupClip`を接続し、既存paletteフィールドの順序を維持しました。track種類の追加によってeffect処理や入れ子sequenceの再生エンジンを提供するものではありません。


Monitor title and timecode now use opaque theme-surface plates with theme text color, preserving readability over host textures. Padding scales with font size; text is clipped to the plate, and a short monitor omits the lower timecode rather than overlapping its title. Native light 100% and dark 150% appearance was inspected in `out/monitor-label-contrast/`.

Monitorの見出しとtimecodeは、不透明なtheme surface背景とtheme text色でhost texture上の可読性を確保します。font sizeに合わせて余白を調整し、文字を背景範囲内へclipします。高さが足りない場合は下部timecodeを省き、見出しとの重複を防ぎます。`out/monitor-label-contrast/`でnative light 100%とdark 150%の表示を確認しました。


`MonitorOptions` accepts borrowed UTF-8 `clipName`, `markerComment`, and `metadata` lines. `MonitorMetadataPreset::Off` hides those fields, `Clip` shows name/comment, and `Details` adds metadata lines; title and timecode retain their independent options. Lines stop before the timecode area and are clipped horizontally without allocating text storage. Gallery's Program Monitor context menu changes its host-owned preset and supplies selected-clip duration/speed/media status. Selection/revision changes refresh the cached clip lookup; steady frames do not scan all clips for metadata. A marker label is shown on its matching timecode frame. API compilation covers all fields; native 150% dark capture verifies selected clip text in `out/monitor-clip-metadata/`. The focused `--verify-monitors` runner verifies all three context-menu presets through public IO and captures Japanese marker comments with Details metadata. Popup navigation leaves Timeline scale/origin unchanged. Evidence: `out/monitor-preset-focus-fix/`.

`MonitorOptions`はUTF-8の`clipName`・`markerComment`・`metadata`行を非所有参照で受けます。`MonitorMetadataPreset::Off`はこれらを非表示、`Clip`は名前とcomment、`Details`は追加metadata行まで表示します。見出しとtimecodeは独立した既存設定を維持します。行はtimecode領域の前で打ち切り、文字領域を確保せず水平方向にclipします。GalleryのProgram Monitorの右クリックでホスト所有presetを切り替え、選択clipのduration／speed／media状態を渡します。selectionまたはrevision変更時だけclip検索を更新し、定常描画ではmetadataのために全clipを走査しません。markerと同じtimecode frameではそのlabelをcommentとして表示します。全フィールドのAPI compileと`out/monitor-clip-metadata/`の150% darkで選択clip名の表示を確認しました。専用`--verify-monitors`で3種類のcontext menu presetを公開IOで切り替え、Details metadataと日本語marker commentをnative captureで確認しました。popup操作中にTimelineのscale／originが変化しないことも確認しています。検証物は`out/monitor-preset-focus-fix/`にあります。


`CommandPressed` suppresses editor binding dispatch while any popup is open, preserving menu keyboard navigation. A regression test confirms that Home in the Monitor menu does not also invoke Timeline Fit. Video/CG direct regression tests pass after this focus change.

`CommandPressed`はpopupが開いている間、Editor bindingを発火させずmenuのキーボード操作を優先します。Monitor menuのHomeキーがTimeline Fitにも伝わらないことを回帰確認し、Video／CGの直接回帰テストも合格しました。


`MonitorOptions::transformBounds` replaces the fixed transform rectangle with host-supplied normalized display coordinates. `anchor` uses the same display space; `flipY` affects only texture UVs. Non-finite or inverted bounds are omitted, and the monitor clips bounds that extend outside its display. Gallery's Scale/Position X Inspector values drive the rectangle and anchor as a host overlay example; this does not transform decoded media. `--verify-monitors` applies those property Commit values and captures the resulting rectangle in `out/monitor-host-bounds/monitor-transform-host.png`. API compilation and native 150% rendering were verified.

`MonitorOptions::transformBounds`で固定枠をホスト指定の表示正規化座標へ置き換えました。`anchor`も同じ表示座標で、`flipY`はtexture UVだけへ作用します。非有限値や逆転した矩形は描画せず、表示外へ広がる枠はMonitor内へclipします。GalleryではScale／Position XのInspector値から枠とanchorを算出するホストoverlay例を接続しました。decode済みmediaの変形処理ではありません。`--verify-monitors`でproperty Commitを適用し、`out/monitor-host-bounds/monitor-transform-host.png`へ結果を保存しました。API compileと150% native描画を確認しています。

Gallery Clip Inspector now resolves the active clip to separately allocated property IDs and host-owned values. Switching clips preserves each clip's Opacity, Scale and Position X; Speed writes the selected ClipView speed. Deleted, stale-selection and locked clip/track owners reject property edits. The Inspector displays a selection prompt when no clip is active. `--verify-inspector-model` covers independent IDs, restored values, stale owner rejection and locked tracks; `--verify-monitors` checks the resulting host overlay in native GL.

GalleryのClip Inspectorは選択clipごとに個別割当のproperty IDとホスト所有値を使用します。選択を切り替えてもOpacity／Scale／Position Xを保持し、Speedは選択ClipViewへ反映します。削除済み、選択先が変わった、clipまたはtrackがlockedの編集は拒否します。clip未選択時は選択案内を表示します。`--verify-inspector-model`でIDの独立、値の復元、古い選択先とlocked trackの拒否を、`--verify-monitors`でnative GL上のホストoverlay連携を確認しました。

Gallery ordinary clip duplication now copies Inspector values/flags, property animation keys, the inline key channel and audio envelope into independent host storage with newly allocated IDs. Media thumbnail/waveform resources remain borrowed. Locked clip or track owners reject duplication. The focused host-model verifier checks copied values and key IDs and confirms that editing a duplicated envelope leaves its source unchanged. Split remapping is a separate operation and is not covered by this duplication check.

Galleryの通常clip複製では、Inspector値・flags、property animation key、clip内key channel、audio envelopeを新しいIDと独立したホスト領域へコピーします。mediaのthumbnail／waveform資源は非所有参照を維持します。clipまたはtrackがlockedの場合は複製を拒否します。モデル検証で値とkey IDのコピー、および複製先envelopeの編集が元clipへ影響しないことを確認しました。split時の再配置は別操作であり、この複製検証には含みません。

Gallery split partitions the host audio envelope at the cut. It interpolates a boundary point when needed, preserves left-side point IDs, and assigns new IDs and clip-local ticks to the right side. Both halves retain the original linear gain evaluation. Locked tracks reject the host split. The model verifier checks the boundary, shifted points, independence and gain on each side.

Galleryのsplitは音量envelopeを分割位置で分け、必要な境界点を補間します。左側の既存point IDを保持し、右側には新しいIDとclipローカル時刻を割り当てるため、両側で分割前の線形gain評価を維持します。locked trackのsplitはホストでも拒否します。モデル検証で境界、時刻移動、IDの独立、両側のgainを確認しました。

Ctrl-clicking a selected Timeline clip now only removes it from selection; it does not begin an edit or emit Update/Commit after subsequent mouse movement. The public ImGui IO video regression covers this path.

Timelineで選択済みclipをCtrlクリックすると選択解除だけを行い、編集を開始しません。その後マウスを移動してもUpdate／Commitを発生させないことを公開ImGui IOのVideo回帰で確認しました。

Razor emits a complete Split Begin/Commit pair with the original clip value, proposed cut, owner ID, starting revision and modifiers. Two event slots are reserved before publishing either event; shortage reports overflow with no partial transaction. Public ImGui IO regression verifies both paths.

Razorは元clip値、分割提案、対象ID、開始revision、modifierを含むSplitのBegin／Commitを返します。出力前に2イベント分の容量を確認し、不足時は部分transactionを出さずoverflowを通知します。通常時と容量不足時を公開ImGui IOの回帰で確認しました。

Track header toggles, including collapse and the narrow-pane control menu, emit a capacity-checked Begin/Commit pair with original and proposed boolean values. Insufficient capacity emits neither event and reports overflow. Public IO regression covers collapse, Source patch, narrow controls and shortage.

Track headerのtoggleは、折り畳みと狭いpaneのmenuを含め、元値と提案値を持つBegin／Commitの組を返します。容量不足時はどちらも出力せずoverflowを通知します。折り畳み、Source patch、狭いpaneの操作と容量不足を公開IO回帰で確認しました。

Gallery Japanese mode supplies translated track controls via `TrackLabels`. Wider translated labels automatically use the compact menu when the header is too narrow. API compilation and public IO menu operation with a host UTF-8 label pass.

Galleryの日本語modeは`TrackLabels`を通じてtrack操作を翻訳します。翻訳後のラベルがheader幅に収まらない場合は省略menuへ切り替わります。API compileとホストUTF-8ラベルを使用した公開IOのmenu操作を確認しました。

Timeline track controls reuse Eye/EyeOff, Volume/Mute, Unlock/Lock and Record from the host icon atlas. The compact menu includes the same glyphs with labels and checked state. Active inline controls retain a background and underline. Without an atlas, host text buttons remain available. Solo, Target and Source still use host labels. Build and text-route public IO regression pass; native inspection of this icon route remains pending.

Timelineのtrack操作へホストatlasのEye／EyeOff、Volume／Mute、Unlock／Lock、Recordを接続しました。省略menuも同じglyph、操作名、check状態を表示します。activeのinline操作は背景と下線を維持し、atlas未指定時はホスト文字ボタンを使用します。Solo／Target／Sourceは引き続きホストラベルを使用します。ビルドと文字ボタン経路の公開IO回帰は通過し、このicon経路のnative確認は未実施です。

`--verify-track-controls` now exercises the native icon route: visibility off/on and Japanese compact-menu mute reach host track state. Light inline controls and dark 150% Japanese menu captures are in `out/track-controls-native/`. Text fallback buttons beside icons use the same height when an atlas is present. This is public ImGui IO, not native OS/IME verification.

`--verify-track-controls`でnative icon経路の表示off／onと日本語省略menuのmuteがホストtrackへ反映されることを確認しました。lightのinline操作とdark・150%の日本語menuを`out/track-controls-native/`へcaptureしました。atlas使用時は文字ボタンもiconと同じ高さに揃えています。これは公開ImGui IOでの確認で、native OS／IME確認ではありません。

Gallery validates clip/track ownership and locks before applying clip edits, including before writing rename storage. Its model regression verifies that locked-track Move, both trims, Ripple, Roll, Slip and Slide leave clip data and host revision unchanged.

Galleryはclip編集の適用前にclip／trackの存在とlockを確認し、rename保存領域への書込み前にも拒否します。locked trackのMove、両端trim、Ripple、Roll、Slip、Slideがclip値とホストrevisionを変えないことをモデル回帰で確認しました。

Before applying a host Ripple edit, Gallery rejects it if any following clip on that track that would move is locked. The original trim and following shifts remain unchanged on rejection. The model verifier covers rejection and the corresponding unlocked edit.

GalleryはRipple適用前に、移動対象となる同一trackの後続clipにlockがあれば編集を拒否します。拒否時は元clipのtrimも後続clipの位置も変えません。拒否経路とlock解除後の一括適用をモデル回帰で確認しました。

Gallery uses the Timeline host preflight to prevent Ripple Begin when a following clip on the track is locked. Public IO tests cover rejection without events/overflow and the optional-callback path.

GalleryはTimelineのホスト判定を使用し、同一trackの後続clipがlockedならRippleのBeginを抑止します。イベントもoverflowも発生しない拒否経路と、任意callback未指定時の開始を公開IOで確認しました。

Gallery split copies Inspector values and property IDs to the right clip and assigns an independent inline key channel. Right-channel key ticks are shifted by the cut offset; values, interpolation and relative handle offsets are copied. The host retains off-clip keys in its channel storage. The model regression covers an interior key and Inspector independence. Boundary evaluation from the clipped inline span is not established by this check.

Galleryのsplitは右clipへInspector値をコピーし、property IDとclip内key channelを独立させます。右channelのkey時刻から分割offsetを引き、値・補間mode・相対handleをコピーします。clip範囲外のkeyもホストchannel領域には保持します。モデル回帰で内部keyの時刻移動とInspectorの独立を確認しました。clip範囲に絞ったinline spanの境界評価は、この確認では保証していません。

Gallery supplies full-channel `keyEvaluation` alongside the clipped key span. The split model regression now confirms equal original/right-channel interpolation at the cut and inside the right clip. Public IO verifies that inserting a key uses outside context rather than the visible-only span.

Galleryはclip内key spanと併せて全channelの`keyEvaluation`を渡します。splitモデル回帰で分割点および右clip内部の補間値が元channelと一致することを確認しました。key挿入が可視spanだけでなくclip外contextを使うことも公開IOで確認しました。

Timeline bounds inline-key drawing and hit testing with binary searches of the sorted clip-local span. Active keys are resolved by original tick/ID and drawn separately when their original positions lie outside the visible interval. A 100,000-key public IO fixture verifies bounded emitted geometry; existing multi-key and transaction regressions pass. This is not a new Release frame-time measurement.

Timelineは時刻順のclip内key spanを二分探索し、可視区間の描画とhit testに絞ります。操作中のkeyは開始時刻とIDで解決し、元位置が可視区間外でも別途描画します。10万keyの公開IO fixtureで描画geometryが有界であることを確認し、既存の複数keyとtransaction回帰も通過しました。Releaseのフレーム時間を再測定した結果ではありません。

Timeline envelope rendering now binary-searches the visible interval and retains one neighboring point on each side for connecting segments. Active point validation uses original tick/ID separately from drawing, so culling a point inside a still-visible clip does not end its gesture. A 100,000-point fixture verifies bounded geometry; existing envelope edit/remove/insert regressions pass.

Timelineのenvelope描画は可視区間を二分探索し、接続線用に両側の隣接点を1つずつ残します。操作点は描画とは別に開始時刻とIDで検証し、clipが可視のまま点だけ描画範囲外へ出ても操作を終了しません。10万点のfixtureで描画geometryが有界であることを確認し、既存のenvelope編集・削除・挿入回帰も通過しました。

Track names are fine-clipped to the header width, with a bounded right-click item for the height menu and full text in a tooltip. Long mixed English/Japanese names were captured in narrow dark 150% headers in `out/track-label-clipping/`; existing Track control public IO and Video regressions pass.

Track名はheader幅へfine clipし、高さmenuの右クリック領域も同じ幅へ制限します。全文はtooltipへ表示します。長い英日混在名を狭いdark・150% headerで`out/track-label-clipping/`へcaptureし、既存Track操作の公開IOとVideo回帰も通過しました。

Track height edits cancel when their popup is no longer submitted. A full event buffer retains the pending Cancel and retries it on the next frame with capacity; pending Commit is preserved. Focused transaction tests cover popup closure and Cancel retry.

Track高さの編集popupが表示されなくなった場合はCancelします。event bufferが満杯ならCancelを保持して容量回復後のフレームで再送し、保留中のCommitは維持します。popup終了とCancel再送をtransaction回帰で確認しました。

Gallery Japanese mode now translates Timeline tools/tooltips and snapping/follow/Fit controls through public TimelineLabels. Video regressions, API compilation and the external source consumer pass.

Galleryの日本語modeは公開TimelineLabelsを使い、Timelineのtool名・tooltipとsnap／追従／Fit操作を翻訳します。Video回帰、API compile、外部source consumerを確認しました。

Timeline tool switching now uses host Command bindings, and tooltips include the configured shortcut. Public IO regression verifies all seven remapped tools and that an unbound default key cannot bypass the map. Core, Video and API compile checks pass.

Timelineのtool切替をホストCommand bindingへ接続し、tooltipへ設定済みshortcutを表示します。7種類の再割当と、未割当の既定キーがbinding mapを迂回しないことを公開IOで確認しました。Core／Video／API compileを確認済みです。

Timeline consumes the host-bound Split command at the playhead. The selected callback must resolve every selected clip, including offscreen clips and locked-track state. Missing selected IDs report overflow; invalid or locked eligible targets reject the whole batch. All Begin/Commit slots are reserved before emission. Clips that do not cross the playhead are unchanged. Public IO regression covers remapping, multiple offscreen clips, locks, missing IDs and event shortage.

Timelineはホストが割り当てたSplitコマンドで、再生位置をまたぐ選択clipを分割します。selected callbackは画面外を含む全選択clipとtrackのロック状態を返す必要があります。選択IDの欠落はoverflowを通知し、対象の制約違反・ロック・イベント容量不足では部分分割を出しません。公開IO回帰でキー再割当、画面外の複数clip、ロック、ID欠落、容量不足を確認しています。

Move and Duplicate also require a complete selected-clip query for multiple selections. Missing or duplicate IDs reject the entire Begin batch and report overflow, so truncated host scratch cannot silently edit only part of a selection. Without a selected callback only a single selected clip can start these edits.

移動・複製も複数選択の全clipをselected queryで解決します。IDの欠落・重複ではBeginを一切出さずoverflowを通知し、ホストscratchの切り詰めによる部分編集を防ぎます。selected callbackがない場合、単一選択だけが編集を開始できます。

Gallery clip queries use an end-time segment tree rebuilt after host edits. Track/start bounds and subtree maximum ends prune nonoverlapping clips without a fixed lookback. The borrowed query result uses host scratch reserved during rebuild. A focused 100,000-clip fixture verifies a long overlapping clip, track isolation, duration updates, fewer than 150 visited nodes for a sparse query and unchanged scratch capacity. These are query-contract checks, not a replacement for final Release frame measurements.

Galleryのclip検索はホスト編集後に再構築する終了時刻のsegment treeを使用します。track・開始時刻の範囲と部分木の最大終了時刻で範囲外を除外し、固定秒数の探索制限をなくしました。返却spanは再構築時に確保したホスト領域を借用します。10万clipの回帰で長いclip、track分離、duration変更、疎な検索の訪問node数150未満、作業領域の容量不変を確認しています。これは検索契約の検証であり、最終Releaseフレーム計測は別途必要です。

### Interval-index performance checkpoint / 区間索引の性能確認

Release, 1920×1440, 256 tracks, 100,096 clips and 100,000 keys; 20 warm-up and 180 measured frames per operation. Public ImGui IO drives the native GL window. The wall-time boundary includes host apply, preview render, ImGui, GL submission and swap with vsync off. This checkpoint includes the interval clip index and selected-clip completeness guards; it does not establish completion of the whole suite.

Release・1920×1440・256 track・100,096 clip・100,000 keyで、操作ごとに20 warm-up frame後の180 frameを測定しました。公開ImGui IOでnative GL windowを操作し、ホスト適用・preview描画・ImGui・GL送信・swapを含むwall時間です。vsyncは無効です。区間索引と選択clipの完全性検査を含む時点の測定であり、Suite全体の完成を証明するものではありません。

| Operation / 操作 | P95 ms | Max ms | Terminal frame ms / 確定frame |
|---|---:|---:|---:|
| Pan | 1.8873 | 2.5984 | 1.0424 |
| Zoom | 1.6569 | 2.5332 | 1.7151 |
| Selection | 1.6229 | 2.1567 | 0.9766 |
| Clip drag | 1.6642 | 1.8953 | 8.5416 |
| Clip trim | 1.6272 | 2.2675 | 7.1264 |
| Keyframe drag | 1.5771 | 3.0091 | 9.3691 |

Every operation passed its interaction check. Per-frame maxima were 7 queries, 30 returned clips, 8 returned editing keys and 6 returned track rows. Keys count clip-local editing spans and Curve query neighbors per return; full borrowed evaluation channels are excluded. C++ new and ImGui allocator counts were both zero during the measured interval; driver/OS allocations are excluded. The benchmark now enforces these allocation gates as well as P95 and bounded query results.

全操作で編集結果を確認しました。1 frame当たり最大7 query、返却30 clip・8編集key・6 track行でした。key数はclip内編集spanとCurve queryの隣接keyを返却ごとに数え、評価専用の全channel借用spanは含めません。測定区間のC++ newとImGui allocatorはともに0で、driver・OS allocationは計測対象外です。benchmarkはP95・返却数に加え、このallocation条件も判定します。native OS／IMEの検証ではありません。

Gallery selected-clip resolution treats nonzero linked and group IDs as set IDs and expands their union transitively for Move, Duplicate and command Split. It deduplicates members, propagates missing/locked track ownership into the locked flag, and returns an empty result on missing IDs or scratch exhaustion so the Timeline completeness guard rejects the entire operation. Expansion runs on edit start, not steady drawing. Host-model regression verifies transitive membership, locked tracks and capacity rejection. This does not yet prove linked behavior for all trim tools.

Galleryは非0のlinked・group IDを集合IDとして扱い、移動・複製・コマンド分割の対象を関連先へ推移的に広げます。重複を除き、trackの消失・ロックをlockedへ反映します。ID欠落・作業領域不足は空の結果を返し、Timelineの完全性検査で操作全体を拒否します。展開は編集開始時だけ行います。ホストモデル回帰で関連の推移、locked track、容量不足を確認しました。全trimツールの連動は、この検証の対象ではありません。

Gallery Duplicate assigns new linked/group set IDs per event batch. Copies retain relationships with other copies in that batch while original relationship sets remain unchanged. Host-model regression checks a transitive linked/group selection and verifies that selecting the original set excludes the copies.

Galleryの複製はイベントバッチごとに新しいlinked／group集合IDを割り当てます。同じバッチの複製同士の関係を維持し、元の集合は変更しません。ホストモデル回帰で推移的なlinked／group関係の複製と、元集合の選択に複製が混ざらないことを確認しています。

Gallery Split retains the original incoming transition on the left clip and outgoing transition on the right clip, clamping each duration to its resulting clip. Newly cut edges have no transition. Host-model regression verifies both kinds, durations and cleared edges.

Galleryの分割は元の開始transitionを左clip、終了transitionを右clipへ残し、それぞれの長さを分割後のclip長まで制限します。新しくできた切れ目のtransitionは解除します。ホストモデル回帰で種類・長さ・切れ目の解除を確認しています。
