# Editor API reference / Editor API reference

These are custom APIs, separate from the generated native Dear ImGui overload inventory.
These are the Editor Suite 1.0 APIs. Include only the modules used
by the host. All module targets publish C++20 and retain the host's ImGui target.
以下はDear ImGui標準overloadとは別の開発中APIです。必要なmoduleだけをinclude/linkします。
すべてC++20とホストのImGui targetを継承します。

| Target | Header / namespace | Main API |
|---|---|---|
| `imkit::editor_core` | `imkit/editor_core.h`, `imkit::editor` | `StableId`, `Tick`, `FrameRate`, `FrameToTick`, `TickToFrame`, `FormatTimecode`, `ParseTimecode`, `EventBuffer`, `Transaction`, `Selection`, `ResolveSnap`, `CanvasState`, `BeginCanvas`, `CanvasSelection`, `TimeRuler`, `Transport`, `CurveEditor`, `ResolveHandles`, `MoveHandle`, `Evaluate`, `PropertyGrid`, `AssetBrowser`, `Splitter`, `StatusBar` |
| `imkit::video` | `imkit/video.h`, `imkit::video` | `TimelineProvider`, `TimelineState`, `Timeline`, `EditClip`, `RollClips`, `SlideClip`, `SplitClip`, `EditTransition`, `TransitionPicker`, `EvaluateEnvelope`, `Monitor`, `MonitorOptions`, `MonitorControls`, `MonitorLabels`, `BuildAudioBuckets`, `UpdateMeter`, `Waveform`, `LevelMeter`, `AudioStrip`, `BuildScopes`, `Histogram`, `ScopeImage`, `ColorControls` |
| `imkit::cg` | `imkit/cg.h`, `imkit::cg` | `Project`, `TransformDelta`, `OrientationBasis`, `BeginViewport`, `ViewportObjects`, `TransformGizmo`, `Outliner`, `TransformUV`, `UVEditor`, `DopeSheet`, `AnimationStrips` |
| `imkit::cg` | `imkit/preview.h`, `imkit::preview` | `Vertex`, `Mesh`, `Triangle`, `Cube`, `Sphere`, `DrawListPreview` |
| `imkit::preview_opengl3` | `imkit/preview.h`, `imkit::preview` | `GLFunctions`, `OpenGL3Renderer::Init/Resize/Render/Pick/Shutdown/Texture` |
| `imkit::editor_suite` | Interface aggregate / interface集約 | `editor_core` + `video` + `cg`; excludes `preview_opengl3` |

Dependency graph: editor_core → imkit; video → editor_core; cg → editor_core;
preview_opengl3 → cg. No editor target fetches an editor/plot/gizmo dependency or GL
loader. Source and installed targets use the same exported names.
依存はeditor_core→imkit、video/cg→editor_core、preview_opengl3→cgです。
外部editor/plot/gizmo libraryやGL loaderを取り込みません。install後もtarget名は同じです。

## Provider and event use / Providerとevent

```cpp
std::array<imkit::editor::Event, 128> eventStorage;
imkit::editor::EventBuffer events{eventStorage};
std::array<imkit::editor::StableId, 128> selectionStorage;
imkit::editor::Selection selection{selectionStorage};
imkit::video::TimelineState ui; // Retained by the host between frames.

// During a normal host-owned ImGui frame/window:
imkit::video::Timeline("timeline", provider, ui, selection, events, theme);
for (const auto &event : events.Events()) {
    // Host validates revision, previews drafts, applies Commit and records Undo.
    DispatchToHost(event);
}
events.Clear();
```

`provider`, `theme` and `DispatchToHost` in this snippet are supplied by the host.
`Value.first/last/offset` retain exact time; `parent` retains an exact StableId.
`x/y/z/w` encode the edit's numeric channels. Text rename events retain original and
proposed UTF-8 bytes in 256-byte fixed arrays, including the terminator. Longer labels
require a host rename flow. Selection storage and transaction member storage have
explicit capacities. The host must size event buffers for all phases in one frame.
上記provider/theme/DispatchToHostはホストが用意します。時刻とIDは整数のまま保持します。
renameは元・提案UTF-8を終端込み256 byteの固定配列で返します。それ以上の文字列はホストの
rename処理を使用してください。選択・複数drag・eventの容量は明示的に確保します。

Clip values use `{start, end, sourceIn, trackId, speed}`. Property values use `x`.
Transform values use `x/y/z`. Key events use `first` for tick and `x` for value;
handle events use `x/y` for relative seconds/value. Reparent uses `parent`.
Gallery applies only the subset documented in [editor-suite.md](editor-suite.md).
clipはstart/end/sourceIn/trackId/speed、propertyはx、transformはx/y/z、keyはfirst/x、
handleは相対秒/値のx/y、reparentはparentを使います。Galleryの適用範囲は上記文書で区別します。

## GL pass contract / GL pass契約

Camera API additions / camera追加API:

| API | Contract / 契約 |
|---|---|
| `NavigateCamera(Camera&, Point orbitPixels, Point panPixels, double wheel, double viewportHeight)` | Pure camera navigation, view-relative pan, orthographic extent zoom / 描画倍率に従うcamera操作 |
| `AlignCamera(Camera&, Axis, bool negative = false)` | X/Y/Z positive/negative orthographic views / 6方向の正投影 |
| `ViewportState::cameraView` | Non-owning host camera, valid during BeginViewport / ホストcameraの非所有参照 |
| `ViewportState::navigationGizmo` | Toggle the clickable view-axis overlay / 軸alignment overlayの表示切替 |

Color API inventory / 色操作API一覧:

| Signature / 署名 | Contract / 契約 |
|---|---|
| `ScopeBuffers::redWaveform/greenWaveform/blueWaveform` | Optional three `width * 256` spans; all-or-none / RGB binは3本一括指定 |
| `ScopeImage(..., const Theme&, ImVec4 tint)` | Tinted CPU scope drawing, no texture ownership / 指定色でbin描画 |
| `Rgba ApplyColorCurves(Rgba, const ColorCurveSet&)` | Sorted non-owning RGB key spans; normalized input maps to 0..TicksPerSecond, empty channel is identity, output clamped, alpha preserved / 時刻順の非所有RGBキー、正規化入力、空channelは恒等、出力制限、alpha保持 |
| `bool ColorControls(const char*, ColorValues&)` | Host draft editing; compatible existing signature / 既存draft編集 |
| `void ColorControls(const char*, const ColorValues&, const ColorPropertyIds&, uint64_t, ColorState&, EventBuffer&, const ColorLabels& = {})` | Immutable source and typed RGB/scalar transactions; six independent IDs / 不変sourceと明示IDのtransaction |

