#include <imkit/decor.h>
#include <algorithm>
#include <cmath>

namespace imkit {
int NineSliceQuads(ImVec2 minimum, ImVec2 maximum, ImVec2 texturePixels, NineSliceBorder border, float scale,
                   ImVec2 uvMin, ImVec2 uvMax, NineSliceQuad (&quads)[9]) {
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    if (!(width > 0.f) || !(height > 0.f) || !(texturePixels.x > 0.f) || !(texturePixels.y > 0.f) || !(scale > 0.f))
        return 0;
    // Borders cannot take more than the whole picture; if they do, shrink the pair together.
    const auto fit = [](float& first, float& second, float whole) {
        first = std::max(first, 0.f);
        second = std::max(second, 0.f);
        if (first + second > whole) {
            const float factor = whole / (first + second);
            first *= factor;
            second *= factor;
        }
    };
    fit(border.left, border.right, texturePixels.x);
    fit(border.top, border.bottom, texturePixels.y);
    // On screen the corners keep their size, unless the destination is smaller than both corners together.
    float left = border.left * scale, right = border.right * scale, top = border.top * scale, bottom = border.bottom * scale;
    fit(left, right, width);
    fit(top, bottom, height);
    const float uSpan = uvMax.x - uvMin.x, vSpan = uvMax.y - uvMin.y;
    const float xs[4] = {minimum.x, minimum.x + left, maximum.x - right, maximum.x};
    const float ys[4] = {minimum.y, minimum.y + top, maximum.y - bottom, maximum.y};
    const float us[4] = {uvMin.x, uvMin.x + uSpan * border.left / texturePixels.x, uvMax.x - uSpan * border.right / texturePixels.x, uvMax.x};
    const float vs[4] = {uvMin.y, uvMin.y + vSpan * border.top / texturePixels.y, uvMax.y - vSpan * border.bottom / texturePixels.y, uvMax.y};
    int count = 0;
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) {
            if (!(xs[column + 1] > xs[column]) || !(ys[row + 1] > ys[row]))
                continue;
            quads[count++] = {{xs[column], ys[row]}, {xs[column + 1], ys[row + 1]}, {us[column], vs[row]}, {us[column + 1], vs[row + 1]}};
        }
    return count;
}

void DrawNineSlice(ImDrawList* draw, ImTextureRef texture, ImVec2 minimum, ImVec2 maximum, ImVec2 texturePixels,
                   NineSliceBorder border, float scale, ImU32 tint, ImVec2 uvMin, ImVec2 uvMax) {
    NineSliceQuad quads[9];
    const int count = NineSliceQuads(minimum, maximum, texturePixels, border, scale, uvMin, uvMax, quads);
    for (int i = 0; i < count; ++i)
        draw->AddImage(texture, quads[i].min, quads[i].max, quads[i].uv0, quads[i].uv1, tint);
}

namespace {
constexpr int kMaxDots = 20000;
constexpr int kMaxStripes = 4000;
constexpr float kPi = 3.14159265358979323846f;

void DrawDots(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const BackdropStyle& style) {
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    float spacing = std::max(style.spacing, 2.f);
    // Widen the grid rather than flood the draw list with dots.
    while ((std::ceil(width / spacing) + 1.f) * (std::ceil(height / spacing) + 1.f) > static_cast<float>(kMaxDots))
        spacing *= 1.25f;
    const float radius = std::max(style.size, 0.5f) * .5f;
    const int rows = static_cast<int>(std::ceil(height / spacing)) + 1, columns = static_cast<int>(std::ceil(width / spacing)) + 1;
    for (int row = 0; row < rows; ++row) {
        const float y = minimum.y + static_cast<float>(row) * spacing;
        const float shift = (row & 1) ? spacing * .5f : 0.f; // Staggered rows read as halftone, not as a grid.
        for (int column = 0; column < columns; ++column)
            draw->AddCircleFilled({minimum.x + shift + static_cast<float>(column) * spacing, y}, radius, style.color, 8);
    }
}

void DrawStripes(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const BackdropStyle& style) {
    const ImVec2 center{(minimum.x + maximum.x) * .5f, (minimum.y + maximum.y) * .5f};
    const float halfDiagonal = std::hypot(maximum.x - minimum.x, maximum.y - minimum.y) * .5f;
    const ImVec2 direction{std::cos(style.angle), std::sin(style.angle)}, normal{-direction.y, direction.x};
    float spacing = std::max(style.spacing, 2.f);
    while (halfDiagonal * 2.f / spacing > static_cast<float>(kMaxStripes))
        spacing *= 1.25f;
    const float thickness = std::max(style.size, 0.5f);
    // Bands are laid out along the normal, starting from the centre line so the pattern does not jump when resized.
    const int steps = static_cast<int>(std::ceil(halfDiagonal / spacing));
    for (int i = -steps; i <= steps; ++i) {
        const float offset = static_cast<float>(i) * spacing;
        const ImVec2 middle{center.x + normal.x * offset, center.y + normal.y * offset};
        draw->AddLine({middle.x - direction.x * halfDiagonal, middle.y - direction.y * halfDiagonal},
                      {middle.x + direction.x * halfDiagonal, middle.y + direction.y * halfDiagonal}, style.color, thickness);
    }
}

void DrawRays(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const BackdropStyle& style) {
    const ImVec2 origin{minimum.x + (maximum.x - minimum.x) * style.origin.x, minimum.y + (maximum.y - minimum.y) * style.origin.y};
    float farthest = 0.f;
    for (const ImVec2 corner : {minimum, maximum, ImVec2{minimum.x, maximum.y}, ImVec2{maximum.x, minimum.y}})
        farthest = std::max(farthest, std::hypot(corner.x - origin.x, corner.y - origin.y));
    const int rays = std::clamp(style.rays, 1, 360);
    const float step = kPi / static_cast<float>(rays); // A coloured wedge and an equal gap per ray.
    // The outer edge of a wedge is a chord; push it out so the corners stay covered.
    const float radius = farthest / std::cos(step * .5f) * 1.02f;
    for (int i = 0; i < rays; ++i) {
        const float a0 = style.angle + static_cast<float>(i) * 2.f * step, a1 = a0 + step;
        draw->AddTriangleFilled(origin, {origin.x + std::cos(a0) * radius, origin.y + std::sin(a0) * radius},
                                {origin.x + std::cos(a1) * radius, origin.y + std::sin(a1) * radius}, style.color);
    }
}
} // namespace

void DrawBackdropPattern(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const BackdropStyle& style) {
    if (!(maximum.x > minimum.x) || !(maximum.y > minimum.y) || (style.color >> IM_COL32_A_SHIFT) == 0)
        return;
    draw->PushClipRect(minimum, maximum, true);
    switch (style.pattern) {
    case BackdropPattern::Dots: DrawDots(draw, minimum, maximum, style); break;
    case BackdropPattern::Stripes: DrawStripes(draw, minimum, maximum, style); break;
    case BackdropPattern::Rays: DrawRays(draw, minimum, maximum, style); break;
    }
    draw->PopClipRect();
}
} // namespace imkit
