#pragma once
#include <imgui.h>
#include <imkit/theme.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>

// Small drawing helpers shared by the component implementations. Everything is
// stateless and draws into the supplied list; nothing here owns ImGui state.
namespace imkit::detail {
// Component sizes of the theme applied to the current context; defaults when none was applied.
const PartMetrics& CurrentParts();
void SetCurrentParts(const PartMetrics& parts);
// A frame border of OutlineMinimumWidth or more is an outline, not a hairline (see theme.h). ApplyTheme truncates scaled
// borders, so the minimum is scaled and truncated the same way. This reads the style, so it works without a Theme.
inline bool IsOutlined(const ImGuiStyle& style) {
    return style.FrameBorderSize > 0.f &&
           style.FrameBorderSize >= std::trunc(OutlineMinimumWidth * style.FontScaleMain);
}
// The width of a thin frame line: the outline width when the theme is outlined, otherwise `thin`. The line colour of
// components is already the full-strength border colour, so only the width needs to follow the theme.
inline float FrameLineWidth(float thin) {
    const auto& style = ImGui::GetStyle();
    return IsOutlined(style) ? style.FrameBorderSize : thin;
}
// The alpha of a thin frame line: full strength when outlined, so thick outlines are not washed out.
inline float FrameLineAlpha(float thin) {
    return IsOutlined(ImGui::GetStyle()) ? 1.f : thin;
}
// Strokes a shape in the border colour at the style's border width when the theme is outlined; draws nothing otherwise.
inline void DrawOutline(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding, ImDrawFlags flags = ImDrawFlags_None) {
    const auto& style = ImGui::GetStyle();
    if (IsOutlined(style))
        draw->AddRect(minimum, maximum, ImGui::GetColorU32(ImGuiCol_Border), rounding, flags, style.FrameBorderSize);
}
// The same for a ring: a border-coloured circle wider than the ring by the outline on both sides. Draw it before the ring.
inline void DrawRingOutline(ImDrawList* draw, ImVec2 center, float radius, float thickness, int segments) {
    const auto& style = ImGui::GetStyle();
    if (IsOutlined(style))
        draw->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_Border), segments, thickness + style.FrameBorderSize * 2.f);
}
inline ImVec4 MixColor(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
inline ImVec4 WithAlpha(ImVec4 c, float alpha) {
    c.w *= alpha;
    return c;
}
// Chevron pointing in `direction` (Left, Right, Up or Down), centred on `center`.
inline void DrawChevron(ImDrawList* draw, ImVec2 center, float size, ImU32 color, ImGuiDir direction,
                        float thickness = 1.6f) {
    const float h = size * .5f, w = size * .3f;
    ImVec2 a, b, c;
    switch (direction) {
    case ImGuiDir_Left:  a = {center.x + w * .5f, center.y - h}; b = {center.x - w * .5f, center.y}; c = {a.x, center.y + h}; break;
    case ImGuiDir_Up:    a = {center.x - h, center.y + w * .5f}; b = {center.x, center.y - w * .5f}; c = {center.x + h, a.y}; break;
    case ImGuiDir_Down:  a = {center.x - h, center.y - w * .5f}; b = {center.x, center.y + w * .5f}; c = {center.x + h, a.y}; break;
    default:             a = {center.x - w * .5f, center.y - h}; b = {center.x + w * .5f, center.y}; c = {a.x, center.y + h}; break;
    }
    draw->PathLineTo(a);
    draw->PathLineTo(b);
    draw->PathLineTo(c);
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
inline void DrawCheck(ImDrawList* draw, ImVec2 center, float size, ImU32 color, float thickness = 1.8f) {
    const float s = size * .5f;
    draw->PathLineTo({center.x - s * .8f, center.y + s * .05f});
    draw->PathLineTo({center.x - s * .25f, center.y + s * .55f});
    draw->PathLineTo({center.x + s * .85f, center.y - s * .5f});
    draw->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
}
// The point an overlay is pinned to and its pivot, for SetNextWindowPos or for placing a box of known size at
// point - pivot * size. The margin is kept to at most half the work area on each axis.
struct OverlayPlacement { ImVec2 point, pivot; bool bottom; };
inline OverlayPlacement PlaceOverlay(OverlayAnchor anchor, ImVec2 workPos, ImVec2 workSize, float margin) {
    const int index = std::clamp(static_cast<int>(anchor), 0, 5);
    const int column = index % 3;
    const bool bottom = index >= 3;
    margin = std::isfinite(margin) ? std::max(margin, 0.f) : 0.f;
    const float mx = std::min(margin, workSize.x * .5f), my = std::min(margin, workSize.y * .5f);
    const float x = column == 0 ? workPos.x + mx : column == 1 ? workPos.x + workSize.x * .5f : workPos.x + workSize.x - mx;
    const float y = bottom ? workPos.y + workSize.y - my : workPos.y + my;
    return {{x, y}, {column * .5f, bottom ? 1.f : 0.f}, bottom};
}
inline void DrawCross(ImDrawList* draw, ImVec2 center, float size, ImU32 color, float thickness = 1.6f) {
    const float s = size * .5f;
    draw->AddLine({center.x - s, center.y - s}, {center.x + s, center.y + s}, color, thickness);
    draw->AddLine({center.x + s, center.y - s}, {center.x - s, center.y + s}, color, thickness);
}
// Stroked arc with round caps; angles in radians, thickness in pixels.
inline void DrawRingArc(ImDrawList* draw, ImVec2 c, float radius, float thickness, float a0, float a1, ImU32 color) {
    if (a1 <= a0)
        return;
    const int segments = std::max(4, static_cast<int>((a1 - a0) * radius / 3.f));
    draw->PathArcTo(c, radius, a0, a1, segments);
    draw->PathStroke(color, ImDrawFlags_None, thickness);
    for (float angle : {a0, a1})
        draw->AddCircleFilled({c.x + std::cos(angle) * radius, c.y + std::sin(angle) * radius}, thickness * .5f, color, 16);
}
// Soft shadow around a rectangle: concentric hairline rings fading outwards. The
// caller must have a clip rectangle that allows drawing outside the rectangle.
inline void DrawSoftShadow(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding, float extent,
                           float peakAlpha, ImVec4 color = {0, 0, 0, 1}) {
    const int steps = std::clamp(static_cast<int>(std::ceil(extent)), 1, 16);
    const float step = extent / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        const float falloff = 1.f - (static_cast<float>(i) + .5f) / static_cast<float>(steps);
        const float spread = step * static_cast<float>(i) + step * .5f;
        draw->AddRect({minimum.x - spread, minimum.y - spread}, {maximum.x + spread, maximum.y + spread},
                      ImGui::ColorConvertFloat4ToU32({color.x, color.y, color.z, peakAlpha * falloff * falloff}), rounding + spread,
                      ImDrawFlags_None, step + .5f);
    }
}
// The weakest mix of `from` toward `to`, starting at `start`, that reaches `ratio` against every background; `to` when
// none does. Surfaces, tray tints and secondary text all use this one rule, so "readable" means the same everywhere.
inline ImVec4 MixToContrast(ImVec4 from, ImVec4 to, float start, float ratio, std::initializer_list<ImVec4> backgrounds) {
    for (float k = std::clamp(start, 0.f, 1.f); k < 1.f; k += .02f) {
        const ImVec4 c = MixColor(from, to, k);
        if (std::all_of(backgrounds.begin(), backgrounds.end(), [&](ImVec4 b) { return ContrastRatio(c, b) >= ratio; }))
            return c;
    }
    return to;
}
// OKLCH (Björn Ottosson's OKLab in polar form): L is lightness as the eye sees it (0..1), C chroma, h hue in degrees.
// Surfaces are stepped in L so that neighbouring regions look evenly apart in every theme.
struct Oklch { float L = 0, C = 0, h = 0; };
inline float SrgbToLinear(float v) { return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f); }
inline float LinearToSrgb(float v) { return v <= .0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.f / 2.4f) - .055f; }
inline Oklch ToOklch(ImVec4 c) {
    const float r = SrgbToLinear(c.x), g = SrgbToLinear(c.y), b = SrgbToLinear(c.z);
    const float l = std::cbrt(.4122214708f * r + .5363325363f * g + .0514459929f * b);
    const float m = std::cbrt(.2119034982f * r + .6806995451f * g + .1073969566f * b);
    const float s = std::cbrt(.0883024619f * r + .2817188376f * g + .6299787005f * b);
    const float L = .2104542553f * l + .7936177850f * m - .0040720468f * s;
    const float A = 1.9779984951f * l - 2.4285922050f * m + .4505937099f * s;
    const float B = .0259040371f * l + .7827717662f * m - .8086757660f * s;
    float h = std::atan2(B, A) * 57.29577951f;
    if (h < 0) h += 360.f;
    return {L, std::hypot(A, B), h};
}
// Back to sRGB. A chroma outside the sRGB gamut is reduced until the colour fits, keeping lightness and hue.
inline ImVec4 FromOklch(Oklch o, float alpha = 1.f) {
    const auto convert = [&](float chroma, bool& inside) {
        const float rad = o.h * .01745329252f, A = chroma * std::cos(rad), B = chroma * std::sin(rad);
        const float l = std::pow(o.L + .3963377774f * A + .2158037573f * B, 3.f);
        const float m = std::pow(o.L - .1055613458f * A - .0638541728f * B, 3.f);
        const float s = std::pow(o.L - .0894841775f * A - 1.2914855480f * B, 3.f);
        const float lin[3] = {4.0767416621f * l - 3.3077115913f * m + .2309699292f * s,
                              -1.2684380046f * l + 2.6097574011f * m - .3413193965f * s,
                              -.0041960863f * l - .7034186147f * m + 1.7076147010f * s};
        inside = true;
        for (float v : lin) inside = inside && v >= -1e-4f && v <= 1.0001f;
        return ImVec4{LinearToSrgb(std::clamp(lin[0], 0.f, 1.f)), LinearToSrgb(std::clamp(lin[1], 0.f, 1.f)),
                      LinearToSrgb(std::clamp(lin[2], 0.f, 1.f)), alpha};
    };
    o.L = std::clamp(o.L, 0.f, 1.f);
    float chroma = std::max(0.f, o.C);
    for (int i = 0; i < 24; ++i, chroma *= .85f) {
        bool inside = false;
        const ImVec4 c = convert(chroma, inside);
        if (inside) return c;
    }
    bool ignored = false;
    return convert(0.f, ignored);
}
// The signed hue step from `from` toward `to`, at most `limit` degrees.
inline float HueToward(float from, float to, float limit) {
    float d = std::fmod(to - from + 540.f, 360.f) - 180.f;
    return std::clamp(d, -limit, limit);
}
// White or a near black, whichever reads better on `background`.
inline ImVec4 InkOn(ImVec4 background) {
    background.w = 1.f;
    const ImVec4 light{1, 1, 1, 1}, dark{.07f, .07f, .08f, 1};
    return ContrastRatio(light, background) >= ContrastRatio(dark, background) ? light : dark;
}
// `color` itself when it reads on `background` (4.5:1), otherwise the weakest mix toward `text` that does.
inline ImVec4 ReadableOn(ImVec4 color, ImVec4 background, ImVec4 text) {
    return MixToContrast(color, text, 0.f, 4.5f, {background});
}

