#include <imkit/charts.h>
#include "component_internal.h"
#include "draw_internal.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

namespace imkit {
namespace {
constexpr float Pi = 3.14159265358979323846f;
const char* Safe(const char* s) { return s ? s : ""; }
bool Finite(float v) { return std::isfinite(v); }
using detail::MotionAllowed;
ImVec4 TextColor(const ComponentOptions& o) {
    return o.theme ? o.theme->semantic.text : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}
ImVec4 SecondaryColor(const ComponentOptions& o) {
    return o.theme ? o.theme->semantic.textSecondary : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
}
ImVec4 KindColor(FeedbackKind kind, const ComponentOptions& o) {
    if (!o.theme)
        return ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    switch (kind) {
    case FeedbackKind::Success: return o.theme->semantic.success;
    case FeedbackKind::Warning: return o.theme->semantic.warning;
    case FeedbackKind::Error:   return o.theme->semantic.error;
    default:                    return o.theme->semantic.accent;
    }
}
ImVec4 Datum(const ChartDatum& d, std::size_t index, const ComponentOptions& o) {
    return d.color.w > 0 ? d.color : ChartColor(o.theme, index);
}
void Describe(const ComponentOptions& o, accessibility::SemanticRole role, const char* name, const char* value) {
    if (!o.accessibility)
        return;
    accessibility::SemanticNode node;
    node.id = ImGui::GetItemID();
    node.parent = o.parent;
    node.name = name;
    node.value = value;
    node.role = role;
    node.state.readOnly = true;
    accessibility::AnnotateLastItem(*o.accessibility, node);
}
} // namespace

ImVec4 ChartColor(const Theme* theme, std::size_t index) {
    // Okabe-Ito derived hues (colour-blind safe). Slot 0 is the accent.
    // Slot 0 (the accent) is blue in most themes, so the sequence avoids blue.
    static constexpr std::array<std::array<float, 3>, 7> light{{
        {.835f, .369f, .000f}, {.000f, .620f, .451f}, {.800f, .475f, .655f}, {.690f, .510f, .000f},
        {.337f, .706f, .914f}, {.550f, .310f, .700f}, {.350f, .350f, .420f}}};
    static constexpr std::array<std::array<float, 3>, 7> dark{{
        {.902f, .624f, .000f}, {.267f, .780f, .612f}, {.839f, .565f, .722f}, {.941f, .894f, .259f},
        {.949f, .475f, .300f}, {.690f, .620f, .950f}, {.690f, .710f, .780f}}};
    const bool isDark = theme ? theme->scheme == ColorScheme::Dark : false;
    if (index % 8 == 0)
        return theme ? theme->semantic.accent : ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    const auto& rgb = (isDark ? dark : light)[(index % 8 - 1) % 7];
    return {rgb[0], rgb[1], rgb[2], 1.f};
}

int DonutChart(const char* id, const DonutChartView& view, DonutChartOptions layout, ComponentOptions o) {
    const auto& style = ImGui::GetStyle();
    const float diameter = std::max(48.f, layout.diameter > 0 ? layout.diameter : ImGui::GetFrameHeight() * 6.f);
    const float thickness = std::clamp(layout.thickness > 0 ? layout.thickness : diameter * .13f, 3.f, diameter * .3f);
    float sum = 0;
    for (const auto& s : view.segments)
        if (Finite(s.value) && s.value > 0)
            sum += s.value;
    const float total = Finite(view.total) && view.total > sum ? view.total : sum;
    int clicked = -1, hovered = -1;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##ring", {diameter, diameter});
    const bool ringHovered = ImGui::IsItemHovered();
    const bool ringClicked = ImGui::IsItemClicked();
    const ImVec2 c{origin.x + diameter * .5f, origin.y + diameter * .5f};
    const float radius = diameter * .5f - thickness * .5f - 3.f;
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddCircle(c, radius, ImGui::GetColorU32(detail::WithAlpha(TextColor(o), .10f)), 64, thickness);

    // Hit test by polar angle so hover follows the ring rather than the square.
    if (ringHovered && total > 0) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const float dx = m.x - c.x, dy = m.y - c.y, dist = std::sqrt(dx * dx + dy * dy);
        if (std::abs(dist - radius) <= thickness * .6f + 2.f) {
            float angle = std::atan2(dy, dx) + Pi * .5f;
            if (angle < 0)
                angle += 2 * Pi;
            float at = 0;
            for (std::size_t i = 0; i < view.segments.size(); ++i) {
                const auto& s = view.segments[i];
                if (!Finite(s.value) || s.value <= 0)
                    continue;
                const float span = s.value / total * 2 * Pi;
                if (angle >= at && angle < at + span) {
                    hovered = static_cast<int>(i);
                    break;
                }
                at += span;
            }
        }
    }
    if (hovered >= 0 && ringClicked)
        clicked = hovered;

