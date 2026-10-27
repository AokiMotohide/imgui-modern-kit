#include <imkit/show/led.h>
#include "internal.h"
#include <imkit/charts.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace imkit::show {
using namespace impl;

// ---- Pixel preview ---------------------------------------------------------------------------------------------
PixelPreviewRequest PixelPreview(const char* id, const PixelPreviewView& view, int* selected, PixelPreviewOptions o, ComponentOptions options) {
    PixelPreviewRequest request;
    const int n = static_cast<int>(view.colors.size());
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const bool custom = n > 0 && view.positions.size() >= view.colors.size();
    const int cols = custom ? 1 : std::max(1, view.columns > 0 ? std::min(view.columns, std::max(1, n)) : std::max(1, n));
    const int rows = custom ? 1 : std::max(1, (n + cols - 1) / cols);
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 8.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0) {
        if (custom)
            size.y = size.x * .5f;
        else if (rows == 1)
            size.y = u * 2.4f;
        else
            size.y = std::clamp(size.x * static_cast<float>(rows) / static_cast<float>(cols), u * 3.f, u * 30.f);
    }
    const float round = ControlRound(options);
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##pixels", size);
    const bool hovered = ImGui::IsItemHovered();
    draw->AddRectFilled(a, b, U32(ImVec4{.05f, .05f, .07f, 1.f}), round); // LEDs are shown on black in every theme.
    draw->AddRect(a, b, U32(c.border, .8f), round, 0, 1.f);
    AnnotateText(options, "Pixel preview", FrameText("%d pixels", n), accessibility::SemanticRole::Status);
    if (n == 0) {
        draw->AddText({a.x + u * .5f, a.y + u * .4f}, U32(c.muted), Str(options, "show.noPixels", "No pixels"));
        ImGui::PopID();
        return request;
    }
    const float pad = u * .4f;
    const ImVec2 imin{a.x + pad, a.y + pad}, imax{b.x - pad, b.y - pad};
    const float iw = std::max(1.f, imax.x - imin.x), ih = std::max(1.f, imax.y - imin.y);
    float pitch, ox = imin.x, oy = imin.y;
    if (custom) {
        pitch = std::max(1.f, std::min(iw, ih) / std::sqrt(static_cast<float>(n)));
    } else {
        pitch = std::max(.5f, std::min(iw / static_cast<float>(cols), ih / static_cast<float>(rows)));
        ox += (iw - pitch * static_cast<float>(cols)) * .5f;
        oy += (ih - pitch * static_cast<float>(rows)) * .5f;
    }
    auto centre = [&](int i) {
        if (custom) {
            const ImVec2 p = view.positions[static_cast<std::size_t>(i)];
            return ImVec2{imin.x + Clamp01(p.x) * iw, imin.y + Clamp01(p.y) * ih};
        }
        const int row = i / cols;
        int col = i % cols;
        if (view.serpentine && (row & 1))
            col = cols - 1 - col;
        return ImVec2{ox + (static_cast<float>(col) + .5f) * pitch, oy + (static_cast<float>(row) + .5f) * pitch};
    };
    const float radius = std::max(.4f, pitch * .5f * std::clamp(o.dotScale, .1f, 1.f));

    // Pointer: which pixel is under it.
    int under = -1;
    if (hovered) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        if (custom) {
            float best = radius * radius * 4.f + 4.f;
            for (int i = 0; i < n; ++i) {
                const ImVec2 p = centre(i);
                const float d = (p.x - m.x) * (p.x - m.x) + (p.y - m.y) * (p.y - m.y);
                if (d < best) {
                    best = d;
                    under = i;
                }
            }
        } else {
            const int col = static_cast<int>(std::floor((m.x - ox) / pitch)), row = static_cast<int>(std::floor((m.y - oy) / pitch));
            if (col >= 0 && col < cols && row >= 0 && row < rows) {
                const int k = row * cols + ((view.serpentine && (row & 1)) ? cols - 1 - col : col);
                if (k < n)
                    under = k;
            }
        }
    }
    request.hovered = under;
    if (ImGui::IsItemClicked() && under >= 0) {
        request.clicked = under;
        if (selected)
            *selected = under;
    }

    draw->PushClipRect(a, b, true);
    const bool rich = n <= 2000;
    for (int i = 0; i < n; ++i) {
        const ImVec4 col = view.colors[static_cast<std::size_t>(i)];
        const ImVec2 p = centre(i);
        const ImVec4 shown{Clamp01(col.x), Clamp01(col.y), Clamp01(col.z), 1.f};
        if (radius < 1.5f) {
            draw->AddRectFilled({p.x - std::max(.5f, pitch * .5f), p.y - radius}, {p.x + std::max(.5f, pitch * .5f), p.y + radius}, U32(shown));
            continue;
        }
        if (!rich) {
            // Very large previews trade detail for speed: one small square per pixel.
            draw->AddRectFilled({p.x - radius, p.y - radius}, {p.x + radius, p.y + radius}, U32(shown));
            continue;
        }
        const float peak = std::max({shown.x, shown.y, shown.z});
        if (o.glow && radius >= 3.f && peak > .35f)
            draw->AddCircleFilled(p, radius * 1.7f, U32(shown, .14f * peak), 12);
        const int segments = radius > 6.f ? 14 : 8;
        draw->AddCircleFilled(p, radius, U32(shown), segments);
        if (radius > 5.f)
            draw->AddCircle(p, radius, U32(Mix(shown, ImVec4{0, 0, 0, 1}, .55f)), segments, 1.f);
    }
    if (o.showWiring && n <= 2000 && n > 1) {
        const int arrowEvery = std::max(1, n / 12);
        for (int i = 0; i + 1 < n; ++i) {
            const ImVec2 p = centre(i), q = centre(i + 1);
            draw->AddLine(p, q, U32(c.muted, .45f), 1.f);
            if (i % arrowEvery == 0) {
                const float dx = q.x - p.x, dy = q.y - p.y, len = std::max(.001f, std::sqrt(dx * dx + dy * dy));
                const ImVec2 mid{(p.x + q.x) * .5f, (p.y + q.y) * .5f}, dir{dx / len, dy / len};
                const float s = std::max(3.f, radius * .7f);
                draw->AddTriangleFilled({mid.x + dir.x * s, mid.y + dir.y * s}, {mid.x - dir.x * s * .6f - dir.y * s * .6f, mid.y - dir.y * s * .6f + dir.x * s * .6f},
                                        {mid.x - dir.x * s * .6f + dir.y * s * .6f, mid.y - dir.y * s * .6f - dir.x * s * .6f}, U32(c.muted, .75f));
            }
        }
    }
    if (o.showIndices && pitch >= u * 1.4f && n <= 1024)
        for (int i = 0; i < n; ++i) {
            const ImVec2 p = centre(i);
            const char* text = FrameText("%d", i);
            const ImVec2 ts = ImGui::CalcTextSize(text);
            const ImVec4 col = view.colors[static_cast<std::size_t>(i)];
            const float lum = .3f * col.x + .59f * col.y + .11f * col.z;
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .72f, {p.x - ts.x * .36f, p.y - ts.y * .36f}, lum > .5f ? IM_COL32(0, 0, 0, 200) : IM_COL32(255, 255, 255, 220), text);
        }
    for (int pass = 0; pass < 2; ++pass) {
        const int i = pass == 0 ? under : (selected ? *selected : -1);
        if (i < 0 || i >= n)
            continue;
        draw->AddCircle(centre(i), radius + 2.f, U32(pass == 0 ? c.text : c.accent, pass == 0 ? .7f : 1.f), 16, pass == 0 ? 1.f : 2.f);
    }
    draw->PopClipRect();
    if (under >= 0) {
        const ImVec4 col = view.colors[static_cast<std::size_t>(under)];
        ImGui::SetTooltip("#%d   R %d  G %d  B %d", under, static_cast<int>(std::lround(Clamp01(col.x) * 255.f)),
                          static_cast<int>(std::lround(Clamp01(col.y) * 255.f)), static_cast<int>(std::lround(Clamp01(col.z) * 255.f)));
    }
    ImGui::PopID();
    return request;
}

