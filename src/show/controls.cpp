#include <imkit/show/controls.h>
#include "internal.h"
#include <cfloat>

namespace imkit::show {
using namespace impl;

namespace {
constexpr float Pi = 3.14159265358979323846f;

float Finite(float v, float fallback) { return std::isfinite(v) ? v : fallback; }
bool Visible(const char* label) { return label && label[0] && !(label[0] == '#' && label[1] == '#'); }

// Binding indicator in a control's top-right corner: a dot, ringed while the host waits for Learn.
void DrawBindingDot(ImDrawList* draw, const ShowColors& c, const BindingInfo* b, ImVec2 at, float radius, const ComponentOptions& o) {
    if (!b || b->kind == BindingKind::None)
        return;
    const ImVec4 col = b->learning ? c.warning : c.accent;
    draw->AddCircleFilled(at, radius, U32(col), 12);
    if (b->learning) {
        const float pulse = MotionAllowed(o) ? .5f + .5f * std::sin(static_cast<float>(ImGui::GetTime()) * 6.f) : 1.f;
        draw->AddCircle(at, radius * (1.6f + .6f * pulse), U32(col, .8f - .4f * pulse), 16, 1.5f);
    }
}
float Step(float step, float minimum, float maximum) { return step > 0 ? step : (maximum - minimum) / 100.f; }
} // namespace

// ---- Fader ----------------------------------------------------------------------------------------------------
float FaderPosition(float value, float minimum, float maximum, FaderScale scale) {
    if (!(maximum > minimum) || !std::isfinite(value))
        return 0;
    const float t = std::clamp((value - minimum) / (maximum - minimum), 0.f, 1.f);
    return scale == FaderScale::Decibel ? t * t * t : t;
}
float FaderValue(float position, float minimum, float maximum, FaderScale scale) {
    if (!(maximum > minimum))
        return minimum;
    const float p = Clamp01(position);
    const float t = scale == FaderScale::Decibel ? std::cbrt(p) : p;
    return minimum + t * (maximum - minimum);
}

ValueEdit Fader(const char* label, float* value, FaderOptions o, ComponentOptions options) {
    if (!value)
        return {};
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float mn = o.minimum, mx = o.maximum > o.minimum ? o.maximum : o.minimum + 1;
    *value = std::clamp(Finite(*value, mn), mn, mx);

    const bool vertical = o.vertical;
    const float labelW = o.tickFormat && vertical ? u * 1.05f : 0.f;
    const float gutter = vertical ? labelW + (o.ticks.empty() ? 0.f : u * .55f) : 0.f;
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = vertical ? gutter + u * 1.8f : std::max(u * 6.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = vertical ? u * 7.5f : u * 1.8f;
    const bool named = Visible(label);
    const float labelH = named && vertical ? u * .8f : 0.f;
    const float readH = o.showValue && vertical ? u * .9f : 0.f;
    const float readW = o.showValue && !vertical ? u * 3.4f : 0.f;

    ImGui::PushID(label);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##fader", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();
    const ImGuiID itemId = ImGui::GetItemID();

    // Track geometry: `start` is where the minimum sits, `end` the maximum (cap centres).
    const float capA = u * .7f;
    const float capB = vertical ? std::min(size.x - gutter - 2.f, u * 1.15f) : std::min(size.y - 2.f, u * 1.15f);
    ImVec2 start, end;
    if (vertical) {
        const float railX = p0.x + gutter + (size.x - gutter) * .5f;
        start = {railX, p0.y + size.y - labelH - capA * .5f};
        end = {railX, p0.y + readH + capA * .5f};
    } else {
        const float railY = p0.y + size.y * .5f;
        start = {p0.x + capA * .5f, railY};
        end = {p0.x + size.x - readW - capA * .5f, railY};
    }
    const float length = std::max(1.f, std::hypot(end.x - start.x, end.y - start.y));
    const ImVec2 dir{(end.x - start.x) / length, (end.y - start.y) / length};
    auto project = [&](ImVec2 m) { return ((m.x - start.x) * dir.x + (m.y - start.y) * dir.y) / length; };
    auto at = [&](float t) { return ImVec2{start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t}; };

    const bool openEntry = ImGui::IsItemActivated() && io.KeyCtrl;
    EditFrame frame(value, openEntry);
    ImGuiStorage* st = frame.Storage();
    const ImGuiID anchorMouseKey = ImGui::GetID("##anchor.mouse"), anchorPosKey = ImGui::GetID("##anchor.pos"),
                  fineKey = ImGui::GetID("##anchor.fine");
    float pos = FaderPosition(*value, mn, mx, o.scale);
    const auto apply = [&](float newPos) {
        *value = SnapValue(FaderValue(newPos, mn, mx, o.scale), mn, mx, o.step);
    };
    if (ImGui::IsItemActivated()) {
        const float t = project(io.MousePos);
        const bool onCap = std::abs(t - pos) * length <= capA * .6f;
        if (!onCap) {
            apply(t);
            pos = FaderPosition(*value, mn, mx, o.scale);
        }
        st->SetFloat(anchorMouseKey, t);
        st->SetFloat(anchorPosKey, onCap ? pos : t);
        st->SetBool(fineKey, io.KeyShift);
    } else if (frame.Active()) {
        const float t = project(io.MousePos);
        if (io.KeyShift != st->GetBool(fineKey)) {
            st->SetFloat(anchorMouseKey, t);
            st->SetFloat(anchorPosKey, pos);
            st->SetBool(fineKey, io.KeyShift);
        }
        apply(st->GetFloat(anchorPosKey) + (t - st->GetFloat(anchorMouseKey)) * (io.KeyShift ? .1f : 1.f));
    }
    frame.CancelOnEscape();

    // Discrete inputs: double-click reset, Ctrl+wheel, +/- keys, accessibility steps, typed entry.
    const float stepSize = Step(o.step, mn, mx);
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !io.KeyCtrl) {
        frame.Instant(std::clamp(o.defaultValue, mn, mx));
        frame.Suppress();
    }
    if (hovered && io.KeyCtrl && io.MouseWheel != 0)
        frame.Instant(SnapValue(*value + io.MouseWheel * stepSize * (io.KeyShift ? .1f : 1.f), mn, mx, o.step));
    if (focused) {
        const float big = io.KeyShift ? 10.f : 1.f;
        if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
            frame.Instant(SnapValue(*value + stepSize * big, mn, mx, o.step));
        if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
            frame.Instant(SnapValue(*value - stepSize * big, mn, mx, o.step));
    }
    float typed = *value;
    if (NumericEntry("##entry", openEntry, &typed, mn, mx, o.format))
        frame.Instant(SnapValue(typed, mn, mx, o.step));

    pos = FaderPosition(*value, mn, mx, o.scale);
    const bool active = frame.Active();
    const float emph = Ease(options, itemId, active ? 1.f : hovered ? .6f : 0.f);

    // Ticks and tick labels along the rail.
    const float railW = std::max(3.f, u * .14f);
    for (float tick : o.ticks) {
        const ImVec2 q = at(FaderPosition(tick, mn, mx, o.scale));
        const bool isDefault = std::abs(tick - o.defaultValue) < (mx - mn) * 1e-4f;
        const float len = u * (isDefault ? .3f : .2f);
        const float off = capB * .5f + 3.f;
        // Ticks sit left of a vertical rail and above a horizontal one.
        const ImVec2 start{vertical ? q.x - off : q.x, vertical ? q.y : q.y - off};
        const ImVec2 stop{vertical ? start.x - len : start.x, vertical ? start.y : start.y - len};
        draw->AddLine(start, stop, U32(isDefault ? c.text : c.muted, isDefault ? .8f : .55f), 1.f);
        if (o.tickFormat && vertical) {
            char text[24];
            std::snprintf(text, sizeof(text), SafeFormat(o.tickFormat, "fge", "%.0f"), tick);
            const ImVec2 ts = ImGui::CalcTextSize(text);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .8f, {stop.x - ts.x * .8f - 3.f, stop.y - ts.y * .4f}, U32(c.muted), text);
        }
    }

