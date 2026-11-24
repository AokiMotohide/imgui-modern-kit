#pragma once

// Easing curves as pure arithmetic. This header and the imkit::motion target need no Dear ImGui, so a renderer or a
// tool can share the exact curves the widgets use without linking the UI.
namespace imkit {
// New values are appended, so stored numbers keep their meaning.
enum class Easing { Linear, EaseOut, EaseInOut, EaseIn, OutBack, InBack, Spring };
// Shape of the curves that overshoot. `overshoot` is the Back constant (1.70158 is the common one); a spring makes
// `oscillations` swings that fade with `damping` (larger settles sooner).
struct EasingShape { float overshoot = 1.70158f; float oscillations = 1.5f; float damping = 6.f; };
// Maps progress 0..1 (clamped) through the curve. The result is 0 at 0 and 1 at 1; Back and Spring pass beyond 1 or
// below 0 on the way. Pure arithmetic: it needs no Dear ImGui context.
float ApplyEasing(Easing easing, float t, const EasingShape& shape = {});
} // namespace imkit
