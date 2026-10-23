#pragma once
#include <imkit/editor_canvas.h>
#include <imkit/shell.h>
#include <array>
namespace imkit::gallery {
struct GalleryState;
struct WorkflowPages {
    StepNavigatorState steps,rail;
    StableId selected=1,tileSelected=1;
    StableId choiceSource=701;
    StableId workspace=101;
    bool hierarchyOpen=true,settingEnabled=true;
    ToolbarState toolbar;
    CommandPaletteState palette;
    DialogState progressDialog;
    DiagnosticsDrawerState diagnostics;
    RightSidePanelState rightPanel;
    ThemePickerState themePicker;
    editor::ImageViewportState image;
    std::array<editor::ImageViewportState,4> imageComparisons{};
    editor::TileStripState strip;
    std::array<editor::TileSize,5> sizes{{{1,260},{2,260},{3,260},{4,260},{5,260}}};
    std::array<StableId,16> selectedStorage{};
    editor::Selection selection{selectedStorage};
    std::array<editor::Point,256> lassoScratch{};
    std::array<accessibility::SemanticNode,512> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    bool notice=false,open=true,advanced=false,chip=false,vertical=false,japanese=false,disabled=false,lasso=false;
    int actions=0,toastPriority=1;
    float fraction=.4f;
    float taskFraction=.28f;
    int taskUpdates=0;
    float position[3]{};
    double expiresAt=0;
    std::array<bool,3> visible{{true,true,true}}, locked{};
    bool workSettings=false;
    void Workbench(GalleryState& host);
    void Show(int page,GalleryState& host);
};
}
