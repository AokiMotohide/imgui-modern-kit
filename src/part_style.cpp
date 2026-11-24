#include <imkit/part_style.h>
#include <imkit/theme.h>
#include "draw_internal.h"
#include "part_style_internal.h"
#include "imgui_internal_compat.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace imkit {
namespace {
constexpr std::string_view kPartNames[PartCount] = {
    "button", "small-button", "checkbox", "toggle", "radio", "combo", "selectable", "color",
    "slider", "drag", "input", "progress", "header", "tree", "tab", "separator",
    "table", "scrollbar", "popup", "tooltip", "window", "modal"};
constexpr std::string_view kStateNames[PartStateCount] = {"rest", "hover", "pressed", "focused", "selected",
                                                          "disabled", "active", "on", "off"};
constexpr std::string_view kColorNames[PartColorCount] = {"fill", "border", "text", "mark"};
constexpr std::string_view kVarNames[PartVarCount] = {
    "height", "min-width", "border.width", "radius", "padding-x", "padding-y", "font-size", "press-depth",
    "box.size", "mark.width", "track.height", "thumb.size", "indicator.height", "gap", "focus-ring"};

// Compares names treating '.' and '-' alike, so `thumb.size` and `thumb-size` are the same key.
bool SameKey(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const char x = a[i] == '.' ? '-' : a[i], y = b[i] == '.' ? '-' : b[i];
        if (x != y)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Per-context state.
struct PushedVar {
    Part part;
    PartVar var;
    float value;
};
struct PushedColor {
    Part part;
    PartState state;
    PartColor color;
    ImVec4 value;
};
struct NamedStyle {
    std::string name;
    std::vector<std::pair<Part, PartValues>> parts; // Empty after UnregisterStyle; the slot keeps scope indices valid.
};
struct PartContext {
    ImGuiContext *context = nullptr;
    detail::PartThemeState theme{};
    std::vector<detail::PartThemeState> themeStack;
    std::vector<PushedVar> vars;
    std::vector<PushedColor> colors;
    std::vector<NamedStyle> styles;
    std::vector<int> scopes;
};
std::vector<std::unique_ptr<PartContext>> &Contexts() {
    static std::vector<std::unique_ptr<PartContext>> contexts;
    return contexts;
}
void ForgetContext(ImGuiContext *context, ImGuiContextHook *) {
    auto &all = Contexts();
    all.erase(std::remove_if(all.begin(), all.end(), [&](const auto &c) { return c->context == context; }), all.end());
}
PartContext &Ctx() {
    ImGuiContext *context = ImGui::GetCurrentContext();
    IM_ASSERT(context && "ImKit parts need a current ImGui context");
    for (auto &c : Contexts())
        if (c->context == context)
            return *c;
    auto entry = std::make_unique<PartContext>();
    entry->context = context;
    ImGuiContextHook hook;
    hook.Type = ImGuiContextHookType_Shutdown;
    hook.Callback = ForgetContext;
    ImGui::AddContextHook(context, &hook);
    Contexts().push_back(std::move(entry));
    return *Contexts().back();
}
// The value of a style variable as Dear ImGui currently has it.
ImVec2 StyleVarValue(ImGuiStyleVar idx) {
    const ImGuiStyleVarInfo *info = ImGui::GetStyleVarInfo(idx);
    const float *p = static_cast<const float *>(info->GetVarPtr(&ImGui::GetStyle()));
    return info->Count == 2 ? ImVec2(p[0], p[1]) : ImVec2(p[0], p[0]);
}
bool OnColorStack(ImGuiCol col) {
    if (col < 0)
        return false;
    for (const auto &m : GImGui->ColorStack)
        if (m.Col == col)
            return true;
    return false;
}
bool OnVarStack(ImGuiStyleVar idx) {
    if (idx < 0)
        return false;
    for (const auto &m : GImGui->StyleVarStack)
        if (m.VarIdx == idx)
            return true;
    return false;
}
float Scale(const PartContext &c) {
    return c.theme.applied ? c.theme.scale : ImGui::GetStyle().FontScaleMain;
}
const PartValues *NamedValues(const PartContext &c, int index, Part part) {
    if (index < 0 || index >= static_cast<int>(c.styles.size()))
        return nullptr;
    for (const auto &[p, values] : c.styles[static_cast<std::size_t>(index)].parts)
        if (p == part)
            return &values;
    return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------
// Defaults (layer 5).
using detail::PartImGuiMap;
constexpr ImGuiCol N = -1;
constexpr PartImGuiMap kMaps[PartCount] = {
    // fill, hover, pressed, selected, mark, markHover, markPressed, border, rounding, border, padding
    {ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive, ImGuiCol_ButtonActive, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Button
    {ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive, ImGuiCol_ButtonActive, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // SmallButton
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_CheckboxSelectedBg, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Checkbox
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Toggle
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Radio
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_Text,
     ImGuiCol_Text, ImGuiCol_Text, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Combo
    {N, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive, ImGuiCol_Header, ImGuiCol_CheckMark, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, N, ImGuiStyleVar_FramePadding}, // Selectable
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_Text,
     ImGuiCol_Text, ImGuiCol_Text, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // ColorEdit
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_SliderGrab,
     ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive, ImGuiCol_Border, ImGuiStyleVar_FrameRounding,
     ImGuiStyleVar_FrameBorderSize, ImGuiStyleVar_FramePadding}, // Slider
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive, ImGuiCol_Text,
     ImGuiCol_Text, ImGuiCol_Text, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Drag
    {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_FrameBgActive,
     ImGuiCol_InputTextCursor, ImGuiCol_InputTextCursor, ImGuiCol_InputTextCursor, ImGuiCol_Border,
     ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize, ImGuiStyleVar_FramePadding}, // Input
    {ImGuiCol_FrameBg, ImGuiCol_FrameBg, ImGuiCol_FrameBg, ImGuiCol_FrameBg, ImGuiCol_PlotHistogram,
     ImGuiCol_PlotHistogramHovered, ImGuiCol_PlotHistogram, ImGuiCol_Border, ImGuiStyleVar_FrameRounding,
     ImGuiStyleVar_FrameBorderSize, ImGuiStyleVar_FramePadding}, // Progress
    {ImGuiCol_Header, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive, ImGuiCol_Header, ImGuiCol_Text, ImGuiCol_Text,
     ImGuiCol_Text, ImGuiCol_Border, ImGuiStyleVar_FrameRounding, ImGuiStyleVar_FrameBorderSize,
     ImGuiStyleVar_FramePadding}, // Header
    {N, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive, ImGuiCol_Header, ImGuiCol_Text, ImGuiCol_Text, ImGuiCol_Text,
     ImGuiCol_TreeLines, ImGuiStyleVar_FrameRounding, N, ImGuiStyleVar_FramePadding}, // Tree
    {ImGuiCol_Tab, ImGuiCol_TabHovered, ImGuiCol_TabHovered, ImGuiCol_TabSelected, ImGuiCol_TabSelectedOverline,
     ImGuiCol_TabSelectedOverline, ImGuiCol_TabSelectedOverline, ImGuiCol_Border, ImGuiStyleVar_TabRounding,
     ImGuiStyleVar_TabBorderSize, ImGuiStyleVar_FramePadding}, // Tab
    {ImGuiCol_Separator, ImGuiCol_SeparatorHovered, ImGuiCol_SeparatorActive, ImGuiCol_Separator, ImGuiCol_Separator,
     ImGuiCol_SeparatorHovered, ImGuiCol_SeparatorActive, ImGuiCol_Separator, N, ImGuiStyleVar_SeparatorTextBorderSize,
     ImGuiStyleVar_SeparatorTextPadding}, // Separator
    {ImGuiCol_TableRowBg, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive, ImGuiCol_Header, ImGuiCol_TableRowBgAlt,
     ImGuiCol_TableRowBgAlt, ImGuiCol_TableRowBgAlt, ImGuiCol_TableBorderLight, N, N, ImGuiStyleVar_CellPadding}, // Table
    {ImGuiCol_ScrollbarBg, ImGuiCol_ScrollbarBg, ImGuiCol_ScrollbarBg, ImGuiCol_ScrollbarBg, ImGuiCol_ScrollbarGrab,
     ImGuiCol_ScrollbarGrabHovered, ImGuiCol_ScrollbarGrabActive, N, ImGuiStyleVar_ScrollbarRounding, N, N}, // Scrollbar
    {ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_CheckMark, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_PopupRounding, ImGuiStyleVar_PopupBorderSize,
     ImGuiStyleVar_WindowPadding}, // Popup
    {ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_CheckMark, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_PopupRounding, ImGuiStyleVar_PopupBorderSize,
     ImGuiStyleVar_WindowPadding}, // Tooltip
    {ImGuiCol_WindowBg, ImGuiCol_WindowBg, ImGuiCol_WindowBg, ImGuiCol_WindowBg, ImGuiCol_CheckMark, ImGuiCol_CheckMark,
     ImGuiCol_CheckMark, ImGuiCol_Border, ImGuiStyleVar_WindowRounding, ImGuiStyleVar_WindowBorderSize,
     ImGuiStyleVar_WindowPadding}, // Window
    {ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_PopupBg, ImGuiCol_ModalWindowDimBg,
     ImGuiCol_ModalWindowDimBg, ImGuiCol_ModalWindowDimBg, ImGuiCol_Border, ImGuiStyleVar_PopupRounding,
     ImGuiStyleVar_PopupBorderSize, ImGuiStyleVar_WindowPadding}, // Modal
};

bool IsCheckLike(Part p) { return p == Part::Checkbox || p == Part::Radio || p == Part::Toggle; }
bool IsButtonLike(Part p) { return p == Part::Button || p == Part::SmallButton; }

ImVec4 StyleColor(ImGuiCol col) {
    return col < 0 ? ImVec4(0, 0, 0, 0) : ImGui::GetStyleColorVec4(col);
}

float DefaultVar(const PartContext &c, Part part, PartVar var) {
    const ImGuiStyle &style = ImGui::GetStyle();
    const auto &map = kMaps[static_cast<std::size_t>(part)];
    const float s = Scale(c), font = ImGui::GetFontSize(), frame = ImGui::GetFrameHeight();
    const bool modern = c.theme.look == ThemeLook::Modern;
    const auto &parts = detail::CurrentParts();
    const auto read = [](ImGuiStyleVar idx, int component) {
        if (idx < 0)
            return 0.f;
        const ImVec2 v = StyleVarValue(idx);
        return component == 0 ? v.x : v.y;
    };
    switch (var) {
    case PartVar::Height:
        switch (part) {
        case Part::SmallButton: return font;
        case Part::Checkbox:
        case Part::Radio: return std::round(font * parts.checkHeightScale);
        case Part::Toggle: return std::round(font * parts.toggleHeightScale);
        case Part::Selectable:
        case Part::Tree: return font;
        case Part::Progress: return std::clamp(frame * parts.progressBarHeightScale, 4.f * s, 8.f * s);
        case Part::Separator: return std::max(1.f, style.SeparatorTextBorderSize);
        case Part::Table: return font + style.CellPadding.y * 2.f;
        case Part::Scrollbar: return style.ScrollbarSize;
        case Part::Popup:
        case Part::Tooltip:
        case Part::Window:
        case Part::Modal: return 0.f;
        default: return frame;
        }
    case PartVar::MinWidth: return 0.f;
    case PartVar::BorderWidth: return read(map.borderVar, 0);
    case PartVar::Radius: return read(map.roundingVar, 0);
    case PartVar::PaddingX: return read(map.paddingVar, 0);
    case PartVar::PaddingY: return part == Part::SmallButton ? 0.f : read(map.paddingVar, 1);
    case PartVar::FontSize: return font;
    case PartVar::PressDepth: return modern && IsButtonLike(part) ? std::round(1.f * s) : 0.f;
    case PartVar::BoxSize:
        if (part == Part::Checkbox || part == Part::Radio)
            return std::round(font * parts.checkHeightScale);
        if (part == Part::Toggle)
            return std::round(font * parts.toggleHeightScale);
        return part == Part::ColorEdit ? frame : 0.f;
    case PartVar::MarkWidth:
        switch (part) {
        case Part::Checkbox: return std::max(1.f, std::round(font * parts.checkHeightScale) / 6.f);
        case Part::Radio: return std::round(font * parts.checkHeightScale * .42f);
        case Part::Selectable: return std::round(3.f * s);
        case Part::Combo:
        case Part::Header:
        case Part::Tree: return 1.5f * s;
        default: return 0.f;
        }
    case PartVar::TrackHeight:
        if (part == Part::Slider)
            return modern ? std::round(4.f * s) : frame;
        if (part == Part::Progress)
            return DefaultVar(c, part, PartVar::Height);
        return part == Part::Toggle ? std::round(font * parts.toggleHeightScale) : 0.f;
    case PartVar::ThumbSize:
        if (part == Part::Slider)
            return modern ? std::round(std::max(style.GrabMinSize, font)) : style.GrabMinSize;
        if (part == Part::Toggle)
            return std::round(font * parts.toggleHeightScale) - 2.f * std::round(2.f * s);
        return part == Part::Scrollbar ? style.GrabMinSize : 0.f;
    case PartVar::IndicatorHeight: return part == Part::Tab ? std::round(2.f * s) : 0.f;
    case PartVar::Gap: return style.ItemInnerSpacing.x;
    case PartVar::FocusRing: return (c.theme.applied ? c.theme.focusWidth : 2.f) * s;
    }
    return 0.f;
}

ImVec4 DefaultColor(const PartContext &c, Part part, PartState state, PartColor color) {
    const auto &map = kMaps[static_cast<std::size_t>(part)];
    const bool modern = c.theme.look == ThemeLook::Modern;
    const ImVec4 accent = c.theme.applied ? c.theme.accent : StyleColor(ImGuiCol_CheckMark);
    const ImVec4 onAccent = c.theme.applied ? c.theme.onAccent : StyleColor(ImGuiCol_Text);
    const ImVec4 focus = c.theme.applied ? c.theme.focus : StyleColor(ImGuiCol_NavCursor);
    switch (color) {
    case PartColor::Text:
        if (state == PartState::Disabled)
            return StyleColor(ImGuiCol_TextDisabled);
        if (modern && IsCheckLike(part) && state == PartState::On && part != Part::Radio)
            return StyleColor(ImGuiCol_Text);
        return StyleColor(ImGuiCol_Text);
    case PartColor::Border:
        if (state == PartState::Focused)
            return focus;
        if (modern && part == Part::Checkbox && state == PartState::On)
            return accent;
        if (modern && c.theme.applied && !c.theme.highContrast && !detail::IsOutlined(ImGui::GetStyle()) &&
            state == PartState::Rest) {
            return detail::WithAlpha(StyleColor(ImGuiCol_Text), c.theme.scheme == ColorScheme::Dark ? .12f : .14f);
        }
        return StyleColor(map.border);
    case PartColor::Fill:
        switch (state) {
        case PartState::Hover: return StyleColor(map.fillHover);
        case PartState::Pressed:
        case PartState::Active: return StyleColor(map.fillPressed);
        case PartState::Selected: return StyleColor(map.fillSelected);
        case PartState::On:
            if (modern && (part == Part::Checkbox || part == Part::Toggle))
                return accent;
            return StyleColor(map.fill);
        default: return StyleColor(map.fill);
        }
    case PartColor::Mark:
        switch (state) {
        case PartState::Hover: return StyleColor(map.markHover);
        case PartState::Pressed:
        case PartState::Active: return StyleColor(map.markPressed);
        case PartState::On:
            if (modern && (part == Part::Checkbox || part == Part::Toggle))
                return onAccent;
            return StyleColor(map.mark);
        default: return StyleColor(map.mark);
        }
    }
    return {};
}

// A state no layer names, derived from the rest colour of the strongest layer that names it.
ImVec4 DeriveState(PartState state, PartColor color, ImVec4 rest) {
    if (color != PartColor::Fill)
        return rest;
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if (rest.w < .05f) {
        switch (state) {
        case PartState::Hover: return ImVec4(text.x, text.y, text.z, .08f);
        case PartState::Pressed: return ImVec4(text.x, text.y, text.z, .15f);
        default: return rest;
        }
    }
    const bool dark = detail::CurrentIsDark();
    const detail::Oklch o = detail::ToOklch(rest);
    switch (state) {
    case PartState::Hover: return detail::FromOklch({o.L + (dark ? .035f : -.025f), o.C, o.h}, rest.w);
    case PartState::Pressed: return detail::FromOklch({o.L + (dark ? .07f : -.055f), o.C, o.h}, rest.w);
    default: return rest;
    }
}
bool DerivesFromRest(PartState state) {
    return state == PartState::Hover || state == PartState::Pressed || state == PartState::Focused ||
           state == PartState::Disabled || state == PartState::Off;
}

std::optional<float> FindVar(const PartContext &c, Part part, PartVar var, const PartValues *call, PartLayer &layer) {
    const std::size_t v = static_cast<std::size_t>(var);
    const float s = Scale(c);
    if (call && call->vars[v]) {
        layer = PartLayer::Call;
        return *call->vars[v] * s;
    }
    for (auto it = c.vars.rbegin(); it != c.vars.rend(); ++it)
        if (it->part == part && it->var == var) {
            layer = PartLayer::Pushed;
            return it->value * s;
        }
    if (OnVarStack(detail::ImGuiVarOf(part, var))) {
        layer = PartLayer::Pushed;
        return DefaultVar(c, part, var); // Reads the pushed ImGui value.
    }
    for (auto it = c.scopes.rbegin(); it != c.scopes.rend(); ++it)
        if (const PartValues *named = NamedValues(c, *it, part); named && named->vars[v]) {
            layer = PartLayer::NamedStyle;
            return *named->vars[v] * s;
        }
    if (auto value = c.theme.overrides.Var(part, var)) {
        layer = PartLayer::Theme;
        return *value * s;
    }
    if (var == PartVar::BorderWidth && IsButtonLike(part) && c.theme.buttonStroke > 0.f) {
        layer = PartLayer::Theme; // Stroke::button, the legacy spelling of button.border.width.
        return c.theme.buttonStroke * s;
    }
    return std::nullopt;
}

std::optional<ImVec4> FindColor(const PartContext &c, Part part, PartState state, PartColor color,
                                const PartValues *call, PartLayer &layer) {
    const std::size_t st = static_cast<std::size_t>(state), co = static_cast<std::size_t>(color);
    if (call && call->colors[st][co]) {
        layer = PartLayer::Call;
        return call->colors[st][co];
    }
    for (auto it = c.colors.rbegin(); it != c.colors.rend(); ++it)
        if (it->part == part && it->state == state && it->color == color) {
            layer = PartLayer::Pushed;
            return it->value;
        }
    if (const ImGuiCol col = detail::ImGuiColorOf(part, state, color); OnColorStack(col)) {
        layer = PartLayer::Pushed;
        return ImGui::GetStyleColorVec4(col);
    }
    for (auto it = c.scopes.rbegin(); it != c.scopes.rend(); ++it)
        if (const PartValues *named = NamedValues(c, *it, part); named && named->colors[st][co]) {
            layer = PartLayer::NamedStyle;
            return named->colors[st][co];
        }
    if (auto value = c.theme.overrides.Color(part, state, color)) {
        layer = PartLayer::Theme;
        return value;
    }
    return std::nullopt;
}

ImVec4 ResolveColor(const PartContext &c, Part part, PartState state, PartColor color, const PartValues *call,
                    PartLayer &layer) {
    if (auto value = FindColor(c, part, state, color, call, layer))
        return *value;
    if (state != PartState::Rest && DerivesFromRest(state))
        if (auto rest = FindColor(c, part, PartState::Rest, color, call, layer))
            return DeriveState(state, color, *rest);
    layer = PartLayer::Default;
    return DefaultColor(c, part, state, color);
}

void AddValues(PartValues &out, const PartStyle &s) {
    const auto set = [&](PartVar v, const std::optional<float> &value) {
        if (value)
            out.vars[static_cast<std::size_t>(v)] = value;
    };
    set(PartVar::Height, s.height);
    set(PartVar::MinWidth, s.minWidth);
    set(PartVar::BorderWidth, s.border.width);
    set(PartVar::Radius, s.radius);
    set(PartVar::PaddingX, s.paddingX);
    set(PartVar::PaddingY, s.paddingY);
    set(PartVar::FontSize, s.fontSize);
    set(PartVar::Gap, s.gap);
    set(PartVar::FocusRing, s.focusRing);
    for (std::size_t i = 0; i < PartStateCount; ++i) {
        const PartColors &colors = s.colors[static_cast<PartState>(i)];
        auto &row = out.colors[i];
        if (colors.fill) row[static_cast<std::size_t>(PartColor::Fill)] = colors.fill;
        if (colors.border) row[static_cast<std::size_t>(PartColor::Border)] = colors.border;
        if (colors.text) row[static_cast<std::size_t>(PartColor::Text)] = colors.text;
        if (colors.mark) row[static_cast<std::size_t>(PartColor::Mark)] = colors.mark;
    }
}
void SetVar(PartValues &out, PartVar v, const std::optional<float> &value) {
    if (value)
        out.vars[static_cast<std::size_t>(v)] = value;
}
} // namespace

// ---------------------------------------------------------------------------------------------------------------
std::string_view PartName(Part part) noexcept { return kPartNames[static_cast<std::size_t>(part) % PartCount]; }
std::string_view PartStateName(PartState state) noexcept {
    return kStateNames[static_cast<std::size_t>(state) % PartStateCount];
}
std::string_view PartColorName(PartColor color) noexcept {
    return kColorNames[static_cast<std::size_t>(color) % PartColorCount];
}
std::string_view PartVarName(PartVar var) noexcept { return kVarNames[static_cast<std::size_t>(var) % PartVarCount]; }
std::optional<Part> PartFromName(std::string_view name) noexcept {
    for (std::size_t i = 0; i < PartCount; ++i)
        if (SameKey(kPartNames[i], name))
            return static_cast<Part>(i);
    return std::nullopt;
}
std::optional<PartState> PartStateFromName(std::string_view name) noexcept {
    for (std::size_t i = 0; i < PartStateCount; ++i)
        if (kStateNames[i] == name)
            return static_cast<PartState>(i);
    return std::nullopt;
}
std::optional<PartColor> PartColorFromName(std::string_view name) noexcept {
    for (std::size_t i = 0; i < PartColorCount; ++i)
        if (kColorNames[i] == name)
            return static_cast<PartColor>(i);
    return std::nullopt;
}
std::optional<PartVar> PartVarFromName(std::string_view name) noexcept {
    for (std::size_t i = 0; i < PartVarCount; ++i)
        if (SameKey(kVarNames[i], name))
            return static_cast<PartVar>(i);
    return std::nullopt;
}

ImVec4 Hex(std::string_view text) noexcept {
    if (!text.empty() && text.front() == '#')
        text.remove_prefix(1);
    const ImVec4 invalid{1, 0, 1, 1};
    if (text.size() != 6 && text.size() != 8)
        return invalid;
    unsigned value = 0;
    for (const char ch : text) {
        const int digit = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                        : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
        if (digit < 0)
            return invalid;
        value = value << 4 | static_cast<unsigned>(digit);
    }
    if (text.size() == 6)
        value = value << 8 | 0xffu;
    return {static_cast<float>(value >> 24 & 255) / 255.f, static_cast<float>(value >> 16 & 255) / 255.f,
            static_cast<float>(value >> 8 & 255) / 255.f, static_cast<float>(value & 255) / 255.f};
}

bool PartOverrides::HasPart(Part part) const noexcept {
    for (std::size_t v = 0; v < PartVarCount; ++v)
        if (varSet_[VarIndex(part, static_cast<PartVar>(v))])
            return true;
    const std::size_t first = ColorIndex(part, PartState::Rest, PartColor::Fill);
    for (std::size_t i = 0; i < PartStateCount * PartColorCount; ++i)
        if (colorSet_[first + i])
            return true;
    return false;
}

const PartColors &PartStateColors::operator[](PartState state) const noexcept {
    switch (state) {
    case PartState::Hover: return hover;
    case PartState::Pressed: return pressed;
    case PartState::Focused: return focused;
    case PartState::Selected: return selected;
    case PartState::Disabled: return disabled;
    case PartState::Active: return active;
    case PartState::On: return on;
    case PartState::Off: return off;
    default: return rest;
    }
}
PartColors &PartStateColors::operator[](PartState state) noexcept {
    return const_cast<PartColors &>(static_cast<const PartStateColors &>(*this)[state]);
}

bool PartValues::Empty() const noexcept {
    for (const auto &v : vars)
        if (v)
            return false;
    for (const auto &row : colors)
        for (const auto &c : row)
            if (c)
                return false;
    return true;
}
PartValues ToPartValues(const PartStyle &style) {
    PartValues out;
    AddValues(out, style);
    return out;
}
PartValues ToPartValues(const ButtonStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::PressDepth, style.pressDepth);
    return out;
}
PartValues ToPartValues(const CheckboxStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::BoxSize, style.boxSize);
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const RadioStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::BoxSize, style.boxSize);
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const SliderStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::TrackHeight, style.trackHeight);
    SetVar(out, PartVar::ThumbSize, style.thumbSize);
    return out;
}
PartValues ToPartValues(const ComboStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const SelectableStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const HeaderStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const TreeStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::MarkWidth, style.markWidth);
    return out;
}
PartValues ToPartValues(const TabStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::IndicatorHeight, style.indicatorHeight);
    return out;
}
PartValues ToPartValues(const ProgressStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::TrackHeight, style.trackHeight);
    return out;
}
PartValues ToPartValues(const ColorEditStyle &style) {
    PartValues out = ToPartValues(static_cast<const PartStyle &>(style));
    SetVar(out, PartVar::BoxSize, style.boxSize);
    return out;
}

