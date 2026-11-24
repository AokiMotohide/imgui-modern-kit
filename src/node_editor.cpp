#include "node_editor_internal.h"
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
namespace detail {
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
ImVec4 Mix(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
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
// Screen radius of a reroute knot. Its centre moves the knot; the ring just outside starts a wire.
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
                             const LinkStyle *overrideStyle) {
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
        const ImU32 ink = ImGui::GetColorU32(::imkit::detail::InkOn(st.error));
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
EditRequest Request(EditorFrame &f, EditKind kind, NodeId node) {
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
} // namespace detail
using namespace detail;
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
} // namespace imkit::node_editor