    // The visible gap equals a small fraction of the thickness; round caps eat half the thickness each side.
    const float pad = (thickness * .22f + thickness * .5f) / radius;
    float start = -Pi * .5f;
    for (std::size_t i = 0; i < view.segments.size(); ++i) {
        const auto& s = view.segments[i];
        if (!Finite(s.value) || s.value <= 0 || total <= 0)
            continue;
        const float span = s.value / total * 2 * Pi;
        const ImU32 color = ImGui::GetColorU32(Datum(s, i, o));
        const float bump = static_cast<int>(i) == hovered ? 2.f : 0.f;
        if (sum == total && view.segments.size() == 1) {
            draw->AddCircle(c, radius, color, 64, thickness + bump);
        } else if (span <= pad * 2.f) {
            const float mid = start + span * .5f;
            draw->AddCircleFilled({c.x + std::cos(mid) * radius, c.y + std::sin(mid) * radius}, thickness * .5f + bump * .5f, color, 16);
        } else {
            detail::DrawRingArc(draw, c, radius, thickness + bump, start + pad, start + span - pad, color);
        }
        start += span;
    }

    // Centre read-out: the hovered segment's share replaces the headline value.
    char hoverValue[32]{};
    const char* value = Safe(view.value);
    const char* caption = Safe(view.caption);
    if (hovered >= 0 && total > 0) {
        std::snprintf(hoverValue, sizeof(hoverValue), "%.0f%%", view.segments[static_cast<std::size_t>(hovered)].value / total * 100.f);
        value = hoverValue;
        caption = Safe(view.segments[static_cast<std::size_t>(hovered)].label);
    }
    const float inner = std::max(1.f, (radius - thickness * .5f) * 1.5f);
    const ImVec2 valueSize = ImGui::CalcTextSize(value);
    const float scale = std::min(1.55f, inner / std::max(1.f, valueSize.x));
    const float captionHeight = *caption ? ImGui::GetTextLineHeight() * .85f : 0.f;
    const float blockHeight = valueSize.y * scale + captionHeight;
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale,
                  {c.x - valueSize.x * scale * .5f, c.y - blockHeight * .5f}, ImGui::GetColorU32(TextColor(o)), value);
    if (*caption) {
        const float size = ImGui::GetFontSize() * .85f;
        const ImVec2 cs = ImGui::CalcTextSize(caption);
        const float fit = std::min(1.f, inner / std::max(1.f, cs.x * .85f));
        draw->AddText(ImGui::GetFont(), size * fit, {c.x - cs.x * .85f * fit * .5f, c.y - blockHeight * .5f + valueSize.y * scale},
                      ImGui::GetColorU32(SecondaryColor(o)), caption);
    }
    if (hovered >= 0) {
        const auto& s = view.segments[static_cast<std::size_t>(hovered)];
        ImGui::SetTooltip("%s: %.4g (%.1f%%)", Safe(s.label), s.value, s.value / total * 100.f);
    }
    Describe(o, accessibility::SemanticRole::Group, Safe(view.caption), Safe(view.value));

    if (layout.legend && !view.segments.empty()) {
        const bool beside = ImGui::GetContentRegionAvail().x > diameter + ImGui::GetFontSize() * 14.f;
        if (beside)
            ImGui::SameLine(0, style.ItemSpacing.x * 2.f);
        ImGui::BeginGroup();
        const float dot = ImGui::GetFontSize() * .62f;
        for (std::size_t i = 0; i < view.segments.size(); ++i) {
            const auto& s = view.segments[i];
            if (!Finite(s.value) || s.value < 0)
                continue;
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Dummy({dot, ImGui::GetTextLineHeight()});
            draw->AddRectFilled({p.x, p.y + (ImGui::GetTextLineHeight() - dot) * .5f},
                                {p.x + dot, p.y + (ImGui::GetTextLineHeight() + dot) * .5f},
                                ImGui::GetColorU32(Datum(s, i, o)), dot * .3f);
            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            const bool active = static_cast<int>(i) == hovered;
            ImGui::PushStyleColor(ImGuiCol_Text, active ? TextColor(o) : SecondaryColor(o));
            ImGui::TextUnformatted(Safe(s.label));
            ImGui::PopStyleColor();
            if (total > 0) {
                char share[16];
                std::snprintf(share, sizeof(share), "%.0f%%", s.value / total * 100.f);
                ImGui::SameLine(0, style.ItemSpacing.x * 2.f);
                ImGui::TextUnformatted(share);
            }
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return clicked;
}

void Sparkline(const char* id, const SparklineView& view, SparklineOptions layout, ComponentOptions o) {
    const auto& style = ImGui::GetStyle();
    const float width = std::max(24.f, layout.size.x > 0 ? layout.size.x : ImGui::GetContentRegionAvail().x);
    const float height = std::max(16.f, layout.size.y > 0 ? layout.size.y : ImGui::GetFrameHeight() * 1.6f);
    const bool header = *Safe(view.label) || *Safe(view.valueText);
    ImGui::PushID(id);
    ImGui::BeginGroup();
    auto* draw = ImGui::GetWindowDrawList();
    if (header) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy({width, ImGui::GetTextLineHeight()});
        if (*Safe(view.label))
            draw->AddText(p, ImGui::GetColorU32(SecondaryColor(o)), view.label);
        if (*Safe(view.valueText)) {
            const float w = ImGui::CalcTextSize(view.valueText).x;
            draw->AddText({p.x + width - w, p.y}, ImGui::GetColorU32(TextColor(o)), view.valueText);
        }
    }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##spark", {width, height});
    const bool hovered = ImGui::IsItemHovered();
    const auto values = view.values;
    const ImVec4 color = KindColor(layout.kind, o);
    if (values.size() >= 2) {
        float low = std::numeric_limits<float>::max(), high = std::numeric_limits<float>::lowest();
        for (float v : values)
            if (Finite(v)) {
                low = std::min(low, v);
                high = std::max(high, v);
            }
        if (low > high) {
            low = 0;
            high = 1;
        }
        if (high - low < 1e-6f) {
            low -= .5f;
            high += .5f;
        }
        const float inset = 4.f, innerHeight = height - inset * 2.f;
        // Series longer than the point buffer are sampled evenly so the line always spans the full width.
        constexpr std::size_t MaxPoints = 256;
        const std::size_t shown = std::min(values.size(), MaxPoints);
        const float step = (width - inset * 2.f) / static_cast<float>(shown - 1);
        auto point = [&](std::size_t i) {
            const std::size_t source = shown == values.size() ? i : i * (values.size() - 1) / (shown - 1);
            const float v = Finite(values[source]) ? values[source] : low;
            return ImVec2{origin.x + inset + step * static_cast<float>(i),
                          origin.y + inset + innerHeight * (1.f - (v - low) / (high - low))};
        };
        if (layout.area) {
            const ImU32 fill = ImGui::GetColorU32(detail::WithAlpha(color, .14f));
            for (std::size_t i = 0; i + 1 < values.size(); ++i) {
                const ImVec2 a = point(i), b = point(i + 1);
                draw->AddQuadFilled(a, b, {b.x, origin.y + height - inset + 1}, {a.x, origin.y + height - inset + 1}, fill);
            }
        }
        draw->PushClipRect(origin, {origin.x + width, origin.y + height}, true);
        std::array<ImVec2, MaxPoints> points{};
        const std::size_t count = shown;
        for (std::size_t i = 0; i < count; ++i)
            points[i] = point(i);
        draw->AddPolyline(points.data(), static_cast<int>(count), ImGui::GetColorU32(color), ImDrawFlags_RoundCornersAll, 2.f);
        draw->PopClipRect();
        const ImVec2 last = point(count - 1);
        draw->AddCircleFilled(last, 5.f, ImGui::GetColorU32(detail::WithAlpha(color, .22f)));
        draw->AddCircleFilled(last, 2.8f, ImGui::GetColorU32(color));
        if (hovered) {
            const float mouse = ImGui::GetIO().MousePos.x;
            const std::size_t at = static_cast<std::size_t>(std::clamp((mouse - origin.x - inset) / step + .5f, 0.f, static_cast<float>(count - 1)));
            const ImVec2 p = point(at);
            draw->AddLine({p.x, origin.y}, {p.x, origin.y + height}, ImGui::GetColorU32(detail::WithAlpha(TextColor(o), .25f)));
            draw->AddCircleFilled(p, 3.5f, ImGui::GetColorU32(color));
            draw->AddCircle(p, 3.5f, ImGui::GetColorU32(ImGuiCol_WindowBg), 16, 1.5f);
            ImGui::SetTooltip("%.4g", values[shown == values.size() ? at : at * (values.size() - 1) / (shown - 1)]);
        }
    }
    (void)style;
    Describe(o, accessibility::SemanticRole::Group, Safe(view.label), Safe(view.valueText));
    ImGui::EndGroup();
    ImGui::PopID();
}

