#include <imkit/cg.h>
#include <imkit/preview.h>
#include <cmath>
#include <cstdio>
using namespace imkit::cg;
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *s) {
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "FAIL %s\n", s);
        }
    };
    auto center = Project({}, Camera{}, {0, 0}, {800, 600});
    check(center.visible && center.screen.x == 400 && center.screen.y == 300, "camera center");
    Camera c;
    c.projection = Projection::Orthographic;
    c.yaw = 0;
    c.pitch = 0;
    c.orthographicHeight = 6;
    auto p = Project({1, 1, 0}, c, {0, 0}, {800, 600});
    check(p.visible && p.screen.x == 500 && p.screen.y == 200, "orthographic projection");
    auto t = TransformDelta({}, TransformTool::Translate, Axis::X, {1.26, 5, 5}, {}, .5);
    check(t.translation.x == 1.5 && t.translation.y == 0 && t.translation.z == 0, "axis snapped translation");
    Basis b;
    b.x = {0, 1, 0};
    t = TransformDelta({}, TransformTool::Translate, Axis::X, {1, 0, 0}, b);
    check(t.translation.y == 1, "custom basis");
    t = TransformDelta({}, TransformTool::Scale, Axis::Y, {1, 1, 1}, {});
    check(t.scale.x == 1 && t.scale.y == 2, "axis scale");
    auto uv = TransformUV({1, 0}, {0, 0}, {0, 0}, 3.141592653589793 / 2, {1, 1});
    check(std::abs(uv.x) < 1e-12 && std::abs(uv.y - 1) < 1e-12, "UV rotation");
    uv = TransformUV({.13, .37}, {0, 0}, {0, 0}, 0, {2, 2}, .25);
    check(uv.x == .25 && uv.y == .75, "UV scale snap");
    auto *context = ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {800, 600};
    io.DeltaTime = 1.f / 60;
    unsigned char *pixels; int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::NewFrame();
    ImGui::Begin("Preview normal contract");
    imkit::preview::Vertex vertices[3] = {
        {{-1,-1,0},{1,1,1}}, {{1,-1,0},{1,1,1}}, {{0,1,0},{1,1,1}}};
    std::uint32_t indices[] = {0,1,2};
    imkit::preview::Mesh mesh{1, vertices, indices};
    mesh.transform.scale = {2,1,.5};
    mesh.transform.rotation.z = 3.141592653589793 / 2;
    imkit::preview::Triangle triangles[1];
    check(imkit::preview::DrawListPreview(*ImGui::GetWindowDrawList(), {&mesh,1}, c,
                                         {0,0}, {800,600}, triangles) == 1,
          "DrawList transformed triangle");
    auto expected = static_cast<int>(std::lround(255 * (.25 + .75 * 1.1 / std::sqrt(5.25 * .98))));
    check(std::abs(static_cast<int>(triangles[0].color & 255) - expected) <= 1,
          "DrawList inverse transpose normal under rotation and nonuniform scale");
    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
