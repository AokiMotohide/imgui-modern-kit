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

// ---- Glow frame -----------------------------------------------------------------------------------------------
// A soft light along the edge of a rectangle: a window, a panel or the whole viewport. It marks a state that applies
// to everything inside, such as remote control, recording or a live output. The motions combine: a base motion,
// ripples started by GlowFrameState::Pulse() or by `rippleInterval`, and an optional crisp line.
enum class GlowMotion {
    Steady,  // Constant light.
    Breathe, // The light rises and falls once per `period`.
    Orbit,   // Bright heads travel along the edge, one lap per `period`, over a dimmer track.
};
enum class GlowPlacement {
    Inside,  // Fades inward from the edge; suits a viewport or a window that fills the screen.
    Outside, // Fades outward; suits a panel with room around it.
    Centered,// Fades both ways from the edge.
};
struct GlowFrameStyle {
    ImVec4 color{.851f, .467f, .341f, 1.f};
    GlowMotion motion = GlowMotion::Breathe;
    GlowPlacement placement = GlowPlacement::Inside;
    float intensity = .8f;         // Peak opacity, 0..1.
    float minimumIntensity = .35f; // Breathe: the low point. Orbit: the track. A share of `intensity`, 0..1.
    float period = 2.4f;           // Seconds per breath or per lap.
    float spread = 28.f;           // Width of the soft light in pixels.
    float falloff = 2.f;           // Curve of the fade: 1 is linear, higher keeps the light closer to the edge.
    float lineThickness = 1.5f;    // Crisp line on the edge in pixels; 0 draws none.
    float lineIntensity = 1.f;     // Line opacity as a share of the current light.
    float rounding = 0.f;          // Corner radius in pixels.
    int orbitHeads = 1;            // Orbit: heads spaced evenly along the edge (1..8).
    float orbitLength = .18f;      // Orbit: length of each head's tail as a share of the perimeter (0..1).
    float rippleSeconds = .9f;     // Life of one ripple.
    float rippleDistance = 90.f;   // How far a ripple travels from the edge, in pixels.
    float rippleInterval = 0.f;    // Seconds between automatic ripples while active; 0 starts them only on Pulse().
    float fadeSeconds = .25f;      // Fade in when activated and out when deactivated.
};
// Host-owned. Keep one per frame that is drawn.
struct GlowFrameState {
    float visibility = 0;          // 0..1, eased toward 1 while active and 0 while not.
    double start = -1;             // Clock of the motion; reset when the frame appears.
    double last = -1;              // Time of the previous draw.
    double lastAutoRipple = -1;
    double ripples[4]{-1, -1, -1, -1};
    int pendingPulses = 0;
    // Starts a ripple at the next draw, for example when a command arrives. Ignored while the frame is hidden.
    void Pulse() { ++pendingPulses; }
    // Forgets visibility, motion and ripples; an active frame fades in again from the next draw.
    void Reset() { *this = {}; }
};
// The brightness of the light, 0..1 before `intensity`, `elapsedSeconds` after the frame appeared. Orbit and Steady
// return 1 here; Orbit's variation is along the edge. With `animate` false the result is 1. Needs no Dear ImGui context.
float GlowFrameLevel(const GlowFrameStyle& style, double elapsedSeconds, bool animate);
// Draws the frame. `now` is the host's monotonic time in seconds. Pass `animate = !theme.motion.reducedMotion &&
// theme.motion.enabled`; without it the light is steady, ripples flash in place and nothing fades. Returns true while
// the picture changes over time, so a host that sleeps between frames knows to draw another one.
bool DrawGlowFrame(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, const GlowFrameStyle& style, GlowFrameState& state,
                   double now, bool active, bool animate = true);
} // namespace imkit
