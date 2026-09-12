#include <imkit/node_editor.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace imkit::node_editor {
namespace {
ImVec2 Add(ImVec2 a, ImVec2 b) {
    return {a.x + b.x, a.y + b.y};
}
ImVec2 Sub(ImVec2 a, ImVec2 b) {
    return {a.x - b.x, a.y - b.y};
}
ImVec2 Mul(ImVec2 a, float n) {
    return {a.x * n, a.y * n};
}
float Dot(ImVec2 a, ImVec2 b) {
    return a.x * b.x + a.y * b.y;
}
ImU32 Color(ImVec4 c) {
    return ImGui::GetColorU32(c);
}
void Text(std::string_view s) {
    ImGui::TextUnformatted(s.data(), s.data() + s.size());
}
const NodeView *Find(GraphView g, NodeId id) {
    auto i = std::find_if(g.nodes.begin(), g.nodes.end(), [&](auto &n) { return n.id == id; });
    return i == g.nodes.end() ? nullptr : &*i;
}
const PinView *FindPin(GraphView g, PinId id) {
    auto i = std::find_if(g.pins.begin(), g.pins.end(), [&](auto &p) { return p.id == id; });
    return i == g.pins.end() ? nullptr : &*i;
}
const NodeView *Find(const EditorFrame &f, NodeId id) {
    if (!f.editorId)
        return Find(f.graph, id);
    auto &index = f.state->nodeIndex;
    auto i = std::lower_bound(index.begin(), index.end(), id.value,
                              [](auto entry, auto value) { return entry.first < value; });
    return i != index.end() && i->first == id.value ? &f.graph.nodes[i->second] : nullptr;
}
const PinView *FindPin(const EditorFrame &f, PinId id) {
    if (!f.editorId)
        return FindPin(f.graph, id);
    auto &index = f.state->pinIndex;
    auto i = std::lower_bound(index.begin(), index.end(), id.value,
                              [](auto entry, auto value) { return entry.first < value; });
    return i != index.end() && i->first == id.value ? &f.graph.pins[i->second] : nullptr;
}
const NodeStyle &Style(const EditorFrame &f, const NodeView &n) {
    return n.style ? *n.style : f.style;
}
Point Position(const EditorFrame &f, const NodeView &n) {
    for (auto &r : f.state->gesture)
        if (r.node == n.id && r.kind == EditKind::Move)
            return r.after;
    return n.position;
}
Point Size(const EditorFrame &f, const NodeView &n) {
    if (n.collapsed)
        return {n.size.x, Style(f, n).headerHeight};
    for (auto &r : f.state->gesture)
        if (r.node == n.id && r.kind == EditKind::Resize)
            return r.after;
    return n.size;
}
ImVec2 Screen(const EditorFrame &f, Point p) {
    return {f.min.x + float((p.x - f.state->origin.x) * f.state->zoom),
            f.min.y + float((p.y - f.state->origin.y) * f.state->zoom)};
}
Point World(const EditorFrame &f, ImVec2 p) {
    return {f.state->origin.x + (p.x - f.min.x) / f.state->zoom,
            f.state->origin.y + (p.y - f.min.y) / f.state->zoom};
}
bool Inside(ImVec2 p, ImVec2 a, ImVec2 b) {
    return p.x >= a.x && p.y >= a.y && p.x <= b.x && p.y <= b.y;
}
bool Hidden(const EditorFrame &f, const NodeView &n) {
    NodeId parent = n.parent;
    for (std::size_t i = 0; parent && i < f.graph.nodes.size(); ++i) {
        auto *p = Find(f, parent);
        if (!p)
            return true;
        if (p->collapsed)
            return true;
        parent = p->parent;
    }
    return bool(parent);
}
bool Locked(const EditorFrame &f, const NodeView &n) {
    if (n.locked)
        return true;
    auto parent = n.parent;
    for (std::size_t i = 0; parent && i < f.graph.nodes.size(); ++i) {
        auto *p = Find(f, parent);
        if (!p)
            return true;
        if (p->locked)
            return true;
        parent = p->parent;
    }
    return bool(parent);
}
bool Visible(const EditorFrame &f, const NodeView &n) {
    if (Hidden(f, n))
        return false;
    const auto a = Screen(f, Position(f, n));
    const auto s = Size(f, n);
    auto b = Add(a, {float(s.x * f.state->zoom), float(s.y * f.state->zoom)});
    return b.x >= f.min.x && b.y >= f.min.y && a.x <= f.max.x && a.y <= f.max.y;
}
ImVec2 PinPosition(const EditorFrame &f, const PinView &pin) {
    auto *n = Find(f, pin.node);
    if (!n)
        return f.min;
    // Collapsed ancestors retain visible boundary sockets for external connections.
    auto *shown = n;
    auto parent = n->parent;
    for (std::size_t depth = 0; parent && depth < f.graph.nodes.size(); ++depth) {
        auto *p = Find(f, parent);
        if (!p)
            break;
        if (p->collapsed)
            shown = p;
        parent = p->parent;
    }
    auto p = Position(f, *shown), size = Size(f, *shown);
    const double offset = shown->collapsed ? Style(f, *shown).headerHeight / 2 : pin.offset;
    switch (pin.side) {
    case Side::Left:
        p.y += std::clamp(offset, 0., size.y);
        break;
    case Side::Right:
        p.x += size.x;
        p.y += std::clamp(offset, 0., size.y);
        break;
    case Side::Top:
        p.x += std::clamp(offset, 0., size.x);
        break;
    case Side::Bottom:
        p.x += std::clamp(offset, 0., size.x);
        p.y += size.y;
        break;
    }
    return Screen(f, p);
}
ImVec2 Direction(Side side) {
    switch (side) {
    case Side::Left:
        return {-1, 0};
    case Side::Right:
        return {1, 0};
    case Side::Top:
        return {0, -1};
    default:
        return {0, 1};
    }
}
float SegmentDistance(ImVec2 p, ImVec2 a, ImVec2 b) {
    auto v = Sub(b, a);
    float t = std::clamp(Dot(Sub(p, a), v) / std::max(.001f, Dot(v, v)), 0.f, 1.f);
    auto d = Sub(p, Add(a, Mul(v, t)));
    return std::sqrt(Dot(d, d));
}
std::array<ImVec2, 25> Curve(const EditorFrame &f, const PinView &a, const PinView &b,
                             const LinkStyle *overrideStyle = nullptr) {
    const auto style = overrideStyle ? *overrideStyle : f.style.linkStyle;
    auto p = PinPosition(f, a), q = PinPosition(f, b);
    float strength = std::max(40.f, float(std::abs(q.x - p.x) + std::abs(q.y - p.y)) * .4f);
    auto c = Add(p, Mul(Direction(a.side), strength)), d = Add(q, Mul(Direction(b.side), strength));
    std::array<ImVec2, 25> points{};
    for (int i = 0; i < 25; ++i) {
        float t = float(i) / 24, u = 1 - t;
        if (style == LinkStyle::Straight)
            points[i] = Add(Mul(p, u), Mul(q, t));
        else if (style == LinkStyle::Orthogonal) {
            auto mid = (p.x + q.x) / 2;
            if (t < 1.f / 3)
                points[i] = {p.x + (mid - p.x) * t * 3, p.y};
            else if (t < 2.f / 3)
                points[i] = {mid, p.y + (q.y - p.y) * (t * 3 - 1)};
            else
                points[i] = {mid + (q.x - mid) * (t * 3 - 2), q.y};
        } else
            points[i] = Add(Add(Mul(p, u * u * u), Mul(c, 3 * u * u * t)),
                            Add(Mul(d, 3 * u * t * t), Mul(q, t * t * t)));
    }
    return points;
}
EditRequest Request(EditorFrame &f, EditKind kind, NodeId node = {}) {
    EditRequest r;
    r.kind = kind;
    r.graph = f.graph.id;
    r.revision = f.graph.revision;
    r.operation = f.state->nextOperation++;
    r.node = node;
    return r;
}
bool EmitGesture(EditorFrame &f, Phase phase) {
    auto &s = *f.state;
    for (auto &r : s.gesture) {
        r.phase = phase;
        if (phase == Phase::Cancel)
            r.after = r.before;
    }
    return f.requests->PushBatch(s.gesture);
}
void Cancel(EditorFrame &f) {
    auto &s = *f.state;
    if (!s.gesture.empty()) {
        s.cancelPending = !EmitGesture(f, Phase::Cancel);
        if (!s.cancelPending)
            s.gesture.clear();
    }
    s.connecting = {};
    s.reconnecting = {};
    s.selecting = s.cutting = false;
    s.lasso.clear();
}
void BeginGesture(EditorFrame &f, const NodeView &n, EditKind kind) {
    auto &s = *f.state;
    if (f.options.readOnly || Locked(f, n) || !s.gesture.empty())
        return;
    std::vector<NodeId> ids = kind == EditKind::Resize ? std::vector<NodeId>{n.id} : s.selection;
    auto op = s.nextOperation++;
    for (auto &candidate : f.graph.nodes) {
        bool include = std::find(ids.begin(), ids.end(), candidate.id) != ids.end();
        if (kind == EditKind::Move) {
            auto parent = candidate.parent;
            for (std::size_t i = 0; parent && i < f.graph.nodes.size(); ++i) {
                if (std::find(ids.begin(), ids.end(), parent) != ids.end())
                    include = true;
                auto *p = Find(f, parent);
                if (!p)
                    break;
                parent = p->parent;
            }
        }
        if (include && !Locked(f, candidate)) {
            auto r = Request(f, kind, candidate.id);
            r.operation = op;
            r.before = r.after = kind == EditKind::Resize ? candidate.size : candidate.position;
            s.gesture.push_back(r);
        }
    }
    for (auto &r : s.gesture)
        r.operationSize = s.gesture.size();
    if (!EmitGesture(f, Phase::Begin))
        s.gesture.clear();
    s.press = World(f, ImGui::GetIO().MousePos);
    s.dragDelta = {};
}
bool Chord(ImGuiKeyChord chord) {
    const auto &io = ImGui::GetIO();
    auto mods = chord & ImGuiMod_Mask_;
    auto key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
    return io.KeyMods == mods && key != ImGuiKey_None && ImGui::IsKeyPressed(key, false);
}
PreviewState &GetPreview(EditorState &s, NodeId node) {
    auto i = std::find_if(s.previews.begin(), s.previews.end(),
                          [&](auto &p) { return p.node == node && p.graph == s.graph; });
    if (i != s.previews.end())
        return *i;
    s.previews.push_back({node, s.graph});
    return s.previews.back();
}
void DrawOutput(const PreviewOutput &o, ImVec2 size, bool interactive) {
    size.x = std::max(1.f, size.x);
    size.y = std::max(1.f, size.y);
    if (o.draw) {
        ImGui::BeginDisabled(!interactive);
        o.draw(o.user, size, interactive);
        ImGui::EndDisabled();
    } else if (o.texture.GetTexID() != ImTextureID{}) {
        auto cursor = ImGui::GetCursorScreenPos();
        float aspect = std::isfinite(o.aspect) && o.aspect > 0 ? o.aspect : 1;
        ImVec2 fitted{std::min(size.x, size.y * aspect), 0};
        fitted.y = fitted.x / aspect;
        ImGui::SetCursorScreenPos(Add(cursor, Mul(Sub(size, fitted), .5f)));
        ImGui::Image(o.texture, fitted, o.uv0, o.uv1);
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy(size);
    } else {
        Text(o.message.empty() ? std::string_view("No preview supplied") : o.message);
        ImGui::Dummy({1, std::max(1.f, size.y - ImGui::GetTextLineHeightWithSpacing())});
    }
    if (o.status != PreviewStatus::Ready) {
        const char *names[] = {"Ready", "Missing", "Waiting", "Stale", "Error"};
        ImGui::TextDisabled("%s", names[int(o.status)]);
    }
}
void ApplyLayout(EditorFrame &f, int action) {
    std::vector<PositionChange> changes(f.graph.nodes.size());
    LayoutResult result;
    if (action < 6)
        result = AlignNodes(f.graph, f.state->selection, changes, {static_cast<Alignment>(action)});
    else if (action < 8)
        result = DistributeNodes(f.graph, f.state->selection, changes,
                                 action == 6 ? Axis::Horizontal : Axis::Vertical);
    else if (action == 8)
        result = ArrangeNodes(f.graph, f.state->selection, changes);
    else
        result = SnapNodesToGrid(f.graph, f.state->selection, changes, f.style.gridSize);
    if (result)
        QueueLayout(f.graph, std::span(changes).first(result.count), *f.requests, f.state->nextOperation++);
}
} // namespace
NodeStyle MakeNodeStyle(const Theme &t) {
    NodeStyle s;
    s.canvas = t.semantic.canvas;
    s.grid = t.semantic.border;
    s.grid.w = .3f;
    s.surface = t.semantic.surface;
    s.header = t.semantic.surfaceRaised;
    s.text = t.semantic.text;
    s.muted = t.semantic.textSecondary;
    s.border = t.semantic.border;
    s.selected = t.semantic.control.selected;
    s.accent = t.semantic.accent;
    s.group = t.semantic.accent;
    s.group.w = .10f;
    s.preview = t.semantic.canvas;
    s.error = t.semantic.error;
    s.warning = t.semantic.warning;
    s.success = t.semantic.success;
    s.rounding = t.radius.overlay;
    s.borderWidth = t.stroke.border;
    s.padding = t.metrics.horizontalPadding;
    s.headerHeight = t.metrics.controlHeight + 4;
    s.animate = t.motion.enabled && !t.motion.reducedMotion;
    return s;
}
void EditorState::Reserve(std::size_t nodes, std::size_t previewCount) {
    selection.reserve(nodes);
    gesture.reserve(nodes);
    nodeIndex.reserve(nodes);
    pinIndex.reserve(nodes * 4);
    lasso.reserve(512);
    previews.reserve(previewCount);
    history.reserve(64);
    graphViews.reserve(16);
}
bool IsSelected(const EditorState &s, NodeId id) {
    return std::find(s.selection.begin(), s.selection.end(), id) != s.selection.end();
}
void Select(EditorState &s, NodeId id, bool additive, bool toggle) {
    if (!additive && !toggle)
        s.selection.clear();
    auto i = std::find(s.selection.begin(), s.selection.end(), id);
    if (i != s.selection.end()) {
        if (toggle)
            s.selection.erase(i);
    } else if (id)
        s.selection.push_back(id);
    s.active = IsSelected(s, id) ? id : (s.selection.empty() ? NodeId{} : s.selection.back());
    s.selectedLink = {};
}
ViewState CaptureView(const EditorState &s) {
    return {s.graph, s.origin, s.zoom, s.selection};
}
void RestoreView(EditorState &s, const ViewState &v) {
    if (!s.gesture.empty())
        return;
    if (!std::isfinite(v.zoom) || v.zoom <= 0 || !std::isfinite(v.origin.x) || !std::isfinite(v.origin.y))
        return;
    s.graph = v.graph;
    s.origin = v.origin;
    s.zoom = std::clamp(v.zoom, .05, 8.);
    s.selection = v.selection;
    s.active = s.selection.empty() ? NodeId{} : s.selection.back();
    s.connecting = {};
    s.reconnecting = {};
    s.selecting = s.cutting = false;
}
void RememberView(EditorState &s) {
    if (!s.history.empty() && s.historyIndex + 1 < s.history.size())
        s.history.resize(s.historyIndex + 1);
    s.history.push_back(CaptureView(s));
    if (s.history.size() > 64)
        s.history.erase(s.history.begin());
    s.historyIndex = s.history.size() - 1;
}
bool NavigateHistory(EditorState &s, int direction) {
    if (s.history.empty() || !s.gesture.empty() || !direction)
        return false;
    auto next = static_cast<long long>(s.historyIndex) + (direction < 0 ? -1 : 1);
    if (next < 0 || next >= static_cast<long long>(s.history.size()))
        return false;
    s.historyIndex = std::size_t(next);
    RestoreView(s, s.history[s.historyIndex]);
    return true;
}
bool AddBookmark(EditorState &s, std::string_view label) {
    if (s.bookmarks.size() >= 64 || label.size() >= 64)
        return false;
    Bookmark b;
    b.view = CaptureView(s);
    std::copy(label.begin(), label.end(), b.label.begin());
    s.bookmarks.push_back(std::move(b));
    return true;
}
void SetZoom(EditorState &s, double zoom, Point anchor) {
    if (!std::isfinite(zoom) || zoom <= 0 || !std::isfinite(anchor.x) || !std::isfinite(anchor.y) ||
        s.zoom <= 0)
        return;
    zoom = std::clamp(zoom, .05, 8.);
    s.origin.x += anchor.x / s.zoom - anchor.x / zoom;
    s.origin.y += anchor.y / s.zoom - anchor.y / zoom;
    s.zoom = zoom;
}
bool FrameNodes(EditorState &s, GraphView g, std::span<const NodeId> ids, Point size, double padding) {
    if (g.nodes.empty() || !std::isfinite(size.x) || !std::isfinite(size.y) || !std::isfinite(padding) ||
        padding < 0 || size.x <= padding * 2 || size.y <= padding * 2)
        return false;
    Rect b{};
    bool found = false;
    for (auto &n : g.nodes)
        if (ids.empty() || std::find(ids.begin(), ids.end(), n.id) != ids.end()) {
            if (!std::isfinite(n.position.x) || !std::isfinite(n.position.y) || n.size.x <= 0 ||
                n.size.y <= 0)
                return false;
            Rect r{n.position, {n.position.x + n.size.x, n.position.y + n.size.y}};
            if (!found)
                b = r;
            else {
                b.min = {std::min(b.min.x, r.min.x), std::min(b.min.y, r.min.y)};
                b.max = {std::max(b.max.x, r.max.x), std::max(b.max.y, r.max.y)};
            }
            found = true;
        }
    if (!found)
        return false;
    s.zoom = std::clamp(std::min((size.x - 2 * padding) / std::max(1., b.max.x - b.min.x),
                                 (size.y - 2 * padding) / std::max(1., b.max.y - b.min.y)),
                        .15, 3.);
    s.origin = {(b.min.x + b.max.x - size.x / s.zoom) / 2, (b.min.y + b.max.y - size.y / s.zoom) / 2};
    return true;
}
EditorFrame BeginEditor(const char *id, GraphView graph, EditorState &s, RequestBuffer &out,
                        const NodeStyle &style, EditorOptions options) {
    EditorFrame f;
    f.graph = graph;
    f.state = &s;
    f.requests = &out;
    f.style = style;
    f.options = options;
    f.context = ImGui::GetCurrentContext();
    f.fontSize = ImGui::GetStyle().FontSizeBase;
    f.editorId = ImGui::GetID(id);
    s.nodeIndex.clear();
    s.pinIndex.clear();
    for (std::size_t i = 0; i < graph.nodes.size(); ++i)
        s.nodeIndex.emplace_back(graph.nodes[i].id.value, i);
    for (std::size_t i = 0; i < graph.pins.size(); ++i)
        s.pinIndex.emplace_back(graph.pins[i].id.value, i);
    std::sort(s.nodeIndex.begin(), s.nodeIndex.end());
    std::sort(s.pinIndex.begin(), s.pinIndex.end());
    for (auto *index : {&s.nodeIndex, &s.pinIndex})
        for (std::size_t i = 0; i < index->size(); ++i)
            if (!(*index)[i].first || (i && (*index)[i - 1].first == (*index)[i].first))
                f.error = "Invalid or duplicate stable ID";
    for (auto &n : graph.nodes) {
        if (!std::isfinite(n.position.x) || !std::isfinite(n.position.y) || !std::isfinite(n.size.x) ||
            !std::isfinite(n.size.y) || n.size.x <= 0 || n.size.y <= 0)
            f.error = "Invalid node bounds";
        auto parent = n.parent;
        for (std::size_t depth = 0; parent; ++depth) {
            auto *p = Find(f, parent);
            if (!p || depth >= graph.nodes.size()) {
                f.error = "Invalid or cyclic containment";
                break;
            }
            parent = p->parent;
        }
    }
    for (auto &pin : graph.pins)
        if (!Find(f, pin.node) || !std::isfinite(pin.offset))
            f.error = "Invalid socket owner or offset";
    if (!std::isfinite(s.zoom) || s.zoom <= 0)
        s.zoom = 1;
    if (!std::isfinite(s.origin.x) || !std::isfinite(s.origin.y))
        s.origin = {};
    if (!s.gesture.empty() &&
        (s.gesture.front().graph != graph.id || s.gesture.front().revision != graph.revision ||
         s.cancelPending || options.readOnly))
        Cancel(f);
    if (!s.gesture.empty())
        for (auto &r : s.gesture)
            if (!Find(f, r.node)) {
                Cancel(f);
                break;
            }
    if (s.graph != graph.id && s.gesture.empty()) {
        if (s.graph) {
            auto i = std::find_if(s.graphViews.begin(), s.graphViews.end(),
                                  [&](auto &v) { return v.graph == s.graph; });
            if (i == s.graphViews.end())
                s.graphViews.push_back(CaptureView(s));
            else
                *i = CaptureView(s);
        }
        auto i = std::find_if(s.graphViews.begin(), s.graphViews.end(),
                              [&](auto &v) { return v.graph == graph.id; });
        if (i == s.graphViews.end())
            RestoreView(s, {graph.id, {}, 1, {}});
        else
            RestoreView(s, *i);
    }
    std::erase_if(s.selection, [&](NodeId node) { return !Find(graph, node); });
    if (!Find(graph, s.active))
        s.active = s.selection.empty() ? NodeId{} : s.selection.back();
    if (s.connecting && !FindPin(graph, s.connecting)) {
        s.connecting = {};
        s.reconnecting = {};
    }
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, style.canvas);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    f.visible = ImGui::BeginChild("##canvas", options.size, ImGuiChildFlags_Borders,
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    f.min = ImGui::GetCursorScreenPos();
    f.max = Add(f.min, ImGui::GetContentRegionAvail());
    // Public last-item flags expose inherited BeginDisabled without internal APIs.
    ImGui::Dummy({0, 0});
    if ((ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0) {
        f.options.readOnly = true;
        f.blocked = true;
        Cancel(f);
    }
    ImGui::SetCursorScreenPos(f.min);
    f.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    ImGui::GetWindowDrawList()->PushClipRect(f.min, f.max, true);
    if (!f.error.empty()) {
        Cancel(f);
        Text(f.error);
        f.visible = false;
        return f;
    }
    if (!f.visible)
        return f;
    auto &io = ImGui::GetIO();
    if (!s.gesture.empty() && !s.cancelPending) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            Cancel(f);
        else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            auto p = World(f, io.MousePos);
            Point delta{p.x - s.press.x, p.y - s.press.y};
            for (auto &r : s.gesture) {
                r.after = {r.before.x + delta.x, r.before.y + delta.y};
                if (r.kind == EditKind::Move && options.snap && style.gridSize > 0) {
                    auto &first = s.gesture.front();
                    auto dx = std::round((first.before.x + delta.x) / style.gridSize) * style.gridSize -
                              first.before.x;
                    auto dy = std::round((first.before.y + delta.y) / style.gridSize) * style.gridSize -
                              first.before.y;
                    r.after = {r.before.x + dx, r.before.y + dy};
                }
                if (r.kind == EditKind::Resize) {
                    r.after.x = std::max(double(style.minimumWidth), r.after.x);
                    r.after.y = std::max(double(style.minimumHeight), r.after.y);
                }
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (EmitGesture(f, Phase::Commit))
                    s.gesture.clear();
            } else if (delta.x != s.dragDelta.x || delta.y != s.dragDelta.y) {
                EmitGesture(f, Phase::Update);
                s.dragDelta = delta;
            }
        } else { // Retry a terminal batch that could not fit on the release frame.
            if (EmitGesture(f, Phase::Commit))
                s.gesture.clear();
        }
    }
    if (options.grid && std::isfinite(style.gridSize) && style.gridSize > 0) {
        double step = style.gridSize * s.zoom;
        while (step < 12)
            step *= 2;
        auto *dl = ImGui::GetWindowDrawList();
        for (double x = f.min.x - std::fmod(s.origin.x * s.zoom, step); x < f.max.x; x += step)
            dl->AddLine({float(x), f.min.y}, {float(x), f.max.y}, Color(style.grid));
        for (double y = f.min.y - std::fmod(s.origin.y * s.zoom, step); y < f.max.y; y += step)
            dl->AddLine({f.min.x, float(y)}, {f.max.x, float(y)}, Color(style.grid));
    }
    // Draw all node silhouettes before links and interactive child windows.
    auto *dl = ImGui::GetWindowDrawList();
    for (auto &n : graph.nodes)
        if (Visible(f, n)) {
            auto &st = Style(f, n);
            auto a = Screen(f, Position(f, n));
            auto size = Size(f, n);
            auto b = Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)});
            const bool group = n.kind == NodeKind::Frame || n.kind == NodeKind::Group;
            if (st.shadows && !group)
                dl->AddRectFilled(Add(a, {2, 3}), Add(b, {2, 3}), IM_COL32(0, 0, 0, 32), st.rounding);
            dl->AddRectFilled(a, b, Color(group ? st.group : st.surface), st.rounding);
            dl->AddRectFilled(a, {b.x, a.y + float(st.headerHeight * s.zoom)}, Color(st.header), st.rounding,
                              ImDrawFlags_RoundCornersTop);
            dl->AddRect(a, b, Color(IsSelected(s, n.id) ? st.accent : st.border), st.rounding, 0,
                        IsSelected(s, n.id) ? 2 : st.borderWidth);
            dl->AddLine({a.x + st.rounding, a.y + 1}, {b.x - st.rounding, a.y + 1}, Color(st.accent), 2);
        }
    return f;
}
bool BeginNode(EditorFrame &f, NodeId id) {
    IM_ASSERT(!f.nodeOpen && f.context == ImGui::GetCurrentContext());
    auto *n = Find(f, id);
    if (!f.visible || !n || Hidden(f, *n))
        return false;
    const bool interacting =
        std::any_of(f.state->gesture.begin(), f.state->gesture.end(), [&](auto &r) { return r.node == id; });
    if (!Visible(f, *n) && !interacting && f.state->active != id)
        return false;
    f.node = n;
    auto &s = *f.state;
    auto &st = Style(f, *n);
    const float z = float(s.zoom);
    auto a = Screen(f, Position(f, *n));
    auto size = Size(f, *n);
    ImVec2 px{float(size.x * z), float(size.y * z)};
    char key[40];
    std::snprintf(key, sizeof(key), "node-%llu", static_cast<unsigned long long>(id.value));
    ImGui::PushID(key);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(st.padding * z, st.padding * z));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Mul(ImGui::GetStyle().FramePadding, z));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Mul(ImGui::GetStyle().ItemSpacing, z));
    ImGui::PushStyleColor(ImGuiCol_Text, st.text);
    ImGui::PushFont(nullptr, f.fontSize * z);
    bool open = ImGui::BeginChild("##body", px, ImGuiChildFlags_AlwaysUseWindowPadding,
                                  ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse);
    if (!open) {
        ImGui::EndChild();
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        ImGui::PopID();
        f.node = nullptr;
        return false;
    }
    f.nodeOpen = true;
    auto *dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(Add(a, {st.padding * z, 2 * z}));
    ImGui::InvisibleButton(
        "##move", {std::max(1.f, px.x - 2 * st.padding * z), std::max(1.f, st.headerHeight * z - 4 * z)});
    auto title = std::string(n->icon);
    if (!title.empty())
        title += ' ';
    title.append(n->title);
    dl->PushClipRect(Add(a, {st.padding * z, 0}), {a.x + px.x - st.padding * z, a.y + st.headerHeight * z},
                     true);
    dl->AddText(Add(a, {st.padding * z, (st.headerHeight * z - ImGui::GetFontSize()) / 2}), Color(st.text),
                title.c_str());
    dl->PopClipRect();
    if (ImGui::IsItemClicked()) {
        if (!IsSelected(s, id) || ImGui::GetIO().KeyCtrl)
            Select(s, id, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
        else
            s.active = id;
        if (!ImGui::GetIO().KeyCtrl)
            BeginGesture(f, *n, EditKind::Move);
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        Cancel(f);
        if (n->childGraph)
            QueueCommand(f, EditKind::EnterGraph, id);
        else if (s.zoom < st.detailZoom) {
            FrameNodes(s, f.graph, {&id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            RememberView(s);
        } else
            QueueCommand(f, EditKind::Collapse, id);
    }
    if (ImGui::IsItemHovered() && !n->description.empty())
        ImGui::SetTooltip("%.*s", int(n->description.size()), n->description.data());
    if (n->locked)
        dl->AddCircleFilled({a.x + px.x - 6 * z, a.y + 6 * z}, 2 * z, Color(st.muted));
    ImGui::SetCursorScreenPos(Add(a, {st.padding * z, st.headerHeight * z + st.padding * z}));
    ImGui::PushItemWidth(std::max(1.f, px.x - 2 * st.padding * z));
    ImGui::BeginDisabled(f.options.readOnly || Locked(f, *n) || n->collapsed || s.zoom < st.detailZoom);
    return true;
}
void EndNode(EditorFrame &f) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    ImGui::EndDisabled();
    ImGui::PopItemWidth();
    auto *n = f.node;
    auto &st = Style(f, *n);
    float z = float(f.state->zoom);
    auto a = Screen(f, Position(f, *n));
    auto size = Size(f, *n);
    auto b = Add(a, {float(size.x * z), float(size.y * z)});
    if (!n->collapsed && f.state->zoom >= st.detailZoom && n->kind != NodeKind::Frame &&
        n->kind != NodeKind::Group) {
        auto *dl = ImGui::GetWindowDrawList();
        const char *names[] = {"Idle",  "Waiting",  "Running",  "Success",
                               "Error", "Disabled", "Bypassed", "Cached"};
        auto color = n->status == Status::Error     ? st.error
                     : n->status == Status::Success ? st.success
                                                    : st.muted;
        char text[96];
        std::snprintf(text, sizeof(text), "%s   %.2f ms", names[int(n->status)], n->milliseconds);
        dl->AddText({a.x + st.padding * z, b.y - 20 * z}, Color(color), text);
        if (n->status == Status::Running)
            dl->AddLine({a.x, b.y - 2}, {a.x + (b.x - a.x) * std::clamp(n->progress, 0.f, 1.f), b.y - 2},
                        Color(st.accent), 3);
        if (n->breakpoint)
            dl->AddCircleFilled({b.x - 14 * z, a.y + 16 * z}, 4 * z, Color(st.error));
    }
    if (!n->collapsed && !Locked(f, *n) && !f.options.readOnly) {
        ImGui::SetCursorScreenPos({b.x - 14, b.y - 14});
        ImGui::InvisibleButton("##resize", {14, 14});
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsItemClicked())
            BeginGesture(f, *n, EditKind::Resize);
        ImGui::GetWindowDrawList()->AddLine({b.x - 11, b.y - 3}, {b.x - 3, b.y - 11}, Color(st.muted));
    }
    f.blocked = f.blocked || ImGui::IsAnyItemActive();
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive() &&
        ImGui::IsMouseClicked(0))
        Select(*f.state, n->id, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
    ImGui::EndChild();
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();
    f.nodeOpen = false;
    f.node = nullptr;
}
void BeginPin(EditorFrame &f, PinId id) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    f.pinOpen = true;
    char key[40];
    std::snprintf(key, sizeof(key), "pin-%llu", static_cast<unsigned long long>(id.value));
    ImGui::PushID(key);
    ImGui::BeginGroup();
    auto *pin = FindPin(f, id);
    if (pin && pin->node == f.node->id && !pin->hidden)
        Text(pin->label);
}
void EndPin(EditorFrame &f) {
    IM_ASSERT(f.pinOpen);
    ImGui::EndGroup();
    ImGui::PopID();
    f.pinOpen = false;
}
void Link(EditorFrame &f, const LinkView &link) {
    if (!f.visible)
        return;
    auto *a = FindPin(f, link.from);
    auto *b = FindPin(f, link.to);
    if (!a || !b || a->hidden || b->hidden)
        return;
    if (!Find(f, a->node) || !Find(f, b->node))
        return;
    auto points = Curve(f, *a, *b, link.style);
    auto color = link.color.w ? link.color : f.style.accent;
    if (f.state->selectedLink == link.id)
        color = f.style.selected;
    auto *dl = ImGui::GetWindowDrawList();
    for (std::size_t i = 1; i < points.size(); ++i)
        if (!link.reference || i % 2)
            dl->AddLine(points[i - 1], points[i], Color(color),
                        link.width > 0 ? link.width : f.style.linkWidth);
    const auto tip = points[18];
    auto direction = Sub(points[18], points[17]);
    const auto length = std::sqrt(Dot(direction, direction));
    if (length > 0) {
        direction = Mul(direction, 1 / length);
        auto normal = ImVec2{-direction.y, direction.x};
        dl->AddTriangleFilled(tip, Add(Sub(tip, Mul(direction, 7)), Mul(normal, 3)),
                              Sub(Sub(tip, Mul(direction, 7)), Mul(normal, 3)), Color(color));
    }
    if (!link.label.empty())
        dl->AddText(points[12], Color(f.style.muted), link.label.data(),
                    link.label.data() + link.label.size());
    if (link.active && f.style.animate) {
        auto i = std::size_t(std::fmod(ImGui::GetTime() * 12, 24));
        dl->AddCircleFilled(points[i], 4, Color(f.style.success));
    }
}
void DrawLinks(EditorFrame &f) {
    for (auto &link : f.graph.links)
        Link(f, link);
}
void DrawNodes(EditorFrame &f, void (*body)(void *, EditorFrame &, const NodeView &), void *user) {
    for (auto &n : f.graph.nodes)
        if (BeginNode(f, n.id)) {
            if (!n.collapsed && f.state->zoom >= Style(f, n).detailZoom) {
                if (body)
                    body(user, f, n);
                else if (n.kind == NodeKind::Note)
                    Text(n.description);
            }
            EndNode(f);
        }
}
bool QueueCommand(EditorFrame &f, EditKind kind, NodeId node) {
    const bool nonEdit = kind == EditKind::Copy || kind == EditKind::EnterGraph || kind == EditKind::Watch;
    if (f.options.readOnly && !nonEdit)
        return false;
    if ((kind == EditKind::Run || kind == EditKind::Stop) && !f.options.canRun)
        return false;
    if ((kind == EditKind::Step || kind == EditKind::Breakpoint) && !f.options.canDebug)
        return false;
    if (node) {
        auto *n = Find(f, node);
        if (!n || (!nonEdit && kind != EditKind::Lock && Locked(f, *n)))
            return false;
        auto r = Request(f, kind, node);
        return f.requests->Push(r);
    }
    if (kind == EditKind::Undo || kind == EditKind::Redo || kind == EditKind::Paste ||
        kind == EditKind::CreateNode || kind == EditKind::Stop)
        return f.requests->Push(Request(f, kind));
    std::vector<EditRequest> batch;
    auto operation = f.state->nextOperation++;
    for (auto id : f.state->selection) {
        auto *n = Find(f, id);
        if (!n || (!nonEdit && kind != EditKind::Lock && Locked(f, *n)))
            continue;
        auto r = Request(f, kind, id);
        r.operation = operation;
        batch.push_back(r);
    }
    for (auto &r : batch)
        r.operationSize = batch.size();
    return f.requests->PushBatch(batch);
}
bool InsertNode(EditorFrame &f, NodeId node, LinkId id) {
    auto *n = Find(f, node);
    if (!n || Locked(f, *n) || f.options.readOnly)
        return false;
    auto it =
        std::find_if(f.graph.links.begin(), f.graph.links.end(), [&](auto &link) { return link.id == id; });
    if (it == f.graph.links.end())
        return false;
    auto *source = FindPin(f, it->from);
    auto *target = FindPin(f, it->to);
    if (!source || !target || source->node == node || target->node == node)
        return false;
    for (auto &input : f.graph.pins)
        if (input.node == node && input.kind == PinKind::Input &&
            CanConnect(f.graph, source->id, input.id, id).allowed)
            for (auto &output : f.graph.pins)
                if (output.node == node && output.kind == PinKind::Output &&
                    CanConnect(f.graph, output.id, target->id, id).allowed) {
                    auto r = Request(f, EditKind::InsertNode, node);
                    r.link = id;
                    r.from = input.id;
                    r.to = output.id;
                    return f.requests->Push(r);
                }
    return false;
}
void LayoutToolbar(EditorFrame &f) {
    ImGui::BeginDisabled(f.options.readOnly || f.state->selection.empty());
    if (ImGui::BeginCombo("Layout", "Align / distribute / arrange")) {
        const char *labels[] = {"Align left",
                                "Align center X",
                                "Align right",
                                "Align top",
                                "Align center Y",
                                "Align bottom",
                                "Distribute horizontally",
                                "Distribute vertically",
                                "Arrange by connections",
                                "Snap to grid"};
        for (int i = 0; i < 10; ++i)
            if (ImGui::Selectable(labels[i]))
                ApplyLayout(f, i);
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
}
void BackgroundImage(EditorFrame &f, ImTextureRef texture, Rect bounds, ImVec4 tint) {
    if (!f.visible || texture.GetTexID() == ImTextureID{})
        return;
    ImGui::GetWindowDrawList()->AddImage(texture, Screen(f, bounds.min), Screen(f, bounds.max), {0, 0},
                                         {1, 1}, Color(tint));
}
void Annotation(EditorFrame &f, std::span<const Point> points, ImVec4 color, float width) {
    if (!f.visible || width <= 0 || !std::isfinite(width))
        return;
    for (std::size_t i = 1; i < points.size(); ++i)
        ImGui::GetWindowDrawList()->AddLine(Screen(f, points[i - 1]), Screen(f, points[i]), Color(color),
                                            width);
}
void NodeSearch(EditorFrame &f) {
    auto &s = *f.state;
    ImGui::InputTextWithHint("##node-search", "Find in graph", s.search.data(), s.search.size());
    if (s.search[0])
        for (auto &n : f.graph.nodes) {
            auto query = std::string_view(s.search.data());
            auto match = [&](std::string_view text) {
                return std::search(text.begin(), text.end(), query.begin(), query.end(),
                                   [](unsigned char a, unsigned char b) {
                                       return std::tolower(a) == std::tolower(b);
                                   }) != text.end();
            };
            if (!match(n.title) && !match(n.description))
                continue;
            auto label = std::string(n.title) + "##search-" + std::to_string(n.id.value);
            if (ImGui::Selectable(label.c_str(), IsSelected(s, n.id))) {
                Select(s, n.id);
                FrameNodes(s, f.graph, {&n.id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            }
        }
    if (ImGui::SmallButton("Back"))
        NavigateHistory(s, -1);
    ImGui::SameLine();
    if (ImGui::SmallButton("Forward"))
        NavigateHistory(s, 1);
    if (s.active) {
        auto trace = [&](bool upstream) {
            std::vector<NodeId> nodes(f.graph.nodes.size());
            auto result = TraceNodes(f.graph, s.active, upstream, nodes);
            if (result)
                s.selection.assign(nodes.begin(), nodes.begin() + result.count);
        };
        if (ImGui::SmallButton("Select upstream"))
            trace(true);
        ImGui::SameLine();
        if (ImGui::SmallButton("Select downstream"))
            trace(false);
    }
}
void NodeInspector(EditorFrame &f, NodeId id) {
    auto *n = Find(f, id);
    if (!n)
        return;
    char title[128]{};
    std::copy_n(n->title.data(), std::min(n->title.size(), sizeof(title) - 1), title);
    ImGui::BeginDisabled(f.options.readOnly || Locked(f, *n));
    if (ImGui::InputText("Name", title, sizeof(title), ImGuiInputTextFlags_EnterReturnsTrue)) {
        auto r = Request(f, EditKind::Rename, id);
        std::copy(std::begin(title), std::end(title), r.text.begin());
        f.requests->Push(r);
    }
    if (ImGui::SmallButton("Collapse / expand"))
        QueueCommand(f, EditKind::Collapse, id);
    ImGui::SameLine();
    if (ImGui::SmallButton("Bypass"))
        QueueCommand(f, EditKind::Bypass, id);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(f.options.readOnly);
    if (ImGui::SmallButton(n->locked ? "Unlock" : "Lock"))
        QueueCommand(f, EditKind::Lock, id);
    if (n->kind == NodeKind::Group || n->kind == NodeKind::Frame) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit contents")) {
            Rect bounds;
            if (FitGroupToContents(f.graph, id, bounds, f.style.padding)) {
                auto a = Request(f, EditKind::Move, id);
                auto b = Request(f, EditKind::Resize, id);
                a.before = n->position;
                a.after = bounds.min;
                b.before = n->size;
                b.after = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y};
                b.operation = a.operation;
                a.operationSize = b.operationSize = 2;
                EditRequest requests[] = {a, b};
                f.requests->PushBatch(requests);
            }
        }
    }
    int index = 0;
    for (auto &pin : f.graph.pins)
        if (pin.node == id) {
            char key[40];
            std::snprintf(key, sizeof(key), "socket-%llu", static_cast<unsigned long long>(pin.id.value));
            ImGui::PushID(key);
            bool shown = !pin.hidden;
            if (ImGui::Checkbox("##visible", &shown)) {
                auto r = Request(f, EditKind::HidePin, id);
                r.from = pin.id;
                r.index = shown ? 0 : 1;
                f.requests->Push(r);
            }
            ImGui::SameLine();
            Text(pin.label);
            auto reorder = [&](int destination) {
                auto r = Request(f, EditKind::ReorderPin, id);
                r.from = pin.id;
                r.index = destination;
                f.requests->Push(r);
            };
            ImGui::SameLine();
            ImGui::BeginDisabled(index == 0);
            if (ImGui::SmallButton("Up"))
                reorder(index - 1);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::SmallButton("Down"))
                reorder(index + 1);
            ImGui::SameLine();
            if (ImGui::SmallButton("Expose")) {
                auto r = Request(f, EditKind::ExposePin, id);
                r.from = pin.id;
                f.requests->Push(r);
            }
            if (pin.internal)
                ImGui::TextDisabled("Internal socket: %llu",
                                    static_cast<unsigned long long>(pin.internal.value));
            ImGui::PopID();
            ++index;
        }
    if (f.options.canDebug) {
        if (ImGui::SmallButton(n->breakpoint ? "Remove breakpoint" : "Add breakpoint"))
            QueueCommand(f, EditKind::Breakpoint, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Watch value"))
            QueueCommand(f, EditKind::Watch, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Step"))
            QueueCommand(f, EditKind::Step, id);
    }
    if (f.options.canRun) {
        if (ImGui::SmallButton("Run node"))
            QueueCommand(f, EditKind::Run, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Stop"))
            QueueCommand(f, EditKind::Stop);
    }
    ImGui::EndDisabled();
}
void ExposedProperties(EditorFrame &f, NodeId node, std::span<const PropertyView> properties) {
    ImGui::BeginDisabled(f.options.readOnly);
    for (std::size_t i = 0; i < properties.size(); ++i) {
        auto &property = properties[i];
        ImGui::PushID(int(i));
        bool exposed = property.exposed;
        if (ImGui::Checkbox(std::string(property.label).c_str(), &exposed)) {
            auto r = Request(f, EditKind::ExposeProperty, node);
            r.type = property.id;
            r.index = exposed ? 1 : 0;
            f.requests->Push(r);
        }
        auto reorder = [&](int index) {
            auto r = Request(f, EditKind::ReorderProperty, node);
            r.type = property.id;
            r.index = index;
            f.requests->Push(r);
        };
        ImGui::SameLine();
        ImGui::BeginDisabled(i == 0);
        if (ImGui::SmallButton("Up"))
            reorder(int(i) - 1);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(i + 1 == properties.size());
        if (ImGui::SmallButton("Down"))
            reorder(int(i) + 1);
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::EndDisabled();
}
void NodePalette(EditorFrame &f, std::span<const PaletteEntry> entries) {
    if (f.state->palette) {
        ImGui::OpenPopup("Add node");
        f.state->palette = false;
    }
    if (ImGui::BeginPopup("Add node")) {
        f.blocked = true;
        ImGui::SetNextItemWidth(280);
        ImGui::InputTextWithHint("##search", "Search nodes", f.state->search.data(), f.state->search.size());
        std::vector<const PaletteEntry *> ordered;
        for (auto &e : entries)
            ordered.push_back(&e);
        auto rank = [&](std::uint64_t type) {
            if (std::find(f.state->favorites.begin(), f.state->favorites.end(), type) !=
                f.state->favorites.end())
                return std::size_t(0);
            auto i = std::find(f.state->recent.begin(), f.state->recent.end(), type);
            return i == f.state->recent.end() ? std::size_t(100)
                                              : std::size_t(i - f.state->recent.begin() + 1);
        };
        std::stable_sort(ordered.begin(), ordered.end(),
                         [&](auto *a, auto *b) { return rank(a->type) < rank(b->type); });
        for (auto *pointer : ordered) {
            const auto &entry = *pointer;
            auto matches = [&](std::string_view value) {
                auto query = std::string_view(f.state->search.data());
                return std::search(value.begin(), value.end(), query.begin(), query.end(),
                                   [](unsigned char a, unsigned char b) {
                                       return std::tolower(a) == std::tolower(b);
                                   }) != value.end() ||
                       query.empty();
            };
            if (!matches(entry.title) && !matches(entry.category))
                continue;
            if (f.state->connecting && entry.compatible && !entry.compatible(entry.user, f.state->connecting))
                continue;
            char key[40];
            std::snprintf(key, sizeof(key), "type-%llu", static_cast<unsigned long long>(entry.type));
            ImGui::PushID(key);
            bool favorite = std::find(f.state->favorites.begin(), f.state->favorites.end(), entry.type) !=
                            f.state->favorites.end();
            if (ImGui::SmallButton(favorite ? "*" : "+")) {
                if (favorite)
                    std::erase(f.state->favorites, entry.type);
                else
                    f.state->favorites.push_back(entry.type);
            }
            ImGui::SameLine();
            auto title = std::string(entry.category) + " / " + std::string(entry.title);
            ImGui::BeginDisabled(f.options.readOnly);
            if (ImGui::Selectable(title.c_str())) {
                auto r = Request(f, EditKind::CreateNode);
                r.type = entry.type;
                r.after = f.state->press;
                r.from = f.state->connecting;
                if (f.requests->Push(r)) {
                    std::erase(f.state->recent, entry.type);
                    f.state->recent.insert(f.state->recent.begin(), entry.type);
                    if (f.state->recent.size() > 16)
                        f.state->recent.resize(16);
                    f.state->connecting = {};
                }
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered() && !entry.description.empty())
                ImGui::SetTooltip("%.*s", int(entry.description.size()), entry.description.data());
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
}
void MiniMap(EditorFrame &f, ImVec2 size) {
    if (f.graph.nodes.empty() || size.x <= 0 || size.y <= 0)
        return;
    auto a = ImVec2{f.max.x - size.x - 12, f.max.y - size.y - 12}, b = Add(a, size);
    auto *dl = ImGui::GetWindowDrawList();
    Rect bounds{f.graph.nodes.front().position, f.graph.nodes.front().position};
    for (auto &n : f.graph.nodes) {
        bounds.min = {std::min(bounds.min.x, n.position.x), std::min(bounds.min.y, n.position.y)};
        bounds.max = {std::max(bounds.max.x, n.position.x + n.size.x),
                      std::max(bounds.max.y, n.position.y + n.size.y)};
    }
    double scale = std::min((size.x - 12) / std::max(1., bounds.max.x - bounds.min.x),
                            (size.y - 12) / std::max(1., bounds.max.y - bounds.min.y));
    auto map = [&](Point p) {
        return ImVec2{a.x + 6 + float((p.x - bounds.min.x) * scale),
                      a.y + 6 + float((p.y - bounds.min.y) * scale)};
    };
    dl->AddRectFilled(a, b, Color(f.style.header), 5);
    dl->PushClipRect(a, b, true);
    for (auto &n : f.graph.nodes)
        dl->AddRectFilled(map(n.position), map({n.position.x + n.size.x, n.position.y + n.size.y}),
                          Color(IsSelected(*f.state, n.id) ? f.style.accent : f.style.muted), 1);
    dl->AddRect(map(f.state->origin),
                map({f.state->origin.x + (f.max.x - f.min.x) / f.state->zoom,
                     f.state->origin.y + (f.max.y - f.min.y) / f.state->zoom}),
                Color(f.style.accent));
    dl->PopClipRect();
    if (Inside(ImGui::GetIO().MousePos, a, b) && f.hovered && !ImGui::IsAnyItemActive()) {
        f.blocked = true;
        if (ImGui::IsMouseDown(0)) {
            auto p = ImGui::GetIO().MousePos;
            f.state->origin = {
                bounds.min.x + (p.x - a.x - 6) / scale - (f.max.x - f.min.x) / f.state->zoom / 2,
                bounds.min.y + (p.y - a.y - 6) / scale - (f.max.y - f.min.y) / f.state->zoom / 2};
        }
    }
}
void Diagnostics(EditorFrame &f) {
    for (auto &n : f.graph.nodes)
        if (n.status == Status::Error || n.status == Status::Waiting || n.breakpoint) {
            auto label = std::string(n.title) + "##diagnostic" + std::to_string(n.id.value);
            if (ImGui::Selectable(label.c_str(), IsSelected(*f.state, n.id))) {
                Select(*f.state, n.id);
                FrameNodes(*f.state, f.graph, {&n.id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            }
            if (!n.description.empty())
                Text(n.description);
        }
}
void Breadcrumbs(EditorFrame &f, std::span<const PathEntry> path) {
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i)
            ImGui::SameLine();
        auto label = std::string(path[i].title) + "##path" + std::to_string(i);
        if (ImGui::SmallButton(label.c_str())) {
            auto r = Request(f, EditKind::EnterGraph, path[i].node);
            r.type = path[i].graph.value;
            f.requests->Push(r);
        }
    }
}
void Preview(EditorFrame &f, std::span<const PreviewOutput> outputs,
             void (*demand)(void *, const PreviewDemand &), void *user) {
    IM_ASSERT(f.nodeOpen);
    auto &p = GetPreview(*f.state, f.node->id);
    float z = float(f.state->zoom);
    ImGui::Separator();
    if (ImGui::SmallButton(p.collapsed ? "> Preview" : "v Preview"))
        p.collapsed = !p.collapsed;
    ImGui::SameLine();
    if (ImGui::SmallButton("Open"))
        p.expanded = true;
    ImGui::SameLine();
    if (ImGui::SmallButton(p.pinned ? "Unpin" : "Pin"))
        p.pinned = !p.pinned;
    if (p.collapsed || outputs.empty())
        return;
    p.output = std::clamp(p.output, 0, int(outputs.size()) - 1);
    if (outputs.size() > 1) {
        if (ImGui::BeginCombo("Output", std::string(outputs[p.output].title).c_str())) {
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.output))
                    p.output = i;
            ImGui::EndCombo();
        }
    }
    ImVec2 size{std::max(1.f, ImGui::GetContentRegionAvail().x), std::max(24.f, p.height * z)};
    if (ImGui::BeginChild("##preview", size, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        bool visible = ImGui::IsRectVisible(size);
        if (demand)
            demand(user, {f.node->id, p.output, size, visible, p.interactive});
        if (visible)
            DrawOutput(outputs[p.output], size, p.interactive);
        if (ImGui::IsWindowHovered() && p.interactive)
            f.blocked = true;
    }
    ImGui::EndChild();
    ImGui::InvisibleButton("##preview-size", {size.x, 6});
    if (ImGui::IsItemActive())
        p.height = std::clamp(p.height + ImGui::GetIO().MouseDelta.y / z, 24.f, 800.f);
    ImGui::Checkbox("Interact", &p.interactive);
}
void DrawDetachedPreviews(EditorFrame &f, NodeId node, std::span<const PreviewOutput> outputs,
                          void (*demand)(void *, const PreviewDemand &), void *user) {
    auto &p = GetPreview(*f.state, node);
    if ((!p.expanded && !p.pinned) || outputs.empty())
        return;
    p.output = std::clamp(p.output, 0, int(outputs.size()) - 1);
    auto *n = Find(f, node);
    auto label = std::string(n ? n->title : std::string_view("Preview")) + "###preview-" +
                 std::to_string(f.editorId) + "-" + std::to_string(f.graph.id.value) + "-" +
                 std::to_string(node.value);
    bool open = true;
    ImGui::SetNextWindowSize({500, 380}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(label.c_str(), &open)) {
        ImGui::Checkbox("Interact", &p.interactive);
        ImGui::SameLine();
        ImGui::Checkbox("Pinned", &p.pinned);
        if (outputs.size() > 1 &&
            ImGui::BeginCombo(
                "Compare",
                p.compare < 0
                    ? "None"
                    : std::string(outputs[std::min(p.compare, int(outputs.size()) - 1)].title).c_str())) {
            if (ImGui::Selectable("None", p.compare < 0))
                p.compare = -1;
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.compare))
                    p.compare = i;
            ImGui::EndCombo();
        }
        auto size = ImGui::GetContentRegionAvail();
        if (p.compare >= int(outputs.size()))
            p.compare = -1;
        if (p.compare >= 0)
            size.x = (size.x - ImGui::GetStyle().ItemSpacing.x) / 2;
        for (int i = 0; i < (p.compare >= 0 ? 2 : 1); ++i) {
            if (i)
                ImGui::SameLine();
            ImGui::PushID(i);
            int index = i ? p.compare : p.output;
            if (ImGui::BeginChild("output", size)) {
                if (demand)
                    demand(user, {node, index, size, true, p.interactive});
                DrawOutput(outputs[index], size, p.interactive);
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
    }
    ImGui::End();
    if (!open)
        p.expanded = p.pinned = false;
}
void EndEditor(EditorFrame &f) {
    IM_ASSERT(!f.nodeOpen && f.context == ImGui::GetCurrentContext());
    auto &s = *f.state;
    auto &io = ImGui::GetIO();
    if (f.visible) {
        auto *dl = ImGui::GetWindowDrawList();
        const PinView *hoveredPin = nullptr;
        for (auto &pin : f.graph.pins) {
            auto *n = Find(f, pin.node);
            if (!n || pin.hidden || !Visible(f, *n))
                continue;
            auto p = PinPosition(f, pin);
            float r = f.style.pinRadius * std::clamp(float(s.zoom), .8f, 2.f);
            auto c = Color(pin.color.w ? pin.color : f.style.accent);
            if (pin.shape == PinShape::Square)
                dl->AddRectFilled(Sub(p, {r, r}), Add(p, {r, r}), c, 1);
            else if (pin.shape == PinShape::Diamond)
                dl->AddQuadFilled(Add(p, {-r, 0}), Add(p, {0, -r}), Add(p, {r, 0}), Add(p, {0, r}), c);
            else if (pin.shape == PinShape::Triangle)
                dl->AddTriangleFilled(Add(p, {-r, -r}), Add(p, {r, 0}), Add(p, {-r, r}), c);
            else
                dl->AddCircleFilled(p, r, c);
            if (Inside(io.MousePos, Sub(p, {r + 5, r + 5}), Add(p, {r + 5, r + 5})))
                hoveredPin = &pin;
        }
        if (f.options.minimap)
            MiniMap(f);
        // Child widgets and popups win over canvas shortcuts and gestures.
        bool widgetActive = ImGui::IsAnyItemActive();
        bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        bool available = f.hovered && !popup && !f.blocked && !widgetActive;
        if (hoveredPin && f.hovered && !popup && !widgetActive) {
            ImGui::SetTooltip("%.*s", int(hoveredPin->label.size()), hoveredPin->label.data());
            if (ImGui::IsMouseClicked(0) && !f.options.readOnly) {
                s.connecting = hoveredPin->id;
                s.reconnecting = {};
                if (io.KeyAlt)
                    for (auto &link : f.graph.links)
                        if (link.to == hoveredPin->id) {
                            s.connecting = link.from;
                            s.reconnecting = link.id;
                            break;
                        }
            }
        }
        if (s.connecting) {
            auto *from = FindPin(f, s.connecting);
            if (from)
                dl->AddLine(PinPosition(f, *from), io.MousePos, Color(f.style.accent), f.style.linkWidth);
            if (hoveredPin && hoveredPin->id != s.connecting) {
                auto verdict = CanConnect(f.graph, s.connecting, hoveredPin->id, s.reconnecting);
                if (!verdict.allowed)
                    ImGui::SetTooltip("%.*s", int(verdict.reason.size()), verdict.reason.data());
                if (ImGui::IsMouseReleased(0)) {
                    if (verdict.allowed) {
                        auto r = Request(f, s.reconnecting ? EditKind::Reconnect : EditKind::CreateLink);
                        r.from = s.connecting;
                        r.to = hoveredPin->id;
                        r.link = s.reconnecting;
                        if (from && from->kind == PinKind::Input)
                            std::swap(r.from, r.to);
                        f.requests->Push(r);
                    }
                    s.connecting = {};
                    s.reconnecting = {};
                }
            } else if (ImGui::IsMouseReleased(0)) {
                s.press = World(f, io.MousePos);
                s.palette = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                s.connecting = {};
                s.reconnecting = {};
            }
        }
        bool overNode = false;
        for (auto &n : f.graph.nodes)
            if (Visible(f, n)) {
                auto a = Screen(f, Position(f, n));
                auto size = Size(f, n);
                if (Inside(io.MousePos, a, Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)})))
                    overNode = true;
            }
        if (available && s.gesture.empty() && !s.connecting) {
            if (ImGui::IsMouseDragging(f.options.keys.panButton)) {
                s.origin.x -= io.MouseDelta.x / s.zoom;
                s.origin.y -= io.MouseDelta.y / s.zoom;
            }
            if (io.MouseWheel) {
                auto target = s.zoom * std::pow(1.15, io.MouseWheel);
                double lo = std::max(.05, f.options.minimumZoom), hi = std::max(lo, f.options.maximumZoom);
                SetZoom(s, std::clamp(target, lo, hi), {io.MousePos.x - f.min.x, io.MousePos.y - f.min.y});
            }
            if (Chord(f.options.keys.frame)) {
                RememberView(s);
                FrameNodes(s, f.graph, s.selection, {f.max.x - f.min.x, f.max.y - f.min.y});
                RememberView(s);
            }
            if (Chord(f.options.keys.selectAll)) {
                s.selection.clear();
                for (auto &n : f.graph.nodes)
                    if (!Hidden(f, n))
                        s.selection.push_back(n.id);
            }
            if (Chord(f.options.keys.add) && !f.options.readOnly) {
                s.press = World(f, io.MousePos);
                s.palette = true;
            }
            const std::pair<ImGuiKeyChord, EditKind> keys[] = {
                {f.options.keys.remove, EditKind::DeleteNode},
                {f.options.keys.duplicate, EditKind::Duplicate},
                {f.options.keys.copy, EditKind::Copy},
                {f.options.keys.cut, EditKind::Cut},
                {f.options.keys.paste, EditKind::Paste},
                {f.options.keys.undo, EditKind::Undo},
                {f.options.keys.redo, EditKind::Redo},
                {f.options.keys.group, EditKind::Group}};
            for (auto [key, kind] : keys)
                if (Chord(key)) {
                    if (kind == EditKind::DeleteNode && s.selectedLink) {
                        auto r = Request(f, EditKind::DeleteLink);
                        r.link = s.selectedLink;
                        if (!f.options.readOnly)
                            f.requests->Push(r);
                    } else
                        QueueCommand(f, kind);
                }
            if (ImGui::IsMouseClicked(0) && !hoveredPin && !overNode) {
                bool hit = false;
                for (auto &link : f.graph.links) {
                    auto *a = FindPin(f, link.from);
                    auto *b = FindPin(f, link.to);
                    if (!a || !b)
                        continue;
                    auto points = Curve(f, *a, *b, link.style);
                    for (std::size_t i = 1; i < points.size(); ++i)
                        if (SegmentDistance(io.MousePos, points[i - 1], points[i]) < 6) {
                            s.selectedLink = link.id;
                            hit = true;
                            break;
                        }
                }
                if (!hit) {
                    if (!io.KeyShift && !io.KeyCtrl)
                        s.selection.clear();
                    s.selectedLink = {};
                    s.press = World(f, io.MousePos);
                    s.selecting = !io.KeyAlt;
                    s.cutting = io.KeyAlt && !f.options.readOnly;
                    s.lasso.clear();
                    s.lasso.push_back(s.press);
                }
            }
        }
        if (s.selecting || s.cutting) {
            auto p = World(f, io.MousePos);
            if (s.lasso.size() < 4096) {
                auto last = s.lasso.back();
                if (std::hypot(p.x - last.x, p.y - last.y) * s.zoom > 3)
                    s.lasso.push_back(p);
            }
            auto a = Screen(f, s.press), b = io.MousePos;
            if (f.options.lasso || s.cutting)
                for (std::size_t i = 1; i < s.lasso.size(); ++i)
                    dl->AddLine(Screen(f, s.lasso[i - 1]), Screen(f, s.lasso[i]), Color(f.style.accent));
            else {
                dl->AddRect({std::min(a.x, b.x), std::min(a.y, b.y)},
                            {std::max(a.x, b.x), std::max(a.y, b.y)}, Color(f.style.accent));
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                s.selecting = s.cutting = false;
                s.lasso.clear();
            } else if (ImGui::IsMouseReleased(0)) {
                if (s.selecting)
                    for (auto &n : f.graph.nodes)
                        if (!Hidden(f, n)) {
                            Point center{n.position.x + n.size.x / 2, n.position.y + n.size.y / 2};
                            bool selected = f.options.lasso
                                                ? editor::InPolygon(center, s.lasso)
                                                : n.position.x <= std::max(s.press.x, p.x) &&
                                                      n.position.x + n.size.x >= std::min(s.press.x, p.x) &&
                                                      n.position.y <= std::max(s.press.y, p.y) &&
                                                      n.position.y + n.size.y >= std::min(s.press.y, p.y);
                            if (selected)
                                Select(s, n.id, true, io.KeyCtrl);
                        }
                if (s.cutting) {
                    std::vector<EditRequest> batch;
                    auto op = s.nextOperation++;
                    for (auto &link : f.graph.links) {
                        auto *x = FindPin(f, link.from);
                        auto *y = FindPin(f, link.to);
                        if (!x || !y)
                            continue;
                        auto points = Curve(f, *x, *y, link.style);
                        bool hit = false;
                        for (std::size_t q = 1; q < s.lasso.size(); ++q)
                            for (std::size_t i = 1; i < points.size(); ++i) {
                                auto a = Screen(f, s.lasso[q - 1]), b = Screen(f, s.lasso[q]);
                                auto cross = [](ImVec2 a, ImVec2 b, ImVec2 c) {
                                    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                                };
                                auto c = points[i - 1], d = points[i];
                                const bool crossing = cross(a, b, c) * cross(a, b, d) <= 0 &&
                                                      cross(c, d, a) * cross(c, d, b) <= 0 &&
                                                      std::max(std::min(a.x, b.x), std::min(c.x, d.x)) <=
                                                          std::min(std::max(a.x, b.x), std::max(c.x, d.x)) &&
                                                      std::max(std::min(a.y, b.y), std::min(c.y, d.y)) <=
                                                          std::min(std::max(a.y, b.y), std::max(c.y, d.y));
                                if (crossing || SegmentDistance(b, c, d) < 7)
                                    hit = true;
                            }
                        if (hit) {
                            auto r = Request(f, EditKind::DeleteLink);
                            r.link = link.id;
                            r.operation = op;
                            batch.push_back(r);
                        }
                    }
                    for (auto &r : batch)
                        r.operationSize = batch.size();
                    f.requests->PushBatch(batch);
                }
                s.selecting = s.cutting = false;
                s.lasso.clear();
            }
        }
        if (f.hovered && !f.blocked && !widgetActive && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            for (auto &n : f.graph.nodes)
                if (Visible(f, n)) {
                    auto a = Screen(f, Position(f, n));
                    auto size = Size(f, n);
                    if (Inside(io.MousePos, a, Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)})) &&
                        !IsSelected(s, n.id))
                        Select(s, n.id);
                }
            s.press = World(f, io.MousePos);
            ImGui::OpenPopup("Node actions");
        }
        if (ImGui::BeginPopup("Node actions")) {
            if (ImGui::MenuItem("Add node", nullptr, false, !f.options.readOnly))
                s.palette = true;
            if (ImGui::MenuItem("Frame selection"))
                FrameNodes(s, f.graph, s.selection, {f.max.x - f.min.x, f.max.y - f.min.y});
            if (ImGui::MenuItem("Bookmark view"))
                AddBookmark(s, "View");
            for (std::size_t i = 0; i < s.bookmarks.size(); ++i) {
                auto label = std::string(s.bookmarks[i].label.data()) + "##bookmark" + std::to_string(i);
                if (ImGui::MenuItem(label.c_str()) && s.bookmarks[i].view.graph == f.graph.id)
                    RestoreView(s, s.bookmarks[i].view);
            }
            ImGui::Separator();
            ImGui::BeginDisabled(f.options.readOnly);
            if (ImGui::MenuItem("Insert active node into selected wire", nullptr, false,
                                bool(s.active) && bool(s.selectedLink)))
                InsertNode(f, s.active, s.selectedLink);
            const std::pair<const char *, EditKind> commands[] = {{"Duplicate", EditKind::Duplicate},
                                                                  {"Delete", EditKind::DeleteNode},
                                                                  {"Copy", EditKind::Copy},
                                                                  {"Cut", EditKind::Cut},
                                                                  {"Paste", EditKind::Paste},
                                                                  {"Group", EditKind::Group},
                                                                  {"Ungroup", EditKind::Ungroup},
                                                                  {"Make subgraph", EditKind::MakeSubgraph},
                                                                  {"Collapse / expand", EditKind::Collapse},
                                                                  {"Lock / unlock", EditKind::Lock},
                                                                  {"Bypass", EditKind::Bypass},
                                                                  {"Save template", EditKind::SaveTemplate}};
            for (auto [label, kind] : commands)
                if (ImGui::MenuItem(label))
                    QueueCommand(f, kind);
            LayoutToolbar(f);
            if (f.options.canRun && ImGui::MenuItem("Run selection"))
                QueueCommand(f, EditKind::Run);
            if (f.options.canDebug && ImGui::MenuItem("Toggle breakpoint"))
                QueueCommand(f, EditKind::Breakpoint);
            if (f.options.canDebug && ImGui::MenuItem("Step"))
                QueueCommand(f, EditKind::Step);
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }
        if (f.requests->overflow) {
            dl->AddText(Add(f.min, {12, 12}), Color(f.style.warning),
                        "Request buffer full: drain / enlarge it to finish the operation");
        }
    }
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopID();
    f.visible = false;
}
} // namespace imkit::node_editor
