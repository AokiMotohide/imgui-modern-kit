#include <imkit/show/common.h>
#include "internal.h"
#include <imkit/charts.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string_view>

namespace imkit::show {
ImVec4 ShowColors::layer(std::size_t index) const { return ChartColor(theme, index); }

ShowColors MakeShowColors(const ComponentOptions& options) {
    ShowColors c;
    c.theme = options.theme;
    if (options.theme) {
        const auto& s = options.theme->semantic;
        c.surface = s.surface;
        c.raised = s.surfaceRaised;
        c.input = s.control.rest;
        c.text = s.text;
        c.muted = s.textSecondary;
        c.border = s.border;
        c.accent = s.accent;
        c.onAccent = s.onAccent;
        c.focus = s.control.focused;
        c.success = s.success;
        c.warning = s.warning;
        c.error = s.error;
    } else {
        c.surface = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        c.raised = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
        c.input = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        c.text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        c.muted = c.text;
        c.muted.w *= .65f;
        c.border = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        c.accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
        c.onAccent = {.06f, .07f, .09f, 1.f};
        c.focus = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive);
        c.success = {.30f, .80f, .55f, 1.f};
        c.warning = {.95f, .72f, .30f, 1.f};
        c.error = {.95f, .40f, .42f, 1.f};
    }
    c.busA = c.accent;
    c.busB = ChartColor(options.theme, 3);
    c.running = c.accent;
    c.queued = c.warning;
    c.armed = c.warning;
    c.broken = c.error;
    c.done = c.success;
    return c;
}

namespace impl {
bool PillButton(const char* id, const char* text, const ShowColors& c, const ComponentOptions& o, ImVec4 ink, ImVec4 tone, ImVec2 size,
                bool dashed) {
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    const ImVec2 ts = ImGui::CalcTextSize(text, nullptr, true);
    if (size.x <= 0)
        size.x = ts.x + u * .9f;
    if (size.y <= 0)
        size.y = u;
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    const float round = size.y * .4f;
    ImGui::InvisibleButton("##pill", size);
    const bool hovered = ImGui::IsItemHovered(), held = ImGui::IsItemActive(), clicked = ImGui::IsItemClicked(), focused = ImGui::IsItemFocused();
    const ImVec4 fill = tone.w > 0 ? Mix(c.input, tone, held ? .45f : hovered ? .28f : .2f) : Mix(c.input, c.text, held ? .14f : hovered ? .09f : .03f);
    SkinFrame frame;
    frame.draw = draw;
    frame.min = a;
    frame.max = b;
    frame.rounding = round;
    frame.part = SkinPart::Button;
    frame.hovered = hovered;
    frame.active = held;
    frame.focused = focused;
    frame.disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    frame.hover = hovered ? 1.f : 0.f;
    if (!detail::TrySkin(o.theme, frame)) {
        draw->AddRectFilled(a, b, U32(fill), round);
        if (dashed)
            detail::DrawDashedRect(draw, a, b, round, U32(c.muted, .7f), 1.f, 3.f);
        else if (detail::IsOutlined(ImGui::GetStyle()))
            detail::DrawOutline(draw, a, b, round);
        else
            draw->AddRect(a, b, U32(tone.w > 0 ? Mix(c.border, tone, .6f) : c.border, .8f), round, 0, detail::FrameLineWidth(1.f));
    }
    DrawTextCentered(draw, a, b, U32(ink), text);
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), round + 2.f, 0, 2.f);
    SemanticItem(o, text, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
    ImGui::PopID();
    return clicked;
}

float Clamp01(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : 0.f; }

ImVec4 InkFor(const ShowColors&, ImVec4 background) {
    const ImVec4 dark{.06f, .07f, .09f, 1.f}, light{.97f, .98f, 1.f, 1.f};
    return ContrastRatio(dark, background) >= ContrastRatio(light, background) ? dark : light;
}

float ControlRound(const ComponentOptions& o) { return o.theme ? o.theme->radius.control + 2.f : Unit() * .4f; }

ImVec4 Recessed(const ShowColors& c, float depth) {
    const float lum = .3f * c.surface.x + .59f * c.surface.y + .11f * c.surface.z;
    return lum > .5f ? Mix(c.surface, c.text, .05f + .14f * depth) : Mix(c.surface, ImVec4{0, 0, 0, 1}, .2f + .5f * depth);
}

const char* FrameText(const char* format, ...) {
    static char ring[4096][64];
    static unsigned next = 0;
    char* slot = ring[next++ % 4096];
    va_list args;
    va_start(args, format);
    std::vsnprintf(slot, 64, format, args);
    va_end(args);
    return slot;
}

const char* FormatValue(const char* format, const char* unit, double value) {
    char text[40];
    std::snprintf(text, sizeof(text), SafeFormat(format, "fge", "%.1f"), value);
    return FrameText(unit && *unit ? "%s %s" : "%s", text, unit);
}

void ReadoutItem(ImVec2 size) {
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    ImGui::InvisibleButton("##readout", size);
    ImGui::PopItemFlag();
}

void AnnotateText(const ComponentOptions& o, const char* name, const char* value, accessibility::SemanticRole role, bool disabled) {
    if (!o.accessibility)
        return;
    accessibility::SemanticNode node;
    node.id = ImGui::GetItemID();
    node.parent = o.parent;
    node.role = role;
    node.name = name;
    if (const auto end = node.name.find("##"); end != std::string_view::npos)
        node.name = node.name.substr(0, end);
    node.value = value;
    node.state.disabled = disabled;
    node.state.readOnly = true;
    accessibility::AnnotateLastItem(*o.accessibility, node);
}

