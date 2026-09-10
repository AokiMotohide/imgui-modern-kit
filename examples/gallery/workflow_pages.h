#pragma once
#include <imkit/editor_canvas.h>
#include <array>
namespace imkit::gallery {
struct GalleryState;
struct WorkflowPages {
    StepNavigatorState steps,rail;
    StableId selected=1,tileSelected=1;
    ToolbarState toolbar;
    CommandPaletteState palette;
    DialogState progressDialog;
    editor::ImageViewportState image;
    editor::TileStripState strip;
    std::array<editor::TileSize,4> sizes{{{1,260},{2,260},{3,260},{4,260}}};
    std::array<StableId,16> selectedStorage{};
    editor::Selection selection{selectedStorage};
    std::array<editor::Point,256> lassoScratch{};
    std::array<accessibility::SemanticNode,512> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    bool notice=true,open=true,chip=false,vertical=false,japanese=false,disabled=false,lasso=false;
    int actions=0,toastPriority=1;
    float fraction=.4f;
    double expiresAt=0;
    void Show(int page,GalleryState& host);
};
}