    // Rail and fill.
    const float zero = o.bipolar ? FaderPosition((mn + mx) * .5f, mn, mx, o.scale) : 0.f;
    const ImVec2 r0 = at(std::min(zero, pos)), r1 = at(std::max(zero, pos));
    const ImVec2 rs = at(0.f), re = at(1.f);
    const float half = railW * .5f;
    auto rail = [&](ImVec2 a, ImVec2 b, ImU32 col) {
        draw->AddRectFilled({std::min(a.x, b.x) - half, std::min(a.y, b.y) - half}, {std::max(a.x, b.x) + half, std::max(a.y, b.y) + half},
                            col, half);
    };
    rail(rs, re, U32(c.border, .85f));
    rail(r0, r1, U32(Mix(c.accent, c.text, .12f * emph), .85f));

    // Cap.
    const ImVec2 cp = at(pos);
    const ImVec2 ch = vertical ? ImVec2{capB * .5f, capA * .5f} : ImVec2{capA * .5f, capB * .5f};
    const ImVec2 cmin{cp.x - ch.x, cp.y - ch.y}, cmax{cp.x + ch.x, cp.y + ch.y};
    if (options.theme)
        draw->AddRectFilled({cmin.x, cmin.y + 2}, {cmax.x, cmax.y + 2}, U32({0, 0, 0, .28f}), u * .18f);
    draw->AddRectFilled(cmin, cmax, U32(Mix(c.raised, c.text, .08f + .08f * emph)), u * .18f);
    draw->AddRect(cmin, cmax, U32(active ? c.accent : c.border, active ? 1.f : .9f), u * .18f, 0, active ? 2.f : 1.f);
    if (vertical)
        draw->AddLine({cmin.x + u * .18f, cp.y}, {cmax.x - u * .18f, cp.y}, U32(Mix(c.muted, c.accent, emph)), 2.f);
    else
        draw->AddLine({cp.x, cmin.y + u * .18f}, {cp.x, cmax.y - u * .18f}, U32(Mix(c.muted, c.accent, emph)), 2.f);
    if (focused && io.NavVisible)
        draw->AddRect({cmin.x - 3, cmin.y - 3}, {cmax.x + 3, cmax.y + 3}, U32(c.focus), u * .26f, 0, 2.f);
    DrawBindingDot(draw, c, o.binding, {p0.x + size.x - 4.f, p0.y + 4.f}, std::max(2.5f, u * .09f), options);

    // Readout and name.
    if (o.showValue) {
        const char* text = FormatValue(o.format, o.unit, *value);
        const ImVec2 ts = ImGui::CalcTextSize(text);
        const ImVec2 tp = vertical ? ImVec2{p0.x + (size.x - ts.x) * .5f, p0.y + (readH - ts.y) * .5f}
                                   : ImVec2{p0.x + size.x - readW + u * .3f, p0.y + (size.y - ts.y) * .5f};
        draw->AddText(tp, U32(hovered || active ? c.text : c.muted), text);
    }
    if (named && vertical) {
        const char* shown = label;
        const ImVec2 ts = ImGui::CalcTextSize(shown, nullptr, true);
        DrawTextFit(draw, {p0.x + std::max(0.f, (size.x - ts.x) * .5f), p0.y + size.y - labelH + (labelH - ts.y) * .5f},
                    size.x, U32(c.muted), shown);
    }

    // Accessibility.
    detail::ValueNode node;
    node.name = Visible(label) ? label : "Fader";
    node.value = *value;
    node.minimum = mn;
    node.maximum = mx;
    node.smallChange = stepSize;
    node.largeChange = stepSize * 10;
    node.text = FormatValue(o.format, o.unit, *value);
    node.disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    if (const int direction = detail::AnnotateValue(options, node); direction != 0)
        frame.Instant(SnapValue(*value + static_cast<float>(direction) * stepSize, mn, mx, o.step));
    ImGui::PopID();
    return frame.Done();
}

