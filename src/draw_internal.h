#pragma once
#include <imgui.h>
#include <algorithm>
#include <cmath>

// Small drawing helpers shared by the component implementations. Everything is
// stateless and draws into the supplied list; nothing here owns ImGui state.
namespace imkit::detail {
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
                           float peakAlpha) {
    const int steps = std::clamp(static_cast<int>(std::ceil(extent)), 1, 16);
    const float step = extent / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        const float falloff = 1.f - (static_cast<float>(i) + .5f) / static_cast<float>(steps);
        const float spread = step * static_cast<float>(i) + step * .5f;
        draw->AddRect({minimum.x - spread, minimum.y - spread}, {maximum.x + spread, maximum.y + spread},
                      ImGui::ColorConvertFloat4ToU32({0, 0, 0, peakAlpha * falloff * falloff}), rounding + spread,
                      ImDrawFlags_None, step + .5f);
    }
}
// Rounded rectangle outline made of dashes; corners stay solid so the shape still reads as rounded.
inline void DrawDashedRect(ImDrawList* draw, ImVec2 a, ImVec2 b, float rounding, ImU32 color, float thickness, float dash) {
    constexpr float pi = 3.14159265358979323846f;
    rounding = std::min(rounding, std::min(b.x - a.x, b.y - a.y) * .5f);
    const ImVec2 corners[4] = {{a.x + rounding, a.y + rounding}, {b.x - rounding, a.y + rounding},
                               {b.x - rounding, b.y - rounding}, {a.x + rounding, b.y - rounding}};
    for (int i = 0; i < 4; ++i) {
        draw->PathArcTo(corners[i], rounding, pi + static_cast<float>(i) * pi * .5f, pi * 1.5f + static_cast<float>(i) * pi * .5f, 6);
        draw->PathStroke(color, 0, thickness);
    }
    auto edge = [&](ImVec2 from, ImVec2 to) {
        const float length = std::hypot(to.x - from.x, to.y - from.y);
        for (float t = 0; t < length; t += dash * 2) {
            const float e = std::min(length, t + dash);
            draw->AddLine({from.x + (to.x - from.x) * t / length, from.y + (to.y - from.y) * t / length},
                          {from.x + (to.x - from.x) * e / length, from.y + (to.y - from.y) * e / length}, color, thickness);
        }
    };
    edge({a.x + rounding, a.y}, {b.x - rounding, a.y});
    edge({b.x, a.y + rounding}, {b.x, b.y - rounding});
    edge({b.x - rounding, b.y}, {a.x + rounding, b.y});
    edge({a.x, b.y - rounding}, {a.x, a.y + rounding});
}
} // namespace imkit::detail
