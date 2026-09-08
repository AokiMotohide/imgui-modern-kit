#pragma once

#include <imgui.h>
#include <array>
#include <map>
#include <string>
#include <span>

namespace imkit::design {

struct Palette {
    ImVec4 canvas, surface, inset, raised, text, muted, border;
    ImVec4 accent, onAccent, selection, danger, onDanger, focus;
};
struct DesignTokens {
    Palette color;
    float height, spacing, radius, border, shadow, body, heading, focus;
    float animation, overlayAnimation;
    int proposal;
    bool dark;
    int refinement = -1;
};
struct Proposal {
    const char* name;
    const char* intent;
    const char* strength;
    const char* caution;
};
enum class SpecimenState { Normal, Hover, Pressed, Focused, Disabled };
enum class Action { Primary, Secondary, Ghost, Destructive, Icon };
struct Rect { ImVec2 min, max; ImVec2 Center() const { return {(min.x+max.x)*0.5f,(min.y+max.y)*0.5f}; } };
struct Motion { float hover = 0, pressed = 0, selected = 0; };
struct Row { int id; const char* name; int quality; };

struct DesignLabState {
    int proposal = 0;
    bool dark = false, comparison = true;
    bool refined = true, paletteOpen = false;
    bool japanese = false, japaneseEnabled = true;
    char japaneseName[128] = "ディスプレイ A";
    std::array<bool,2> customColors{};
    std::array<Palette,2> palettes{};
    bool enabled = true, checked = true;
    int mixed = 2, radio = 0, combo = 1, tab = 0, selected = 1;
    int clicks = 0, inlineActions = 0, sortDirection = 0;
    float volume = 0.64f, lower = 20, upper = 80, number = 48;
    char text[96] = "Display";
    char search[96] = "";
    char invalid[96] = "";
    bool modal = false, openModal = false, restoreFocus = false;
    bool modalVisible = false, comboVisible = false, actionVisible = false;
    bool validation = true, lastFocused = false;
    bool resetTabs = true;
    float modalTime = 0, popupTime = 0;
    ImFont* regular = nullptr;
    ImFont* semibold = nullptr;
    std::map<std::string, Rect> probes;
    std::map<std::string, bool> focusStates;
    std::map<ImGuiID, Motion> motions;
    std::array<Row, 3> rows{{{0,"Low",24},{1,"Medium",64},{2,"High",96}}};
    void Reset();
};

std::span<const Proposal> Proposals(bool refined = true);
DesignTokens Tokens(int proposal, bool dark, bool refined = true);
void Show(DesignLabState& state);
}