// ---- Budget bar ------------------------------------------------------------------------------------------------
float EstimateLedCurrent(int pixels, float amperesPerPixel, float brightness, float load) {
    if (pixels <= 0 || !std::isfinite(amperesPerPixel) || amperesPerPixel <= 0)
        return 0.f;
    return static_cast<float>(pixels) * amperesPerPixel * Clamp01(brightness) * Clamp01(load);
}

BudgetStatus BudgetBar(const char* id, const BudgetBarView& view, BudgetBarOptions o, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    float total = 0.f;
    for (const BudgetSegment& s : view.segments)
        total += std::isfinite(s.amount) ? std::max(0.f, s.amount) : 0.f;
    const float limit = std::isfinite(view.limit) && view.limit > 1e-6f ? view.limit : 1e-6f;
    const float warn = Clamp01(view.warnFraction);
    const BudgetStatus status = total > limit ? BudgetStatus::Over : total >= limit * warn ? BudgetStatus::Warning : BudgetStatus::Ok;
    const float w = o.size.x > 0 ? o.size.x : std::max(u * 8.f, ImGui::GetContentRegionAvail().x);
    const float barH = o.size.y > 0 ? o.size.y : u * 1.2f;
    const float scaleMax = std::max(limit * 1.2f, total * 1.05f);
    const float round = barH * .35f;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float headH = u * 1.1f;
    ReadoutItem({w, headH + barH + (view.segments.size() > 1 ? u * 1.1f : 0.f)});
    const char* unit = Safe(view.unit);
    const char* numberFormat = SafeFormat(o.format, "fge", "%.1f");
    char fmt[48];
    std::snprintf(fmt, sizeof fmt, "%s / %s %%s", numberFormat, numberFormat);
    const char* value = FrameText(fmt, static_cast<double>(total), static_cast<double>(limit), unit);
    const char* word = status == BudgetStatus::Over ? Str(options, "show.overLimit", "Over limit")
                       : status == BudgetStatus::Warning ? Str(options, "show.nearLimit", "Near limit")
                                                         : Str(options, "show.withinLimit", "Within limit");
    const ImVec4 tone = status == BudgetStatus::Over ? c.error : status == BudgetStatus::Warning ? c.warning : c.muted;
    if (view.title && view.title[0])
        draw->AddText({a.x, a.y + u * .1f}, U32(c.text), view.title);
    {
        const ImVec2 vs = ImGui::CalcTextSize(value), ws = ImGui::CalcTextSize(word);
        const float right = a.x + w;
        draw->AddText({right - vs.x, a.y + u * .1f}, U32(c.text), value);
        const float wx = right - vs.x - u * .6f - ws.x;
        draw->AddText({wx, a.y + u * .1f}, U32(tone), word);
        const ImVec2 g{wx - u * .45f, a.y + u * .1f + ws.y * .5f};
        if (status == BudgetStatus::Over)
            detail::DrawCross(draw, g, u * .26f, U32(c.error), 1.8f);
        else if (status == BudgetStatus::Warning)
            GlyphWarning(draw, g, u * .6f, U32(c.warning));
        else
            detail::DrawCheck(draw, g, u * .28f, U32(c.success), 1.8f);
    }
    const ImVec2 ba{a.x, a.y + headH}, bb{a.x + w, a.y + headH + barH};
    draw->AddRectFilled(ba, bb, U32(Recessed(c, .3f)), round);
    draw->PushClipRect(ba, bb, true);
    float x = ba.x;
    for (std::size_t i = 0; i < view.segments.size(); ++i) {
        const BudgetSegment& s = view.segments[i];
        const float amount = std::isfinite(s.amount) ? std::max(0.f, s.amount) : 0.f;
        const float sw = w * amount / scaleMax;
        if (sw <= 0.f)
            continue;
        const ImVec4 col = s.color.w > 0 ? s.color : ChartColor(options.theme, i);
        draw->AddRectFilled({x, ba.y}, {x + sw, bb.y}, U32(col));
        if (i + 1 < view.segments.size())
            draw->AddLine({x + sw, ba.y}, {x + sw, bb.y}, U32(c.surface), 1.5f);
        x += sw;
    }
    if (status == BudgetStatus::Over) {
        // Hatching over the part beyond the limit.
        const float lx = ba.x + w * limit / scaleMax;
        draw->PushClipRect({lx, ba.y}, {bb.x, bb.y}, true);
        for (float hx = lx - barH; hx < bb.x; hx += u * .45f)
            draw->AddLine({hx, bb.y}, {hx + barH, ba.y}, U32(c.error), 1.5f);
        draw->PopClipRect();
    }
    draw->PopClipRect();
    draw->AddRect(ba, bb, U32(c.border, .8f), round, 0, 1.f);
    // Limit and warning marks.
    const float lx = ba.x + w * limit / scaleMax, wx = ba.x + w * limit * warn / scaleMax;
    draw->AddLine({lx, ba.y - 3.f}, {lx, bb.y + 3.f}, U32(c.text), 2.f);
    for (float dy = ba.y - 1.f; dy < bb.y + 1.f; dy += 6.f)
        draw->AddLine({wx, dy}, {wx, std::min(dy + 3.f, bb.y + 1.f)}, U32(c.warning, .9f), 1.5f);
    if (view.segments.size() > 1) {
        float lxg = a.x;
        const float ly = bb.y + u * .25f;
        for (std::size_t i = 0; i < view.segments.size(); ++i) {
            const BudgetSegment& s = view.segments[i];
            const ImVec4 col = s.color.w > 0 ? s.color : ChartColor(options.theme, i);
            const char* text = FrameText("%s ", Safe(s.label));
            const char* amount = FrameText(numberFormat, static_cast<double>(s.amount));
            const float tw = ImGui::CalcTextSize(text).x + ImGui::CalcTextSize(amount).x + u * .9f;
            if (lxg + tw > a.x + w)
                break;
            draw->AddRectFilled({lxg, ly + u * .15f}, {lxg + u * .4f, ly + u * .55f}, U32(col), 2.f);
            draw->AddText({lxg + u * .55f, ly}, U32(c.muted), text);
            draw->AddText({lxg + u * .55f + ImGui::CalcTextSize(text).x, ly}, U32(c.text), amount);
            lxg += tw + u * .5f;
        }
    }
    detail::AnnotateValue(options, {accessibility::SemanticRole::Meter, Safe(view.title)[0] ? view.title : "Budget", "", word, static_cast<double>(total), 0,
                                    static_cast<double>(limit), 1, 10, false, true});
    ImGui::PopID();
    return status;
}