Both ColorControls overloads render actual draggable three-way wheels and level
sliders. ColorState records current wheel centers/radius for host overlays and
public-IO automation. Neither overload provides a color management pipeline.
両overloadはthree-way wheelとlevel sliderを描画します。ColorStateの中心座標・半径はホストのoverlay・
公開IO自動操作に使えます。色管理pipelineは含みません。

Gallery Color uses the common CurveEditor with independent host-owned RGB keys,
including interpolation and handles. Commit regenerates the synthetic ramp and
scope bins; Cancel preserves them. This demonstrates curve evaluation on synthetic
pixels, without a media decode or color management engine.
GalleryのColorは共通CurveEditorでホスト所有のRGBキーを補間・handle付きで編集します。
Commit時に合成ramp画像とscope binを再計算し、Cancelでは保持します。
実メディアdecodeや色管理engineではなく、合成画素へのcurve適用例です。

Audio API inventory / 音声操作API一覧:

| API | Contract / 契約 |
|---|---|
| `TrackControl` | Stable Toggle field selector in event x; boolean state in y / track flagの明示selector |
| `AudioStripView::gainId, panId` | Explicit property IDs; zero disabled; no arithmetic ID inference / gain・panの独立ID |
| `AudioStripView::locked` and label pointers | Host restriction and UTF-8 labels / 制限と表示文字列 |
| `AudioStrip(...)` | Native gain fader/pan transactions; mute/solo/record state events / 音声操作event |
| `ClipView::audioBuckets` | Non-owning PCM min/max buckets rendered inside clips / clip波形の非所有データ |

Track layout API inventory / track配置API一覧:

| API | Contract / 契約 |
|---|---|
| `float TrackExtent(const TrackView&)` | Actual expanded/collapsed row height / 展開状態を含む描画高 |
| `TrackLayout { tracks, top }` | Contiguous non-owning visible rows and absolute first-row pixel offset / 可視行と絶対offset |
| `TimelineProvider::layout`, `totalHeight` | Optional indexed pixel-range query; uniform rows remain supported / 任意の可変高query |
| `TimelineState::heightDrag` | Host-owned track-height transaction / 高さ編集state |
| `EditKind::TrackHeight` | Value x carries original/proposed height, target is track ID / 高さの提案event |

Populate every function pointer from the current OpenGL 3.3 context, initialize the
renderer, render non-owning mesh views and pass `Texture()` to the host UI. Use flipped
UVs for the FBO image. `Pick(x,y)` accepts top-left coordinates. Always call Shutdown
with the original context current. GL state restoration and external synchronization
are host responsibilities. See the concrete host in `examples/gallery/main.cpp`.
current GL 3.3から全関数pointerを取得し、初期化後にmesh viewを描画してTexture()を使用します。
FBO画像は上下反転UV、Pickは左上原点です。必ず同じcurrent contextでShutdownします。
GL state再設定と外部同期はホスト責務です。

The compile fixture `tests/editor_api_compile.cpp` links all public targets. The
external consumer builds that fixture against an independently owned Dear ImGui
target. This demonstrates source integration, not compatibility with arbitrary
Dear ImGui versions or native OS input systems.
API fixtureを全公開targetへlinkし、外部consumerでは別のホスト所有ImGui targetを使用します。
任意版ImGuiとの互換性やnative OS入力の受入を意味しません。

The `Transport(state, bindings, const IconAtlas*)` overload uses host-owned icon textures for play, pause, stop and single-frame stepping. A null atlas uses native text buttons; the original overload remains available.

`Transport(state, bindings, const IconAtlas*)` はホスト所有のアイコンtextureを使い、再生・一時停止・停止・1フレーム移動を表示します。nullではnative文字ボタンを使い、既存overloadも維持します。

`ResolveTimelineSnap` resolves moving clip edges, ignores moving IDs and applies `snapKinds`. `snapping` is the master switch; `magnet` controls provider targets and `snapToFrame` controls frame grid attraction within eight screen pixels. `SnapResult.tick` is the primary anchor; `candidate.tick` is the guide position. The optional `TimelineState.icons` atlas remains host-owned.

`ResolveTimelineSnap` は移動するclipの両端を評価し、移動対象IDと無効な `snapKinds` を除外します。`snapping` が全体切替、`magnet` がprovider候補、`snapToFrame` が8画面pixel以内のフレーム吸着を制御します。`SnapResult.tick` は主端点、`candidate.tick` はguide位置です。任意の `TimelineState.icons` はホスト所有です。

Transport J/K/L uses `PlayReverse`/`Pause`/`PlayForward` bindings. K holds the current playhead; the Stop button rewinds to In. Empty bindings disable keyboard playback. The host can remap every transport command, including Loop.

TransportのJ/K/Lは `PlayReverse` / `Pause` / `PlayForward` bindingを使います。Kは現在位置で一時停止し、StopボタンはInへ戻します。空のbindingではキーボード再生を無効にします。Loopを含む再生commandはホストが変更できます。

Property labels expose Favorite, Locked and Override context actions even while values are locked. Toggle events identify the `PropertyFlags` bit in `Value.x` and the requested state in `Value.y`; the host applies it. Modified/override/locked states use textual indicators in addition to styling. Gallery filters property labels and categories on the following frame and applies object Reset events. `PropertyState.icons` optionally supplies the host atlas for keyframe buttons.

Property名のcontext menuは値がロック中でもFavorite・Locked・Overrideを操作できます。Toggleイベントの `Value.x` は `PropertyFlags` のbit、`Value.y` は変更先の状態で、ホストが反映します。modified・override・lockedは文字でも表示します。Galleryは次フレームで名前とカテゴリの検索を反映し、objectのResetも適用します。`PropertyState.icons` にホストatlasを渡すとkeyframeアイコンを表示します。

Property key actions use `EditKind::PropertyKey`, distinct from curve-key movement. `target` is the property ID; proposed `first` is `PropertyState.time`, `offset` is `PropertyKeyAction`, and `x` is the property value. Gallery stores sorted, host-owned property channels and applies Add/Remove/Previous/Next, updating keyed state at the playhead.

Propertyのkey操作はcurve key移動とは別の `EditKind::PropertyKey` です。`target` はproperty ID、proposedの `first` は `PropertyState.time`、`offset` は `PropertyKeyAction`、`x` はproperty値です。Galleryはソート済みchannelをホスト側で保持し、追加・削除・前後移動とplayhead位置のkeyed表示を反映します。

Asset breadcrumb buttons emit Navigate with a host-supplied `AssetState.breadcrumbIds` target and the breadcrumb index in `proposed.first`. Optional `AssetProvider.filteredCount` updates the host filter index after search/tag/status input, before clipped queries. Gallery applies these filters and breadcrumb navigation; row heights include status/tag labels.

