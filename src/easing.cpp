#include <imkit/easing.h>
#include <algorithm>
#include <cmath>

namespace imkit {
float ApplyEasing(Easing easing, float t, const EasingShape& shape) {
    t = std::clamp(t, 0.f, 1.f);
    const float c = shape.overshoot;
    switch (easing) {
    case Easing::Linear: return t;
    case Easing::EaseOut: return 1 - (1 - t) * (1 - t);
    case Easing::EaseInOut: return t * t * (3 - 2 * t);
    case Easing::EaseIn: return t * t;
    case Easing::OutBack: { const float u = t - 1; return 1 + (c + 1) * u * u * u + c * u * u; }
    case Easing::InBack: return (c + 1) * t * t * t - c * t * t;
    case Easing::Spring: {
        if (t >= 1) return 1;
        constexpr float kPi = 3.14159265358979f;
        return 1 - std::exp(-std::max(0.f, shape.damping) * t) * std::cos(2 * kPi * std::max(0.f, shape.oscillations) * t);
    }
    }
    return t;
}
} // namespace imkit