// ---- Test patterns ---------------------------------------------------------------------------------------------
const char* TestPatternName(TestPattern pattern) {
    switch (pattern) {
    case TestPattern::White: return "White";
    case TestPattern::Red: return "Red";
    case TestPattern::Green: return "Green";
    case TestPattern::Blue: return "Blue";
    case TestPattern::Black: return "Black";
    case TestPattern::Gradient: return "Gradient";
    case TestPattern::Grid: return "Grid";
    case TestPattern::Checker: return "Checker";
    case TestPattern::Chase: return "Chase";
    case TestPattern::Strobe: return "Strobe";
    case TestPattern::Numbers: return "Numbers";
    case TestPattern::BurnIn: return "Burn-in";
    default: return "Off";
    }
}

namespace {
void DrawPatternGlyph(ImDrawList* d, const ShowColors& c, TestPattern p, ImVec2 a, float s) {
    const ImVec2 b{a.x + s, a.y + s};
    const ImU32 line = U32(c.text, .85f);
    switch (p) {
    case TestPattern::White: d->AddRectFilled(a, b, IM_COL32(245, 245, 245, 255), s * .2f); break;
    case TestPattern::Red: d->AddRectFilled(a, b, IM_COL32(235, 60, 60, 255), s * .2f); break;
    case TestPattern::Green: d->AddRectFilled(a, b, IM_COL32(60, 200, 90, 255), s * .2f); break;
    case TestPattern::Blue: d->AddRectFilled(a, b, IM_COL32(70, 110, 240, 255), s * .2f); break;
    case TestPattern::Black:
        d->AddRectFilled(a, b, IM_COL32(8, 8, 10, 255), s * .2f);
        d->AddRect(a, b, U32(c.border), s * .2f, 0, 1.f);
        break;
    case TestPattern::Gradient:
        d->AddRectFilledMultiColor(a, b, IM_COL32(0, 0, 0, 255), IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255));
        d->AddRect(a, b, U32(c.border), s * .2f, 0, 1.f);
        break;
    case TestPattern::Grid:
        d->AddRect(a, b, line, s * .15f, 0, 1.2f);
        d->AddLine({a.x + s * .5f, a.y}, {a.x + s * .5f, b.y}, line, 1.2f);
        d->AddLine({a.x, a.y + s * .5f}, {b.x, a.y + s * .5f}, line, 1.2f);
        break;
    case TestPattern::Checker:
        d->AddRect(a, b, line, s * .15f, 0, 1.2f);
        d->AddRectFilled(a, {a.x + s * .5f, a.y + s * .5f}, line);
        d->AddRectFilled({a.x + s * .5f, a.y + s * .5f}, b, line);
        break;
    case TestPattern::Chase:
        for (int i = 0; i < 4; ++i)
            d->AddCircleFilled({a.x + s * (.12f + .25f * static_cast<float>(i)), a.y + s * .5f}, s * (.06f + .03f * static_cast<float>(i)), line, 8);
        break;
    case TestPattern::Strobe:
        for (int i = 0; i < 4; ++i) {
            const float ang = 3.14159265f * .25f * static_cast<float>(i);
            d->AddLine({a.x + s * (.5f - .45f * std::cos(ang)), a.y + s * (.5f - .45f * std::sin(ang))}, {a.x + s * (.5f + .45f * std::cos(ang)), a.y + s * (.5f + .45f * std::sin(ang))}, line, 1.4f);
        }
        break;
    case TestPattern::Numbers: d->AddText(ImGui::GetFont(), s, {a.x + s * .02f, a.y - s * .12f}, line, "12"); break;
    case TestPattern::BurnIn:
        d->AddRect(a, b, line, s * .15f, 0, 1.2f);
        d->AddLine(a, b, line, 1.2f);
        d->AddLine({a.x, b.y}, {b.x, a.y}, line, 1.2f);
        break;
    default: break;
    }
}
} // namespace

