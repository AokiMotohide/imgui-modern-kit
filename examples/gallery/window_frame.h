#pragma once
#include <imkit/theme.h>
#include <string>
#include <string_view>

namespace imkit::gallery {
// Gallery prototype only. Coordinates are client pixels, independent of UI zoom.
enum class FrameAction { None, Minimize, MaximizeRestore, Close };
struct FrameRect {
    ImVec2 min{}, max{};
    bool Contains(ImVec2 p) const { return p.x>=min.x && p.y>=min.y && p.x<max.x && p.y<max.y; }
};
struct FrameLayout {
    FrameRect title, icon, buttons[3];
    float scale=1;
};
struct FrameState {
    bool active=true, maximized=false;
    int hovered=-1, pressed=-1;
    FrameAction requested=FrameAction::None;
};
struct FrameResult { FrameLayout layout; FrameAction action; };
FrameLayout LayoutWindowFrame(float width, float dpiScale);
std::string ElideWindowTitle(std::string_view title, float width, ImFont* font, float size);
FrameResult DrawWindowFrame(const Theme& theme, std::string_view title,
                            const FrameLayout& layout, const FrameState& state);
}
