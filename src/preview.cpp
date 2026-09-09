#include <imkit/preview.h>
#include <algorithm>
#include <cmath>
namespace imkit::preview {
namespace {
cg::Vec3 Transform(cg::Vec3 p, const cg::Transform &t) {
    p = {p.x * t.scale.x, p.y * t.scale.y, p.z * t.scale.z};
    auto rotate = [](double &a, double &b, double r) {
        double c = std::cos(r), s = std::sin(r), v = a * c - b * s;
        b = a * s + b * c;
        a = v;
    };
    rotate(p.y, p.z, t.rotation.x);
    rotate(p.z, p.x, t.rotation.y);
    rotate(p.x, p.y, t.rotation.z);
    return {p.x + t.translation.x, p.y + t.translation.y, p.z + t.translation.z};
}
} // namespace
std::size_t DrawListPreview(ImDrawList &draw, std::span<const Mesh> meshes, const cg::Camera &camera,
                            ImVec2 origin, ImVec2 size, std::span<Triangle> scratch) {
    std::size_t needed = 0, written = 0;
    for (const auto &mesh : meshes)
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            ++needed;
            if (written == scratch.size())
                continue;
            Triangle tri{};
            bool valid = true;
            float intensity = 0;
            ImVec4 color{};
            for (int j = 0; j < 3; ++j) {
                auto index = mesh.indices[i + j];
                if (index >= mesh.vertices.size()) {
                    valid = false;
                    break;
                }
                auto &v = mesh.vertices[index];
                auto projected =
                    cg::Project(Transform({v.position[0], v.position[1], v.position[2]}, mesh.transform),
                                camera, origin, size);
                if (!projected.visible) {
                    valid = false;
                    break;
                }
                tri.points[j] = projected.screen;
                tri.depth += projected.depth;
                cg::Transform normalTransform = mesh.transform;
                normalTransform.translation = {};
                auto reciprocal = [](double value) { return value == 0 ? 0. : 1. / value; };
                normalTransform.scale = {reciprocal(mesh.transform.scale.x),
                                         reciprocal(mesh.transform.scale.y),
                                         reciprocal(mesh.transform.scale.z)};
                auto normal = Transform({v.normal[0], v.normal[1], v.normal[2]}, normalTransform);
                double length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                double lambert = length > 0 ? (normal.x * .3 + normal.y * .8 + normal.z * .5) /
                                                (length * std::sqrt(.98)) : 0;
                intensity += .25f + .75f * static_cast<float>((std::max)(0., lambert));
                color.x += v.color[0] / 3;
                color.y += v.color[1] / 3;
                color.z += v.color[2] / 3;
                color.w += v.color[3] / 3;
            }
            if (valid) {
                intensity /= 3;
                color.x *= intensity;
                color.y *= intensity;
                color.z *= intensity;
                tri.color = ImGui::GetColorU32(color);
                tri.wire = mesh.wire;
                scratch[written++] = tri;
            }
        }
    auto triangles = scratch.first(written);
    std::sort(triangles.begin(), triangles.end(), [](auto &a, auto &b) { return a.depth > b.depth; });
    draw.PushClipRect(origin, {origin.x + size.x, origin.y + size.y}, true);
    for (auto &t : triangles)
        if (t.wire)
            draw.AddTriangle(t.points[0], t.points[1], t.points[2], t.color);
        else
            draw.AddTriangleFilled(t.points[0], t.points[1], t.points[2], t.color);
    draw.PopClipRect();
    return needed;
}
bool Cube(std::span<Vertex> vertices, std::span<std::uint32_t> indices) {
    if (vertices.size() < 24 || indices.size() < 36)
        return false;
    const float positions[6][4][3] = {{{1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}},
                                      {{-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1}},
                                      {{-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}},
                                      {{-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}},
                                      {{1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, 1}},
                                      {{-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {1, -1, -1}}};
    const float normals[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int f = 0; f < 6; ++f) {
        for (int j = 0; j < 4; ++j) {
            auto &v = vertices[f * 4 + j];
            std::copy_n(positions[f][j], 3, v.position);
            std::copy_n(normals[f], 3, v.normal);
            v.color[0] = .55f;
            v.color[1] = .45f;
            v.color[2] = .82f;
            v.color[3] = 1;
        }
        const unsigned order[] = {0, 1, 2, 0, 2, 3};
        for (int j = 0; j < 6; ++j)
            indices[f * 6 + j] = f * 4 + order[j];
    }
    return true;
}
bool Sphere(std::span<Vertex> vertices, std::span<std::uint32_t> indices, int slices, int rings) {
    if (slices < 3 || rings < 2 || vertices.size() < static_cast<std::size_t>(slices + 1) * (rings + 1) ||
        indices.size() < static_cast<std::size_t>(slices) * rings * 6)
        return false;
    for (int y = 0; y <= rings; ++y)
        for (int x = 0; x <= slices; ++x) {
            double phi = 3.141592653589793 * y / rings, theta = 6.283185307179586 * x / slices;
            auto &v = vertices[y * (slices + 1) + x];
            v.position[0] = static_cast<float>(std::sin(phi) * std::cos(theta));
            v.position[1] = static_cast<float>(std::cos(phi));
            v.position[2] = static_cast<float>(std::sin(phi) * std::sin(theta));
            std::copy_n(v.position, 3, v.normal);
        }
    std::size_t i = 0;
    for (int y = 0; y < rings; ++y)
        for (int x = 0; x < slices; ++x) {
            unsigned a = y * (slices + 1) + x, b = a + slices + 1;
            for (auto index : {a, b, a + 1, a + 1, b, b + 1})
                indices[i++] = index;
        }
    return true;
}
} // namespace imkit::preview
