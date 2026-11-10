#pragma once
#include <imkit/motion_curve.h>
#include <imkit/theme.h>
#include <cstdint>
#include <span>
#include <string_view>

// Layered pictures posed by keyframe motion. The curves and tracks are in imkit/motion_curve.h (no Dear ImGui).
namespace imkit {
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