// ---- Knob -----------------------------------------------------------------------------------------------------
ValueEdit Knob(const char* label, float* value, KnobOptions o, ComponentOptions options) {
    if (!value)
        return {};
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float mn = o.minimum, mx = o.maximum > o.minimum ? o.maximum : o.minimum + 1;
    *value = std::clamp(Finite(*value, mn), mn, mx);
    const float d = o.diameter > 0 ? o.diameter : u * 1.9f;
    const bool named = o.showLabel && Visible(label);
    const float lineH = ImGui::GetTextLineHeight();
    const float below = (o.showValue ? lineH : 0.f) + (named ? lineH : 0.f);
    const float width = std::max(d, named ? ImGui::CalcTextSize(label, nullptr, true).x : d);

    ImGui::PushID(label);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##knob", {width, d + below + 2.f});
    const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
    const ImGuiID itemId = ImGui::GetItemID();
    const bool openEntry = ImGui::IsItemActivated() && io.KeyCtrl;
    EditFrame frame(value, openEntry);
    ImGuiStorage* st = frame.Storage();
    const ImGuiID anchorYKey = ImGui::GetID("##anchor.y"), anchorTKey = ImGui::GetID("##anchor.t"), fineKey = ImGui::GetID("##anchor.fine");
    const auto norm = [&](float v) { return (v - mn) / (mx - mn); };
    if (ImGui::IsItemActivated()) {
        st->SetFloat(anchorYKey, io.MousePos.y);
        st->SetFloat(anchorTKey, norm(*value));
        st->SetBool(fineKey, io.KeyShift);
    } else if (frame.Active()) {
        if (io.KeyShift != st->GetBool(fineKey)) {
            st->SetFloat(anchorYKey, io.MousePos.y);
            st->SetFloat(anchorTKey, norm(*value));
            st->SetBool(fineKey, io.KeyShift);
        }
        const float dragLength = u * 7.f * (io.KeyShift ? 10.f : 1.f);
        const float t = st->GetFloat(anchorTKey) + (st->GetFloat(anchorYKey) - io.MousePos.y) / dragLength;
        *value = SnapValue(mn + std::clamp(t, 0.f, 1.f) * (mx - mn), mn, mx, o.step);
    }
    frame.CancelOnEscape();
    const float stepSize = Step(o.step, mn, mx);
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !io.KeyCtrl) {
        frame.Instant(std::clamp(o.defaultValue, mn, mx));
        frame.Suppress();
    }
    if (hovered && io.KeyCtrl && io.MouseWheel != 0)
        frame.Instant(SnapValue(*value + io.MouseWheel * stepSize * (io.KeyShift ? .1f : 1.f), mn, mx, o.step));
    if (focused) {
        const float big = io.KeyShift ? 10.f : 1.f;
        if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
            frame.Instant(SnapValue(*value + stepSize * big, mn, mx, o.step));
        if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
            frame.Instant(SnapValue(*value - stepSize * big, mn, mx, o.step));
    }
    float typed = *value;
    if (NumericEntry("##entry", openEntry, &typed, mn, mx, o.format))
        frame.Instant(SnapValue(typed, mn, mx, o.step));

    const bool active = frame.Active();
    const float emph = Ease(options, itemId, active ? 1.f : hovered ? .6f : 0.f);
    const ImVec2 centre{p0.x + width * .5f, p0.y + d * .5f + 1.f};
    const float r = d * .5f;
    const float a0 = Pi * .75f, a1 = Pi * 2.25f;
    const float t = norm(*value), tMid = o.bipolar ? .5f : 0.f;
    const float arcR = r * .88f, arcW = std::max(3.f, r * .16f);
    detail::DrawRingArc(draw, centre, arcR, arcW, a0, a1, U32(c.border, .85f));
    const float va = a0 + (a1 - a0) * t, vm = a0 + (a1 - a0) * tMid;
    detail::DrawRingArc(draw, centre, arcR, arcW, std::min(va, vm), std::max(va, vm) + (o.bipolar ? 0.f : 0.f),
                        U32(Mix(c.accent, c.text, .15f * emph)));
    if (!o.bipolar && t <= 0.001f)
        draw->AddCircleFilled({centre.x + std::cos(a0) * arcR, centre.y + std::sin(a0) * arcR}, arcW * .5f, U32(c.border), 12);
    draw->AddCircleFilled(centre, r * .66f, U32(Mix(c.raised, c.text, .06f + .06f * emph)), 40);
    draw->AddCircle(centre, r * .66f, U32(active ? c.accent : c.border), 40, active ? 2.f : 1.f);
    const ImVec2 tip{centre.x + std::cos(va) * r * .56f, centre.y + std::sin(va) * r * .56f};
    const ImVec2 tail{centre.x + std::cos(va) * r * .2f, centre.y + std::sin(va) * r * .2f};
    draw->AddLine(tail, tip, U32(active ? c.accent : c.text), std::max(2.f, r * .1f));
    if (focused && io.NavVisible)
        draw->AddCircle(centre, r + 3.f, U32(c.focus), 40, 2.f);
    DrawBindingDot(draw, c, o.binding, {p0.x + width - 3.f, p0.y + 4.f}, std::max(2.5f, u * .09f), options);

    float y = p0.y + d + 3.f;
    if (o.showValue) {
        const char* text = FormatValue(o.format, o.unit, *value);
        const ImVec2 ts = ImGui::CalcTextSize(text);
        draw->AddText({p0.x + (width - ts.x) * .5f, y}, U32(hovered || active ? c.text : c.muted), text);
        y += lineH;
    }
    if (named) {
        const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
        draw->AddText({p0.x + (width - ts.x) * .5f, y}, U32(c.muted), label);
    }
    detail::ValueNode node;
    node.name = Visible(label) ? label : "Knob";
    node.value = *value;
    node.minimum = mn;
    node.maximum = mx;
    node.smallChange = stepSize;
    node.largeChange = stepSize * 10;
    node.text = FormatValue(o.format, o.unit, *value);
    node.disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    if (const int direction = detail::AnnotateValue(options, node); direction != 0)
        frame.Instant(SnapValue(*value + static_cast<float>(direction) * stepSize, mn, mx, o.step));
    ImGui::PopID();
    return frame.Done();
}

