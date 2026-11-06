#pragma once
#include <imkit/editor_core.h>
#include <imkit/locale.h>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

// Independent immediate-mode graph UI. The host owns every object passed here.
// No evaluation engine, renderer, filesystem, clipboard or current-editor singleton.
namespace imkit::node_editor {
template <class Tag> struct Id {
    std::uint64_t value = 0;
    explicit constexpr operator bool() const {
        return value != 0;
    }
    auto operator<=>(const Id &) const = default;
};
using NodeId = Id<struct NodeTag>;
using PinId = Id<struct PinTag>;
using LinkId = Id<struct LinkTag>;
using GraphId = Id<struct GraphTag>;
using Point = editor::Point;
using Rect = editor::Rect;
using Phase = editor::Phase;
enum class NodeKind { Node, Frame, Group, Subgraph, Note, Reroute };
enum class PinKind { Input, Output };
enum class Side { Left, Right, Top, Bottom };
enum class PinShape { Circle, Square, Diamond, Triangle };
// What a socket carries. Flow sockets order execution and draw as a chevron; their links are thicker.
enum class PinRole { Data, Flow };
enum class PinContainer { Single, Array, Set, Map };
enum PinFlags : std::uint32_t {
    PinFlags_None = 0,
    PinFlags_Reference = 1u << 0,   // Passes a reference, not a value (hexagon).
    PinFlags_Wildcard = 1u << 1,    // Any type until the host resolves it (dashed ring); skips type checks.
    PinFlags_Field = 1u << 2,       // A value per element (ring with an inner diamond).
    PinFlags_Optional = 1u << 3,    // Works unconnected (open ring).
    PinFlags_Unavailable = 1u << 4, // Not usable in the current mode: faded, rejected as a target.
};
enum class LinkStyle { Bezier, Straight, Orthogonal };
enum class Status { Idle, Waiting, Running, Success, Error, Disabled, Bypassed, Cached };
// Silhouette of a NodeKind::Node. Frames, groups, notes, reroutes and subgraphs take theirs from NodeKind.
// Event: an entry tab marks where execution starts. Compact: no header, a large glyph (operators).
// Variable: a pill with a type chip. Boundary: a dashed outline for subgraph inputs and outputs.
enum class NodeShape { Standard, Event, Compact, Variable, Boundary };
enum class Severity { None, Info, Warning, Error };
struct NodeDiagnostic {
    Severity severity = Severity::None;
    std::string_view message{}; // Shown in a band at the bottom of the node.
};
// Header and link appearance share their enums with Theme::node, so theme files can choose them.
using HeaderFill = ::imkit::NodeHeaderFill;
using LinkColorSource = ::imkit::NodeLinkColor;
using LinkArrow = ::imkit::NodeLinkArrow;
// WhenActive hides the status line of an Idle node that reports no time or progress.
enum class StatusDisplay { WhenActive, Always, Never };
// Host-defined node category: colour and an optional one-glyph tile shown left of the title.
struct NodeCategoryStyle {
    std::uint64_t id = 0;
    ImVec4 color{};
    std::string_view glyph{};
};
struct NodeStyle;
struct NodeView;
struct LinkView;
struct PinChromeView;
// Resolved geometry and colours of one node, handed to NodeDrawHooks.
struct NodeChromeView {
    const NodeView *node = nullptr;
    ImVec2 min{}, max{};
    float zoom = 1, headerHeight = 0, rounding = 0;
    ImVec4 category{}, surface{}, header{}, border{}, text{};
    bool hovered = false, selected = false;
};
// Resolved points and colours of one link, handed to NodeDrawHooks.
struct LinkChromeView {
    const LinkView *link = nullptr;
    std::span<const ImVec2> points{};
    ImVec4 from{}, to{};
    float width = 2;
    bool hovered = false, selected = false, flow = false;
};
// Optional host drawing per part, on the canvas draw list. Return true to replace the default drawing of that
// part, false to keep it. Input handling, IDs and hit areas stay with the editor. Borrowed until EndEditor.
struct NodeDrawHooks {
    void *user = nullptr;
    bool (*background)(ImDrawList *, const NodeChromeView &, void *) = nullptr; // Body, header fill and outline.
    bool (*header)(ImDrawList *, const NodeChromeView &, void *) = nullptr;     // Title row contents.
    bool (*pin)(ImDrawList *, const PinChromeView &, void *) = nullptr;         // Socket glyph.
    bool (*link)(ImDrawList *, const LinkChromeView &, void *) = nullptr;       // Link line.
};
struct NodeStyle {
    ImVec4 canvas{}, grid{}, surface{}, header{}, text{}, muted{}, border{}, selected{}, accent{}, group{},
        preview{}, error{}, warning{}, success{};
    float rounding = 8, borderWidth = 1, padding = 10, headerHeight = 32, pinRadius = 5, linkWidth = 2;
    float gridSize = 24, minimumWidth = 150, minimumHeight = 90, detailZoom = .55f;
    bool shadows = true, animate = true;
    LinkStyle linkStyle = LinkStyle::Bezier;
    float pinSeat = 0;         // Canvas-coloured ring around sockets, so they read as docked into the edge.
    float flowLinkWidth = 3.5f; // Links leaving a Flow socket.
    float flowSpeed = 1, flowSpacing = 14; // Moving marks on active links: multiples of 60 px/s, and pixels.
    HeaderFill headerFill = HeaderFill::Strip;
    float headerTint = .16f;                            // Category colour share for Tinted headers.
    float hoverBorderWidth = 1.5f, selectedBorderWidth = 2; // Zero keeps borderWidth.
    LinkColorSource linkColor = LinkColorSource::OutputType; // Falls back to accent without type colours.
    LinkArrow linkArrow = LinkArrow::Middle;
    bool highlightConnectedLinks = false; // Thicken links of hovered or selected nodes and the hovered link.
    bool hollowUnconnectedPins = false;   // Outline-only sockets until something is connected.
    // While wiring: sockets that cannot accept the wire fade to incompatibleAlpha, nodes without any
    // candidate fade to incompatibleNodeAlpha, and the nearest candidate within snapDistance pixels is taken.
    float incompatibleAlpha = .25f, incompatibleNodeAlpha = .5f, snapDistance = 18;
    StatusDisplay status = StatusDisplay::WhenActive;
    std::span<const NodeCategoryStyle> categories{}; // Borrowed until EndEditor.
    const NodeDrawHooks *hooks = nullptr;            // Borrowed until EndEditor.
};
NodeStyle MakeNodeStyle(const Theme &theme);
// Starting points built on MakeNodeStyle(theme); each sets the appearance choices it names.
// Classic: the plain theme look. Studio: tinted headers, hollow docked sockets, no arrows (materials,
// geometry, compositing). Logic: stronger tint, hollow docked sockets, highlighted links (event and flow graphs).
enum class NodeStylePreset { Classic, Studio, Logic };
NodeStyle MakeNodeStylePreset(const Theme &theme, NodeStylePreset preset);
enum class ConnectionMatch { Exact, Convertible, Rejected };
struct SocketTypeView {
    std::uint64_t id = 0;
    std::string_view name{}, category{};
    ImVec4 color{};
    PinShape shape = PinShape::Circle;
};
struct PinCapabilities {
    bool add = false, remove = false, rename = false, reorder = false, changeType = false;
    bool changeMultiplicity = false, required = false;
    std::uint64_t group = 0;
    int minimum = 0, maximum = 64;
    std::string_view reason{};
};
struct NodeView {
    NodeId id{};
    Point position{}, size{240, 240};
    std::string_view title{}, description{}, icon{};
    NodeId parent{}; // Visual containment, not a graph ownership relation.
    NodeKind kind = NodeKind::Node;
    bool locked = false, collapsed = false;
    GraphId childGraph{};
    Status status = Status::Idle;
    float progress = 0;
    double milliseconds = 0;
    bool breakpoint = false;
    const NodeStyle *style = nullptr; // Borrowed until EndEditor.
    PinCapabilities inputs{}, outputs{};
    std::uint64_t category = 0;           // Looks up NodeStyle::categories.
    ImVec4 tint{};                        // Alpha zero inherits the category, then the accent.
    std::string_view subtitle{}, glyph{}; // Right-aligned header note; tile glyph (overrides the category).
    NodeShape shape = NodeShape::Standard;
    NodeDiagnostic diagnostic{};
    bool hideUnusedPins = false; // Unconnected sockets and their rows disappear.
};
struct PinView {
    PinId id{};
    NodeId node{};
    std::string_view label{};
    PinKind kind = PinKind::Input;
    Side side = Side::Left;
    float offset = 65; // Along the chosen edge, in graph units.
    std::uint64_t type = 0;
    bool multiple = false, hidden = false;
    PinShape shape = PinShape::Circle;
    ImVec4 color{};   // Alpha zero inherits the editor accent.
    PinId internal{}; // Optional exposed -> internal socket correspondence.
    PinCapabilities capabilities{};
    bool inheritTypeStyle = true; // False preserves explicit shape even when type metadata is present.
    bool manualPosition = true;   // Set false when submitting standard PinRow.
    PinRole role = PinRole::Data;
    PinContainer container = PinContainer::Single;
    std::uint32_t flags = PinFlags_None;
    std::uint64_t keyType = 0;       // Map key type; colours the left half of the glyph.
    std::uint64_t panel = 0;         // BeginNodePanel group; a closed panel gathers its sockets on its title row.
    std::string_view inspect{};      // Current value shown when hovering the socket.
    std::string_view reason{};       // Why an Unavailable socket cannot be used.
};
// Everything needed to draw one socket glyph; also handed to host draw hooks.
struct PinChromeView {
    const PinView *pin = nullptr;
    ImVec2 center{};
    float radius = 5, seat = 0;
    ImVec4 color{}, keyColor{}, canvas{};
    PinShape shape = PinShape::Circle;
    bool connected = false, hollow = false, hovered = false;
    int links = 0; // Links into a multi-input socket (an input with multiple=true).
};
// Draws the default glyph for a socket: role, container and flags decide the symbol.
void DrawPinGlyph(ImDrawList *drawList, const PinChromeView &pin);
// Host verdict on an existing link. The editor only draws it; it never removes or repairs links.
enum class LinkValidity { Valid, Converted, Invalid, Cycle };
struct LinkView {
    LinkId id{};
    PinId from{}, to{};
    std::string_view label{};
    bool reference = false, active = false;
    ImVec4 color{};
    const LinkStyle *style = nullptr;
    float width = 0; // Zero inherits the editor width.
    LinkValidity validity = LinkValidity::Valid;
    std::string_view reason{}; // Shown when hovering the link's marker.
};
struct ConnectionVerdict {
    bool allowed = true;
    std::string_view reason{};
    ConnectionMatch match = ConnectionMatch::Exact;
    ConnectionVerdict() = default;
    ConnectionVerdict(bool ok, std::string_view why = {})
        : allowed(ok), reason(why), match(ok ? ConnectionMatch::Exact : ConnectionMatch::Rejected) {}
    ConnectionVerdict(ConnectionMatch result, std::string_view why = {})
        : allowed(result != ConnectionMatch::Rejected), reason(why), match(result) {}
};
struct EditRequest;
struct GraphView {
    GraphId id{1};
    std::uint64_t revision = 0;
    std::span<const NodeView> nodes{};
    std::span<const PinView> pins{};
    std::span<const LinkView> links{};
    void *user = nullptr;
    // Called on the UI thread. The host decides conversions, cycles and semantics.
    ConnectionVerdict (*canConnect)(void *, PinId output, PinId input) = nullptr;
    std::span<const SocketTypeView> socketTypes{};
    ConnectionVerdict (*typeCompatibility)(void *, std::uint64_t output, std::uint64_t input) = nullptr;
    // Optional preflight; host must still validate the complete operation on application.
    ConnectionVerdict (*canEditPin)(void *, const EditRequest &) = nullptr;
};
enum class EditKind {
    Move,
    Resize,
    CreateNode,
    DeleteNode,
    CreateLink,
    DeleteLink,
    Reconnect,
    InsertNode,
    Duplicate,
    Copy,
    Cut,
    Paste,
    Undo,
    Redo,
    Collapse,
    Lock,
    Group,
    Ungroup,
    MakeSubgraph,
    EnterGraph,
    ExposePin,
    ReorderPin,
    HidePin,
    ExposeProperty,
    ReorderProperty,
    SaveTemplate,
    Bypass,
    Breakpoint,
    Watch,
    Run,
    Step,
    Stop,
    Rename,
    PreviewOutput,
    PreviewRefresh,
    CreatePin,
    DeletePin,
    RenamePin,
    ChangePinType,
    SetPinMultiplicity,
    SetPinLimits,
    SetPinValue,
    InsertReroute // Double-click on a link: `link` and the graph position in `after`. Host splits it with a knot.
};
enum class ValueKind { None, Color, Float, Integer, Boolean, Vector, Enum, Text, Custom };
struct PinValue {
    ValueKind kind = ValueKind::None;
    std::array<float, 4> number{};
    int integer = 0;
    bool boolean = false;
    std::array<char, 128> text{};
};
struct EditRequest {
    EditKind kind = EditKind::Move;
    Phase phase = Phase::Commit;
    GraphId graph{};
    std::uint64_t revision = 0, operation = 0;
    std::size_t operationSize = 1;
    NodeId node{}, other{};
    PinId from{}, to{};
    LinkId link{};
    Point before{}, after{};
    std::uint64_t type = 0;
    int index = 0;
    std::array<char, 128> text{}; // Bounded owned text; no retained host pointers.
    PinKind pinKind = PinKind::Input;
    std::uint64_t group = 0;
    int minimum = 0, maximum = 64;
    PinValue value{}, previousValue{};
    std::size_t affectedLinks = 0; // Advisory; host atomically accepts or rejects with links.
};
struct RequestBuffer {
    std::span<EditRequest> storage{};
    std::size_t count = 0;
    bool overflow = false;
    bool PushBatch(std::span<const EditRequest> requests);
    bool Push(EditRequest request);
    void Clear();
    std::span<const EditRequest> Requests() const;
};
enum class LayoutError { None, InvalidInput, MissingNode, DuplicateId, Capacity };
struct PositionChange {
    NodeId node{};
    Point before{}, after{};
};
struct LayoutResult {
    LayoutError error = LayoutError::None;
    std::size_t count = 0;
    explicit operator bool() const {
        return error == LayoutError::None;
    }
};
enum class Alignment { Left, CenterX, Right, Top, CenterY, Bottom };
enum class Axis { Horizontal, Vertical };
enum class Distribution { Centers, Gaps };
enum class AlignReference { Selection, Node, Rectangle };
struct AlignOptions {
    Alignment alignment = Alignment::Left;
    AlignReference reference = AlignReference::Selection;
    NodeId anchor{};
    Rect bounds{};
};
struct ArrangeOptions {
    Axis direction = Axis::Horizontal;
    Point gap{80, 40};
};
// All layout calls are context-free and atomic: errors leave output unchanged.
// Moving a container also translates its unlocked descendants; locked subtrees stay put.
LayoutResult AlignNodes(GraphView graph, std::span<const NodeId> selection, std::span<PositionChange> output,
                        AlignOptions options = {});
LayoutResult DistributeNodes(GraphView graph, std::span<const NodeId> selection,
                             std::span<PositionChange> output, Axis axis = Axis::Horizontal,
                             Distribution distribution = Distribution::Gaps);
LayoutResult ArrangeNodes(GraphView graph, std::span<const NodeId> selection,
                          std::span<PositionChange> output, ArrangeOptions options = {});
LayoutResult SnapNodesToGrid(GraphView graph, std::span<const NodeId> selection,
                             std::span<PositionChange> output, double spacing = 24);
bool FitGroupToContents(GraphView graph, NodeId group, Rect &bounds, double padding = 24);
bool QueueLayout(GraphView graph, std::span<const PositionChange> changes, RequestBuffer &output,
                 std::uint64_t operation);
ConnectionVerdict CanConnect(GraphView graph, PinId a, PinId b, LinkId replacing = {});
// Returns node IDs in deterministic traversal order; no writes on capacity error.
LayoutResult TraceNodes(GraphView graph, NodeId start, bool upstream, std::span<NodeId> output);

// Queue a complete operation or write nothing. IDs of created pins come from the host.
ConnectionVerdict ValidatePinEdit(GraphView graph, const EditRequest &request);
bool QueuePinEdits(GraphView graph, std::span<const EditRequest> edits, RequestBuffer &output,
                   std::uint64_t operation);
bool IsPinConnected(GraphView graph, PinId pin);
struct PinRowOptions {
    PinValue value{};
    float minimum = 0, maximum = 1;
    std::span<const std::string_view> choices{};
    bool hideConnectedValue = true;
    void *user = nullptr;
    // Draw normal ImGui widgets, return changed. Set phase for multi-widget custom editors.
    bool (*draw)(void *, PinValue &, Phase &) = nullptr;
};
struct PinRowPosition {
    PinId pin{};
    Point local{};
};
struct ViewState {
    GraphId graph{};
    Point origin{};
    double zoom = 1;
    std::vector<NodeId> selection;
};
struct Bookmark {
    std::array<char, 64> label{};
    ViewState view;
};
struct PreviewState {
    NodeId node{};
    GraphId graph{};
    bool collapsed = false, interactive = false, expanded = false, pinned = false;
    int output = 0, compare = -1;
    float height = 100;
};
// Open state and title row of one panel inside one node. UI state, not graph data.
struct PanelState {
    NodeId node{};
    std::uint64_t panel = 0;
    bool open = true;
    double row = 0; // Title row centre, graph units below the node's top edge.
};
struct EditorState {
    Point origin{};
    double zoom = 1;
    GraphId graph{};
    std::vector<NodeId> selection;
    NodeId active{};
    LinkId selectedLink{};
    std::vector<ViewState> graphViews, history;
    std::size_t historyIndex = 0;
    std::vector<Bookmark> bookmarks;
    std::vector<PreviewState> previews;
    std::vector<std::uint64_t> favorites, recent;
    std::array<char, 128> search{};
    // UI scratch. Reserve once if desired; never stores host labels or textures.
    std::vector<EditRequest> gesture;
    std::vector<Point> lasso;
    std::vector<std::pair<std::uint64_t, std::size_t>> nodeIndex, pinIndex;
    std::vector<PinRowPosition> rowPositions;
    std::vector<PanelState> panels;
    EditRequest valueEdit{}, pendingPinEdit{};
    std::array<char, 128> pinEditReason{};
    bool paletteWasOpen = false;
    bool valueEditing = false, valueTerminalPending = false;
    Point press{}, lastMouse{}, dragDelta{};
    PinId connecting{};
    LinkId reconnecting{};
    bool selecting = false, cutting = false, palette = false, cancelPending = false;
    bool movingLinks = false; // Ctrl-drag from a socket carries all of its links to another socket.
    LinkId dropLink{};        // Link under a single dragged node; releasing inserts the node there.
    std::uint64_t nextOperation = 1;
    void Reserve(std::size_t nodes, std::size_t previewCount = 64);
};
struct Keymap {
    ImGuiKeyChord remove = ImGuiKey_Delete, duplicate = ImGuiMod_Ctrl | ImGuiKey_D,
                  copy = ImGuiMod_Ctrl | ImGuiKey_C, cut = ImGuiMod_Ctrl | ImGuiKey_X,
                  paste = ImGuiMod_Ctrl | ImGuiKey_V, undo = ImGuiMod_Ctrl | ImGuiKey_Z,
                  redo = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, selectAll = ImGuiMod_Ctrl | ImGuiKey_A,
                  add = ImGuiKey_Tab, frame = ImGuiKey_F, group = ImGuiMod_Ctrl | ImGuiKey_G,
                  bypass = ImGuiMod_Ctrl | ImGuiKey_B, collapse = ImGuiMod_Ctrl | ImGuiKey_H;
    int panButton = ImGuiMouseButton_Middle;
};
struct EditorOptions {
    ImVec2 size{};
    bool grid = true, snap = false, minimap = true, readOnly = false, lasso = false;
    bool canRun = false, canDebug = false;
    bool confirmPinImpact = true;
    double minimumZoom = .15, maximumZoom = 3;
    Keymap keys{};
    const LocaleContext *locale = nullptr; // Optional; keys start with "node_editor.".
};
struct EditorFrame {
    GraphView graph;
    EditorState *state = nullptr;
    RequestBuffer *requests = nullptr;
    NodeStyle style;
    EditorOptions options;
    ImVec2 min{}, max{};
    bool visible = false, hovered = false, blocked = false, nodeOpen = false, pinOpen = false;
    bool linksQueued = false;
    std::string_view error{};
    const NodeView *node = nullptr;
    float fontSize = 14;
    ImGuiContext *context = nullptr;
    ImGuiID editorId = 0;
    NodeId hoveredNode{}; // Topmost node under the mouse this frame.
};
ViewState CaptureView(const EditorState &state);
void RestoreView(EditorState &state, const ViewState &view);
void RememberView(EditorState &state);
bool NavigateHistory(EditorState &state, int direction);
bool AddBookmark(EditorState &state, std::string_view label);
void SetZoom(EditorState &state, double zoom, Point anchorInViewport = {});
bool FrameNodes(EditorState &state, GraphView graph, std::span<const NodeId> nodes, Point viewportSize,
                double padding = 40);
bool IsSelected(const EditorState &state, NodeId node);
void Select(EditorState &state, NodeId node, bool additive = false, bool toggle = false);
EditorFrame BeginEditor(const char *id, GraphView graph, EditorState &state, RequestBuffer &requests,
                        const NodeStyle &style, EditorOptions options = {});
void EndEditor(EditorFrame &frame);              // Always required, even when frame.visible is false.
bool BeginNode(EditorFrame &frame, NodeId node); // EndNode only when true.
void EndNode(EditorFrame &frame);
void BeginPin(EditorFrame &frame, PinId pin);
void EndPin(EditorFrame &frame);
void PinRow(EditorFrame &frame, PinId pin, PinRowOptions options = {});
void PinAddRow(EditorFrame &frame, NodeId node, PinKind kind, std::uint64_t group = 0);
// Collapsible group of rows inside a node. Submit the panel's rows and EndNodePanel only when it returns true.
// `open` is optional host storage; without it the editor keeps the state. While closed, sockets whose
// PinView::panel matches sit on the title row, so their links stay attached.
bool BeginNodePanel(EditorFrame &frame, std::uint64_t panel, std::string_view label, bool *open = nullptr);
void EndNodePanel(EditorFrame &frame);
bool GetPinPosition(const EditorFrame &frame, PinId pin, ImVec2 &screenPosition);
void Link(EditorFrame &frame, const LinkView &link);
void DrawLinks(EditorFrame &frame);
// Standard rendering uses the same Begin/End API. Body callback is optional.
void DrawNodes(EditorFrame &frame, void (*body)(void *, EditorFrame &, const NodeView &) = nullptr,
               void *user = nullptr);
bool QueueCommand(EditorFrame &frame, EditKind kind, NodeId node = {});
bool InsertNode(EditorFrame &frame, NodeId node, LinkId link);
// Links the first free compatible output of the left selected node to the first free compatible input of the
// right one. Needs exactly two selected nodes; returns false when nothing fits.
bool ConnectSelected(EditorFrame &frame);
struct PaletteEntry {
    std::uint64_t type = 0;
    std::string_view title{}, category{}, description{};
    void *user = nullptr;
    bool (*compatible)(void *, PinId) = nullptr;
};
void NodePalette(EditorFrame &frame, std::span<const PaletteEntry> entries);
void MiniMap(EditorFrame &frame, ImVec2 size = {180, 120});
void Diagnostics(EditorFrame &frame);
void LayoutToolbar(EditorFrame &frame);
void NodeSearch(EditorFrame &frame);
void NodeInspector(EditorFrame &frame, NodeId node);
struct PropertyView {
    std::uint64_t id = 0;
    std::string_view label{};
    bool exposed = false;
};
void ExposedProperties(EditorFrame &frame, NodeId node, std::span<const PropertyView> properties);
void BackgroundImage(EditorFrame &frame, ImTextureRef texture, Rect bounds, ImVec4 tint = {1, 1, 1, 1});
void Annotation(EditorFrame &frame, std::span<const Point> points, ImVec4 color, float width = 2);
struct PathEntry {
    GraphId graph{};
    NodeId node{};
    std::string_view title{};
};
void Breadcrumbs(EditorFrame &frame, std::span<const PathEntry> path);
enum class PreviewStatus { Ready, Missing, Waiting, Stale, Error };
struct PreviewOutput {
    std::string_view title{};
    ImTextureRef texture{};
    float aspect = 16.f / 9.f;
    PreviewStatus status = PreviewStatus::Missing;
    std::string_view message{};
    ImVec2 uv0{0, 0}, uv1{1, 1};
    void *user = nullptr;
    void (*draw)(void *, ImVec2 size, bool interactive) = nullptr;
};
struct PreviewDemand {
    NodeId node{};
    int output = 0;
    ImVec2 pixels{};
    bool visible = false, interactive = false;
};
// Call inside a successful BeginNode. Offscreen demand must be handled by the host
// (absence this frame means not requested). Expanded/pinned views are independent.
void Preview(EditorFrame &frame, std::span<const PreviewOutput> outputs,
             void (*demand)(void *, const PreviewDemand &) = nullptr, void *user = nullptr);
void DrawDetachedPreviews(EditorFrame &frame, NodeId node, std::span<const PreviewOutput> outputs,
                          void (*demand)(void *, const PreviewDemand &) = nullptr, void *user = nullptr);
} // namespace imkit::node_editor
