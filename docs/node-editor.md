# Node editor (development API)

The independent `imkit::node_editor` target provides native Dear ImGui node editing,
context-free layout utilities and host-driven previews. It is included by
`imkit::editor_suite`, or can be linked on its own. Include `<imkit/node_editor.h>`.
The baseline remains Dear ImGui 1.92.9b docking / C++20.

This is an original implementation. `imgui-node-editor` is an API/interaction
reference, not a source, build or runtime dependency. No third-party assets were
added. The standalone demo optionally loads the installed Windows Segoe UI font;
the font is neither redistributed nor loaded by the library.

## First frame

```cpp
namespace ne = imkit::node_editor;
// Persistent, explicitly host-owned. Reserve is optional, not a node limit.
ne::EditorState editor;
editor.Reserve(1024);
std::vector<ne::EditRequest> storage(4096); // Keep large buffers off the stack.

// Each frame: build graph from your model; views/labels stay alive through draw.
ne::RequestBuffer requests{storage};
auto style = ne::MakeNodeStyle(theme);
auto frame = ne::BeginEditor("graph", graph, editor, requests, style);
ne::DrawLinks(frame);
for (const auto& node : graph.nodes) {
    if (ne::BeginNode(frame, node.id)) {
        // Ordinary native ImGui controls, using the available content width.
        // Supply body/Preview only when !node.collapsed and
        // editor.zoom >= style.detailZoom (DrawNodes does this for you).
        DrawMyNodeProperties(node.id);
        ne::EndNode(frame);
    }
}
ne::NodePalette(frame, availableNodeTypes);
ne::EndEditor(frame); // Always paired, even for a clipped/invalid graph.
ApplyMyCompleteTransactions(requests.Requests());
```

`BeginNode`/`EndNode` and `BeginPin`/`EndPin` take the explicit frame; there is no
global current editor. Nodes and sockets have strongly typed 64-bit IDs. Zero is
reserved; IDs must be unique within the current graph. Positions and sizes are in
graph units. Socket offset is measured along its selected node edge.

The graph data is borrowed and immutable for the entire draw, including detached
previews and inspector panels. `EditorState` owns only UI state/scratch, never your
graph. Do not reallocate graph spans from a drawing callback. Do not copy an active
frame or nest node scopes. Each simultaneous view/context needs its own state.

## Layout without an ImGui context

```cpp
std::vector<ne::PositionChange> changes(graph.nodes.size());
ne::AlignOptions options;
options.alignment = ne::Alignment::Left;
options.reference = ne::AlignReference::Node;
options.anchor = lastSelected;
auto result = ne::AlignNodes(graph, selectedIds, changes, options);
if (result) {
    ne::QueueLayout(graph, std::span(changes).first(result.count), requests,
                    editor.nextOperation++);
}
```

| API | Behavior |
|---|---|
| `AlignNodes` | Left/center/right/top/center/bottom; selection bounds, anchor node or explicit rectangle |
| `DistributeNodes` | Horizontal/vertical centers or edge gaps; locked nodes partition stationary intervals |
| `ArrangeNodes` | Connection-directed layers, horizontal/vertical, deterministic cycle remainder |
| `SnapNodesToGrid` | Explicit position quantization; never automatically rearranges the graph |
| `FitGroupToContents` | Returns container bounds including padding; host applies position/size together |
| `FrameNodes` | Changes only the view; an empty selection frames the graph |
| `TraceNodes` | Cycle-safe upstream/downstream selection candidates |

Layout returns changed positions only. Capacity, invalid geometry, duplicate IDs,
missing selections and invalid containment fail without partially writing output.
Container movement preserves the offsets of unlocked descendants. A descendant
selected with its ancestor is not moved twice. Locked subtrees are stationary.
Arrange partitions visual parents and avoids stationary sibling rectangles; it is
a practical layered layout, not a general constraint solver or an optimal router.
Center/gap distribution retains the end nodes. Negative gaps are possible when
the bounding interval is smaller than the combined node sizes.

## Editing and host integration

`RequestBuffer` emits complete operation batches. Consume `operationSize` records
together; check `graph` and `revision`, then apply or reject the complete operation.
Apply a multi-node move as one undo entry. A drag emits Begin/Update/Commit/Cancel;
Begin/Update are draft observations, not changes to the persistent model revision.
The editor draws the drag draft itself. Update the model and revision on Commit.
Cancel restores the draft; capacity failures retain pending terminal gestures for
retry after the host drains/enlarges the buffer. Single-shot operations return
failure / set `overflow` rather than silently applying a prefix.

