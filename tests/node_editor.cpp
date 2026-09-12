#include <imkit/node_editor.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace imkit::node_editor;
namespace {
int failures = 0;
void Check(bool ok, const char *text) {
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL %s\n", text);
    }
}
bool Near(double a, double b) {
    return std::abs(a - b) < .001;
}
void LayoutTests() {
    std::array<NodeView, 4> nodes{{{{1}, {0, 0}, {100, 50}, "A"},
                                   {{2}, {170, 20}, {200, 80}, "B"},
                                   {{3}, {500, 60}, {50, 40}, "C"},
                                   {{4}, {180, 40}, {30, 20}, "child", {}, {}, {2}}}};
    GraphView g;
    g.nodes = nodes;
    g.revision = 7;
    NodeId selected[] = {{1}, {2}, {3}};
    std::array<PositionChange, 4> out{};
    auto r = AlignNodes(g, selected, out, {Alignment::Top});
    Check(r && r.count == 3, "align includes selected group's child exactly once");
    Check(out[0].node == NodeId{2} && Near(out[0].after.y, 0) && out[1].node == NodeId{4} &&
              Near(out[1].after.y, 20),
          "group child preserves relative offset");
    AlignOptions opt;
    opt.alignment = Alignment::Right;
    opt.reference = AlignReference::Node;
    opt.anchor = {1};
    r = AlignNodes(g, selected, out, opt);
    Check(r && Near(out[0].after.x, -100), "anchor and unequal widths");
    out[0].node = {999};
    auto small = std::span(out).first(1);
    r = AlignNodes(g, selected, small);
    Check(r.error == LayoutError::Capacity && out[0].node == NodeId{999}, "capacity failure writes nothing");
    NodeId duplicate[] = {{1}, {1}};
    r = AlignNodes(g, duplicate, out);
    Check(r.error == LayoutError::DuplicateId, "duplicate selection rejected");
    NodeId missing[] = {{99}};
    Check(AlignNodes(g, missing, out).error == LayoutError::MissingNode, "stale selection rejected");
    nodes[1].locked = true;
    r = AlignNodes(g, selected, out, {Alignment::Top});
    Check(r && r.count == 1 && out[0].node == NodeId{3}, "locked subtree remains stationary");
    nodes[1].locked = false;
    r = DistributeNodes(g, selected, out);
    Check(r && Near(out[0].after.x, 200), "equal gaps use actual widths");
    r = DistributeNodes(g, selected, out, Axis::Horizontal, Distribution::Centers);
    Check(r && Near(out[0].after.x, 187.5), "equal centers distinct from equal gaps");
    Rect fitted{};
    Check(FitGroupToContents(g, {2}, fitted, 10) && Near(fitted.min.x, 170) && Near(fitted.max.y, 70),
          "fit group uses content bounds and padding");
    Check(SnapNodesToGrid(g, selected, out, 0).error == LayoutError::InvalidInput, "invalid grid");
    nodes[0].position.x = std::numeric_limits<double>::quiet_NaN();
    Check(AlignNodes(g, selected, out).error == LayoutError::InvalidInput, "nonfinite rejected");
    nodes[0].position.x = 0;
    nodes[1].parent = {4};
    Check(AlignNodes(g, selected, out).error == LayoutError::InvalidInput, "containment cycle rejected");
    nodes[1].parent = {};
    PinView pins[] = {{{1}, {1}, "out", PinKind::Output, Side::Right}, {{2}, {2}, "in"},
                      {{3}, {2}, "out", PinKind::Output, Side::Right}, {{4}, {3}, "in"},
                      {{5}, {3}, "out", PinKind::Output, Side::Right}, {{6}, {1}, "in"}};
    LinkView links[] = {{{1}, {1}, {2}}, {{2}, {3}, {4}}, {{3}, {5}, {6}}};
    g.pins = pins;
    g.links = links;
    r = ArrangeNodes(g, selected, out);
    Check(bool(r), "cyclic graph has a finite deterministic layout");
    for (std::size_t i = 0; i < r.count; ++i)
        Check(std::isfinite(out[i].after.x) && std::isfinite(out[i].after.y), "finite cycle layout");
    Check(!CanConnect(g, {1}, {2}).allowed, "duplicate / occupied socket rejected");
    Check(CanConnect(g, {1}, {2}, {1}).allowed, "reconnect excludes original link");
    Check(!CanConnect(g, {1}, {3}).allowed, "same direction rejected");
    std::array<NodeId, 4> trace{};
    r = TraceNodes(g, {1}, false, trace);
    Check(r && r.count == 3, "cycle-safe downstream traversal");
    EditRequest requests[4];
    RequestBuffer buffer{requests};
    r = AlignNodes(g, selected, out, {Alignment::Top});
    Check(r && QueueLayout(g, std::span(out).first(r.count), buffer, 33),
          "queue layout as atomic host transaction");
    for (auto &e : buffer.Requests())
        Check(e.operation == 33 && e.operationSize == r.count && e.revision == 7,
              "shared operation and revision");
    buffer.Clear();
    out[0].before.x += 1;
    Check(!QueueLayout(g, std::span(out).first(1), buffer, 34) && buffer.count == 0,
          "stale positions rejected atomically");
}
void InputTests() {
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1000, 700};
    io.DeltaTime = 1.f / 60;
    unsigned char *pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = imkit::MakeTheme(imkit::ColorScheme::Dark);
    auto style = MakeNodeStyle(theme);
    NodeView nodes[] = {{{1}, {30, 30}, {260, 320}, "Input"}, {{2}, {400, 30}, {260, 320}, "Output"}};
    PinView pins[] = {{{1}, {1}, "out", PinKind::Output, Side::Right}, {{2}, {2}, "in"}};
    GraphView graph;
    graph.nodes = nodes;
    graph.pins = pins;
    EditorState state;
    state.Reserve(16);
    EditRequest events[16];
    RequestBuffer out{events};
    ImVec2 inputPos{};
    ImVec2 canvasMin{};
    char text[32] = "safe";
    int demands = 0;
    bool disabled = false;
    auto frame = [&]() {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({1000, 700});
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginDisabled(disabled);
        auto f = BeginEditor("graph", graph, state, out, style, {{960, 640}, true, false, false});
        canvasMin = f.min;
        DrawLinks(f);
        for (auto &n : nodes)
            if (BeginNode(f, n.id)) {
                ImGui::InputText("##text", text, sizeof(text));
                if (n.id == NodeId{1})
                    inputPos = ImGui::GetItemRectMin();
                PreviewOutput preview;
                preview.title = "value";
                preview.status = PreviewStatus::Ready;
                preview.draw = [](void *, ImVec2 size, bool) {
                    ImGui::TextUnformatted("Host supplied value");
                    ImGui::Dummy({size.x, 10});
                };
                Preview(
                    f, {&preview, 1},
                    [](void *user, const PreviewDemand &d) {
                        if (d.visible)
                            ++*static_cast<int *>(user);
                    },
                    &demands);
                EndNode(f);
            }
        EndEditor(f);
        ImGui::EndDisabled();
        ImGui::End();
        ImGui::Render();
    };
    frame();
    frame();
    Check(demands > 0, "visible node requests preview");
    state.selection = {{1}};
    io.AddMousePosEvent(inputPos.x + 20, inputPos.y + 8);
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    out.Clear();
    io.AddKeyEvent(ImGuiKey_Delete, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Delete, false);
    frame();
    Check(out.count == 0, "Delete inside native text input does not delete node");
    io.AddKeyEvent(ImGuiKey_Escape, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Escape, false);
    frame();
    // Host-state zoom preserves the point under the chosen viewport anchor.
    Point anchor{200, 150};
    auto before = Point{state.origin.x + anchor.x / state.zoom, state.origin.y + anchor.y / state.zoom};
    SetZoom(state, 1.7, anchor);
    Check(Near(state.origin.x + anchor.x / state.zoom, before.x) &&
              Near(state.origin.y + anchor.y / state.zoom, before.y),
          "cursor anchored zoom");
    frame();
    Check(!ImGui::IsAnyItemActive(), "zoom frame leaves no active native item");
    // Actual public IO: header drag -> complete transaction, with graph geometry
    // unchanged until a host applies Commit.
    state.zoom = 1;
    state.origin = {};
    out.Clear();
    frame();
    io.AddMousePosEvent(canvasMin.x + 80, canvasMin.y + 45);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMousePosEvent(canvasMin.x + 122, canvasMin.y + 65);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    bool moved = false;
    for (auto &e : out.Requests())
        if (e.kind == EditKind::Move && e.phase == Phase::Commit && e.node == NodeId{1})
            moved = Near(e.after.x, 72) && Near(e.after.y, 50);
    Check(moved && Near(nodes[0].position.x, 30), "header drag emits host-owned position commit");
    // Link endpoints use the same transformed coordinates as the drawn ports.
    out.Clear();
    io.AddMousePosEvent(canvasMin.x + 290, canvasMin.y + 95);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMousePosEvent(canvasMin.x + 400, canvasMin.y + 95);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    bool linked = false;
    for (auto &e : out.Requests())
        if (e.kind == EditKind::CreateLink)
            linked = e.from == PinId{1} && e.to == PinId{2};
    Check(linked, "port drag emits a validated connection");
    disabled = true;
    out.Clear();
    io.AddMousePosEvent(canvasMin.x + 290, canvasMin.y + 95);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMousePosEvent(canvasMin.x + 400, canvasMin.y + 95);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    Check(out.count == 0, "inherited BeginDisabled blocks graph edits");
    disabled = false;
    out.Clear();
    EditRequest gesture;
    gesture.graph = graph.id;
    gesture.revision = 0;
    gesture.operation = 42;
    gesture.node = {1};
    gesture.before = nodes[0].position;
    gesture.after = {100, 100};
    state.gesture = {gesture};
    graph.revision = 1;
    frame();
    Check(state.gesture.empty() && out.count == 1 && out.Requests()[0].phase == Phase::Cancel,
          "external revision cancels entire gesture");
    out.Clear();
    state.gesture = {gesture};
    out.count = out.storage.size();
    frame();
    Check(state.cancelPending && !state.gesture.empty(), "cancel retained when output is full");
    out.Clear();
    frame();
    Check(!state.cancelPending && state.gesture.empty() && out.Requests()[0].phase == Phase::Cancel,
          "cancel delivered after buffer drained");
    EditorState other;
    other.graph = {9};
    other.origin = {99, 77};
    Check(state.graph != other.graph, "independent editors");
    ImGui::DestroyContext();
}
} // namespace
int main() {
    LayoutTests();
    InputTests();
    std::printf("node_editor: %d failure(s)\n", failures);
    return failures ? 1 : 0;
}
