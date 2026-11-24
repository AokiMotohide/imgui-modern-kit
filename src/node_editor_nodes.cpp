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
using namespace detail;
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
        ::imkit::detail::DrawSoftShadow(dl, a, b, rounding, 10.f * z, .14f);
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
        dl->AddTriangleFilled({c.x - k * .7f, c.y - k}, {c.x + k, c.y}, {c.x - k * .7f, c.y + k}, Color(::imkit::detail::InkOn(category)));
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
            dl->AddText({t0.x + 9 * z, t0.y + (tabHeight - extent.y) * .5f}, Color(::imkit::detail::InkOn(tone)), title.c_str());
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
        const ImVec4 ink = onCategory ? ::imkit::detail::InkOn(category) : st.text;
        float x = a.x + st.padding * z;
        float right = a.x + px.x - st.padding * z;
        const auto glyph = CategoryGlyph(st, *n);
        if (!glyph.empty() && !headerHooked) {
            const float tile = std::max(4.f, std::min(header - 10 * z, 20 * z));
            const ImVec2 t0{x, a.y + (header - tile) / 2}, t1{x + tile, a.y + (header + tile) / 2};
            const ImVec4 fill = onCategory ? WithAlpha(::imkit::detail::InkOn(category), .22f) : category;
            dl->AddRectFilled(t0, t1, Color(fill), std::min(st.rounding, tile * .3f));
            const std::string g(glyph);
            const float glyphSize = tile * .68f;
            const auto extent = ImGui::GetFont()->CalcTextSizeA(glyphSize, FLT_MAX, 0, g.c_str());
            dl->AddText(ImGui::GetFont(), glyphSize, {(t0.x + t1.x - extent.x) / 2, (t0.y + t1.y - extent.y) / 2},
                        Color(onCategory ? ink : ::imkit::detail::InkOn(category)), g.c_str());
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
            const ImU32 inkOn = Color(::imkit::detail::InkOn(tone));
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
} // namespace imkit::node_editor
