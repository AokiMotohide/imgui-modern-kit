#include <imkit/transfer_curve.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace imkit {
namespace {
enum DragKind { kNone = 0, kNode = 1, kIn = 2, kOut = 3 };

float Clamp01(float v) { return std::isfinite(v) ? std::min(std::max(v, 0.f), 1.f) : 0.f; }
float ToDisplay(const TransferCurveSeries& s, float y) {
    return s.toDisplay && s.fromDisplay ? s.toDisplay(y, s.user) : y;
}
float FromDisplay(const TransferCurveSeries& s, float y) {
    return s.toDisplay && s.fromDisplay ? s.fromDisplay(y, s.user) : y;
}

struct Plot {
    ImVec2 min, max;
    ImVec2 Screen(const TransferCurveSeries& s, ImVec2 p) const {
        return {min.x + Clamp01(p.x) * (max.x - min.x), max.y - Clamp01(ToDisplay(s, Clamp01(p.y))) * (max.y - min.y)};
    }
    // Screen point to stored value space.
    ImVec2 Value(const TransferCurveSeries& s, ImVec2 p) const {
        const float dy = (max.y - p.y) / std::max(1.f, max.y - min.y);
        const float x = (p.x - min.x) / std::max(1.f, max.x - min.x);
        return {Clamp01(x), Clamp01(FromDisplay(s, Clamp01(dy)))};
    }
};

ImVec2 Bezier(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float t) {
    const float u = 1.f - t;
    const float w0 = u * u * u, w1 = 3.f * u * u * t, w2 = 3.f * u * t * t, w3 = t * t * t;
    return {w0 * a.x + w1 * b.x + w2 * c.x + w3 * d.x, w0 * a.y + w1 * b.y + w2 * c.y + w3 * d.y};
}

constexpr int kSamples = 24;
// Fills out with the screen polyline of the whole series.
int Polyline(const TransferCurveSeries& s, const Plot& plot, std::vector<ImVec2>& out) {
    out.clear();
    const auto n = s.nodes;
    if (n.size() < 2) return 0;
    for (size_t i = 0; i + 1 < n.size(); ++i)
        for (int k = (i == 0 ? 0 : 1); k <= kSamples; ++k)
            out.push_back(plot.Screen(s, Bezier(n[i].pos, n[i].out, n[i + 1].in, n[i + 1].pos, float(k) / kSamples)));
    return static_cast<int>(out.size());
}

void DrawLine(ImDrawList* draw, const std::vector<ImVec2>& pts, ImU32 color, float thickness, bool dashed) {
    if (!dashed) {
        draw->AddPolyline(pts.data(), static_cast<int>(pts.size()), color, 0, thickness);
        return;
    }
    const float dash = 7.f, gap = 5.f;
    float carry = 0.f;
    bool on = true;
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        ImVec2 a = pts[i];
        const ImVec2 b = pts[i + 1];
        float len = std::hypot(b.x - a.x, b.y - a.y);
        while (len > 1e-3f) {
            const float run = (on ? dash : gap) - carry;
            const float step = std::min(run, len);
            const ImVec2 e{a.x + (b.x - a.x) * step / len, a.y + (b.y - a.y) * step / len};
            if (on) draw->AddLine(a, e, color, thickness);
            a = e;
            len -= step;
            carry += step;
            if (carry >= (on ? dash : gap) - 1e-4f) {
                carry = 0.f;
                on = !on;
            }
        }
    }
}

float Dist2(ImVec2 a, ImVec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); }
float SegDist2(ImVec2 p, ImVec2 a, ImVec2 b) {
    const float dx = b.x - a.x, dy = b.y - a.y, l2 = dx * dx + dy * dy;
    const float t = l2 > 0.f ? std::min(std::max(((p.x - a.x) * dx + (p.y - a.y) * dy) / l2, 0.f), 1.f) : 0.f;
    return Dist2(p, {a.x + dx * t, a.y + dy * t});
}

// Neighbour limits for x of knot i.
void XLimits(std::span<TransferCurveNode> n, int i, float& lo, float& hi) {
    lo = i > 0 ? n[i - 1].pos.x : 0.f;
    hi = i + 1 < static_cast<int>(n.size()) ? n[i + 1].pos.x : 1.f;
}