// ---- XY pad ---------------------------------------------------------------------------------------------------
ValueEdit XYPad(const char* label, float* x, float* y, XYPadOptions o, ComponentOptions options) {
    if (!x || !y)
        return {};
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float mnx = o.minimumX, mxx = o.maximumX > o.minimumX ? o.maximumX : o.minimumX + 1;
    const float mny = o.minimumY, mxy = o.maximumY > o.minimumY ? o.maximumY : o.minimumY + 1;
    *x = std::clamp(Finite(*x, mnx), mnx, mxx);
    *y = std::clamp(Finite(*y, mny), mny, mxy);
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = u * 8.f;
    if (size.y <= 0)
        size.y = size.x;

    ImGui::PushID(label);
    const ImVec2 p0 = ImGui::GetCursorScreenPos(), p1{p0.x + size.x, p0.y + size.y};
    ImGui::InvisibleButton("##xy", size);
    const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID kx = ImGui::GetID("##orig.x"), ky = ImGui::GetID("##orig.y"), ks = ImGui::GetID("##suppressed");
    ValueEdit edit;
    const float beforeX = *x, beforeY = *y;
    bool active = ImGui::IsItemActive();
    if (ImGui::IsItemActivated()) {
        st->SetFloat(kx, *x);
        st->SetFloat(ky, *y);
        st->SetBool(ks, false);
        edit.started = true;
    }
    edit.original = st->GetFloat(kx, *x);
    const bool deactivated = ImGui::IsItemDeactivated();
    if (active && st->GetBool(ks))
        active = false;
    if (active) {
        const float tx = Clamp01((io.MousePos.x - p0.x) / size.x), ty = Clamp01(1.f - (io.MousePos.y - p0.y) / size.y);
        *x = mnx + tx * (mxx - mnx);
        *y = mny + ty * (mxy - mny);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            *x = st->GetFloat(kx, *x);
            *y = st->GetFloat(ky, *y);
            edit.cancelled = true;
            st->SetBool(ks, true);
            active = false;
        }
    }
    bool instant = false;
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        *x = std::clamp(o.defaultX, mnx, mxx);
        *y = std::clamp(o.defaultY, mny, mxy);
        edit.started = edit.changed = edit.finished = true;
        edit.original = beforeX;
        instant = true;
        st->SetBool(ks, true);
        active = false;
    }
    if (!instant && (*x != beforeX || *y != beforeY))
        edit.changed = true;
    if (!instant && deactivated) {
        if (st->GetBool(ks))
            st->SetBool(ks, false);
        else
            edit.finished = true;
    }
    edit.value = *x;

    const float round = u * .3f;
    draw->AddRectFilled(p0, p1, U32(c.input), round);
    draw->PushClipRect(p0, p1, true);
    for (int i = 1; i < o.divisions; ++i) {
        const float f = static_cast<float>(i) / static_cast<float>(o.divisions);
        const bool centre = o.divisions % 2 == 0 && i == o.divisions / 2;
        draw->AddLine({p0.x + size.x * f, p0.y}, {p0.x + size.x * f, p1.y}, U32(c.border, centre ? .9f : .45f));
        draw->AddLine({p0.x, p0.y + size.y * f}, {p1.x, p0.y + size.y * f}, U32(c.border, centre ? .9f : .45f));
    }
    const float px = p0.x + (*x - mnx) / (mxx - mnx) * size.x, py = p1.y - (*y - mny) / (mxy - mny) * size.y;
    const float emph = Ease(options, ImGui::GetItemID(), ImGui::IsItemActive() ? 1.f : hovered ? .6f : 0.f);
    draw->AddLine({px, p0.y}, {px, p1.y}, U32(c.accent, .35f + .25f * emph));
    draw->AddLine({p0.x, py}, {p1.x, py}, U32(c.accent, .35f + .25f * emph));
    draw->PopClipRect();
    const float hr = u * (.3f + .06f * emph);
    draw->AddCircleFilled({px, py}, hr, U32(c.accent), 20);
    draw->AddCircle({px, py}, hr + 2.f, U32(c.text, .9f), 20, 1.5f);
    draw->AddRect(p0, p1, U32(active ? c.accent : c.border), round, 0, active ? 2.f : 1.f);
    if (focused && io.NavVisible)
        draw->AddRect({p0.x - 3, p0.y - 3}, {p1.x + 3, p1.y + 3}, U32(c.focus), round + 3.f, 0, 2.f);
    char text[64];
    std::snprintf(text, sizeof(text), "%s  %s", FormatValue(o.format, "", *x), FormatValue(o.format, "", *y));
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {p0.x + 6.f, p1.y - ImGui::GetTextLineHeight() * .85f - 4.f},
                  U32(c.muted), text);

    detail::ValueNode node;
    node.name = Visible(label) ? label : "XY pad";
    node.value = *x;
    node.minimum = mnx;
    node.maximum = mxx;
    node.smallChange = (mxx - mnx) / 100.f;
    node.largeChange = (mxx - mnx) / 10.f;
    node.description = "X axis";
    node.text = FrameText("X %s, Y %s", FormatValue(o.format, "", *x), FormatValue(o.format, "", *y));
    node.disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    if (const int direction = detail::AnnotateValue(options, node); direction != 0) {
        const float next = std::clamp(*x + static_cast<float>(direction) * static_cast<float>(node.smallChange), mnx, mxx);
        edit.original = *x;
        *x = next;
        edit.value = next;
        edit.started = edit.changed = edit.finished = true;
    }
    ImGui::PopID();
    return edit;
}