// Smooth corners. A circular arc meets the straight edge with a sudden change of curvature, which reads as a bump on
// buttons; these corners start curving earlier (k = 1.28 r from the corner) and ease into the edge with one cubic
// Bezier per corner, the continuous-curvature look of a superellipse. `radius` is the radius a circular corner would
// have; it is limited to what fits the rectangle.
inline float SmoothCornerReach(ImVec2 a, ImVec2 b, float radius) {
    return std::max(0.f, std::min(radius * 1.28f, std::min(b.x - a.x, b.y - a.y) * .5f));
}
inline void PathSmoothRect(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius) {
    const float k = SmoothCornerReach(a, b, radius);
    if (k <= .5f) {
        draw->PathRect(a, b);
        return;
    }
    const float c = k * (.62f / 1.28f); // Control handle length: 0.62 r for the full reach.
    draw->PathLineTo({a.x + k, a.y});
    draw->PathLineTo({b.x - k, a.y});
    draw->PathBezierCubicCurveTo({b.x - k + c, a.y}, {b.x, a.y + k - c}, {b.x, a.y + k});
    draw->PathLineTo({b.x, b.y - k});
    draw->PathBezierCubicCurveTo({b.x, b.y - k + c}, {b.x - k + c, b.y}, {b.x - k, b.y});
    draw->PathLineTo({a.x + k, b.y});
    draw->PathBezierCubicCurveTo({a.x + k - c, b.y}, {a.x, b.y - k + c}, {a.x, b.y - k});
    draw->PathLineTo({a.x, a.y + k});
    draw->PathBezierCubicCurveTo({a.x, a.y + k - c}, {a.x + k - c, a.y}, {a.x + k, a.y});
}
inline void AddSmoothRectFilled(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float radius) {
    if ((color & IM_COL32_A_MASK) == 0 || b.x <= a.x || b.y <= a.y)
        return;
    PathSmoothRect(draw, a, b, radius);
    draw->PathFillConvex(color);
}
inline void AddSmoothRect(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float radius, float thickness) {
    if ((color & IM_COL32_A_MASK) == 0 || b.x <= a.x || b.y <= a.y)
        return;
    // Stroke on the centre line of the border, like ImDrawList::AddRect.
    const float h = thickness * .5f;
    PathSmoothRect(draw, {a.x + h, a.y + h}, {b.x - h, b.y - h}, std::max(0.f, radius - h));
    draw->PathStroke(color, ImDrawFlags_Closed, thickness);
}
// Dashes along a smooth-cornered rectangle, the dash phase running continuously around the shape.
inline void DrawDashedRect(ImDrawList* draw, ImVec2 a, ImVec2 b, float radius, ImU32 color, float thickness, float dash) {
    PathSmoothRect(draw, a, b, radius);
    ImVector<ImVec2> path;
    path.swap(draw->_Path);
    draw->PathClear();
    if (path.Size < 2)
        return;
    path.push_back(path[0]);
    dash = std::max(1.f, dash);
    const float gap = dash * .8f;
    float phase = 0; // Distance into the current dash+gap period.
    for (int i = 1; i < path.Size; ++i) {
        ImVec2 p = path[i - 1];
        const ImVec2 q = path[i];
        float length = std::hypot(q.x - p.x, q.y - p.y);
        const ImVec2 dir = length > 0 ? ImVec2{(q.x - p.x) / length, (q.y - p.y) / length} : ImVec2{0, 0};
        while (length > 0) {
            const bool on = phase < dash;
            const float step = std::min(length, on ? dash - phase : dash + gap - phase);
            const ImVec2 e{p.x + dir.x * step, p.y + dir.y * step};
            if (on)
                draw->AddLine(p, e, color, thickness);
            p = e;
            length -= step;
            phase += step;
            if (phase >= dash + gap)
                phase = 0;
        }
    }
}
// A dotted line: round dots `gap` apart, for links that may be skipped.
inline void DrawDottedLine(ImDrawList* draw, ImVec2 from, ImVec2 to, ImU32 color, float thickness, float gap) {
    const float length = std::hypot(to.x - from.x, to.y - from.y);
    if (length <= 0)
        return;
    gap = std::max(gap, thickness * 1.5f);
    const int count = std::max(1, static_cast<int>(length / gap));
    for (int i = 0; i <= count; ++i) {
        const float f = static_cast<float>(i) / static_cast<float>(count);
        draw->AddCircleFilled({from.x + (to.x - from.x) * f, from.y + (to.y - from.y) * f}, thickness * .5f, color, 8);
    }
}
} // namespace imkit::detail
