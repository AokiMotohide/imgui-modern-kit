#include <imkit/components.h>
#include <imkit/widgets.h>
#include "component_internal.h"
#include "draw_internal.h"
#include "imgui_internal_compat.h"
#include "part_style_internal.h"
#include <algorithm>
#include <cstring>
#include <string_view>
#include <vector>

namespace imkit {
namespace {
struct SectionCardState {
    ImGuiContext *context = nullptr;
    ImVec2 start{};
    float width = 0.f;
    float padX = 0.f;
    float padY = 0.f;
    const Theme *theme = nullptr;
};

std::vector<SectionCardState> &SectionCardStack() {
    static std::vector<SectionCardState> stack;
    return stack;
}

std::string_view VisibleText(const char *label) {
    const std::string_view full = label ? label : "";
    return full.substr(0, full.find("##"));
}
} // namespace

bool LabeledSliderFloat(const char *label, float *value, float minimum, float maximum, const char *unit,
                        const char *format, float step, SliderInputInteraction *interaction,
                        ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-slider");
    ImGui::BeginGroup();
    const std::string_view visible = VisibleText(label);
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float rowStartX = ImGui::GetCursorPosX();
    if (!visible.empty())
        ImGui::TextUnformatted(visible.data(), visible.data() + visible.size());
    if (unit && *unit) {
        const float unitW = ImGui::CalcTextSize(unit).x;
        if (!visible.empty() && avail > unitW + 24.f) {
            ImGui::SameLine();
            ImGui::SetCursorPosX(rowStartX + std::max(0.f, avail - unitW));
        }
        ImGui::TextDisabled("%s", unit);
    }
    ImGui::SetNextItemWidth(avail);
    const bool changed = SliderFloatWithInput("##slider", value, minimum, maximum, format, step, interaction, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool ChoiceCard(const char *id, const char *title, const ChoiceCardOptions &options) {
    const auto &style = ImGui::GetStyle();
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float width = options.width > 0.f ? options.width : avail;
    const bool hasSub = options.subtitle && *options.subtitle;
    const float font = ImGui::GetFontSize();
    const float padX = style.FramePadding.x;
    const float padY = std::max(6.f, style.FramePadding.y);
    const float height = hasSub ? (font * 2.f + padY * 2.f + 2.f) : std::max(ImGui::GetFrameHeight(), font + padY * 2.f);

    ImGui::BeginDisabled(options.disabled);
    const bool pressedByWidget = ImGui::InvisibleButton(id ? id : "##choice-card", {width, height});
    if (ImGui::IsItemFocused())
        ImGui::SetNavCursorVisible(true);

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const bool focused = ImGui::IsItemFocused();

    const Theme *t = options.components.theme;
    const SurfaceStyle surfaceStyle = t ? t->surfaceStyle : detail::CurrentSurfaceStyle();
    const bool isDark = t ? (t->scheme == ColorScheme::Dark) : detail::CurrentIsDark();
    const ImVec4 accent = t ? t->colors.accent : style.Colors[ImGuiCol_CheckMark];
    ImVec4 baseFill = t ? t->semantic.surface : style.Colors[ImGuiCol_ChildBg];
    if (baseFill.w <= 0.f)
        baseFill = style.Colors[ImGuiCol_WindowBg];

    ImVec4 fill = baseFill;
    if (options.selected)
        fill = detail::MixColor(baseFill, accent, isDark ? .16f : .10f);
    else if (hovered && !options.disabled)
        fill = detail::MixColor(baseFill, style.Colors[ImGuiCol_Text], .04f);

    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float radius = style.FrameRounding;
    detail::DrawSurfaceRect(draw, min, max, fill, radius, surfaceStyle, isDark, !options.disabled && !active);

    const ImVec4 borderCol = options.selected
        ? accent
        : detail::WithAlpha(style.Colors[ImGuiCol_Text], (hovered && !options.disabled) ? .22f : .12f);
    detail::AddSmoothRect(draw, min, max, ImGui::GetColorU32(borderCol), radius, options.selected ? 1.5f : 1.f);

    if (focused && ImGui::GetIO().NavVisible && !options.disabled)
        detail::DrawFocusGlow(draw, min, max, radius, 1.f, accent, t ? t->stroke.focus : 2.f);

    float leftX = min.x + padX;
    if (options.dotColor.w > 0.f) {
        const float dotR = 3.5f;
        const float dotY = hasSub ? (min.y + padY + font * .5f) : (min.y + height * .5f);
        draw->AddCircleFilled({leftX + dotR, dotY}, dotR, ImGui::GetColorU32(options.dotColor), 16);
        leftX += dotR * 2.f + 6.f;
    }

    float rightLimit = max.x - padX;
    const ImVec4 mutedCol = style.Colors[ImGuiCol_TextDisabled];
    if (options.trailingText && *options.trailingText) {
        const float trailingW = ImGui::CalcTextSize(options.trailingText).x;
        const float trailingY = hasSub ? (min.y + padY) : (min.y + (height - font) * .5f);
        draw->AddText({std::max(leftX, rightLimit - trailingW), trailingY}, ImGui::GetColorU32(mutedCol),
                      options.trailingText);
        rightLimit -= trailingW + 8.f;
    }

    const std::string_view titleView = VisibleText(title);
    if (!titleView.empty() && rightLimit > leftX) {
        const float titleY = hasSub ? (min.y + padY) : (min.y + (height - font) * .5f);
        const ImVec4 clip{leftX, min.y, rightLimit, max.y};
        draw->AddText(ImGui::GetFont(), font, {leftX, titleY}, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]),
                      titleView.data(), titleView.data() + titleView.size(), 0.f, &clip);
    }
    if (hasSub && rightLimit > leftX) {
        const float subY = min.y + padY + font + 2.f;
        const ImVec4 clip{leftX, min.y, rightLimit, max.y};
        draw->AddText(ImGui::GetFont(), font, {leftX, subY}, ImGui::GetColorU32(mutedCol), options.subtitle, nullptr,
                      0.f, &clip);
    }

    const bool pressed = detail::SemanticItem(options.components, title ? title : "", accessibility::SemanticRole::Radio,
                                              accessibility::SemanticAction::Select, options.selected) ||
                         pressedByWidget;
    ImGui::EndDisabled();
    return pressed;
}

bool BeginSectionCard(const char *id, const char *title, const char *description, ComponentOptions options) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGui::PushID(id ? id : "##section-card");
    const auto &style = ImGui::GetStyle();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float padX = style.WindowPadding.x;
    const float padY = style.WindowPadding.y;

