# Editor API Reference

[日本語](エディタAPI.md) · [English](editor-api.md)

These are custom Editor Suite APIs designed for professional creative tools, separate from the native Dear ImGui overload inventory.

- **Timeline Additions**: `FadeCurve`, `FadeView`, `EvaluateFade`, `FadePicker`, `CutTransitionView`, `TimelineEditingProvider`, `TrackAction`, `ClipboardAction`, and `TransitionShelf`. See [timeline editing contracts](../components/timeline-editing.md) for detailed fields, gestures, and host ownership.
- **Outliner Customization**: `cg::ObjectView::icon` optionally overrides the Outliner kind icon (`IconId::Count` retains the default).
- **Batch Editing**: `Event::operation` and `operationSize` describe complete terminal edit groups.
- **Commands**: `CopyClips`, `CutClips`, and `PasteClips` are bindable commands.
- **Integration**: All module targets publish C++20 and retain the host's ImGui target. Include only the modules required by the host application.

| Target | Header / Namespace | Main APIs |
|---|---|---|
| `imkit::editor_core` | `imkit/editor_core.h`<br>`imkit::editor` | `StableId`, `Tick`, `FrameRate`, `FrameToTick`, `TickToFrame`, `FormatTimecode`, `ParseTimecode`, `EventBuffer::PushBatch`, `EventBuffer`, `Transaction`, `Selection`, `ResolveSnap`, `CanvasState`, `BeginCanvas`, `CanvasSelection`, `TimeRuler`, `Transport`, `CurveEditor`, `ResolveHandles`, `MoveHandle`, `Evaluate`, `PropertyGrid`, `AssetBrowser`, `Splitter`, `StatusBar` |
| `imkit::video` | `imkit/video.h`<br>`imkit::video` | `TimelineProvider`, `TimelineState`, `Timeline`, `EditClip`, `RollClips`, `SlideClip`, `SplitClip`, `EditTransition`, `TransitionPicker`, `EvaluateEnvelope`, `Monitor`, `MonitorView`, `MonitorOptions`, `MonitorControls`, `MonitorLabels`, `BuildAudioBuckets`, `UpdateMeter`, `Waveform`, `WaveformProvider`, `WaveformQuery`, `WaveformView`, `WaveformPixel`, `DrawWaveform`, `PlacementMode`, `LevelMeter`, `AudioStrip`, `BuildScopes`, `Histogram`, `ScopeImage`, `ColorControls` |
| `imkit::cg` | `imkit/cg.h`<br>`imkit::cg` | `Project`, `TransformDelta`, `OrientationBasis`, `BeginViewport`, `ViewportObjects`, `TransformGizmo`, `PreviewTransform`, `Outliner`, `TransformUV`, `UVEditor`, `DopeSheet`, `AnimationStrips` |
| `imkit::cg` | `imkit/preview.h`<br>`imkit::preview` | `Vertex`, `Mesh`, `Triangle`, `Cube`, `Sphere`, `DrawListPreview` |
| `imkit::preview_opengl3` | `imkit/preview.h`<br>`imkit::preview` | `GLFunctions`, `OpenGL3Renderer::Init/Resize/Render/Pick/Shutdown/Texture` |
| `imkit::editor_suite` | Module headers above | Interface aggregate: `editor_core` + `video` + `cg` (excludes `preview_opengl3`) |

**Dependency Graph**:
- `editor_core` → `imkit`
- `video` → `editor_core`
- `cg` → `editor_core`
- `preview_opengl3` → `cg`

No editor target fetches external editor, plot, gizmo libraries, or GL loaders. Source and installed targets share identical exported names.

---

## Provider and event use

```cpp
std::array<imkit::editor::Event, 128> eventStorage;
imkit::editor::EventBuffer events{eventStorage};
std::array<imkit::editor::StableId, 128> selectionStorage;
imkit::editor::Selection selection{selectionStorage};
imkit::video::TimelineState ui; // Retained by the host between frames.

// During a normal host-owned ImGui frame/window:
imkit::video::Timeline("timeline", provider, ui, selection, events, theme);
for (const auto &event : events.Events()) {
    // Host validates revision, previews drafts, applies Commit, and records Undo.
    DispatchToHost(event);
}
events.Clear();
```

### Event and State Contracts

