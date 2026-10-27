#include <imkit/show/common.h>
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
} // namespace imkit::show
