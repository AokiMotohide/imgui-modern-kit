#include <imkit/decor.h>
#include <algorithm>
#include <climits>
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

int SpriteFrameAt(const SpriteSheet& sheet, double elapsedSeconds, bool animate) {
    const int frames = std::max(sheet.frames, 1);
    const int rest = std::clamp(sheet.restFrame, 0, frames - 1);
    if (!animate || !(sheet.fps > 0.f) || !std::isfinite(elapsedSeconds) || frames == 1)
        return rest;
    const double position = std::max(elapsedSeconds, 0.0) * static_cast<double>(sheet.fps);
    if (position >= static_cast<double>(INT_MAX))
        return sheet.playback == SpritePlayback::Loop ? 0 : (sheet.playback == SpritePlayback::Once ? rest : frames - 1);
    const int index = static_cast<int>(position);
    switch (sheet.playback) {
    case SpritePlayback::Loop:
        return index % frames;
    case SpritePlayback::Once:
        return index < frames ? index : rest;
    case SpritePlayback::HoldLast:
        return std::min(index, frames - 1);
    }
    return rest;
}

bool SpriteFrameUV(const SpriteSheet& sheet, int frame, ImVec2& uv0, ImVec2& uv1) {
    if (sheet.frames < 1 || sheet.columns < 1 || frame < 0 || frame >= sheet.frames || !(sheet.cellPixels.x > 0.f) ||
        !(sheet.cellPixels.y > 0.f) || !(sheet.sheetPixels.x > 0.f) || !(sheet.sheetPixels.y > 0.f))
        return false;
    const int column = frame % sheet.columns, row = frame / sheet.columns;
    // The cell must lie inside the sheet; a frame that would sample outside it is refused rather than clamped.
    const float right = static_cast<float>(column + 1) * sheet.cellPixels.x;
    const float bottom = static_cast<float>(row + 1) * sheet.cellPixels.y;
    if (right > sheet.sheetPixels.x + 1e-3f || bottom > sheet.sheetPixels.y + 1e-3f)
        return false;
    const float uSpan = sheet.uvMax.x - sheet.uvMin.x, vSpan = sheet.uvMax.y - sheet.uvMin.y;
    uv0 = {sheet.uvMin.x + uSpan * static_cast<float>(column) * sheet.cellPixels.x / sheet.sheetPixels.x,
           sheet.uvMin.y + vSpan * static_cast<float>(row) * sheet.cellPixels.y / sheet.sheetPixels.y};
    uv1 = {sheet.uvMin.x + uSpan * right / sheet.sheetPixels.x, sheet.uvMin.y + vSpan * bottom / sheet.sheetPixels.y};
    return true;
}

void DrawSprite(ImDrawList* draw, const SpriteSheet& sheet, int frame, ImVec2 minimum, ImVec2 maximum, ImU32 tint) {
    ImVec2 uv0, uv1;
    if (!draw || !SpriteFrameUV(sheet, frame, uv0, uv1) || !(maximum.x > minimum.x) || !(maximum.y > minimum.y))
        return;
    draw->AddImage(sheet.texture, minimum, maximum, uv0, uv1, tint);
}

int Sprite(const char* id, const SpriteSheet& sheet, SpriteState& state, double now, ImVec2 size, bool animate) {
    ImGui::PushID(id);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    const ImVec2 maximum{position.x + size.x, position.y + size.y};
    if (state.start < 0.0)
        state.start = now;
    const int frame = SpriteFrameAt(sheet, now - state.start, animate);
    if (ImGui::IsRectVisible(position, maximum))
        DrawSprite(ImGui::GetWindowDrawList(), sheet, frame, position, maximum);
    ImGui::PopID();
    return frame;
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