    ImDrawList *draw = window->DrawList;
    draw->ChannelsSplit(2);
    draw->ChannelsSetCurrent(1);

    ImGui::SetCursorScreenPos({start.x + padX, start.y + padY});
    ImGui::BeginGroup();
    const float innerW = std::max(1.f, width - padX * 2.f);
    ImGui::PushTextWrapPos(start.x + width - padX - ImGui::GetWindowPos().x + ImGui::GetScrollX());
    ImGui::PushItemWidth(innerW);

    if (title && *title) {
        if (options.theme)
            PushTextRole(*options.theme, TextRole::Heading);
        ImGui::TextUnformatted(title);
        if (options.theme)
            PopTextRole();
    }
    if (description && *description)
        ImGui::TextDisabled("%s", description);
    if ((title && *title) || (description && *description))
        ImGui::Spacing();

    SectionCardStack().push_back({ImGui::GetCurrentContext(), start, width, padX, padY, options.theme});
    return true;
}

void EndSectionCard() {
    ImGui::PopItemWidth();
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    auto &stack = SectionCardStack();
    SectionCardState state{};
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        if (it->context == ImGui::GetCurrentContext()) {
            state = *it;
            stack.erase(std::next(it).base());
            break;
        }
    }

    const float bottom = ImGui::GetItemRectMax().y + state.padY;
    const ImVec2 end{state.start.x + state.width, std::max(state.start.y + 1.f, bottom)};
    ImDrawList *draw = ImGui::GetWindowDrawList();
    draw->ChannelsSetCurrent(0);

    const auto &style = ImGui::GetStyle();
    const Theme *t = state.theme;
    const SurfaceStyle surfaceStyle = t ? t->surfaceStyle : detail::CurrentSurfaceStyle();
    const bool isDark = t ? (t->scheme == ColorScheme::Dark) : detail::CurrentIsDark();
    ImVec4 fill = t ? t->semantic.surface : style.Colors[ImGuiCol_ChildBg];
    if (fill.w <= 0.f)
        fill = style.Colors[ImGuiCol_WindowBg];

    const float radius = style.ChildRounding > 0.f ? style.ChildRounding : style.FrameRounding;
    detail::DrawSurfaceRect(draw, state.start, end, fill, radius, surfaceStyle, isDark, false);
    const ImVec4 borderCol = detail::IsOutlined(style)
        ? style.Colors[ImGuiCol_Border]
        : detail::WithAlpha(style.Colors[ImGuiCol_Text], isDark ? .09f : .10f);
    detail::AddSmoothRect(draw, state.start, end, ImGui::GetColorU32(borderCol), radius,
                          std::max(1.f, style.ChildBorderSize));
    draw->ChannelsMerge();

    ImGui::SetCursorScreenPos(state.start);
    ImGui::Dummy({state.width, end.y - state.start.y});
    ImGui::PopID();
}

int ActionButtonRow(const char *id, std::span<const ActionRowItem> items, ComponentOptions options) {
    if (items.empty())
        return -1;
    const auto &style = ImGui::GetStyle();
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float gap = style.ItemSpacing.x;
    const int count = static_cast<int>(items.size());
    const float buttonW = std::max(1.f, (avail - gap * static_cast<float>(count - 1)) / static_cast<float>(count));

    int pressed = -1;
    ImGui::PushID(id ? id : "##action-row");
    for (int i = 0; i < count; ++i) {
        if (i > 0)
            ImGui::SameLine(0.f, gap);
        ImGui::PushID(i);
        ImGui::BeginDisabled(!items[i].enabled);
        ComponentOptions itemOpts = options;
        if (items[i].disabledReason)
            itemOpts.disabledReason = items[i].disabledReason;
        if (ActionButton(items[i].label ? items[i].label : "", items[i].variant, {buttonW, 0.f}, itemOpts) &&
            pressed < 0) {
            pressed = i;
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::PopID();
    return pressed;
}
} // namespace imkit
