#pragma once
#include <imkit/components.h>
#include <imkit/skin.h>
#include <cmath>
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
// A frame border of OutlineMinimumWidth or more is an outline, not a hairline (see theme.h). ApplyTheme truncates scaled
// borders, so the minimum is scaled and truncated the same way. This reads the style, so it works without a Theme.
inline bool IsOutlined(const ImGuiStyle& style) {
    return style.FrameBorderSize > 0.f &&
           style.FrameBorderSize >= std::trunc(OutlineMinimumWidth * style.FontScaleMain);
}
// Strokes a frame in the border colour at the style's border width when the theme is outlined; draws nothing otherwise.
inline void DrawOutline(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float rounding) {
    const auto& style = ImGui::GetStyle();
    if (IsOutlined(style))
        draw->AddRect(minimum, maximum, ImGui::GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_None, style.FrameBorderSize);
}
// Offers a frame to the theme's skin. True when the skin painted it and the caller must skip its default frame.
inline bool TrySkin(const Theme* theme, SkinFrame frame) {
    if (!theme || !theme->skin || !theme->skin->drawFrame)
        return false;
    frame.theme = theme;
    return theme->skin->drawFrame(frame, theme->skin->user);
}
inline bool MotionAllowed(const ComponentOptions& o) {
    return !o.theme || (o.theme->motion.enabled && !o.theme->motion.reducedMotion);
}
// 0..1 transition toward `target`; immediate when motion is off or no animation state is supplied.
inline float Ease(const ComponentOptions& o, ImGuiID id, float target) {
    if (!o.animation || !MotionAllowed(o))
        return target;
    const auto motion = o.theme ? o.theme->motion : Motion{};
    return o.animation->Update(id, target, ImGui::GetIO().DeltaTime, motion.controlSeconds, ImGui::GetFrameCount(), true,
                               motion.easing);
}
inline ImVec4 AccentColor(const ComponentOptions& o) {
    return o.theme ? o.theme->semantic.accent : ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
}
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
inline int AnnotateValue(const ComponentOptions& o, const ValueNode& v) {
    using namespace accessibility;
    if (!o.accessibility)
        return 0;
    const ImGuiID id = ImGui::GetItemID();
    if (o.accessibility->Take(id, SemanticAction::Focus) && !v.disabled) {
        ImGui::SetKeyboardFocusHere(-1);
        ImGui::SetNavCursorVisible(true);
    }
    int direction = 0;
    if (!v.disabled && !v.readOnly) {
        if (o.accessibility->Take(id, SemanticAction::Increment))
            direction = 1;
        else if (o.accessibility->Take(id, SemanticAction::Decrement))
            direction = -1;
    }
    SemanticNode node;
    node.id = id;
    node.parent = o.parent;
    node.role = v.role;
    node.name = v.name;
    if (const auto end = node.name.find("##"); end != std::string_view::npos)
        node.name = node.name.substr(0, end);
    node.description = v.description;
    node.value = v.text;
    node.state.disabled = v.disabled;
    node.state.readOnly = v.readOnly;
    node.numericValue = v.value;
    node.minimumValue = v.minimum;
    node.maximumValue = v.maximum;
    node.smallChange = v.smallChange;
    node.largeChange = v.largeChange;
    node.actions = SemanticAction::Focus;
    if (!v.readOnly)
        node.actions = node.actions | SemanticAction::Increment | SemanticAction::Decrement;
    AnnotateLastItem(*o.accessibility, node);
    return direction;
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