Asset breadcrumbは `AssetState.breadcrumbIds` のホスト指定IDをtarget、indexを `proposed.first` とするNavigateイベントを返します。任意の `AssetProvider.filteredCount` は検索・tag・status入力後、可視query前にホストの絞り込みindexを更新します。Galleryは検索とbreadcrumb移動を適用し、行高にtag・状態表示を含めます。

The CG Gallery Inspector exposes all nine translation, rotation and scale components. Rotation values are radians; reset restores translation/rotation to zero and scale to one. Each component has an explicit host-assigned property ID and shares property lock, favorite, override and keyframe handling.

CG Gallery Inspectorは移動・回転・scaleの全9成分を公開します。回転の単位はradianで、resetは移動・回転を0、scaleを1へ戻します。各成分はホストが明示したproperty IDを持ち、lock・favorite・override・keyframe操作を共用します。

CG Inspector component edits and reset apply an absolute value to the selected objects. Different values display Mixed. A locked object/property rejects the whole edit, and changing selection during a gesture rejects its commit.

CG Inspectorの成分編集とresetは選択objectへ同じ絶対値を適用します。値が異なる場合はMixed表示になります。locked object/propertyを含む場合は一括編集全体を拒否し、gesture中に選択が変わった場合もcommitを拒否します。

`FollowPlayhead` provides Off/Smooth/Page following in seconds. Smooth keeps a ten-percent screen margin in either direction; Page advances whole visible widths and handles reverse playback and multi-page jumps. `TimelineState.autoScroll` selects the behavior through Timeline options.

`FollowPlayhead` は秒単位のOff/Smooth/Page追従を提供します。Smoothは進行方向の両側に表示幅の10%の余白を保ち、Pageは表示幅単位で切り替え、逆再生と複数page移動にも対応します。Timeline optionsで `TimelineState.autoScroll` を変更できます。

TimeRuler draws frame-aligned major/minor ticks with density derived from horizontal zoom. Playback following updates the shared canvas before the Timeline ruler and clips draw. Invalid horizontal scale skips ruler geometry.

TimeRulerは横zoomに応じた密度でフレーム単位のmajor/minor tickを描画します。Timelineは再生追従を共有canvasへ先に反映し、ルーラーとclipを描画します。横scaleが不正な場合はルーラー描画を省略します。

Timeline Fit uses `TimelineProvider.contentRange`, with 24 logical pixels of horizontal padding, preserving vertical scale/scroll. The host supplies `TimelineState.bindings` for the Fit command; the icon button uses the same action. Gallery caches the total range when rebuilding edited data, not during visible queries.

TimelineのFitは `TimelineProvider.contentRange` に左右24論理pixelの余白を付け、縦scale/scrollを維持します。ホストは `TimelineState.bindings` でFitキーを渡し、アイコンボタンも同じ処理を使います。Galleryは編集データ更新時に全体範囲を保持し、可視query中には全件走査しません。

Gallery curve queries use a channel/time index and return channel-contiguous visible keys plus two neighbors at each boundary for automatic tangents. The index is rebuilt on data changes; queries use binary search per channel and reusable host scratch storage. The sample includes three channels.

Galleryのcurve queryはchannel/time indexを使い、channelごとに連続した可視keyと、自動接線用に各境界の隣接2keyを返します。indexはデータ変更時に更新し、queryはchannelごとの二分探索とホストの再利用bufferを使います。sampleは3channelを含みます。

Right-clicking a Curve key exposes interpolation and handle-mode menus. `KeyInterpolation`/`KeyHandleMode` commits target the explicit key ID and carry the enum value in `proposed.x`; Gallery validates and applies them. Locked keys reject these edits.

Curveのkeyを右クリックすると補間とhandle modeのメニューを開きます。`KeyInterpolation` / `KeyHandleMode` のcommitは明示key IDをtargetにし、`proposed.x` にenum値を返します。Galleryは値を検証して適用し、locked keyへの編集は拒否します。

`CurveState.activeChannel` follows clicked keys. With `ghostOtherChannels`, inactive channels draw as faint dashed curves with hollow key diamonds and no tangent handles; clicking a key activates its channel. The canvas context menu toggles ghost display.

`CurveState.activeChannel` はクリックしたkeyのchannelへ切り替わります。`ghostOtherChannels` 有効時は他channelを薄い破線と中空diamondで描画し、接線handleを隠します。keyクリックでchannelをactiveにでき、canvas context menuからghost表示を切り替えられます。

Optional `CurveProvider.sample` evaluates a complete host channel at an exact tick and requested extrapolation mode. With this callback, CurveEditor samples across the visible width and exposes Constant/Linear/Repeat in its context menu through `CurveState.extrapolation`. Gallery provides indexed complete-channel evaluation, including portions outside the key range.

任意の `CurveProvider.sample` は正確なtickと指定extrapolationでホストの全channelを評価します。callbackがある場合、CurveEditorは可視幅を描画し、context menuから `CurveState.extrapolation` のConstant/Linear/Repeatを切り替えます。Galleryはkey範囲外も含む全channel評価を索引経由で提供します。

Curve Fit uses optional complete-channel `CurveProvider.bounds` in seconds/negative-value coordinates. `CurveState.bindings` supplies the remappable Fit command; the context menu shares this action. Flat ranges expand by one unit before fitting. Gallery caches bounds during index rebuild.

Curve Fitは秒/負の値座標による全channelの `CurveProvider.bounds` を使います。`CurveState.bindings` から変更可能なFit commandを渡し、context menuも同じ処理を使います。幅のない範囲は1単位に広げて表示し、Galleryはindex更新時に範囲を保持します。

Optional host-owned `CurveState.previewKeys` scratch (at least the visible query size) renders the drag proposal before commit, including sorted key crossings and resolved handle edits. Gallery maintains this scratch alongside its visible-key buffer. Clicking an already-selected key preserves the other selected IDs.

ホスト所有の任意buffer `CurveState.previewKeys` に可視query以上の要素数を渡すと、commit前の移動案を描画します。keyの順序変更と解決済みhandle編集も反映します。Galleryは可視key bufferとともにこの領域を保持します。選択済みkeyをクリックしても他の選択IDを維持します。

Curve multi-key move uses complete selected-key views from `CurveProvider.selected` and host-owned `CurveState.companionDrags`. Each key emits its own typed transaction with a shared time/value delta. Capacity is checked for the whole batch; terminal events retry together, and any locked selected member prevents the gesture. Gallery supplies these buffers and applies each committed key.

Curveの複数key移動は `CurveProvider.selected` の全選択keyとホスト所有 `CurveState.companionDrags` を使います。各keyは共通の時間/値差分を持つtransactionを返します。容量はbatch全体で確認し、終端イベントも一括再送します。lockedな選択keyを含むgestureは開始しません。Galleryはbufferを提供し、各keyのcommitを反映します。

