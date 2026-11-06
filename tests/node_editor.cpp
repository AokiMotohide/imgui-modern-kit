#include <imkit/node_editor.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif
#include "../examples/node_editor/material_mock.h"
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
void SocketTests() {
    material_mock::Model m;
    auto n = m.Add(7, {});
    auto g = m.View();
    EditRequest edits[5]{};
    const auto pin = m.data.pins[0].view.id;
    edits[0].kind = EditKind::RenamePin;
    edits[0].from = pin;
    edits[0].text[0] = 'X';
    edits[1].kind = EditKind::ChangePinType;
    edits[1].from = pin;
    edits[1].type = 2;
    edits[2].kind = EditKind::ReorderPin;
    edits[2].from = pin;
    edits[2].index = 2;
    edits[3].kind = EditKind::CreatePin;
    edits[3].pinKind = PinKind::Input;
    edits[3].index = 5;
    edits[3].type = 1;
    edits[3].text[0] = 'I';
    edits[4].kind = EditKind::CreatePin;
    edits[4].pinKind = PinKind::Output;
    edits[4].index = 1;
    edits[4].type = 6;
    edits[4].text[0] = 'O';
    for (auto &r : edits)
        r.node = n;
    EditRequest storage[8]{};
    RequestBuffer out{storage};
    RequestBuffer tiny{std::span(storage).first(2)};
    Check(!QueuePinEdits(g, edits, tiny, 55) && tiny.count == 0 && tiny.overflow,
          "socket batch capacity writes nothing");
    Check(QueuePinEdits(g, edits, out, 55), "typed socket batch queued");
    for (auto &r : out.Requests())
        Check(r.operation == 55 && r.revision == g.revision && r.operationSize == 5,
              "socket batch operation/revision");
    Check(m.Apply(out.Requests()) && m.undo.size() == 1, "socket changes are one host undo unit");
    Check(m.Find(pin)->name == "X" && m.Find(pin)->view.type == 2 && m.data.pins.size() == 8,
          "rename type and dynamic input/output apply");
    auto command = [&](EditKind kind) {
        EditRequest r;
        r.kind = kind;
        r.graph = {77};
        r.revision = m.revision;
        r.operation = 90;
        return r;
    };
    auto undo = command(EditKind::Undo);
    m.Apply({&undo, 1});
    Check(m.data.pins.size() == 6, "dynamic batch undo");
    auto redo = command(EditKind::Redo);
    m.Apply({&redo, 1});
    Check(m.data.pins.size() == 8, "dynamic batch redo");
    auto source = m.Add(2, {400, 0});
    auto output = m.Socket(source, PinKind::Output);
    Check(m.Connect(output, pin), "mock exact connection");
    auto links = m.data.links.size();
    auto reject = command(EditKind::DeletePin);
    reject.node = n;
    reject.from = pin;
    Check(!m.Apply({&reject, 1}) && m.data.links.size() == links && m.Find(pin),
          "connected deletion refusal preserves links");
    reject = command(EditKind::ChangePinType);
    reject.node = n;
    reject.from = pin;
    reject.type = 1;
    Check(!m.Apply({&reject, 1}) && m.Find(pin)->view.type == 2 && m.data.links.size() == links,
          "connected type refusal preserves model");
    auto in = m.Add(1, {700, 0});
    g = m.View();
    Check(CanConnect(g, output, m.Socket(in, PinKind::Input)).match == ConnectionMatch::Convertible,
          "convertible verdict");
    auto closure = m.Socket(n, PinKind::Output);
    g = m.View();
    Check(CanConnect(g, closure, m.Socket(in, PinKind::Input)).match == ConnectionMatch::Rejected,
          "rejected verdict");
    auto mix = m.Add(8, {});
    g = m.View();
    EditRequest add;
    add.kind = EditKind::CreatePin;
    add.node = mix;
    add.pinKind = PinKind::Input;
    add.group = 1;
    add.type = 6;
    add.index = 2;
    add.text[0] = 'C';
    out.Clear();
    Check(QueuePinEdits(g, {&add, 1}, out, 91) && m.Apply(out.Requests()), "variadic append");
    auto added = m.data.pins.back().view.id;
    for (auto &p : m.data.pins)
        if (p.name == "C")
            added = p.view.id;
    auto move = command(EditKind::ReorderPin);
    move.node = mix;
    move.from = added;
    move.index = 0;
    Check(m.Apply({&move, 1}), "variadic reorder");
    auto remove = command(EditKind::DeletePin);
    remove.node = mix;
    remove.from = added;
    Check(m.Apply({&remove, 1}) && !m.Find(added), "variadic remove");
    remove = command(EditKind::DeletePin);
    remove.node = mix;
    remove.from = m.Socket(mix, PinKind::Input, 6);
    Check(!m.Apply({&remove, 1}), "variadic minimum enforced");
    auto before = m.Find(in)->preview;
    m.Find(pin)->value.number[0] = .95f;
    m.Evaluate();
    auto create = command(EditKind::CreateNode);
    create.type = 11;
    create.from = closure;
    create.after = {900, 0};
    Check(m.Apply({&create, 1}) && m.data.links.size() == links + 1,
          "palette create and automatic connect atomic");
    auto target = m.data.nodes.back().view.id;
    auto oldColor = m.Find(target)->preview;
    m.Find(pin)->value.number[0] = .1f;
    m.Find(output)->value.number[0] = .1f;
    for (auto &p : m.data.pins)
        if (p.view.node == source)
            p.value.number = {.1f, .1f, .1f, 1};
    m.Evaluate();
    Check(!Near(oldColor.x, m.Find(target)->preview.x), "host CPU preview changes after upstream input");
    Check(node_gallery::ApplyColor(m, target, {.8f, .2f, .1f, 1}), "node header color commits");
    auto undoColor = command(EditKind::Undo);
    Check(m.Apply({&undoColor, 1}) && m.Find(target)->color.w == 0, "node color undo restores theme");
    auto redoColor = command(EditKind::Redo);
    Check(m.Apply({&redoColor, 1}) && Near(m.Find(target)->color.x, .8), "node color redo restores custom color");
    auto rename = command(EditKind::Rename);
    rename.node = target;
    std::snprintf(rename.text.data(), rename.text.size(), "Custom output");
    Check(m.Apply({&rename, 1}) && m.Find(target)->name == "Custom output", "node name commits");
    auto undoName = command(EditKind::Undo);
    Check(m.Apply({&undoName, 1}) && m.Find(target)->name == "Material Output", "node name undo");
    auto view = m.View();
    auto style = MakeNodeStyle(imkit::MakeTheme(imkit::ColorScheme::Dark));
    node_gallery::StyleNodes(m.nodes, m, style);
    Check(view.nodes.back().style && view.nodes.back().style->headerFill == HeaderFill::Flat &&
              Near(view.nodes.back().tint.x, .8),
          "custom header color is supplied as the node tint with a flat header");
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
    bool disabled = false, standard = false, rowText = false, minimap = false;
    ImVec2 rowTextPos{};
    ImVec2 expectedPin{};
    int valueDraws = 0;

    auto frame = [&]() {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({1000, 700});
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginDisabled(disabled);
        auto f = BeginEditor("graph", graph, state, out, style, {{960, 640}, true, false, minimap});
        canvasMin = f.min;
        DrawLinks(f);
        for (auto &n : nodes)
            if (BeginNode(f, n.id)) {
                if (standard) {
                    auto id = n.id == NodeId{1} ? PinId{1} : PinId{2};
                    auto start = ImGui::GetCursorScreenPos();
                    float height =
                        std::max(ImGui::GetFrameHeight(), style.headerHeight * float(state.zoom) * .7f);
                    PinRowOptions opt;
                    opt.value.kind = ValueKind::Custom;
                    opt.user = &valueDraws;
                    opt.draw = [](void *user, PinValue &, Phase &) {
                        ++*static_cast<int *>(user);
                        ImGui::Button("Value");
                        return false;
                    };
                    if (rowText) {
                        opt.value.kind = ValueKind::Text;
                        opt.value.text[0] = 'A';
                    }
                    PinRow(f, id, opt);
                    if (n.id == NodeId{2})
                        rowTextPos = {start.x +
                                          float(n.size.x * state.zoom - 2 * style.padding * state.zoom) * .7f,
                                      start.y + height * .5f};
                    ImVec2 actual;
                    GetPinPosition(f, id, actual);
                    Check(Near(actual.y, start.y + height * .5f),
                          "row center equals socket/link endpoint after scale changes");
                    if (n.id == NodeId{2})
                        expectedPin = actual;
                }
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
    standard = true;
    pins[0].manualPosition = pins[1].manualPosition = false;
    for (float zoom : {.4f, .8f, 1.5f}) {
        state.zoom = zoom;
        for (float scale : {1.f, 1.5f}) {
            style.headerHeight = 32 * scale;
            style.padding = 10 * scale;
            frame();
        }
    }
    state.zoom = 1;
    valueDraws = 0;
    frame();
    Check(valueDraws == 1, "unconnected input draws default editor");
    LinkView connected{{9}, {1}, {2}};
    graph.links = {&connected, 1};
    valueDraws = 0;
    frame();
    Check(valueDraws == 0, "connected input omits default editor");
    graph.links = {};
    rowText = true;
    style.headerHeight = 32;
    style.padding = 10;
    out.Clear();
    frame();
    io.AddMousePosEvent(rowTextPos.x, rowTextPos.y);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    io.AddInputCharactersUTF8("Z");
    frame();
    io.AddKeyEvent(ImGuiKey_Delete, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Delete, false);
    frame();
    bool began = false, updated = false;
    for (auto &e : out.Requests()) {
        Check(e.kind == EditKind::SetPinValue, "socket text shortcuts do not escape to graph");
        began |= e.phase == Phase::Begin;
        updated |= e.phase == Phase::Update;
    }
    Check(began && updated, "default text emits Begin and Update");
    out.Clear();
    io.AddKeyEvent(ImGuiKey_Escape, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Escape, false);
    frame();
    bool cancelled = false;
    for (auto &e : out.Requests())
        cancelled |= e.phase == Phase::Cancel;
    Check(cancelled && !state.valueEditing, "default text Escape cancels transaction");
    out.Clear();
    io.AddMousePosEvent(rowTextPos.x, rowTextPos.y);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    io.AddInputCharactersUTF8("Q");
    frame();
    io.AddMousePosEvent(canvasMin.x + 800, canvasMin.y + 500);
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    bool committed = false;
    std::uint64_t operation = 0;
    for (auto &e : out.Requests())
        if (e.kind == EditKind::SetPinValue) {
            if (!operation)
                operation = e.operation;
            Check(e.operation == operation && e.revision == graph.revision,
                  "value phases share operation/revision");
            committed |= e.phase == Phase::Commit;
        }
    Check(committed && !state.valueEditing, "default text blur commits transaction");
    out.Clear();
    state.valueEdit = {};
    state.valueEdit.kind = EditKind::SetPinValue;
    state.valueEdit.graph = graph.id;
    state.valueEdit.revision = graph.revision;
    state.valueEdit.node = {2};
    state.valueEdit.from = {2};
    state.valueEdit.operation = 501;
    state.valueEdit.phase = Phase::Commit;
    state.valueEditing = state.valueTerminalPending = true;
    out.count = out.storage.size();
    frame();
    Check(state.valueTerminalPending, "value terminal retained on capacity failure");
    out.Clear();
    frame();
    Check(!state.valueEditing && !state.valueTerminalPending && out.count == 1 &&
              out.Requests()[0].phase == Phase::Commit,
          "value terminal retry after drain");
    out.Clear();
    rowText = false;
    standard = false;
    minimap = true;
    nodes[1].position = {740, 480};
    state.origin = {};
    frame();
    frame();
    io.AddMousePosEvent(canvasMin.x + 810, canvasMin.y + 545);
    frame();
    io.AddMouseButtonEvent(0, true);
    frame();
    io.AddMouseButtonEvent(0, false);
    frame();
    Check(out.count == 0 && state.gesture.empty(), "minimap consumes pointer without node edits");
    Check(!Near(state.origin.x, 0) || !Near(state.origin.y, 0), "minimap click navigates viewport");
    // Empty/collapsed node bodies must still finish a valid child layout.
    nodes[1].position = {400, 30};
    for (int i = 0; i < 20; ++i) {
        nodes[1].collapsed = i % 2 == 0;
        state.origin = {};
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({1000, 700});
        ImGui::Begin("collapse regression");
        auto f = BeginEditor("collapse", graph, state, out, style);
        DrawNodes(f);
        EndEditor(f);
        ImGui::End();
        ImGui::Render();
    }
    ImGui::DestroyContext();
}

// Counts draw-list vertices of one exact colour across the rendered frame.
int CountVertices(ImU32 color) {
    int count = 0;
    auto *data = ImGui::GetDrawData();
    for (int l = 0; data && l < data->CmdListsCount; ++l)
        for (auto &v : data->CmdLists[l]->VtxBuffer)
            count += v.col == color;
    return count;
}
void AppearanceTests() {
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigMacOSXBehaviors = false; // The steps inject a literal Ctrl; on macOS Dear ImGui would swap it with Cmd.
    io.DisplaySize = {1000, 700};
    io.DeltaTime = 1.f / 60;
    unsigned char *pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    // Theme file keys reach the node style and survive a round trip.
    auto theme = imkit::MakeTheme(imkit::ColorScheme::Dark);
    auto parsed = imkit::ParseTheme("node.header-fill = tinted\nnode.link-arrow = none\nnode.link-color = gradient\n"
                                    "node.rounding = 3\nnode.hollow-pins = true\nnode.header-tint = 0.3\n",
                                    theme);
    Check(parsed.applied == 6 && parsed.ignored == 0, "node theme keys parse");
    auto style = MakeNodeStyle(theme);
    Check(style.headerFill == HeaderFill::Tinted && style.linkArrow == LinkArrow::None &&
              style.linkColor == LinkColorSource::Gradient && Near(style.rounding, 3) && style.hollowUnconnectedPins &&
              Near(style.headerTint, .3),
          "node theme tokens shape MakeNodeStyle");
    auto text = imkit::SerializeTheme(theme);
    imkit::Theme restored = imkit::MakeTheme(imkit::ColorScheme::Dark);
    imkit::ParseTheme(text, restored);
    Check(imkit::SerializeTheme(restored) == text && text.find("node.header-fill = tinted") != std::string::npos,
          "node theme tokens round-trip");
    Check(imkit::SerializeTheme(imkit::MakeTheme()).find("node.rounding") == std::string::npos,
          "unset node numbers are not written");

    theme = imkit::MakeTheme(imkit::ColorScheme::Dark);
    style = MakeNodeStyle(theme);
    Check(style.status == StatusDisplay::WhenActive && style.headerFill == HeaderFill::Strip,
          "default node look keeps the strip header");
    const ImVec4 red{1, 0, 0, 1}, blue{0, 0, 1, 1};
    SocketTypeView types[] = {{1, "Red", "Test", red}, {2, "Blue", "Test", blue}};
    NodeView nodes[] = {{{1}, {20, 20}, {200, 160}, "Source"},
                        {{2}, {420, 20}, {200, 160}, "Target"},
                        {{3}, {300, 300}, {240, 240}, "Knot", {}, {}, {}, NodeKind::Reroute}};
    PinView pins[] = {{{1}, {1}, "out", PinKind::Output, Side::Right, 60, 1},
                      {{2}, {2}, "in", PinKind::Input, Side::Left, 60, 2},
                      {{3}, {2}, "need", PinKind::Input, Side::Left, 100, 2},
                      {{4}, {3}, "in", PinKind::Input},
                      {{5}, {3}, "out", PinKind::Output, Side::Right}};
    pins[2].capabilities.required = true;
    LinkView links[] = {{{1}, {1}, {2}}};
    GraphView graph;
    graph.nodes = nodes;
    graph.pins = pins;
    graph.links = links;
    graph.socketTypes = types;
    EditorState state;
    EditRequest events[16];
    RequestBuffer out{events};
    struct Lookups {
        int idle = 0, running = 0;
    } lookups;
    imkit::LocaleContext locale;
    locale.user = &lookups;
    locale.lookup = [](void *user, std::string_view key, const char *fallback) {
        auto &l = *static_cast<Lookups *>(user);
        l.idle += key == "node_editor.status.idle";
        l.running += key == "node_editor.status.running";
        return fallback;
    };
    EditorOptions options{{960, 640}, true, false, false};
    options.locale = &locale;
    ImVec2 knot{};
    NodeId hovered{};
    auto frame = [&]() {
        out.Clear();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({1000, 700});
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        auto f = BeginEditor("graph", graph, state, out, style, options);
        hovered = f.hoveredNode;
        DrawLinks(f);
        DrawNodes(f);
        GetPinPosition(f, {4}, knot);
        EndEditor(f);
        ImGui::End();
        ImGui::Render();
    };
    frame();
    Check(lookups.idle == 0, "idle node without timing hides its status line");
    nodes[0].status = Status::Running;
    frame();
    Check(lookups.running > 0, "running node shows its localised status line");
    nodes[0].status = Status::Idle;
    style.status = StatusDisplay::Always;
    frame();
    Check(lookups.idle > 0, "StatusDisplay::Always keeps the status line");
    style.status = StatusDisplay::WhenActive;

    // A reroute is a 16-unit knot whatever size the host stored; both sockets sit at its centre.
    ImVec2 knotOut{};
    {
        ImGui::NewFrame();
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        auto f = BeginEditor("graph", graph, state, out, style, options);
        GetPinPosition(f, {4}, knot);
        GetPinPosition(f, {5}, knotOut);
        const float expectX = f.min.x + float((300 + 8 - state.origin.x) * state.zoom);
        const float expectY = f.min.y + float((300 + 8 - state.origin.y) * state.zoom);
        Check(Near(knot.x, expectX) && Near(knot.y, expectY) && Near(knotOut.x, knot.x) && Near(knotOut.y, knot.y),
              "reroute sockets share the knot centre");
        EndEditor(f);
        ImGui::End();
        ImGui::Render();
    }

    // Link colour follows the output socket type by default and the accent when asked.
    style.linkArrow = LinkArrow::None;
    frame();
    const int typed = CountVertices(ImGui::GetColorU32(red));
    style.linkColor = LinkColorSource::Accent;
    frame();
    const int accent = CountVertices(ImGui::GetColorU32(red));
    Check(typed > accent && accent > 0, "links take the output type colour by default");
    style.linkColor = LinkColorSource::OutputType;

    // An unconnected required input carries an error mark that disappears once connected.
    const ImU32 error = ImGui::GetColorU32(style.error);
    frame();
    const int marked = CountVertices(error);
    links[0].to = {3};
    frame();
    const int satisfied = CountVertices(error);
    links[0].to = {2};
    Check(marked > satisfied, "unconnected required input is marked");

    // Hollow sockets draw an outline instead of a disc while unconnected.
    style.hollowUnconnectedPins = true;
    frame();
    const int hollow = CountVertices(ImGui::GetColorU32(blue));
    style.hollowUnconnectedPins = false;
    frame();
    Check(hollow != CountVertices(ImGui::GetColorU32(blue)), "hollow sockets change the unconnected glyph");

    // The topmost node under the mouse is reported for hover styling.
    io.MousePos = {120, 140};
    frame();
    frame();
    Check(hovered == NodeId{1}, "hovered node is reported");

    // Link markers: an invalid link turns to the error colour with a cross, a converted one gets a marker.
    io.MousePos = {-100, -100};
    const ImU32 warning = ImGui::GetColorU32(style.warning);
    frame();
    const int validError = CountVertices(error), validWarning = CountVertices(warning);
    links[0].validity = LinkValidity::Invalid;
    frame();
    Check(CountVertices(error) > validError + 20, "invalid link is drawn in the error colour with a marker");
    links[0].validity = LinkValidity::Converted;
    frame();
    Check(CountVertices(warning) > validWarning, "converted link carries a marker");
    links[0].validity = LinkValidity::Valid;

    // Wiring snaps to the nearest compatible socket and fades the others.
    graph.typeCompatibility = [](void *, std::uint64_t, std::uint64_t) {
        return ConnectionVerdict{ConnectionMatch::Convertible, "Host conversion"};
    };
    ImVec2 source{}, target{};
    auto wire = [&](ImVec2 mouse, bool down) {
        io.MousePos = mouse;
        io.MouseDown[0] = down;
        out.Clear();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({1000, 700});
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        auto f = BeginEditor("graph", graph, state, out, style, options);
        DrawLinks(f);
        DrawNodes(f);
        GetPinPosition(f, {1}, source);
        GetPinPosition(f, {3}, target);
        EndEditor(f);
        ImGui::End();
        ImGui::Render();
    };
    state.selection.clear();
    pins[0].multiple = true; // The source already feeds one link.
    wire({-100, -100}, false);
    wire(source, false);
    wire(source, true);
    Check(state.connecting == PinId{1}, "pressing an output starts a wire");
    wire({target.x - 15, target.y + 2}, true); // Outside the socket's own hit box, inside the snap radius.
    wire({target.x - 15, target.y + 2}, false);
    bool created = false;
    for (auto &r : out.Requests())
        created = created || (r.kind == EditKind::CreateLink && r.from == PinId{1} && r.to == PinId{3});
    Check(created, "releasing near a compatible socket snaps the wire to it");
    Check(!state.connecting, "wire ends after the drop");
    graph.typeCompatibility = nullptr;
    pins[0].multiple = false;
    io.MousePos = {-100, -100};

    // Node states: an empty locked node closes cleanly, a diagnostic draws a band, Disabled disables the body,
    // and the open-subgraph button queues EnterGraph.
    {
        nodes[1].locked = true;
        frame(); // Would assert if the body cursor were left unsettled.
        nodes[1].locked = false;
        frame();
        const int plain = CountVertices(error);
        nodes[1].diagnostic = {Severity::Error, "Missing input"};
        frame();
        Check(CountVertices(error) > plain, "error diagnostic draws a badge, band and outline");
        nodes[1].diagnostic = {};
        bool disabledBody = false;
        nodes[0].status = Status::Disabled;
        out.Clear();
        ImGui::NewFrame();
        ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        {
            auto f = BeginEditor("graph", graph, state, out, style, options);
            if (BeginNode(f, {1})) {
                ImGui::Dummy({1, 1});
                disabledBody = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
                EndNode(f);
            }
            EndEditor(f);
        }
        ImGui::End();
        ImGui::Render();
        nodes[0].status = Status::Idle;
        Check(disabledBody, "a Disabled node disables its body widgets");

        nodes[1].childGraph = {9};
        auto click = [&](ImVec2 at, bool down) {
            io.MousePos = at;
            io.MouseDown[0] = down;
            frame();
        };
        ImVec2 origin{};
        {
            ImGui::NewFrame();
            ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            auto f = BeginEditor("graph", graph, state, out, style, options);
            origin = f.min;
            EndEditor(f);
            ImGui::End();
            ImGui::Render();
        }
        const float z = float(state.zoom), header = style.headerHeight * z;
        const float badge = std::max(6.f, std::min(header * .5f, 16 * z));
        const ImVec2 button{origin.x + float((420 + 200 - state.origin.x) * z) - style.padding * z - badge * .5f,
                            origin.y + float((20 - state.origin.y) * z) + header * .5f};
        click(button, false);
        click(button, true);
        bool entered = false;
        for (auto &r : out.Requests())
            entered = entered || (r.kind == EditKind::EnterGraph && r.node == NodeId{2});
        click(button, false);
        for (auto &r : out.Requests())
            entered = entered || (r.kind == EditKind::EnterGraph && r.node == NodeId{2});
        Check(entered, "the header button of a subgraph queues EnterGraph");
        nodes[1].childGraph = {};
        io.MousePos = {-100, -100};
        frame();
    }

    // Presets, hooks and the editing gestures added with them.
    {
        auto studio = MakeNodeStylePreset(theme, NodeStylePreset::Studio);
        auto logic = MakeNodeStylePreset(theme, NodeStylePreset::Logic);
        auto classic = MakeNodeStylePreset(theme, NodeStylePreset::Classic);
        Check(studio.headerFill == HeaderFill::Tinted && studio.hollowUnconnectedPins && studio.pinSeat > 0 &&
                  studio.linkArrow == LinkArrow::None,
              "Studio preset sets its appearance");
        Check(logic.highlightConnectedLinks && logic.headerTint > studio.headerTint, "Logic preset sets its appearance");
        const auto plain = MakeNodeStyle(theme);
        Check(classic.headerFill == plain.headerFill && classic.linkArrow == plain.linkArrow &&
                  classic.hollowUnconnectedPins == plain.hollowUnconnectedPins,
              "Classic preset equals the plain theme style");

        struct Calls {
            int background = 0, pin = 0;
        } calls;
        NodeDrawHooks hooks;
        hooks.user = &calls;
        hooks.background = [](ImDrawList *, const NodeChromeView &v, void *user) {
            ++static_cast<Calls *>(user)->background;
            return v.node && v.max.x > v.min.x;
        };
        hooks.pin = [](ImDrawList *, const PinChromeView &v, void *user) {
            ++static_cast<Calls *>(user)->pin;
            return v.pin != nullptr;
        };
        style.hooks = &hooks;
        frame();
        Check(calls.background > 0 && calls.pin > 0, "draw hooks are called for nodes and sockets");
        Check(CountVertices(ImGui::GetColorU32(blue)) < 4, "a hook that returns true replaces the socket glyph");
        style.hooks = nullptr;

        ImVec2 canvas{};
        auto at = [&](double x, double y) {
            return ImVec2{canvas.x + float((x - state.origin.x) * state.zoom), canvas.y + float((y - state.origin.y) * state.zoom)};
        };
        auto step = [&](ImVec2 mouse, bool down, bool ctrl = false, bool alt = false) {
            io.MousePos = mouse;
            io.MouseDown[0] = down;
            io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
            io.AddKeyEvent(ImGuiMod_Alt, alt);
            out.Clear();
            ImGui::NewFrame();
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({1000, 700});
            ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            auto f = BeginEditor("graph", graph, state, out, style, options);
            canvas = f.min;
            DrawLinks(f);
            DrawNodes(f);
            EndEditor(f);
            ImGui::End();
            ImGui::Render();
        };
        auto has = [&](EditKind kind, auto predicate) {
            for (auto &r : out.Requests())
                if (r.kind == kind && predicate(r))
                    return true;
            return false;
        };
        state.selection.clear();
        step({-50, -50}, false);
        ImVec2 inPin{}, needPin{};
        {
            ImGui::NewFrame();
            ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            auto f = BeginEditor("graph", graph, state, out, style, options);
            GetPinPosition(f, {2}, inPin);
            GetPinPosition(f, {3}, needPin);
            EndEditor(f);
            ImGui::End();
            ImGui::Render();
        }

        // Alt-click breaks the socket's links as one operation.
        step(inPin, false, false, true);
        step(inPin, true, false, true);
        Check(has(EditKind::DeleteLink, [](auto &r) { return r.link == LinkId{1}; }) && !state.connecting,
              "alt-click on a socket deletes its links");
        step(inPin, false);

        // Ctrl-drag carries the links of a socket to another socket of the same direction.
        graph.typeCompatibility = [](void *, std::uint64_t, std::uint64_t) { return ConnectionVerdict{}; };
        step(inPin, false, true);
        step(inPin, true, true);
        Check(state.movingLinks && state.connecting == PinId{2}, "ctrl-drag on a linked socket starts moving links");
        step(needPin, true, true);
        step(needPin, false, true);
        Check(has(EditKind::Reconnect,
                  [](auto &r) { return r.link == LinkId{1} && r.from == PinId{1} && r.to == PinId{3}; }),
              "dropping moves every link to the new socket");
        Check(!state.movingLinks && !state.connecting, "moving links ends after the drop");
        step({-50, -50}, false);

        // Double-clicking a link asks the host for a reroute knot at that point.
        const ImVec2 mid = at(320, 80);
        step(mid, false);
        step(mid, true);
        step(mid, false);
        step(mid, true);
        Check(has(EditKind::InsertReroute, [](auto &r) { return r.link == LinkId{1}; }),
              "double-clicking a link requests a reroute");
        step(mid, false);
        state.selectedLink = {};

        // Two selected nodes connect through their first free compatible sockets.
        state.selection = {{2}, {1}};
        pins[0].multiple = true; // The output already feeds the first link.
        {
            ImGui::NewFrame();
            ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            out.Clear();
            auto f = BeginEditor("graph", graph, state, out, style, options);
            Check(ConnectSelected(f) &&
                      has(EditKind::CreateLink, [](auto &r) { return r.from == PinId{1} && r.to == PinId{3}; }),
                  "ConnectSelected links left output to right free input");
            EndEditor(f);
            ImGui::End();
            ImGui::Render();
        }
        state.selection.clear();
        pins[0].multiple = false;

        // Dropping a node onto a link it can split inserts it there.
        step({-50, -50}, false);
        const ImVec2 knot = at(308, 308);
        step(knot, false);
        step(knot, true);
        step(at(316, 200), true);
        step(mid, true);
        Check(state.dropLink == LinkId{1}, "a dragged node over a link marks it for insertion");
        step(mid, false);
        Check(has(EditKind::Move, [](auto &r) { return r.phase == Phase::Commit && r.node == NodeId{3}; }) &&
                  has(EditKind::InsertNode, [](auto &r) { return r.node == NodeId{3} && r.link == LinkId{1}; }),
              "releasing over the link commits the move and inserts the node");
        step({-50, -50}, false);
        graph.typeCompatibility = nullptr;
    }

    // A closed panel gathers its sockets on its title row; hideUnusedPins removes unconnected sockets.
    {
        pins[2].panel = 7;
        bool open = false;
        ImVec2 gathered{}, titleCenter{};
        for (int i = 0; i < 2; ++i) { // Second frame: the title row position is known.
            ImGui::NewFrame();
            ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
            auto f = BeginEditor("graph", graph, state, out, style, options);
            if (BeginNode(f, {2})) {
                const auto start = ImGui::GetCursorScreenPos();
                const float height = std::max(ImGui::GetFrameHeight(), style.headerHeight * float(state.zoom) * .7f);
                titleCenter = {start.x, start.y + height * .5f};
                if (BeginNodePanel(f, 7, "More", &open))
                    EndNodePanel(f);
                EndNode(f);
            }
            GetPinPosition(f, {3}, gathered);
            EndEditor(f);
            ImGui::End();
            ImGui::Render();
        }
        Check(Near(gathered.y, titleCenter.y), "closed panel gathers its sockets on the title row");
        pins[2].panel = 0;
        const ImU32 blueU = ImGui::GetColorU32(blue);
        frame();
        const int shown = CountVertices(blueU);
        nodes[1].hideUnusedPins = true;
        frame();
        Check(CountVertices(blueU) < shown, "hideUnusedPins hides unconnected sockets");
        nodes[1].hideUnusedPins = false;
    }

    // Socket roles and flags take part in connection rules.
    {
        NodeView rn[] = {{{1}, {0, 0}, {100, 100}, "A"}, {{2}, {300, 0}, {100, 100}, "B"}};
        PinView rp[] = {{{1}, {1}, "out", PinKind::Output, Side::Right, 50, 1},
                        {{2}, {2}, "in", PinKind::Input, Side::Left, 50, 2},
                        {{3}, {1}, "then", PinKind::Output, Side::Right, 70}};
        rp[2].role = PinRole::Flow;
        GraphView rg;
        rg.nodes = rn;
        rg.pins = rp;
        Check(!CanConnect(rg, {1}, {2}).allowed, "different types are rejected without a callback");
        rp[1].flags = PinFlags_Wildcard;
        Check(CanConnect(rg, {1}, {2}).allowed, "a wildcard socket accepts any type");
        Check(!CanConnect(rg, {3}, {2}).allowed, "flow does not connect to data");
        rp[1].flags = PinFlags_Unavailable;
        rp[1].reason = "Only in volume mode";
        auto verdict = CanConnect(rg, {1}, {2});
        Check(!verdict.allowed && verdict.reason == "Only in volume mode", "unavailable socket reports its reason");
    }

    // Every glyph draws, and the state it encodes changes the drawing.
    {
        ImGui::NewFrame();
        ImGui::Begin("glyphs", nullptr, ImGuiWindowFlags_NoSavedSettings);
        auto *dl = ImGui::GetWindowDrawList();
        auto draw = [&](PinView pin, bool connected, bool hollow, int links = 0) {
            const int before = dl->VtxBuffer.Size;
            PinChromeView v;
            v.pin = &pin;
            v.center = {50, 50};
            v.radius = 6;
            v.seat = 2;
            v.color = {1, 1, 1, 1};
            v.canvas = {0, 0, 0, 1};
            v.connected = connected;
            v.hollow = hollow;
            v.links = links;
            DrawPinGlyph(dl, v);
            return dl->VtxBuffer.Size - before;
        };
        PinView flow{};
        flow.role = PinRole::Flow;
        Check(draw(flow, true, false) > 0 && draw(flow, true, false) != draw(flow, false, false),
              "flow glyph shows whether it is connected");
        PinView data{};
        Check(draw(data, false, true) != draw(data, true, true), "hollow data glyph gains a core when connected");
        PinView multi{};
        multi.multiple = true;
        Check(draw(multi, true, false, 3) > draw(multi, true, false, 1), "multi-input glyph shows one dot per link");
        PinView single{}, array{}, set{};
        array.container = PinContainer::Array;
        set.container = PinContainer::Set;
        const int singleCount = draw(single, true, true);
        Check(draw(array, true, true) > singleCount && draw(set, true, true) != singleCount,
              "array and set glyphs differ from a single value");
        PinView map{};
        map.container = PinContainer::Map;
        const ImU32 key = ImGui::GetColorU32(ImVec4{1, 0, 0, 1});
        const int before = dl->VtxBuffer.Size;
        PinChromeView mv;
        mv.pin = &map;
        mv.center = {50, 50};
        mv.radius = 6;
        mv.color = {1, 1, 1, 1};
        mv.keyColor = {1, 0, 0, 1};
        DrawPinGlyph(dl, mv);
        int keyed = 0;
        for (int i = before; i < dl->VtxBuffer.Size; ++i)
            keyed += dl->VtxBuffer[i].col == key;
        Check(keyed > 0, "map glyph shows the key type colour");
        for (auto flag : {PinFlags_Reference, PinFlags_Wildcard, PinFlags_Field, PinFlags_Optional, PinFlags_Unavailable}) {
            PinView flagged{};
            flagged.flags = flag;
            Check(draw(flagged, true, true) > 0, "flag glyph draws");
        }
        ImGui::End();
        ImGui::Render();
    }
    ImGui::DestroyContext();
}
} // namespace
int main() {
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    LayoutTests();
    SocketTests();
    InputTests();
    AppearanceTests();
    std::printf("node_editor: %d failure(s)\n", failures);
    return failures ? 1 : 0;
}
