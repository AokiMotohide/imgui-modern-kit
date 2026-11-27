#include <imkit/components.h>
#include <imkit/widgets.h>
#include "component_internal.h"
#include "draw_internal.h"
#include "imgui_internal_compat.h"
#include "part_style_internal.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
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
    ImDrawListSplitter splitter{};
};

std::vector<std::unique_ptr<SectionCardState>> &SectionCardStack() {
    static std::vector<std::unique_ptr<SectionCardState>> stack;
    return stack;
}

std::string_view VisibleText(const char *label) {
    const std::string_view full = label ? label : "";
    return full.substr(0, full.find("##"));
}

struct ResolvedTextSpec {
    ImFont *font = nullptr;
    float size = 0.f;
    ImVec4 color{0, 0, 0, 0};
};

ResolvedTextSpec ResolveTextSpec(TextRole role, bool muted, ComponentOptions options) {
    const auto &pt = detail::CurrentPartTheme();
    const Theme *t = options.theme;
    const Typography &typo = t ? t->typography : (pt.applied ? pt.typography : Typography{});
    const FontSet &fonts = t ? t->fonts : pt.fonts;
    const auto &style = ImGui::GetStyle();
    const float currentBase = ImGui::GetFontSize();
    const float bodyRef = std::max(1.f, typo.body);
    const float scale = currentBase / bodyRef;

    ResolvedTextSpec spec;
    switch (role) {
    case TextRole::Title:
        spec.font = fonts.display ? fonts.display : (fonts.emphasis ? fonts.emphasis : fonts.regular);
        spec.size = std::round(typo.title * scale);
        spec.color = style.Colors[ImGuiCol_Text];
        break;
    case TextRole::Heading:
        spec.font = fonts.display ? fonts.display : (fonts.emphasis ? fonts.emphasis : fonts.regular);
        spec.size = std::round(typo.heading * scale);
        spec.color = style.Colors[ImGuiCol_Text];
        break;
    case TextRole::Caption:
        spec.font = fonts.regular;
        spec.size = std::max(10.f, std::round(typo.caption * scale));
        spec.color = style.Colors[ImGuiCol_TextDisabled];
        break;
    case TextRole::Numeric:
        spec.font = fonts.numeric ? fonts.numeric : fonts.regular;
        spec.size = currentBase;
        spec.color = style.Colors[ImGuiCol_Text];
        break;
    case TextRole::Mono:
        spec.font = fonts.numeric ? fonts.numeric : fonts.regular;
        spec.size = std::max(10.f, std::round(typo.monospace * scale));
        spec.color = style.Colors[ImGuiCol_Text];
        break;
    case TextRole::Body:
    default:
        spec.font = muted && fonts.emphasis ? fonts.emphasis : fonts.regular;
        spec.size = currentBase;
        spec.color = muted ? (t ? t->semantic.textSecondary
                                : (pt.applied ? pt.textSecondary : style.Colors[ImGuiCol_TextDisabled]))
                           : style.Colors[ImGuiCol_Text];
        break;
    }
    if (muted && role == TextRole::Body)
        spec.size = std::max(11.f, std::round(typo.label * scale * .92f));
    if (!spec.font)
        spec.font = ImGui::GetFont();
    return spec;
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

    auto state = std::make_unique<SectionCardState>();
    state->context = ImGui::GetCurrentContext();
    state->start = start;
    state->width = width;
    state->padX = padX;
    state->padY = padY;
    state->theme = options.theme;

    ImDrawList *draw = window->DrawList;
    state->splitter.Split(draw, 2);
    state->splitter.SetCurrentChannel(draw, 1);

    ImGui::SetCursorScreenPos({start.x + padX, start.y + padY});
    ImGui::BeginGroup();
    const float innerW = std::max(1.f, width - padX * 2.f);
    ImGui::PushTextWrapPos(start.x + width - padX - ImGui::GetWindowPos().x + ImGui::GetScrollX());
    ImGui::PushItemWidth(innerW);

    if (title && *title)
        TextHeading(title, options);
    if (description && *description)
        TextCaption(description, options);
    if ((title && *title) || (description && *description))
        ImGui::Spacing();

    SectionCardStack().push_back(std::move(state));
    return true;
}