Alt-dragging Curve keys emits a Duplicate transaction per selected key. Gallery copies the source key metadata, assigns a new StableId and applies the proposed time/value, leaving source keys unchanged.

CurveのAlt-dragは選択keyごとのDuplicate transactionを返します。Galleryは元keyの属性をコピーして新しいStableIdを割り当て、提案された時間/値を適用し、元keyを維持します。

Curve frame snapping uses host `CurveState.rate` and `snapToFrame`, available in the context menu. The primary key snaps to the nearest frame; companion keys receive the same tick delta, preserving their spacing. Preview and committed proposals share this calculation.

Curveのフレーム吸着はホストの `CurveState.rate` とcontext menuの `snapToFrame` を使います。主keyを最寄りのフレームへ合わせ、他の選択keyには同じtick差分を適用して間隔を保ちます。previewとcommit案は同じ計算を使います。

Curve context option `scaleTime` starts `KeyScale` transactions around the earliest selected tick: 100 horizontal pixels doubles timing distances, preserving values. The pivot/mode are latched at Begin, and optional frame snapping applies to each scaled time.

Curveのcontext設定 `scaleTime` は選択keyの最初のtickを基準に `KeyScale` transactionを開始します。横100pixelで時間間隔を2倍にし、値を維持します。基準とmodeはBegin時に固定し、フレーム吸着は各scale後の時刻へ適用します。

Curve Delete uses the host Delete binding or context menu and emits one Remove commit per selected key. Locked targets reject the whole deletion; missing selected views or insufficient event capacity report overflow without partial events. Gallery removes keys by explicit ID.

CurveのDeleteはホストbindingまたはcontext menuから選択keyごとにRemove commitを返します。locked対象を含む削除は全体を拒否し、選択view不足・event容量不足は部分イベントを返さずoverflowを通知します。Galleryは明示IDでkeyを削除します。

Curve AddKey inserts at host `CurveState.time` on the active channel (`KeyInsert` target is the channel ID). Optional `neighbor` locates previous/next keys without a full scan; navigation events carry the key ID and exact tick. Gallery updates playhead/selection and avoids duplicate keys at the same channel/time.

CurveのAddKeyはホストの `CurveState.time` とactive channelへ追加します（`KeyInsert` のtargetはchannel ID）。任意の `neighbor` から全件走査せず前後keyを取得し、key IDと正確なtickをNavigateで返します。Galleryはplayhead/選択を反映し、同じchannel/時刻にkeyを重複追加しません。

Curve box/lasso selection uses optional `selectionQuery` and the shared CanvasSelection widget. Gallery supplies query results and lasso path scratch. Selection/event capacity is preflighted; insufficient capacity leaves the old selection intact, and an exhausted lasso path cancels rather than selecting from a truncated polygon.

Curveのbox/lasso選択は任意の `selectionQuery` と共通CanvasSelectionを使います。Galleryはquery結果とlasso用bufferを提供します。選択/event容量を先に確認し、不足時は元の選択を保持します。lasso pathを使い切った場合も途中のpolygonで選択せず中止します。

`EditKind::StripSettings` carries strip range in `first/last`, channel in `parent`,
scale/repeat/blend in `x/y/z`, and mute/lock bits 1/2 in `offset`. The host applies
Commit and retains its source data during Update. A locked strip still exposes Unlock.

`EditKind::StripSettings`はrangeを`first/last`、channelを`parent`、scale/repeat/blendを
`x/y/z`、mute/lockを`offset`のbit 1/2で返します。ホストはCommit時に適用し、Update中は
元データを維持します。ロック中も解除操作は可能です。

Animation strip `Reorder` uses the neighboring strip StableId in `proposed.parent`
and direction (-1/+1) in `proposed.offset`. The Gallery swaps the adjacent strips.
Animation stripの`Reorder`は隣接stripのStableIdを`proposed.parent`、方向（-1/+1）を
`proposed.offset`で返します。Galleryは隣接stripを交換します。

UV Select All uses `UVState::bindings` and `UVProvider::all(user, selectionMode)`.
Return unique selectable IDs for the entire mode, including offscreen elements.
Missing provider or insufficient selection/event storage preserves the old selection and sets overflow.
UVの全選択は`UVState::bindings`と`UVProvider::all(user, selectionMode)`を使用します。
画面外も含む、その選択単位の一意な選択可能IDを返してください。provider未設定や
選択・イベント容量不足では元の選択を保持し、overflowを通知します。

UV box/lasso uses `UVProvider::selectionQuery` with UV-coordinate bounds and the active
selection mode. Supply unique representative points; CanvasSelection performs the final
box/polygon containment test. Lasso requires host storage in `UVState::canvas.selectionPath`.
UVのbox/lassoは`UVProvider::selectionQuery`へUV座標の範囲と選択単位を渡します。
一意な代表点を返すと、CanvasSelectionが矩形・多角形の内外判定を行います。
lassoには`UVState::canvas.selectionPath`へホストの作業領域を設定してください。

Multi-vertex UV transforms require `UVProvider::selected` to return the complete selection
and `UVState::companionDrags` to hold the other vertex transactions. Insufficient buffers
reject Begin or retain the complete terminal batch for retry; no partial Commit is emitted.
UVの複数頂点変換では`UVProvider::selected`で選択全体を返し、他の頂点のtransactionを
`UVState::companionDrags`へ確保します。容量不足はBeginを拒否するか、終了イベント全体を
再試行用に保持します。Commitの一部だけを返すことはありません。

UV coordinate display keeps geometry in normalized UV units. Set `UVState::imageSize`
to the host texture dimensions for pixel tooltips. UDIM labels use 1001 + u + 10v for
nonnegative tile rows and columns 0..9; other columns are explicitly outside this convention.
UVの内部座標はnormalizedを維持します。pixel表示には`UVState::imageSize`へホストtextureの
寸法を設定します。UDIMは非負の行と0..9列で1001 + u + 10vを表示し、範囲外は明示します。

UV Edge mode keeps edge IDs in Selection. `selected` expands them into unique endpoint
vertices; transform events target those vertex IDs. `UVEdge::aVertex/bVertex` link edge
preview to vertex proposals. Gallery box/lasso tests edge midpoints.
UV EdgeモードのSelectionは辺IDを保持します。`selected`は重複を除いた端点頂点へ展開し、
変換イベントは頂点IDを対象にします。`UVEdge::aVertex/bVertex`で辺のプレビューを頂点の
提案座標へ接続します。Galleryのbox/lasso選択は辺の中点を判定します。

`UVFace` references an ordered non-owning vertex span and carries separate face/island
IDs. Face/Island click selection tests the polygon interior; `selected` must expand the
selected face/island IDs into every unique transformable vertex. `overlap` is host-computed
and displayed as a marked outline, not a library topology analysis.
`UVFace`は順序付き頂点spanを非所有参照し、面IDとisland IDを保持します。面内クリックで
選択し、`selected`は選択IDを重複のない構成頂点へ展開します。`overlap`はホストが計算し、
ライブラリは輪郭とラベルで表示します。トポロジー解析は行いません。


