#pragma once
#include <imkit/theme.h>
#include <imkit/accessibility.h>
#include <imkit/locale.h>
#include <span>
#include <cstddef>

namespace imkit {
enum class ActionVariant { Primary, Secondary, Ghost, Destructive };
enum class CheckState { Unchecked, Checked, Mixed };
enum class StatusKind { Neutral, Success, Warning, Error };
struct ComponentOptions {
    const Theme *theme = nullptr; // Optional semantic overrides, never retained.
    AnimationState *animation = nullptr;
    accessibility::AccessibilityFrame* accessibility = nullptr;
    accessibility::StableId parent = 0;
    const LocaleContext* locale = nullptr;
};
bool ActionButton(const char *label, ActionVariant variant = ActionVariant::Primary,
                  const ImVec2 &size = ImVec2(0, 0), ComponentOptions options = {});
bool IconButton(const char *id, ImGuiDir direction, const char *accessibleLabel, ComponentOptions options = {});
bool Toggle(const char *label, bool *value, ComponentOptions options = {});
inline bool Switch(const char *label, bool *value, ComponentOptions options = {}) {
    return Toggle(label, value, options);
}
bool IndeterminateCheckbox(const char *label, CheckState *value);
bool Segmented(const char *id, int *selected, std::span<const char *const> labels, ComponentOptions options = {});
// Search text and selection are host-owned. Disabled item indices remain stable.
bool SearchableCombo(const char *label, int *selected, std::span<const char *const> labels, char *search,
                     std::size_t capacity, std::span<const bool> disabled = {}, ComponentOptions options = {});
bool InputScalarWithUnit(const char *label, ImGuiDataType type, void *value, const char *unit,
                         const void *step = nullptr, const void *fastStep = nullptr,
                         const char *format = nullptr, ImGuiInputTextFlags flags = 0);
bool DragFloatWithUnit(const char *label, float *value, const char *unit, float speed = 1, float minimum = 0,
                       float maximum = 0, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool InputVector3WithUnit(const char *label, float value[3], const char *unit, const char *format = "%.3f");
// Begin/End pair; editor is placed in second column. End only when Begin is true.
bool BeginSettingRow(const char *id, const char *label, float labelWidth = 140);
void EndSettingRow();
void StatusBadge(const char *text, StatusKind kind = StatusKind::Neutral, const Theme *theme = nullptr);
struct Notification {
    const char *id;
    const char *text;
    StatusKind kind = StatusKind::Neutral;
    double expiresAt = 0;
};
// Returns dismiss request. Host owns removal and lifetime. expiresAt==0 persists.
bool NotificationCard(const Notification &notification, double now, const Theme *theme = nullptr);
bool BeginToolbar(const char *id, const ImVec2 &size = ImVec2(0, 0));
void EndToolbar(); // Always, like EndChild().
// Draw-only decoration: no extra item submission; safe after InputText etc.
void ValidationMessage(const char *message, bool invalid, const Theme *theme = nullptr);
// Optional inset elevation for the CURRENT popup window. Clip stays inside it.
void OverlayDecoration(const Theme &theme, AnimationState *animation = nullptr);
} // namespace imkit
