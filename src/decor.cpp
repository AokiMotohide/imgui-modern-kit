#include <imkit/decor.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <vector>

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

// ---- Glow frame ----------------------------------------------------------------------------------------------
namespace {
constexpr float kTwoPi = 2.f * kPi;
float Finite(float value, float fallback) { return std::isfinite(value) ? value : fallback; }
float Fraction(double value) { return static_cast<float>(value - std::floor(value)); }

GlowFrameStyle Sanitize(GlowFrameStyle s) {
    s.intensity = std::clamp(Finite(s.intensity, .8f), 0.f, 1.f);
    s.minimumIntensity = std::clamp(Finite(s.minimumIntensity, .35f), 0.f, 1.f);
    s.period = std::clamp(Finite(s.period, 2.4f), .05f, 3600.f);
    s.spread = std::clamp(Finite(s.spread, 28.f), 0.f, 4096.f);
    s.falloff = std::clamp(Finite(s.falloff, 2.f), .1f, 8.f);
    s.lineThickness = std::clamp(Finite(s.lineThickness, 0.f), 0.f, 256.f);
    s.lineIntensity = std::clamp(Finite(s.lineIntensity, 1.f), 0.f, 1.f);
    s.rounding = std::max(Finite(s.rounding, 0.f), 0.f);
    s.orbitHeads = std::clamp(s.orbitHeads, 1, 8);
    s.orbitLength = std::clamp(Finite(s.orbitLength, .18f), .005f, 1.f);
    s.rippleSeconds = std::clamp(Finite(s.rippleSeconds, .9f), .05f, 60.f);
    s.rippleDistance = std::clamp(Finite(s.rippleDistance, 90.f), 0.f, 4096.f);
    s.rippleInterval = std::max(Finite(s.rippleInterval, 0.f), 0.f);
    s.fadeSeconds = std::max(Finite(s.fadeSeconds, 0.f), 0.f);
    return s;
}

// One closed outline of a rounded rectangle, moved `offset` pixels inward (negative: outward). Every outline built
// from the same shape has the same number of points in the same order, so neighbouring outlines form a strip.
struct GlowShape {
    ImVec2 minimum, maximum;
    float rounding = 0;
    int arcSegments = 2;
    int edgeSegments[4]{1, 1, 1, 1}; // Top, right, bottom, left.
    int Count() const {
        return 4 * (arcSegments + 1) + edgeSegments[0] + edgeSegments[1] + edgeSegments[2] + edgeSegments[3] - 4;
    }
};
void Outline(const GlowShape& shape, float offset, std::vector<ImVec2>& out) {
    out.clear();
    const float half = std::min(shape.maximum.x - shape.minimum.x, shape.maximum.y - shape.minimum.y) * .5f;
    offset = std::min(offset, half);
    // The corner centre sits inside by max(rounding, offset), so an inset larger than the radius gives a sharp corner.
    const float inset = std::min(std::max(shape.rounding, offset), half);
    const float radius = std::max(inset - offset, 0.f);
    const ImVec2 centres[4] = {{shape.minimum.x + inset, shape.minimum.y + inset},
                               {shape.maximum.x - inset, shape.minimum.y + inset},
                               {shape.maximum.x - inset, shape.maximum.y - inset},
                               {shape.minimum.x + inset, shape.maximum.y - inset}};
    const float starts[4] = {kPi, kPi * 1.5f, 0.f, kPi * .5f};
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= shape.arcSegments; ++i) {
            const float angle = starts[corner] + kPi * .5f * static_cast<float>(i) / static_cast<float>(shape.arcSegments);
            out.push_back({centres[corner].x + std::cos(angle) * radius, centres[corner].y + std::sin(angle) * radius});
        }
        // The straight edge to the next corner, split so the brightness can vary along it.
        const ImVec2 from = out.back();
        const int next = (corner + 1) % 4;
        const ImVec2 to{centres[next].x + std::cos(starts[next]) * radius, centres[next].y + std::sin(starts[next]) * radius};
        for (int i = 1; i < shape.edgeSegments[corner]; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(shape.edgeSegments[corner]);
            out.push_back({from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t});
        }
    }
}