## Timeline and monitor additions / TimelineとMonitorの追加API

| API | Host contract / ホスト契約 |
|---|---|
| `ClipView::keyChannel`, `keyDefaultValue` | Explicit insertion channel and empty-channel default; never inferred from adjacent IDs / 明示的な追加先channelと空channelの値。隣接IDから推定しない |
| `ClipView::envelope`, `EnvelopePoint` | Borrowed time-sorted local-tick points with StableId, gain and lock / StableId・gain・lockを持つclip内時刻順の非所有点列 |
| `EvaluateEnvelope(points,tick)` | Linear gain, constant endpoints, empty=1 / 線形gain、範囲外は端点、空なら1 |
| `EditTransition(clip,end,delta)` | Pure duration edit, preserves the other end and clamps to clip duration / 他端を維持しclip長内に制限する純粋計算 |
| `TransitionPicker(id,clip,revision,events,trackLocked)` | Atomic Begin/Commit type choice; host applies resulting transition kinds / 種別変更をBegin／Commitの一括イベントで返しホストが適用 |
| `MonitorOptions::transformBounds`, `anchor` | Host-provided normalized display rectangle and point; independent of texture flip / texture反転とは独立したホスト指定の表示正規化矩形と点 |
| `MonitorOptions::metadataPreset` | `Off`, `Clip`, `Details`; host owns choice / 選択状態はホスト所有 |
| `MonitorOptions::clipName`, `markerComment`, `metadata` | UTF-8 strings/lines borrowed for the call; clipped to available overlay space / 呼出し中だけ借用するUTF-8文字列・行。overlay範囲内にclip |
| `EditorPalette::effectClip`, `adjustmentClip`, `groupClip` | Semantic colors used by the corresponding Timeline track roles / 対応するTimeline track種別の意味色 |

Timeline `AddKey` emits `KeyInsert` with target=channel, parent=clip, first=clip-local tick, x=value. Previous/next key commands emit `Navigate` with target=key, parent=clip, first=destination timeline tick. Envelope actions use `AudioEnvelope`: target=point for edit/remove or clip for insert, parent=clip, first=local tick, x=gain, offset=0 edit/1 insert/2 remove. Storage, stable IDs, revision changes and actual application remain host responsibilities.

Timelineの`AddKey`はtarget=channel、parent=clip、first=clip内tick、x=値の`KeyInsert`を返します。前後key操作の`Navigate`はtarget=key、parent=clip、first=移動先timeline tickです。`AudioEnvelope`は編集／削除でtarget=point、追加でtarget=clip、parent=clip、first=clip内tick、x=gain、offset=0編集／1追加／2削除を使用します。格納領域・StableId・revision更新・データ適用はホストの責任です。

`video::TrackLabels`, stored in `TimelineState::trackLabels`, supplies borrowed UTF-8 strings for seven track buttons/tooltips (Visible through Source order), the compact controls menu, height control, boolean status and expand/collapse tooltip. Defaults retain English labels. Keep the strings valid for the Timeline call.

`TimelineState::trackLabels`の`video::TrackLabels`は7つのtrackボタンとtooltip（VisibleからSourceの順）、省略menu、高さ操作、真偽状態、展開・折り畳みtooltipへ非所有UTF-8文字列を指定します。既定値は英語で、文字列はTimeline呼出し中有効に保つ必要があります。

`TimelineProvider::canBeginEdit(user, clip, kind)` is an optional host preflight for clip-body tools and Razor. Returning false suppresses their edit Begin without reporting a buffer shortage. It runs on gesture start, not steady frames. The host can inspect nonlocal constraints such as locked ripple followers; revision changes still cancel active transactions. Caption, envelope, transition and key tools retain their own contracts.

`TimelineProvider::canBeginEdit(user, clip, kind)`はclip本体の編集toolとRazor向けの任意ホスト判定です。falseなら容量不足扱いにせず編集Beginを抑止します。定常フレームでは呼ばず、操作開始時に後続clipのlockなど非局所制約を確認できます。操作中のrevision変更は既存契約に従ってCancelします。caption、envelope、transition、keyの操作はそれぞれの契約を維持します。

`ClipView::keyEvaluation` optionally borrows the full sorted local-time channel for key insertion evaluation. `keys` continues to supply drawable/editable clip-local keys. Empty evaluation context falls back to `keys`, then `keyDefaultValue`. The evaluator uses binary search and neighboring tangents; it does not draw or select off-clip context. Both spans remain host-owned.

`ClipView::keyEvaluation`はkey挿入値の評価用に、ローカル時刻順の全channelを非所有参照できます。`keys`は引き続き描画・編集対象のclip内keyです。評価context未指定時は`keys`、さらに空なら`keyDefaultValue`を使用します。評価は二分探索と隣接接線を使用し、clip外contextを描画・選択しません。両spanともホスト所有です。

`TrackLabels::kinds` supplies six UTF-8 role tooltips in TrackKind order for the track heading glyph. Gallery uses Japanese role names when its language option is enabled.

`TrackLabels::kinds`はtrack見出しglyphの種類tooltipをTrackKind順に6つ指定します。Galleryの日本語切替では日本語の種類名を渡します。

`TimelineState::labels` borrows `TimelineLabels` strings for seven tools/tooltips, Snap, Magnet, the options popup, follow modes, snap kinds and Fit. Keep UTF-8 strings valid during the call; defaults are English. Binding behavior and popup IDs are unchanged.

`TimelineState::labels`の`TimelineLabels`は7種類のtool名・tooltip、Snap、Magnet、設定popup、追従mode、snap対象、FitのUTF-8文字列を非所有参照します。呼出し中の寿命をホストで保証し、未指定時は英語を使用します。bindingの挙動とpopup IDは維持します。

ToolSelect/ToolRazor/ToolRipple/ToolRoll/ToolSlip/ToolSlide/ToolHand are appended Command IDs. Timeline consumes them only with canvas focus and no active edit. MakeBindings supplies V/C/B/N/Y/U/H respectively across the built-in presets; the host may replace or remove these bindings. Tooltips show the first active host binding. These are ImKit preset defaults, not a claim of exact third-party shortcut parity.

ToolSelect／ToolRazor／ToolRipple／ToolRoll／ToolSlip／ToolSlide／ToolHandをCommand末尾へ追加しました。Timelineはcanvasにfocusがあり編集中でない場合に使用します。MakeBindingsは各presetで順にV／C／B／N／Y／U／Hを設定し、ホストで変更・削除できます。tooltipは最初の有効なホストbindingを表示します。これはImKitの既定割当であり、第三者製品との完全一致を示すものではありません。

