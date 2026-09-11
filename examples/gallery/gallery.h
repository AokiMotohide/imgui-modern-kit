#pragma once
#include <imkit/imkit.h>
#include <map>
#include <string>
#include <array>
#include <cstdint>
#include <vector>
#include "editor_workspaces.h"
#include "design_pages.h"
#include "workflow_pages.h"
namespace imkit::gallery {
struct Probe {
    ImVec2 min, max;
    ImVec2 Center() const {
        return {(min.x + max.x) / 2, (min.y + max.y) / 2};
    }
};
struct GalleryState {
    DesignPages design;
    WorkflowPages workflow;
    EditorWorkspaces editors;
    Theme theme = MakeTheme(ThemePreset::PrecisionLight);
    FontSet fonts{};
    AnimationState animation;
    int page = -1;
    int presetIndex = 0, presetFilter = 0, copyClicks = 0;
    float scale = 1;
    float windowFrameHeight = 0;
    WindowFramePreset framePreset = WindowFramePreset::Studio;
    std::array<WindowFrameStyle, 4> framePresetStyles{
        MakeWindowFrameStyle(WindowFramePreset::Native, theme), MakeWindowFrameStyle(WindowFramePreset::Studio, theme),
        MakeWindowFrameStyle(WindowFramePreset::Workspace, theme), MakeWindowFrameStyle(WindowFramePreset::Tool, theme)};
    WindowFrameStyle frameStyle = MakeWindowFrameStyle(WindowFramePreset::Studio, theme);
    std::array<std::string_view, 4> frameWorkspaces{"Edit", "Color", "Audio", "Deliver"};
    std::size_t frameWorkspace = 0;
    bool frameUnsaved = true;
    bool dark = false, palette = false;
    char gallerySearch[96]{};
    bool focusApply = false, applyFocused = false;
    bool checked = true, toggle = false, selected = false;
    int radio = 0, combo = 1, segment = 0, clicks = 0;
    CheckState mixed = CheckState::Mixed;
    float scalar = .5f, vector[4] = {1, 2, 3, 4}, lower = 20, upper = 80;
    int integers[4] = {1, 2, 3, 4};
    std::int64_t integer64 = 9007199254740993LL;
    double precise = .125;
    char name[128] = "Display / 表示", notes[512] = "Line one\n日本語の入力例", search[96] = "",
         validation[64] = "";
    bool treeOpen = false, tabOpen = true, modalVisible = false, popupVisible = false, notice = true;
    int selectedRow = 1, inlineActions = 0, tab = 0, callbackCount = 0;
    std::array<int, 3> rows = {24, 64, 96};
    std::vector<char> growing = std::vector<char>(8, 0);
    bool imageActivated = false;
    bool modalLauncherFocused = false;
    float tableScroll = 0;
    ImVec4 color{.42f, .32f, .72f, 1};
    ImTextureRef texture{};
    IconAtlas icons;
    char iconSearch[96]{};
    int iconCategory = 0, iconSizeIndex = 1, selectedIcon = 0, iconClicks = 0;
    bool iconCustomColor = false, iconFocus = false, iconFocused = false;
    ImVec4 iconColor{.25f, .5f, .85f, 1};
    std::map<std::string, Probe> probes;
};
void Show(GalleryState &s);
void Record(GalleryState &s, const char *name);
void SelectFramePreset(GalleryState& s, WindowFramePreset preset);
void RegenerateFrameColors(GalleryState& s);
void ResetFramePreset(GalleryState& s);
} // namespace imkit::gallery
