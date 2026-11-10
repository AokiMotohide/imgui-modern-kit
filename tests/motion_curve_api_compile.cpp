// imkit::motion builds without Dear ImGui: this file includes no ImGui header and links imkit::motion only.
#include <imkit/motion_curve.h>
#include <cstdio>

int main() {
    const imkit::MotionKey keys[] = {{0.0, 0.0, imkit::MotionEase::Linear}, {1.0, 10.0, imkit::MotionEase::Linear}};
    imkit::MotionEase ease{};
    const bool ok = imkit::ParseMotionEase("spring", ease) && ease == imkit::MotionEase::Spring &&
                    imkit::EvaluateMotionTrack(keys, 0.5) == 5.0 && imkit::WrapMotionTime(2.5, 2.0) == 0.5 &&
                    imkit::ApplyEasing(imkit::Easing::Linear, 0.25f) == 0.25f;
    if (!ok)
        std::fprintf(stderr, "FAIL motion curve without Dear ImGui\n");
    return ok ? 0 : 1;
}
