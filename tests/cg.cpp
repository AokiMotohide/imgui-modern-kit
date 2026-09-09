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
    NavigateCamera(c,{}, {},1,600);
    auto zoomed=Project({1,1,0},c,{0,0},{800,600});
    check(std::abs(c.orthographicHeight-5.1)<1e-12 && zoomed.screen.x>p.screen.x && c.distance==6,
          "orthographic navigation zoom changes projected extent");
    AlignCamera(c,Axis::X);
    NavigateCamera(c,{}, {60,0},0,600);
    check(std::abs(c.target.x)<1e-12 && c.target.z>0 && std::abs(c.target.z-.51)<1e-12,
          "pan follows the view right axis and projected scale");
    for (Axis axis:{Axis::X,Axis::Y,Axis::Z})
        for (bool negative:{false,true}) {
            AlignCamera(c,axis,negative);
            auto basis=OrientationBasis(Orientation::View,{},c);
            double facing=axis==Axis::X ? basis.z.x : axis==Axis::Y ? basis.z.y : basis.z.z;
            check(std::abs(facing-(negative ? -1 : 1))<1e-12,"six axis view alignment");
        }
    c.projection=Projection::Camera;
    auto originalCamera=c;
    NavigateCamera(c,{20,10},{20,10},3,600);
    check(c.yaw==originalCamera.yaw && c.distance==originalCamera.distance && c.target.z==originalCamera.target.z,
          "host camera view ignores free viewport navigation");
    c=Camera{}; c.projection=Projection::Orthographic; c.yaw=c.pitch=0;c.orthographicHeight=6;
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
    ViewportState navigation;
    navigation.camera.yaw=navigation.camera.pitch=0;
    navigation.camera.projection=Projection::Orthographic;
    ViewportView navigationView;
    auto navigationFrame=[&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Viewport navigation");
        navigationView=BeginViewport("view",navigation,{}, {750,500},
                                     imkit::MakePrecisionTheme(imkit::ColorScheme::Dark));
        EndViewport(); ImGui::End(); ImGui::Render();
    };
    navigationFrame(); navigationFrame();
    io.AddMousePosEvent(navigationView.min.x+100,navigationView.min.y+100);
    navigationFrame();
    double originalHeight=navigation.camera.orthographicHeight;
    io.AddMouseWheelEvent(0,1); navigationFrame();
    check(navigation.camera.orthographicHeight<originalHeight,"orthographic wheel through public IO");
    io.AddMousePosEvent(navigationView.min.x+navigationView.size.x-24,navigationView.min.y+56);
    navigationFrame();
    io.AddMouseButtonEvent(0,true); navigationFrame();
    io.AddMouseButtonEvent(0,false); navigationFrame();
    check(std::abs(navigation.camera.yaw-3.141592653589793/2)<1e-12,
          "navigation gizmo axis click through public IO");
    Camera hostCamera; hostCamera.distance=12;hostCamera.yaw=.2;
    navigation.cameraView=&hostCamera;navigation.camera.projection=Projection::Camera;
    navigationFrame();
    check(navigation.camera.distance==12 && navigation.camera.yaw==.2,
          "camera view consumes the non-owning host camera");
    ImGui::DestroyContext(context);
    return failures ? 1 : 0;
}