// ---- Pads -----------------------------------------------------------------------------------------------------
namespace {
const char* PadName(int index) {
    static const auto names = [] {
        std::array<std::array<char, 12>, 128> table{};
        for (std::size_t i = 0; i < table.size(); ++i)
            std::snprintf(table[i].data(), table[i].size(), "Pad %zu", i + 1);
        return table;
    }();
    return index >= 0 && index < static_cast<int>(names.size()) ? names[static_cast<std::size_t>(index)].data() : "Pad";
}
} // namespace

namespace impl {
void DrawPadCell(ImDrawList* draw, const ShowColors& c, const ComponentOptions& options, ImVec2 a, ImVec2 b, const PadView& pad,
                 int i, bool hovered, bool held, bool focused, float round) {
    const float u = Unit();
    const float padW = b.x - a.x;
        const float emph = Ease(options, ImGui::GetItemID(), held ? 1.f : hovered ? .6f : 0.f);
        const ImVec4 base = pad.color.w > 0 ? pad.color : c.layer(static_cast<std::size_t>(i));
        const ImVec2 inset = held ? ImVec2{1.5f, 1.5f} : ImVec2{0, 0};
        const ImVec2 ia{a.x + inset.x, a.y + inset.y}, ib{b.x - inset.x, b.y - inset.y};
        ImVec4 ink = c.text;
        if (pad.state == PadState::Empty) {
            detail::DrawDashedRect(draw, ia, ib, round, U32(c.muted, .45f + .3f * emph), 1.f, std::max(2.5f, u * .1f));
        } else {
            ImVec4 fill = Mix(c.input, base, .16f + .1f * emph);
            ImVec4 border = Mix(c.border, base, .5f);
            switch (pad.state) {
            case PadState::Loaded: fill = Mix(c.input, base, .3f + .1f * emph); border = base; break;
            case PadState::Queued: fill = Mix(c.input, base, .3f + .1f * emph); border = c.queued; break;
            case PadState::Playing: fill = Mix(c.input, base, .92f); border = base; ink = InkFor(c, fill); break;
            case PadState::Error: fill = Mix(c.input, c.error, .2f + .1f * emph); border = c.error; break;
            default: break;
            }
            draw->AddRectFilled(ia, ib, U32(fill), round);
            if (pad.state == PadState::Queued)
                detail::DrawDashedRect(draw, ia, ib, round, U32(border), 1.5f, std::max(3.f, u * .12f));
            else
                draw->AddRect(ia, ib, U32(border), round, 0, pad.state == PadState::Playing ? 1.f : 1.f);
            if (pad.progress >= 0) {
                const float t = Clamp01(pad.progress);
                const float h = std::max(3.f, u * .12f);
                draw->PushClipRect({ia.x, ib.y - h}, {ia.x + (ib.x - ia.x) * t, ib.y}, true);
                draw->AddRectFilled(ia, ib, U32(pad.state == PadState::Playing ? ink : base, .85f), round);
                draw->PopClipRect();
            }
            // State glyph (shape, not just colour) in the top-right corner.
            const ImVec2 gc{ib.x - u * .55f, ia.y + u * .55f};
            switch (pad.state) {
            case PadState::Playing: GlyphPlay(draw, gc, u * .5f, U32(ink)); break;
            case PadState::Queued:
                for (int k = -1; k <= 1; ++k)
                    draw->AddCircleFilled({gc.x + static_cast<float>(k) * u * .2f, gc.y}, u * .06f, U32(c.queued), 8);
                break;
            case PadState::Error: detail::DrawCross(draw, gc, u * .3f, U32(c.error), 1.8f); break;
            case PadState::Loaded: draw->AddCircle(gc, u * .13f, U32(base), 12, 1.6f); break;
            default: break;
            }
            if (pad.latched)
                draw->AddCircleFilled({ib.x - u * .4f, ib.y - u * .4f}, u * .1f, U32(pad.state == PadState::Playing ? ink : base), 10);
        }
        if (focused && ImGui::GetIO().NavVisible)
            draw->AddRect({a.x - 3, a.y - 3}, {b.x + 3, b.y + 3}, U32(c.focus), round + 3.f, 0, 2.f);
        if (pad.state != PadState::Empty) {
            draw->PushClipRect(ia, ib, true);
            DrawTextFit(draw, {ia.x + u * .4f, ia.y + u * .3f}, padW - u * 1.4f, U32(ink), Safe(pad.label));
            if (pad.caption && pad.caption[0])
                draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {ia.x + u * .4f, ib.y - u * .3f - ImGui::GetTextLineHeight() * .82f},
                              U32(ink, .7f), pad.caption);
            draw->PopClipRect();
        }
}
} // namespace impl

