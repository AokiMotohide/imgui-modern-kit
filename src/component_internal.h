#pragma once
#include <imkit/components.h>
#include <string_view>

// Helpers shared by the component implementations; nothing here owns ImGui state.
namespace imkit::detail {
// Annotates the previous item and returns true when the host's accessibility queue requested
// `action` for it. Also honours a Focus request. A null accessibility frame is a no-op.
inline bool SemanticItem(ComponentOptions options, const char* label, accessibility::SemanticRole role,
                         accessibility::SemanticAction action, bool checked = false) {
    using namespace accessibility;
    if (!options.accessibility)
        return false;
    auto id = ImGui::GetItemID();
    bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    bool focus = options.accessibility->Take(id, SemanticAction::Focus);
    if (focus && !disabled) {
        ImGui::SetKeyboardFocusHere(-1);
        ImGui::SetNavCursorVisible(true);
    }
    bool requested = options.accessibility->Take(id, action);
    SemanticNode node;
    node.id = id;
    node.parent = options.parent;
    node.role = role;
    node.name = label;
    if (auto end = node.name.find("##"); end != std::string_view::npos)
        node.name = node.name.substr(0, end);
    node.state.checked = checked;
    node.actions = action | SemanticAction::Focus;
    AnnotateLastItem(*options.accessibility, node);
    return requested && !disabled;
}
inline bool MotionAllowed(const ComponentOptions& o) {
    return !o.theme || (o.theme->motion.enabled && !o.theme->motion.reducedMotion);
}
// Accepts one conversion of the given kinds (e.g. "fge" or "d") such as "%.0f" or "%.1f%%". Anything else
// falls back so a bad host format cannot make printf read the wrong argument type.
inline const char* SafeFormat(const char* format, const char* kinds, const char* fallback) {
    int conversions = 0;
    for (const char* c = format ? format : ""; *c; ++c) {
        if (*c != '%')
            continue;
        if (c[1] == '%') {
            ++c;
            continue;
        }
        const char* end = c + 1;
        while (*end == '.' || (*end >= '0' && *end <= '9') || *end == '+' || *end == '-')
            ++end;
        bool known = false;
        for (const char* k = kinds; *k; ++k)
            known = known || *k == *end;
        if (!*end || !known)
            return fallback;
        ++conversions;
        c = end;
    }
    return conversions == 1 ? format : fallback;
}
} // namespace imkit::detail
