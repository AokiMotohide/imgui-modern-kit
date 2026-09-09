#include <imkit/preview.h>
#include <algorithm>
#include <cmath>
namespace imkit::preview {
namespace {
cg::Vec3 Transform(cg::Vec3 p, const cg::Transform &t) {
    const auto b=cg::LinearBasis(t);
    return {b.x.x*p.x+b.y.x*p.y+b.z.x*p.z+t.translation.x,
            b.x.y*p.x+b.y.y*p.y+b.z.y*p.z+t.translation.y,
            b.x.z*p.x+b.y.z*p.y+b.z.z*p.z+t.translation.z};
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
                const auto n=cg::NormalBasis(mesh.transform);
                const cg::Vec3 normal{n.x.x*v.normal[0]+n.y.x*v.normal[1]+n.z.x*v.normal[2],
                    n.x.y*v.normal[0]+n.y.y*v.normal[1]+n.z.y*v.normal[2],
                    n.x.z*v.normal[0]+n.y.z*v.normal[1]+n.z.z*v.normal[2]};
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
std::size_t DrawMeshNormals(ImDrawList &draw,std::span<const Mesh> meshes,const cg::Camera &camera,
                           ImVec2 origin,ImVec2 size,NormalOverlayOptions options) {
    if (!(options.length>0) || !std::isfinite(options.length) || size.x<=0 || size.y<=0) return 0;
    std::size_t count=0;
    draw.PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
    const auto line=[&](cg::Vec3 position,cg::Vec3 normal) {
        const auto length=std::hypot(normal.x,normal.y,normal.z);
        if (!(length>0) || !std::isfinite(length)) return;
        const auto factor=options.length/length;
        const auto a=cg::Project(position,camera,origin,size);
        const auto b=cg::Project({position.x+normal.x*factor,position.y+normal.y*factor,position.z+normal.z*factor},camera,origin,size);
        if (a.visible && b.visible) {draw.AddLine(a.screen,b.screen,options.color,1.5f);++count;}
    };
    for (const auto &mesh:meshes) {
        const auto basis=cg::NormalBasis(mesh.transform);
        if (options.vertices) for (const auto &vertex:mesh.vertices)
            line(Transform({vertex.position[0],vertex.position[1],vertex.position[2]},mesh.transform),
                {basis.x.x*vertex.normal[0]+basis.y.x*vertex.normal[1]+basis.z.x*vertex.normal[2],
                 basis.x.y*vertex.normal[0]+basis.y.y*vertex.normal[1]+basis.z.y*vertex.normal[2],
                 basis.x.z*vertex.normal[0]+basis.y.z*vertex.normal[1]+basis.z.z*vertex.normal[2]});
        if (options.faces) for (std::size_t i=0;i+2<mesh.indices.size();i+=3) {
            cg::Vec3 p[3];bool valid=true;
            for (int j=0;j<3;++j) {
                const auto index=mesh.indices[i+j];if (index>=mesh.vertices.size()) {valid=false;break;}
                const auto &v=mesh.vertices[index];p[j]=Transform({v.position[0],v.position[1],v.position[2]},mesh.transform);
            }
            if (!valid) continue;
            const cg::Vec3 u{p[1].x-p[0].x,p[1].y-p[0].y,p[1].z-p[0].z},v{p[2].x-p[0].x,p[2].y-p[0].y,p[2].z-p[0].z};
            line({(p[0].x+p[1].x+p[2].x)/3,(p[0].y+p[1].y+p[2].y)/3,(p[0].z+p[1].z+p[2].z)/3},
                 {u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x});
        }
    }
    draw.PopClipRect();return count;
}
std::size_t DrawMeshOutline(ImDrawList &draw,const Mesh &mesh,const cg::Camera &camera,
    ImVec2 origin,ImVec2 size,std::span<OutlineEdge> scratch,ImU32 color,float thickness) {
    const auto needed=mesh.indices.size()/3*3;
    if (scratch.size()<needed || size.x<=0 || size.y<=0 || !(thickness>0)) return needed;
    std::size_t count=0;
    for (std::size_t i=0;i<needed;i+=3) {
        std::array<float,3> p[3];ImVec2 screen[3];bool valid=true;
        for (int j=0;j<3;++j) {
            const auto index=mesh.indices[i+j];
            if (index>=mesh.vertices.size()) {valid=false;break;}
            const auto &vertex=mesh.vertices[index];
            std::copy_n(vertex.position,3,p[j].begin());
            if (!std::all_of(p[j].begin(),p[j].end(),[](float x){return std::isfinite(x);})) {valid=false;break;}
            const auto projected=cg::Project(Transform({p[j][0],p[j][1],p[j][2]},mesh.transform),camera,origin,size);
            if (!projected.visible) {valid=false;break;}
            screen[j]=projected.screen;
        }
        if (!valid) continue;
        const double area=(screen[1].x-screen[0].x)*(screen[2].y-screen[0].y)-
                          (screen[1].y-screen[0].y)*(screen[2].x-screen[0].x);
        if (area==0 || !std::isfinite(area)) continue;
        for (int j=0;j<3;++j) {
            const int k=(j+1)%3;
            auto edge=OutlineEdge{p[j],p[k],screen[j],screen[k],area>0};
            if (edge.b<edge.a) {std::swap(edge.a,edge.b);std::swap(edge.screenA,edge.screenB);}
            scratch[count++]=edge;
        }
    }
    auto edges=scratch.first(count);
    std::sort(edges.begin(),edges.end(),[](const auto &a,const auto &b){return a.a<b.a || (a.a==b.a && a.b<b.b);});
    draw.PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
    for (std::size_t i=0;i<count;) {
        std::size_t end=i+1;bool front=edges[i].front,back=!front;
        while (end<count && edges[end].a==edges[i].a && edges[end].b==edges[i].b) {
            front|=edges[end].front;back|=!edges[end].front;++end;
        }
        if (end==i+1 || (front && back)) draw.AddLine(edges[i].screenA,edges[i].screenB,color,thickness);
        i=end;
    }
    draw.PopClipRect();return needed;
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
bool CameraPrimitive(std::span<Vertex> vertices,std::span<std::uint32_t> indices) {
    if (!Cube(vertices,indices)) return false;
    for (auto &v:vertices.first(24)) {
        const float extent=v.position[2]>0 ? .65f : .2f;
        v.position[0]*=extent;v.position[1]*=extent*.65f;
        v.position[2]=v.position[2]>0 ? 1.f : 0.f;
        v.color[0]=.35f;v.color[1]=.65f;v.color[2]=.85f;
    }
    for (int face=0;face<6;++face) {
        const auto &a=vertices[indices[face*6]],&b=vertices[indices[face*6+1]],&c=vertices[indices[face*6+2]];
        const cg::Vec3 u{b.position[0]-a.position[0],b.position[1]-a.position[1],b.position[2]-a.position[2]};
        const cg::Vec3 v{c.position[0]-a.position[0],c.position[1]-a.position[1],c.position[2]-a.position[2]};
        const cg::Vec3 n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
        const auto length=std::hypot(n.x,n.y,n.z);
        for (int j=0;j<4;++j) {auto &target=vertices[face*4+j];target.normal[0]=static_cast<float>(n.x/length);target.normal[1]=static_cast<float>(n.y/length);target.normal[2]=static_cast<float>(n.z/length);}
    }
    return true;
}
bool LightPrimitive(std::span<Vertex> vertices,std::span<std::uint32_t> indices) {
    if (vertices.size()<24 || indices.size()<24) return false;
    int index=0;
    for (int x : {-1,1}) for (int y : {-1,1}) for (int z : {-1,1}) {
        cg::Vec3 points[]={{x*.3,0,0},{0,y*.3,0},{0,0,z*.3}};
        if (x*y*z<0) std::swap(points[1],points[2]);
        for (const auto &p:points) {
            auto &v=vertices[index];v={};v.position[0]=static_cast<float>(p.x);v.position[1]=static_cast<float>(p.y);v.position[2]=static_cast<float>(p.z);
            v.normal[0]=x/std::sqrt(3.f);v.normal[1]=y/std::sqrt(3.f);v.normal[2]=z/std::sqrt(3.f);
            v.color[0]=1;v.color[1]=.8f;v.color[2]=.25f;v.color[3]=1;
            indices[index]=index;++index;
        }
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