void EndSectionCard() {
    ImGui::PopItemWidth();
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    auto &stack = SectionCardStack();
    std::unique_ptr<SectionCardState> state;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        if ((*it)->context == ImGui::GetCurrentContext()) {
            state = std::move(*it);
            stack.erase(std::next(it).base());
            break;
        }
    }
    if (!state) {
        ImGui::PopID();
        return;
    }

    const float bottom = ImGui::GetItemRectMax().y + state->padY;
    const ImVec2 end{state->start.x + state->width, std::max(state->start.y + 1.f, bottom)};
    ImDrawList *draw = ImGui::GetWindowDrawList();
    state->splitter.SetCurrentChannel(draw, 0);

    const auto &style = ImGui::GetStyle();
    const Theme *t = state->theme;
    const SurfaceStyle surfaceStyle = t ? t->surfaceStyle : detail::CurrentSurfaceStyle();
    const bool isDark = t ? (t->scheme == ColorScheme::Dark) : detail::CurrentIsDark();
    ImVec4 fill = t ? t->semantic.surface : style.Colors[ImGuiCol_ChildBg];
    if (fill.w <= 0.f)
        fill = style.Colors[ImGuiCol_WindowBg];

    const float radius = style.ChildRounding > 0.f ? style.ChildRounding : style.FrameRounding;
    detail::DrawSurfaceRect(draw, state->start, end, fill, radius, surfaceStyle, isDark, false);
    const ImVec4 borderCol = detail::IsOutlined(style)
        ? style.Colors[ImGuiCol_Border]
        : detail::WithAlpha(style.Colors[ImGuiCol_Text], isDark ? .09f : .10f);
    detail::AddSmoothRect(draw, state->start, end, ImGui::GetColorU32(borderCol), radius,
                          std::max(1.f, style.ChildBorderSize));
    state->splitter.Merge(draw);

    ImGui::SetCursorScreenPos(state->start);
    ImGui::Dummy({state->width, end.y - state->start.y});
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

void TextTitle(const char *text, ComponentOptions options) {
    const auto spec = ResolveTextSpec(TextRole::Title, false, options);
    ImGui::PushFont(spec.font, spec.size);
    ImGui::TextWrapped("%s", text ? text : "");
    ImGui::PopFont();
}

void TextHeading(const char *text, ComponentOptions options) {
    const auto spec = ResolveTextSpec(TextRole::Heading, false, options);
    ImGui::PushFont(spec.font, spec.size);
    ImGui::TextWrapped("%s", text ? text : "");
    ImGui::PopFont();
}

