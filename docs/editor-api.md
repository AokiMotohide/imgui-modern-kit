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
