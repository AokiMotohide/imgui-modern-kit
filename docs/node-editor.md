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
    ne::DrawLinks(frame); // schedule links after row layout
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


## Dynamic sockets and standard rows

The development API now exposes `CreatePin`, `DeletePin`, `RenamePin`,
`ChangePinType`, `ReorderPin`, `SetPinMultiplicity`, `SetPinLimits` and
`SetPinValue` requests. The host allocates stable IDs, retains graph data and
applies complete operations against the supplied revision. `QueuePinEdits`
assigns one operation ID and size to a batch; insufficient buffer capacity writes
nothing. The host must validate the resulting group counts and every affected
link, then accept the complete operation or reject it without changes. In
particular, deletion/type changes do not implicitly delete links. `affectedLinks`
is advisory; the host examines its current model. One accepted operation becomes
one Undo record. Begin/Update values are temporary UI drafts; apply only Commit
and discard Cancel, or maintain a separate host preview transaction.

```cpp
namespace ne = imkit::node_editor;
ne::SocketTypeView types[] = {
    {1, "Scalar", "Value", {.6f, .7f, .8f, 1}, ne::PinShape::Circle},
    {2, "Color", "Value", {.9f, .7f, .2f, 1}, ne::PinShape::Square}
};
graph.socketTypes = types; // borrowed through EndEditor / inspector calls
graph.typeCompatibility = [](void*, uint64_t out, uint64_t in) {
    return out == in ? ne::ConnectionVerdict{} :
        ne::ConnectionVerdict{ne::ConnectionMatch::Convertible, "Host conversion"};
};
node.inputs.add = true; // enable only capabilities supported by the host
pin.capabilities = {true, true, true, true, true, true};
pin.manualPosition = false;
ne::EditRequest create;
create.kind = ne::EditKind::CreatePin;
create.node = node.id;
create.pinKind = ne::PinKind::Input;
create.index = 0;
create.type = 1;
create.text[0] = 'X';
ne::QueuePinEdits(graph, {&create, 1}, requests, operationId);
```

`SocketTypeView` has numeric stable type ID, display name, semantic category,
color and shape. The library defines no material or geometry type enum.
`typeCompatibility` receives normalized output/input type IDs. `canConnect`, when
provided, takes precedence and can also enforce cycles or application semantics.
`Exact`, `Convertible` and `Rejected` include a borrowed reason. Structural
availability, duplicate wiring and multiplicity are checked before callbacks.
`canEditPin` optionally supplies a host preflight reason; final validation is
always the host's responsibility.

```cpp
ne::DrawLinks(frame); // schedule links after row layout
if (ne::BeginNode(frame, node.id)) {
    ne::PinRowOptions row;
    row.value.kind = ne::ValueKind::Float;
    row.value.number[0] = hostValue;
    row.minimum = 0; row.maximum = 1;
    ne::PinRow(frame, pin.id, row);
    ne::PinAddRow(frame, node.id, ne::PinKind::Input);
    ne::EndNode(frame);
}
ne::EndEditor(frame); // draws links using this frame's measured row positions
```

Standard rows align labels, values and sockets, with inputs on the left and
outputs on the right. `PinRowOptions` supports Color, Float, Integer, Boolean,
Vector, Enum, Text and a custom native-widget callback. Values are owned copies
in requests, never pointers into host storage. Unconnected inputs alone are
editable. Connected values are hidden by default, or disabled with
`hideConnectedValue=false`. Low zoom hides fine value editors but retains rows and
socket structure. Submit rows even at low zoom. `GetPinPosition` exposes the same
screen position used for socket drawing and links after row submission.

Use `BeginPin`/`EndPin` with manual edge/offset for specialized content. Use
`PinRow` with `manualPosition=false` for regular editing. `DrawLinks` is now a
compatibility scheduling call; `EndEditor` draws graph links once after row
submission. Explicit `Link` calls remain immediate and should follow the relevant
rows. Views, labels, type metadata and callback storage must remain alive until
all editor and Inspector calls finish.

For variadic inputs, set a nonzero `capabilities.group`, minimum and maximum,
and corresponding node input capabilities. Submit `PinAddRow` with that group.
Rows support drag reordering within the same graph/node/direction/group and a
context menu for deletion and metadata editing. Inspector splits Inputs/Outputs,
provides creation, naming, type, visibility, multiplicity, exposure and group
limits. `confirmPinImpact` controls the short linked-socket confirmation. The
host can still reject confirmed edits.

Dragging an occupied input rewires its existing link. Rejected connections leave
the original link untouched. Compatible targets receive semantic feedback rings;
convertible targets have an extra mark. Palette entries need a `compatible`
callback to appear during connection-drop filtering. A CreateNode request includes
its source pin and replacement link; the host creates and auto-connects atomically.
`InsertNode` identifies the candidate input/output and existing link for atomic
host application.

## Material Graph Mock

The independent Gallery defaults to Material Graph Mock. Its host implementation
is `examples/node_editor/material_mock.h`; the previous studio page remains
available through the page checkbox. Twelve node templates include image/color/
scalar/normal sources, three BRDF examples, Principled, Mix/Add Closure, Emission
and Material Output. All nodes are present in the graph; pan or Frame all to see
the extended workspace. Principled inputs are editable, Mix has a variadic group,
and the palette filters types before atomic creation/autoconnection.

The mock owns IDs, snapshots, Undo/Redo and bounded CPU color propagation. Each
node supplies a simple sphere-like color preview through `PreviewOutput::draw`.
This is a GUI example, **not a shader engine or physical material evaluation**.
Connected socket deletion/type changes are deliberately rejected by this host
until disconnected, demonstrating the unchanged-link contract.

Shape/color overrides remain available; set `inheritTypeStyle=false` to use an
explicit pin shape instead of type metadata. Custom value callbacks return
Begin/Update/Commit/Cancel explicitly. Graph links are scheduled only when
`DrawLinks` is called, so explicit low-level `Link` rendering is not duplicated.
See the [P0/P1 review](node-editor-review.md) for validation and remaining work.
