#pragma once
#include <imkit/editor_core.h>
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
enum class LinkStyle { Bezier, Straight, Orthogonal };
enum class Status { Idle, Waiting, Running, Success, Error, Disabled, Bypassed, Cached };
struct NodeStyle {
    ImVec4 canvas{}, grid{}, surface{}, header{}, text{}, muted{}, border{}, selected{}, accent{}, group{},
        preview{}, error{}, warning{}, success{};
    float rounding = 8, borderWidth = 1, padding = 10, headerHeight = 32, pinRadius = 5, linkWidth = 2;
    float gridSize = 24, minimumWidth = 150, minimumHeight = 90, detailZoom = .55f;
    bool shadows = true, animate = true;
    LinkStyle linkStyle = LinkStyle::Bezier;
};
NodeStyle MakeNodeStyle(const Theme &theme);
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
};
struct LinkView {
    LinkId id{};
    PinId from{}, to{};
    std::string_view label{};
    bool reference = false, active = false;
    ImVec4 color{};
    const LinkStyle *style = nullptr;
    float width = 0; // Zero inherits the editor width.
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
    SetPinValue
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
    EditRequest valueEdit{}, pendingPinEdit{};
    std::array<char, 128> pinEditReason{};
    bool paletteWasOpen = false;
    bool valueEditing = false, valueTerminalPending = false;
    Point press{}, lastMouse{}, dragDelta{};
    PinId connecting{};
    LinkId reconnecting{};
    bool selecting = false, cutting = false, palette = false, cancelPending = false;
    std::uint64_t nextOperation = 1;
    void Reserve(std::size_t nodes, std::size_t previewCount = 64);
};
struct Keymap {
    ImGuiKeyChord remove = ImGuiKey_Delete, duplicate = ImGuiMod_Ctrl | ImGuiKey_D,
                  copy = ImGuiMod_Ctrl | ImGuiKey_C, cut = ImGuiMod_Ctrl | ImGuiKey_X,
                  paste = ImGuiMod_Ctrl | ImGuiKey_V, undo = ImGuiMod_Ctrl | ImGuiKey_Z,
                  redo = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, selectAll = ImGuiMod_Ctrl | ImGuiKey_A,
                  add = ImGuiKey_Tab, frame = ImGuiKey_F, group = ImGuiMod_Ctrl | ImGuiKey_G;
    int panButton = ImGuiMouseButton_Middle;
};
struct EditorOptions {
    ImVec2 size{};
    bool grid = true, snap = false, minimap = true, readOnly = false, lasso = false;
    bool canRun = false, canDebug = false;
    bool confirmPinImpact = true;
    double minimumZoom = .15, maximumZoom = 3;
    Keymap keys{};
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
bool GetPinPosition(const EditorFrame &frame, PinId pin, ImVec2 &screenPosition);
void Link(EditorFrame &frame, const LinkView &link);
void DrawLinks(EditorFrame &frame);
// Standard rendering uses the same Begin/End API. Body callback is optional.
void DrawNodes(EditorFrame &frame, void (*body)(void *, EditorFrame &, const NodeView &) = nullptr,
               void *user = nullptr);
bool QueueCommand(EditorFrame &frame, EditKind kind, NodeId node = {});
bool InsertNode(EditorFrame &frame, NodeId node, LinkId link);
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