PadEvent PadGrid(const char* id, std::span<const PadView> pads, PadGridOptions o, ComponentOptions options) {
    PadEvent event;
    if (pads.empty())
        return event;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const int columns = std::max(1, o.columns);
    const float gap = std::max(4.f, ImGui::GetStyle().ItemSpacing.x * .6f);
    const float padH = o.padHeight > 0 ? o.padHeight : u * 2.6f;
    const float avail = ImGui::GetContentRegionAvail().x;
    const float padW = o.padWidth > 0 ? o.padWidth : std::max(u * 2.f, (avail - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns));
    const float round = options.theme ? options.theme->radius.control + 2.f : u * .4f;

    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const int rows = (static_cast<int>(pads.size()) + columns - 1) / columns;
    for (int i = 0; i < static_cast<int>(pads.size()); ++i) {
        const PadView& pad = pads[static_cast<std::size_t>(i)];
        const int col = i % columns, row = i / columns;
        const ImVec2 a{origin.x + static_cast<float>(col) * (padW + gap), origin.y + static_cast<float>(row) * (padH + gap)};
        const ImVec2 b{a.x + padW, a.y + padH};
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##pad", {padW, padH});
        const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
        const bool focused = ImGui::IsItemFocused();
        if (ImGui::IsItemActivated() && event.index < 0) {
            event.index = i;
            event.pressed = true;
        }
        if (ImGui::IsItemDeactivated() && event.index < 0) {
            event.index = i;
            event.released = true;
        }
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && event.index < 0) {
            event.index = i;
            event.secondary = true;
        }
        if (o.dropPayload && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(o.dropPayload)) {
                event.index = i;
                event.payload = payload;
            }
            ImGui::EndDragDropTarget();
        }
        impl::DrawPadCell(draw, c, options, a, b, pad, i, hovered, held, focused, round);
        const char* name = pad.state == PadState::Empty ? FrameText("Empty pad %d", i + 1)
                           : (pad.label && pad.label[0]) ? pad.label : PadName(i);
        if (SemanticItem(options, name, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press, pad.latched) &&
            event.index < 0) {
            event.index = i;
            event.pressed = event.released = true;
        }
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({std::max(0.f, padW * static_cast<float>(columns) + gap * static_cast<float>(columns - 1)),
                  static_cast<float>(rows) * (padH + gap) - gap});
    ImGui::PopID();
    return event;
}

// ---- Level meter ----------------------------------------------------------------------------------------------
float AmplitudeToDecibels(float amplitude) {
    if (!(amplitude > 1e-6f))
        return -120.f;
    return 20.f * std::log10(amplitude);
}

void PeakHold::Update(std::span<const float> decibels, float deltaSeconds, float holdSeconds, float fallDbPerSecond) {
    const float dt = std::isfinite(deltaSeconds) ? std::max(0.f, deltaSeconds) : 0.f;
    for (std::size_t i = 0; i < level.size(); ++i) {
        const float in = i < decibels.size() && std::isfinite(decibels[i]) ? decibels[i] : -120.f;
        level[i] = in;
        if (in >= peak[i]) {
            peak[i] = in;
            age[i] = 0;
        } else {
            age[i] += dt;
            if (age[i] > holdSeconds)
                peak[i] = std::max(in, peak[i] - fallDbPerSecond * dt);
        }
        held[i] = std::max(held[i] - fallDbPerSecond * dt, in);
    }
}

void MeterBar(const char* id, std::span<const float> levelsDb, std::span<const float> peaksDb, MeterOptions o, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    static const float silent[1] = {-120.f};
    if (levelsDb.empty())
        levelsDb = silent; // An empty span draws one silent channel instead of reading past the end.
    const std::size_t channels = std::clamp<std::size_t>(levelsDb.size(), 1, 8);
    const float lo = o.minimumDb, hi = o.maximumDb > o.minimumDb ? o.maximumDb : o.minimumDb + 1;
    const float scaleW = !o.scaleDb.empty() && o.vertical ? u * 1.5f : 0.f;
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = o.vertical ? u * (.55f * static_cast<float>(channels) + .35f) + scaleW : std::max(u * 6.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = o.vertical ? u * 7.f : u * (.55f * static_cast<float>(channels) + .35f);
    ImGui::PushID(id);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ReadoutItem(size);
    const float gap = 2.f;
    const float bar = o.vertical ? (size.x - scaleW - gap * static_cast<float>(channels - 1)) / static_cast<float>(channels)
                                 : (size.y - gap * static_cast<float>(channels - 1)) / static_cast<float>(channels);
    const float along = o.vertical ? size.y : size.x - 0.f;
    const float segment = std::max(2.f, u * .16f), segGap = 1.f;
    const int segments = std::max(4, static_cast<int>(along / (segment + segGap)));
    const auto zoneColor = [&](float db) { return db >= o.clipDb ? c.error : db >= o.warnDb ? c.warning : c.success; };
    float maxLevel = -120.f;
    for (std::size_t ch = 0; ch < channels; ++ch) {
        const float level = std::isfinite(levelsDb[ch]) ? levelsDb[ch] : -120.f;
        maxLevel = std::max(maxLevel, level);
        const float peak = ch < peaksDb.size() && std::isfinite(peaksDb[ch]) ? peaksDb[ch] : -120.f;
        for (int s = 0; s < segments; ++s) {
            const float db = lo + (static_cast<float>(s) + .5f) / static_cast<float>(segments) * (hi - lo);
            const bool lit = level >= db;
            const bool isPeak = o.showPeak && peak >= db && peak < db + (hi - lo) / static_cast<float>(segments) && !lit;
            const ImU32 col = lit ? U32(zoneColor(db)) : isPeak ? U32(zoneColor(db), .95f) : U32(zoneColor(db), .13f);
            if (o.vertical) {
                const float x0 = p0.x + static_cast<float>(ch) * (bar + gap);
                const float y1 = p0.y + size.y - static_cast<float>(s) * (size.y / static_cast<float>(segments));
                draw->AddRectFilled({x0, y1 - (size.y / static_cast<float>(segments) - segGap)}, {x0 + bar, y1}, col, 1.5f);
            } else {
                const float y0 = p0.y + static_cast<float>(ch) * (bar + gap);
                const float x0 = p0.x + static_cast<float>(s) * (size.x / static_cast<float>(segments));
                draw->AddRectFilled({x0, y0}, {x0 + (size.x / static_cast<float>(segments) - segGap), y0 + bar}, col, 1.5f);
            }
        }
    }
    if (o.vertical && !o.scaleDb.empty()) {
        for (float tick : o.scaleDb) {
            if (tick < lo || tick > hi)
                continue;
            const float y = p0.y + size.y - (tick - lo) / (hi - lo) * size.y;
            char text[16];
            std::snprintf(text, sizeof(text), "%.0f", tick);
            const ImVec2 ts = ImGui::CalcTextSize(text);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .8f, {p0.x + size.x - scaleW + 4.f, y - ts.y * .4f}, U32(c.muted), text);
        }
    }
    detail::ValueNode node;
    node.role = accessibility::SemanticRole::Meter;
    node.name = Visible(id) ? id : "Level";
    node.value = maxLevel;
    node.minimum = lo;
    node.maximum = hi;
    node.text = FrameText("%.1f dB", maxLevel);
    node.readOnly = true;
    detail::AnnotateValue(options, node);
    ImGui::PopID();
}