int BarChart(const char* id, const BarChartView& view, BarChartOptions layout, ComponentOptions o) {
    const float width = std::max(48.f, layout.size.x > 0 ? layout.size.x : ImGui::GetContentRegionAvail().x);
    const bool vertical = layout.orientation == Orientation::Vertical;
    const float height = std::max(48.f, layout.size.y > 0 ? layout.size.y : ImGui::GetFrameHeight() * (vertical ? 8.f : 1.6f * static_cast<float>(std::max<std::size_t>(1, view.bars.size()))));
    float maximum = layout.maximum;
    if (!(maximum > 0)) {
        for (const auto& b : view.bars)
            if (Finite(b.value))
                maximum = std::max(maximum, b.value);
        // Round the axis up to a friendly step so bars never touch the top.
        if (maximum > 0) {
            const float magnitude = std::pow(10.f, std::floor(std::log10(maximum)));
            maximum = std::ceil(maximum / magnitude * 1.1f * 2.f) / 2.f * magnitude;
        }
    }
    if (!(maximum > 0))
        maximum = 1;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##bars", {width, height});
    const bool hoveredItem = ImGui::IsItemHovered(), clickedItem = ImGui::IsItemClicked();
    auto* draw = ImGui::GetWindowDrawList();
    const std::size_t count = view.bars.size();
    int hovered = -1;
    const float lineHeight = ImGui::GetTextLineHeight();
    const ImU32 grid = ImGui::GetColorU32(detail::WithAlpha(TextColor(o), .09f));
    if (count) {
        if (vertical) {
            const float labelBand = lineHeight + 6.f, top = layout.values ? lineHeight + 4.f : 4.f;
            const ImVec2 plotMin{origin.x, origin.y + top}, plotMax{origin.x + width, origin.y + height - labelBand};
            if (layout.grid)
                for (int i = 0; i <= 3; ++i) {
                    const float y = plotMax.y - (plotMax.y - plotMin.y) * static_cast<float>(i) / 3.f;
                    draw->AddLine({plotMin.x, y}, {plotMax.x, y}, grid);
                }
            const float slot = width / static_cast<float>(count), barWidth = std::min(slot * .62f, 56.f);
            for (std::size_t i = 0; i < count; ++i) {
                const auto& b = view.bars[i];
                const float v = Finite(b.value) ? std::clamp(b.value, 0.f, maximum) : 0.f;
                const float cx = origin.x + slot * (static_cast<float>(i) + .5f);
                const float barTop = plotMax.y - (plotMax.y - plotMin.y) * v / maximum;
                const ImVec2 a{cx - barWidth * .5f, barTop}, c{cx + barWidth * .5f, plotMax.y};
                if (hoveredItem) {
                    const ImVec2 m = ImGui::GetIO().MousePos;
                    if (m.x >= cx - slot * .5f && m.x < cx + slot * .5f)
                        hovered = static_cast<int>(i);
                }
                ImVec4 color = b.color.w > 0 ? b.color : ChartColor(o.theme, 0); // One series: one colour.
                if (hovered >= 0 && hovered != static_cast<int>(i))
                    color = detail::WithAlpha(color, .55f);
                if (c.y - a.y > 0)
                    draw->AddRectFilled(a, c, ImGui::GetColorU32(color), std::min(barWidth * .3f, 6.f), ImDrawFlags_RoundCornersTop);
                if (layout.values && Finite(b.value)) {
                    char text[32];
                    std::snprintf(text, sizeof(text), detail::SafeFormat(layout.format, "fge", "%.0f"), b.value);
                    const float w = ImGui::CalcTextSize(text).x;
                    draw->AddText({cx - w * .5f, std::max(origin.y, barTop - lineHeight - 2.f)}, ImGui::GetColorU32(TextColor(o)), text);
                }
                const float labelWidth = ImGui::CalcTextSize(Safe(b.label)).x;
                if (labelWidth <= slot - 4.f)
                    draw->AddText({cx - labelWidth * .5f, plotMax.y + 4.f}, ImGui::GetColorU32(SecondaryColor(o)), Safe(b.label));
            }
        } else {
            float labelWidth = 0;
            for (const auto& b : view.bars)
                labelWidth = std::max(labelWidth, ImGui::CalcTextSize(Safe(b.label)).x);
            labelWidth = std::min(labelWidth + 10.f, width * .4f);
            const float valueBand = layout.values ? ImGui::CalcTextSize("0000000").x : 4.f;
            const ImVec2 plotMin{origin.x + labelWidth, origin.y}, plotMax{origin.x + width - valueBand, origin.y + height};
            const float slot = height / static_cast<float>(count), barHeight = std::min(slot * .56f, 28.f);
            if (layout.grid)
                for (int i = 0; i <= 3; ++i) {
                    const float x = plotMin.x + (plotMax.x - plotMin.x) * static_cast<float>(i) / 3.f;
                    draw->AddLine({x, plotMin.y}, {x, plotMax.y}, grid);
                }
            for (std::size_t i = 0; i < count; ++i) {
                const auto& b = view.bars[i];
                const float v = Finite(b.value) ? std::clamp(b.value, 0.f, maximum) : 0.f;
                const float cy = origin.y + slot * (static_cast<float>(i) + .5f);
                const float barEnd = plotMin.x + (plotMax.x - plotMin.x) * v / maximum;
                if (hoveredItem) {
                    const ImVec2 m = ImGui::GetIO().MousePos;
                    if (m.y >= cy - slot * .5f && m.y < cy + slot * .5f)
                        hovered = static_cast<int>(i);
                }
                ImVec4 color = b.color.w > 0 ? b.color : ChartColor(o.theme, 0);
                if (hovered >= 0 && hovered != static_cast<int>(i))
                    color = detail::WithAlpha(color, .55f);
                if (barEnd > plotMin.x)
                    draw->AddRectFilled({plotMin.x, cy - barHeight * .5f}, {barEnd, cy + barHeight * .5f}, ImGui::GetColorU32(color),
                                        std::min(barHeight * .3f, 6.f), ImDrawFlags_RoundCornersRight);
                draw->PushClipRect({origin.x, cy - slot * .5f}, {plotMin.x - 4.f, cy + slot * .5f}, true);
                draw->AddText({origin.x, cy - lineHeight * .5f}, ImGui::GetColorU32(SecondaryColor(o)), Safe(b.label));
                draw->PopClipRect();
                if (layout.values && Finite(b.value)) {
                    char text[32];
                    std::snprintf(text, sizeof(text), detail::SafeFormat(layout.format, "fge", "%.0f"), b.value);
                    draw->AddText({barEnd + 6.f, cy - lineHeight * .5f}, ImGui::GetColorU32(TextColor(o)), text);
                }
            }
        }
    }
    int clicked = -1;
    if (hovered >= 0) {
        const auto& b = view.bars[static_cast<std::size_t>(hovered)];
        char text[32];
        std::snprintf(text, sizeof(text), detail::SafeFormat(layout.format, "fge", "%.0f"), b.value);
        ImGui::SetTooltip("%s: %s", Safe(b.label), text);
        if (clickedItem)
            clicked = hovered;
    }
    Describe(o, accessibility::SemanticRole::Group, Safe(id), "");
    ImGui::EndGroup();
    ImGui::PopID();
    return clicked;
}

