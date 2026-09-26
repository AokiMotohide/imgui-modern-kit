# Editor Suite 2.0

[日本語](editor-suite.ja.md)

[Timeline editing](timeline-editing.md) documents independent fades, cut transitions, track management and selection/clipboard operations.

ImKit provides reusable C++20 editor controls on the pinned Dear ImGui 1.93.0 WIP docking commit.

## Modules

| Target | Header | Role |
|---|---|---|
| `imkit::imkit` | `imkit/imkit.h` | Native wrappers, Precision Layers theme, icons |
| `imkit::editor_core` | `imkit/editor_core.h` | Canvas, time, curves, property/asset controls |
| `imkit::video` | `imkit/video.h` | Timeline, monitors, audio, color |
| `imkit::cg` | `imkit/cg.h`, `imkit/preview.h` | Viewport, hierarchy, animation, UV, DrawList preview |
| `imkit::editor_suite` | Module headers above | Core + Video + CG, without mandatory OpenGL |
| `imkit::preview_opengl3` | `imkit/preview.h` | Optional host-context renderer |

## Ownership and events

The host owns the Context, backend, fonts, textures, source data, providers, UI state, selection, undo, persistence and workers. Spans and UTF-8 label pointers are borrowed for each call. StableId is a unique nonzero uint64_t; times use int64_t Tick at 705600000 ticks/second with a rational FrameRate, including negative pre-roll and 29.97/59.94 drop-frame timecode. Parsing accepts two-digit hour fields.

Preview components own layout, overlays, input UI and requests only. Image/video textures, media decode/capture, CG rendering and FBO resize, frame clocks, devices, native windows, workers and persistence remain host-owned. The Video Monitor displays the texture and state supplied for the current frame; it does not play or seek media.

Continuous model edits emit Begin/Update/Commit/Cancel with revision, original and proposed values. Apply preview separately, increment revision on accepted commits or external changes, and cancel stale gestures. A full event or scratch buffer sets overflow. Terminal intent is retained for retry; related edits reserve the complete batch and reject locked members together. Instant context actions carry typed Commit events. Navigation ranges and display options are mutable host UI state.

Visible providers bound steady-frame work. Complete selected-member and neighbor queries run when needed for editing. Keep IDs stable across sorting and filtering. All keyboard presets route through host-owned Command bindings; canvas interaction stays inside the focused widget. See [API contracts](editor-api.md) for event payloads, query completeness, scratch sizes and overloads.

## Editor controls

Timeline selection keeps the selected set when dragging a selected clip; Shift adds and Ctrl toggles clips. Move/Duplicate clamp the entire set at timeline zero without changing spacing. `TimelineState::memberDrags` must hold all companion transactions, and `TimelineProvider::selected` must resolve offscreen and linked members with lock information. The host must test strict interval overlap: touching clip ends are valid.

Timeline `SelectAll` uses `editing.box` over the full Tick and track-height range; return unique editable clip IDs, including offscreen clips. Insufficient selection storage preserves the previous set. Delete and Duplicate emit complete selected-clip Begin/Commit batches when no clip key is selected. Duplicate offsets the set by its total time extent. A selected clip key takes precedence; clicking a clip body or selecting all clips clears key selection. The host owns collision policy and Undo.

The Gallery Shortcuts popup edits the same host-owned bindings used by the editor, including modifiers and unbinding, and flags shared chords. Changes last for the Gallery session; persistence belongs to the consuming application. Presets are starter maps, not complete replicas of the named applications.

Core supplies canvas pan/zoom/fit, box/lasso selection, time ruler and editable ranges, marker edits, transport, multi-key curves and Bezier handles, property states and array reorder, numeric copy/paste, and filtered/renamable asset grid/list views.

Timeline places Fit, zoom-out, a logarithmic zoom bar and zoom-in at the lower right. Ctrl+wheel zooms around the pointer, middle drag or Hand pans, and the overview range moves or resizes the visible interval. Hosts can restrict the visible tool set and zoom bounds through `TimelineState::options` while retaining existing defaults.

`TimelineProvider::externalDrops` accepts multiple host-defined ImGui payload types. Payload memory is borrowed only for the callback, and the delivery flag distinguishes preview from the single accepted drop. `drawClipOverlay` adds application decoration inside clip bounds; it does not transfer hit testing or edit ownership from Timeline. An optional route preview returns the exact candidate range, track kind and a borrowed label. Timeline then draws a clip-shaped target instead of the native whole-row target; the host remains responsible for using the same planning rules at delivery.

Video supplies variable-height role tracks, restrictions/source/target controls, related clip moves and trims, split/ripple/roll/slip/slide/ripple-delete, snap targets, transition overlap/type/duration, captions and property keys. Monitor overlays use host textures. PCM buckets, envelope/mixer controls and CPU RGB/luma/vector scopes are separate from playback. Three-way wheels and RGB CurveEditor edits apply to synthetic host data; ApplyColorCurves evaluates normalized RGB and preserves alpha.

CG supplies camera/navigation/shading/overlays, restricted hierarchy selection and reparent/reorder, oriented multi-object gizmos with explicit affine scale/shear, component/modifier rows, graph/dope-sheet/strip controls, and UV vertex/edge/face/island selection and transforms with normalized/pixel/UDIM coordinates and pin/seam overlays.

## Preview lifecycle

DrawList preview projects non-owning indexed mesh spans into host triangle scratch, then depth-sorts them. Cube, Sphere, CameraPrimitive and LightPrimitive fill host buffers. It has no z-buffer; intersecting/cyclic surfaces and occluded outline edges can be approximate. Both renderers use the affine inverse-transpose normal transform.

The OpenGL3 object owns FBO/color/ID/depth textures, shaders, VAO and buffers. The host supplies GLFunctions and a current OpenGL 3.3 context for Init/Resize/Render/Pick/Shutdown. Call Shutdown before destroying that context; the destructor makes no GL calls. Resize replaces texture IDs. Pick returns the full uint64_t ID, zero for the background. Rendering changes GL bindings, viewport, depth/cull/blend/scissor/polygon state and ends with framebuffer/program/VAO zero. The host restores state for later passes.

## Native Gallery

Pages 7/8/9 use the public Core/Video/CG APIs and apply edits in the sample host. Context menus expose secondary actions; range endpoints and timeline track names have their own menus. Color/Inspector panes scroll when compact. The 100k toggle creates 256 tracks, 100096 clips and 100000 keys. The transition history menu demonstrates host Undo/Redo. Icons are host-uploaded atlases: 284 icons (120 stable IDs plus 164 additions).

`imkit_gallery.exe --list-monitors` lists displays; `--monitor N` places the native window on a selected display, including hidden capture runs. This is useful with DisplayLink/multiple-adapter desktops. It does not change system display settings.

![Native Video](images/editor-video-1.0.png)
![Native CG](images/editor-cg-1.0.png)

## Scope

The independent development node module is documented in [Node editor](node-editor.md).

This is an editor UI suite, not a media decoder/player, resampler, color-management engine, UV unwrapper, IK/simulation/animation runtime, PBR/shadow renderer or file-format loader. Native OS/IME and real-project integration are not inferred from public ImGui IO or GPU tests. See [validation](editor-validation.md).

See [Editor 2.0 migration and interaction design](editor-refresh.md).