// ---- Timecode and countdown -----------------------------------------------------------------------------------
ClockUrgency CountdownUrgency(double secondsRemaining, double warnSeconds, double criticalSeconds) {
    if (!std::isfinite(secondsRemaining))
        return ClockUrgency::Normal;
    if (secondsRemaining <= criticalSeconds)
        return ClockUrgency::Critical;
    if (secondsRemaining <= warnSeconds)
        return ClockUrgency::Warning;
    return ClockUrgency::Normal;
}

void TimecodeDisplay(const char* id, const TimecodeView& view, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImFont* font = ImGui::GetFont();
    const float fontSize = view.size > 0 ? view.size : u * 1.7f;
    char text[40] = "--:--:--:--";
    editor::FormatTimecode(view.time < 0 ? -view.time : view.time, view.rate, view.dropFrame, std::span<char>(text, 32));
    const float digit = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, "0").x;
    const bool minus = view.countdown || view.time < 0;
    float width = minus ? digit * .7f : 0.f;
    for (const char* ch = text; *ch; ++ch)
        width += (*ch >= '0' && *ch <= '9') ? digit : digit * .5f;
    const bool labelled = view.label && view.label[0];
    const float captionH = labelled ? ImGui::GetTextLineHeight() + 2.f : 0.f;
    ImGui::PushID(id);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ReadoutItem({width + u * .2f, fontSize * 1.15f + captionH});
    ImVec4 ink = view.urgency == ClockUrgency::Critical ? c.error : view.urgency == ClockUrgency::Warning ? c.warning : c.text;
    float x = p0.x + u * .1f;
    const float y = p0.y + captionH;
    if (labelled)
        draw->AddText({p0.x + u * .1f, p0.y}, U32(c.muted), view.label);
    if (minus) {
        draw->AddText(font, fontSize, {x, y}, U32(ink), "-");
        x += digit * .7f;
    }
    for (const char* ch = text; *ch; ++ch) {
        const bool isDigit = *ch >= '0' && *ch <= '9';
        const float advance = isDigit ? digit : digit * .5f;
        const float cw = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, ch, ch + 1).x;
        draw->AddText(font, fontSize, {x + (advance - cw) * .5f, y}, U32(isDigit ? ink : Mix(ink, c.surface, view.running ? .15f : .6f)), ch, ch + 1);
        x += advance;
    }
    AnnotateText(options, view.label && view.label[0] ? view.label : "Timecode", FrameText("%s%s", minus ? "-" : "", text),
                 accessibility::SemanticRole::Status);
    ImGui::PopID();
}