void Gauge(const char* id, const GaugeView& view, GaugeOptions layout, ComponentOptions o) {
    const float diameter = std::max(48.f, layout.diameter > 0 ? layout.diameter : ImGui::GetFrameHeight() * 5.f);
    const float thickness = std::clamp(layout.thickness > 0 ? layout.thickness : diameter * .1f, 3.f, diameter * .25f);
    const bool available = Finite(view.fraction) && view.fraction >= 0.f;
    float fraction = available ? std::clamp(view.fraction, 0.f, 1.f) : 0.f;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy({diameter, diameter * .86f});
    const ImVec2 c{origin.x + diameter * .5f, origin.y + diameter * .5f};
    const float radius = diameter * .5f - thickness * .5f - 2.f;
    auto* draw = ImGui::GetWindowDrawList();
    // Open ring: 240 degrees, gap at the bottom.
    const float start = Pi * (150.f / 180.f), sweep = Pi * (240.f / 180.f);
    if (available && MotionAllowed(o) && o.animation) {
        const auto m = o.theme ? o.theme->motion : Motion{};
        fraction = o.animation->Update(ImGui::GetID("gauge-value"), fraction, ImGui::GetIO().DeltaTime,
                                       std::max(m.controlSeconds * 2.f, .05f), ImGui::GetFrameCount(), true, m.easing);
    }
    detail::DrawRingArc(draw, c, radius, thickness, start, start + sweep, ImGui::GetColorU32(detail::WithAlpha(TextColor(o), .11f)));
    FeedbackKind kind = view.kind;
    for (const auto& zone : view.zones)
        if (view.fraction <= zone.upTo + 1e-6f) {
            kind = zone.kind;
            break;
        } else
            kind = zone.kind;
    const ImVec4 color = available ? KindColor(kind, o) : (o.theme ? o.theme->semantic.textDisabled : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    if (available && fraction > 0.f)
        detail::DrawRingArc(draw, c, radius, thickness, start, start + sweep * fraction, ImGui::GetColorU32(color));
    const char* value = *Safe(view.value) ? view.value : (available ? "" : "\xE2\x80\x94");
    const ImVec2 valueSize = ImGui::CalcTextSize(value);
    const float inner = std::max(1.f, radius * 1.3f);
    const float scale = std::min(1.7f, inner / std::max(1.f, valueSize.x));
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, {c.x - valueSize.x * scale * .5f, c.y - valueSize.y * scale * .55f},
                  ImGui::GetColorU32(TextColor(o)), value);
    if (*Safe(view.label)) {
        const float w = ImGui::CalcTextSize(view.label).x;
        ImGui::SetCursorScreenPos({c.x - w * .5f, c.y + valueSize.y * scale * .5f + 2.f});
        ImGui::PushStyleColor(ImGuiCol_Text, SecondaryColor(o));
        ImGui::TextUnformatted(view.label);
        ImGui::PopStyleColor();
    }
    ImGui::SetCursorScreenPos({origin.x, origin.y + diameter * .86f});
    ImGui::Dummy({0, 0});
    if (o.accessibility) {
        accessibility::SemanticNode node;
        node.id = ImGui::GetID("gauge");
        node.parent = o.parent;
        node.name = Safe(view.label);
        node.value = value;
        node.role = accessibility::SemanticRole::Progress;
        node.state.readOnly = true;
        node.state.invalid = !available;
        if (available) {
            node.numericValue = std::clamp(view.fraction, 0.f, 1.f);
            node.minimumValue = 0;
            node.maximumValue = 1;
        }
        node.minimum = origin;
        node.maximum = {origin.x + diameter, origin.y + diameter * .86f};
        o.accessibility->Add(node);
    }
    ImGui::EndGroup();
    ImGui::PopID();
}
} // namespace imkit
