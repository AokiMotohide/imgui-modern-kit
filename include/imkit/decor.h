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

enum class SpritePlayback {
    Loop,     // Wraps around to the first frame.
    Once,     // Plays once, then shows `restFrame`.
    HoldLast, // Plays once, then stays on the last frame.
};
// A sheet of equally sized frames laid out row by row in one texture. The host loads and owns the texture.
struct SpriteSheet {
    ImTextureRef texture{};
    ImVec2 sheetPixels{0, 0}; // Size in pixels of the picture that `uvMin`/`uvMax` select.
    ImVec2 cellPixels{0, 0};  // Size in pixels of one frame.
    int columns = 1;          // Frames per row.
    int frames = 1;           // Frames in the sheet; the last row may be partly empty.
    float fps = 24.f;
    SpritePlayback playback = SpritePlayback::Loop;
    int restFrame = 0;        // Shown when motion is off, and after a `Once` sprite finishes.
    ImVec2 uvMin{0, 0}, uvMax{1, 1}; // The part of the texture that holds the sheet.
};
// Host-owned playback clock. Start() and Restart() make the next Sprite() call begin at frame zero.
struct SpriteState {
    double start = -1;
    void Restart() { start = -1; }
};
// The frame to show `elapsedSeconds` after the start. With `animate` false, or an unusable fps, it is `restFrame`.
// The result is always inside [0, frames - 1]. Pure arithmetic: it needs no Dear ImGui context.
int SpriteFrameAt(const SpriteSheet& sheet, double elapsedSeconds, bool animate);
// Texture coordinates of one frame. Returns false when the sheet or the frame number is unusable.
bool SpriteFrameUV(const SpriteSheet& sheet, int frame, ImVec2& uv0, ImVec2& uv1);
// Draws one frame into the rectangle. Draws nothing when the sheet is unusable.
void DrawSprite(ImDrawList* draw, const SpriteSheet& sheet, int frame, ImVec2 minimum, ImVec2 maximum,
                ImU32 tint = IM_COL32_WHITE);
// Reserves `size` in the layout and plays the sheet. `now` is the host's monotonic time in seconds. Pass
// `animate = !theme.motion.reducedMotion && theme.motion.enabled` so that reduced motion shows the rest frame.
// A rectangle outside the visible area is not drawn. Returns the frame that was shown.
int Sprite(const char* id, const SpriteSheet& sheet, SpriteState& state, double now, ImVec2 size, bool animate = true);
} // namespace imkit