EditFrame::EditFrame(float* value, bool suppressStart)
    : value_(value), before_(*value), storage_(ImGui::GetStateStorage()), key_(ImGui::GetID("##edit.original")),
      suppressKey_(ImGui::GetID("##edit.suppressed")) {
    active_ = ImGui::IsItemActive();
    deactivated_ = ImGui::IsItemDeactivated();
    if (ImGui::IsItemActivated()) {
        storage_->SetBool(suppressKey_, suppressStart);
        if (!suppressStart) {
            storage_->SetFloat(key_, *value);
            edit_.started = true;
        }
    }
    edit_.original = storage_->GetFloat(key_, *value);
    if (active_ && storage_->GetBool(suppressKey_))
        active_ = false;
}

bool EditFrame::CancelOnEscape() {
    if (!active_ || !ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        return false;
    *value_ = static_cast<float>(edit_.original);
    edit_.cancelled = true;
    Suppress();
    return true;
}

void EditFrame::Suppress() {
    storage_->SetBool(suppressKey_, true);
    active_ = false;
}

void EditFrame::Instant(float newValue) {
    if (newValue == *value_)
        return;
    edit_.original = *value_;
    *value_ = newValue;
    edit_.started = edit_.changed = edit_.finished = true;
    instant_ = true;
}

ValueEdit EditFrame::Done() {
    if (!instant_ && *value_ != before_)
        edit_.changed = true;
    if (!instant_ && deactivated_) {
        if (storage_->GetBool(suppressKey_))
            storage_->SetBool(suppressKey_, false);
        else
            edit_.finished = true;
    }
    edit_.value = *value_;
    return edit_;
}

float SnapValue(float v, float minimum, float maximum, float step) {
    if (step > 0)
        v = minimum + std::round((v - minimum) / step) * step;
    return std::clamp(v, minimum, maximum);
}

bool NumericEntry(const char* popupId, bool open, float* value, float minimum, float maximum, const char* format) {
    if (open)
        ImGui::OpenPopup(popupId);
    bool committed = false;
    if (ImGui::BeginPopup(popupId)) {
        static float draft = 0;
        if (open || ImGui::IsWindowAppearing()) {
            draft = *value;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 4.5f);
        if (ImGui::InputFloat("##value", &draft, 0, 0, SafeFormat(format, "fge", "%.2f"),
                              ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            *value = std::clamp(std::isfinite(draft) ? draft : *value, minimum, maximum);
            committed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return committed;
}

void GlyphPlay(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddTriangleFilled({c.x - s * .32f, c.y - s * .45f}, {c.x - s * .32f, c.y + s * .45f}, {c.x + s * .48f, c.y}, col);
}

void GlyphStop(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddRectFilled({c.x - s * .38f, c.y - s * .38f}, {c.x + s * .38f, c.y + s * .38f}, col, s * .08f);
}

void GlyphPause(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddRectFilled({c.x - s * .36f, c.y - s * .42f}, {c.x - s * .12f, c.y + s * .42f}, col, s * .05f);
    d->AddRectFilled({c.x + s * .12f, c.y - s * .42f}, {c.x + s * .36f, c.y + s * .42f}, col, s * .05f);
}

void GlyphLock(ImDrawList* d, ImVec2 c, float s, ImU32 col, bool locked) {
    const float w = s * .56f, h = s * .40f;
    d->AddRectFilled({c.x - w * .5f, c.y - h * .05f}, {c.x + w * .5f, c.y + h * 1.05f}, col, s * .08f);
    const float r = w * .36f;
    const ImVec2 ctr{c.x + (locked ? 0.f : r * .8f), c.y - h * .05f};
    d->PathArcTo(ctr, r, 3.14159265f, 6.2831853f, 10);
    d->PathStroke(col, 0, std::max(1.4f, s * .09f));
    if (locked) {
        d->AddLine({c.x - r, ctr.y}, {c.x - r, c.y - h * .05f}, col, std::max(1.4f, s * .09f));
        d->AddLine({c.x + r, ctr.y}, {c.x + r, c.y - h * .05f}, col, std::max(1.4f, s * .09f));
    }
}

void GlyphWarning(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddTriangleFilled({c.x, c.y - s * .46f}, {c.x - s * .5f, c.y + s * .4f}, {c.x + s * .5f, c.y + s * .4f}, col);
}

void DrawTextCentered(ImDrawList* d, ImVec2 a, ImVec2 b, ImU32 col, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    d->AddText({a.x + (b.x - a.x - size.x) * .5f, a.y + (b.y - a.y - size.y) * .5f}, col, text);
}

void DrawTextFit(ImDrawList* d, ImVec2 pos, float width, ImU32 col, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    if (size.x <= width) {
        d->AddText(pos, col, text);
        return;
    }
    const float dots = ImGui::CalcTextSize("..").x;
    d->PushClipRect(pos, {pos.x + std::max(0.f, width - dots), pos.y + size.y + 2}, true);
    d->AddText(pos, col, text);
    d->PopClipRect();
    d->AddText({pos.x + std::max(0.f, width - dots), pos.y}, col, "..");
}
} // namespace impl
} // namespace imkit::show