The library does not own undo, clipboard bytes, file persistence, node evaluation,
workers, textures, contexts or backend initialization. `CaptureView`/`RestoreView`
return/accept value state; serialization is the host's job. `canConnect` decides
type conversions, same-node links and graph cycles. The UI checks socket direction,
existence, locks, duplicate links and connection capacity first. Use `multiple=true`
on output sockets intended to fan out. No callback means matching nonzero types
are allowed; zero is an unspecified type. Cycles are not forbidden by default.

Group/frame membership (`parent`) is distinct from subgraph identity (`childGraph`).
The host supplies one graph at a time and switches after EnterGraph requests;
camera and selection are remembered by graph. Exposed sockets use `internal` for
correspondence. Group/unpack/expose/template commands are requests, not an imposed
graph schema. The example implements group/subgraph boundaries, unpack, independent
template copies, duplicate/delete and undo using an ordinary host model.

`InsertNode` finds compatible node input/output sockets and requests replacing one
wire with two in one operation. `from`/`to` in that request identify the inserted
node's input/output, and `link` identifies the original wire. Copy, cut, paste,
duplicate and grouping requests contain selected node IDs. `ReorderPin` uses a
zero-based per-node socket order; HidePin uses index 1 for hidden, 0 for visible.
ExposeProperty uses its property ID in `type` and index 1/0 for exposed/hidden.
Rename text is owned by the request (127 UTF-8 bytes plus terminator).

## Preview, customization and interaction

Every node can call `Preview` below its properties. Outputs may contain a borrowed
texture, or a custom callback for text/tables/waves/3D results. The callback receives
a size and an interactive flag. It must fit that region and respect the flag.
Results are supplied by the host, not computed by the editor. Missing, waiting,
stale and error states are explicit. Call `DrawDetachedPreviews` for every relevant
node after the editor, including offscreen nodes, to maintain pinned/expanded views.
The texture must remain valid until the host renderer consumes the draw data.

Preview demand reports UI pixel size and visibility; absence of a demand in a frame
means that output is not requested. The host decides rendering resolution, DPI
conversion, update rate and scheduling. Comparing two outputs does not perform
image processing. Body sizing is explicit: choose a node size that fits your native
widgets; low-detail rendering does not provide a second property editor.

MakeNodeStyle derives semantic colors, density, corner/stroke metrics and motion
from Theme. Edit the returned value for an editor; override `NodeView::style` for
one node, socket color/shape/side for ports, or link color/style/width per wire.
Ordinary body contents remain native ImGui widgets. Use BackgroundImage/Annotation
for host-supplied canvas decoration. Link flow is a display flag, not execution.
`NodeInspector`, `ExposedProperties`, `NodeSearch`, `Diagnostics`, `LayoutToolbar`
and `Breadcrumbs` can be placed in host panels outside the canvas.

Default interactions: header drag, middle-button pan, wheel zoom, F frame, Tab add,
Ctrl selection toggle, Shift additive selection, optional lasso, Alt-drag wire cut,
Alt-input drag reconnect. Native text editing and popups take precedence. The Keymap
is replaceable. Read-only mode disables graph edits while preserving navigation.
Run/debug controls appear only when the host advertises those capabilities.

## Development demo and checks

Build `imkit_node_editor_gallery` and run the executable from `catalog/Debug`.
`--smoke [absolute-output.png]` checks host edits/undo/subgraph unpack, renders five
native frames and optionally captures a real backbuffer. Captures belong in build
output, not source assets. Existing Gallery implementation files are unchanged.

Focused checks are `imkit.node_editor` (layout, transaction safety and native public
IO) and `imkit.node_editor_api_compile` (independent public header/link consumer).
The API inventory is [node-editor-api.json](node-editor-api.json), regenerated with
`python tools/generate_node_api.py` without rewriting existing API inventories.

This remains a design-stage API: no fixed performance acceptance threshold or
claim of exhaustive product parity. Native OS/IME, physical DPI, assistive technology,
Release performance and installed-SDK distribution are not established by these
development checks. [日本語](node-editor.ja.md)
