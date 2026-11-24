#pragma once
#include <imgui.h>
#include <cstdint>
#include <initializer_list>

namespace imkit {
struct PartMetrics;
enum class OverlayAnchor;
enum class SurfaceStyle : std::uint8_t;
} // namespace imkit

// Small drawing helpers shared by the component implementations. Everything is
// stateless and draws into the supplied list; nothing here owns ImGui state.
namespace imkit::detail {
// Component sizes of the theme applied to the current context; defaults when none was applied.
const PartMetrics& CurrentParts();
void SetCurrentParts(const PartMetrics& parts);
// A frame border of OutlineMinimumWidth or more is an outline, not a hairline (see theme.h). ApplyTheme truncates scaled
// borders, so the minimum is scaled and truncated the same way. This reads the style, so it works without a Theme.
bool IsOutlined(const ImGuiStyle& style);
// The width of a thin frame line: the outline width when the theme is outlined, otherwise `thin`. The line colour of
// components is already the full-strength border colour, so only the width needs to follow the theme.
float FrameLineWidth(float thin);
// The alpha of a thin frame line: full strength when outlined, so thick outlines are not washed out.
float FrameLineAlpha(float thin);
// Strokes a shape in the border colour at the style's border width when the theme is outlined; draws nothing otherwise.
void DrawOutline(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding, ImDrawFlags flags = ImDrawFlags_None);
// The same for a ring: a border-coloured circle wider than the ring by the outline on both sides. Draw it before the ring.
void DrawRingOutline(ImDrawList* draw, ImVec2 center, float radius, float thickness, int segments);
inline ImVec4 MixColor(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
inline ImVec4 WithAlpha(ImVec4 c, float alpha) {
    c.w *= alpha;
    return c;
}
// Chevron pointing in `direction` (Left, Right, Up or Down), centred on `center`.
void DrawChevron(ImDrawList* draw, ImVec2 center, float size, ImU32 color, ImGuiDir direction, float thickness = 1.6f);
void DrawCheck(ImDrawList* draw, ImVec2 center, float size, ImU32 color, float thickness = 1.8f);
// The point an overlay is pinned to and its pivot, for SetNextWindowPos or for placing a box of known size at
// point - pivot * size. The margin is kept to at most half the work area on each axis.
struct OverlayPlacement { ImVec2 point, pivot; bool bottom; };
OverlayPlacement PlaceOverlay(OverlayAnchor anchor, ImVec2 workPos, ImVec2 workSize, float margin);
void DrawCross(ImDrawList* draw, ImVec2 center, float size, ImU32 color, float thickness = 1.6f);
// Stroked arc with round caps; angles in radians, thickness in pixels.
void DrawRingArc(ImDrawList* draw, ImVec2 c, float radius, float thickness, float a0, float a1, ImU32 color);
// Soft shadow around a rectangle: concentric hairline rings fading outwards. The
// caller must have a clip rectangle that allows drawing outside the rectangle.
void DrawSoftShadow(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding, float extent,
                    float peakAlpha, ImVec4 color = {0, 0, 0, 1});
// The weakest mix of `from` toward `to`, starting at `start`, that reaches `ratio` against every background; `to` when
// none does. Surfaces, tray tints and secondary text all use this one rule, so "readable" means the same everywhere.
ImVec4 MixToContrast(ImVec4 from, ImVec4 to, float start, float ratio, std::initializer_list<ImVec4> backgrounds);
// OKLCH (Björn Ottosson's OKLab in polar form): L is lightness as the eye sees it (0..1), C chroma, h hue in degrees.
// Surfaces are stepped in L so that neighbouring regions look evenly apart in every theme.
struct Oklch { float L = 0, C = 0, h = 0; };
float SrgbToLinear(float v);
float LinearToSrgb(float v);
Oklch ToOklch(ImVec4 c);
// Back to sRGB. A chroma outside the sRGB gamut is reduced until the colour fits, keeping lightness and hue.
ImVec4 FromOklch(Oklch o, float alpha = 1.f);
// The signed hue step from `from` toward `to`, at most `limit` degrees.
float HueToward(float from, float to, float limit);
// White or a near black, whichever reads better on `background`.
ImVec4 InkOn(ImVec4 background);
// `color` itself when it reads on `background` (4.5:1), otherwise the weakest mix toward `text` that does.
ImVec4 ReadableOn(ImVec4 color, ImVec4 background, ImVec4 text);

// Smooth corners. A circular arc meets the straight edge with a sudden change of curvature, which reads as a bump on
// buttons; these corners start curving earlier (k = 1.28 r from the corner) and ease into the edge with one cubic
// Bezier per corner, the continuous-curvature look of a superellipse. `radius` is the radius a circular corner would
// have; it is limited to what fits the rectangle.
float SmoothCornerReach(ImVec2 a, ImVec2 b, float radius);
void PathSmoothRect(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius);
void AddSmoothRectFilled(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float radius);
void AddSmoothRect(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float radius, float thickness);
void AddSmoothRectGradient(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 colTop, ImU32 colBottom, float radius);
void AddTopSpecular(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius, float alpha);
void DrawLayeredShadow(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding, float extent,
                       float peakAlpha, ImVec4 color = {0, 0, 0, 1});
void DrawSurfaceRect(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec4 fill, float radius, SurfaceStyle style, bool isDark,
                     bool elevated = false);
void DrawFocusGlow(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius, float amount, ImVec4 accent, float width);
// Dashes along a smooth-cornered rectangle, the dash phase running continuously around the shape.
void DrawDashedRect(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius, ImU32 color, float thickness, float dash);
// A dotted line: round dots `gap` apart, for links that may be skipped.
void DrawDottedLine(ImDrawList* draw, ImVec2 from, ImVec2 to, ImU32 color, float thickness, float gap);
} // namespace imkit::detail
