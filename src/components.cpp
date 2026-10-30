// This file implements or exercises the deprecated spellings on purpose.
#define IMKIT_DEPRECATED(message)
#include <imkit/components.h>
#include <imkit/widgets.h>
#include "component_internal.h"
#include "draw_internal.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

namespace imkit {
namespace {
using detail::SemanticItem;
ImVec4 Mix(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
ImVec4 Accent(const Theme *t) {
    return t ? t->colors.accent : ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
}
ImVec4 Status(StatusKind kind, const Theme *t) {
    if (!t)
        return ImGui::GetStyleColorVec4(kind == StatusKind::Neutral ? ImGuiCol_TextDisabled
                                                                    : ImGuiCol_CheckMark);
    switch (kind) {
    case StatusKind::Success:
        return t->colors.success;
    case StatusKind::Warning:
        return t->colors.warning;
    case StatusKind::Error:
        return t->colors.destructive;
    default:
        return t->colors.muted;
    }
}
bool Contains(const char *text, const char *query) {
    // ASCII case folding only. UTF-8 non-ASCII bytes are compared unchanged.
    auto fold = [](unsigned char c) { return c < 128 ? static_cast<unsigned char>(std::tolower(c)) : c; };
    std::string a = text, b = query;
    for (auto &c : a)
        c = static_cast<char>(fold(static_cast<unsigned char>(c)));
    for (auto &c : b)
        c = static_cast<char>(fold(static_cast<unsigned char>(c)));
    return a.find(b) != std::string::npos;
}
} // namespace
bool ActionButton(const char *label, ActionVariant variant, const ImVec2 &size, ComponentOptions options) {
    const auto *t = options.theme;
    auto &style = ImGui::GetStyle();
    const bool subtleDestructive = variant == ActionVariant::SubtleDestructive;
    const ImVec4 destructive = t ? t->colors.destructive : ImVec4(.68f, .20f, .25f, 1);
    ImVec4 fill = style.Colors[ImGuiCol_Button], text = style.Colors[ImGuiCol_Text];
    if (variant == ActionVariant::Primary) {
        fill = Accent(t);
        text = t ? t->colors.onAccent : ImVec4(1, 1, 1, 1);
    }
    if (variant == ActionVariant::Destructive) {
        fill = destructive;
        text = t ? t->colors.onDestructive : ImVec4(1, 1, 1, 1);
    }
    if (subtleDestructive) {
        fill = destructive;
        fill.w *= .12f;
        text = destructive;
    }
    if (variant == ActionVariant::Ghost)
        fill.w = 0;
    ImGui::PushStyleColor(ImGuiCol_Button, fill);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Mix(fill, subtleDestructive ? destructive : Accent(t), .18f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Mix(fill, subtleDestructive ? destructive : Accent(t), .32f));
    ImGui::PushStyleColor(ImGuiCol_Text, text);
    const bool solid = variant == ActionVariant::Primary || variant == ActionVariant::Destructive;
    // Solid and ghost buttons are drawn without a frame, unless the theme is outlined and every button carries it.
    const bool borderless = (solid || variant == ActionVariant::Ghost) && !detail::IsOutlined(style);
    if (borderless)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    else if (subtleDestructive) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, std::max(1.f, style.FrameBorderSize));
        auto border = destructive;
        border.w *= .58f;
        ImGui::PushStyleColor(ImGuiCol_Border, border);
    }
    const auto id = ImGui::GetID(label);
    bool pressed = ImGui::Button(label, size);
    if(ImGui::IsItemFocused()) ImGui::SetNavCursorVisible(true);
    if((ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0 &&
       ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s",options.disabledReason?options.disabledReason:"Unavailable");
    pressed = SemanticItem(options,label,accessibility::SemanticRole::Button,accessibility::SemanticAction::Press) || pressed;
    if (options.animation) {
        const auto motion = t ? t->motion : Motion{};
        float a = options.animation->Update(id, ImGui::IsItemHovered() ? 1.f : 0.f, ImGui::GetIO().DeltaTime,
                                            motion.controlSeconds, ImGui::GetFrameCount(), motion.enabled && !motion.reducedMotion, motion.easing);
        // Hover is a soft tonal wash that fades in; no outline.
        auto color = solid ? ImVec4(1, 1, 1, 1) : Accent(t);
        color.w = (solid ? .10f : .08f) * a;
        if (a > 0)
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                      ImGui::GetColorU32(color), style.FrameRounding);
    }
    if (borderless || subtleDestructive)
        ImGui::PopStyleVar();
    ImGui::PopStyleColor(subtleDestructive ? 5 : 4);
    return pressed;
}
void DrawAtlasIcon(const IconAtlas& atlas,IconId icon,ImVec2 position,float size) {
    const auto scale=ImGui::GetIO().DisplayFramebufferScale;
    const float pixels=size*std::max({1.f,scale.x,scale.y});
    int level=5;
    for(int i=0;i<5;++i) if(IconPixelSizes[i]>=pixels) {level=i;break;}
    const auto texture=atlas.textures[static_cast<std::size_t>(level)];
    if(!GetIconInfo(icon) || texture.GetTexID()==ImTextureID{}) return;
    const auto region=GetIconRegion(icon,IconPixelSizes[static_cast<std::size_t>(level)]);
    ImGui::GetWindowDrawList()->AddImage(texture,position,{position.x+size,position.y+size},
                                         region.uv0,region.uv1,ImGui::GetColorU32(ImGuiCol_Text));
}
bool IconActionButton(const char *id, const IconAtlas &atlas, IconId icon,
                      const char *label, const char *description, ActionVariant variant,
                      ComponentOptions options) {
    const auto *t = options.theme;
    auto &style = ImGui::GetStyle();
    const bool subtleDestructive = variant == ActionVariant::SubtleDestructive;
    const ImVec4 destructive = t ? t->colors.destructive : ImVec4(.68f, .20f, .25f, 1);
    ImVec4 fill = style.Colors[ImGuiCol_Button], text = style.Colors[ImGuiCol_Text];
    if (variant == ActionVariant::Primary) {
        fill = Accent(t);
        text = t ? t->colors.onAccent : ImVec4(1, 1, 1, 1);
    }
    if (variant == ActionVariant::Destructive) {
        fill = destructive;
        text = t ? t->colors.onDestructive : ImVec4(1, 1, 1, 1);
    }
    if (subtleDestructive) {
        fill = destructive;
        fill.w *= .12f;
        text = destructive;
    }
    if (variant == ActionVariant::Ghost)
        fill.w = 0;
    ImGui::PushStyleColor(ImGuiCol_Button, fill);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Mix(fill, subtleDestructive ? destructive : Accent(t), .18f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Mix(fill, subtleDestructive ? destructive : Accent(t), .32f));
    ImGui::PushStyleColor(ImGuiCol_Text, text);
    const bool borderless = variant != ActionVariant::Secondary && !subtleDestructive && !detail::IsOutlined(style);
    if (borderless)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    else if (subtleDestructive) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, std::max(1.f, style.FrameBorderSize));
        auto border = destructive;
        border.w *= .58f;
        ImGui::PushStyleColor(ImGuiCol_Border, border);
    }
    const float iconSize = std::max(12.f, ImGui::GetFontSize());
    const std::string_view fullLabel=label?label:"";
    const auto visibleLabel=fullLabel.substr(0,fullLabel.find("##"));
    const auto labelSize=ImGui::CalcTextSize(visibleLabel.data(),visibleLabel.data()+visibleLabel.size());
    const float gap=visibleLabel.empty()?0.f:style.ItemInnerSpacing.x;
    const ImVec2 extent{iconSize+gap+labelSize.x+2*style.FramePadding.x,
                        std::max(iconSize,labelSize.y)+2*style.FramePadding.y};
    std::array<char,1024> widgetLabel{};
    std::snprintf(widgetLabel.data(),widgetLabel.size(),"%.*s##%s",
                  static_cast<int>(std::min<std::size_t>(visibleLabel.size(),900)),
                  visibleLabel.data(),id?id:"");
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    const bool pressedByWidget=ImGui::Button(widgetLabel.data(),extent);
    ImGui::PopStyleColor();
    if(ImGui::IsItemFocused()) ImGui::SetNavCursorVisible(true);
    const auto minimum=ImGui::GetItemRectMin();
    DrawAtlasIcon(atlas,icon,{minimum.x+style.FramePadding.x,
                             minimum.y+(extent.y-iconSize)*.5f},iconSize);
    ImGui::GetWindowDrawList()->AddText(
        {minimum.x+style.FramePadding.x+iconSize+gap,
         minimum.y+(extent.y-labelSize.y)*.5f},ImGui::GetColorU32(ImGuiCol_Text),
        visibleLabel.data(),visibleLabel.data()+visibleLabel.size());
    bool pressed = SemanticItem(options, label, accessibility::SemanticRole::Button,
                                accessibility::SemanticAction::Press) || pressedByWidget;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemFocused()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.f);
        ImGui::TextUnformatted(label ? label : "");
        const bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
        if (disabled && options.disabledReason && *options.disabledReason)
            ImGui::TextDisabled("%s", options.disabledReason);
        else if (description && *description)
            ImGui::TextUnformatted(description);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    if (borderless || subtleDestructive)
        ImGui::PopStyleVar();
    ImGui::PopStyleColor(subtleDestructive ? 5 : 4);
    return pressed;
}
bool IconButton(const char *id, ImGuiDir direction, const char *accessibleLabel, ComponentOptions options) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0)); // The stock arrow is replaced below.
    bool pressed = ImGui::ArrowButton(id, direction);
    ImGui::PopStyleColor();
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    detail::DrawChevron(ImGui::GetWindowDrawList(), {(a.x + b.x) * .5f, (a.y + b.y) * .5f}, (b.y - a.y) * .36f,
                        ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)), direction);
    pressed = SemanticItem(options,accessibleLabel,accessibility::SemanticRole::Button,accessibility::SemanticAction::Press) || pressed;
    if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
        ImGui::SetTooltip("%s", accessibleLabel);
    return pressed;
}
bool Toggle(const char *label, bool *value, ComponentOptions options) {
    const auto id = ImGui::GetID(label);
    const ImGuiCol hidden[] = {ImGuiCol_FrameBg,       ImGuiCol_FrameBgHovered,
                               ImGuiCol_FrameBgActive, ImGuiCol_CheckboxSelectedBg,
                               ImGuiCol_CheckMark,     ImGuiCol_Border};
    for (auto color : hidden)
        ImGui::PushStyleColor(color, ImVec4(0, 0, 0, 0));
    // The hit square is 1.75 font heights: a switch needs less height than a 30 px control.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {ImGui::GetStyle().FramePadding.x, ImGui::GetFontSize() * .375f});
    bool changed = imkit::Checkbox(label, value);
    ImGui::PopStyleVar();
    if(SemanticItem(options,label,accessibility::SemanticRole::Toggle,accessibility::SemanticAction::Toggle,*value)) {
        *value=!*value; changed=true;
    }
    ImGui::PopStyleColor(6);
    auto min = ImGui::GetItemRectMin();
    float h = ImGui::GetItemRectSize().y, round = h * .5f;
    auto &style = ImGui::GetStyle();
    float position = *value ? 1.f : 0.f;
    if (options.animation) {
        auto motion = options.theme ? options.theme->motion : Motion{};
        position = options.animation->Update(id, position, ImGui::GetIO().DeltaTime, motion.controlSeconds,
                                             ImGui::GetFrameCount(), motion.enabled && !motion.reducedMotion, motion.easing);
    }
    auto *d = ImGui::GetWindowDrawList();
    const ImVec4 off = detail::WithAlpha(style.Colors[ImGuiCol_Text], .22f);
    auto fill = Mix(off, Accent(options.theme), position);
    auto thumb = Mix(ImVec4(1, 1, 1, 1), options.theme ? options.theme->colors.onAccent : ImVec4(1, 1, 1, 1), position);
    // Fit in native checkbox's square so label layout and hit target remain native.
    const float trackHeight = h * .6f, trackWidth = std::min(h, trackHeight * 1.8f);
    const float y = min.y + (h - trackHeight) * .5f;
    // An outline takes room from inside the track, so the thumb shrinks with it.
    const float inset = detail::IsOutlined(style) ? style.FrameBorderSize * .5f : 0.f;
    const float thumbRadius = std::max(2.f, trackHeight * .5f - 2.5f - inset);
    d->AddRectFilled({min.x, y}, {min.x + trackWidth, y + trackHeight}, ImGui::GetColorU32(fill), trackHeight * .5f);
    detail::DrawOutline(d, {min.x, y}, {min.x + trackWidth, y + trackHeight}, trackHeight * .5f);
    // The thumb overshoots slightly mid-travel, then settles.
    const float travel = position + std::sin(position * 3.14159265f) * .06f;
    const float left = min.x + trackHeight * .5f, right = min.x + trackWidth - trackHeight * .5f;
    d->AddCircleFilled({left + (right - left) * travel, y + trackHeight * .5f}, thumbRadius, ImGui::GetColorU32(thumb));
    if (ImGui::IsItemFocused())
        d->AddRect({min.x - 2, y - 2}, {min.x + trackWidth + 2, y + trackHeight + 2}, ImGui::GetColorU32(ImGuiCol_NavCursor),
                   trackHeight * .5f + 2, ImDrawFlags_None, 1.5f);
    return changed;
}
bool IndeterminateCheckbox(const char *label, CheckState *value) {
    bool checked = *value == CheckState::Checked;
    bool mixed = *value == CheckState::Mixed;
    bool changed = imkit::Checkbox(label, &checked);
    if (changed)
        *value = mixed ? CheckState::Checked : checked ? CheckState::Checked : CheckState::Unchecked;
    if (mixed && !changed) {
        auto p = ImGui::GetItemRectMin();
        float h = ImGui::GetItemRectSize().y;
        ImGui::GetWindowDrawList()->AddLine({p.x + h * .27f, p.y + h * .5f}, {p.x + h * .73f, p.y + h * .5f},
                                            ImGui::GetColorU32(ImGuiCol_CheckMark), 2);
    }
    return changed;
}
bool Segmented(const char *id, int *selected, std::span<const char *const> labels, ComponentOptions options) {
    bool changed = false;
    if (labels.empty())
        return false;
    const auto &style = ImGui::GetStyle();
    const float inset = 3.f, height = ImGui::GetFrameHeight();
    const int count = static_cast<int>(labels.size());
    std::array<float, 32> widths{}, lefts{};
    float total = 0;
    for (int i = 0; i < std::min(count, 32); ++i) {
        widths[i] = ImGui::CalcTextSize(labels[i], nullptr, true).x + style.FramePadding.x * 2;
        lefts[i] = total;
        total += widths[i];
    }
    const int shown = std::min(count, 32);
    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    // Submit the whole control first so a preceding SameLine() applies to it, not to the first segment.
    ImGui::Dummy({total + inset * 2, height});
    const ImVec2 after = ImGui::GetCursorScreenPos();
    auto *draw = ImGui::GetWindowDrawList();
    const float rounding = style.FrameRounding;
    draw->AddRectFilled(origin, {origin.x + total + inset * 2, origin.y + height},
                        ImGui::GetColorU32(detail::WithAlpha(style.Colors[ImGuiCol_Text], .08f)), rounding + 2.f);
    detail::DrawOutline(draw, origin, {origin.x + total + inset * 2, origin.y + height}, rounding + 2.f);
    // The raised thumb glides between segments (fractional index while animating).
    float index = static_cast<float>(std::clamp(*selected, 0, shown - 1));
    if (options.animation) {
        const auto motion = options.theme ? options.theme->motion : Motion{};
        index = options.animation->Update(ImGui::GetID("##segment-thumb"), index, ImGui::GetIO().DeltaTime,
                                          motion.controlSeconds, ImGui::GetFrameCount(),
                                          motion.enabled && !motion.reducedMotion, motion.easing);
    }
    const int lower = std::clamp(static_cast<int>(std::floor(index)), 0, shown - 1);
    const int upper = std::min(lower + 1, shown - 1);
    const float blend = index - static_cast<float>(lower);
    const float thumbLeft = lefts[lower] + (lefts[upper] - lefts[lower]) * blend;
    const float thumbWidth = widths[lower] + (widths[upper] - widths[lower]) * blend;
    const ImVec2 thumbMin{origin.x + inset + thumbLeft, origin.y + inset},
        thumbMax{origin.x + inset + thumbLeft + thumbWidth, origin.y + height - inset};
    draw->AddRectFilled({thumbMin.x, thumbMin.y + 1}, {thumbMax.x, thumbMax.y + 1}, IM_COL32(0, 0, 0, 26), rounding);
    draw->AddRectFilled(thumbMin, thumbMax, ImGui::GetColorU32(options.theme ? options.theme->colors.raised : style.Colors[ImGuiCol_PopupBg]), rounding);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, detail::WithAlpha(style.Colors[ImGuiCol_Text], .06f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, detail::WithAlpha(style.Colors[ImGuiCol_Text], .10f));
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {.5f, .5f});
    for (int i = 0; i < shown; ++i) {
        ImGui::SetCursorScreenPos({origin.x + inset + lefts[i], origin.y + inset});
        ImGui::PushID(i);
        const bool active = *selected == i;
        if (!active)
            ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
        const bool hit = ImGui::Selectable(labels[i], false, 0, {widths[i], height - inset * 2});
        if (!active)
            ImGui::PopStyleColor();
        if (hit | SemanticItem(options, labels[i], accessibility::SemanticRole::Radio,
                               accessibility::SemanticAction::Select, active)) {
            changed = *selected != i;
            *selected = i;
        }
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::SetCursorScreenPos(after);
    ImGui::PopID();
    return changed;
}
bool SearchableCombo(const char *label, int *selected, std::span<const char *const> labels, char *search,
                     std::size_t capacity, std::span<const bool> disabled, ComponentOptions options) {
    const char *preview =
        *selected >= 0 && *selected < static_cast<int>(labels.size()) ? labels[*selected] : "";
    bool changed = false;
    if (imkit::BeginCombo(label, preview)) {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##search", options.locale?options.locale->Text("search","Search..."):"Search...", search, capacity);
        SemanticItem(options,label,accessibility::SemanticRole::TextField,accessibility::SemanticAction::Focus);
        for (int i = 0; i < static_cast<int>(labels.size()); ++i)
            if (Contains(labels[i], search)) {
                ImGui::PushID(i);
                ImGui::BeginDisabled(i < static_cast<int>(disabled.size()) && disabled[i]);
                if (imkit::Selectable(labels[i], *selected == i)) {
                    changed = *selected != i;
                    *selected = i;
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }
        imkit::EndCombo();
    }
    return changed;
}
bool InputScalarWithUnit(const char *label, ImGuiDataType type, void *value, const char *unit,
                         const void *step, const void *fast, const char *format, ImGuiInputTextFlags flags) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    bool changed = ImGui::InputScalar("##value", type, value, step, fast, format, flags);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unit);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool DragFloatWithUnit(const char *label, float *value, const char *unit, float speed, float minimum,
                       float maximum, const char *format, ImGuiSliderFlags flags) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    bool changed = ImGui::DragFloat("##value", value, speed, minimum, maximum, format, flags);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unit);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool InputVector3WithUnit(const char *label, float *value, const char *unit, const char *format) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::BeginGroup();
    const float available = ImGui::CalcItemWidth();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label, std::strstr(label, "##"));
    if (unit && *unit) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", unit);
    }
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const bool stacked = available < 240.f;
    const float width = stacked ? std::max(72.f, available) : (available - 2 * spacing) / 3;
    const char *axes[] = {"X", "Y", "Z"};
    const ImGuiCol colors[] = {ImGuiCol_PlotLines, ImGuiCol_PlotHistogram, ImGuiCol_CheckMark};
    for (int i = 0; i < 3; ++i) {
        if (i && !stacked)
            ImGui::SameLine();
        ImGui::PushID(i);
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImGui::GetStyleColorVec4(colors[i]), "%s", axes[i]);
        ImGui::SameLine(0, spacing * .5f);
        ImGui::SetNextItemWidth(std::max(45.f, width - ImGui::CalcTextSize(axes[i]).x - spacing * .5f));
        changed |= ImGui::InputFloat("##value", value + i, 0, 0, format);
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool DragVector3WithUnit(const char *label, float *value, const char *unit, float speed,
                         float minimum, float maximum, const char *format, ImGuiSliderFlags flags) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::BeginGroup();
    const float available = ImGui::CalcItemWidth();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label, std::strstr(label, "##"));
    if (unit && *unit) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", unit);
    }
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const bool stacked = available < 240.f;
    const float width = stacked ? std::max(72.f, available) : (available - 2 * spacing) / 3;
    const char *axes[] = {"X", "Y", "Z"};
    const ImGuiCol colors[] = {ImGuiCol_PlotLines, ImGuiCol_PlotHistogram, ImGuiCol_CheckMark};
    for (int i = 0; i < 3; ++i) {
        if (i && !stacked)
            ImGui::SameLine();
        ImGui::PushID(i);
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImGui::GetStyleColorVec4(colors[i]), "%s", axes[i]);
        ImGui::SameLine(0, spacing * .5f);
        ImGui::SetNextItemWidth(std::max(45.f, width - ImGui::CalcTextSize(axes[i]).x - spacing * .5f));
        changed |= ImGui::DragFloat("##value", value + i, speed, minimum, maximum, format, flags);
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool BeginSettingRow(const char *id, const char *label, float width) {
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp))
        return false;
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, width);
    ImGui::TableSetupColumn("Value");
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    return true;
}
void EndSettingRow() {
    ImGui::EndTable();
}
static ImVec2 StatusBadgeSize(const char *text) {
    const float pad = ImGui::GetFontSize() * .75f;
    return {ImGui::CalcTextSize(text).x + pad * 2 + ImGui::GetFontSize() * .55f, ImGui::GetFontSize() + 8};
}
void StatusBadge(const char *text, StatusKind kind, const Theme *theme) {
    auto p = ImGui::GetCursorScreenPos();
    const auto size = StatusBadgeSize(text);
    ImGui::Dummy(size);
    auto color = Status(kind, theme);
    auto *d = ImGui::GetWindowDrawList();
    auto fill = color;
    fill.w = .14f;
    d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(fill), size.y * .5f);
    detail::DrawOutline(d, p, {p.x + size.x, p.y + size.y}, size.y * .5f);
    // Marker shape and text carry meaning in addition to color.
    const float u = ImGui::GetFontSize() * .3f;
    const ImVec2 m{p.x + size.y * .5f + u * .4f, p.y + size.y * .5f};
    if (kind == StatusKind::Error)
        d->AddRectFilled({m.x - u * .8f, m.y - u * .8f}, {m.x + u * .8f, m.y + u * .8f}, ImGui::GetColorU32(color), 1.5f);
    else if (kind == StatusKind::Warning)
        d->AddTriangleFilled({m.x, m.y - u}, {m.x - u * 1.05f, m.y + u * .8f}, {m.x + u * 1.05f, m.y + u * .8f},
                             ImGui::GetColorU32(color));
    else
        d->AddCircleFilled(m, u * .8f, ImGui::GetColorU32(color));
    d->AddText({m.x + u * 1.6f, p.y + (size.y - ImGui::GetTextLineHeight()) * .5f}, ImGui::GetColorU32(color), text);
}
CompactActionRowRequest CompactActionRow(const char *id,
                                         const CompactActionRowOptions &options) {
    const bool hasStatus = options.statusText && options.statusText[0] != '\0';
    const bool hasPrimary = options.primaryLabel && options.primaryLabel[0] != '\0';
    const bool hasSettings = options.settingsLabel && options.settingsLabel[0] != '\0';
    const auto &style = ImGui::GetStyle();
    const float spacing = style.ItemSpacing.x;
    const auto textWidth = [](const char *label) {
        return ImGui::CalcTextSize(label, nullptr, true).x +
               ImGui::GetStyle().FramePadding.x * 2.0f;
    };
    const float statusWidth = hasStatus ? StatusBadgeSize(options.statusText).x : 0.0f;
    const float primaryWidth = hasPrimary ? textWidth(options.primaryLabel) : 0.0f;
    const float settingsWidth = hasSettings ? textWidth(options.settingsLabel) : 0.0f;
    const float available = ImGui::GetContentRegionAvail().x;
    float oneLineWidth = statusWidth + primaryWidth + settingsWidth;
    int itemCount = static_cast<int>(hasStatus) + static_cast<int>(hasPrimary) + static_cast<int>(hasSettings);
    if (itemCount > 1)
        oneLineWidth += spacing * static_cast<float>(itemCount - 1);
    const bool allFit = oneLineWidth <= available;
    const bool actionsFit = primaryWidth + settingsWidth +
        ((hasPrimary && hasSettings) ? spacing : 0.0f) <= available;

    CompactActionRowRequest request = CompactActionRowRequest::None;
    ImGui::PushID(id ? id : "##compactActionRow");
    ImGui::BeginGroup();
    if (hasStatus)
        StatusBadge(options.statusText, options.statusKind, options.components.theme);

    if (hasPrimary) {
        if (hasStatus && allFit)
            ImGui::SameLine();
        ImGui::BeginDisabled(!options.primaryEnabled);
        ImGui::PushID("primary");
        auto primaryOptions = options.components;
        primaryOptions.disabledReason = options.primaryDisabledReason
            ? options.primaryDisabledReason : options.components.disabledReason;
        if (ActionButton(options.primaryLabel, options.primaryVariant, {}, primaryOptions))
            request = CompactActionRowRequest::Primary;
        ImGui::PopID();
        ImGui::EndDisabled();
    }

    if (hasSettings) {
        if ((hasPrimary || hasStatus) && (allFit || (!allFit && actionsFit && hasPrimary)))
            ImGui::SameLine();
        ImGui::BeginDisabled(!options.settingsEnabled);
        ImGui::PushID("settings");
        auto settingsOptions = options.components;
        settingsOptions.disabledReason = options.settingsDisabledReason
            ? options.settingsDisabledReason : options.components.disabledReason;
        if (ActionButton(options.settingsLabel, options.settingsVariant, {}, settingsOptions) &&
            request == CompactActionRowRequest::None)
            request = CompactActionRowRequest::Settings;
        ImGui::PopID();
        ImGui::EndDisabled();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}
bool NotificationCard(const Notification &n, double now, const Theme *t) {
    if (n.expiresAt > 0 && now >= n.expiresAt)
        return false;
    ImGui::PushID(n.id);
    const auto &style = ImGui::GetStyle();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float pad = style.FramePadding.x, close = ImGui::GetFrameHeight() * .8f;
    const float textWidth = std::max(1.f, width - pad * 2 - close - 6.f);
    const float height = std::max(close, ImGui::CalcTextSize(n.text, nullptr, false, textWidth).y) + style.FramePadding.y * 2;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    auto *d = ImGui::GetWindowDrawList();
    const auto color = Status(n.kind, t);
    d->AddRectFilled(origin, {origin.x + width, origin.y + height}, ImGui::GetColorU32(detail::WithAlpha(color, .10f)),
                     style.FrameRounding);
    detail::DrawOutline(d, origin, {origin.x + width, origin.y + height}, style.FrameRounding);
    d->AddRectFilled({origin.x + 5, origin.y + 8}, {origin.x + 8, origin.y + height - 8}, ImGui::GetColorU32(color), 1.5f);
    const float textHeight = ImGui::CalcTextSize(n.text, nullptr, false, textWidth).y;
    d->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {origin.x + pad + 6, origin.y + (height - textHeight) * .5f},
               ImGui::GetColorU32(ImGuiCol_Text), n.text, nullptr, textWidth);
    ImGui::Dummy({width, height});
    ImGui::SetCursorScreenPos({origin.x + width - close - 4, origin.y + (height - close) * .5f});
    const bool dismiss = ImGui::InvisibleButton("##dismiss", {close, close});
    const ImVec2 c{origin.x + width - close * .5f - 4, origin.y + height * .5f};
    if (ImGui::IsItemHovered())
        d->AddCircleFilled(c, close * .5f, ImGui::GetColorU32(detail::WithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text), .10f)));
    detail::DrawCross(d, c, close * .32f, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
        ImGui::SetTooltip("Dismiss");
    ImGui::SetCursorScreenPos({origin.x, origin.y + height});
    ImGui::Dummy({0, 0});
    ImGui::PopID();
    return dismiss;
}
bool BeginToolbar(const char *id, const ImVec2 &size) {
    ImVec2 actual = size;
    const float padding = ImGui::GetStyle().ItemSpacing.y + 2.f;
    if (actual.y == 0)
        actual.y = ImGui::GetFrameHeight() + 2 * padding;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().FrameRounding + 2.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {ImGui::GetStyle().WindowPadding.x, padding});
    const bool visible = ImGui::BeginChild(id, actual, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    return visible;
}
void EndToolbar() {
    ImGui::EndChild();
}
void ValidationMessage(const char *message, bool invalid, const Theme *t) {
    if (!invalid)
        return;
    auto p = ImGui::GetItemRectMin(), q = ImGui::GetItemRectMax();
    auto color = t ? t->colors.destructive : ImVec4(.8f, .2f, .25f, 1);
    ImGui::GetWindowDrawList()->AddRect(p, q, ImGui::GetColorU32(color), ImGui::GetStyle().FrameRounding,
                                        ImDrawFlags_None, 1.5f);
    if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
        ImGui::SetTooltip("%s", message);
}
void OverlayDecoration(const Theme &t, AnimationState *state) {
    float a = 1;
    if (state) {
        auto id = ImGui::GetID("##imkit-elevation");
        int frame = ImGui::GetFrameCount();
        if (ImGui::IsWindowAppearing())
            state->Update(id, 0, 0, t.motion.overlaySeconds, frame, t.motion.enabled && !t.motion.reducedMotion, t.motion.easing);
        a = state->Update(id, 1, ImGui::GetIO().DeltaTime, t.motion.overlaySeconds, frame, t.motion.enabled && !t.motion.reducedMotion, t.motion.easing);
    }
    const auto p = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    DrawShadow(t, ImGui::GetWindowDrawList(), p, {p.x + size.x, p.y + size.y}, t.radius.overlay, ElevationLevel::Overlay, a);
}
void DrawShadow(const Theme &t, ImDrawList *draw, ImVec2 minimum, ImVec2 maximum, float rounding, ElevationLevel level,
                float alpha) {
    const float extent = level == ElevationLevel::Raised ? t.elevation.raised : t.elevation.overlay;
    const bool hasColor = t.semantic.shadow.w > 0;
    if (alpha <= 0 || (!t.elevation.hard && extent <= 0))
        return;
    // The shadow lies outside the shape, so allow drawing beyond the current clip rectangle.
    draw->PushClipRectFullScreen();
    if (t.elevation.hard) {
        // A solid copy of the shape, moved by the offset; it scales with the UI like the outline does.
        const float scale = ImGui::GetStyle().FontScaleMain;
        const ImVec2 offset{t.elevation.offset.x * scale, t.elevation.offset.y * scale};
        ImVec4 color = hasColor ? t.semantic.shadow : ImVec4(0, 0, 0, .25f);
        color.w *= alpha;
        draw->AddRectFilled({minimum.x + offset.x, minimum.y + offset.y}, {maximum.x + offset.x, maximum.y + offset.y},
                            ImGui::GetColorU32(color), rounding);
    } else {
        const float peak = (t.scheme == ColorScheme::Dark ? .34f : .16f) * alpha;
        detail::DrawSoftShadow(draw, minimum, maximum, rounding, extent, peak, hasColor ? t.semantic.shadow : ImVec4(0, 0, 0, 1));
    }
    draw->PopClipRect();
}
void StatusBadge(const char *text, StatusKind kind, ComponentOptions options) { StatusBadge(text, kind, options.theme); }
bool NotificationCard(const Notification &n, double now, ComponentOptions options) { return NotificationCard(n, now, options.theme); }
void ValidationMessage(const char *message, bool invalid, ComponentOptions options) { ValidationMessage(message, invalid, options.theme); }
} // namespace imkit
