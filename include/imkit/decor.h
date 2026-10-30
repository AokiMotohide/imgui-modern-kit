#pragma once
#include <imkit/version.h>

// Drawing helpers for custom looks. They draw with the public draw list only and never own textures: the host loads the
// picture and passes it in. Both are meant to be used from a Skin or from the host's own backgrounds.
namespace imkit {
// The part of a picture, in texture pixels, that stays at its size in the corners and stretches along the edges.
struct NineSliceBorder {
    float left = 0, top = 0, right = 0, bottom = 0;
};
struct NineSliceQuad {
    ImVec2 min{0, 0}, max{0, 0};          // Destination on screen.
    ImVec2 uv0{0, 0}, uv1{0, 0};          // Source in the texture (0..1).
};
// Splits the destination into up to nine quads. Corners keep their pixel size times `scale`; the edges and the centre
// stretch. When the destination is smaller than the corners, the borders shrink together instead of overlapping.
// `texturePixels` is the size in pixels of the picture that `uvMin`/`uvMax` select, which may be a region of a larger
// atlas. Returns the number of quads written (empty quads are skipped); a non-positive size yields none.
int NineSliceQuads(ImVec2 minimum, ImVec2 maximum, ImVec2 texturePixels, NineSliceBorder border, float scale,
                   ImVec2 uvMin, ImVec2 uvMax, NineSliceQuad (&quads)[9]);
void DrawNineSlice(ImDrawList* draw, ImTextureRef texture, ImVec2 minimum, ImVec2 maximum, ImVec2 texturePixels,
                   NineSliceBorder border, float scale = 1.f, ImU32 tint = IM_COL32_WHITE, ImVec2 uvMin = {0, 0},
                   ImVec2 uvMax = {1, 1});

enum class BackdropPattern {
    Dots,    // A staggered grid of dots, like a halftone print.
    Stripes, // Parallel bands at `angle`.
    Rays,    // Alternating wedges around `origin`, like a sunburst.
};
struct BackdropStyle {
    BackdropPattern pattern = BackdropPattern::Dots;
    ImU32 color = IM_COL32(0, 0, 0, 40);
    float spacing = 14.f;    // Dots: distance between dots. Stripes: distance between band starts.
    float size = 3.f;        // Dots: diameter. Stripes: band width.
    float angle = 0.f;       // Stripes: direction in radians. Rays: rotation of the first wedge.
    int rays = 24;           // Rays: number of coloured wedges.
    ImVec2 origin{.5f, .5f}; // Rays: centre, as a fraction of the rectangle.
};
// Fills the rectangle with the pattern, clipped to it. The pattern is redrawn every frame; dots are capped at about
// 20000 per call by widening the spacing, so bake a texture for large, fine halftones.
void DrawBackdropPattern(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const BackdropStyle& style);
} // namespace imkit
