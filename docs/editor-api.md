# Editor API reference / Editor API reference

These are custom APIs, separate from the generated native Dear ImGui overload inventory.
They are development APIs; full 1.0 acceptance is pending. Include only the modules used
by the host. All module targets publish C++20 and retain the host's ImGui target.
以下はDear ImGui標準overloadとは別の開発中APIです。必要なmoduleだけをinclude/linkします。
すべてC++20とホストのImGui targetを継承します。

| Target | Header / namespace | Main API |
|---|---|---|
| `imkit::editor_core` | `imkit/editor_core.h`, `imkit::editor` | `StableId`, `Tick`, `FrameRate`, `FrameToTick`, `TickToFrame`, `FormatTimecode`, `ParseTimecode`, `EventBuffer`, `Transaction`, `Selection`, `ResolveSnap`, `CanvasState`, `BeginCanvas`, `CanvasSelection`, `TimeRuler`, `Transport`, `CurveEditor`, `ResolveHandles`, `MoveHandle`, `Evaluate`, `PropertyGrid`, `AssetBrowser`, `Splitter`, `StatusBar` |
| `imkit::video` | `imkit/video.h`, `imkit::video` | `TimelineProvider`, `TimelineState`, `Timeline`, `EditClip`, `RollClips`, `SlideClip`, `SplitClip`, `Monitor`, `BuildAudioBuckets`, `UpdateMeter`, `Waveform`, `LevelMeter`, `AudioStrip`, `BuildScopes`, `Histogram`, `ScopeImage`, `ColorControls` |
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
| `bool ColorControls(const char*, ColorValues&)` | Host draft editing; compatible existing signature / 既存draft編集 |
| `void ColorControls(const char*, const ColorValues&, const ColorPropertyIds&, uint64_t, ColorState&, EventBuffer&, const ColorLabels& = {})` | Immutable source and typed RGB/scalar transactions; six independent IDs / 不変sourceと明示IDのtransaction |

Both ColorControls overloads render actual draggable three-way wheels and level
sliders. ColorState records current wheel centers/radius for host overlays and
public-IO automation. Neither overload provides a color management pipeline.
両overloadはthree-way wheelとlevel sliderを描画します。ColorStateの中心座標・半径はホストのoverlay・
公開IO自動操作に使えます。色管理pipelineは含みません。

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