// A band is a list of outlines (offset) with an opacity each; the strips between them are filled.
struct Ring {
    float offset, alpha;
};
// `weights` scales each point's opacity along the outline (empty: 1).
void DrawRings(ImDrawList* draw, const GlowShape& shape, const Ring* rings, int count, ImVec4 color,
               const std::vector<float>& weights, std::vector<ImVec2>& scratch) {
    const int points = shape.Count();
    if (count < 2 || points < 3 || color.w <= 0.f)
        return;
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    for (int r = 0; r + 1 < count; ++r) {
        if (rings[r].alpha <= 0.f && rings[r + 1].alpha <= 0.f)
            continue;
        draw->PrimReserve(points * 6, points * 2);
        const unsigned int base = draw->_VtxCurrentIdx;
        for (int ring = 0; ring < 2; ++ring) {
            Outline(shape, rings[r + ring].offset, scratch);
            for (int i = 0; i < points; ++i) {
                ImVec4 c = color;
                c.w *= rings[r + ring].alpha * (weights.empty() ? 1.f : weights[static_cast<std::size_t>(i)]);
                draw->PrimWriteVtx(scratch[static_cast<std::size_t>(i)], uv, ImGui::ColorConvertFloat4ToU32(c));
            }
        }
        for (int i = 0; i < points; ++i) {
            const unsigned int j = static_cast<unsigned int>((i + 1) % points), k = static_cast<unsigned int>(i);
            const auto a = static_cast<ImDrawIdx>(base + k), b = static_cast<ImDrawIdx>(base + j);
            const auto c = static_cast<ImDrawIdx>(base + points + j), d = static_cast<ImDrawIdx>(base + points + k);
            draw->PrimWriteIdx(a);
            draw->PrimWriteIdx(b);
            draw->PrimWriteIdx(c);
            draw->PrimWriteIdx(a);
            draw->PrimWriteIdx(c);
            draw->PrimWriteIdx(d);
        }
    }
}

// The soft profile around the outline at `centre`, reaching `inward` and `outward` pixels and fading with `falloff`.
int SoftRings(Ring* rings, int capacity, float centre, float inward, float outward, float falloff, float alpha) {
    int n = 0;
    const int steps = std::clamp(static_cast<int>(std::max(inward, outward) / 3.f), 4, (capacity - 1) / 2);
    for (int i = outward > 0.f ? steps : 0; i >= 1; --i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        rings[n++] = {centre - outward * t, alpha * std::pow(1.f - t, falloff)};
    }
    rings[n++] = {centre, alpha};
    for (int i = 1; inward > 0.f && i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        rings[n++] = {centre + inward * t, alpha * std::pow(1.f - t, falloff)};
    }
    return n;
}

// Opacity of each outline point for Orbit: bright behind each head, the track level elsewhere.
std::vector<float> OrbitWeights(const GlowShape& shape, const GlowFrameStyle& s, double elapsed) {
    std::vector<ImVec2> base;
    Outline(shape, 0.f, base);
    std::vector<float> along(base.size(), 0.f);
    float perimeter = 0;
    for (std::size_t i = 1; i <= base.size(); ++i) {
        const ImVec2 a = base[i - 1], b = base[i % base.size()];
        perimeter += std::hypot(b.x - a.x, b.y - a.y);
        if (i < base.size())
            along[i] = perimeter;
    }
    const float phase = Fraction(elapsed / s.period), lead = s.orbitLength * .12f;
    std::vector<float> weights(base.size());
    for (std::size_t i = 0; i < base.size(); ++i) {
        const float f = perimeter > 0 ? along[i] / perimeter : 0.f;
        float w = 0;
        for (int h = 0; h < s.orbitHeads; ++h) {
            const float head = Fraction(phase + static_cast<double>(h) / s.orbitHeads);
            const float behind = Fraction(static_cast<double>(head) - f), ahead = Fraction(static_cast<double>(f) - head);
            if (behind < s.orbitLength)
                w = std::max(w, std::pow(1.f - behind / s.orbitLength, 1.5f));
            else if (ahead < lead)
                w = std::max(w, 1.f - ahead / lead);
        }
        weights[i] = s.minimumIntensity + (1.f - s.minimumIntensity) * w;
    }
    return weights;
}
} // namespace

float GlowFrameLevel(const GlowFrameStyle& style, double elapsedSeconds, bool animate) {
    if (!animate || style.motion != GlowMotion::Breathe || !std::isfinite(elapsedSeconds))
        return 1.f;
    const GlowFrameStyle s = Sanitize(style);
    // Starts at the peak, so the frame is at full strength the moment it appears.
    const float wave = .5f + .5f * std::cos(kTwoPi * Fraction(elapsedSeconds / s.period));
    return s.minimumIntensity + (1.f - s.minimumIntensity) * wave;
}

