#pragma once
#include <imkit/theme.h>
#include <imkit/icons.h>
#include <span>

namespace imkit::gallery {
struct GalleryState;

struct Probe {
    ImVec2 min, max;
    ImVec2 Center() const {
        return {(min.x + max.x) / 2, (min.y + max.y) / 2};
    }
};
enum PageId : int {
    PageBasic = 0, PageNumeric, PageInput, PageHierarchy, PageOverlay, PageComposites, PageIcons,
    PageEditorCore, PageVideo, PageCG,
    PageFoundations, PageReferenceComponents, PagePatterns, PageAccessibility, PageResponsive,
    PageGenericWorkspace, PageFeedback, PagePreviewTiles, PageFrameLab,
    PageStart, PageToasts, PageCompare, PageWorkbench, PageCharts, PagePtz,
    PageShowControls, PageShowLive, PageShowRules, PageShowSchedule, PageShowBeat, PageShowScript, PageShowSurface,
    PageShowLed, PageShowImages, PageLookStudio, PageLookSkins, PageGlowFrame
};

Theme &HostTheme(GalleryState &s);
const Theme &HostTheme(const GalleryState &s);
AnimationState &HostAnimation(GalleryState &s);
float HostScale(const GalleryState &s);
const IconAtlas &HostIcons(const GalleryState &s);
ImTextureRef HostTexture(const GalleryState &s);
std::span<const ImTextureRef> HostSampleImages(const GalleryState &s);
ImTextureRef HostSpriteSheet(const GalleryState &s);
bool &HostDark(GalleryState &s);
int &HostPresetIndex(GalleryState &s);
int &HostDesignDensity(GalleryState &s);

void Record(GalleryState &s, const char *name);
void RecordProbe(GalleryState &s, const char *name, ImVec2 min, ImVec2 max);
bool BeginGrid(GalleryState &s, const char *id, int maxColumns = 2);
void EndGrid();
bool BeginSection(GalleryState &s, const char *title, const char *hint = nullptr);
void EndSection(bool visible);

// Restores plain Dear ImGui colours and metrics (fonts and scale stay) for the lifetime of the scope, so a specimen can
// be drawn "before" next to the same calls drawn with an ImKit theme.
class DefaultStyleScope {
  public:
    DefaultStyleScope();
    ~DefaultStyleScope();
    DefaultStyleScope(const DefaultStyleScope &) = delete;
    DefaultStyleScope &operator=(const DefaultStyleScope &) = delete;

  private:
    ImGuiStyle previous_;
};
} // namespace imkit::gallery