float ResolvePartVar(Part part, PartVar var, const PartValues *call, PartLayer *layer) {
    const PartContext &c = Ctx();
    PartLayer found = PartLayer::Default;
    float value = 0.f;
    if (auto v = FindVar(c, part, var, call, found))
        value = *v;
    else
        value = DefaultVar(c, part, var);
    if (layer)
        *layer = found;
    return value;
}
ImVec4 ResolvePartColor(Part part, PartState state, PartColor color, const PartValues *call, PartLayer *layer) {
    PartLayer found = PartLayer::Default;
    const ImVec4 value = ResolveColor(Ctx(), part, state, color, call, found);
    if (layer)
        *layer = found;
    return value;
}
bool PartCustomized(Part part, const PartValues *call) {
    const PartContext &c = Ctx();
    if (call && !call->Empty())
        return true;
    for (const auto &v : c.vars)
        if (v.part == part)
            return true;
    for (const auto &v : c.colors)
        if (v.part == part)
            return true;
    for (const int index : c.scopes)
        if (NamedValues(c, index, part))
            return true;
    if (IsButtonLike(part) && c.theme.buttonStroke > 0.f)
        return true;
    return c.theme.overrides.HasPart(part);
}
ResolvedPartStyle ResolvePartStyle(Part part, const PartValues *call) {
    const PartContext &c = Ctx();
    ResolvedPartStyle out;
    out.part = part;
    for (std::size_t v = 0; v < PartVarCount; ++v) {
        PartLayer layer = PartLayer::Default;
        const auto var = static_cast<PartVar>(v);
        auto value = FindVar(c, part, var, call, layer);
        out.vars[v] = value ? *value : DefaultVar(c, part, var);
        out.varLayers[v] = value ? layer : PartLayer::Default;
    }
    for (std::size_t s = 0; s < PartStateCount; ++s)
        for (std::size_t k = 0; k < PartColorCount; ++k) {
            PartLayer layer = PartLayer::Default;
            out.colors[s][k] = ResolveColor(c, part, static_cast<PartState>(s), static_cast<PartColor>(k), call, layer);
            out.colorLayers[s][k] = layer;
        }
    return out;
}

