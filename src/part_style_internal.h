#pragma once
#include <imkit/part_style.h>
#include <imkit/theme.h>
#include <vector>

// Per-context state of the four customisation layers, shared by theme.cpp, part_style.cpp and the widgets.
namespace imkit::detail {
// The theme values the part defaults need, captured by ApplyTheme. Lengths are design pixels.
struct PartThemeState {
    bool applied = false;
    ThemeLook look = ThemeLook::Modern;
    float scale = 1.f;
    float buttonStroke = 0.f;  // Stroke::button, the legacy key for button.border.width.
    float focusWidth = 2.f;    // Stroke::focus.
    float controlSeconds = .14f;
    bool motion = true;
    ImVec4 accent{}, onAccent{}, focus{};
    PartOverrides overrides{};
};
void SetPartTheme(const Theme &theme, float scale);
// ThemeScope saves and restores the part state of the theme it replaces.
void PushPartThemeSnapshot();
void PopPartThemeSnapshot();
const PartThemeState &CurrentPartTheme();
// The editor changes the current context's theme values directly.
PartThemeState &MutablePartTheme();
inline bool ClassicLook() { return CurrentPartTheme().look == ThemeLook::Classic; }

// How a part maps onto Dear ImGui's style, for defaults, for detecting ImGui pushes and for projecting values.
// -1 means "none"; a fill of -1 is transparent.
struct PartImGuiMap {
    ImGuiCol fill, fillHover, fillPressed, fillSelected;
    ImGuiCol mark, markHover, markPressed;
    ImGuiCol border;
    ImGuiStyleVar roundingVar, borderVar, paddingVar;
};
const PartImGuiMap &ImGuiMapOf(Part part);
// The ImGui colour that carries (part, state, colour) by default, or -1.
ImGuiCol ImGuiColorOf(Part part, PartState state, PartColor color);
// The ImGui style variable that carries (part, var), or -1.
ImGuiStyleVar ImGuiVarOf(Part part, PartVar var);
// True when the part's height comes from FramePadding (frame-like parts).
bool UsesFrameHeight(Part part);
} // namespace imkit::detail