// Moves a knot with its handles. The first and last knot keep their x.
void MoveNode(std::span<TransferCurveNode> n, int i, ImVec2 target) {
    const int last = static_cast<int>(n.size()) - 1;
    float lo, hi;
    XLimits(n, i, lo, hi);
    const float x = (i == 0 || i == last) ? n[i].pos.x : std::min(std::max(target.x, lo), std::max(lo, hi));
    const float y = Clamp01(target.y);
    const ImVec2 delta{x - n[i].pos.x, y - n[i].pos.y};
    n[i].pos = {x, y};
    n[i].in = {std::min(std::max(n[i].in.x + delta.x, std::min(lo, x)), x), Clamp01(n[i].in.y + delta.y)};
    n[i].out = {std::min(std::max(n[i].out.x + delta.x, x), std::max(x, hi)), Clamp01(n[i].out.y + delta.y)};
}

void MoveHandle(std::span<TransferCurveNode> n, int i, bool out, ImVec2 target) {
    float lo, hi;
    XLimits(n, i, lo, hi);
    const float px = n[i].pos.x;
    if (out) n[i].out = {std::min(std::max(target.x, px), std::max(px, hi)), Clamp01(target.y)};
    else n[i].in = {std::min(std::max(target.x, std::min(lo, px)), px), Clamp01(target.y)};
}

bool Differs(const std::vector<TransferCurveNode>& a, std::span<const TransferCurveNode> b) {
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].pos.x != b[i].pos.x || a[i].pos.y != b[i].pos.y || a[i].in.x != b[i].in.x ||
            a[i].in.y != b[i].in.y || a[i].out.x != b[i].out.x || a[i].out.y != b[i].out.y)
            return true;
    return false;
}
} // namespace

bool TransferCurveClampMonotone(std::span<TransferCurveNode> nodes) {
    bool changed = false;
    const auto set = [&](float& v, float to) {
        if (v != to) { v = to; changed = true; }
    };
    for (size_t i = 0; i < nodes.size(); ++i) {
        set(nodes[i].pos.y, Clamp01(nodes[i].pos.y));
        if (i > 0) set(nodes[i].pos.y, std::max(nodes[i].pos.y, nodes[i - 1].pos.y));
    }
    for (size_t i = 0; i + 1 < nodes.size(); ++i) {
        const float x0 = nodes[i].pos.x, x1 = std::max(x0, nodes[i + 1].pos.x);
        const float y0 = nodes[i].pos.y, y1 = nodes[i + 1].pos.y;
        set(nodes[i].out.x, std::min(std::max(nodes[i].out.x, x0), x1));
        set(nodes[i].out.y, std::min(std::max(nodes[i].out.y, y0), y1));
        set(nodes[i + 1].in.x, std::min(std::max(nodes[i + 1].in.x, x0), x1));
        set(nodes[i + 1].in.y, std::min(std::max(nodes[i + 1].in.y, y0), y1));
    }
    return changed;
}