`TimelineProvider::isEditable(user, clip)` optionally validates the existence and lock state of each active clip-body edit owner, including offscreen related clips. A false result cancels the whole active batch; full output buffers retain the terminal operation for retry. Use an indexed lookup, not a full clip scan. Without this callback the host must change revision when owners disappear or become locked.

`TimelineProvider::isEditable(user, clip)`は、画面外の関連clipを含む編集中の各clip本体について、存在とロック状態を確認する任意callbackです。falseなら編集バッチ全体をCancelし、出力容量不足時は終了処理を保持して再試行します。全clip走査ではなく索引を使用してください。callbackを省略する場合、対象の削除・ロック変更時にホストがrevisionを更新する必要があります。

The Timeline clip context menu can detach the clicked clip from its link or group set. `EditKind::Link` emits Begin/Commit with target=clip ID, offset=0 for linked or 1 for group, original.parent=old set ID and proposed.parent=0. The host validates ownership, revision and original membership before applying. TimelineLabels::unlink/ungroup are borrowed UTF-8 labels. Locked owners disable both actions.

Timelineのclip context menuはクリック対象をlinked／group集合から外せます。`EditKind::Link`のBegin／Commitでtarget=clip ID、offset=linkedなら0・groupなら1、original.parent=元集合ID、proposed.parent=0を返します。ホストが所有権・revision・元集合を検証して適用します。TimelineLabels::unlink／ungroupは借用UTF-8文字列です。locked対象では両操作を無効化します。

The same context menu creates link/group sets for multiple selected clips and their provider-resolved related members. `EditKind::Link` offset=2 requests a new link set, offset=3 a new group set; proposed.parent=0 asks the host to allocate one set ID per kind per batch. original.parent retains each previous membership. Preflight every member before applying; Gallery implements this and preserves the other relationship type. TimelineLabels::linkSelection/groupSelection provide UTF-8 labels. Relation actions share the existing transition/envelope popup rather than opening competing popups.

同じcontext menuから複数選択clipとproviderが解決した関連先をlink／group化できます。`EditKind::Link`のoffset=2は新規link集合、3は新規group集合で、proposed.parent=0はホストへバッチ・種類ごとに1つの集合ID発行を要求します。original.parentには各対象の元所属を保持します。全対象を事前検証してから適用し、Galleryはこの契約と別種の関係の保持を実装しています。TimelineLabels::linkSelection／groupSelectionでUTF-8ラベルを指定します。関係操作は既存transition／envelope popupへ統合し、競合するpopupを開きません。

`cg::Transform::shear` stores upper-triangular xy, xz and yz terms. The linear map is rotation multiplied by a matrix with scale on its diagonal and these three off-diagonal terms. `LinearBasis` returns its world-space columns; `NormalBasis` returns the inverse transpose (zero for singular maps). Oriented nonuniform gizmo scaling preserves the full affine map through QR decomposition. Scale event Values retain scale in x/y/z and set hasAffine=true with affine={rotation.x,y,z,shear.xy,xz,yz}. Hosts must apply this payload together with scale; pivot translation remains its companion transaction. Gallery and both preview paths implement this contract. Zero shear and default hasAffine=false preserve ordinary TRS host events.

`cg::Transform::shear`は上三角行列のxy・xz・yz成分です。線形変換はrotationと、対角をscale・上三角をshearとする行列の積です。`LinearBasis`はworld空間の列、`NormalBasis`は逆転置（特異変換では0）を返します。任意方向の非均等gizmo scaleはQR分解で全アフィン変換を保持します。Scaleイベントはx／y／zにscaleを保持し、hasAffine=true、affine={rotation.x,y,z,shear.xy,xz,yz}を返します。ホストはscaleと一緒にこのpayloadを適用し、pivot位置は併用transactionで処理します。Galleryと両preview経路はこの契約を実装しています。shear=0と既定hasAffine=falseでは従来のTRSイベントを扱えます。

`preview::DrawMeshNormals` draws borrowed mesh vertex and/or face normals over either preview path, using `NormalOverlayOptions`. Length is in world units after normalization; vertex directions use inverse-transpose affine transforms and face directions use transformed triangle geometry. Invalid indices and zero/invalid lengths are skipped. It returns emitted segments, clips to the viewport and does not perform depth occlusion. Gallery connects ViewportState::normals/faceNormals to this overlay.

`preview::DrawMeshNormals`は借用meshの頂点・面normalを両preview上へ描く公開DrawList APIで、`NormalOverlayOptions`で選択します。長さは正規化後のworld単位、頂点方向はアフィン変換の逆転置、面方向は変換後の三角形から計算します。無効indexや0・無効な長さを除外し、描いた線分数を返します。viewportでclipしますが、depthによる遮蔽は行いません。GalleryはViewportState::normals／faceNormalsをこの表示へ接続します。

### Viewport labels / Viewportの表示文字列

`cg::ViewportState::labels` accepts borrowed UTF-8 strings for tools, projection,
orientation, pivot, shading, overlays and navigation hints. The host retains string
storage through each viewport submission. Defaults are English; Gallery supplies Japanese.

`cg::ViewportState::labels`でツール、投影、orientation、pivot、shading、overlay、
navigationの表示文字列をUTF-8で指定できます。文字列は非所有参照で、ホストが描画中の寿命を保証します。
既定は英語、Galleryは日本語切替に接続しています。

### Viewport framing and measurement / 枠と寸法表示

`ViewportState` exposes camera-frame, safe-frame, render-region, passepartout and
measurement overlays, submitted by `ViewportObjects` after the host preview.
`frameAspect` fits a centered frame with a 5% viewport margin. Safe frames use 5%
and 10% insets. `renderBounds` is normalized within that frame; it is a visual guide,
not a renderer scissor. `measurementStart/End` are host-owned world coordinates;
the label reports distance in host world units. These DrawList overlays have no depth occlusion.
Gallery measures two selected origins, or the selected origin from world zero.

`ViewportState`でカメラ枠、安全枠、レンダー領域、枠外暗転、寸法を指定し、ホストのpreview後に
`ViewportObjects`で描画します。`frameAspect`の枠はviewportの上下左右に最低5%の余白を取り、
安全枠はその内側5%・10%です。`renderBounds`は枠内の正規化座標で、描画範囲制限ではなく表示ガイドです。
寸法端点はホストのワールド座標、数値はホストの距離単位です。overlayは深度遮蔽を行いません。
Galleryは選択した2原点間、単一選択ではワールド原点からの距離を表示します。

### Mesh selection outline / メッシュの選択輪郭