- **Host Ownership**: `provider`, `theme`, and `DispatchToHost` in this snippet are supplied by the host.
- **Timestamp & ID Precision**: `Value.first`, `last`, and `offset` retain exact integer timestamps (`Tick`). `parent` retains a 64-bit non-zero `StableId`.
- **Numeric Channels**: `x`, `y`, `z`, and `w` encode numeric channels:
  - **Clip values**: `{start, end, sourceIn, trackId, speed}`
  - **Property values**: `x`
  - **Transform values**: `x / y / z`
  - **Curve keys**: `first` (tick) and `x` (value)
  - **Curve handles**: `x / y` (relative seconds / value)
  - **Reparenting**: `parent`
- **Text Renaming**: Rename events return original and proposed UTF-8 bytes in fixed 256-byte arrays (including null terminator). Longer labels require a host-side rename workflow.
- **Fixed Capacities & Overflow**: Selection storage and transaction member storage have explicit fixed capacities provided by the host. The host must size event buffers to accommodate all phases in a single frame. When capacity is exceeded, an overflow flag is raised rather than performing dynamic heap allocation.
- **Gallery Scope**: Gallery applies only the subset documented in [Editor Suite](../components/editor-suite.md).

---

## GL pass contract

### Camera Navigation API

| API | Contract |
|---|---|
| `NavigateCamera(Camera&, Point orbitPixels, Point panPixels, double wheel, double viewportHeight)` | Pure camera navigation, view-relative pan, and orthographic extent zoom based on viewport scale. |
| `AlignCamera(Camera&, Axis, bool negative = false)` | Aligns camera to 6 orthographic cardinal views (X / Y / Z positive and negative). |
| `ViewportState::cameraView` | Non-owning host camera reference, valid during `BeginViewport`. |
| `ViewportState::navigationGizmo` | Toggles the clickable view-axis navigation overlay. |

### Color Processing API

| Signature | Contract |
|---|---|
| `ScopeBuffers::redWaveform/greenWaveform/blueWaveform` | Optional three `width * 256` spans; must be provided all-or-none. |
| `ScopeImage(..., const Theme&, ImVec4 tint)` | Tinted CPU scope drawing without texture ownership. |
| `Rgba ApplyColorCurves(Rgba, const ColorCurveSet&)` | Evaluates sorted non-owning RGB key spans. Normalized input maps to `0..TicksPerSecond`; empty channels evaluate as identity; output is clamped and alpha is preserved. |
| `bool ColorControls(const char*, ColorValues&)` | Edits host draft values directly; compatible with legacy signature. |
| `void ColorControls(const char*, const ColorValues&, const ColorPropertyIds&, uint64_t, ColorState&, EventBuffer&, const ColorLabels& = {})` | Uses immutable source data with typed RGB/scalar transactions and six independent property IDs. |

Both `ColorControls` overloads render draggable three-way wheels and level sliders. `ColorState` records current wheel centers and radii for host overlays and public-IO automation. Neither overload provides a color management pipeline.

Gallery Color uses the common `CurveEditor` with independent host-owned RGB keys, including interpolation and handles. Commit regenerates synthetic ramp images and scope bins; Cancel preserves existing buffers. This demonstrates curve evaluation on synthetic pixels without a media decode or color engine.

### Audio API

| API | Contract |
|---|---|
| `TrackControl` | Stable Toggle field selector in event `x`; boolean state in `y`. |
| `AudioStripView::gainId, panId` | Explicit property IDs; zero indicates disabled; no arithmetic ID inference. |
| `AudioStripView::locked` and label pointers | Host restriction flags and borrowed UTF-8 label pointers. |
| `AudioStrip(...)` | Native gain fader and pan transactions; mute/solo/record state events. |
| `ClipView::audioBuckets` | Non-owning PCM min/max buckets rendered inside clips. |

### Track Layout API

| API | Contract |
|---|---|
| `float TrackExtent(const TrackView&)` | Computes actual row height including expanded and collapsed states. |
| `TrackLayout { tracks, top }` | Contiguous non-owning visible rows and absolute first-row pixel offset. |
| `TimelineProvider::layout`, `totalHeight` | Optional indexed pixel-range query; uniform rows remain fully supported. |
| `TimelineState::heightDrag` | Host-owned track-height transaction state. |
| `EditKind::TrackHeight` | Event `x` carries original and proposed height; `target` is track ID. |

### OpenGL 3.3 Renderer Lifecycle

