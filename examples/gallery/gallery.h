#pragma once
#include <imkit/imkit.h>
#include <map>
#include <string>
#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include "editor_workspaces.h"
#include "design_pages.h"
#include "workflow_pages.h"
#include "toast_page.h"
#include "charts_page.h"
#include "ptz_page.h"
#include "show_pages.h"
#include "look_pages.h"
#include "start_page.h"
namespace imkit::gallery {
struct Probe {
    ImVec2 min, max;
    ImVec2 Center() const {
        return {(min.x + max.x) / 2, (min.y + max.y) / 2};
    }
};
// Gallery-only state for the floating comparison. Both columns borrow this
// value, so the difference remains visual rather than a comparison of two
// independent application models.
struct ComparisonState {
    bool open = false;
    int mode = 0;
    bool disabled = false;
    bool focusDefault = false;
    bool enabled = true;
    float level = .64f;
    int quality = 1;
    int applyCount = 0;
    char name[96] = "Display Console";
};
// Page ids are stable: --page and the capture scripts address pages by number. Navigation order and grouping live in
// Pages(), so reordering the sidebar never renumbers a page. PageStart is the Gallery-only start page.
enum PageId : int {
    PageBasic = 0, PageNumeric, PageInput, PageHierarchy, PageOverlay, PageComposites, PageIcons,
    PageEditorCore, PageVideo, PageCG,
    PageFoundations, PageReferenceComponents, PagePatterns, PageAccessibility, PageResponsive,
    PageGenericWorkspace, PageFeedback, PagePreviewTiles, PageFrameLab,
    PageStart, PageToasts, PageCompare, PageWorkbench, PageCharts, PagePtz,
    PageShowControls, PageShowLive, PageShowRules, PageShowSchedule, PageShowBeat, PageShowScript, PageShowSurface,
    PageShowLed, PageShowImages, PageLookStudio, PageLookSkins
};
struct PageInfo {
    PageId id;
    const char *title;
    int group; // Index into PageGroups().
    const char *summary;
};
std::span<const PageInfo> Pages();
std::span<const char *const> PageGroups();
const PageInfo *FindPage(int id);
struct GalleryState {
    DesignPages design;
    WorkflowPages workflow;
    ToastPage toasts;
    ChartsPage charts;
    PtzPage ptz;
    ShowPages show;
    LookPages look;
    StartPage start;
    EditorWorkspaces editors;
    Theme theme = MakeTheme(ThemePreset::PrecisionLight);
    FontSet fonts{};
    AnimationState animation;
    int page = PageStart;
    // Hides the navigation and top bar so a page fills the window; used by the motion captures.
    bool presentation = false;
    int presetIndex = 0, presetFilter = 0, copyClicks = 0;
    float scale = ThemeScaleDefault;
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
    bool floatingComparison = false, showDearImGuiDemo = false, demoWindowPositioned = false;
    ImVec2 floatingCatalogPosition{}, floatingCatalogSize{};
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
    bool qualityOpen = true;
    bool treeOpen = false, tabOpen = true, modalVisible = false, popupVisible = false, notice = true;
    int selectedRow = 1, inlineActions = 0, tab = 0, callbackCount = 0;
    std::array<int, 3> rows = {24, 64, 96};
    std::vector<char> growing = std::vector<char>(8, 0);
    bool imageActivated = false;
    bool modalLauncherFocused = false;
    float tableScroll = 0;
    ImVec4 color{.42f, .32f, .72f, 1};
    ImTextureRef texture{};
    std::array<ImTextureRef, 6> sampleImages{}; // Generated pictures for the picture-button pages.
    IconAtlas icons;
    char iconSearch[96]{};
    int iconCategory = 0, iconSizeIndex = 1, selectedIcon = 0, iconClicks = 0;
    bool iconCustomColor = false, iconFocus = false, iconFocused = false;
    ImVec4 iconColor{.25f, .5f, .85f, 1};
    ComparisonState comparison;
    std::map<std::string, Probe> probes;
};
// Restores plain Dear ImGui colours and metrics (fonts and scale stay) for the lifetime of the scope, so a specimen can
// be drawn "before" next to the same calls drawn with an ImKit theme.
class DefaultStyleScope {
  public:
    DefaultStyleScope() : previous_(ImGui::GetStyle()) {
        ImGui::GetStyle() = ImGuiStyle{};
        ImGui::StyleColorsDark(&ImGui::GetStyle());
        ImGui::GetStyle().FontSizeBase = previous_.FontSizeBase;
        ImGui::GetStyle().FontScaleMain = previous_.FontScaleMain;
        ImGui::GetStyle().FontScaleDpi = previous_.FontScaleDpi;
        ImGui::GetStyle().ScaleAllSizes(ImGui::GetIO().DisplayFramebufferScale.x);
        ImGui::GetStyle().Colors[ImGuiCol_ChildBg].w = 1.f;
    }
    ~DefaultStyleScope() { ImGui::GetStyle() = previous_; }
    DefaultStyleScope(const DefaultStyleScope &) = delete;
    DefaultStyleScope &operator=(const DefaultStyleScope &) = delete;

  private:
    ImGuiStyle previous_;
};
// Makes ThemePresets()[index] the Gallery theme and keeps the Appearance controls in step with it.
void ApplyThemePreset(GalleryState &s, int index);
// Layout helpers shared by the Gallery pages: a responsive card grid and titled cards.
bool BeginGrid(GalleryState &s, const char *id, int maxColumns = 2);
void EndGrid();
bool BeginSection(GalleryState &s, const char *title, const char *hint = nullptr);
void EndSection(bool visible);
void Show(GalleryState &s);
void ShowComparison(GalleryState &s);
void Record(GalleryState &s, const char *name);
void SelectFramePreset(GalleryState& s, WindowFramePreset preset);
void RegenerateFrameColors(GalleryState& s);
void ResetFramePreset(GalleryState& s);
} // namespace imkit::gallery