`preview::DrawMeshOutline` draws open boundary edges and front/back silhouette edges
from indexed triangles. Coincident vertex positions share an edge even when normal/UV
seams use separate indices. The host supplies `OutlineEdge` scratch (one entry per
triangle edge); the return value reports required capacity and shortage draws nothing.
The overlay allocates no memory, retains no mesh, and has no depth occlusion.
Triangles crossing the near plane are omitted. Gallery connects selected object IDs
and the viewport outline toggle to this overlay for both preview paths.

`preview::DrawMeshOutline`はindexed triangleの開いた境界と表裏の境界を描画します。
normal/UVの継ぎ目でindexが分かれていても、同一位置の頂点は同じ辺として扱います。
ホストが三角形の辺ごとに`OutlineEdge` scratchを用意し、戻り値は必要容量を返します。
不足時は部分描画せず、メモリ確保やmesh保持も行いません。深度遮蔽はなく、near planeを横切る三角形は省略します。
Galleryは両preview経路で選択IDと輪郭表示切替へ接続しています。

The viewport reuses SelectPointer/Move/Rotate/Scale for transform tools, Layers for
the overlay menu, Camera for camera choices/frame, and Ruler for measurement.
Selected tools retain a background and underline. Native state-driven captures in
`out/cg-overlays-integrated/` cover light and Japanese dark 150% framing, normals,
and selected silhouette; they do not establish native OS/IME input acceptance.

Viewportは選択・移動・回転・拡縮、表示メニュー、カメラ、寸法へ既存アイコンを再利用し、
選択ツールを背景と下線でも区別します。`out/cg-overlays-integrated/`のnative captureは
lightと日本語dark 150%で枠・法線・選択輪郭を確認したものです。状態を直接設定した描画確認で、OS／IME入力の合格根拠ではありません。

### Outliner filters and labels / Outlinerのfilterと表示文字列

`OutlinerState::filter` selects all, visible, hidden, locked or selected objects.
The host applies it with the text search when building provider rows and `visibleCount`,
including ancestors of matches. Gallery opens matching paths while a filter is active.
`labels` supplies borrowed UTF-8 UI/context strings; `icons` supplies a borrowed atlas
for restriction controls, whose checked state also uses an underline.

`OutlinerState::filter`は全対象・表示・非表示・ロック・選択対象を切り替えます。
ホストが検索文字列と併せてprovider行数と行を構築し、一致対象の祖先も含めます。
Galleryはfilter中に一致する階層を展開します。`labels`は非所有UTF-8文字列、`icons`は
非所有atlasです。制限状態の操作はアイコンと有効状態の下線で表示します。

### Camera/light primitives / カメラ・ライトのprimitive

`preview::CameraPrimitive` writes a 24-vertex/36-index frustum along local +Z;
`LightPrimitive` writes a 24-vertex/24-index point-light octahedron. Both use host
storage, reject insufficient spans before writing, and provide face normals/colors.
They feed the ordinary DrawList/OpenGL mesh path, including transforms and picking.
Gallery camera/light objects own these helper meshes; they are visualization helpers,
not a camera projection model or light simulation.

`CameraPrimitive`はローカル+Z方向の視錐台（24頂点・36index）、`LightPrimitive`は
点光源を表す八面体（24頂点・24index）をホスト領域へ書き込みます。容量不足時は書き込まず、
面法線と色も供給します。通常のDrawList／OpenGL mesh経路で変換・pickingへ接続し、
Galleryのカメラとライトが所有します。補助表示形状であり、投影モデルや光源simulationではありません。

### Related ripple trims / 関連対象のRipple trim

Ripple joins the complete selected/linked/group batch returned by `selected`.
Every participant passes lock and `canBeginEdit` checks before Begin, shares the
most restrictive media-handle delta, and commits atomically through member scratch.
Gallery computes follower shifts from the pre-edit snapshot and applies their sum
once, so a later selected clip cannot overwrite an earlier ripple shift.

Rippleも`selected`が返す選択・linked・group全対象を一括編集します。各対象のlockと
`canBeginEdit`をBegin前に確認し、media handleが許す共通deltaで編集します。
Galleryは編集前の位置から後続clipの移動量を合算して一度だけ適用し、後の選択clipの
Commitが先行Rippleの移動を上書きしないようにしています。

### Related Roll/Slide / 関連Roll・Slide

MemberDrag stores neighboring originals and transactions for related Roll/Slide.
The complete batch reserves Begin/terminal capacity, checks every adjacent owner,
and constrains a common delta across all pairs/triples. Neighbor previews use the
same values that commit. Conflicting sets that write the same clip twice are rejected
before Begin; hosts should expose disjoint linked edit neighborhoods.

関連Roll／Slideでは`MemberDrag`に隣接clipの元値とtransactionも保持します。
全対象のBegin／終了容量と編集可否を確認し、すべてのpair／tripleへ共通deltaを適用します。
隣接clipのpreview値もCommitと一致します。同じclipを二重に編集する競合はBegin前に拒否します。

### Camera object pose / カメラオブジェクトの姿勢

`CameraFromTransform` derives a camera from local +Z forward/+Y up, preserving lens
settings and ignoring scale/shear. `Camera::roll` is shared by projection, view-oriented
gizmos and both preview renderers. Gallery tracks its camera by StableId and applies
committed and gizmo-preview poses to Camera View; a missing camera disables that choice.

`CameraFromTransform`はローカル+Z前方・+Y上方の姿勢からカメラを求め、lens設定を保持します。
scale／shearは視点へ適用しません。rollは投影・View orientation・両previewで共通です。
GalleryはStableIdでカメラを保持し、確定値とgizmo previewをCamera Viewへ同期します。

### Transport actions and labels / Transport操作と表示文字列

`TimeState::labels` borrows UTF-8 transport strings. The timecode context menu offers
work-range start/end navigation, In/Out reset to the work range, and forward/reverse
playback. `GoToStart`, `GoToEnd` and `ClearInOut` also accept host bindings; no new
preset shortcuts override existing Fit bindings. Boundary navigation reuses previous/
next-track glyphs with explicit labels; single-frame navigation keeps its distinct frame glyphs.

`TimeState::labels`でTransportのUTF-8表示文字列を指定できます。時刻表示の右クリックから
work rangeの先頭／末尾、In／Outのwork rangeへのリセット、順／逆再生を操作できます。
追加Commandもホストbindingへ割り当てられます。既存Fitとの競合を避け、presetは変更しません。
境界移動と1フレーム移動は別のglyphで区別します。

`PropertyState::labels` and `ComponentStackOptions::labels` borrow UTF-8 headings,
status suffixes, context actions and keyframe tooltips. Gallery supplies Japanese
labels for Video/CG property panels and the component stack.

`PropertyState::labels`と`ComponentStackOptions::labels`は見出し、状態接尾辞、
context操作、keyframe tooltipの非所有UTF-8文字列を受けます。GalleryのVideo／CG
propertyとcomponent stackは日本語切替へ接続しています。