TestPatternRequest TestPatternBar(const char* id, TestPattern active, TestPatternOptions o, ComponentOptions options) {
    TestPatternRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float h = u * (o.compact ? 1.2f : 1.6f), glyph = h * .55f;
    const TestPattern all[] = {TestPattern::White, TestPattern::Red, TestPattern::Green, TestPattern::Blue, TestPattern::Black, TestPattern::Gradient,
                               TestPattern::Grid, TestPattern::Checker, TestPattern::Chase, TestPattern::Strobe, TestPattern::Numbers, TestPattern::BurnIn};
    ImGui::PushID(id);
    const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    for (std::size_t i = 0; i < std::size(all); ++i) {
        const TestPattern p = all[i];
        const char* name = Str(options, FrameText("show.pattern.%d", static_cast<int>(p)), TestPatternName(p));
        const float w = ImGui::CalcTextSize(name).x + glyph + u * 1.4f;
        if (i > 0 && FitsOnLine(right, w, ImGui::GetStyle().ItemSpacing.x))
            ImGui::SameLine();
        ImGui::PushID(static_cast<int>(p));
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + h};
        ImGui::InvisibleButton("##pattern", {w, h});
        const bool hovered = ImGui::IsItemHovered(), on = active == p;
        const bool clicked = ImGui::IsItemClicked();
        const bool byTool = SemanticItem(options, name, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, on);
        if (clicked || byTool) {
            request.changed = true;
            request.pattern = on ? TestPattern::Off : p;
        }
        draw->AddRectFilled(a, b, U32(on ? Mix(c.input, c.accent, .22f) : Mix(c.input, c.text, hovered ? .08f : .03f)), h * .35f);
        draw->AddRect(a, b, U32(on ? c.accent : c.border, .9f), h * .35f, 0, on ? 1.5f : 1.f);
        DrawPatternGlyph(draw, c, p, {a.x + u * .45f, a.y + (h - glyph) * .5f}, glyph);
        draw->AddText({a.x + u * .45f + glyph + u * .4f, a.y + (h - ImGui::GetTextLineHeight()) * .5f}, U32(on ? c.text : c.muted), name);
        if (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible)
            draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), h * .35f + 2.f, 0, 2.f);
        ImGui::PopID();
    }
    ImGui::PopID();
    return request;
}

