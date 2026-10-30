#include <imkit/show/common.h>
#include "internal.h"
#include <imkit/charts.h>

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
    draw->AddRectFilled(a, b, U32(fill), round);
    if (dashed)
        detail::DrawDashedRect(draw, a, b, round, U32(c.muted, .7f), 1.f, 3.f);
    else if (detail::IsOutlined(ImGui::GetStyle()))
        detail::DrawOutline(draw, a, b, round);
    else
        draw->AddRect(a, b, U32(tone.w > 0 ? Mix(c.border, tone, .6f) : c.border, .8f), round, 0, 1.f);
    DrawTextCentered(draw, a, b, U32(ink), text);
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), round + 2.f, 0, 2.f);
    SemanticItem(o, text, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
    ImGui::PopID();
    return clicked;
}
} // namespace impl
} // namespace imkit::show