// ---- Binding badge, safety controls and status lights --------------------------------------------------------
BindingRequest BindingBadge(const char* id, const BindingInfo& info, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float h = u * .85f;
    const bool assigned = info.kind != BindingKind::None;
    const char* kind = info.kind == BindingKind::Midi ? "MIDI" : info.kind == BindingKind::Osc ? "OSC" : info.kind == BindingKind::Key ? "KEY"
                       : info.kind == BindingKind::Modulator ? "MOD" : "";
    char body[96];
    if (info.learning)
        std::snprintf(body, sizeof(body), "%s", Str(options, "show.learning", "Learning..."));
    else if (!assigned)
        std::snprintf(body, sizeof(body), "%s", Str(options, "show.unassigned", "Not assigned"));
    else
        std::snprintf(body, sizeof(body), info.range && info.range[0] ? "%s %s  %s" : "%s %s", kind, Safe(info.source), Safe(info.range));
    const float textW = ImGui::CalcTextSize(body).x;
    const float closeW = assigned && !info.learning ? h : 0.f;
    const float w = h * .9f + textW + h * .5f + closeW;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
    ImGui::InvisibleButton("##badge", {w - closeW, h});
    const bool hovered = ImGui::IsItemHovered();
    BindingRequest request = ImGui::IsItemClicked() ? BindingRequest::ToggleLearn : BindingRequest::None;
    SemanticItem(options, FrameText("%s", body), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
    bool closeHovered = false;
    if (closeW > 0) {
        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##clear", {closeW, h});
        closeHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            request = BindingRequest::Clear;
        SemanticItem(options, Str(options, "show.clearBinding", "Clear assignment"), accessibility::SemanticRole::Button,
                     accessibility::SemanticAction::Press);
    }
    const ImVec4 tone = info.learning ? c.warning : assigned ? c.accent : c.muted;
    const float pulse = info.learning && MotionAllowed(options) ? .5f + .5f * std::sin(static_cast<float>(ImGui::GetTime()) * 6.f) : 1.f;
    draw->AddRectFilled(a, b, U32(Mix(c.input, tone, assigned || info.learning ? .14f + (hovered ? .06f : 0.f) : (hovered ? .05f : 0.f))), h * .5f);
    if (assigned || info.learning)
        draw->AddRect(a, b, U32(tone, info.learning ? .5f + .5f * pulse : .5f), h * .5f, 0, 1.f);
    else
        detail::DrawDashedRect(draw, a, b, h * .5f, U32(c.muted, .6f), 1.f, 3.f);
    draw->AddCircleFilled({a.x + h * .5f, a.y + h * .5f}, h * .14f, U32(tone), 10);
    draw->AddText({a.x + h * .9f, a.y + (h - ImGui::GetTextLineHeight()) * .5f}, U32(assigned || info.learning ? c.text : c.muted), body);
    if (closeW > 0)
        detail::DrawCross(draw, {b.x - h * .5f, a.y + h * .5f}, h * .3f, U32(closeHovered ? c.text : c.muted), 1.4f);
    ImGui::PopID();
    return request;
}

bool HoldButton(const char* label, HoldButtonOptions o, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = ImGui::CalcTextSize(label, nullptr, true).x + u * 1.6f;
    if (size.y <= 0)
        size.y = u * 1.1f;
    ImGui::PushID(label);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##hold", size);
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive(), focused = ImGui::IsItemFocused();
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID heldKey = ImGui::GetID("##held"), firedKey = ImGui::GetID("##fired");
    float held = active ? st->GetFloat(heldKey, 0.f) + io.DeltaTime : 0.f;
    st->SetFloat(heldKey, held);
    const float seconds = std::max(.1f, o.seconds);
    bool fired = false;
    if (active && held >= seconds && !st->GetBool(firedKey)) {
        fired = true;
        st->SetBool(firedKey, true);
    }
    if (!active)
        st->SetBool(firedKey, false);
    if (SemanticItem(options, label, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press))
        fired = true;
    const float progress = Clamp01(held / seconds);
    const ImVec4 tone = o.destructive ? c.error : c.accent;
    const float round = size.y * .5f;
    draw->AddRectFilled(a, b, U32(Mix(c.input, tone, hovered ? .2f : .12f)), round);
    if (progress > 0) {
        draw->PushClipRect(a, {a.x + size.x * progress, b.y}, true);
        draw->AddRectFilled(a, b, U32(tone, .5f), round);
        draw->PopClipRect();
    }
    draw->AddRect(a, b, U32(tone, .65f), round, 0, 1.f);
    if (focused && io.NavVisible)
        draw->AddRect({a.x - 3, a.y - 3}, {b.x + 3, b.y + 3}, U32(c.focus), round + 3.f, 0, 2.f);
    const char* shown = label;
    DrawTextCentered(draw, a, b, U32(c.text), shown);
    if (hovered && !active)
        ImGui::SetTooltip("%s", Str(options, "show.holdToConfirm", "Hold to confirm"));
    ImGui::PopID();
    return fired;
}

bool ArmSwitch(const char* label, bool* armed, ComponentOptions options) {
    if (!armed)
        return false;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float h = u * 1.0f, trackW = u * 2.1f;
    const char* state = *armed ? Str(options, "show.armed", "Armed") : Str(options, "show.safe", "Safe");
    const float textW = std::max(ImGui::CalcTextSize(Str(options, "show.armed", "Armed")).x, ImGui::CalcTextSize(Str(options, "show.safe", "Safe")).x);
    const float w = trackW + 8.f + textW;
    ImGui::PushID(label);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##arm", {w, h});
    bool changed = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered(), focused = ImGui::IsItemFocused();
    if (SemanticItem(options, label, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, *armed))
        changed = true;
    if (changed)
        *armed = !*armed;
    const float t = Ease(options, ImGui::GetItemID(), *armed ? 1.f : 0.f);
    const ImVec2 b{a.x + trackW, a.y + h};
    draw->AddRectFilled(a, b, U32(Mix(c.input, c.armed, .15f + .65f * t + (hovered ? .06f : 0.f))), h * .5f);
    draw->AddRect(a, b, U32(Mix(c.border, c.armed, t)), h * .5f, 0, 1.f);
    const float kr = h * .38f;
    const ImVec2 kc{a.x + h * .5f + (trackW - h) * t, a.y + h * .5f};
    draw->AddCircleFilled(kc, kr, U32(Mix(c.text, c.onAccent, 0.f)), 20);
    GlyphLock(draw, kc, kr * 1.45f, U32(Mix(c.surface, c.armed, t)), !*armed);
    draw->AddText({b.x + 8.f, a.y + (h - ImGui::GetTextLineHeight()) * .5f}, U32(*armed ? c.armed : c.muted), state);
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 3, a.y - 3}, {b.x + 3, b.y + 3}, U32(c.focus), h * .5f + 3.f, 0, 2.f);
    ImGui::PopID();
    return changed;
}

void StatusLed(const char* id, LedState state, const char* label, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const bool named = label && label[0];
    const float textW = named ? ImGui::CalcTextSize(label).x + u * .4f : 0.f;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    ReadoutItem({u * .9f + textW, u * .9f});
    const ImVec2 ctr{a.x + u * .45f, a.y + u * .45f};
    const float r = u * .2f;
    switch (state) {
    case LedState::Live: {
        const float pulse = MotionAllowed(options) ? .5f + .5f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.f) : 1.f;
        draw->AddCircleFilled(ctr, r, U32(c.error), 16);
        draw->AddCircle(ctr, r * (1.5f + .3f * pulse), U32(c.error, .5f), 16, 1.5f);
        break;
    }
    case LedState::On: draw->AddCircle(ctr, r, U32(c.success), 16, 2.f); draw->AddCircleFilled(ctr, r * .45f, U32(c.success), 12); break;
    case LedState::Warning: GlyphWarning(draw, ctr, r * 2.4f, U32(c.warning)); break;
    case LedState::Error: draw->AddCircleFilled(ctr, r * 1.1f, U32(c.error), 16); detail::DrawCross(draw, ctr, r * 1.1f, U32(c.surface), 1.8f); break;
    default: draw->AddCircleFilled(ctr, r * .7f, U32(c.border), 12); break;
    }
    if (named)
        draw->AddText({a.x + u * .9f, a.y + (u * .9f - ImGui::GetTextLineHeight()) * .5f}, U32(state == LedState::Off ? c.muted : c.text), label);
    const char* word = state == LedState::Live ? "live" : state == LedState::On ? "on" : state == LedState::Warning ? "warning" : state == LedState::Error ? "error" : "off";
    AnnotateText(options, named ? label : id, word, accessibility::SemanticRole::Status);
    ImGui::PopID();
}
} // namespace imkit::show
