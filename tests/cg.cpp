#include <imkit/cg.h>
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
    return failures ? 1 : 0;
}
