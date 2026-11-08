#include <imkit/imkit.h>
#include <cmath>
#include <cstdio>

namespace {
int failures = 0;
void Check(bool ok, const char *text) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", text);
        ++failures;
    }
}
bool Near(double a, double b, double tolerance = 1e-6) { return std::fabs(a - b) < tolerance; }

void TestEaseNames() {
    imkit::MotionEase ease = imkit::MotionEase::Linear;
    Check(imkit::ParseMotionEase("outBack", ease) && ease == imkit::MotionEase::OutBack, "outBack parses");
    Check(imkit::ParseMotionEase("", ease) && ease == imkit::MotionEase::InOut, "an empty name is inOut");
    ease = imkit::MotionEase::Sine;
    Check(!imkit::ParseMotionEase("bounce", ease) && ease == imkit::MotionEase::Sine, "unknown names leave the value");
    for (auto e : {imkit::MotionEase::Linear, imkit::MotionEase::In, imkit::MotionEase::Out, imkit::MotionEase::InOut,
                   imkit::MotionEase::Sine, imkit::MotionEase::OutBack, imkit::MotionEase::InBack, imkit::MotionEase::Hold,
                   imkit::MotionEase::Spring}) {
        imkit::MotionEase back{};
        Check(imkit::ParseMotionEase(imkit::MotionEaseName(e), back) && back == e, "names round-trip");
        Check(Near(imkit::EvaluateMotionEase(e, 1.0), 1.0), "every curve ends at 1");
        if (e != imkit::MotionEase::Hold)
            Check(Near(imkit::EvaluateMotionEase(e, 0.0), 0.0), "every curve starts at 0");
    }
}

void TestCurves() {
    // Values the designer's player produces (cubic in/out, sine, Back with 1.70158).
    Check(Near(imkit::EvaluateMotionEase(imkit::MotionEase::In, .5), .125), "in is cubic");
    Check(Near(imkit::EvaluateMotionEase(imkit::MotionEase::Out, .5), .875), "out is cubic");
    Check(Near(imkit::EvaluateMotionEase(imkit::MotionEase::InOut, .25), .0625), "inOut is cubic");
    Check(Near(imkit::EvaluateMotionEase(imkit::MotionEase::Sine, .5), .5), "sine is symmetric");
    Check(imkit::EvaluateMotionEase(imkit::MotionEase::OutBack, .7) > 1.0, "outBack passes the target");
    Check(imkit::EvaluateMotionEase(imkit::MotionEase::InBack, .2) < 0.0, "inBack pulls back first");
    Check(Near(imkit::EvaluateMotionEase(imkit::MotionEase::Hold, .999), 0.0), "hold waits for the key");
    Check(imkit::ApplyEasing(imkit::Easing::Spring, .2f) > 1.f, "spring overshoots");
    Check(Near(imkit::ApplyEasing(imkit::Easing::OutBack, 1.f), 1.0, 1e-5), "OutBack easing ends at 1");
    Check(Near(imkit::ApplyEasing(imkit::Easing::EaseIn, .5f), .25, 1e-6), "EaseIn is quadratic");
}

void TestTrack() {
    const imkit::MotionKey keys[] = {{0, -150, imkit::MotionEase::InOut},
                                     {.12, -150, imkit::MotionEase::Hold},
                                     {.5, 0, imkit::MotionEase::In},
                                     {1, 10, imkit::MotionEase::Linear}};
    Check(Near(imkit::EvaluateMotionTrack(keys, -1), -150), "before the first key holds");
    Check(Near(imkit::EvaluateMotionTrack(keys, .3), -150 + 150 * std::pow(.18 / .38, 3)), "the arriving key names the curve");
    Check(Near(imkit::EvaluateMotionTrack(keys, .75), 5), "linear between keys");
    Check(Near(imkit::EvaluateMotionTrack(keys, 9), 10), "after the last key holds");
    Check(Near(imkit::EvaluateMotionTrack({}, 1, 7), 7), "an empty track is the fallback");
    Check(Near(imkit::WrapMotionTime(9.5, 4), 1.5), "time wraps into the loop");
    Check(Near(imkit::WrapMotionTime(-1, 4), 3), "negative time wraps forward");
    Check(Near(imkit::WrapMotionTime(1, 0), 0), "no loop without a duration");
}

void TestLayerQuad() {
    imkit::MotionLayer layer;
    layer.position = {40, 58};
    layer.size = {560, 197};
    layer.pivot = {.5f, 1.f};
    ImVec2 q[4];
    imkit::PoseLayerQuad(layer, {}, {10, 20}, .5f, q);
    Check(Near(q[0].x, 30) && Near(q[0].y, 49) && Near(q[2].x, 310) && Near(q[2].y, 147.5), "rest pose maps the stage");
    imkit::LayerPose pose;
    pose.offset = {0, -16};
    pose.scaleX = 2;
    imkit::PoseLayerQuad(layer, pose, {0, 0}, 1, q);
    Check(Near(q[0].x, 40 - 280) && Near(q[3].y, 255 - 16), "scale keeps the pivot, offset moves it");
    pose = {};
    pose.rotationDegrees = 90;
    imkit::PoseLayerQuad(layer, pose, {0, 0}, 1, q);
    Check(Near(q[3].x, 320, 1e-3) && Near(q[3].y, 255 - 280, 1e-3), "rotation turns around the pivot");
}
} // namespace

int main() {
    TestEaseNames();
    TestCurves();
    TestTrack();
    TestLayerQuad();
    if (failures == 0)
        std::puts("imkit motion tests passed");
    return failures == 0 ? 0 : 1;
}
