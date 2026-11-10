#include <imkit/motion.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace imkit {
namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

void PoseLayerQuad(const MotionLayer& layer, const LayerPose& pose, ImVec2 origin, float scale,
                   ImVec2 (&corners)[4]) noexcept {
    const float pivotX = layer.position.x + layer.size.x * layer.pivot.x;
    const float pivotY = layer.position.y + layer.size.y * layer.pivot.y;
    const float angle = pose.rotationDegrees * static_cast<float>(kPi / 180.0);
    const float cosine = std::cos(angle), sine = std::sin(angle);
    const ImVec2 local[4] = {
        {layer.position.x, layer.position.y},
        {layer.position.x + layer.size.x, layer.position.y},
        {layer.position.x + layer.size.x, layer.position.y + layer.size.y},
        {layer.position.x, layer.position.y + layer.size.y},
    };
    for (int i = 0; i < 4; ++i) {
        const float x = (local[i].x - pivotX) * pose.scaleX, y = (local[i].y - pivotY) * pose.scaleY;
        const float rx = x * cosine - y * sine, ry = x * sine + y * cosine;
        corners[i] = {origin.x + (pivotX + pose.offset.x + rx) * scale, origin.y + (pivotY + pose.offset.y + ry) * scale};
    }
}

void DrawLayeredMotion(ImDrawList* draw, std::span<const MotionLayer> layers, std::span<const LayerPose> poses,
                       ImVec2 origin, float scale, ImU32 tint) {
    if (!draw)
        return;
    for (std::size_t i = 0; i < layers.size(); ++i) {
        const MotionLayer& layer = layers[i];
        const LayerPose pose = i < poses.size() ? poses[i] : LayerPose{};
        const float opacity = std::clamp(pose.opacity, 0.f, 1.f);
        if (layer.texture.GetTexID() == ImTextureID_Invalid || opacity <= 0.f)
            continue;
        ImVec2 q[4];
        PoseLayerQuad(layer, pose, origin, scale, q);
        const auto alpha = static_cast<float>((tint >> IM_COL32_A_SHIFT) & 0xFF) * opacity;
        const ImU32 color = (tint & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
        draw->AddImageQuad(layer.texture, q[0], q[1], q[2], q[3], layer.uvMin, {layer.uvMax.x, layer.uvMin.y},
                           layer.uvMax, {layer.uvMin.x, layer.uvMax.y}, color);
    }
}
} // namespace imkit
