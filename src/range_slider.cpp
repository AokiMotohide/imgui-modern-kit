#include <imkit/range_slider.h>
#include "component_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace imkit {
namespace {
enum class Part { None = 0, Low = 1, High = 2, Span = 3 };

double Clamp(double v, double lo, double hi) { return std::min(std::max(v, lo), hi); }

// Formats a value with the host format; integer ranges print whole numbers.
void Format(char (&buffer)[48], double value, const char* format, bool integer) {
    if (integer)
        std::snprintf(buffer, sizeof(buffer), detail::SafeFormat(format, "d", "%d"), static_cast<int>(std::lround(value)));
    else
        std::snprintf(buffer, sizeof(buffer), detail::SafeFormat(format, "fge", "%.1f"), value);
}

// Value model: snapping, the minimum gap and which handle moves. Pure arithmetic so it stays easy to reason about.
struct Model {
    double minimum, maximum, low, high, minGap, step;
    bool integer;
    double Unit() const { return integer ? std::max(1., std::round(step)) : step; }
    double Snap(double v) const {
        if (const double unit = Unit(); unit > 0)
            v = minimum + std::round((v - minimum) / unit) * unit;
        return Clamp(v, minimum, maximum);
    }
    double Gap() const { return std::min(std::max(integer ? 1. : 0., minGap), maximum - minimum); }
    double Ratio(double v) const { return maximum > minimum ? Clamp((v - minimum) / (maximum - minimum), 0, 1) : 0; }
    void SetLow(double v) { low = std::max(minimum, std::min(Snap(v), high - Gap())); }
    void SetHigh(double v) { high = std::min(maximum, std::max(Snap(v), low + Gap())); }
    // Moves both handles, keeping the span width.
    void Move(double delta) {
        const double width = high - low;
        low = Clamp(Snap(low + delta), minimum, maximum - width);
        high = low + width;
    }
};

// Announces one handle to the host's accessibility frame and returns -1, 0 or +1 for a requested step.
int AnnotateHandle(const ComponentOptions& o, const char* label, const char* which, double value, const Model& m,
                   bool disabled) {
    using namespace accessibility;
    if (!o.accessibility)
        return 0;
    const ImGuiID id = ImGui::GetItemID();
    if (o.accessibility->Take(id, SemanticAction::Focus) && !disabled) {
        ImGui::SetKeyboardFocusHere(-1);
        ImGui::SetNavCursorVisible(true);
    }
    int direction = 0;
    if (!disabled) {
        if (o.accessibility->Take(id, SemanticAction::Increment))
            direction = 1;
        else if (o.accessibility->Take(id, SemanticAction::Decrement))
            direction = -1;
    }
    const double unit = m.Unit() > 0 ? m.Unit() : (m.maximum - m.minimum) / 100.;
    SemanticNode node;
    node.id = id;
    node.parent = o.parent;
    node.role = SemanticRole::Group;
    node.name = label;
    if (const auto end = node.name.find("##"); end != std::string_view::npos)
        node.name = node.name.substr(0, end);
    node.description = which;
    node.state.disabled = disabled;
    node.numericValue = value;
    node.minimumValue = m.minimum;
    node.maximumValue = m.maximum;
    node.smallChange = unit;
    node.largeChange = unit * 10;
    node.actions = SemanticAction::Increment | SemanticAction::Decrement | SemanticAction::Focus;
    AnnotateLastItem(*o.accessibility, node);
    return direction;
}

bool RangeCore(const char* label, double* lowValue, double* highValue, double minimum, double maximum,
               const char* format, bool integer, const RangeSliderOptions& layout, const ComponentOptions& options) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum))
        return false;
    if (maximum < minimum)
        std::swap(minimum, maximum);
    Model m{minimum, maximum, Clamp(std::isfinite(*lowValue) ? *lowValue : minimum, minimum, maximum),
            Clamp(std::isfinite(*highValue) ? *highValue : maximum, minimum, maximum), layout.minGap, layout.step, integer};
    if (m.low > m.high)
        std::swap(m.low, m.high);

    const auto& style = ImGui::GetStyle();
    auto* draw = ImGui::GetWindowDrawList();
    const char* hidden = std::strstr(label, "##");
    const float height = ImGui::GetFrameHeight();
    const float full = layout.width > 0 ? layout.width : ImGui::CalcItemWidth();
    const float gap = style.ItemInnerSpacing.x;
    const float radius = std::max(5.f, height * .3f);
    float labelWidth = 0;
    if (layout.valueLabels) {
        char text[48];
        Format(text, minimum, format, integer);
        labelWidth = ImGui::CalcTextSize(text).x;
        Format(text, maximum, format, integer);
        labelWidth = std::max(labelWidth, ImGui::CalcTextSize(text).x);
        if (full < (labelWidth + gap) * 2 + radius * 6)
            labelWidth = 0; // Too narrow for end labels: keep the rail usable.
    }
    const float inner = labelWidth > 0 ? labelWidth + gap : 0;

    ImGui::PushID(label);
    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float railMin = origin.x + inner + radius, railMax = origin.x + full - inner - radius;
    const float pixels = std::max(1.f, railMax - railMin);
    const double perPixel = (maximum - minimum) / pixels;
    auto xOf = [&](double v) { return railMin + static_cast<float>(m.Ratio(v)) * (railMax - railMin); };
    auto valueAt = [&](float x) { return minimum + static_cast<double>(x - railMin) * perPixel; };

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID kPart = ImGui::GetID("part"), kStart = ImGui::GetID("start"), kLow = ImGui::GetID("low"),
                  kHigh = ImGui::GetID("high");

    // Hit areas, back to front: the rail, the filled span, then both handles. Each earlier item allows overlap so
    // the later ones can take the hover.
    const float lowX = xOf(m.low), highX = xOf(m.high);
    ImGui::SetCursorScreenPos({railMin - radius, origin.y});
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##rail", {railMax - railMin + 2 * radius, height});
    const bool railHeld = ImGui::IsItemActive(), railStarted = ImGui::IsItemActivated();
    ImGui::SetCursorScreenPos({lowX, origin.y});
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##span", {std::max(1.f, highX - lowX), height});
    const bool spanHeld = ImGui::IsItemActive(), spanStarted = ImGui::IsItemActivated(), spanHovered = ImGui::IsItemHovered();
    const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    ImGui::SetCursorScreenPos({lowX - radius, origin.y});
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##low", {radius * 2, height});
    const bool lowHeld = ImGui::IsItemActive(), lowStarted = ImGui::IsItemActivated(), lowHovered = ImGui::IsItemHovered(),
               lowFocused = ImGui::IsItemFocused();
    const char* minimumName = options.locale ? options.locale->Text("range.minimum", "Minimum") : "Minimum";
    const char* maximumName = options.locale ? options.locale->Text("range.maximum", "Maximum") : "Maximum";
    const int lowRequest = AnnotateHandle(options, label, minimumName, m.low, m, disabled);
    ImGui::SetCursorScreenPos({highX - radius, origin.y});
    ImGui::InvisibleButton("##high", {radius * 2, height});
    const bool highHeld = ImGui::IsItemActive(), highStarted = ImGui::IsItemActivated(), highHovered = ImGui::IsItemHovered(),
               highFocused = ImGui::IsItemFocused();
    const int highRequest = AnnotateHandle(options, label, maximumName, m.high, m, disabled);

    const double before[2] = {m.low, m.high};
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    Part part = static_cast<Part>(storage->GetInt(kPart, 0));
    if (lowStarted || highStarted || spanStarted || railStarted) {
        if (lowStarted)
            part = Part::Low;
        else if (highStarted)
            part = m.low >= m.high ? Part::None : Part::High; // Stacked handles: the drag direction decides.
        else if (spanStarted)
            part = Part::Span;
        else
            part = mouse.x < (lowX + highX) * .5f || (lowX == highX && mouse.x < lowX) ? Part::Low : Part::High;
        storage->SetInt(kPart, static_cast<int>(part));
        storage->SetFloat(kStart, mouse.x);
        storage->SetFloat(kLow, static_cast<float>(m.low));
        storage->SetFloat(kHigh, static_cast<float>(m.high));
        if (railStarted && !disabled)
            (part == Part::Low ? m.SetLow(valueAt(mouse.x)) : m.SetHigh(valueAt(mouse.x)));
    }
    const bool held = lowHeld || highHeld || spanHeld || railHeld;
    if (held && !disabled) {
        const double startLow = storage->GetFloat(kLow, static_cast<float>(m.low));
        const double startHigh = storage->GetFloat(kHigh, static_cast<float>(m.high));
        const double delta = static_cast<double>(mouse.x - storage->GetFloat(kStart, mouse.x)) * perPixel;
        if (part == Part::None && delta != 0) {
            part = delta < 0 ? Part::Low : Part::High;
            storage->SetInt(kPart, static_cast<int>(part));
        }
        const bool rail = railHeld && !lowHeld && !highHeld && !spanHeld;
        if (part == Part::Low)
            m.SetLow(rail ? valueAt(mouse.x) : startLow + delta);
        else if (part == Part::High)
            m.SetHigh(rail ? valueAt(mouse.x) : startHigh + delta);
        else if (part == Part::Span) {
            m.low = startLow;
            m.high = startHigh;
            m.Move(delta);
        }
    }
    if (!disabled) {
        const double unit = m.Unit() > 0 ? m.Unit() : (maximum - minimum) / 100.;
        const double times = ImGui::GetIO().KeyShift ? 10. : 1.;
        auto keyStep = [&](bool focused) {
            if (!focused)
                return 0;
            return (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd) ? 1 : 0) -
                   (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract) ? 1 : 0);
        };
        if (const int s = keyStep(lowFocused) + lowRequest)
            m.SetLow(m.low + s * unit * (lowRequest ? 1. : times));
        if (const int s = keyStep(highFocused) + highRequest)
            m.SetHigh(m.high + s * unit * (highRequest ? 1. : times));
    }
    const bool changed = m.low != before[0] || m.high != before[1];
    *lowValue = m.low;
    *highValue = m.high;

    // Drawing. The handle that moved last keeps its pressed look until release.
    const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
    const float cy = origin.y + height * .5f;
    const float railH = std::max(4.f, ImGui::GetFontSize() * .3f);
    const float x0 = railMin - radius * .5f, x1 = railMax + radius * .5f;
    const float finalLow = xOf(m.low), finalHigh = xOf(m.high);
    draw->AddRectFilled({x0, cy - railH * .5f}, {x1, cy + railH * .5f}, ImGui::GetColorU32(ImGuiCol_FrameBg), railH);
    if (style.FrameBorderSize > 0)
        draw->AddRect({x0, cy - railH * .5f}, {x1, cy + railH * .5f}, ImGui::GetColorU32(ImGuiCol_Border), railH);
    if (layout.ticks > 1) {
        const ImU32 tick = ImGui::GetColorU32(ImGuiCol_TextDisabled, .6f);
        for (int i = 0; i < layout.ticks; ++i) {
            const float x = railMin + (railMax - railMin) * static_cast<float>(i) / static_cast<float>(layout.ticks - 1);
            draw->AddLine({x, cy + railH * .5f + 3}, {x, cy + railH * .5f + 3 + railH}, tick, 1.f);
        }
    }
    ImVec4 spanColor = accent;
    spanColor.w *= spanHeld ? .92f : spanHovered ? .80f : .68f;
    draw->AddRectFilled({finalLow, cy - railH * .5f}, {finalHigh, cy + railH * .5f}, ImGui::GetColorU32(spanColor), railH);

    const ImU32 hole = ImGui::GetColorU32(ImGuiCol_WindowBg);
    auto thumb = [&](float x, bool hovered, bool active, bool focused) {
        const float r = radius * (active ? 1.1f : hovered ? 1.05f : 1.f);
        if (hovered || active || focused) {
            ImVec4 halo = accent;
            halo.w *= focused ? .42f : active ? .36f : .24f;
            draw->AddCircleFilled({x, cy}, r + std::max(3.f, radius * .38f), ImGui::GetColorU32(halo), 28);
        }
        draw->AddCircleFilled({x, cy + 1.f}, r, ImGui::GetColorU32(ImVec4(0, 0, 0, .20f)), 28);
        draw->AddCircleFilled({x, cy}, r, ImGui::GetColorU32(accent), 28);
        draw->AddCircleFilled({x, cy}, r - std::max(2.5f, radius * .3f), active ? ImGui::GetColorU32(accent) : hole, 28);
    };
    thumb(finalLow, lowHovered, lowHeld || (railHeld && part == Part::Low), lowFocused);
    thumb(finalHigh, highHovered, highHeld || (railHeld && part == Part::High), highFocused);

    if (labelWidth > 0) {
        char lowText[48], highText[48];
        Format(lowText, m.low, format, integer);
        Format(highText, m.high, format, integer);
        const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
        const float ty = cy - ImGui::GetFontSize() * .5f;
        draw->AddText({origin.x + labelWidth - ImGui::CalcTextSize(lowText).x, ty}, text, lowText);
        draw->AddText({origin.x + full - labelWidth, ty}, text, highText);
    }

    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({full, height});
    if (hidden != label && *label) {
        ImGui::SameLine(0, gap);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label, hidden);
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
} // namespace

bool RangeSlider(const char* label, float* low, float* high, float minimum, float maximum, const char* format,
                 RangeSliderOptions layout, ComponentOptions options) {
    if (!label || !low || !high)
        return false;
    double l = *low, h = *high;
    const bool changed = RangeCore(label, &l, &h, minimum, maximum, format, false, layout, options);
    *low = static_cast<float>(l);
    *high = static_cast<float>(h);
    return changed;
}

bool RangeSliderInt(const char* label, int* low, int* high, int minimum, int maximum, const char* format,
                    RangeSliderOptions layout, ComponentOptions options) {
    if (!label || !low || !high)
        return false;
    double l = *low, h = *high;
    const bool changed = RangeCore(label, &l, &h, minimum, maximum, format, true, layout, options);
    *low = static_cast<int>(std::lround(l));
    *high = static_cast<int>(std::lround(h));
    return changed;
}
} // namespace imkit
