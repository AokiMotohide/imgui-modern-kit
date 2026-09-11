#include <imkit/components.h>
#include <imkit/widgets.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace imkit {
namespace {
bool SemanticItem(ComponentOptions options, const char* label, accessibility::SemanticRole role,
                  accessibility::SemanticAction action, bool checked=false) {
    using namespace accessibility;
    if(!options.accessibility) return false;
    auto id=ImGui::GetItemID();
    bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    bool focus=options.accessibility->Take(id,SemanticAction::Focus);
    if(focus && !disabled) { ImGui::SetKeyboardFocusHere(-1); ImGui::SetNavCursorVisible(true); }
    bool requested=options.accessibility->Take(id,action);
    SemanticNode node; node.id=id; node.parent=options.parent; node.role=role;
    node.name=label; if(auto end=node.name.find("##");end!=std::string_view::npos) node.name=node.name.substr(0,end);
    node.state.checked=checked; node.actions=action|SemanticAction::Focus;
    AnnotateLastItem(*options.accessibility,node);
    return requested && !disabled;
}
ImVec4 Mix(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
ImVec4 Accent(const Theme *t) {
    return t ? t->colors.accent : ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
}
ImVec4 Status(StatusKind kind, const Theme *t) {
    if (!t)
        return ImGui::GetStyleColorVec4(kind == StatusKind::Neutral ? ImGuiCol_TextDisabled
                                                                    : ImGuiCol_CheckMark);
    switch (kind) {
    case StatusKind::Success:
        return t->colors.success;
    case StatusKind::Warning:
        return t->colors.warning;
    case StatusKind::Error:
        return t->colors.destructive;
    default:
        return t->colors.muted;
    }
}
bool Contains(const char *text, const char *query) {
    // ASCII case folding only. UTF-8 non-ASCII bytes are compared unchanged.
    auto fold = [](unsigned char c) { return c < 128 ? static_cast<unsigned char>(std::tolower(c)) : c; };
    std::string a = text, b = query;
    for (auto &c : a)
        c = static_cast<char>(fold(static_cast<unsigned char>(c)));
    for (auto &c : b)
        c = static_cast<char>(fold(static_cast<unsigned char>(c)));
    return a.find(b) != std::string::npos;
}
} // namespace
bool ActionButton(const char *label, ActionVariant variant, const ImVec2 &size, ComponentOptions options) {
    const auto *t = options.theme;
    auto &style = ImGui::GetStyle();
    ImVec4 fill = style.Colors[ImGuiCol_Button], text = style.Colors[ImGuiCol_Text];
    if (variant == ActionVariant::Primary) {
        fill = Accent(t);
        text = t ? t->colors.onAccent : ImVec4(1, 1, 1, 1);
    }
    if (variant == ActionVariant::Destructive) {
        fill = t ? t->colors.destructive : ImVec4(.68f, .20f, .25f, 1);
        text = t ? t->colors.onDestructive : ImVec4(1, 1, 1, 1);
    }
    if (variant == ActionVariant::Ghost)
        fill.w = 0;
    ImGui::PushStyleColor(ImGuiCol_Button, fill);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Mix(fill, Accent(t), .18f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Mix(fill, Accent(t), .32f));
    ImGui::PushStyleColor(ImGuiCol_Text, text);
    if (variant == ActionVariant::Ghost)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    const auto id = ImGui::GetID(label);
    bool pressed = ImGui::Button(label, size);
    if(ImGui::IsItemFocused()) ImGui::SetNavCursorVisible(true);
    if((ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0) {
        const auto p=ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine({p.x-7,p.y-3},{p.x-3,p.y-7},ImGui::GetColorU32(ImGuiCol_Text),1.f);
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s",options.disabledReason?options.disabledReason:"Unavailable");
    }
    pressed = SemanticItem(options,label,accessibility::SemanticRole::Button,accessibility::SemanticAction::Press) || pressed;
    if (options.animation) {
        const auto motion = t ? t->motion : Motion{};
        float a = options.animation->Update(id, ImGui::IsItemHovered() ? 1.f : 0.f, ImGui::GetIO().DeltaTime,
                                            motion.controlSeconds, ImGui::GetFrameCount(), motion.enabled && !motion.reducedMotion, motion.easing);
        auto color = Accent(t);
        color.w *= a;
        if (a > 0)
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                ImGui::GetColorU32(color), style.FrameRounding,
                                                t ? t->metrics.focusWidth : 1.5f, ImDrawFlags_None);
    }
    if (variant == ActionVariant::Ghost)
        ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    return pressed;
}
bool IconButton(const char *id, ImGuiDir direction, const char *accessibleLabel, ComponentOptions options) {
    bool pressed = ImGui::ArrowButton(id, direction);
    pressed = SemanticItem(options,accessibleLabel,accessibility::SemanticRole::Button,accessibility::SemanticAction::Press) || pressed;
    if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
        ImGui::SetTooltip("%s", accessibleLabel);
    return pressed;
}
bool Toggle(const char *label, bool *value, ComponentOptions options) {
    const auto id = ImGui::GetID(label);
    const ImGuiCol hidden[] = {ImGuiCol_FrameBg,       ImGuiCol_FrameBgHovered,
                               ImGuiCol_FrameBgActive, ImGuiCol_CheckboxSelectedBg,
                               ImGuiCol_CheckMark,     ImGuiCol_Border};
    for (auto color : hidden)
        ImGui::PushStyleColor(color, ImVec4(0, 0, 0, 0));
    bool changed = ImGui::Checkbox(label, value);
    if(SemanticItem(options,label,accessibility::SemanticRole::Toggle,accessibility::SemanticAction::Toggle,*value)) {
        *value=!*value; changed=true;
    }
    ImGui::PopStyleColor(6);
    auto min = ImGui::GetItemRectMin();
    float h = ImGui::GetFrameHeight(), round = h * .5f;
    auto &style = ImGui::GetStyle();
    float position = *value ? 1.f : 0.f;
    if (options.animation) {
        auto motion = options.theme ? options.theme->motion : Motion{};
        position = options.animation->Update(id, position, ImGui::GetIO().DeltaTime, motion.controlSeconds,
                                             ImGui::GetFrameCount(), motion.enabled && !motion.reducedMotion, motion.easing);
    }
    auto *d = ImGui::GetWindowDrawList();
    auto fill = Mix(style.Colors[ImGuiCol_FrameBg], Accent(options.theme), position);
    // Fit in native checkbox's square so label layout and hit target remain native.
    float y = min.y + h * .25f;
    d->AddRectFilled({min.x, y}, {min.x + h, y + h * .5f}, ImGui::GetColorU32(fill), round);
    d->AddRect({min.x, y}, {min.x + h, y + h * .5f}, ImGui::GetColorU32(style.Colors[ImGuiCol_Border]),
               round);
    d->AddCircleFilled({min.x + h * (.25f + .5f * position), min.y + h * .5f}, h * .19f,
                       ImGui::GetColorU32(style.Colors[ImGuiCol_Text]));
    if (ImGui::IsItemFocused())
        d->AddRect(min, {min.x + h, min.y + h}, ImGui::GetColorU32(ImGuiCol_NavCursor), style.FrameRounding,
                   1.5f, ImDrawFlags_None);
    return changed;
}
bool IndeterminateCheckbox(const char *label, CheckState *value) {
    bool checked = *value == CheckState::Checked;
    bool mixed = *value == CheckState::Mixed;
    bool changed = ImGui::Checkbox(label, &checked);
    if (changed)
        *value = mixed ? CheckState::Checked : checked ? CheckState::Checked : CheckState::Unchecked;
    if (mixed && !changed) {
        auto p = ImGui::GetItemRectMin();
        float h = ImGui::GetFrameHeight();
        ImGui::GetWindowDrawList()->AddLine({p.x + h * .25f, p.y + h * .5f}, {p.x + h * .75f, p.y + h * .5f},
                                            ImGui::GetColorU32(ImGuiCol_CheckMark), 2);
    }
    return changed;
}
bool Segmented(const char *id, int *selected, std::span<const char *const> labels, ComponentOptions options) {
    bool changed = false;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        if (i)
            ImGui::SameLine(0, 2);
        ImGui::PushID(i);
        if (imkit::Selectable(labels[i], *selected == i, 0,
                              {ImGui::CalcTextSize(labels[i]).x + 20, ImGui::GetFrameHeight()}) |
            SemanticItem(options,labels[i],accessibility::SemanticRole::Radio,accessibility::SemanticAction::Select,*selected==i)) {
            changed = *selected != i;
            *selected = i;
        }
        ImGui::PopID();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool SearchableCombo(const char *label, int *selected, std::span<const char *const> labels, char *search,
                     std::size_t capacity, std::span<const bool> disabled, ComponentOptions options) {
    const char *preview =
        *selected >= 0 && *selected < static_cast<int>(labels.size()) ? labels[*selected] : "";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview)) {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##search", options.locale?options.locale->Text("search","Search..."):"Search...", search, capacity);
        SemanticItem(options,label,accessibility::SemanticRole::TextField,accessibility::SemanticAction::Focus);
        for (int i = 0; i < static_cast<int>(labels.size()); ++i)
            if (Contains(labels[i], search)) {
                ImGui::PushID(i);
                ImGui::BeginDisabled(i < static_cast<int>(disabled.size()) && disabled[i]);
                if (imkit::Selectable(labels[i], *selected == i)) {
                    changed = *selected != i;
                    *selected = i;
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }
        ImGui::EndCombo();
    }
    return changed;
}
bool InputScalarWithUnit(const char *label, ImGuiDataType type, void *value, const char *unit,
                         const void *step, const void *fast, const char *format, ImGuiInputTextFlags flags) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    bool changed = ImGui::InputScalar("##value", type, value, step, fast, format, flags);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unit);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool DragFloatWithUnit(const char *label, float *value, const char *unit, float speed, float minimum,
                       float maximum, const char *format, ImGuiSliderFlags flags) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    bool changed = ImGui::DragFloat("##value", value, speed, minimum, maximum, format, flags);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unit);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool InputVector3WithUnit(const char *label, float *value, const char *unit, const char *format) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::BeginGroup();
    float w = std::max(60.f, (ImGui::CalcItemWidth() - 2 * ImGui::GetStyle().ItemSpacing.x) / 3);
    const char *axes[] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i) {
        if (i)
            ImGui::SameLine();
        ImGui::PushID(i);
        ImGui::SetNextItemWidth(w);
        changed |= ImGui::InputFloat(axes[i], value + i, 0, 0, format);
        ImGui::PopID();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unit);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}