void PushPartVar(Part part, PartVar var, float value) { Ctx().vars.push_back({part, var, value}); }
void PopPartVar(int count) {
    auto &vars = Ctx().vars;
    IM_ASSERT(count >= 0 && count <= static_cast<int>(vars.size()) && "PopPartVar() called more than PushPartVar()");
    vars.resize(vars.size() - static_cast<std::size_t>(std::clamp(count, 0, static_cast<int>(vars.size()))));
}
void PushPartColor(Part part, PartState state, PartColor color, ImVec4 value) {
    Ctx().colors.push_back({part, state, color, value});
}
void PopPartColor(int count) {
    auto &colors = Ctx().colors;
    IM_ASSERT(count >= 0 && count <= static_cast<int>(colors.size()) && "PopPartColor() called more than PushPartColor()");
    colors.resize(colors.size() - static_cast<std::size_t>(std::clamp(count, 0, static_cast<int>(colors.size()))));
}

namespace {
void Register(std::string_view name, Part part, const PartValues &values) {
    auto &styles = Ctx().styles;
    auto it = std::find_if(styles.begin(), styles.end(), [&](const NamedStyle &s) { return s.name == name; });
    if (it == styles.end()) {
        styles.push_back({std::string(name), {}});
        it = std::prev(styles.end());
    }
    for (auto &[p, v] : it->parts)
        if (p == part) {
            v = values;
            return;
        }
    it->parts.emplace_back(part, values);
}
} // namespace
void RegisterStyle(std::string_view name, Part part, const PartStyle &style) { Register(name, part, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const ButtonStyle &style) { Register(name, Part::Button, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const CheckboxStyle &style) { Register(name, Part::Checkbox, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const RadioStyle &style) { Register(name, Part::Radio, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const SliderStyle &style) { Register(name, Part::Slider, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const DragStyle &style) {
    Register(name, Part::Drag, ToPartValues(static_cast<const PartStyle &>(style)));
}
void RegisterStyle(std::string_view name, const InputStyle &style) {
    Register(name, Part::Input, ToPartValues(static_cast<const PartStyle &>(style)));
}
void RegisterStyle(std::string_view name, const ComboStyle &style) { Register(name, Part::Combo, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const SelectableStyle &style) {
    Register(name, Part::Selectable, ToPartValues(style));
}
void RegisterStyle(std::string_view name, const HeaderStyle &style) { Register(name, Part::Header, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const TreeStyle &style) { Register(name, Part::Tree, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const TabStyle &style) { Register(name, Part::Tab, ToPartValues(style)); }
void RegisterStyle(std::string_view name, const ProgressStyle &style) {
    Register(name, Part::Progress, ToPartValues(style));
}
void RegisterStyle(std::string_view name, const ColorEditStyle &style) {
    Register(name, Part::ColorEdit, ToPartValues(style));
}
void UnregisterStyle(std::string_view name) {
    for (auto &s : Ctx().styles)
        if (s.name == name)
            s.parts.clear();
}
bool IsStyleRegistered(std::string_view name) {
    for (const auto &s : Ctx().styles)
        if (s.name == name && !s.parts.empty())
            return true;
    return false;
}
StyleScope::StyleScope(std::string_view name) : context_(ImGui::GetCurrentContext()) {
    auto &c = Ctx();
    int index = -1;
    for (std::size_t i = 0; i < c.styles.size(); ++i)
        if (c.styles[i].name == name) {
            index = static_cast<int>(i);
            break;
        }
    c.scopes.push_back(index); // -1 keeps the pop balanced for an unknown name.
}
StyleScope::~StyleScope() {
    IM_ASSERT(ImGui::GetCurrentContext() == context_);
    auto &scopes = Ctx().scopes;
    if (!scopes.empty())
        scopes.pop_back();
}

// ---------------------------------------------------------------------------------------------------------------
namespace detail {
void SetPartTheme(const Theme &theme, float scale) {
    auto &state = Ctx().theme;
    state.applied = true;
    state.look = theme.look;
    state.surfaceStyle = theme.surfaceStyle;
    state.scheme = theme.scheme;
    state.highContrast = theme.contrast == ContrastMode::HighContrast;
    state.scale = scale;
    state.buttonStroke = theme.stroke.button;
    state.focusWidth = theme.stroke.focus;
    state.controlSeconds = theme.motion.controlSeconds;
    state.motion = theme.motion.enabled && !theme.motion.reducedMotion;
    state.accent = theme.colors.accent;
    state.onAccent = theme.colors.onAccent;
    state.focus = theme.colors.focus;
    state.overrides = theme.partOverrides;
}
void PushPartThemeSnapshot() {
    auto &c = Ctx();
    c.themeStack.push_back(c.theme);
}
void PopPartThemeSnapshot() {
    auto &c = Ctx();
    if (c.themeStack.empty())
        return;
    c.theme = c.themeStack.back();
    c.themeStack.pop_back();
}
const PartThemeState &CurrentPartTheme() { return Ctx().theme; }
PartThemeState &MutablePartTheme() { return Ctx().theme; }
const PartImGuiMap &ImGuiMapOf(Part part) { return kMaps[static_cast<std::size_t>(part)]; }
ImGuiCol ImGuiColorOf(Part part, PartState state, PartColor color) {
    const auto &m = kMaps[static_cast<std::size_t>(part)];
    switch (color) {
    case PartColor::Text: return state == PartState::Disabled ? ImGuiCol_TextDisabled : ImGuiCol_Text;
    case PartColor::Border: return state == PartState::Rest ? m.border : N;
    case PartColor::Fill:
        switch (state) {
        case PartState::Rest: return m.fill;
        case PartState::Hover: return m.fillHover;
        case PartState::Pressed:
        case PartState::Active: return m.fillPressed;
        case PartState::Selected: return m.fillSelected;
        default: return N;
        }
    case PartColor::Mark:
        switch (state) {
        case PartState::Rest: return m.mark;
        case PartState::Hover: return m.markHover;
        case PartState::Pressed:
        case PartState::Active: return m.markPressed;
        default: return N;
        }
    }
    return N;
}
bool UsesFrameHeight(Part part) {
    switch (part) {
    case Part::Button:
    case Part::Checkbox:
    case Part::Radio:
    case Part::Combo:
    case Part::ColorEdit:
    case Part::Slider:
    case Part::Drag:
    case Part::Input:
    case Part::Header:
    case Part::Tab: return true;
    default: return false;
    }
}
ImGuiStyleVar ImGuiVarOf(Part part, PartVar var) {
    const auto &m = kMaps[static_cast<std::size_t>(part)];
    switch (var) {
    case PartVar::Height:
        if (part == Part::Scrollbar)
            return ImGuiStyleVar_ScrollbarSize;
        return UsesFrameHeight(part) ? m.paddingVar : N;
    case PartVar::Radius: return m.roundingVar;
    case PartVar::BorderWidth: return m.borderVar;
    case PartVar::PaddingX:
    case PartVar::PaddingY: return part == Part::SmallButton && var == PartVar::PaddingY ? N : m.paddingVar;
    case PartVar::ThumbSize: return part == Part::Slider || part == Part::Scrollbar ? ImGuiStyleVar_GrabMinSize : N;
    case PartVar::Gap: return ImGuiStyleVar_ItemInnerSpacing;
    default: return N;
    }
}
} // namespace detail

// ---------------------------------------------------------------------------------------------------------------
float AnimatePart(ImGuiID id, float target) {
    ImGuiStorage *storage = ImGui::GetStateStorage();
    const auto &theme = Ctx().theme;
    float *value = storage->GetFloatRef(id, -1.f);
    const float seconds = theme.applied ? theme.controlSeconds : .14f;
    const bool motion = theme.applied ? theme.motion : true;
    if (*value < 0.f || !motion || seconds <= 0.f) {
        *value = target;
        return target;
    }
    const float dt = std::max(0.f, ImGui::GetIO().DeltaTime);
    const float k = 1.f - std::exp(-dt * 5.f / seconds); // About 99% of the way after `seconds`.
    *value += (target - *value) * k;
    if (std::fabs(target - *value) < .002f)
        *value = target;
    return *value;
}

InteractResult Interact(ImGuiID id, ImVec2 min, ImVec2 max, InteractFlags flags) {
    InteractResult r;
    r.id = id;
    r.min = min;
    r.max = max;
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return r;
    const ImRect bb(min, max);
    if (flags & InteractFlags_Layout)
        ImGui::ItemSize(bb);
    if (flags & InteractFlags_AllowOverlap)
        ImGui::SetNextItemAllowOverlap();
    r.disabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    const ImGuiItemFlags itemFlags = (flags & InteractFlags_NoNavFocus) ? ImGuiItemFlags_NoNav : ImGuiItemFlags_None;
    if (!ImGui::ItemAdd(bb, id, nullptr, itemFlags))
        return r;
    r.visible = true;
    const ImGuiButtonFlags buttonFlags = (flags & InteractFlags_PressOnClick) ? ImGuiButtonFlags_PressedOnClick : 0;
    r.pressed = ImGui::ButtonBehavior(bb, id, &r.hovered, &r.held, buttonFlags);
    r.focused = g.NavId == id && !r.disabled;
    r.focusVisible = r.focused && g.NavCursorVisible;
    r.hover01 = AnimatePart(ImHashData(&id, sizeof id, 0x68u), r.hovered ? 1.f : 0.f);
    r.active01 = AnimatePart(ImHashData(&id, sizeof id, 0x61u), r.held ? 1.f : 0.f);
    r.focus01 = AnimatePart(ImHashData(&id, sizeof id, 0x66u), r.focusVisible ? 1.f : 0.f);
    return r;
}
InteractResult Interact(const char *strId, ImVec2 size, InteractFlags flags) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(strId);
    const ImVec2 min = window->DC.CursorPos;
    return Interact(id, min, {min.x + size.x, min.y + size.y}, flags | InteractFlags_Layout);
}

void DrawSmoothRect(ImDrawList *draw, ImVec2 min, ImVec2 max, ImU32 fill, float radius) {
    detail::AddSmoothRectFilled(draw, min, max, fill, radius);
}
void DrawSmoothRectBorder(ImDrawList *draw, ImVec2 min, ImVec2 max, ImU32 color, float radius, float thickness) {
    detail::AddSmoothRect(draw, min, max, color, radius, thickness);
}
void DrawDashedRect(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, ImU32 color, float thickness, float dash) {
    detail::DrawDashedRect(draw, min, max, radius, color, thickness, dash);
}
void DrawSoftShadow(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, float extent, float alpha, ImVec4 color) {
    detail::DrawSoftShadow(draw, min, max, radius, extent, alpha, color);
}
void DrawFocusRing(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, float amount, float width, float gap) {
    (void)gap;
    amount = std::clamp(amount, 0.f, 1.f);
    if (amount <= 0.f)
        return;
    const auto &c = Ctx();
    if (width < 0.f)
        width = (c.theme.applied ? c.theme.focusWidth : 2.f) * Scale(c);
    if (width <= 0.f)
        return;
    const ImVec4 color = c.theme.applied ? c.theme.focus : ImGui::GetStyleColorVec4(ImGuiCol_NavCursor);
    detail::DrawFocusGlow(draw, min, max, radius, amount, color, width);
}
void DrawChevron(ImDrawList *draw, ImVec2 center, float size, ImU32 color, ImGuiDir direction, float thickness) {
    detail::DrawChevron(draw, center, size, color, direction, thickness);
}
void DrawChevronRotated(ImDrawList *draw, ImVec2 center, float size, ImU32 color, float angle, float thickness) {
    const float h = size * .5f, w = size * .3f;
    const float cs = std::cos(angle), sn = std::sin(angle);
    const auto at = [&](float x, float y) { return ImVec2(center.x + x * cs - y * sn, center.y + x * sn + y * cs); };
    draw->PathLineTo(at(-w * .5f, -h));
    draw->PathLineTo(at(w * .5f, 0));
    draw->PathLineTo(at(-w * .5f, h));
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
void DrawCheckMark(ImDrawList *draw, ImVec2 center, float size, ImU32 color, float thickness, float progress) {
    progress = std::clamp(progress, 0.f, 1.f);
    if (progress <= 0.f)
        return;
    const float s = size * .5f;
    const ImVec2 a{center.x - s * .8f, center.y + s * .05f}, b{center.x - s * .25f, center.y + s * .55f},
        c{center.x + s * .85f, center.y - s * .5f};
    const float first = std::hypot(b.x - a.x, b.y - a.y), second = std::hypot(c.x - b.x, c.y - b.y);
    const float length = (first + second) * progress;
    draw->PathLineTo(a);
    if (length <= first) {
        const float t = first > 0 ? length / first : 1.f;
        draw->PathLineTo({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
    } else {
        const float t = second > 0 ? (length - first) / second : 1.f;
        draw->PathLineTo(b);
        draw->PathLineTo({b.x + (c.x - b.x) * t, b.y + (c.y - b.y) * t});
    }
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
} // namespace imkit
