#pragma once
#include <imkit/components.h>
#include <imkit/skin.h>
#include "draw_internal.h"

// Helpers shared by the component implementations; nothing here owns ImGui state.
namespace imkit::detail {
// Annotates the previous item and returns true when the host's accessibility queue requested
// `action` for it. Also honours a Focus request. A null accessibility frame is a no-op.
bool SemanticItem(ComponentOptions options, const char* label, accessibility::SemanticRole role,
                  accessibility::SemanticAction action, bool checked = false);
// Offers a frame to the theme's skin. True when the skin painted it and the caller must skip its default frame.
bool TrySkin(const Theme* theme, SkinFrame frame);
inline bool MotionAllowed(const ComponentOptions& o) {
    return !o.theme || (o.theme->motion.enabled && !o.theme->motion.reducedMotion);
}
// 0..1 transition toward `target`; immediate when motion is off or no animation state is supplied.
float Ease(const ComponentOptions& o, ImGuiID id, float target);
ImVec4 AccentColor(const ComponentOptions& o);
inline ImU32 Col(ImVec4 c, float alpha = 1.f) {
    c.w *= alpha;
    return ImGui::GetColorU32(c);
}
// Describes the previous item as a value control (Slider, Meter or Group) and returns -1, 0 or +1 when the
// host's accessibility queue asked for a decrement or increment. Also honours a Focus request.
struct ValueNode {
    accessibility::SemanticRole role = accessibility::SemanticRole::Slider;
    const char* name = "";
    const char* description = "";
    const char* text = ""; // Optional spoken value such as "-6 dB".
    double value = 0, minimum = 0, maximum = 1, smallChange = 1, largeChange = 10;
    bool disabled = false, readOnly = false;
};
int AnnotateValue(const ComponentOptions& o, const ValueNode& v);
// Accepts one conversion of the given kinds (e.g. "fge" or "d") such as "%.0f" or "%.1f%%". Anything else
// falls back so a bad host format cannot make printf read the wrong argument type.
const char* SafeFormat(const char* format, const char* kinds, const char* fallback);
} // namespace imkit::detail