1. Populate every function pointer from the current OpenGL 3.3 context into `GLFunctions`.
2. Initialize `OpenGL3Renderer` and render non-owning mesh views.
3. Pass `Texture()` to the host ImGui UI (use vertically flipped UVs for FBO images).
4. `Pick(x, y)` accepts top-left coordinates and returns a 64-bit stable ID (0 for background).
5. Always call `Shutdown()` while the original OpenGL context is active. Destructors issue no GL calls.
6. Restoring GL pipeline states and managing multi-thread synchronization remain host responsibilities. See `examples/gallery/main.cpp` for a concrete implementation.

### Compile Fixtures

The compile fixture `tests/editor_api_compile.cpp` links all public targets. External consumers build this fixture against an independently owned Dear ImGui target to verify source integration.

### Transport Controls

- **Atlas Icons**: `Transport(state, bindings, const IconAtlas*)` uses host-owned icon textures for play, pause, stop, and single-frame stepping. A null atlas falls back to native text buttons.
- **J/K/L Bindings**: Transport uses `PlayReverse`, `Pause`, and `PlayForward` bindings:
  - `K` holds the current playhead.
  - `Stop` rewinds to In point.
  - Empty bindings disable keyboard playback.
  - The host can remap every transport command, including Loop.

### Timeline Snapping and Navigation

- `ResolveTimelineSnap` resolves moving clip edges, ignores moving IDs, and applies active `snapKinds`.
  - `snapping` acts as the master switch.
  - `magnet` controls provider snap targets.
  - `snapToFrame` enables frame grid attraction within 8 screen pixels.
  - `SnapResult.tick` is the primary anchor; `candidate.tick` is the guide position.
  - Optional `TimelineState.icons` atlas remains host-owned.
- **Follow Playhead**: `FollowPlayhead` provides Off, Smooth, and Page modes:
  - `Smooth` maintains a 10% screen margin in both directions.
  - `Page` advances by full visible widths and supports reverse playback.
  - Controlled via `TimelineState.autoScroll`.
- **TimeRuler**: Draws frame-aligned major and minor ticks with density derived from horizontal zoom. Playback following updates the shared canvas before ruler and clip drawing.
- **Timeline Fit**: Fits `TimelineProvider.contentRange` with 24 logical pixels of horizontal padding while preserving vertical scale and scroll.
- **Timeline Options**: `TimelineState::options` controls the visible tool mask, minimum/maximum pixels per second, overview range, and lower-right Fit/zoom controls. The zoom bar maps logarithmically.

### Property Controls & CG Inspector

- **Property Actions**: Labels expose Favorite, Locked, and Override context actions even when values are locked. Toggle events identify `PropertyFlags` bits in `Value.x` and requested states in `Value.y`.
- **Property Key Events**: `EditKind::PropertyKey` is distinct from curve-key moves: `target` is property ID, proposed `first` is `PropertyState.time`, `offset` is `PropertyKeyAction`, and `x` is the value.
- **Asset Breadcrumbs**: Emit `Navigate` events with `AssetState.breadcrumbIds` target and breadcrumb index in `proposed.first`.
- **CG Inspector Components**: Exposes all 9 translation, rotation (radians), and scale components. Reset restores translation/rotation to 0 and scale to 1. Each component has an explicit host property ID.
- **Mixed Values**: Multi-object selection displays `Mixed` when values differ. Locked properties reject edits across the selection.

### Curve Editor

- **Visible Queries**: Curve queries use a channel/time index and return contiguous visible keys plus two boundary neighbors for tangent calculation.
- **Context Menus**: Right-clicking keys opens interpolation and handle-mode menus (`KeyInterpolation` / `KeyHandleMode`).
- **Channel Activation**: `CurveState.activeChannel` follows clicked keys. Inactive channels draw as faint dashed lines when `ghostOtherChannels` is enabled.
- **Channel Evaluation**: Optional `CurveProvider.sample` evaluates complete channels at exact ticks with Constant, Linear, or Repeat extrapolation.
- **Fit & Zoom**: Fits `CurveProvider.bounds` in seconds/negative-value coordinates.
- **Preview & Duplication**: `CurveState.previewKeys` renders drag proposals before commit. Alt-dragging keys emits `Duplicate` transactions with new `StableId`s.
- **Frame Snapping & Time Scaling**: `snapToFrame` snaps the primary key and applies the delta to companion keys. `scaleTime` scales distances around the earliest selected tick (100 horizontal pixels doubles timing).
- **Batch Deletion & Selection**: Delete emits one `Remove` commit per selected key. Box and lasso selection use `CanvasSelection`.

### Animation Strips and UV Editor