// ---- Port strip ------------------------------------------------------------------------------------------------
int PortStrip(const char* id, std::span<const PortView> ports, int* selected, PortStripOptions o, ComponentOptions options) {
    int clicked = -1;
    if (ports.empty())
        return clicked;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float w = o.portWidth > 0 ? o.portWidth : u * 6.f, h = u * 2.6f, gap = u * .35f;
    const float avail = std::max(w, ImGui::GetContentRegionAvail().x);
    const int perRow = std::max(1, static_cast<int>((avail + gap) / (w + gap)));
    const float round = ControlRound(options);
    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const int rows = (static_cast<int>(ports.size()) + perRow - 1) / perRow;
    for (int i = 0; i < static_cast<int>(ports.size()); ++i) {
        const PortView& p = ports[static_cast<std::size_t>(i)];
        const ImVec2 a{origin.x + static_cast<float>(i % perRow) * (w + gap), origin.y + static_cast<float>(i / perRow) * (h + gap)}, b{a.x + w, a.y + h};
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##port", {w, h});
        const bool hovered = ImGui::IsItemHovered(), on = selected && *selected == i;
        if (ImGui::IsItemClicked()) {
            clicked = i;
            if (selected)
                *selected = i;
        }
        const ImVec4 tone = p.state == PortState::Active ? c.success : p.state == PortState::Warning ? c.warning : p.state == PortState::Error ? c.error : c.muted;
        draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, hovered ? .07f : .025f)), round);
        draw->AddRect(a, b, U32(on ? c.accent : c.border, on ? 1.f : .8f), round, 0, on ? 2.f : 1.f);
        const ImVec2 g{a.x + u * .75f, a.y + u * .85f};
        switch (p.state) {
        case PortState::Active: draw->AddCircle(g, u * .24f, U32(tone), 14, 2.f); draw->AddCircleFilled(g, u * .11f, U32(tone), 10); break;
        case PortState::Warning: GlyphWarning(draw, g, u * .72f, U32(tone)); break;
        case PortState::Error: draw->AddCircleFilled(g, u * .27f, U32(tone), 14); detail::DrawCross(draw, g, u * .17f, U32(c.surface), 1.8f); break;
        case PortState::Disabled: draw->AddLine({g.x - u * .2f, g.y}, {g.x + u * .2f, g.y}, U32(c.muted, .7f), 2.f); break;
        default: draw->AddCircleFilled(g, u * .1f, U32(c.muted, .7f), 10); break;
        }
        draw->PushClipRect(a, b, true);
        DrawTextFit(draw, {a.x + u * 1.35f, a.y + u * .3f}, w - u * 1.6f, U32(p.state == PortState::Disabled ? c.muted : c.text), Safe(p.label));
        if (p.detail && p.detail[0])
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {a.x + u * 1.35f, a.y + u * 1.25f}, U32(c.muted), p.detail);
        if (p.load >= 0) {
            const float t = Clamp01(p.load);
            const ImVec4 lc = t > .9f ? c.error : t > .7f ? c.warning : c.accent;
            draw->AddRectFilled({a.x, b.y - 3.f}, {a.x + w * t, b.y}, U32(lc));
        }
        draw->PopClipRect();
        const char* word = p.state == PortState::Active ? "active" : p.state == PortState::Warning ? "warning" : p.state == PortState::Error ? "error"
                           : p.state == PortState::Disabled ? "disabled" : "idle";
        if (SemanticItem(options, FrameText("%s, %s", Safe(p.label), word), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, on) && clicked < 0) {
            clicked = i;
            if (selected)
                *selected = i;
        }
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({static_cast<float>(std::min<int>(perRow, static_cast<int>(ports.size()))) * (w + gap) - gap, static_cast<float>(rows) * (h + gap) - gap});
    ImGui::PopID();
    return clicked;
}
} // namespace imkit::show