void TextLabel(const char *text, ComponentOptions options) {
    const auto spec = ResolveTextSpec(TextRole::Body, true, options);
    ImGui::PushFont(spec.font, spec.size);
    ImGui::PushStyleColor(ImGuiCol_Text, spec.color);
    ImGui::TextUnformatted(text ? text : "");
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void TextCaption(const char *text, ComponentOptions options) {
    const auto spec = ResolveTextSpec(TextRole::Caption, true, options);
    ImGui::PushFont(spec.font, spec.size);
    ImGui::PushStyleColor(ImGuiCol_Text, spec.color);
    ImGui::TextWrapped("%s", text ? text : "");
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void KeyValueRow(const char *label, const char *value, bool numericValue, ComponentOptions options) {
    const auto labelSpec = ResolveTextSpec(TextRole::Body, true, options);
    const auto valueSpec = ResolveTextSpec(numericValue ? TextRole::Numeric : TextRole::Body, false, options);
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const ImVec2 start = ImGui::GetCursorPos();

    ImGui::PushFont(valueSpec.font, valueSpec.size);
    const float valueW = ImGui::CalcTextSize(value ? value : "").x;
    ImGui::PopFont();

    ImGui::PushFont(labelSpec.font, labelSpec.size);
    ImGui::PushStyleColor(ImGuiCol_Text, labelSpec.color);
    ImGui::TextUnformatted(label ? label : "");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    const float gap = ImGui::GetStyle().ItemSpacing.x;
    if (avail > valueW + gap + 24.f) {
        ImGui::SameLine();
        const float targetX = start.x + std::max(0.f, avail - valueW);
        if (ImGui::GetCursorPosX() < targetX)
            ImGui::SetCursorPosX(targetX);
    }
    ImGui::PushFont(valueSpec.font, valueSpec.size);
    ImGui::TextUnformatted(value ? value : "");
    ImGui::PopFont();
}

bool SettingToggleRow(const char *label, bool *value, const char *hint, ComponentOptions options) {
    ImGui::PushID(label ? label : "##setting-toggle");
    ImGui::BeginGroup();
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float startX = ImGui::GetCursorPosX();
    const float toggleScale = detail::CurrentParts().toggleHeightScale;
    const float toggleH = ImGui::GetFontSize() * toggleScale;
    const float toggleW = std::min(toggleH, toggleH * .6f * 1.8f) + ImGui::GetStyle().FramePadding.x;
    const std::string_view visible = VisibleText(label);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(visible.data(), visible.data() + visible.size());
    if (avail > toggleW + 24.f) {
        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + std::max(0.f, avail - toggleW));
    }
    const bool changed = Toggle("##toggle", value, options);
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool LabeledCombo(const char *label, int *current, std::span<const char *const> items, const char *hint,
                  ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-combo");
    ImGui::BeginGroup();
    const std::string_view visible = VisibleText(label);
    if (!visible.empty()) {
        std::string labelCopy(visible);
        TextLabel(labelCopy.c_str(), options);
    }
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = Combo("##combo", current, items.data(), static_cast<int>(items.size()));
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

void SectionHeader(const char *title, const char *subtitle, ComponentOptions options) {
    ImGui::BeginGroup();
    if (title && *title)
        TextHeading(title, options);
    if (subtitle && *subtitle)
        TextCaption(subtitle, options);
    ImGui::EndGroup();
}

namespace {
void RenderFieldTopRow(const char *label, const char *unit, ComponentOptions options) {
    const std::string_view visible = VisibleText(label);
    const float avail = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float rowStartX = ImGui::GetCursorPosX();
    if (!visible.empty()) {
        std::string copy(visible);
        TextLabel(copy.c_str(), options);
    }
    if (unit && *unit) {
        const auto capSpec = ResolveTextSpec(TextRole::Caption, true, options);
        ImGui::PushFont(capSpec.font, capSpec.size);
        const float unitW = ImGui::CalcTextSize(unit).x;
        ImGui::PopFont();
        if (!visible.empty() && avail > unitW + 24.f) {
            ImGui::SameLine();
            ImGui::SetCursorPosX(rowStartX + std::max(0.f, avail - unitW));
        }
        ImGui::PushFont(capSpec.font, capSpec.size);
        ImGui::PushStyleColor(ImGuiCol_Text, capSpec.color);
        ImGui::TextUnformatted(unit);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
}
} // namespace

bool LabeledDragFloat(const char *label, float *value, float speed, float minimum, float maximum,
                      const char *unit, const char *format, const char *hint, ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-drag");
    ImGui::BeginGroup();
    RenderFieldTopRow(label, unit, options);
    const auto numSpec = ResolveTextSpec(TextRole::Numeric, false, options);
    const bool pushNum = numSpec.font != nullptr && numSpec.font != ImGui::GetFont();
    if (pushNum)
        ImGui::PushFont(numSpec.font, numSpec.size);
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = DragFloat("##drag", value, speed, minimum, maximum, format ? format : "%.3f");
    if (pushNum)
        ImGui::PopFont();
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool LabeledInputText(const char *label, char *buffer, std::size_t bufferSize,
                      const char *placeholder, const char *hint, ImGuiInputTextFlags flags,
                      ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-input");
    ImGui::BeginGroup();
    RenderFieldTopRow(label, nullptr, options);
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = (placeholder && *placeholder)
        ? ImGui::InputTextWithHint("##input", placeholder, buffer, bufferSize, flags)
        : InputText("##input", buffer, bufferSize, flags);
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool LabeledColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags,
                       const char *hint, ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-color");
    ImGui::BeginGroup();
    RenderFieldTopRow(label, nullptr, options);
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = ColorEdit4("##color", col, flags);
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool LabeledSearchableCombo(const char *label, int *current, std::span<const char *const> items,
                            char *searchBuffer, std::size_t searchBufferSize,
                            const char *hint, ComponentOptions options) {
    ImGui::PushID(label ? label : "##labeled-searchable-combo");
    ImGui::BeginGroup();
    RenderFieldTopRow(label, nullptr, options);
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = SearchableCombo("##combo", current, items, searchBuffer, searchBufferSize, {}, options);
    if (hint && *hint)
        TextCaption(hint, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool BeginCollapsibleSectionCard(const char *id, const char *title, bool *open, const char *subtitle,
                                 const IconAtlas *icons, IconId icon, ComponentOptions options) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const auto &style = ImGui::GetStyle();
    const Theme *t = options.theme;
    const SurfaceStyle surfaceStyle = t ? t->surfaceStyle : detail::CurrentSurfaceStyle();
    const bool isDark = t ? (t->scheme == ColorScheme::Dark) : detail::CurrentIsDark();
    ImVec4 fill = t ? t->semantic.surface : style.Colors[ImGuiCol_ChildBg];
    if (fill.w <= 0.f)
        fill = style.Colors[ImGuiCol_WindowBg];
    const float radius = style.ChildRounding > 0.f ? style.ChildRounding : style.FrameRounding;
    const ImVec4 borderCol = detail::IsOutlined(style)
        ? style.Colors[ImGuiCol_Border]
        : detail::WithAlpha(style.Colors[ImGuiCol_Text], isDark ? .09f : .10f);

    ImGui::PushID(id ? id : "##collapsible-section-card");
    bool isOpen = open ? *open : true;

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float padX = style.WindowPadding.x;
    const float padY = style.WindowPadding.y;

    auto state = std::make_unique<SectionCardState>();
    state->context = ImGui::GetCurrentContext();
    state->start = start;
    state->width = width;
    state->padX = padX;
    state->padY = padY;
    state->theme = options.theme;

    ImDrawList *draw = window->DrawList;
    state->splitter.Split(draw, 2);
    state->splitter.SetCurrentChannel(draw, 1);

    const float headerH = std::max(ImGui::GetFrameHeight(), ImGui::GetTextLineHeight() + padY);
    ImGui::SetCursorScreenPos({start.x + padX, start.y + padY * .5f});
    const float innerW = std::max(1.f, width - padX * 2.f);
    if (ImGui::InvisibleButton("##header-toggle", {innerW, headerH})) {
        isOpen = !isOpen;
        if (open)
            *open = isOpen;
    }
    const bool hdrHovered = ImGui::IsItemHovered();
    const ImVec2 hMin = ImGui::GetItemRectMin();
    const ImVec2 hMax = ImGui::GetItemRectMax();
    if (hdrHovered) {
        detail::AddSmoothRectFilled(draw, hMin, hMax,
                                    ImGui::GetColorU32(detail::WithAlpha(style.Colors[ImGuiCol_Text], .05f)),
                                    std::min(radius, 6.f));
    }
    const float chevSize = ImGui::GetFontSize() * .6f;
    float cx = hMin.x + 6.f;
    const float cy = (hMin.y + hMax.y) * .5f;
    detail::DrawChevron(draw, {cx + chevSize * .4f, cy}, chevSize,
                        ImGui::GetColorU32(style.Colors[ImGuiCol_TextDisabled]),
                        isOpen ? ImGuiDir_Down : ImGuiDir_Right, 1.6f);
    cx += chevSize + style.ItemInnerSpacing.x + 4.f;
    if (icons && icon != IconId::Count && GetIconInfo(icon)) {
        const float iconSz = ImGui::GetFontSize();
        const auto fbScale = ImGui::GetIO().DisplayFramebufferScale;
        const float px = iconSz * std::max({1.f, fbScale.x, fbScale.y});
        std::size_t lv = IconPixelSizes.size() - 1;
        for (std::size_t i = 0; i < IconPixelSizes.size(); ++i) {
            if (static_cast<float>(IconPixelSizes[i]) >= px) {
                lv = i;
                break;
            }
        }
        const auto tex = icons->textures[lv];
        if (tex.GetTexID() != ImTextureID{}) {
            const auto reg = GetIconRegion(icon, IconPixelSizes[lv]);
            draw->AddImage(tex, {cx, cy - iconSz * .5f}, {cx + iconSz, cy + iconSz * .5f},
                           reg.uv0, reg.uv1, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]));
            cx += iconSz + style.ItemInnerSpacing.x;
        }
    }
    const auto headSpec = ResolveTextSpec(TextRole::Heading, false, options);
    const std::string_view titleVis = VisibleText(title);
    if (!titleVis.empty()) {
        draw->AddText(headSpec.font, headSpec.size, {cx, cy - headSpec.size * .5f},
                      ImGui::GetColorU32(headSpec.color), titleVis.data(), titleVis.data() + titleVis.size());
    }
    if (subtitle && *subtitle) {
        const auto capSpec = ResolveTextSpec(TextRole::Caption, true, options);
        const float subW = capSpec.font->CalcTextSizeA(capSpec.size, FLT_MAX, 0.f, subtitle).x;
        const float titleRight = cx + headSpec.font->CalcTextSizeA(headSpec.size, FLT_MAX, 0.f,
                                                                   titleVis.data(), titleVis.data() + titleVis.size()).x;
        if (hMax.x - subW - 6.f > titleRight + 8.f) {
            draw->AddText(capSpec.font, capSpec.size, {hMax.x - subW - 4.f, cy - capSpec.size * .5f},
                          ImGui::GetColorU32(capSpec.color), subtitle);
        }
    }

    if (!isOpen) {
        const ImVec2 end{start.x + width, hMax.y + padY * .5f};
        state->splitter.SetCurrentChannel(draw, 0);
        detail::DrawSurfaceRect(draw, start, end, fill, radius, surfaceStyle, isDark, false);
        detail::AddSmoothRect(draw, start, end, ImGui::GetColorU32(borderCol), radius,
                              std::max(1.f, style.ChildBorderSize));
        state->splitter.Merge(draw);
        ImGui::SetCursorScreenPos(start);
        ImGui::Dummy({width, end.y - start.y});
        ImGui::PopID();
        return false;
    }

    const float sepY = hMax.y + padY * .35f;
    draw->AddLine({start.x + padX, sepY}, {start.x + width - padX, sepY},
                  ImGui::GetColorU32(detail::WithAlpha(style.Colors[ImGuiCol_Text], isDark ? .08f : .09f)), 1.f);
    ImGui::SetCursorScreenPos({start.x + padX, sepY + padY * .65f});
    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(start.x + width - padX - ImGui::GetWindowPos().x + ImGui::GetScrollX());
    ImGui::PushItemWidth(innerW);
    SectionCardStack().push_back(std::move(state));
    return true;
}

void EndCollapsibleSectionCard() {
    EndSectionCard();
}

void AlertBanner(const char *id, const char *message, StatusKind kind, const char *title,
                 ComponentOptions options) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    const auto &style = ImGui::GetStyle();
    const auto &pt = detail::CurrentPartTheme();
    const Theme *t = options.theme;
    const bool isDark = t ? (t->scheme == ColorScheme::Dark) : detail::CurrentIsDark();

    ImVec4 tone = style.Colors[ImGuiCol_CheckMark];
    switch (kind) {
    case StatusKind::Success:
        tone = t ? t->colors.success : (pt.applied ? pt.success : ImVec4(.30f, .78f, .48f, 1.f));
        break;
    case StatusKind::Warning:
        tone = t ? t->colors.warning : (pt.applied ? pt.warning : ImVec4(.95f, .68f, .24f, 1.f));
        break;
    case StatusKind::Error:
    case StatusKind::Live:
        tone = t ? t->colors.destructive : (pt.applied ? pt.error : ImVec4(.92f, .36f, .34f, 1.f));
        break;
    case StatusKind::Active:
        tone = t ? t->colors.accent : (pt.applied ? pt.accent : style.Colors[ImGuiCol_CheckMark]);
        break;
    case StatusKind::Neutral:
    default:
        tone = t ? t->semantic.textSecondary : style.Colors[ImGuiCol_TextDisabled];
        break;
    }

    ImGui::PushID(id ? id : "##alert-banner");
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float padX = std::max(10.f, style.FramePadding.x);
    const float padY = std::max(7.f, style.FramePadding.y);
    const float barW = 3.f;
    const float textLeft = start.x + barW + padX;
    const float textRight = start.x + width - padX;

    ImDrawList *draw = window->DrawList;
    ImDrawListSplitter splitter;
    splitter.Split(draw, 2);
    splitter.SetCurrentChannel(draw, 1);

    ImGui::SetCursorScreenPos({textLeft, start.y + padY});
    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(std::max(textLeft + 16.f, textRight) - ImGui::GetWindowPos().x + ImGui::GetScrollX());

    const ImVec4 baseBg = (t && t->semantic.surface.w > 0.f) ? t->semantic.surface : style.Colors[ImGuiCol_WindowBg];
    const ImVec4 bg = detail::MixColor(baseBg, tone, isDark ? .14f : .10f);
    const ImVec4 textCol = detail::ReadableOn(tone, bg, style.Colors[ImGuiCol_Text]);
    if (title && *title) {
        const auto headSpec = ResolveTextSpec(TextRole::Body, true, options);
        ImGui::PushFont(headSpec.font, headSpec.size);
        ImGui::PushStyleColor(ImGuiCol_Text, textCol);
        ImGui::TextWrapped("%s", title);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    if (message && *message) {
        ImGui::PushStyleColor(ImGuiCol_Text, (title && *title) ? style.Colors[ImGuiCol_Text] : textCol);
        ImGui::TextWrapped("%s", message);
        ImGui::PopStyleColor();
    }
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    const float bottom = std::max(start.y + padY * 2.f + ImGui::GetTextLineHeight(),
                                  ImGui::GetItemRectMax().y + padY);
    const ImVec2 end{start.x + width, bottom};
    const float radius = std::max(4.f, style.FrameRounding);

    splitter.SetCurrentChannel(draw, 0);
    detail::AddSmoothRectFilled(draw, start, end, ImGui::GetColorU32(bg), radius);
    detail::AddSmoothRect(draw, start, end, ImGui::GetColorU32(detail::WithAlpha(tone, .32f)), radius, 1.f);
    draw->AddRectFilled({start.x + 2.f, start.y + 4.f}, {start.x + 2.f + barW, end.y - 4.f},
                        ImGui::GetColorU32(tone), 1.5f);
    splitter.Merge(draw);

    ImGui::SetCursorScreenPos(start);
    ImGui::Dummy({width, end.y - start.y});
    ImGui::PopID();
}
} // namespace imkit
