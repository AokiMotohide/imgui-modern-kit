#include <imkit/motion_curve.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace imkit {
namespace {
constexpr double kPi = 3.14159265358979323846;
struct EaseName { std::string_view name; MotionEase ease; };
constexpr std::array<EaseName, 9> kEaseNames{{
    {"linear", MotionEase::Linear}, {"in", MotionEase::In}, {"out", MotionEase::Out}, {"inOut", MotionEase::InOut},
    {"sine", MotionEase::Sine}, {"outBack", MotionEase::OutBack}, {"inBack", MotionEase::InBack},
    {"hold", MotionEase::Hold}, {"spring", MotionEase::Spring},
}};
} // namespace

bool ParseMotionEase(std::string_view name, MotionEase& out) noexcept {
    if (name.empty()) {
        out = MotionEase::InOut;
        return true;
    }
    for (const auto& entry : kEaseNames)
        if (entry.name == name) {
            out = entry.ease;
            return true;
        }
    return false;
}

std::string_view MotionEaseName(MotionEase ease) noexcept {
    for (const auto& entry : kEaseNames)
        if (entry.ease == ease)
            return entry.name;
    return "inOut";
}

double EvaluateMotionEase(MotionEase ease, double u, const EasingShape& shape) noexcept {
    u = std::clamp(u, 0.0, 1.0);
    const double c = shape.overshoot;
    switch (ease) {
    case MotionEase::Linear: return u;
    case MotionEase::In: return u * u * u;
    case MotionEase::Out: return 1.0 - std::pow(1.0 - u, 3.0);
    case MotionEase::Sine: return 0.5 - std::cos(kPi * u) / 2.0;
    case MotionEase::OutBack: return 1.0 + (c + 1.0) * std::pow(u - 1.0, 3.0) + c * std::pow(u - 1.0, 2.0);
    case MotionEase::InBack: return (c + 1.0) * u * u * u - c * u * u;
    case MotionEase::Hold: return u >= 1.0 ? 1.0 : 0.0;
    case MotionEase::Spring: return ApplyEasing(Easing::Spring, static_cast<float>(u), shape);
    case MotionEase::InOut: break;
    }
    return u < 0.5 ? 4.0 * u * u * u : 1.0 - std::pow(-2.0 * u + 2.0, 3.0) / 2.0;
}

double EvaluateMotionTrack(std::span<const MotionKey> keys, double t, double fallback, const EasingShape& shape) {
    if (keys.empty())
        return fallback;
    if (!(t > keys.front().time))
        return keys.front().value;
    for (std::size_t i = 1; i < keys.size(); ++i) {
        const auto& b = keys[i];
        if (t <= b.time) {
            const auto& a = keys[i - 1];
            const double span = b.time - a.time;
            const double u = span > 0.0 ? (t - a.time) / span : 1.0;
            return a.value + (b.value - a.value) * EvaluateMotionEase(b.ease, u, shape);
        }
    }
    return keys.back().value;
}

double WrapMotionTime(double t, double duration) noexcept {
    if (!(duration > 0.0) || !std::isfinite(t))
        return 0.0;
    const double wrapped = std::fmod(t, duration);
    return wrapped < 0.0 ? wrapped + duration : wrapped;
}

} // namespace imkit