### Typed Outliner rows / Outlinerの行種別

`ObjectView::kind` distinguishes object, collection, mesh, camera, light, component
and modifier rows. `OutlinerState::kindFilter` (-1 for all) is applied by the host
alongside text/state filters. Gallery includes component/renderer-override children
and their ancestors, supports component rename/duplicate/reorder/reparent, and maps
component selection to its owner's Inspector. Component restriction fields map
visible to enabled and locked to component lock; unsupported restrictions are disabled.

`ObjectView::kind`はObject／Collection／Mesh／Camera／Light／Component／Modifierを区別します。
`kindFilter`（-1は全種類）をホストが検索・状態filterと併せて適用します。Galleryはcomponentと
描画modifierを子行へ含め、名前変更・複製・並べ替え・所有対象変更を適用します。
component選択では所有objectのInspectorを表示し、表示制限は有効状態、lockはcomponent lockへ対応します。

`MonitorControls(id, options, atlas, labels)` edits host-owned display options for safe area, guides, timecode, transform bounds, anchor and metadata preset. Atlas and UTF-8 labels are borrowed. `MonitorOptions::showAnchor` is optional: unset preserves the existing coupling to `transform`; an explicit bool controls anchor visibility independently. Gallery keeps separate Source/Program options and exposes controls in each monitor's context menu. No media state, texture or renderer is owned by this widget.

MonitorControlsはホスト所有のセーフエリア・ガイド・timecode・変形枠・anchor・metadata設定を変更します。atlasとUTF-8ラベルは非所有です。MonitorOptions::showAnchorは未指定なら従来のtransform連動を維持し、bool指定時は独立して表示を制御します。GalleryはSource／Program別の設定を保持し、各Monitorのcontext menuへ公開部品を接続します。media状態・texture・rendererは所有しません。

TimelineLabels also supplies borrowed UTF-8 clip metadata/status, envelope actions and key-drag tooltip labels. Host strings are passed as text arguments rather than printf formats. Gallery supplies Japanese translations.

TimelineLabelsはclip情報・素材状態・envelope操作・keyドラッグのtooltipも非所有UTF-8で受け取ります。ホスト文字列はprintf形式ではなく文字列引数として扱い、Galleryは日本語訳を渡します。

The nine-argument AnimationStrips overload adds borrowed StripOptions (atlas and UTF-8 StripLabels), retaining the existing eight-argument entry point. EditStripRange is a pure range edit: trims saturate at Tick bounds and preserve at least one Tick; invalid/empty ranges, unsupported kinds and overflowing moves return nullopt. Gallery uses it for strip dragging and provides Japanese setting labels. DopeSheet reuses CurveLabels for scale/snap settings.

9引数AnimationStripsは非所有atlas・UTF-8 StripLabelsをStripOptionsで追加し、従来の8引数入口を維持します。EditStripRangeは純粋な範囲編集で、trimはTick境界に制限して最小1 Tickを保持します。不正/空範囲・未対応kind・Moveのoverflowではnulloptを返します。Galleryはstripドラッグに使用し、設定名を日本語で渡します。DopeSheetのscale/snap設定はCurveLabelsを共用します。

## Range, marker and property array controls / 範囲・マーカー・配列

`TimeState::rulerLabels` and `icons` borrow host display resources. TimeRuler draws
work and In/Out intervals; endpoint drag changes host navigation state and Escape
restores the starting range. The context menu edits range endpoints in seconds.
Existing marker drag emits Begin/Update/Commit/Cancel with its explicit marker ID;
context removal emits Remove. Add uses target zero so the host allocates a new ID.
Gallery updates existing markers by ID and applies removal.

`TimeState::rulerLabels`と`icons`はホストの表示資源を借用します。TimeRulerは作業範囲と
In/Outを表示し、端点ドラッグでホストの移動範囲を更新、Escapeで開始時の範囲へ戻します。
右クリックから秒単位の端点編集もできます。既存マーカーのドラッグは明示ID付きの
Begin/Update/Commit/Cancel、削除はRemoveを返します。追加のtargetは0で、ホストがIDを
採番します。Galleryは既存IDへの移動と削除を適用します。

`PropertyView::arrayElement` enables reorder context actions when
`PropertyProvider::neighbor(user,id,-1/+1)` supplies an unlocked sibling. Reorder
carries the source ID and destination sibling ID in proposed.parent. The host owns
order and storage. `PropertyView::icon` supplies an optional semantic glyph;
`PropertyLabels::mixed/moveUp/moveDown` are borrowed UTF-8 strings. Mixed text is
rendered as text, never interpreted as a numeric format string. Gallery demonstrates
editable, reorderable custom parameters with reset, lock, favorite and key actions.

`PropertyView::arrayElement`と`PropertyProvider::neighbor(user,id,-1/+1)`で配列の並べ替え
操作を有効にします。ロックされた要素・隣接要素は移動できません。Reorderのtargetは移動元ID、
proposed.parentは移動先の兄弟IDで、順序とデータはホスト所有です。`PropertyView::icon`は
任意の意味glyph、`PropertyLabels::mixed/moveUp/moveDown`は借用UTF-8文字列です。
mixed文字列は数値formatとして解釈しません。Galleryのカスタム配列では値編集・並べ替え・
reset・lock・favorite・key操作をホストのデータへ反映します。

`RippleDeletePosition` computes a surviving clip position from sorted, non-overlapping
removed clips. It rejects interval overlap and tick overflow. Timeline emits a reserved
related-clip RippleDelete batch. Gallery preflights related membership and locked
followers before removing clips and shifting survivors together. Caption track context
insertion emits CaptionInsert; Gallery creates a three-second synthetic caption in the
first free interval at or after the requested playhead, then selects it for inline editing.

`RippleDeletePosition`は時刻順で重複しない削除クリップから残存位置を計算し、重複・Tick溢れを
拒否します。Timelineは関連クリップのRippleDeleteイベントをまとめて確保し、Galleryは
関連対象と後続のロックを確認してから削除・後続移動を一括適用します。字幕trackの追加操作は
CaptionInsertを返し、Galleryは指定再生位置以降の最初の空きに3秒の合成字幕を作成して選択します。

`ComponentView::icon` and `ComponentTypeView::icon` let the host distinguish component
and modifier rows/types with borrowed atlas glyphs. / 両iconフィールドによりcomponentとmodifierの
行・追加候補をホスト指定glyphで区別できます。

Property value context menus provide Copy/Paste through the host ImGui clipboard callbacks. Paste accepts one finite number and emits a typed Property event; invalid text leaves the model unchanged. / 値の右クリックメニューはホストのImGui clipboard callbackを使用し、有限な数値だけをPropertyイベントで貼り付けます。