bool BeginSettingRow(const char *id, const char *label, float width) {
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp))
        return false;
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, width);
    ImGui::TableSetupColumn("Value");
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    return true;
}
void EndSettingRow() {
    ImGui::EndTable();
}
void StatusBadge(const char *text, StatusKind kind, const Theme *theme) {
    auto p = ImGui::GetCursorScreenPos(), size = ImGui::CalcTextSize(text);
    size.x += 24;
    size.y += 8;
    ImGui::Dummy(size);
    auto color = Status(kind, theme);
    auto *d = ImGui::GetWindowDrawList();
    auto fill = color;
    fill.w = .12f;
    d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(fill),
                     ImGui::GetStyle().FrameRounding);
    // Marker shape and text carry meaning in addition to color.
    if (kind == StatusKind::Error)
        d->AddRectFilled({p.x + 5, p.y + size.y / 2 - 3}, {p.x + 11, p.y + size.y / 2 + 3},
                         ImGui::GetColorU32(color));
    else if (kind == StatusKind::Warning)
        d->AddTriangleFilled({p.x + 8, p.y + size.y / 2 - 4}, {p.x + 4, p.y + size.y / 2 + 3},
                             {p.x + 12, p.y + size.y / 2 + 3}, ImGui::GetColorU32(color));
    else
        d->AddCircleFilled({p.x + 8, p.y + size.y / 2}, 3, ImGui::GetColorU32(color));
    d->AddText({p.x + 17, p.y + 4}, ImGui::GetColorU32(color), text);
}
bool NotificationCard(const Notification &n, double now, const Theme *t) {
    if (n.expiresAt > 0 && now >= n.expiresAt)
        return false;
    ImGui::PushID(n.id);
    ImGui::BeginGroup();
    StatusBadge(n.text, n.kind, t);
    ImGui::SameLine();
    bool dismiss = ImGui::SmallButton("Dismiss");
    ImGui::EndGroup();
    ImGui::PopID();
    return dismiss;
}
bool BeginToolbar(const char *id, const ImVec2 &size) {
    ImVec2 actual = size;
    if (actual.y == 0)
        actual.y = ImGui::GetFrameHeight() + 2 * ImGui::GetStyle().WindowPadding.y;
    return ImGui::BeginChild(id, actual, ImGuiChildFlags_Borders,
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}
void EndToolbar() {
    ImGui::EndChild();
}
void ValidationMessage(const char *message, bool invalid, const Theme *t) {
    if (!invalid)
        return;
    auto p = ImGui::GetItemRectMin(), q = ImGui::GetItemRectMax();
    auto color = t ? t->colors.destructive : ImVec4(.8f, .2f, .25f, 1);
    ImGui::GetWindowDrawList()->AddRect(p, q, ImGui::GetColorU32(color), ImGui::GetStyle().FrameRounding,
                                        1.5f, ImDrawFlags_None);
    if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
        ImGui::SetTooltip("%s", message);
}
void OverlayDecoration(const Theme &t, AnimationState *state) {
    float a = 1;
    if (state) {
        auto id = ImGui::GetID("##imkit-elevation");
        int frame = ImGui::GetFrameCount();
        if (ImGui::IsWindowAppearing())
            state->Update(id, 0, 0, t.motion.overlaySeconds, frame, t.motion.enabled && !t.motion.reducedMotion, t.motion.easing);
        a = state->Update(id, 1, ImGui::GetIO().DeltaTime, t.motion.overlaySeconds, frame, t.motion.enabled && !t.motion.reducedMotion, t.motion.easing);
    }
    auto p = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    auto *d = ImGui::GetWindowDrawList();
    for (int i = 1; i <= static_cast<int>(t.metrics.elevation); ++i) {
        ImVec4 c = {0, 0, 0, .025f * a};
        d->AddRect({p.x + i, p.y + i}, {p.x + size.x - i, p.y + size.y - i}, ImGui::GetColorU32(c),
                   t.metrics.radius);
    }
}
} // namespace imkit
