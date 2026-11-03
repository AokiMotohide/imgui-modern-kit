#include <imkit/node_editor.h>
#include <imkit/widgets.h>
#include "draw_internal.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

namespace imkit::node_editor {
namespace {
constexpr float kPi = 3.14159265358979f;
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
constexpr double kRerouteSize = 16; // Graph units; a reroute is a knot, not a box.
Point Size(const EditorFrame &f, const NodeView &n) {
    if (n.kind == NodeKind::Reroute)
        return {kRerouteSize, kRerouteSize};
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
const char *Tr(const EditorFrame &f, std::string_view key, const char *fallback) {
    return f.options.locale ? f.options.locale->Text(key, fallback) : fallback;
}
ImVec4 CategoryColor(const NodeStyle &st, const NodeView &n) {
    if (n.tint.w > 0)
        return n.tint;
    if (n.category)
        for (auto &c : st.categories)
            if (c.id == n.category && c.color.w > 0)
                return c.color;
    return st.accent;
}
std::string_view CategoryGlyph(const NodeStyle &st, const NodeView &n) {
    if (!n.glyph.empty() || !n.category)
        return n.glyph;
    for (auto &c : st.categories)
        if (c.id == n.category)
            return c.glyph;
    return {};
}
// Dark or light ink, whichever reads on the given fill.
ImVec4 ReadableOn(ImVec4 fill) {
    const float luminance = .2126f * fill.x + .7152f * fill.y + .0722f * fill.z;
    return luminance > .55f ? ImVec4{.07f, .08f, .10f, 1} : ImVec4{1, 1, 1, 1};
}
ImVec4 Mix(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
ImVec4 WithAlpha(ImVec4 c, float alpha) {
    c.w *= alpha;
    return c;
}
// Explicit colour, then socket type colour, then the owning node's accent.
ImVec4 PinColor(const EditorFrame &f, const PinView &pin) {
    if (pin.color.w > 0)
        return pin.color;
    for (auto &type : f.graph.socketTypes)
        if (type.id == pin.type && type.color.w > 0)
            return type.color;
    auto *n = Find(f, pin.node);
    const auto &st = n ? Style(f, *n) : f.style;
    return pin.role == PinRole::Flow ? st.text : st.accent; // Flow sockets match their links.
}
bool HasTypeColor(const EditorFrame &f, const PinView &pin) {
    if (pin.color.w > 0)
        return true;
    for (auto &type : f.graph.socketTypes)
        if (type.id == pin.type && type.color.w > 0)
            return true;
    return false;
}
// Recolours the vertices added since `start` with a left-to-right gradient, keeping their alpha.
void ShadeHorizontal(ImDrawList *dl, int start, float x0, float x1, ImVec4 from, ImVec4 to) {
    const float width = std::max(1.f, x1 - x0);
    for (int i = start; i < dl->VtxBuffer.Size; ++i) {
        auto &v = dl->VtxBuffer[i];
        const float t = std::clamp((v.pos.x - x0) / width, 0.f, 1.f);
        auto c = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(Mix(from, to, t)));
        c.w = ImGui::ColorConvertU32ToFloat4(v.col).w;
        v.col = ImGui::ColorConvertFloat4ToU32(c);
    }
}
const PanelState *FindPanel(const EditorState &s, NodeId node, std::uint64_t panel) {
    for (auto &p : s.panels)
        if (p.node == node && p.panel == panel)
            return &p;
    return nullptr;
}
// Hidden by the host, or an unused socket of a node that hides them.
bool Concealed(const EditorFrame &f, const PinView &pin) {
    if (pin.hidden)
        return true;
    auto *n = Find(f, pin.node);
    return n && n->hideUnusedPins && f.state->connecting != pin.id && !IsPinConnected(f.graph, pin.id);
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
    if (shown->kind == NodeKind::Reroute) // Every socket of a knot sits at its centre.
        return Screen(f, {p.x + size.x / 2, p.y + size.y / 2});
    if (pin.panel && shown == n && !n->collapsed)
        if (auto *panel = FindPanel(*f.state, n->id, pin.panel); panel && !panel->open)
            return Screen(f, {p.x + (pin.kind == PinKind::Output ? size.x : 0), p.y + panel->row});
    if (!pin.manualPosition && shown == n && !n->collapsed) {
        for (auto &row : f.state->rowPositions)
            if (row.pin == pin.id)
                return Screen(f, {p.x + (pin.kind == PinKind::Output ? size.x : 0), p.y + row.local.y});
    }
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
// Screen radius of a reroute knot. Its centre moves the knot; the ring just outside starts a wire.
float KnotRadius(double zoom) {
    return std::max(4.f, 6.5f * float(zoom));
}
constexpr float kKnotRing = 8;
float SegmentDistance(ImVec2 p, ImVec2 a, ImVec2 b) {
    auto v = Sub(b, a);
    float t = std::clamp(Dot(Sub(p, a), v) / std::max(.001f, Dot(v, v)), 0.f, 1.f);
    auto d = Sub(p, Add(a, Mul(v, t)));
    return std::sqrt(Dot(d, d));
}
Side PinSide(const EditorFrame &f, const PinView &pin) {
    auto *n = Find(f, pin.node);
    if (pin.manualPosition && !(n && n->kind == NodeKind::Reroute))
        return pin.side;
    return pin.kind == PinKind::Input ? Side::Left : Side::Right;
}
// Link geometry between two points leaving along dp and arriving against dq.
std::array<ImVec2, 25> CurvePoints(LinkStyle style, ImVec2 p, ImVec2 dp, ImVec2 q, ImVec2 dq) {
    float strength = std::max(40.f, float(std::abs(q.x - p.x) + std::abs(q.y - p.y)) * .4f);
    auto c = Add(p, Mul(dp, strength)), d = Add(q, Mul(dq, strength));
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
std::array<ImVec2, 25> Curve(const EditorFrame &f, const PinView &a, const PinView &b,
                             const LinkStyle *overrideStyle = nullptr) {
    return CurvePoints(overrideStyle ? *overrideStyle : f.style.linkStyle, PinPosition(f, a),
                       Direction(PinSide(f, a)), PinPosition(f, b), Direction(PinSide(f, b)));
}
bool IsMultiInput(const PinView &pin) {
    return pin.kind == PinKind::Input && pin.multiple && pin.role == PinRole::Data;
}
float PinRadius(const EditorFrame &f, const PinView &pin) {
    auto *n = Find(f, pin.node);
    return (n ? Style(f, *n) : f.style).pinRadius * std::clamp(float(f.state->zoom), .8f, 2.f);
}
// Links into a multi-input socket land side by side along it, in graph order.
ImVec2 LinkEndOffset(const EditorFrame &f, const PinView &pin, LinkId link) {
    if (!IsMultiInput(pin))
        return {};
    int count = 0, index = 0;
    for (auto &l : f.graph.links)
        if (l.to == pin.id || l.from == pin.id) {
            if (l.id == link)
                index = count;
            ++count;
        }
    const float d = (float(index) - float(count - 1) / 2) * PinRadius(f, pin) * 1.4f;
    const auto side = PinSide(f, pin);
    return side == Side::Left || side == Side::Right ? ImVec2{0, d} : ImVec2{d, 0};
}
// Verdict for dropping the dragged wire on `target`: a new connection, or with movingLinks every link of the
// source socket moving to a socket of the same direction.
ConnectionVerdict DragVerdict(const EditorFrame &f, PinId target) {
    const auto &s = *f.state;
    if (auto *pin = FindPin(f, target); pin && Concealed(f, *pin))
        return {false, "Socket is hidden"};
    if (!s.movingLinks)
        return CanConnect(f.graph, s.connecting, target, s.reconnecting);
    const auto *source = FindPin(f, s.connecting), *to = FindPin(f, target);
    if (!source || !to || to->id == source->id)
        return {false, "Move to another socket"};
    if (to->kind != source->kind)
        return {false, "Move to a socket of the same direction"};
    ConnectionVerdict result;
    for (auto &link : f.graph.links)
        if (link.from == source->id || link.to == source->id) {
            const PinId other = link.from == source->id ? link.to : link.from;
            auto v = CanConnect(f.graph, other, target, link.id);
            if (!v.allowed)
                return v;
            if (v.match == ConnectionMatch::Convertible)
                result = v;
        }
    return result;
}
std::array<ImVec2, 25> LinkCurve(const EditorFrame &f, const LinkView &link, const PinView &a, const PinView &b) {
    return CurvePoints(link.style ? *link.style : f.style.linkStyle, Add(PinPosition(f, a), LinkEndOffset(f, a, link.id)),
                       Direction(PinSide(f, a)), Add(PinPosition(f, b), LinkEndOffset(f, b, link.id)),
                       Direction(PinSide(f, b)));
}
// Draws every segment whose index falls in the "on" part of an on/off pattern.
void DashedPolyline(ImDrawList *dl, const std::array<ImVec2, 25> &points, ImU32 color, float width, int on,
                    int off) {
    for (std::size_t i = 1; i < points.size(); ++i)
        if (int(i - 1) % (on + off) < on)
            dl->AddLine(points[i - 1], points[i], color, width);
}
// Marker in the middle of a link that the host flagged: converted (two arrows), invalid (cross) or cycle (loop).
float BeadRadius(double zoom) {
    return 7 * std::clamp(float(zoom), .8f, 1.4f);
}
void DrawBead(ImDrawList *dl, ImVec2 c, LinkValidity validity, const NodeStyle &st, double zoom) {
    const float r = BeadRadius(zoom), k = r / 7;
    switch (validity) {
    case LinkValidity::Valid:
        return;
    case LinkValidity::Converted: {
        const ImU32 ink = ImGui::GetColorU32(st.warning);
        dl->AddCircleFilled(c, r, ImGui::GetColorU32(st.surface));
        dl->AddCircle(c, r, ink, 0, 1.4f);
        const float w = 1.3f;
        dl->AddLine({c.x - 3.5f * k, c.y - 1.6f * k}, {c.x + 3.2f * k, c.y - 1.6f * k}, ink, w);
        dl->AddLine({c.x + 1.2f * k, c.y - 3.6f * k}, {c.x + 3.4f * k, c.y - 1.6f * k}, ink, w);
        dl->AddLine({c.x + 3.5f * k, c.y + 1.8f * k}, {c.x - 3.2f * k, c.y + 1.8f * k}, ink, w);
        dl->AddLine({c.x - 1.2f * k, c.y + 3.8f * k}, {c.x - 3.4f * k, c.y + 1.8f * k}, ink, w);
        return;
    }
    case LinkValidity::Invalid: {
        dl->AddCircleFilled(c, r, ImGui::GetColorU32(st.error));
        const ImU32 ink = ImGui::GetColorU32(ReadableOn(st.error));
        dl->AddLine({c.x - 2.8f * k, c.y - 2.8f * k}, {c.x + 2.8f * k, c.y + 2.8f * k}, ink, 1.8f);
        dl->AddLine({c.x + 2.8f * k, c.y - 2.8f * k}, {c.x - 2.8f * k, c.y + 2.8f * k}, ink, 1.8f);
        return;
    }
    case LinkValidity::Cycle: {
        const ImU32 ink = ImGui::GetColorU32(st.error);
        dl->AddCircleFilled(c, r, ImGui::GetColorU32(st.surface));
        dl->AddCircle(c, r, ink, 0, 1.4f);
        dl->PathArcTo(c, 3.6f * k, -1.2f, 4.2f, 12);
        dl->PathStroke(ink, 0, 1.4f);
        const ImVec2 tip{c.x + 3.6f * k * std::cos(-1.2f), c.y + 3.6f * k * std::sin(-1.2f)};
        dl->AddTriangleFilled({tip.x - 2.2f * k, tip.y - 1.6f * k}, {tip.x + 1.8f * k, tip.y - 0.6f * k},
                              {tip.x - 0.4f * k, tip.y + 2.4f * k}, ink);
        return;
    }
    }
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
    s.rounding = t.radius.control;
    s.borderWidth = t.stroke.border;
    s.padding = t.metrics.horizontalPadding;
    s.headerHeight = t.metrics.controlHeight + 4;
    s.animate = t.motion.enabled && !t.motion.reducedMotion;
    const auto &node = t.node;
    if (node.canvas.w > 0)
        s.canvas = node.canvas;
    if (node.grid.w > 0)
        s.grid = node.grid;
    if (node.header.w > 0)
        s.header = node.header;
    s.headerFill = node.headerFill;
    s.linkColor = node.linkColor;
    s.linkArrow = node.linkArrow;
    s.highlightConnectedLinks = node.highlightLinks;
    s.hollowUnconnectedPins = node.hollowPins;
    s.headerTint = node.headerTint.value_or(s.headerTint);
    s.rounding = node.rounding.value_or(s.rounding);
    s.headerHeight = node.headerHeight.value_or(s.headerHeight);
    s.pinRadius = node.pinRadius.value_or(s.pinRadius);
    s.linkWidth = node.linkWidth.value_or(s.linkWidth);
    s.gridSize = node.gridSize.value_or(s.gridSize);
    s.pinSeat = node.pinSeat.value_or(s.pinSeat);
    s.flowLinkWidth = node.flowLinkWidth.value_or(s.flowLinkWidth);
    return s;
}
NodeStyle MakeNodeStylePreset(const Theme &t, NodeStylePreset preset) {
    auto s = MakeNodeStyle(t);
    switch (preset) {
    case NodeStylePreset::Classic:
        break;
    case NodeStylePreset::Studio:
        s.headerFill = HeaderFill::Tinted;
        s.headerTint = .16f;
        s.linkArrow = LinkArrow::None;
        s.linkColor = LinkColorSource::OutputType;
        s.hollowUnconnectedPins = true;
        s.pinSeat = 2.6f;
        break;
    case NodeStylePreset::Logic:
        s.headerFill = HeaderFill::Tinted;
        s.headerTint = .3f;
        s.linkArrow = LinkArrow::None;
        s.linkColor = LinkColorSource::OutputType;
        s.hollowUnconnectedPins = true;
        s.pinSeat = 2.2f;
        s.highlightConnectedLinks = true;
        s.pinRadius = std::max(s.pinRadius, 5.5f);
        break;
    }
    return s;
}
void EditorState::Reserve(std::size_t nodes, std::size_t previewCount) {
    selection.reserve(nodes);
    gesture.reserve(nodes);
    nodeIndex.reserve(nodes);
    pinIndex.reserve(nodes * 4);
    rowPositions.reserve(nodes * 4);
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
void DrawPinGlyph(ImDrawList *dl, const PinChromeView &v) {
    if (!dl)
        return;
    static const PinView plain{};
    const PinView &pin = v.pin ? *v.pin : plain;
    const ImVec2 p = v.center;
    const float r = std::max(1.f, v.radius);
    const ImU32 c = ImGui::GetColorU32(v.color), canvas = ImGui::GetColorU32(v.canvas);
    const float t = std::max(1.2f, r * .34f); // Stroke stays legible when zoomed out.
    const float q = r - t * .5f;               // Stroke centre radius, so outlines keep the glyph size.
    const bool multi = IsMultiInput(pin);
    const bool vertical = !(pin.manualPosition && (pin.side == Side::Top || pin.side == Side::Bottom));
    const int links = std::max(1, v.links);
    const float spacing = r * 1.4f;
    const float half = multi ? std::max(r, links * spacing / 2 + r * .25f) : r; // Capsule half length.
    const ImVec2 extent = vertical ? ImVec2{r, half} : ImVec2{half, r};
    if (v.seat > 0) { // Canvas-coloured cut-out: the socket reads as docked into the node edge.
        if (multi)
            dl->AddRectFilled(Sub(p, Add(extent, {v.seat, v.seat})), Add(p, Add(extent, {v.seat, v.seat})), canvas,
                              r + v.seat);
        else
            dl->AddCircleFilled(p, r + v.seat, canvas);
    }
    if (pin.flags & PinFlags_Unavailable) {
        const ImU32 faint = ImGui::GetColorU32(WithAlpha(v.color, .4f));
        dl->AddCircle(p, q, faint, 0, t * .8f);
        dl->AddLine({p.x - q * .7f, p.y + q * .7f}, {p.x + q * .7f, p.y - q * .7f}, faint, t * .8f);
        return;
    }
    if (pin.role == PinRole::Flow) { // Rounded square with a chevron cut into it.
        const float w = r * 1.05f;
        const ImVec2 lo{p.x - w, p.y - w}, hi{p.x + w, p.y + w};
        if (v.connected)
            dl->AddRectFilled(lo, hi, c, w * .42f);
        else
            dl->AddRect({lo.x + t * .5f, lo.y + t * .5f}, {hi.x - t * .5f, hi.y - t * .5f}, c, w * .42f, 0, t * .9f);
        const ImVec2 chevron[3] = {{p.x - w * .3f, p.y - w * .52f}, {p.x + w * .3f, p.y}, {p.x - w * .3f, p.y + w * .52f}};
        dl->AddPolyline(chevron, 3, v.connected ? canvas : c, ImDrawFlags_None, std::max(1.2f, w * .32f));
        return;
    }
    if (multi) { // A capsule that grows with its links; one dot per link.
        dl->AddRect(Sub(p, {extent.x - t * .5f, extent.y - t * .5f}), Add(p, {extent.x - t * .5f, extent.y - t * .5f}), c,
                    r, 0, t);
        for (int i = 0; i < v.links; ++i) {
            const float d = (float(i) - float(v.links - 1) / 2) * spacing;
            dl->AddCircleFilled(vertical ? ImVec2{p.x, p.y + d} : ImVec2{p.x + d, p.y}, r * .34f, c);
        }
        return;
    }
    const bool solid = !v.hollow; // Classic look: filled glyphs whatever the connection state.
    const bool core = solid || v.connected;
    if (pin.container == PinContainer::Array) // A fainter ring behind: more than one.
        dl->AddCircle({p.x - r * .42f, p.y - r * .42f}, q * .92f, ImGui::GetColorU32(WithAlpha(v.color, .45f)), 0,
                      t * .8f);
    if (pin.container == PinContainer::Set) {
        dl->AddRect({p.x - q, p.y - q}, {p.x + q, p.y + q}, c, r * .35f, 0, t);
        dl->AddCircleFilled(p, r * .3f, c);
        return;
    }
    if (pin.container == PinContainer::Map) { // Key colour on the left, value colour on the right.
        const ImU32 key = ImGui::GetColorU32(v.keyColor.w > 0 ? v.keyColor : v.color);
        dl->PathArcTo(p, r, kPi * .5f, kPi * 1.5f, 12);
        dl->PathFillConvex(key);
        dl->PathArcTo(p, r, -kPi * .5f, kPi * .5f, 12);
        dl->PathFillConvex(c);
        dl->AddLine({p.x, p.y - r - 1}, {p.x, p.y + r + 1}, canvas, std::max(1.f, r * .25f));
        return;
    }
    if (pin.flags & PinFlags_Wildcard) { // Dashed ring and a plus: any type until resolved.
        for (int i = 0; i < 8; ++i) {
            const float a0 = i * kPi / 4, a1 = a0 + kPi / 6.5f;
            dl->PathArcTo(p, q, a0, a1, 4);
            dl->PathStroke(c, 0, t * .9f);
        }
        dl->AddLine({p.x - r * .45f, p.y}, {p.x + r * .45f, p.y}, c, t * .8f);
        dl->AddLine({p.x, p.y - r * .45f}, {p.x, p.y + r * .45f}, c, t * .8f);
        return;
    }
    if (pin.flags & PinFlags_Reference) { // Hexagon: a reference, not a value.
        if (solid)
            dl->AddNgonFilled(p, r * 1.08f, c, 6);
        else {
            dl->AddNgon(p, q * 1.08f, c, 6, t);
            if (core)
                dl->AddCircleFilled(p, r * .45f, c);
        }
        return;
    }
    if (pin.flags & PinFlags_Field) { // Ring with an inner diamond: one value per element.
        dl->AddCircle(p, q, c, 0, t);
        const float d = r * .5f;
        dl->AddQuadFilled({p.x, p.y - d}, {p.x + d, p.y}, {p.x, p.y + d}, {p.x - d, p.y}, c);
        return;
    }
    if (pin.flags & PinFlags_Optional) { // Open ring: fine to leave unconnected.
        dl->PathArcTo(p, q, -kPi / 3, kPi * 1.6f, 20);
        dl->PathStroke(c, 0, t);
        if (v.connected)
            dl->AddCircleFilled(p, r * .5f, c);
        return;
    }
    if (solid) {
        switch (v.shape) {
        case PinShape::Square:
            dl->AddRectFilled(Sub(p, {r, r}), Add(p, {r, r}), c, 1);
            break;
        case PinShape::Diamond:
            dl->AddQuadFilled(Add(p, {-r, 0}), Add(p, {0, -r}), Add(p, {r, 0}), Add(p, {0, r}), c);
            break;
        case PinShape::Triangle:
            dl->AddTriangleFilled(Add(p, {-r, -r}), Add(p, {r, 0}), Add(p, {-r, r}), c);
            break;
        case PinShape::Circle:
            dl->AddCircleFilled(p, r, c);
            break;
        }
        return;
    }
    switch (v.shape) { // Ring, plus a core once something is connected.
    case PinShape::Square:
        dl->AddRect(Sub(p, {q, q}), Add(p, {q, q}), c, 1, 0, t);
        if (core)
            dl->AddRectFilled(Sub(p, {r * .45f, r * .45f}), Add(p, {r * .45f, r * .45f}), c);
        break;
    case PinShape::Diamond:
        dl->AddQuad(Add(p, {-q, 0}), Add(p, {0, -q}), Add(p, {q, 0}), Add(p, {0, q}), c, t);
        if (core)
            dl->AddQuadFilled(Add(p, {-r * .45f, 0}), Add(p, {0, -r * .45f}), Add(p, {r * .45f, 0}), Add(p, {0, r * .45f}), c);
        break;
    case PinShape::Triangle:
        dl->AddTriangle(Add(p, {-q, -q}), Add(p, {q, 0}), Add(p, {-q, q}), c, t);
        if (core)
            dl->AddTriangleFilled(Add(p, {-r * .4f, -r * .4f}), Add(p, {r * .45f, 0}), Add(p, {-r * .4f, r * .4f}), c);
        break;
    case PinShape::Circle:
        dl->AddCircle(p, q, c, 0, t);
        if (core)
            dl->AddCircleFilled(p, r * .52f, c);
        break;
    }
}
namespace {
void DrawNodeChrome(const EditorFrame &f, const NodeView &n, ImDrawList *dl) {
    const auto &s = *f.state;
    const auto &st = Style(f, n);
    const float z = float(s.zoom);
    const auto a = Screen(f, Position(f, n));
    const auto size = Size(f, n);
    const auto b = Add(a, {float(size.x * z), float(size.y * z)});
    const bool selected = IsSelected(s, n.id), hovered = f.hoveredNode == n.id;
    const ImVec4 category = CategoryColor(st, n);
    if (n.kind == NodeKind::Reroute) {
        ImVec4 color = category;
        if (n.tint.w <= 0 && !n.category)
            for (auto &pin : f.graph.pins)
                if (pin.node == n.id) {
                    color = PinColor(f, pin);
                    break;
                }
        const ImVec2 c{(a.x + b.x) * .5f, (a.y + b.y) * .5f};
        const float r = KnotRadius(s.zoom);
        auto diamond = [&](float radius, ImU32 col, float thickness) {
            const ImVec2 p[4] = {{c.x, c.y - radius}, {c.x + radius, c.y}, {c.x, c.y + radius}, {c.x - radius, c.y}};
            if (thickness <= 0)
                dl->AddQuadFilled(p[0], p[1], p[2], p[3], col);
            else
                dl->AddQuad(p[0], p[1], p[2], p[3], col, thickness);
        };
        diamond(r + 2.5f * std::min(z, 1.5f), Color(st.canvas), 0);
        diamond(r, Color(color), 0);
        if (selected || hovered)
            diamond(r + 4, Color(selected ? st.selected : WithAlpha(st.accent, .55f)), selected ? 2.f : 1.5f);
        return;
    }
    const bool group = n.kind == NodeKind::Frame || n.kind == NodeKind::Group;
    const bool note = n.kind == NodeKind::Note;
    const bool subgraph = n.kind == NodeKind::Subgraph || bool(n.childGraph);
    const bool headerless = group || note || n.shape == NodeShape::Compact || n.shape == NodeShape::Variable;
    const float header = headerless ? 0.f : std::min(float(st.headerHeight * z), b.y - a.y);
    if (st.hooks && st.hooks->background) {
        NodeChromeView view;
        view.node = &n;
        view.min = a;
        view.max = b;
        view.zoom = z;
        view.headerHeight = header;
        view.rounding = st.rounding;
        view.category = category;
        view.surface = st.surface;
        view.header = st.header;
        view.border = selected ? st.accent : st.border;
        view.text = st.text;
        view.hovered = hovered;
        view.selected = selected;
        if (st.hooks->background(dl, view, st.hooks->user))
            return;
    }
    const ImVec2 headerMax{b.x, a.y + header};
    const float rounding = n.shape == NodeShape::Variable ? (b.y - a.y) * .5f
                           : n.shape == NodeShape::Compact ? std::min(st.rounding * 1.5f, (b.y - a.y) * .5f)
                                                           : st.rounding;
    // State colour for the outline: diagnostics first, then the run status.
    const auto severity = n.diagnostic.severity;
    ImVec4 state{};
    if (severity == Severity::Error || (severity == Severity::None && n.status == Status::Error))
        state = st.error;
    else if (severity == Severity::Warning)
        state = st.warning;
    else if (n.status == Status::Success)
        state = st.success;
    if (n.status == Status::Running && !group) // A soft outer glow while the host runs the node.
        dl->AddRect(Sub(a, {3, 3}), Add(b, {3, 3}), Color(WithAlpha(st.accent, .35f)), rounding + 3, ImDrawFlags_None, 4.f);
    if (st.shadows && !group && !note)
        detail::DrawSoftShadow(dl, a, b, rounding, 10.f * z, .14f);
    if (subgraph && !group) { // A second card behind: there is a graph inside.
        const ImVec2 o{5 * std::min(z, 1.5f), 5 * std::min(z, 1.5f)};
        dl->AddRectFilled(Add(a, o), Add(b, o), Color(st.surface), rounding);
        dl->AddRect(Add(a, o), Add(b, o), Color(st.border), rounding, 0, st.borderWidth);
    }
    if (n.shape == NodeShape::Event && !headerless) { // Entry tab on the left edge: execution starts here.
        const float tab = std::max(6.f, 14 * z);
        const ImVec2 t0{a.x - tab, a.y + header * .18f}, t1{a.x + rounding, a.y + header * .82f};
        dl->AddRectFilled(t0, t1, Color(category), std::min(6 * z, (t1.y - t0.y) * .5f), ImDrawFlags_RoundCornersLeft);
        const float k = (t1.y - t0.y) * .2f;
        const ImVec2 c{a.x - tab * .5f, (t0.y + t1.y) * .5f};
        dl->AddTriangleFilled({c.x - k * .7f, c.y - k}, {c.x + k, c.y}, {c.x - k * .7f, c.y + k}, Color(ReadableOn(category)));
    }
    ImVec4 body = st.surface;
    if (group)
        body = (n.tint.w > 0 || n.category) ? WithAlpha(category, std::max(st.group.w, .06f)) : st.group;
    else if (note)
        body = Mix(st.surface, (n.tint.w > 0 || n.category) ? category : st.warning, .16f);
    dl->AddRectFilled(a, b, Color(body), rounding);
    if (!headerless)
        switch (st.headerFill) {
        case HeaderFill::Strip:
            dl->AddRectFilled(a, headerMax, Color(st.header), rounding, ImDrawFlags_RoundCornersTop);
            dl->AddLine({a.x + rounding, a.y + 1}, {b.x - rounding, a.y + 1}, Color(category), 2);
            break;
        case HeaderFill::Tinted:
            dl->AddRectFilled(a, headerMax, Color(WithAlpha(category, std::clamp(st.headerTint, 0.f, 1.f))), rounding,
                              ImDrawFlags_RoundCornersTop);
            dl->AddLine({a.x, headerMax.y}, headerMax, Color(WithAlpha(category, .35f)));
            break;
        case HeaderFill::Flat:
            dl->AddRectFilled(a, headerMax, Color(category), rounding, ImDrawFlags_RoundCornersTop);
            break;
        case HeaderFill::Gradient: {
            const int start = dl->VtxBuffer.Size;
            dl->AddRectFilled(a, headerMax, Color(category), rounding, ImDrawFlags_RoundCornersTop);
            ShadeHorizontal(dl, start, a.x, b.x, category, Mix(category, st.header, .85f));
            break;
        }
        case HeaderFill::None:
            dl->AddLine({a.x, headerMax.y}, headerMax, Color(WithAlpha(st.border, .6f)));
            break;
        }
    if (note) { // Folded corner.
        const float fold = std::min(14 * z, (b.y - a.y) * .3f);
        dl->AddTriangleFilled({b.x - fold, a.y}, {b.x, a.y + fold}, {b.x - fold, a.y + fold},
                              Color(Mix(body, st.text, .12f)));
    }
    if (group) { // Label tab and member count.
        const float tabHeight = std::max(10.f, std::min(float(st.headerHeight * z) * .72f, (b.y - a.y) * .5f));
        const std::string title(n.title);
        const auto extent = ImGui::CalcTextSize(title.c_str());
        const ImVec2 t0{a.x + 8 * z, a.y + 6 * z};
        const ImVec2 t1{std::min(b.x - 8 * z, t0.x + extent.x + 18 * z), t0.y + tabHeight};
        const ImVec4 tone = (n.tint.w > 0 || n.category) ? category : st.accent;
        if (t1.x > t0.x) {
            dl->AddRectFilled(t0, t1, Color(WithAlpha(tone, .9f)), std::min(7 * z, tabHeight * .5f));
            dl->PushClipRect(t0, t1, true);
            dl->AddText({t0.x + 9 * z, t0.y + (tabHeight - extent.y) * .5f}, Color(ReadableOn(tone)), title.c_str());
            dl->PopClipRect();
        }
        int members = 0;
        for (auto &m : f.graph.nodes)
            members += m.parent == n.id;
        if (members > 0 && b.x - t1.x > 60 * z) {
            char count[48];
            std::snprintf(count, sizeof(count), Tr(f, "node_editor.frame.members", "%d nodes"), members);
            const auto countSize = ImGui::CalcTextSize(count);
            dl->AddText({b.x - 10 * z - countSize.x, t0.y + (tabHeight - countSize.y) * .5f}, Color(st.muted), count);
        }
    }
    if (n.shape == NodeShape::Compact) { // A lens with the operator glyph.
        const ImVec2 c{(a.x + b.x) * .5f, (a.y + b.y) * .5f};
        const float lens = std::max(4.f, std::min(b.x - a.x, b.y - a.y) * .32f);
        dl->AddCircleFilled(c, lens, Color(WithAlpha(category, .14f)));
        dl->AddCircle(c, lens, Color(WithAlpha(category, .5f)), 0, 1.2f);
        // The operator itself: the node's glyph, else its title; the category glyph is a last resort.
        std::string g(!n.glyph.empty() ? n.glyph : !n.title.empty() ? n.title : CategoryGlyph(st, n));
        const float glyphSize = lens * 1.2f;
        const auto extent = ImGui::GetFont()->CalcTextSizeA(glyphSize, FLT_MAX, 0, g.c_str());
        dl->AddText(ImGui::GetFont(), glyphSize, {c.x - extent.x * .5f, c.y - extent.y * .5f}, Color(category), g.c_str());
    }
    if (n.shape == NodeShape::Variable) { // Type chip and name.
        ImVec4 chip = category;
        if (n.tint.w <= 0 && !n.category)
            for (auto &pin : f.graph.pins)
                if (pin.node == n.id) {
                    chip = PinColor(f, pin);
                    break;
                }
        const float h = b.y - a.y, k = h * .44f;
        const ImVec2 c0{a.x + h * .35f, a.y + (h - k) * .5f};
        dl->AddRectFilled(c0, {c0.x + k, c0.y + k}, Color(WithAlpha(chip, .85f)), k * .3f);
        const std::string title(n.title);
        const auto extent = ImGui::CalcTextSize(title.c_str());
        dl->PushClipRect(a, b, true);
        dl->AddText({c0.x + k + 8 * z, a.y + (h - extent.y) * .5f}, Color(st.text), title.c_str());
        dl->PopClipRect();
        if (!selected && !hovered && state.w <= 0)
            state = WithAlpha(chip, .55f);
    }
    float width = st.borderWidth;
    ImVec4 border = group ? WithAlpha((n.tint.w > 0 || n.category) ? category : st.accent, .45f) : st.border;
    if (state.w > 0)
        border = state;
    if (selected) {
        width = st.selectedBorderWidth > 0 ? st.selectedBorderWidth : st.borderWidth;
        border = st.accent;
    } else if (hovered && !group) {
        width = st.hoverBorderWidth > 0 ? st.hoverBorderWidth : st.borderWidth;
        if (state.w <= 0)
            border = Mix(st.border, st.accent, .5f);
    }
    if (n.breakpoint && !group) { // Seated on the top-left corner, outside the body clip.
        const float r = std::max(3.f, 4.5f * z);
        dl->AddCircleFilled(a, r + 1.5f, Color(st.canvas));
        dl->AddCircleFilled(a, r, Color(st.error));
    }
    const bool dashed = n.shape == NodeShape::Boundary || n.status == Status::Waiting;
    if (!dashed)
        dl->AddRect(a, b, Color(border), rounding, 0, width);
    else { // Straight dashes along the sides; corners stay rounded.
        const float dash = 6 * std::max(z, .6f), gap = 4 * std::max(z, .6f), r = std::min(rounding, (b.x - a.x) * .5f);
        auto side = [&](ImVec2 p0, ImVec2 p1) {
            const auto d = Sub(p1, p0);
            const float len = std::sqrt(Dot(d, d));
            if (len <= 0)
                return;
            for (float t = 0; t < len; t += dash + gap)
                dl->AddLine(Add(p0, Mul(d, t / len)), Add(p0, Mul(d, std::min(len, t + dash) / len)), Color(border), width);
        };
        side({a.x + r, a.y}, {b.x - r, a.y});
        side({b.x, a.y + r}, {b.x, b.y - r});
        side({b.x - r, b.y}, {a.x + r, b.y});
        side({a.x, b.y - r}, {a.x, a.y + r});
        for (auto [c, a0] : {std::pair{ImVec2{a.x + r, a.y + r}, kPi}, std::pair{ImVec2{b.x - r, a.y + r}, kPi * 1.5f},
                             std::pair{ImVec2{b.x - r, b.y - r}, 0.f}, std::pair{ImVec2{a.x + r, b.y - r}, kPi * .5f}}) {
            dl->PathArcTo(c, r, a0, a0 + kPi * .5f, 6);
            dl->PathStroke(Color(border), 0, width);
        }
    }
}
} // namespace
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
    if (s.graph != graph.id)
        s.rowPositions.clear();
    std::erase_if(s.rowPositions, [&](auto &row) { return !FindPin(graph, row.pin); });
    std::erase_if(s.panels, [&](auto &panel) { return !Find(graph, panel.node); });
    if (s.valueEditing &&
        (s.valueEdit.graph != graph.id || s.valueEdit.revision != graph.revision || options.readOnly ||
         IsPinConnected(graph, s.valueEdit.from) || !FindPin(graph, s.valueEdit.from))) {
        s.valueEdit.phase = Phase::Cancel;
        s.valueTerminalPending = true;
    }
    if (s.valueEditing) {
        auto *pin = FindPin(graph, s.valueEdit.from);
        auto *node = pin ? Find(graph, pin->node) : nullptr;
        if (!node || node->collapsed || (pin && pin->hidden) || (node && node->locked) ||
            s.zoom < style.detailZoom) {
            s.valueEdit.phase = Phase::Cancel;
            s.valueTerminalPending = true;
        }
    }
    if (s.valueTerminalPending && out.Push(s.valueEdit)) {
        s.valueTerminalPending = false;
        s.valueEditing = false;
    }
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
    if (!s.connecting)
        s.movingLinks = false;
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
        if (s.valueEditing) {
            s.valueEdit.phase = Phase::Cancel;
            if (out.Push(s.valueEdit))
                s.valueEditing = false;
            else
                s.valueTerminalPending = true;
        }
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
                const NodeId moved = s.gesture.size() == 1 && s.gesture.front().kind == EditKind::Move
                                         ? s.gesture.front().node
                                         : NodeId{};
                if (EmitGesture(f, Phase::Commit)) {
                    s.gesture.clear();
                    if (moved && s.dropLink) // Released over a link: insert the node into it.
                        InsertNode(f, moved, s.dropLink);
                }
                s.dropLink = {};
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
        // Dot grid: quieter than full lines and keeps node edges readable.
        const float dot = std::clamp(float(s.zoom), .6f, 1.4f);
        for (double x = f.min.x - std::fmod(s.origin.x * s.zoom, step); x < f.max.x; x += step)
            for (double y = f.min.y - std::fmod(s.origin.y * s.zoom, step); y < f.max.y; y += step)
                dl->AddRectFilled({float(x) - dot, float(y) - dot}, {float(x) + dot, float(y) + dot}, Color(style.grid));
    }
    // Topmost node under the mouse; later nodes draw above earlier ones.
    if (f.hovered)
        for (auto &n : graph.nodes)
            if (Visible(f, n)) {
                auto a = Screen(f, Position(f, n));
                auto size = Size(f, n);
                if (Inside(io.MousePos, a, Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)})))
                    f.hoveredNode = n.id;
            }
    // Draw all node silhouettes before links and interactive child windows.
    auto *dl = ImGui::GetWindowDrawList();
    for (auto &n : graph.nodes)
        if (Visible(f, n))
            DrawNodeChrome(f, n, dl);
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
    const bool reroute = n->kind == NodeKind::Reroute;
    const bool group = n->kind == NodeKind::Frame || n->kind == NodeKind::Group;
    const bool solid = n->shape == NodeShape::Compact || n->shape == NodeShape::Variable; // No header, no body.
    const bool titled = !reroute && !group && !solid; // Title drawn here; frames and solid shapes draw their own.
    ImVec2 enterMin{}, enterMax{};
    if (reroute) { // The knot centre is the move handle; the ring outside it belongs to the sockets.
        const float k = KnotRadius(s.zoom) * .85f;
        ImGui::SetCursorScreenPos({a.x + px.x / 2 - k, a.y + px.y / 2 - k});
        ImGui::InvisibleButton("##move", {2 * k, 2 * k});
    } else if (solid) { // The whole shape moves it; sockets sit on its edge, outside the button.
        ImGui::SetCursorScreenPos(Add(a, {2 * z, 2 * z}));
        ImGui::InvisibleButton("##move", {std::max(1.f, px.x - 4 * z), std::max(1.f, px.y - 4 * z)});
    } else {
        // The open-subgraph button sits at the right of the header; the move handle stops short of it,
        // because the first item submitted keeps the hover.
        const float header = st.headerHeight * z;
        const bool enter = titled && (n->kind == NodeKind::Subgraph || n->childGraph);
        const float reserve = enter ? std::max(6.f, std::min(header * .5f, 16 * z)) * 1.2f + 5 * z : 0;
        ImGui::SetCursorScreenPos(Add(a, {st.padding * z, 2 * z}));
        ImGui::InvisibleButton("##move", {std::max(1.f, px.x - 2 * st.padding * z - reserve),
                                          std::max(1.f, header - 4 * z)});
    }
    bool headerHooked = false;
    if (titled && st.hooks && st.hooks->header) {
        NodeChromeView view;
        view.node = n;
        view.min = a;
        view.max = Add(a, px);
        view.zoom = z;
        view.headerHeight = st.headerHeight * z;
        view.rounding = st.rounding;
        view.category = CategoryColor(st, *n);
        view.surface = st.surface;
        view.header = st.header;
        view.border = IsSelected(s, id) ? st.accent : st.border;
        view.text = st.text;
        view.hovered = f.hoveredNode == id;
        view.selected = IsSelected(s, id);
        headerHooked = st.hooks->header(dl, view, st.hooks->user);
    }
    if (titled) {
        const float header = st.headerHeight * z;
        const ImVec4 category = CategoryColor(st, *n);
        const bool onCategory = n->kind != NodeKind::Note &&
                                (st.headerFill == HeaderFill::Flat || st.headerFill == HeaderFill::Gradient);
        const ImVec4 ink = onCategory ? ReadableOn(category) : st.text;
        float x = a.x + st.padding * z;
        float right = a.x + px.x - st.padding * z;
        const auto glyph = CategoryGlyph(st, *n);
        if (!glyph.empty() && !headerHooked) {
            const float tile = std::max(4.f, std::min(header - 10 * z, 20 * z));
            const ImVec2 t0{x, a.y + (header - tile) / 2}, t1{x + tile, a.y + (header + tile) / 2};
            const ImVec4 fill = onCategory ? WithAlpha(ReadableOn(category), .22f) : category;
            dl->AddRectFilled(t0, t1, Color(fill), std::min(st.rounding, tile * .3f));
            const std::string g(glyph);
            const float glyphSize = tile * .68f;
            const auto extent = ImGui::GetFont()->CalcTextSizeA(glyphSize, FLT_MAX, 0, g.c_str());
            dl->AddText(ImGui::GetFont(), glyphSize, {(t0.x + t1.x - extent.x) / 2, (t0.y + t1.y - extent.y) / 2},
                        Color(onCategory ? ink : ReadableOn(category)), g.c_str());
            x = t1.x + 6 * z;
        }
        // Header badges, right to left: open subgraph, lock, diagnostic, cached, progress.
        const float badge = std::max(6.f, std::min(header * .5f, 16 * z));
        const float by = a.y + header * .5f;
        auto slot = [&]() {
            const float cx = right - badge * .5f;
            right -= badge + 5 * z;
            return ImVec2{cx, by};
        };
        if (n->kind == NodeKind::Subgraph || n->childGraph) {
            const auto c = slot();
            enterMin = {c.x - badge * .6f, c.y - badge * .6f};
            enterMax = {c.x + badge * .6f, c.y + badge * .6f};
            const float k = badge * .28f;
            const ImU32 col = Color(onCategory ? ink : st.muted);
            dl->AddRect({c.x - k * 1.4f, c.y - k * 1.4f}, {c.x + k * 1.4f, c.y + k * 1.4f}, col, 2 * z, 0, 1.2f);
            dl->AddLine({c.x - k * .5f, c.y + k * .5f}, {c.x + k * 1.9f, c.y - k * 1.9f}, col, 1.4f);
            dl->AddLine({c.x + k * .6f, c.y - k * 1.9f}, {c.x + k * 1.9f, c.y - k * 1.9f}, col, 1.4f);
            dl->AddLine({c.x + k * 1.9f, c.y - k * 1.9f}, {c.x + k * 1.9f, c.y - k * .6f}, col, 1.4f);
        }
        if (n->locked) { // Padlock drawn as vectors; the editor owns no icon atlas.
            const auto c = slot();
            const float k = badge * .3f;
            const ImU32 col = Color(onCategory ? WithAlpha(ink, .8f) : st.muted);
            dl->AddRectFilled({c.x - k, c.y - k * .1f}, {c.x + k, c.y + k * 1.3f}, col, k * .25f);
            dl->PathArcTo({c.x, c.y - k * .1f}, k * .65f, kPi, kPi * 2, 8);
            dl->PathStroke(col, 0, std::max(1.f, k * .35f));
        }
        if (n->diagnostic.severity != Severity::None) {
            const auto c = slot();
            const ImVec4 tone = n->diagnostic.severity == Severity::Error     ? st.error
                                : n->diagnostic.severity == Severity::Warning ? st.warning
                                                                              : st.accent;
            dl->AddCircleFilled(c, badge * .5f, Color(tone));
            const float k = badge * .5f;
            const ImU32 inkOn = Color(ReadableOn(tone));
            if (n->diagnostic.severity == Severity::Info) {
                dl->AddLine({c.x, c.y - k * .1f}, {c.x, c.y + k * .5f}, inkOn, std::max(1.2f, k * .22f));
                dl->AddCircleFilled({c.x, c.y - k * .45f}, std::max(.8f, k * .13f), inkOn);
            } else {
                dl->AddLine({c.x, c.y - k * .5f}, {c.x, c.y + k * .1f}, inkOn, std::max(1.2f, k * .22f));
                dl->AddCircleFilled({c.x, c.y + k * .45f}, std::max(.8f, k * .13f), inkOn);
            }
        }
        if (n->status == Status::Cached) {
            const char *label = Tr(f, "node_editor.badge.cached", "cached");
            const float chipSize = ImGui::GetFontSize() * .72f;
            const auto extent = ImGui::GetFont()->CalcTextSizeA(chipSize, FLT_MAX, 0, label);
            const ImVec2 hi{right, by + extent.y * .5f + 2 * z}, lo{right - extent.x - 10 * z, by - extent.y * .5f - 2 * z};
            if (lo.x > x + 24 * z) {
                dl->AddRectFilled(lo, hi, Color(WithAlpha(st.success, .2f)), (hi.y - lo.y) * .5f);
                dl->AddText(ImGui::GetFont(), chipSize, {lo.x + 5 * z, lo.y + 2 * z}, Color(st.success), label);
                right = lo.x - 5 * z;
            }
        }
        if (n->status == Status::Running && n->progress > 0) { // Progress ring.
            const auto c = slot();
            const float r = badge * .42f;
            dl->AddCircle(c, r, Color(WithAlpha(onCategory ? ink : st.muted, .35f)), 0, std::max(1.5f, 2 * z));
            dl->PathArcTo(c, r, -kPi * .5f, -kPi * .5f + kPi * 2 * std::clamp(n->progress, 0.f, 1.f), 20);
            dl->PathStroke(Color(onCategory ? ink : st.accent), 0, std::max(1.5f, 2 * z));
        }
        if (!n->subtitle.empty() && !headerHooked) {
            const std::string sub(n->subtitle);
            const float subSize = ImGui::GetFontSize() * .82f;
            const auto extent = ImGui::GetFont()->CalcTextSizeA(subSize, FLT_MAX, 0, sub.c_str());
            const float sx = std::max(x + 24 * z, right - extent.x);
            const ImVec4 color = onCategory ? WithAlpha(ink, .72f)
                                 : st.headerFill == HeaderFill::Tinted ? category
                                                                       : st.muted;
            dl->PushClipRect({sx, a.y}, {right, a.y + header}, true);
            dl->AddText(ImGui::GetFont(), subSize, {sx, a.y + (header - extent.y) / 2}, Color(color), sub.c_str());
            dl->PopClipRect();
            right = sx - 6 * z;
        }
        auto title = std::string(n->icon);
        if (!title.empty())
            title += ' ';
        title.append(n->title);
        if (!headerHooked) {
            dl->PushClipRect({x, a.y}, {std::max(x, right), a.y + header}, true);
            dl->AddText({x, a.y + (header - ImGui::GetFontSize()) / 2}, Color(ink), title.c_str());
            dl->PopClipRect();
        }
    }
    if (ImGui::IsItemClicked()) {
        if (!IsSelected(s, id) || ImGui::GetIO().KeyCtrl)
            Select(s, id, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
        else
            s.active = id;
        if (!ImGui::GetIO().KeyCtrl)
            BeginGesture(f, *n, EditKind::Move);
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !reroute) {
        Cancel(f);
        if (n->childGraph)
            QueueCommand(f, EditKind::EnterGraph, id);
        else if (s.zoom < st.detailZoom) {
            FrameNodes(s, f.graph, {&id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            RememberView(s);
        } else
            QueueCommand(f, EditKind::Collapse, id);
    }
    if (ImGui::IsItemHovered() && !n->description.empty() && n->kind != NodeKind::Note)
        ImGui::SetTooltip("%.*s", int(n->description.size()), n->description.data());
    if (enterMax.x > enterMin.x) { // Open-subgraph button in the header.
        ImGui::SetCursorScreenPos(enterMin);
        if (ImGui::InvisibleButton("##enter", Sub(enterMax, enterMin)))
            QueueCommand(f, EditKind::EnterGraph, id);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Tr(f, "node_editor.subgraph.open", "Open subgraph"));
    }
    // A collapsed node has no body item to validate an extended cursor position.
    if (!n->collapsed && !reroute && !solid)
        ImGui::SetCursorScreenPos(Add(a, {st.padding * z, st.headerHeight * z + st.padding * z}));
    ImGui::PushItemWidth(std::max(1.f, px.x - 2 * st.padding * z));
    ImGui::BeginDisabled(f.options.readOnly || Locked(f, *n) || n->collapsed || s.zoom < st.detailZoom ||
                         n->status == Status::Disabled);
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
    const bool reroute = n->kind == NodeKind::Reroute;
    const bool group = n->kind == NodeKind::Frame || n->kind == NodeKind::Group;
    const bool solid = n->shape == NodeShape::Compact || n->shape == NodeShape::Variable;
    auto *body = ImGui::GetWindowDrawList();
    const bool detailed = !n->collapsed && !reroute && !group && f.state->zoom >= st.detailZoom;
    if (n->status == Status::Bypassed && !reroute && !group) {
        // Faint hatching over the body, and the data passing straight through.
        body->PushClipRect(a, b, true);
        const float step = 12 * std::max(z, .5f), top = solid ? a.y : a.y + float(st.headerHeight * z);
        for (float x = a.x - (b.y - top); x < b.x; x += step)
            body->AddLine({x, b.y}, {x + (b.y - top), top}, Color(WithAlpha(st.warning, .12f)), 4 * std::max(z, .5f));
        body->PopClipRect();
        const PinView *in = nullptr, *out = nullptr;
        for (auto &pin : f.graph.pins)
            if (pin.node == n->id && !pin.hidden && pin.role == PinRole::Data) {
                if (pin.kind == PinKind::Input && !in)
                    in = &pin;
                if (pin.kind == PinKind::Output && !out && (!in || !pin.type || pin.type == in->type))
                    out = &pin;
            }
        if (in && out) {
            const auto p = PinPosition(f, *in), q = PinPosition(f, *out);
            const auto points = CurvePoints(LinkStyle::Bezier, p, {1, 0}, q, {-1, 0});
            DashedPolyline(body, points, Color(st.warning), std::max(1.f, st.linkWidth), 2, 1);
        }
    }
    float footer = b.y; // Bottom edge available to the status line.
    if (detailed && n->diagnostic.severity != Severity::None && !n->diagnostic.message.empty() && !solid) {
        const ImVec4 tone = n->diagnostic.severity == Severity::Error     ? st.error
                            : n->diagnostic.severity == Severity::Warning ? st.warning
                                                                          : st.accent;
        const float band = ImGui::GetFontSize() + 8 * z;
        footer = b.y - band;
        body->AddRectFilled({a.x, footer}, b, Color(WithAlpha(tone, .18f)), st.rounding, ImDrawFlags_RoundCornersBottom);
        body->AddLine({a.x, footer}, {b.x, footer}, Color(WithAlpha(tone, .5f)));
        const std::string message(n->diagnostic.message);
        body->PushClipRect({a.x, footer}, {b.x - 16 * z, b.y}, true);
        body->AddText({a.x + st.padding * z, footer + 4 * z}, Color(tone), message.c_str());
        body->PopClipRect();
    }
    if (detailed && !solid && n->kind != NodeKind::Note) {
        auto *dl = body;
        const bool active = n->status != Status::Idle || n->milliseconds > 0 || n->progress > 0;
        if (st.status == StatusDisplay::Always || (st.status == StatusDisplay::WhenActive && active)) {
            const char *keys[] = {"node_editor.status.idle",     "node_editor.status.waiting",
                                  "node_editor.status.running",  "node_editor.status.success",
                                  "node_editor.status.error",    "node_editor.status.disabled",
                                  "node_editor.status.bypassed", "node_editor.status.cached"};
            const char *names[] = {"Idle",  "Waiting",  "Running",  "Success",
                                   "Error", "Disabled", "Bypassed", "Cached"};
            auto color = n->status == Status::Error     ? st.error
                         : n->status == Status::Success ? st.success
                                                        : st.muted;
            char text[96];
            std::snprintf(text, sizeof(text), "%s   %.2f ms", Tr(f, keys[int(n->status)], names[int(n->status)]),
                          n->milliseconds);
            dl->AddText({a.x + st.padding * z, footer - 20 * z}, Color(color), text);
        }
        if (n->status == Status::Running)
            dl->AddLine({a.x, footer - 2}, {a.x + (b.x - a.x) * std::clamp(n->progress, 0.f, 1.f), footer - 2},
                        Color(st.accent), 3);
    }
    if (n->status == Status::Disabled && !reroute)
        body->AddRectFilled(a, b, Color(WithAlpha(st.canvas, .5f)), st.rounding);
    if (f.state->connecting && !reroute && n->kind != NodeKind::Frame && n->kind != NodeKind::Group) {
        // While wiring, a node that offers no socket for the wire steps back.
        bool candidate = false;
        for (auto &pin : f.graph.pins)
            if (pin.node == n->id && !pin.hidden &&
                (pin.id == f.state->connecting ||
                 DragVerdict(f, pin.id).allowed)) {
                candidate = true;
                break;
            }
        if (!candidate)
            ImGui::GetWindowDrawList()->AddRectFilled(
                a, b, Color(WithAlpha(st.canvas, std::clamp(1 - st.incompatibleNodeAlpha, 0.f, 1.f))), st.rounding);
    }
    if (!n->collapsed && !reroute && !Locked(f, *n) && !f.options.readOnly) {
        // The grip follows the zoom but keeps a usable hit area.
        const float grip = std::clamp(14 * z, 10.f, 24.f);
        ImGui::SetCursorScreenPos({b.x - grip, b.y - grip});
        ImGui::InvisibleButton("##resize", {grip, grip});
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsItemClicked())
            BeginGesture(f, *n, EditKind::Resize);
        auto *dl = ImGui::GetWindowDrawList();
        const ImU32 color = Color(ImGui::IsItemHovered() ? st.accent : st.muted);
        dl->AddLine({b.x - grip * .8f, b.y - grip * .22f}, {b.x - grip * .22f, b.y - grip * .8f}, color);
        dl->AddLine({b.x - grip * .5f, b.y - grip * .22f}, {b.x - grip * .22f, b.y - grip * .5f}, color);
    }
    f.blocked = f.blocked || ImGui::IsAnyItemActive();
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive() &&
        ImGui::IsMouseClicked(0))
        Select(*f.state, n->id, ImGui::GetIO().KeyShift, ImGui::GetIO().KeyCtrl);
    // An empty body (locked, solid shapes) leaves the cursor set without an item; settle it inside the node.
    ImGui::SetCursorScreenPos(a);
    ImGui::Dummy({0, 0});
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
bool GetPinPosition(const EditorFrame &f, PinId id, ImVec2 &out) {
    auto *pin = FindPin(f, id);
    if (!pin || !Find(f, pin->node))
        return false;
    out = PinPosition(f, *pin);
    return true;
}
namespace {
bool EmitPin(EditorFrame &f, EditRequest r) {
    if (f.options.readOnly)
        return false;
    auto verdict = ValidatePinEdit(f.graph, r);
    bool queued = verdict.allowed && QueuePinEdits(f.graph, {&r, 1}, *f.requests, f.state->nextOperation++);
    f.state->pinEditReason.fill(0);
    if (!queued) {
        auto reason =
            verdict.allowed ? std::string_view("Request buffer full; retry the edit") : verdict.reason;
        std::copy_n(reason.data(), std::min(reason.size(), f.state->pinEditReason.size() - 1),
                    f.state->pinEditReason.begin());
    }
    return queued;
}
void ReorderTarget(EditorFrame &f, const PinView &pin, int index) {
    struct Payload {
        GraphId graph;
        NodeId node;
        PinId pin;
        PinKind kind;
        std::uint64_t group;
    };
    if (pin.capabilities.reorder && ImGui::BeginDragDropSource()) {
        Payload payload{f.graph.id, pin.node, pin.id, pin.kind, pin.capabilities.group};
        ImGui::SetDragDropPayload("IMKIT_SOCKET", &payload, sizeof(payload));
        Text(pin.label);
        ImGui::EndDragDropSource();
    }
    if (pin.capabilities.reorder && ImGui::BeginDragDropTarget()) {
        if (auto *payload = ImGui::AcceptDragDropPayload("IMKIT_SOCKET")) {
            if (payload->DataSize == sizeof(Payload)) {
                auto value = *static_cast<const Payload *>(payload->Data);
                if (value.graph == f.graph.id && value.node == pin.node && value.kind == pin.kind &&
                    value.group == pin.capabilities.group) {
                    EditRequest r;
                    r.kind = EditKind::ReorderPin;
                    r.node = pin.node;
                    r.from = value.pin;
                    r.index = index;
                    EmitPin(f, r);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}
void PinActions(EditorFrame &f, const PinView &pin) {
    auto emit = [&](EditRequest r) {
        r.node = pin.node;
        r.from = pin.id;
        return EmitPin(f, r);
    };
    char name[128]{};
    std::copy_n(pin.label.data(), std::min(pin.label.size(), sizeof(name) - 1), name);
    ImGui::BeginDisabled(!pin.capabilities.rename);
    if (ImGui::InputText("Name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue)) {
        EditRequest r;
        r.kind = EditKind::RenamePin;
        std::copy_n(name, sizeof(name), r.text.begin());
        emit(r);
    }
    ImGui::EndDisabled();
    const SocketTypeView *current = nullptr;
    for (auto &t : f.graph.socketTypes)
        if (t.id == pin.type)
            current = &t;
    ImGui::BeginDisabled(!pin.capabilities.changeType);
    if (imkit::BeginCombo("Type", current ? std::string(current->name).c_str() : "Custom")) {
        for (auto &t : f.graph.socketTypes) {
            ImGui::PushID(std::to_string(t.id).c_str());
            if (ImGui::Selectable(std::string(t.name).c_str(), t.id == pin.type)) {
                EditRequest r;
                r.kind = EditKind::ChangePinType;
                r.type = t.id;
                if (f.options.confirmPinImpact && IsPinConnected(f.graph, pin.id)) {
                    f.state->pendingPinEdit = r;
                    f.state->pendingPinEdit.node = pin.node;
                    f.state->pendingPinEdit.from = pin.id;
                } else
                    emit(r);
            }
            ImGui::PopID();
        }
        imkit::EndCombo();
    }
    ImGui::EndDisabled();
    bool shown = !pin.hidden;
    if (imkit::Checkbox("Visible", &shown)) {
        EditRequest r;
        r.kind = EditKind::HidePin;
        r.index = !shown;
        emit(r);
    }
    bool multiple = pin.multiple;
    ImGui::BeginDisabled(!pin.capabilities.changeMultiplicity);
    if (imkit::Checkbox("Multiple connections", &multiple)) {
        EditRequest r;
        r.kind = EditKind::SetPinMultiplicity;
        r.index = multiple;
        emit(r);
    }
    ImGui::EndDisabled();
    if (ImGui::SmallButton("Expose")) {
        EditRequest r;
        r.kind = EditKind::ExposePin;
        emit(r);
    }
    if (pin.capabilities.group) {
        int limits[]{pin.capabilities.minimum, pin.capabilities.maximum};
        if (ImGui::InputInt2("Min / Max", limits, ImGuiInputTextFlags_EnterReturnsTrue)) {
            EditRequest r;
            r.kind = EditKind::SetPinLimits;
            r.minimum = limits[0];
            r.maximum = limits[1];
            emit(r);
        }
    }
    EditRequest remove;
    remove.kind = EditKind::DeletePin;
    remove.node = pin.node;
    remove.from = pin.id;
    auto verdict = ValidatePinEdit(f.graph, remove);
    ImGui::BeginDisabled(!verdict.allowed);
    if (ImGui::SmallButton("Delete socket")) {
        if (f.options.confirmPinImpact && IsPinConnected(f.graph, pin.id))
            f.state->pendingPinEdit = remove;
        else
            emit(remove);
    }
    ImGui::EndDisabled();
    if (!verdict.allowed)
        ImGui::TextWrapped("%.*s", int(verdict.reason.size()), verdict.reason.data());
    if (f.state->pendingPinEdit.from == pin.id) {
        auto count = std::count_if(f.graph.links.begin(), f.graph.links.end(),
                                   [&](auto &l) { return l.from == pin.id || l.to == pin.id; });
        ImGui::TextWrapped("Affects %d link(s). The host may reject this change.", int(count));
        if (ImGui::SmallButton("Confirm")) {
            emit(f.state->pendingPinEdit);
            f.state->pendingPinEdit = {};
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cancel"))
            f.state->pendingPinEdit = {};
    }
    if (!pin.capabilities.reason.empty())
        ImGui::TextWrapped("%.*s", int(pin.capabilities.reason.size()), pin.capabilities.reason.data());
}
} // namespace
void PinAddRow(EditorFrame &f, NodeId node, PinKind kind, std::uint64_t group) {
    auto *n = Find(f, node);
    if (!n)
        return;
    auto caps = kind == PinKind::Input ? n->inputs : n->outputs;
    if (!caps.add)
        return;
    int count = 0;
    for (auto &p : f.graph.pins)
        if (p.node == node && p.kind == kind && p.capabilities.group == group)
            ++count;
    ImGui::PushID(std::to_string(node.value).c_str());
    ImGui::PushID(int(kind));
    ImGui::PushID(std::to_string(group).c_str());
    ImGui::BeginDisabled(f.options.readOnly || count >= caps.maximum || n->locked);
    if (ImGui::SmallButton(kind == PinKind::Input ? "+ Input" : "+ Output"))
        ImGui::OpenPopup("Socket type");
    if (ImGui::BeginPopup("Socket type")) {
        for (auto &type : f.graph.socketTypes) {
            ImGui::PushID(std::to_string(type.id).c_str());
            if (ImGui::Selectable(std::string(type.name).c_str())) {
                EditRequest r;
                r.kind = EditKind::CreatePin;
                r.node = node;
                r.pinKind = kind;
                r.index = count;
                r.type = type.id;
                r.group = group;
                std::copy_n(type.name.data(), std::min(type.name.size(), r.text.size() - 1), r.text.begin());
                EmitPin(f, r);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::EndDisabled();
    ImGui::PopID();
    ImGui::PopID();
    ImGui::PopID();
}
void PinRow(EditorFrame &f, PinId id, PinRowOptions options) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    auto *pin = FindPin(f, id);
    if (!pin || pin->node != f.node->id || Concealed(f, *pin))
        return;
    auto &s = *f.state;
    ImGui::PushID(std::to_string(id.value).c_str());
    ImGui::BeginGroup();
    auto start = ImGui::GetCursorScreenPos();
    float rowHeight = std::max(ImGui::GetFrameHeight(), f.style.headerHeight * float(s.zoom) * .7f);
    const bool detail = s.zoom >= Style(f, *f.node).detailZoom;
    bool connected = IsPinConnected(f.graph, id);
    bool showValue = detail && pin->kind == PinKind::Input && options.value.kind != ValueKind::None &&
                     (!connected || !options.hideConnectedValue);
    float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    float labelWidth = showValue ? width * .43f : width;
    ImGui::Selectable("##label", false, ImGuiSelectableFlags_None, {labelWidth, rowHeight});
    auto *dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(start, {start.x + labelWidth, start.y + rowHeight}, true);
    auto label = std::string(pin->label);
    if (pin->capabilities.required)
        label += " *";
    float x = pin->kind == PinKind::Output
                  ? std::max(start.x, start.x + labelWidth - ImGui::CalcTextSize(label.c_str()).x)
                  : start.x;
    dl->AddText({x, start.y + (rowHeight - ImGui::GetFontSize()) * .5f}, Color(f.style.text), label.c_str());
    dl->PopClipRect();
    if (ImGui::IsItemHovered()) {
        for (auto &type : f.graph.socketTypes)
            if (type.id == pin->type)
                ImGui::SetTooltip("%.*s (%.*s)", int(pin->label.size()), pin->label.data(),
                                  int(type.name.size()), type.name.data());
    }
    int index = 0;
    for (auto &p : f.graph.pins) {
        if (p.id == id)
            break;
        if (p.node == pin->node && p.kind == pin->kind && p.capabilities.group == pin->capabilities.group)
            ++index;
    }
    ReorderTarget(f, *pin, index);
    if (ImGui::BeginPopupContextItem("Socket")) {
        PinActions(f, *pin);
        ImGui::EndPopup();
    }
    if (showValue) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x));
        bool editable = !connected && !f.options.readOnly && !Locked(f, *f.node) && !s.valueTerminalPending;
        ImGui::BeginDisabled(!editable);
        auto value = s.valueEditing && s.valueEdit.from == id ? s.valueEdit.value : options.value;
        bool changed = false;
        Phase customPhase = Phase::Update;
        switch (value.kind) {
        case ValueKind::Color:
            changed = ImGui::ColorEdit4("##value", value.number.data(), ImGuiColorEditFlags_NoInputs);
            break;
        case ValueKind::Float:
            changed = imkit::SliderFloat("##value", value.number.data(), options.minimum, options.maximum);
            break;
        case ValueKind::Vector: {
            auto padding = ImGui::GetStyle().FramePadding;
            padding.x = std::min(padding.x, 2.f * float(s.zoom));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
            changed = ImGui::DragFloat3("##value", value.number.data(), .01f, 0, 0, "%.2g");
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("X: %.6g  Y: %.6g  Z: %.6g", value.number[0], value.number[1],
                                  value.number[2]);
            break;
        }
        case ValueKind::Integer:
            changed = ImGui::InputInt("##value", &value.integer);
            break;
        case ValueKind::Boolean:
            changed = imkit::Checkbox("##value", &value.boolean);
            break;
        case ValueKind::Text:
            changed = ImGui::InputText("##value", value.text.data(), value.text.size());
            break;
        case ValueKind::Enum:
            if (imkit::BeginCombo("##value", value.integer >= 0 && value.integer < int(options.choices.size())
                                                 ? std::string(options.choices[value.integer]).c_str()
                                                 : "Choose")) {
                for (int i = 0; i < int(options.choices.size()); ++i)
                    if (ImGui::Selectable(std::string(options.choices[i]).c_str(), value.integer == i)) {
                        value.integer = i;
                        changed = true;
                        customPhase = Phase::Commit;
                    }
                imkit::EndCombo();
            }
            break;
        case ValueKind::Custom:
            if (options.draw)
                changed = options.draw(options.user, value, customPhase);
            break;
        default:
            break;
        }
        bool activated = ImGui::IsItemActivated(), active = ImGui::IsItemActive(),
             deactivated = ImGui::IsItemDeactivated();
        ImGui::EndDisabled();
        if (editable && !s.valueEditing &&
            (activated || changed || (value.kind == ValueKind::Custom && customPhase == Phase::Begin))) {
            EditRequest r;
            r.kind = EditKind::SetPinValue;
            r.node = pin->node;
            r.from = id;
            r.value = options.value;
            r.previousValue = options.value;
            r.graph = f.graph.id;
            r.revision = f.graph.revision;
            r.operation = s.nextOperation++;
            r.phase = Phase::Begin;
            if (f.requests->Push(r)) {
                s.valueEditing = true;
                s.valueEdit = r;
            }
        }
        if (s.valueEditing && s.valueEdit.from == id && !s.valueTerminalPending) {
            if (changed) {
                auto r = s.valueEdit;
                r.value = value;
                r.phase = Phase::Update;
                f.requests->Push(r);
                s.valueEdit = r;
            }
            bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || customPhase == Phase::Cancel;
            if (cancel || (value.kind == ValueKind::Custom
                               ? customPhase == Phase::Commit
                               : deactivated || (changed && (!active || customPhase == Phase::Commit)))) {
                s.valueEdit.phase = cancel ? Phase::Cancel : Phase::Commit;
                if (f.requests->Push(s.valueEdit))
                    s.valueEditing = false;
                else
                    s.valueTerminalPending = true;
            }
        }
    }
    ImGui::EndGroup();
    auto local = World(f, {start.x, start.y + rowHeight * .5f});
    auto origin = Position(f, *f.node);
    auto row =
        std::find_if(s.rowPositions.begin(), s.rowPositions.end(), [&](auto &r) { return r.pin == id; });
    if (row == s.rowPositions.end())
        s.rowPositions.push_back({id, {local.x - origin.x, local.y - origin.y}});
    else
        row->local = {local.x - origin.x, local.y - origin.y};
    ImGui::PopID();
}
bool BeginNodePanel(EditorFrame &f, std::uint64_t panel, std::string_view label, bool *open) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    auto &s = *f.state;
    const auto &st = Style(f, *f.node);
    PanelState *state = nullptr;
    for (auto &p : s.panels)
        if (p.node == f.node->id && p.panel == panel)
            state = &p;
    if (!state) {
        s.panels.push_back({f.node->id, panel, open ? *open : true, 0});
        state = &s.panels.back();
    }
    if (open)
        state->open = *open;
    char key[48];
    std::snprintf(key, sizeof(key), "panel-%llu", static_cast<unsigned long long>(panel));
    ImGui::PushID(key);
    const auto start = ImGui::GetCursorScreenPos();
    const float z = float(s.zoom);
    const float rowHeight = std::max(ImGui::GetFrameHeight(), st.headerHeight * z * .7f);
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    if (ImGui::InvisibleButton("##title", {width, rowHeight})) {
        state->open = !state->open;
        if (open)
            *open = state->open;
    }
    const bool hovered = ImGui::IsItemHovered();
    auto *dl = ImGui::GetWindowDrawList();
    const float cy = start.y + rowHeight * .5f;
    dl->AddLine({start.x, start.y}, {start.x + width, start.y}, Color(WithAlpha(st.border, .7f)));
    const float k = ImGui::GetFontSize() * .26f;
    const ImU32 ink = Color(hovered ? st.text : st.muted);
    if (state->open)
        dl->AddTriangleFilled({start.x, cy - k * .6f}, {start.x + 2 * k, cy - k * .6f}, {start.x + k, cy + k * .9f}, ink);
    else
        dl->AddTriangleFilled({start.x + k * .3f, cy - k}, {start.x + k * 1.8f, cy}, {start.x + k * .3f, cy + k}, ink);
    const std::string text(label);
    dl->AddText({start.x + 3 * k, cy - ImGui::GetFontSize() * .5f}, Color(st.text), text.c_str());
    if (!state->open) { // How many sockets wait on the closed row.
        int count = 0;
        for (auto &pin : f.graph.pins)
            count += pin.node == f.node->id && pin.panel == panel && !Concealed(f, pin);
        if (count) {
            char number[16];
            std::snprintf(number, sizeof(number), "%d", count);
            const auto size = ImGui::CalcTextSize(number);
            dl->AddText({start.x + width - size.x, cy - size.y * .5f}, Color(st.muted), number);
        }
    }
    state->row = World(f, {start.x, cy}).y - Position(f, *f.node).y;
    if (!state->open)
        ImGui::PopID();
    return state->open;
}
void EndNodePanel(EditorFrame &f) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    ImGui::PopID();
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
    auto points = LinkCurve(f, link, *a, *b);
    // The stored direction may be either way round; colour sources speak of the output and the input.
    const PinView &output = a->kind == PinKind::Output ? *a : *b;
    const PinView &input = a->kind == PinKind::Output ? *b : *a;
    auto &s = *f.state;
    ImVec4 from = f.style.accent, to = f.style.accent;
    if (link.color.w > 0)
        from = to = link.color;
    else
        switch (f.style.linkColor) {
        case LinkColorSource::OutputType:
            if (HasTypeColor(f, output))
                from = to = PinColor(f, output);
            break;
        case LinkColorSource::InputType:
            if (HasTypeColor(f, input))
                from = to = PinColor(f, input);
            break;
        case LinkColorSource::Gradient:
            if (HasTypeColor(f, output))
                from = PinColor(f, output);
            to = HasTypeColor(f, input) ? PinColor(f, input) : from;
            if (a->kind != PinKind::Output)
                std::swap(from, to);
            break;
        case LinkColorSource::Accent:
            break;
        }
    const bool flow = output.role == PinRole::Flow;
    if (flow && link.color.w <= 0) // Execution order reads apart from data: the text colour, thicker.
        from = to = f.style.text;
    const bool selected = s.selectedLink == link.id;
    const bool invalid = link.validity == LinkValidity::Invalid;
    if (selected)
        from = to = f.style.selected;
    else if (invalid)
        from = to = f.style.error;
    auto *dl = ImGui::GetWindowDrawList();
    const float linkWidth = link.width > 0 ? link.width : flow ? f.style.flowLinkWidth : f.style.linkWidth;
    bool emphasized = selected;
    if (f.style.highlightConnectedLinks) {
        const auto *na = Find(f, a->node), *nb = Find(f, b->node);
        emphasized = emphasized || IsSelected(s, na->id) || IsSelected(s, nb->id) || f.hoveredNode == na->id ||
                     f.hoveredNode == nb->id;
        if (!emphasized && f.hovered && !s.connecting && !f.hoveredNode) {
            const auto mouse = ImGui::GetIO().MousePos;
            for (std::size_t i = 1; i < points.size() && !emphasized; ++i)
                emphasized = SegmentDistance(mouse, points[i - 1], points[i]) < 6;
        }
        if (emphasized) // A soft halo under the line keeps the hue and reads in light and dark themes.
            dl->AddPolyline(points.data(), static_cast<int>(points.size()), Color(WithAlpha(Mix(from, to, .5f), .3f)),
                            ImDrawFlags_None, linkWidth + 5);
    }
    const float width = emphasized && f.style.highlightConnectedLinks ? linkWidth + 1 : linkWidth;
    LinkChromeView chrome;
    chrome.link = &link;
    chrome.points = points;
    chrome.from = from;
    chrome.to = to;
    chrome.width = width;
    chrome.hovered = emphasized && !selected;
    chrome.selected = selected;
    chrome.flow = flow;
    const bool hooked = f.style.hooks && f.style.hooks->link && f.style.hooks->link(dl, chrome, f.style.hooks->user);
    const int start = dl->VtxBuffer.Size;
    if (hooked) { // The host drew the line; markers and labels below still apply.
    } else if (invalid) // Long dashes: the broken state reads from the line itself, not only from its colour.
        DashedPolyline(dl, points, Color(from), width, 2, 1);
    else if (!link.reference) {
        // One polyline keeps joins smooth; per-segment lines show notches at the joints.
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), Color(from), ImDrawFlags_None, width);
    } else
        DashedPolyline(dl, points, Color(from), width, 1, 1);
    const bool gradient = !hooked && (from.x != to.x || from.y != to.y || from.z != to.z || from.w != to.w);
    if (gradient) { // Shade along the dominant axis between the two ends.
        const auto p = points.front(), q = points.back();
        const float length = std::max(1.f, std::abs(q.x - p.x));
        for (int i = start; i < dl->VtxBuffer.Size; ++i) {
            auto &v = dl->VtxBuffer[i];
            const float t = std::clamp((v.pos.x - p.x) / (q.x - p.x == 0 ? length : q.x - p.x), 0.f, 1.f);
            auto c = Mix(from, to, t);
            c.w *= ImGui::ColorConvertU32ToFloat4(v.col).w / std::max(.001f, from.w);
            v.col = Color(c);
        }
    }
    float curveLength = 0;
    for (std::size_t i = 1; i < points.size(); ++i)
        curveLength += std::sqrt(Dot(Sub(points[i], points[i - 1]), Sub(points[i], points[i - 1])));
    // Chevrons cut into the line show the direction of execution; a short link keeps only the middle one.
    const std::size_t marks[] = {12, 7, 17};
    const std::size_t markCount = curveLength > 160 ? 3 : 1;
    if (flow && !invalid && !link.reference)
        for (std::size_t m = 0; m < markCount; ++m) {
            const std::size_t i = marks[m];
            const bool forward = a->kind == PinKind::Output;
            auto d = forward ? Sub(points[i + 1], points[i - 1]) : Sub(points[i - 1], points[i + 1]);
            const float len = std::sqrt(Dot(d, d));
            if (len <= 0)
                continue;
            d = Mul(d, 1 / len);
            const ImVec2 n{-d.y, d.x};
            const float k = std::max(2.f, width * .8f);
            const auto c = points[i];
            dl->AddPolyline(std::array<ImVec2, 3>{Add(Sub(c, Mul(d, k * .6f)), Mul(n, k)), Add(c, Mul(d, k * .6f)),
                                                  Sub(Sub(c, Mul(d, k * .6f)), Mul(n, k))}
                                .data(),
                            3, Color(f.style.canvas), ImDrawFlags_None, std::max(1.2f, width * .45f));
        }
    if (f.style.linkArrow != LinkArrow::None && !flow) {
        const bool atEnd = f.style.linkArrow == LinkArrow::End;
        // Arrows point from output to input whatever order the host stored the ends in.
        const bool forward = a->kind == PinKind::Output;
        std::size_t i = atEnd ? (forward ? 23 : 1) : 18;
        auto tip = points[i];
        auto direction = forward ? Sub(points[i], points[i - 1]) : Sub(points[i - 1], points[i]);
        const auto length = std::sqrt(Dot(direction, direction));
        if (length > 0) {
            direction = Mul(direction, 1 / length);
            if (atEnd) // Stop short of the socket so the arrow does not cover it.
                tip = Sub(forward ? points.back() : points.front(),
                          Mul(direction, f.style.pinRadius * std::clamp(float(s.zoom), .8f, 2.f) + 3));
            auto normal = ImVec2{-direction.y, direction.x};
            const auto color = Color(atEnd ? (forward ? to : from) : Mix(from, to, .75f));
            dl->AddTriangleFilled(tip, Add(Sub(tip, Mul(direction, 7)), Mul(normal, 3)),
                                  Sub(Sub(tip, Mul(direction, 7)), Mul(normal, 3)), color);
        }
    }
    const bool bead = link.validity != LinkValidity::Valid;
    if (bead) {
        DrawBead(dl, points[12], link.validity, f.style, s.zoom);
        const auto mouse = ImGui::GetIO().MousePos;
        const float r = BeadRadius(s.zoom) + 2;
        const bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        if (f.hovered && !popup && !s.connecting && Inside(mouse, Sub(points[12], {r, r}), Add(points[12], {r, r}))) {
            const char *fallback = link.validity == LinkValidity::Invalid  ? "Invalid link"
                                   : link.validity == LinkValidity::Cycle ? "Creates a cycle"
                                                                          : "Converted";
            const char *key = link.validity == LinkValidity::Invalid  ? "node_editor.link.invalid"
                              : link.validity == LinkValidity::Cycle ? "node_editor.link.cycle"
                                                                     : "node_editor.link.converted";
            if (link.reason.empty())
                ImGui::SetTooltip("%s", Tr(f, key, fallback));
            else
                ImGui::SetTooltip("%s: %.*s", Tr(f, key, fallback), int(link.reason.size()), link.reason.data());
        }
    }
    if (!link.label.empty()) { // A pill keeps the label legible over grid and other links.
        const auto size = ImGui::CalcTextSize(link.label.data(), link.label.data() + link.label.size());
        const auto c = points[bead ? 7 : 12];
        const ImVec2 pad{6, 2};
        const ImVec2 lo{c.x - size.x / 2 - pad.x, c.y - size.y / 2 - pad.y};
        const ImVec2 hi{c.x + size.x / 2 + pad.x, c.y + size.y / 2 + pad.y};
        dl->AddRectFilled(lo, hi, Color(f.style.surface), (hi.y - lo.y) / 2);
        dl->AddRect(lo, hi, Color(WithAlpha(Mix(from, to, .5f), .6f)), (hi.y - lo.y) / 2);
        dl->AddText(Add(lo, pad), Color(f.style.muted), link.label.data(), link.label.data() + link.label.size());
    }
    if (link.active) { // Marks travel from output to input; they hold still when motion is off.
        std::array<float, 25> along{};
        for (std::size_t i = 1; i < points.size(); ++i) {
            const auto d = Sub(points[i], points[i - 1]);
            along[i] = along[i - 1] + std::sqrt(Dot(d, d));
        }
        const float spacing = std::max(4.f, f.style.flowSpacing);
        const float travelled = f.style.animate ? float(ImGui::GetTime()) * 60 * f.style.flowSpeed : 0;
        const bool forward = a->kind == PinKind::Output;
        const ImU32 mark = Color(flow ? f.style.accent : f.style.success);
        const float radius = std::max(1.5f, width * .65f);
        std::size_t segment = 1;
        for (float d = std::fmod(travelled, spacing); d < along.back(); d += spacing) {
            const float at = forward ? d : along.back() - d;
            while (segment + 1 < points.size() && along[segment] < at)
                ++segment;
            while (segment > 1 && along[segment - 1] > at)
                --segment;
            const float span = std::max(.001f, along[segment] - along[segment - 1]);
            const float t = std::clamp((at - along[segment - 1]) / span, 0.f, 1.f);
            dl->AddCircleFilled(Add(points[segment - 1], Mul(Sub(points[segment], points[segment - 1]), t)), radius,
                                mark);
        }
    }
}
void DrawLinks(EditorFrame &f) {
    f.linksQueued = true; // EndEditor draws after standard row positions are known.
}
void DrawNodes(EditorFrame &f, void (*body)(void *, EditorFrame &, const NodeView &), void *user) {
    for (auto &n : f.graph.nodes)
        if (BeginNode(f, n.id)) {
            if (!n.collapsed && n.kind != NodeKind::Reroute && n.shape != NodeShape::Compact &&
                n.shape != NodeShape::Variable) {
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
namespace {
// The node's input and output that can take the two halves of `link`.
bool FindInsertion(const EditorFrame &f, NodeId node, LinkId id, PinId &in, PinId &out) {
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
                    in = input.id;
                    out = output.id;
                    return true;
                }
    return false;
}
} // namespace
bool InsertNode(EditorFrame &f, NodeId node, LinkId id) {
    auto *n = Find(f, node);
    if (!n || Locked(f, *n) || f.options.readOnly)
        return false;
    PinId in{}, out{};
    if (!FindInsertion(f, node, id, in, out))
        return false;
    auto r = Request(f, EditKind::InsertNode, node);
    r.link = id;
    r.from = in;
    r.to = out;
    return f.requests->Push(r);
}
bool ConnectSelected(EditorFrame &f) {
    auto &sel = f.state->selection;
    if (f.options.readOnly || sel.size() != 2)
        return false;
    auto *a = Find(f, sel[0]), *b = Find(f, sel[1]);
    if (!a || !b)
        return false;
    if (Position(f, *b).x < Position(f, *a).x)
        std::swap(a, b);
    for (auto &output : f.graph.pins)
        if (output.node == a->id && output.kind == PinKind::Output && !output.hidden)
            for (auto &input : f.graph.pins)
                if (input.node == b->id && input.kind == PinKind::Input && !input.hidden &&
                    !IsPinConnected(f.graph, input.id) && CanConnect(f.graph, output.id, input.id).allowed) {
                    auto r = Request(f, EditKind::CreateLink);
                    r.from = output.id;
                    r.to = input.id;
                    return f.requests->Push(r);
                }
    return false;
}
void LayoutToolbar(EditorFrame &f) {
    ImGui::BeginDisabled(f.options.readOnly || f.state->selection.empty());
    if (ImGui::Button("Arrange"))
        ImGui::OpenPopup("Layout actions");
    if (ImGui::BeginPopup("Layout actions")) {
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
        ImGui::EndPopup();
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
    float button = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(
        std::max(1.f, ImGui::GetContentRegionAvail().x - button - ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##node-search", "Find in graph", s.search.data(), s.search.size());
    ImGui::SameLine();
    if (ImGui::Button("...##navigation", {button, button}))
        ImGui::OpenPopup("Navigation");
    if (ImGui::BeginPopup("Navigation")) {
        if (ImGui::MenuItem("Back", "", false, s.historyIndex > 0))
            NavigateHistory(s, -1);
        if (ImGui::MenuItem("Forward", "", false, s.historyIndex + 1 < s.history.size()))
            NavigateHistory(s, 1);
        ImGui::Separator();
        for (bool upstream : {true, false})
            if (ImGui::MenuItem(upstream ? "Select upstream" : "Select downstream", nullptr, false,
                                bool(s.active))) {
                std::vector<NodeId> nodes(f.graph.nodes.size());
                auto result = TraceNodes(f.graph, s.active, upstream, nodes);
                if (result)
                    s.selection.assign(nodes.begin(), nodes.begin() + result.count);
            }
        if (ImGui::MenuItem("Clear search", nullptr, false, s.search[0] != 0))
            s.search.fill(0);
        ImGui::EndPopup();
    }
    if (!s.search[0])
        return;
    ImGui::BeginChild("##search-results", {0, ImGui::GetTextLineHeightWithSpacing() * 6},
                      ImGuiChildFlags_Borders);
    bool found = false;
    auto query = std::string_view(s.search.data());
    for (auto &n : f.graph.nodes) {
        auto match = [&](std::string_view text) {
            return std::search(text.begin(), text.end(), query.begin(), query.end(),
                               [](unsigned char a, unsigned char b) {
                                   return std::tolower(a) == std::tolower(b);
                               }) != text.end();
        };
        if (!match(n.title) && !match(n.description))
            continue;
        found = true;
        ImGui::PushID(std::to_string(n.id.value).c_str());
        if (ImGui::Selectable(std::string(n.title).c_str(), IsSelected(s, n.id))) {
            RememberView(s);
            Select(s, n.id);
            FrameNodes(s, f.graph, {&n.id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            RememberView(s);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", int(n.title.size()), n.title.data());
        ImGui::PopID();
    }
    if (!found)
        ImGui::TextDisabled("No matching nodes");
    ImGui::EndChild();
}
void NodeInspector(EditorFrame &f, NodeId id) {
    auto *n = Find(f, id);
    if (!n) {
        ImGui::TextWrapped(
            "Select a node to edit its inputs and outputs. Use Tab on the canvas to add a node.");
        return;
    }
    if (f.state->pinEditReason[0])
        ImGui::TextWrapped("%s", f.state->pinEditReason.data());
    ImGui::PushID(std::to_string(id.value).c_str());
    char title[128]{};
    std::copy_n(n->title.data(), std::min(n->title.size(), sizeof(title) - 1), title);
    ImGui::BeginDisabled(f.options.readOnly || Locked(f, *n));
    const bool rename = ImGui::InputText("Name", title, sizeof(title), ImGuiInputTextFlags_EnterReturnsTrue);
    if ((rename || ImGui::IsItemDeactivatedAfterEdit()) && std::string_view(title) != n->title) {
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
    ImGui::BeginDisabled(Locked(f, *n));
    for (auto kind : {PinKind::Input, PinKind::Output}) {
        ImGui::SeparatorText(kind == PinKind::Input ? "Inputs" : "Outputs");
        PinAddRow(f, id, kind, (kind == PinKind::Input ? n->inputs : n->outputs).group);
        int index = 0;
        for (auto &pin : f.graph.pins)
            if (pin.node == id && pin.kind == kind) {
                ImGui::PushID(std::to_string(pin.id.value).c_str());
                bool open = ImGui::TreeNodeEx("socket", ImGuiTreeNodeFlags_None, "%.*s",
                                              int(pin.label.size()), pin.label.data());
                int groupIndex = 0;
                for (auto &previous : f.graph.pins) {
                    if (previous.id == pin.id)
                        break;
                    if (previous.node == id && previous.kind == kind &&
                        previous.capabilities.group == pin.capabilities.group)
                        ++groupIndex;
                }
                ReorderTarget(f, pin, groupIndex);
                ++index;
                if (open) {
                    PinActions(f, pin);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        if (!index)
            ImGui::TextDisabled("No sockets. Add one above if allowed by the host.");
    }
    ImGui::EndDisabled();
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
    ImGui::PopID();
}
void ExposedProperties(EditorFrame &f, NodeId node, std::span<const PropertyView> properties) {
    ImGui::BeginDisabled(f.options.readOnly);
    for (std::size_t i = 0; i < properties.size(); ++i) {
        auto &property = properties[i];
        ImGui::PushID(int(i));
        bool exposed = property.exposed;
        if (imkit::Checkbox(std::string(property.label).c_str(), &exposed)) {
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
        f.state->paletteWasOpen = true;
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
            if (f.state->connecting &&
                (!entry.compatible || !entry.compatible(entry.user, f.state->connecting)))
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
                r.link = f.state->reconnecting;
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
    } else if (f.state->paletteWasOpen) {
        f.state->paletteWasOpen = false;
        f.state->connecting = {};
        f.state->reconnecting = {};
    }
}
void MiniMap(EditorFrame &f, ImVec2 size) {
    if (f.graph.nodes.empty() || size.x <= 0 || size.y <= 0)
        return;
    const float margin = f.style.padding;
    size.x = std::min(size.x, f.max.x - f.min.x - 2 * margin);
    size.y = std::min(size.y, f.max.y - f.min.y - 2 * margin);
    if (size.x < 24 || size.y < 24)
        return;
    auto a = ImVec2{f.max.x - size.x - margin, f.max.y - size.y - margin}, b = Add(a, size);
    auto cursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::BeginChild("##minimap", size, ImGuiChildFlags_None,
                                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                         ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();
    if (!visible) {
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy({0, 0});
        return;
    }
    ImGui::InvisibleButton("##navigate", size);
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
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
    dl->AddRectFilled(a, b, Color(f.style.header), f.style.rounding);
    dl->AddRect(a, b, Color(f.style.border), f.style.rounding);
    dl->PushClipRect(a, b, true);
    for (auto &n : f.graph.nodes)
        dl->AddRectFilled(map(n.position), map({n.position.x + n.size.x, n.position.y + n.size.y}),
                          Color(IsSelected(*f.state, n.id) ? f.style.accent : f.style.muted), 1);
    const auto viewMin = map(f.state->origin),
               viewMax = map({f.state->origin.x + (f.max.x - f.min.x) / f.state->zoom,
                              f.state->origin.y + (f.max.y - f.min.y) / f.state->zoom});
    auto viewFill = f.style.accent;
    viewFill.w *= .12f;
    dl->AddRectFilled(viewMin, viewMax, Color(viewFill), 2);
    dl->AddRect(viewMin, viewMax, Color(f.style.accent), 2);
    dl->PopClipRect();
    if (hovered || active) {
        f.blocked = true;
        if (active && ImGui::IsMouseDown(0)) {
            auto p = ImGui::GetIO().MousePos;
            f.state->origin = {
                bounds.min.x + (p.x - a.x - 6) / scale - (f.max.x - f.min.x) / f.state->zoom / 2,
                bounds.min.y + (p.y - a.y - 6) / scale - (f.max.y - f.min.y) / f.state->zoom / 2};
        }
    }
    ImGui::EndChild();
    ImGui::SetCursorScreenPos(cursor);
    ImGui::Dummy({0, 0});
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
    const float available = ImGui::GetContentRegionAvail().x;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i) {
            if (ImGui::GetContentRegionAvail().x < ImGui::GetFrameHeight() * 3)
                ImGui::NewLine();
            else {
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
            }
        }
        ImGui::PushID(int(i));
        float width =
            std::min(std::max(1.f, available), ImGui::CalcTextSize(std::string(path[i].title).c_str()).x +
                                                   2 * ImGui::GetStyle().FramePadding.x);
        if (ImGui::GetContentRegionAvail().x < width)
            ImGui::NewLine();
        if (ImGui::Button(std::string(path[i].title).c_str(),
                          {std::min(width, std::max(1.f, ImGui::GetContentRegionAvail().x)), 0})) {
            auto r = Request(f, EditKind::EnterGraph, path[i].node);
            r.type = path[i].graph.value;
            f.requests->Push(r);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", int(path[i].title.size()), path[i].title.data());
        ImGui::PopID();
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
    if (ImGui::SmallButton("...##preview-actions"))
        ImGui::OpenPopup("Preview actions");
    if (ImGui::BeginPopup("Preview actions")) {
        if (ImGui::MenuItem("Open"))
            p.expanded = true;
        ImGui::MenuItem("Pin", nullptr, &p.pinned);
        ImGui::MenuItem("Interact", nullptr, &p.interactive);
        ImGui::EndPopup();
    }
    if (p.collapsed || outputs.empty())
        return;
    p.output = std::clamp(p.output, 0, int(outputs.size()) - 1);
    if (outputs.size() > 1) {
        if (imkit::BeginCombo("##preview-output", std::string(outputs[p.output].title).c_str())) {
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.output))
                    p.output = i;
            imkit::EndCombo();
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
        imkit::Checkbox("Interact", &p.interactive);
        ImGui::SameLine();
        imkit::Checkbox("Pinned", &p.pinned);
        if (outputs.size() > 1 &&
            imkit::BeginCombo(
                "Compare",
                p.compare < 0
                    ? "None"
                    : std::string(outputs[std::min(p.compare, int(outputs.size()) - 1)].title).c_str())) {
            if (ImGui::Selectable("None", p.compare < 0))
                p.compare = -1;
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.compare))
                    p.compare = i;
            imkit::EndCombo();
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
        if (f.linksQueued)
            for (auto &link : f.graph.links)
                Link(f, link);
        auto *dl = ImGui::GetWindowDrawList();
        const PinView *hoveredPin = nullptr;
        const PinView *snap = nullptr; // Nearest compatible socket while wiring.
        float snapBest = std::max(0.f, f.style.snapDistance);
        for (auto &pin : f.graph.pins) {
            auto *n = Find(f, pin.node);
            if (!n || Concealed(f, pin) || !Visible(f, *n))
                continue;
            auto p = PinPosition(f, pin);
            const auto &st = Style(f, *n);
            if (n->kind == NodeKind::Reroute) { // The knot itself is the socket.
                // While wiring, the knot accepts the direction that completes the wire; otherwise its ring
                // starts a new wire from the output.
                const float knot = KnotRadius(s.zoom);
                const float d = std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y);
                const auto *from = s.connecting ? FindPin(f, s.connecting) : nullptr;
                const bool wanted = from ? (s.movingLinks ? from->kind == pin.kind && from->id != pin.id
                                                          : from->kind != pin.kind) &&
                                               d <= knot + kKnotRing
                                         : pin.kind == PinKind::Output && d > knot * .85f && d <= knot + kKnotRing;
                if (wanted)
                    hoveredPin = &pin;
                continue;
            }
            float r = st.pinRadius * std::clamp(float(s.zoom), .8f, 2.f);
            ImVec4 tone = PinColor(f, pin);
            auto shape = pin.shape;
            for (auto &type : f.graph.socketTypes)
                if (type.id == pin.type && pin.inheritTypeStyle)
                    shape = type.shape;
            const bool connected = IsPinConnected(f.graph, pin.id);
            if (s.connecting && s.connecting != pin.id) {
                auto verdict = DragVerdict(f, pin.id);
                if (!verdict.allowed) // Sockets that cannot take the wire step back instead of shouting.
                    tone = WithAlpha(tone, std::clamp(st.incompatibleAlpha, 0.f, 1.f));
                else {
                    const bool convertible = verdict.match == ConnectionMatch::Convertible;
                    dl->AddCircle(p, r + 4, Color(convertible ? f.style.warning : f.style.success), 0, 2.f);
                    const float distance = std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y);
                    if (distance <= snapBest) {
                        snapBest = distance;
                        snap = &pin;
                    }
                }
            }
            for (auto &link : f.graph.links) // An input fed by a link the host marked invalid shows it.
                if (link.validity == LinkValidity::Invalid && pin.kind == PinKind::Input &&
                    (link.to == pin.id || link.from == pin.id)) {
                    dl->AddCircle(p, r + 3, Color(f.style.error), 0, 2.f);
                    break;
                }
            const ImU32 c = Color(tone);
            if (pin.multiple && !IsMultiInput(pin))
                dl->AddCircle(p, r + 2, c);
            if (pin.capabilities.group)
                dl->AddLine(Add(p, {-r, -r - 4}), Add(p, {r, -r - 4}), c, 2);
            PinChromeView glyph;
            glyph.pin = &pin;
            glyph.center = p;
            glyph.radius = r;
            glyph.seat = st.pinSeat * std::clamp(float(s.zoom), .8f, 2.f);
            glyph.color = tone;
            glyph.canvas = st.canvas;
            glyph.shape = shape;
            glyph.connected = connected;
            glyph.hollow = st.hollowUnconnectedPins;
            glyph.hovered = Inside(io.MousePos, Sub(p, {r + 5, r + 5}), Add(p, {r + 5, r + 5}));
            if (pin.keyType)
                for (auto &type : f.graph.socketTypes)
                    if (type.id == pin.keyType)
                        glyph.keyColor = type.color;
            if (IsMultiInput(pin))
                for (auto &link : f.graph.links)
                    glyph.links += link.to == pin.id || link.from == pin.id;
            if (!(st.hooks && st.hooks->pin && st.hooks->pin(dl, glyph, st.hooks->user)))
                DrawPinGlyph(dl, glyph);
            // A required input that has nothing connected carries a small error mark.
            if (pin.kind == PinKind::Input && pin.capabilities.required && !connected) {
                const ImVec2 m = Add(p, {r * .85f, -r * .85f});
                dl->AddCircleFilled(m, std::max(2.5f, r * .5f) + 1.2f, Color(st.canvas));
                dl->AddCircleFilled(m, std::max(2.5f, r * .5f), Color(st.error));
            }
            if (Inside(io.MousePos, Sub(p, {r + 5, r + 5}), Add(p, {r + 5, r + 5})))
                hoveredPin = &pin;
        }
        if (s.connecting && !hoveredPin && snap)
            hoveredPin = snap;
        // Dragging one node over a link it could split: the link lights up, and releasing inserts the node.
        s.dropLink = {};
        if (s.gesture.size() == 1 && s.gesture.front().kind == EditKind::Move && !f.options.readOnly) {
            const NodeId moving = s.gesture.front().node;
            for (auto &link : f.graph.links) {
                auto *x = FindPin(f, link.from), *y = FindPin(f, link.to);
                PinId in{}, out{};
                if (!x || !y || x->node == moving || y->node == moving ||
                    !FindInsertion(f, moving, link.id, in, out))
                    continue;
                const auto points = LinkCurve(f, link, *x, *y);
                bool near = false;
                for (std::size_t i = 1; i < points.size() && !near; ++i)
                    near = SegmentDistance(io.MousePos, points[i - 1], points[i]) < 10;
                if (near) {
                    s.dropLink = link.id;
                    dl->AddPolyline(points.data(), int(points.size()), Color(WithAlpha(f.style.accent, .45f)),
                                    ImDrawFlags_None, f.style.linkWidth + 6);
                    break;
                }
            }
        }
        if (f.options.minimap)
            MiniMap(f);
        // Child widgets and popups win over canvas shortcuts and gestures.
        bool widgetActive = ImGui::IsAnyItemActive();
        bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        bool available = f.hovered && !popup && !f.blocked && !widgetActive;
        if (hoveredPin && f.hovered && !popup && !widgetActive) {
            if (!s.connecting) { // Name, type and the host's current value or the reason it is unavailable.
                std::string tip(hoveredPin->label);
                for (auto &type : f.graph.socketTypes)
                    if (type.id == hoveredPin->type && !type.name.empty()) {
                        tip += tip.empty() ? "" : "  \xC2\xB7  ";
                        tip += type.name;
                    }
                if (!hoveredPin->inspect.empty())
                    tip.append("\n").append(hoveredPin->inspect);
                if ((hoveredPin->flags & PinFlags_Unavailable) && !hoveredPin->reason.empty())
                    tip.append("\n").append(hoveredPin->reason);
                if (!tip.empty())
                    ImGui::SetTooltip("%s", tip.c_str());
            }
            const bool linked = IsPinConnected(f.graph, hoveredPin->id);
            if (ImGui::IsMouseClicked(0) && !f.options.readOnly && io.KeyAlt) {
                // Alt-click breaks every link of the socket in one operation.
                std::vector<EditRequest> batch;
                const auto op = s.nextOperation++;
                for (auto &link : f.graph.links)
                    if (link.from == hoveredPin->id || link.to == hoveredPin->id) {
                        auto r = Request(f, EditKind::DeleteLink);
                        r.link = link.id;
                        r.operation = op;
                        batch.push_back(r);
                    }
                for (auto &r : batch)
                    r.operationSize = batch.size();
                if (!batch.empty())
                    f.requests->PushBatch(batch);
            } else if (ImGui::IsMouseClicked(0) && !f.options.readOnly && io.KeyCtrl && linked) {
                s.connecting = hoveredPin->id; // Ctrl-drag carries all links to another socket.
                s.reconnecting = {};
                s.movingLinks = true;
            } else if (ImGui::IsMouseClicked(0) && !f.options.readOnly) {
                s.connecting = hoveredPin->id;
                s.reconnecting = {};
                s.movingLinks = false;
                if (hoveredPin->kind == PinKind::Input)
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
            const bool target = hoveredPin && hoveredPin->id != s.connecting;
            const auto verdict =
                target ? DragVerdict(f, hoveredPin->id) : ConnectionVerdict{};
            const bool convertible = verdict.allowed && verdict.match == ConnectionMatch::Convertible;
            if (from) {
                // The draft wire has the final shape: it leaves the socket like a link and lands on the target.
                const auto a = PinPosition(f, *from);
                const auto da = Direction(PinSide(f, *from));
                const auto q = target ? PinPosition(f, *hoveredPin) : io.MousePos;
                const auto dq = target ? Direction(PinSide(f, *hoveredPin)) : Mul(da, -1);
                const auto points = CurvePoints(f.style.linkStyle, a, da, q, dq);
                const ImVec4 color = !verdict.allowed ? f.style.error : convertible ? f.style.warning : PinColor(f, *from);
                if (!target)
                    DashedPolyline(dl, points, Color(color), f.style.linkWidth, 2, 1);
                else if (!verdict.allowed)
                    DashedPolyline(dl, points, Color(color), std::max(1.f, f.style.linkWidth * .75f), 1, 1);
                else if (convertible)
                    DashedPolyline(dl, points, Color(color), f.style.linkWidth, 3, 1);
                else
                    dl->AddPolyline(points.data(), int(points.size()), Color(color), ImDrawFlags_None,
                                    f.style.linkWidth);
                if (target && verdict.allowed) { // The socket that will receive the wire.
                    const float r = f.style.pinRadius * std::clamp(float(s.zoom), .8f, 2.f);
                    dl->AddCircle(q, r + 8, Color(WithAlpha(color, .35f)), 0, 3.f);
                }
            }
            if (target) {
                // The verdict sits next to the cursor, above the nodes, in the host's own words when given.
                const char *label = !verdict.allowed ? Tr(f, "node_editor.connect.rejected", "Rejected")
                                    : convertible   ? Tr(f, "node_editor.connect.convertible", "Convertible")
                                                    : Tr(f, "node_editor.connect.exact", "Exact");
                std::string text = verdict.reason.empty() ? std::string(label) : std::string(verdict.reason);
                auto *fg = ImGui::GetForegroundDrawList();
                fg->PushClipRect(f.min, f.max, true);
                const auto size = ImGui::CalcTextSize(text.c_str());
                const ImVec2 lo{io.MousePos.x + 14, io.MousePos.y + 18};
                const ImVec2 hi{lo.x + size.x + 16, lo.y + size.y + 6};
                const ImVec4 tone = !verdict.allowed ? f.style.error : convertible ? f.style.warning : f.style.success;
                fg->AddRectFilled(lo, hi, Color(f.style.surface), (hi.y - lo.y) / 2);
                fg->AddRect(lo, hi, Color(WithAlpha(tone, .8f)), (hi.y - lo.y) / 2, 0, 1.2f);
                fg->AddText({lo.x + 8, lo.y + 3}, Color(tone), text.c_str());
                fg->PopClipRect();
                if (ImGui::IsMouseReleased(0) && s.movingLinks) {
                    if (verdict.allowed) { // Every link of the source moves, as one operation.
                        std::vector<EditRequest> batch;
                        const auto op = s.nextOperation++;
                        for (auto &link : f.graph.links)
                            if (link.from == s.connecting || link.to == s.connecting) {
                                auto r = Request(f, EditKind::Reconnect);
                                r.link = link.id;
                                r.from = link.from == s.connecting ? hoveredPin->id : link.from;
                                r.to = link.to == s.connecting ? hoveredPin->id : link.to;
                                r.operation = op;
                                batch.push_back(r);
                            }
                        for (auto &r : batch)
                            r.operationSize = batch.size();
                        f.requests->PushBatch(batch);
                    }
                    s.connecting = {};
                    s.reconnecting = {};
                    s.movingLinks = false;
                } else if (ImGui::IsMouseReleased(0)) {
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
            } else if (ImGui::IsMouseReleased(0) && !popup) {
                if (s.movingLinks) { // Moving links onto empty canvas does nothing.
                    s.connecting = {};
                    s.movingLinks = false;
                } else {
                    s.press = World(f, io.MousePos);
                    s.palette = true;
                }
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
                {f.options.keys.group, EditKind::Group},
                {f.options.keys.bypass, EditKind::Bypass},
                {f.options.keys.collapse, EditKind::Collapse}};
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
                    auto points = LinkCurve(f, link, *a, *b);
                    for (std::size_t i = 1; i < points.size(); ++i)
                        if (SegmentDistance(io.MousePos, points[i - 1], points[i]) < 6) {
                            s.selectedLink = link.id;
                            hit = true;
                            if (ImGui::IsMouseDoubleClicked(0) && !f.options.readOnly) {
                                auto r = Request(f, EditKind::InsertReroute);
                                r.link = link.id;
                                r.after = World(f, io.MousePos);
                                f.requests->Push(r);
                            }
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
                        auto points = LinkCurve(f, link, *x, *y);
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
            if (ImGui::MenuItem("Connect selected", nullptr, false, s.selection.size() == 2))
                ConnectSelected(f);
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
                        Tr(f, "node_editor.request_buffer_full",
                           "Request buffer full: drain / enlarge it to finish the operation"));
        }
    }
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopID();
    f.visible = false;
}
} // namespace imkit::node_editor