- **Strip Settings**: `EditKind::StripSettings` carries range in `first / last`, channel in `parent`, scale/repeat/blend in `x / y / z`, and mute/lock bits in `offset`.
- **Strip Reorder**: Emits neighboring strip `StableId` in `proposed.parent` and direction in `proposed.offset`.
- **UV Selection Modes**: Supports Select All, Box, and Lasso selection across Vertex, Edge, Face, and Island modes.
- **Multi-Vertex Transforms**: `UVProvider::selected` returns complete selections and `UVState::companionDrags` holds vertex transactions.
- **Coordinate Conventions**: UV geometry stays normalized. Set `UVState::imageSize` for pixel tooltips. UDIM labels use `1001 + u + 10v` for rows/columns 0..9.
- **Topology Handling**: `UVEdge` links edge previews to endpoint vertex proposals. `UVFace` references ordered vertex spans; `overlap` is host-computed and displayed as an outline.

---

## Timeline and monitor additions

### Timeline & Monitor API Inventory

| API | Host Contract |
|---|---|
| `ClipView::keyChannel`, `keyDefaultValue` | Explicit insertion channel and empty-channel default; never inferred from adjacent IDs. |
| `ClipView::envelope`, `EnvelopePoint` | Borrowed time-sorted local-tick points with `StableId`, gain, and lock status. |
| `EvaluateEnvelope(points, tick)` | Evaluates linear gain; constant at endpoints; evaluates to 1.0 when empty. |
| `EditTransition(clip, end, delta)` | Pure duration edit; preserves the opposite endpoint and clamps within clip duration. |
| `TransitionPicker(id, clip, revision, events, trackLocked)` | Atomic Begin/Commit type selection; host applies resulting transition kinds. |
| `MonitorOptions::transformBounds`, `anchor` | Host-provided normalized display rectangle and anchor point; independent of texture flipping. |
| `MonitorOptions::metadataPreset` | `Off`, `Clip`, or `Details`; state selection is host-owned. |
| `MonitorOptions::clipName`, `markerComment`, `metadata` | UTF-8 strings borrowed for the call; clipped to available overlay space. |
| `EditorPalette::effectClip`, `adjustmentClip`, `groupClip` | Semantic colors used by corresponding Timeline track roles. |

### Timeline Interaction Details

- **Key Insertion**: `AddKey` emits `KeyInsert` with `target=channel`, `parent=clip`, `first=local tick`, and `x=value`. Key navigation emits `Navigate` with `target=key`, `parent=clip`, and `first=destination tick`.
- **Audio Envelope Events**: Emits `AudioEnvelope` with `target=point` (for edit/remove) or `target=clip` (for insert), `parent=clip`, `first=local tick`, `x=gain`, and `offset` (0=edit, 1=insert, 2=remove).
- **Track Labels**: `TimelineState::trackLabels` (`video::TrackLabels`) provides borrowed UTF-8 strings for 7 track buttons, tooltips, compact menus, and height controls. Defaults are in English.
- **Preflight Editing Check**: `TimelineProvider::canBeginEdit(user, clip, kind)` is an optional host preflight for clip-body tools and Razor. Returning false suppresses edit start without reporting buffer shortage.
- **Key Evaluation Fallback**: `ClipView::keyEvaluation` optionally borrows sorted local-time channels. When unset, falls back to `keys`, then `keyDefaultValue`.
- **Role Tooltips**: `TrackLabels::kinds` supplies 6 UTF-8 role tooltips in `TrackKind` order for track headings.
- **External Drag & Drop**: `TimelineProvider::externalDrops` provides borrowed payload routes for preview and single-delivery callbacks. `drawClipOverlay` allows custom clip decorations without replacing Timeline hit-testing.
- **Editing Tool Commands**: `ToolSelect`, `ToolRazor`, `ToolRipple`, `ToolRoll`, `ToolSlip`, `ToolSlide`, and `ToolHand` are appended command IDs (mapped by default to V/C/B/N/Y/U/H).
- **Edit Validation Callback**: `TimelineProvider::isEditable(user, clip)` validates clip existence and lock state during active gestures. A false return cancels the active batch.
- **Clip Linking and Grouping**: Context menu supports detaching or grouping clips:
  - `EditKind::Link` emits Begin/Commit with `target=clip ID`, `offset` (0=linked, 1=group, 2=new link set, 3=new group set), and parent IDs.

### CG Viewport and Overlays

