#pragma once
#include <imkit/theme.h>
#include <cstdint>
#include <span>
#include <string_view>

// Keyframe motion for small looping pictures: a logo that waves, a character that peeks out. The curves and the track
// format follow the common "motion file" layout — keys of [time, value, curve name], each key naming the curve used to
// arrive at it — so a designer's player and the application agree frame for frame. ImKit reads no files: the host
// parses its own format into MotionKey spans and owns every texture.
namespace imkit {
enum class MotionEase : std::uint8_t {
    Linear,
    In,      // Cubic ease in.
    Out,     // Cubic ease out.
    InOut,   // Cubic ease in and out. The default when a key names no curve.
    Sine,    // Half a cosine wave.
    OutBack, // Passes the target, then settles back (EasingShape::overshoot).
    InBack,  // Pulls back first, then goes.
    Hold,    // Keeps the previous value until the key's time, then jumps.
    Spring,  // Swings around the target and settles (EasingShape::oscillations, damping).
};
// Names: linear, in, out, inOut, sine, outBack, inBack, hold, spring. An empty name is InOut. Returns false and leaves
// `out` unchanged for an unknown name.
bool ParseMotionEase(std::string_view name, MotionEase& out) noexcept;
std::string_view MotionEaseName(MotionEase ease) noexcept;
// Maps u in 0..1 (clamped) through the curve. Pure arithmetic: it needs no Dear ImGui context.
double EvaluateMotionEase(MotionEase ease, double u, const EasingShape& shape = {}) noexcept;

struct MotionKey {
    double time = 0;  // Seconds.
    double value = 0;
    MotionEase ease = MotionEase::InOut; // Curve from the previous key to this one.
};
// The value of a track at `t`. Keys must be sorted by time. Before the first key and after the last the end values
// hold; an empty track is `fallback`.
double EvaluateMotionTrack(std::span<const MotionKey> keys, double t, double fallback = 0, const EasingShape& shape = {});
// `t` folded into [0, duration) for a loop; a non-positive duration returns 0.
double WrapMotionTime(double t, double duration) noexcept;

// One picture of a layered motion, placed in stage pixels (the coordinate space the designer drew in).
struct MotionLayer {
    ImTextureRef texture{};
    ImVec2 position{0, 0};  // Top-left of the picture on the stage.
    ImVec2 size{0, 0};      // Size of the picture on the stage.
    ImVec2 pivot{.5f, .5f}; // Point the pose scales and rotates around, as a fraction of `size`.
    ImVec2 uvMin{0, 0}, uvMax{1, 1};
};
// Where a layer is at one moment. Applied as translate(offset) · rotate · scale around the pivot, like CSS.
struct LayerPose {
    ImVec2 offset{0, 0};  // Stage pixels.
    float scaleX = 1, scaleY = 1;
    float rotationDegrees = 0;
    float opacity = 1;    // 0..1, multiplied with the tint.
};
// The four screen corners of a posed layer (top-left, top-right, bottom-right, bottom-left). The stage maps to the
// screen as origin + stagePoint * scale. Needs no Dear ImGui context.
void PoseLayerQuad(const MotionLayer& layer, const LayerPose& pose, ImVec2 origin, float scale, ImVec2 (&corners)[4]) noexcept;
// Draws the layers in order, each with the pose at the same index; a missing pose is the rest pose. Layers without a
// texture or with zero opacity are skipped. Pass the poses at rest when the theme asks for reduced motion.
void DrawLayeredMotion(ImDrawList* draw, std::span<const MotionLayer> layers, std::span<const LayerPose> poses,
                       ImVec2 origin, float scale, ImU32 tint = IM_COL32_WHITE);
} // namespace imkit
