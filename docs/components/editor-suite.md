# Editor Suite 2.0

[日本語](エディタスイート.md) · [Documentation index](../README.md)

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

Visible providers bound steady-frame work. Complete selected-member and neighbor queries run when needed for editing. Keep IDs stable across sorting and filtering. All keyboard presets route through host-owned Command bindings; canvas interaction stays inside the focused widget. See [API contracts](../reference/editor-api.md) for event payloads, query completeness, scratch sizes and overloads.

## Editor controls

Timeline selection keeps the selected set when dragging a selected clip; Shift adds and Ctrl toggles clips. Move/Duplicate clamp the entire set at timeline zero without changing spacing. `TimelineState::memberDrags` must hold all companion transactions, and `TimelineProvider::selected` must resolve offscreen and linked members with lock information. The host must test strict interval overlap: touching clip ends are valid.

Timeline `SelectAll` uses `editing.box` over the full Tick and track-height range; return unique editable clip IDs, including offscreen clips. Insufficient selection storage preserves the previous set. Delete and Duplicate emit complete selected-clip Begin/Commit batches when no clip key is selected. Duplicate offsets the set by its total time extent. A selected clip key takes precedence; clicking a clip body or selecting all clips clears key selection. The host owns collision policy and Undo.

The Gallery Shortcuts popup edits the same host-owned bindings used by the editor, including modifiers and unbinding, and flags shared chords. Changes last for the Gallery session; persistence belongs to the consuming application. Presets are starter maps, not complete replicas of the named applications.

Core supplies canvas pan/zoom/fit, box/lasso selection, time ruler and editable ranges, marker edits, transport, multi-key curves and Bezier handles, property states and array reorder, numeric copy/paste, and filtered/renamable asset grid/list views.

Timeline places Fit, zoom-out, a logarithmic zoom bar and zoom-in at the lower right. Ctrl+wheel zooms around the pointer, middle drag or Hand pans, and the overview range moves or resizes the visible interval. Hosts can restrict the visible tool set and zoom bounds through `TimelineState::options` while retaining existing defaults.

`TimelineProvider::externalDrops` accepts multiple host-defined ImGui payload types. The payload buffer is borrowed for the duration of the callback; a delivery flag distinguishes candidate previews from the single committed drop. `drawClipOverlay` adds application-defined visual decorations inside clips without transferring hit-testing or edit ownership from the Timeline. An optional route preview returns precise candidate ranges, track kinds and borrowed labels; the Timeline renders candidate shapes rather than row-wide bars, and the host retains responsibility for applying the same planning rules on commit.

Video provides variable-height role tracks, bounded source/target controls, linked clip move/trim, split/ripple/roll/slip/slide/ripple-delete, snap targets, transition overlap/type/duration, captions, and property keys. Monitor overlays use host textures. PCM bucket caching, envelope/mixer controls, and CPU RGB/luma/vectorscopes operate independently of the playback engine. Three-way wheels and RGB CurveEditor edits apply to composite host data, and ApplyColorCurves evaluates RGB while preserving alpha.

CG supplies camera/navigation/shading/overlay modes, bounded hierarchy selection and reparent/reorder, multi-object transform gizmos with oriented affine scale and shear, component/modifier rows, graph/dope-sheet/strip views, and UV vertex/edge/face/island selection and transforms with normalized/pixel/UDIM coordinates and pin/seam overlays.

## Preview lifecycle

DrawList preview projects unowned indexed mesh spans into the host's scratch triangle buffer and depth-sorts them. Cube, Sphere, CameraPrimitive and LightPrimitive write directly to host buffers. There is no z-buffer; intersecting or cyclic faces and occluded outline edges are approximate. Both renderers use affine inverse-transpose normal transformations.

The OpenGL3 object owns its FBO, color, ID and depth textures, shaders, VAO and buffers. The host supplies GLFunctions and the active OpenGL 3.3 context for Init, Resize, Render, Pick and Shutdown. Call Shutdown before destroying that context; destructors issue no GL calls. Resize replaces the texture ID, and Pick returns a full uint64_t ID (zero for background). Rendering alters GL bindings, viewport, and depth/cull/blend/scissor/polygon state, leaving framebuffer, program and VAO zeroed. The host restores state for subsequent passes.

## Native Gallery

Pages 7/8/9 use the public Core/Video/CG APIs and apply edits in the sample host. Context menus expose secondary actions; range endpoints and timeline track names have their own menus. Color/Inspector panes scroll when compact. The 100k toggle creates 256 tracks, 100096 clips and 100000 keys. The transition history menu demonstrates host Undo/Redo. Icons are host-uploaded atlases: 284 icons (120 stable IDs plus 164 additions).

`imkit_gallery.exe --list-monitors` lists displays; `--monitor N` places the native window on a selected display, including hidden capture runs. This is useful with DisplayLink/multiple-adapter desktops. It does not change system display settings.

![Native Video](../images/editor-video-1.0.png)
![Native CG](../images/editor-cg-1.0.png)

## Scope

The independent development node module is documented in [Node editor](node-editor.md).

This is an editor UI suite, not a media decoder/player, resampler, color-management engine, UV unwrapper, IK/simulation/animation runtime, PBR/shadow renderer or file-format loader. Native OS/IME and real-project integration are not inferred from public ImGui IO or GPU tests. See [validation](../archive/editor-validation.md).

See [Editor 2.0 migration and interaction design](../archive/editor-refresh.md).