- **Shear & Affine Transforms**: `cg::Transform::shear` stores upper-triangular xy, xz, and yz terms. `LinearBasis` returns world-space columns; `NormalBasis` returns the inverse transpose. Non-uniform scaling preserves affine transformations via QR decomposition. Scale events carry `hasAffine=true` with full rotation and shear terms.
- **Mesh Normals Overlay**: `preview::DrawMeshNormals` draws borrowed vertex and face normals over either preview path using `NormalOverlayOptions`. Length is in world units; vertex directions use inverse-transpose transforms.
- **Viewport Labels**: `cg::ViewportState::labels` accepts borrowed UTF-8 strings for tools, projection, orientation, pivot, shading, overlays, and navigation hints.
- **Framing & Safe Areas**: `ViewportState` exposes camera frames, safe frames (5% and 10% insets), passepartout, and measurement overlays submitted by `ViewportObjects`.
- **Mesh Outline**: `preview::DrawMeshOutline` draws open boundaries and silhouette edges from indexed triangles. The host supplies `OutlineEdge` scratch storage.
- **Camera & Light Primitives**: `preview::CameraPrimitive` (frustum) and `LightPrimitive` (octahedron) write helper geometry directly to host vertex/index buffers.
- **Related Ripple, Roll & Slide**: Ripple, Roll, and Slide edits resolve linked/grouped clips through `selected` and `MemberDrag`, verifying lock states before Begin and applying common deltas atomically.
- **Camera Pose derivation**: `CameraFromTransform` derives camera orientations from local +Z forward / +Y up, preserving lens settings and ignoring scale/shear.
- **Typed Outliner Rows**: `ObjectView::kind` distinguishes Object, Collection, Mesh, Camera, Light, Component, and Modifier rows. `OutlinerState::kindFilter` filters rows alongside text search.

### Monitor Display Component

- **Monitor Controls**: `MonitorControls(id, options, atlas, labels)` edits host-owned display options for safe areas, guides, timecode, transform bounds, and metadata presets.
- **Monitor View Overload**: `Monitor(id, MonitorView, ...)` displays host-updated `ImageView` and `PreviewState`:
  - `Fit`: Preserves source aspect ratio.
  - `Fill`: Centers and crops UVs.
  - `Stretch`: Permits anisotropic scaling.
  - Loading, empty, offline, and error states reuse standardized components (`Spinner`, `Skeleton`, state views).
- **Separation of Concerns**: ImKit manages layout, DrawList composition, overlays, and typed input requests. Textures, decoding, rendering, capture, device management, and persistence remain strictly host-owned.

---

## Range, marker and property array controls

### Time Ruler & Range Controls

- **Resource Borrowing**: `TimeState::rulerLabels` and `icons` borrow host display resources.
- **Intervals**: `TimeRuler` renders work areas and In/Out ranges. Endpoint dragging updates host navigation bounds; Escape cancels the drag and restores starting ranges.
- **Context Menus**: Right-clicking allows editing endpoints numerically in seconds, jumping to start/end, and clearing ranges.

### Markers

- **Marker Gestures**: Dragging markers emits `Begin / Update / Commit / Cancel` events with explicit marker IDs.
- **Context Actions**: Removal emits `Remove` events. Adding markers uses target ID zero, requesting the host to allocate a new `StableId`.

### Property Arrays

- **Reordering**: `PropertyView::arrayElement` enables reordering context actions when `PropertyProvider::neighbor(user, id, -1 / +1)` supplies an unlocked sibling.
- **Payload**: Reorder events carry source ID and target sibling ID in `proposed.parent`.
- **Display**: `PropertyView::icon` provides semantic glyphs; `PropertyLabels::mixed / moveUp / moveDown` supply UTF-8 labels. Mixed text is displayed verbatim without numeric format string interpretation.

### Ripple Delete

- `RippleDeletePosition` computes surviving clip positions from sorted, non-overlapping removed clips, checking for interval overlap and tick overflow.
- Timeline emits a reserved batch for related-clip ripple deletion. The host verifies locked followers before deleting clips and shifting survivors.

### Captions & Component Hierarchy

- **Captions**: Context menu insertion emits `CaptionInsert`. Gallery creates a 3-second synthetic caption in the first available slot at or after the playhead.
- **Component Icons**: `ComponentView::icon` and `ComponentTypeView::icon` allow the host to distinguish component and modifier rows using custom atlas glyphs.
- **Clipboard Integration**: Property context menus support Copy and Paste through host ImGui clipboard callbacks, parsing finite numbers into typed `Property` events.

For architecture context and migration notes, see [Editor Refresh Design Notes](../archive/editor-refresh.md).