bool TransferCurveEditor(const char* id, std::span<TransferCurveSeries> series, TransferCurveState& state,
                         TransferCurveOptions layout, TransferCurveInteraction* interaction, ComponentOptions options) {
    (void)options;
    TransferCurveInteraction result;
    if (interaction) *interaction = {};
    const int seriesCount = static_cast<int>(series.size());
    if (seriesCount == 0) return false;
    if (state.series < 0 || state.series >= seriesCount) {
        state.series = 0;
        result.seriesChanged = true;
    }

    ImVec2 size = layout.size;
    if (size.x <= 0.f) size.x = std::max(120.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0.f) size.y = size.x;
    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##curve", size, ImGuiButtonFlags_MouseButtonLeft);
    const bool active = ImGui::IsItemActive();
    const bool activated = ImGui::IsItemActivated();
    const bool focused = ImGui::IsItemFocused();
    ImGuiStorage* store = ImGui::GetStateStorage();
    const ImGuiID kKind = ImGui::GetID("kind"), kOffX = ImGui::GetID("ox"), kOffY = ImGui::GetID("oy"),
                  kDirty = ImGui::GetID("dirty");

    const float pad = 10.f;
    const Plot plot{{origin.x + pad, origin.y + pad}, {origin.x + size.x - pad, origin.y + size.y - pad}};
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float radius = std::max(2.f, layout.pickRadius);
    bool changed = false;

    const auto selected = [&]() -> TransferCurveSeries& { return series[state.series]; };
    const auto setNode = [&](int node) {
        if (state.node != node) {
            state.node = node;
            result.nodeChanged = true;
        }
    };
    if (state.node >= static_cast<int>(selected().nodes.size())) setNode(-1);

    // Press: pick a handle, a knot, or another series' curve.
    if (activated) {
        const auto nodes = selected().nodes;
        const int n = static_cast<int>(nodes.size());
        int kind = kNone, index = -1;
        float best = radius * radius;
        const auto consider = [&](ImVec2 p, int k, int i) {
            const float d = Dist2(p, mouse);
            if (d <= best) {
                best = d;
                kind = k;
                index = i;
            }
        };
        if (state.node >= 0) {
            if (state.node > 0) consider(plot.Screen(selected(), nodes[state.node].in), kIn, state.node);
            if (state.node + 1 < n) consider(plot.Screen(selected(), nodes[state.node].out), kOut, state.node);
        }
        if (kind == kNone)
            for (int i = 0; i < n; ++i) consider(plot.Screen(selected(), nodes[i].pos), kNode, i);
        if (kind != kNone) {
            if (kind == kNode) setNode(index);
            const ImVec2 target = kind == kNode ? nodes[index].pos : (kind == kIn ? nodes[index].in : nodes[index].out);
            const ImVec2 at = plot.Screen(selected(), target);
            store->SetInt(kKind, kind);
            store->SetFloat(kOffX, at.x - mouse.x);
            store->SetFloat(kOffY, at.y - mouse.y);
            store->SetInt(kDirty, 0);
            result.activated = true;
        } else {
            int hit = -1;
            float bestCurve = radius * radius;
            std::vector<ImVec2> pts;
            for (int s = 0; s < seriesCount; ++s) {
                if (s == state.series || Polyline(series[s], plot, pts) < 2) continue;
                for (size_t k = 0; k + 1 < pts.size(); ++k) {
                    const float d = SegDist2(mouse, pts[k], pts[k + 1]);
                    if (d <= bestCurve) {
                        bestCurve = d;
                        hit = s;
                    }
                }
            }
            if (hit >= 0) {
                state.series = hit;
                result.seriesChanged = true;
                setNode(-1);
            }
        }
    }

    // Drag. Esc ends the drag and reports canceled; the host restores its own snapshot.
    const int kind = store->GetInt(kKind, kNone);
    if (kind != kNone && active) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            store->SetInt(kKind, kNone);
            store->SetInt(kDirty, 0);
            result.canceled = true;
        } else if (state.node >= 0) {
            auto& s = selected();
            const std::vector<TransferCurveNode> before(s.nodes.begin(), s.nodes.end());
            const ImVec2 p = plot.Value(s, {mouse.x + store->GetFloat(kOffX, 0.f), mouse.y + store->GetFloat(kOffY, 0.f)});
            if (kind == kNode) MoveNode(s.nodes, state.node, p);
            else MoveHandle(s.nodes, state.node, kind == kOut, p);
            if (layout.monotone) TransferCurveClampMonotone(s.nodes);
            if (Differs(before, s.nodes)) {
                changed = true;
                store->SetInt(kDirty, 1);
            }
        }
    }
    if (!active && kind != kNone) {
        if (store->GetInt(kDirty, 0)) result.committed = true;
        store->SetInt(kKind, kNone);
        store->SetInt(kDirty, 0);
    }

    // Keyboard: only with focus and not while dragging.
    if (focused && !active) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && state.node >= 0) {
            setNode(-1);
        } else if (state.node >= 0) {
            const float step = ImGui::GetIO().KeyShift ? 0.04f : 0.01f;
            ImVec2 d{0.f, 0.f};
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) d.x -= step;
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) d.x += step;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) d.y -= step;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) d.y += step;
            if (d.x != 0.f || d.y != 0.f) {
                auto& s = selected();
                const std::vector<TransferCurveNode> before(s.nodes.begin(), s.nodes.end());
                const ImVec2 pos = s.nodes[state.node].pos;
                MoveNode(s.nodes, state.node, {pos.x + d.x, pos.y + d.y});
                if (layout.monotone) TransferCurveClampMonotone(s.nodes);
                if (Differs(before, s.nodes)) {
                    changed = true;
                    result.committed = true;
                }
            }
        }
    }

    // Draw.
    auto* draw = ImGui::GetWindowDrawList();
    const auto& style = ImGui::GetStyle();
    const ImVec2 corner{origin.x + size.x, origin.y + size.y};
    draw->AddRectFilled(origin, corner, ImGui::GetColorU32(ImGuiCol_FrameBg), style.FrameRounding);
    for (int g = 0; layout.gridDivisions > 0 && g <= layout.gridDivisions; ++g) {
        const float t = float(g) / layout.gridDivisions;
        const bool edge = g == 0 || g == layout.gridDivisions;
        const ImU32 c = ImGui::GetColorU32(ImGuiCol_Separator, edge ? 1.f : 0.45f);
        const float x = plot.min.x + t * (plot.max.x - plot.min.x), y = plot.min.y + t * (plot.max.y - plot.min.y);
        draw->AddLine({x, plot.min.y}, {x, plot.max.y}, c);
        draw->AddLine({plot.min.x, y}, {plot.max.x, y}, c);
    }
    draw->AddRect(origin, corner, ImGui::GetColorU32(focused ? ImGuiCol_NavCursor : ImGuiCol_Border), style.FrameRounding);
    std::vector<ImVec2> pts;
    for (int pass = 0; pass < 2; ++pass)
        for (int s = 0; s < seriesCount; ++s) {
            const bool isSel = s == state.series;
            if (isSel != (pass == 1) || Polyline(series[s], plot, pts) < 2) continue;
            DrawLine(draw, pts, series[s].color, isSel ? 2.5f : 1.5f, series[s].dashed);
        }
    {
        const auto& s = selected();
        const int n = static_cast<int>(s.nodes.size());
        if (state.node >= 0 && state.node < n) {
            const ImVec2 p = plot.Screen(s, s.nodes[state.node].pos);
            const auto handle = [&](ImVec2 value) {
                const ImVec2 h = plot.Screen(s, value);
                draw->AddLine(p, h, ImGui::GetColorU32(ImGuiCol_TextDisabled), 1.f);
                draw->AddCircleFilled(h, 4.f, ImGui::GetColorU32(ImGuiCol_Text));
                draw->AddCircle(h, 4.f, s.color, 0, 1.5f);
            };
            if (state.node > 0) handle(s.nodes[state.node].in);
            if (state.node + 1 < n) handle(s.nodes[state.node].out);
        }
        for (int i = 0; i < n; ++i) {
            const ImVec2 p = plot.Screen(s, s.nodes[i].pos);
            const bool on = i == state.node;
            draw->AddCircleFilled(p, on ? 6.f : 4.5f, on ? s.color : ImGui::GetColorU32(ImGuiCol_WindowBg));
            draw->AddCircle(p, on ? 6.f : 4.5f, s.color, 0, 2.f);
        }
    }
    ImGui::PopID();

    result.changed = changed;
    if (interaction) *interaction = result;
    return changed || result.seriesChanged || result.nodeChanged;
}
bool TransferCurveSectionEditor(const char* id, TransferCurveSection& section,
                                std::span<TransferCurveSeries> crossSeries,
                                std::span<TransferCurveSeries> longitudinalSeries, TransferCurveState& state,
                                TransferCurveOptions layout, TransferCurveInteraction* interaction,
                                bool* sectionChanged, ComponentOptions options) {
    ImGui::PushID(id);
    static const char* const kLabels[] = {"横断面", "縦断面"};
    int selected = static_cast<int>(section);
    const bool switched = Segmented("##section", &selected, kLabels, options);
    if (switched) section = selected == 1 ? TransferCurveSection::Longitudinal : TransferCurveSection::Cross;
    if (sectionChanged) *sectionChanged = switched;
    const bool edited = TransferCurveEditor(
        "##curve", section == TransferCurveSection::Longitudinal ? longitudinalSeries : crossSeries, state, layout,
        interaction, options);
    ImGui::PopID();
    return switched || edited;
}
} // namespace imkit