bool DrawGlowFrame(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const GlowFrameStyle& style, GlowFrameState& state,
                   double now, bool active, bool animate) {
    const GlowFrameStyle s = Sanitize(style);
    if (!std::isfinite(now))
        now = state.last >= 0 ? state.last : 0;
    // Visibility fades toward the target, or jumps when motion is off.
    const float target = active ? 1.f : 0.f;
    const double delta = state.last >= 0 ? std::max(now - state.last, 0.0) : 0.0;
    state.last = now;
    if (state.start < 0 || (active && state.visibility <= 0.f))
        state.start = now;
    if (!animate || s.fadeSeconds <= 0.f) {
        state.visibility = target;
    } else {
        const float step = static_cast<float>(delta) / s.fadeSeconds;
        state.visibility = target > state.visibility ? std::min(target, state.visibility + step)
                                                     : std::max(target, state.visibility - step);
    }
    // Ripples: the requested ones, then the automatic interval. The oldest slot is reused.
    const auto addRipple = [&] {
        std::size_t oldest = 0;
        for (std::size_t i = 1; i < 4; ++i)
            if (state.ripples[i] < state.ripples[oldest])
                oldest = i;
        state.ripples[oldest] = now;
    };
    if (active) {
        for (int i = 0; i < std::min(state.pendingPulses, 4); ++i)
            addRipple();
        if (s.rippleInterval > 0.f && (state.lastAutoRipple < 0 || now - state.lastAutoRipple >= s.rippleInterval)) {
            addRipple();
            state.lastAutoRipple = now;
        }
    } else {
        state.lastAutoRipple = -1;
    }
    state.pendingPulses = 0;
    bool ripplesLive = false;
    for (double& ripple : state.ripples) {
        if (ripple >= 0 && now - ripple >= s.rippleSeconds)
            ripple = -1;
        ripplesLive |= ripple >= 0;
    }
    const bool changing = state.visibility != target ||
                          (state.visibility > 0.f &&
                           (ripplesLive || (animate && (s.motion != GlowMotion::Steady || s.rippleInterval > 0.f))));
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    if (!draw || state.visibility <= 0.f || !(width > 1.f) || !(height > 1.f))
        return changing;

    const float shown = state.visibility * state.visibility * (3.f - 2.f * state.visibility);
    const double elapsed = now - state.start;
    const float level = GlowFrameLevel(s, elapsed, animate);
    ImVec4 color = s.color;
    color.w = std::clamp(Finite(color.w, 1.f), 0.f, 1.f) * shown;

    GlowShape shape{minimum, maximum, std::min(s.rounding, std::min(width, height) * .5f)};
    const float outward = s.placement == GlowPlacement::Inside ? 0.f : std::max(s.spread, s.rippleDistance);
    shape.arcSegments = std::clamp(static_cast<int>((shape.rounding + outward) / 4.f) + 2, 2, 16);
    std::vector<float> weights;
    if (s.motion == GlowMotion::Orbit && animate) {
        // Split the straight edges so the heads move smoothly.
        const float across = width - 2 * shape.rounding, down = height - 2 * shape.rounding;
        const float straight[4] = {across, down, across, down};
        for (int i = 0; i < 4; ++i)
            shape.edgeSegments[i] = std::clamp(static_cast<int>(straight[i] / 16.f) + 1, 1, 512);
        weights = OrbitWeights(shape, s, elapsed);
    }
    std::vector<ImVec2> scratch;
    scratch.reserve(static_cast<std::size_t>(shape.Count()));
    Ring rings[64];
    const float peak = s.intensity * level;
    if (s.spread > 0.f) {
        const float in = s.placement == GlowPlacement::Inside ? s.spread : s.placement == GlowPlacement::Centered ? s.spread * .5f : 0.f;
        const float out = s.placement == GlowPlacement::Outside ? s.spread : s.placement == GlowPlacement::Centered ? s.spread * .5f : 0.f;
        DrawRings(draw, shape, rings, SoftRings(rings, 64, 0.f, in, out, s.falloff, peak), color, weights, scratch);
    }
    // Ripples travel away from the edge and fade; without motion they flash in place.
    const std::vector<float> uniform;
    for (const double ripple : state.ripples) {
        if (ripple < 0)
            continue;
        const float k = std::clamp(static_cast<float>((now - ripple) / s.rippleSeconds), 0.f, 1.f);
        const float travel = animate ? k * s.rippleDistance : 0.f;
        const float half = std::max(s.spread * .5f, 4.f);
        const bool inside = s.placement == GlowPlacement::Inside;
        const float centre = inside ? travel : -travel;
        const float in = inside ? half : std::min(half, travel);
        const float out = inside ? std::min(half, travel) : half;
        DrawRings(draw, shape, rings, SoftRings(rings, 64, centre, in, out, s.falloff, s.intensity * (1.f - k) * (1.f - k)),
                  color, uniform, scratch);
    }
    // The crisp line, with a one-pixel fringe on each side in place of anti-aliasing.
    if (s.lineThickness > 0.f && s.lineIntensity > 0.f) {
        const float t = s.lineThickness;
        const float edge = s.placement == GlowPlacement::Inside ? 0.f : s.placement == GlowPlacement::Outside ? -t : -t * .5f;
        const float a = peak * s.lineIntensity;
        const Ring line[4] = {{edge - 1.f, 0.f}, {edge, a}, {edge + t, a}, {edge + t + 1.f, 0.f}};
        DrawRings(draw, shape, line, 4, color, weights, scratch);
    }
    return changing;
}
} // namespace imkit
