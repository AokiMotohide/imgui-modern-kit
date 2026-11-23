#include "component_internal.h"
#include <imkit/skin.h>
#include <string_view>

namespace imkit::detail {
bool SemanticItem(ComponentOptions options, const char* label, accessibility::SemanticRole role,
                  accessibility::SemanticAction action, bool checked) {
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

bool TrySkin(const Theme* theme, SkinFrame frame) {
    if (!theme || !theme->skin || !theme->skin->drawFrame)
        return false;
    frame.theme = theme;
    return theme->skin->drawFrame(frame, theme->skin->user);
}

float Ease(const ComponentOptions& o, ImGuiID id, float target) {
    if (!o.animation || !MotionAllowed(o))
        return target;
    const auto motion = o.theme ? o.theme->motion : Motion{};
    return o.animation->Update(id, target, ImGui::GetIO().DeltaTime, motion.controlSeconds, ImGui::GetFrameCount(), true,
                               motion.easing);
}

ImVec4 AccentColor(const ComponentOptions& o) {
    return o.theme ? o.theme->semantic.accent : ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
}

int AnnotateValue(const ComponentOptions& o, const ValueNode& v) {
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

const char* SafeFormat(const char* format, const char* kinds, const char* fallback) {
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
