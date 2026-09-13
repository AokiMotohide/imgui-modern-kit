#pragma once
#include <imkit/cg.h>
namespace imkit::preview {
struct Vertex {
    float position[3]{}, normal[3]{0, 1, 0}, color[4]{1, 1, 1, 1};
};
struct Mesh {
    editor::StableId id = 0;
    std::span<const Vertex> vertices;
    std::span<const std::uint32_t> indices;
    cg::Transform transform{};
    bool wire = false;
};
struct Triangle {
    ImVec2 points[3];
    double depth = 0;
    ImU32 color = 0;
    bool wire = false;
};
// Returns required triangle count; writes only within host scratch capacity.
std::size_t DrawListPreview(ImDrawList &draw, std::span<const Mesh> meshes, const cg::Camera &camera,
                            ImVec2 origin, ImVec2 size, std::span<Triangle> scratch);
struct NormalOverlayOptions {
    bool vertices=true,faces=false;
    double length=.2; // World-space length after normalization.
    ImU32 color=IM_COL32(80,180,240,255);
};
// DrawList overlay, without depth occlusion. Returns drawn normal segments; inputs remain borrowed.
std::size_t DrawMeshNormals(ImDrawList &draw,std::span<const Mesh> meshes,const cg::Camera &camera,
                           ImVec2 origin,ImVec2 size,NormalOverlayOptions options={});
struct OutlineEdge {
    std::array<float,3> a{},b{};
    ImVec2 screenA{},screenB{};
    bool front=false;
};
// Returns required scratch size (one entry per triangle edge). Insufficient scratch draws nothing.
// Draws boundary and front/back silhouette edges without depth occlusion; coincident positions weld.
std::size_t DrawMeshOutline(ImDrawList &draw,const Mesh &mesh,const cg::Camera &camera,
    ImVec2 origin,ImVec2 size,std::span<OutlineEdge> scratch,ImU32 color,float thickness=2);
bool Cube(std::span<Vertex> vertices, std::span<std::uint32_t> indices); // 24 vertices, 36 indices.
// Host-owned indexed helper meshes, usable by both preview renderers.
bool CameraPrimitive(std::span<Vertex> vertices,std::span<std::uint32_t> indices); // 24 vertices,36 indices; frustum along +Z.
bool LightPrimitive(std::span<Vertex> vertices,std::span<std::uint32_t> indices); // 24 vertices,24 indices; point-light octahedron.
bool Sphere(std::span<Vertex> vertices, std::span<std::uint32_t> indices, int slices = 24, int rings = 12);
} // namespace imkit::preview
