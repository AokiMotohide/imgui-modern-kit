#pragma once
#include <imkit/theme.h>
#include <imkit/accessibility.h>
#include <imkit/icons.h>
#include <imkit/locale.h>
#include <span>
#include <cstddef>

// Marks a spelling that a newer overload replaces. Define IMKIT_DEPRECATED(message) as nothing to silence it.
#ifndef IMKIT_DEPRECATED
#define IMKIT_DEPRECATED(message) [[deprecated(message)]]
#endif

namespace imkit {
// Button roles. Primary: the one action of a region (at most one per region). Secondary: an ordinary action.
// Ghost: a tool inside the content (undo, fit to view), frameless until hovered. Destructive: a confirmed, final
// action such as a dialog's delete. SubtleDestructive: deleting or stopping inside a list, red text and outline only.
// Choosing one of several values is not a button: use Segmented.
enum class ActionVariant { Primary, Secondary, Ghost, Destructive, SubtleDestructive };
enum class ControlSize : std::uint8_t { Sm, Md, Lg };
enum class CheckState { Unchecked, Checked, Mixed };
// Live: sending to an output right now, like a broadcast tally ("on air"). Active: a mode the user turned on, drawn
// in the accent colour. New kinds are only ever appended.
enum class StatusKind { Neutral, Success, Warning, Error, Live, Active };
enum class CompactActionRowRequest { None, Primary, Settings };
struct ComponentOptions {
    const Theme *theme = nullptr; // Optional semantic overrides, never retained.
    AnimationState *animation = nullptr;
    accessibility::AccessibilityFrame* accessibility = nullptr;
    accessibility::StableId parent = 0;
    const LocaleContext* locale = nullptr;
    // Shown when a disabled control is hovered. Null shows a generic word; an empty string shows nothing (the host
    // explains it elsewhere).
    const char* disabledReason = nullptr;
    // ActionButton only: 0..1 fills the button from the leading edge to show the work it started (put the stage and
    // percentage in the label). Negative or nonfinite draws no fill.
    float progress = -1.f;
    // ActionButton only: marks the one action the user should press next. Adds a trailing arrow, and a ring that
    // spreads once when the button starts being drawn with this mark (not under reduced motion).
    bool callToAction = false;
    ControlSize size = ControlSize::Md;
};
// A width of 0 follows the label and is never narrower than ActionButtonMinimumWidth. Frames have smooth corners.
bool ActionButton(const char *label, ActionVariant variant = ActionVariant::Primary,
                  const ImVec2 &size = ImVec2(0, 0), ComponentOptions options = {});
// Four text heights: the narrowest an action button gets, so "OK" or "x" stays easy to hit.
float ActionButtonMinimumWidth();
// The width ActionButton gives `label` when its width follows the label.
float ActionButtonWidth(const char *label);
// The widest of ActionButtonWidth over `labels`. Pass it as the width of every button in a row or column of related
// actions, so they line up as one group.
float ActionRowWidth(std::span<const char *const> labels);
// A single explicit action with a visible short label. Description and disabled
// reason are shown on hover/focus; atlas and state remain host-owned. With an empty label the button is a square.
bool IconActionButton(const char *id, const IconAtlas &atlas, IconId icon,
                      const char *label, const char *description = "",
                      ActionVariant variant = ActionVariant::Secondary,
                      ComponentOptions options = {});
// Compact icon and text action with a stable ID and restrained role accent.
bool CompactIconActionButton(const char *id, const IconAtlas &atlas, IconId icon,
                             const char *label, bool selected, ImVec4 accent,
                             const ImVec2 &size = ImVec2(0, 0), const char *description = "",
                             ComponentOptions options = {});
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
// Three native drag fields with stable per-axis IDs. Narrow widths stack the axes.
bool DragVector3WithUnit(const char *label, float value[3], const char *unit, float speed = 1.f,
                         float minimum = 0.f, float maximum = 0.f, const char *format = "%.3f",
                         ImGuiSliderFlags flags = 0);
struct AxisVector3Interaction {
    bool activated = false;
    bool committed = false;
    bool canceled = false;
};
// Groups the label and unit above axis fields that stack at narrow widths.
bool AxisVector3Field(const char *id, const char *label, float value[3], const char *unit,
                      float speed = 1.f, float minimum = 0.f, float maximum = 0.f,
                      const char *format = "%.3f", ImGuiSliderFlags flags = 0,
                      AxisVector3Interaction *interaction = nullptr);
struct SliderInputInteraction {
    bool activated = false; // Slider or number field became active.
    bool changed = false;   // The value changed this frame.
    bool committed = false; // Slider released, or number entry confirmed (Enter / focus lost), after a change.
    bool canceled = false;  // Esc while editing.
};
// Slider and number field on one row, both editing `*value` (clamped to [minimum, maximum], snapped to `step` from
// the minimum when step > 0). The width is ImGui::CalcItemWidth(); below ~160 px the number field wraps under the
// slider. Text after "##" in `label` is the ID only. Use `interaction` to send drag progress (changed) and the final
// value (committed) separately. Returns true on the frame the value changes.
bool SliderFloatWithInput(const char* label, float* value, float minimum, float maximum, const char* format = "%.2f",
                          float step = 0.f, SliderInputInteraction* interaction = nullptr, ComponentOptions options = {});
// Begin/End pair; editor is placed in second column. End only when Begin is true.
bool BeginSettingRow(const char *id, const char *label, float labelWidth = 140);
void EndSettingRow();
void StatusBadge(const char *text, StatusKind kind = StatusKind::Neutral, ComponentOptions options = {});
IMKIT_DEPRECATED("pass ComponentOptions instead of a bare Theme pointer")
void StatusBadge(const char *text, StatusKind kind, const Theme *theme);
// Size of a status badge for `text`. `fontSize` <= 0 uses the current font size.
ImVec2 StatusBadgeSize(const char *text, float fontSize = 0.f);
// Draws a status badge with its top-left corner at `position`, without taking layout space. For canvases and
// overlays that place things on a draw list. Same look as StatusBadge. Returns the badge size.
ImVec2 DrawStatusBadge(ImDrawList *draw, ImVec2 position, const char *text, StatusKind kind = StatusKind::Neutral,
                       float fontSize = 0.f, ComponentOptions options = {});
struct CompactActionRowOptions {
    const char *statusText = nullptr;
    StatusKind statusKind = StatusKind::Neutral;
    const char *primaryLabel = nullptr;
    ActionVariant primaryVariant = ActionVariant::Primary;
    const char *settingsLabel = "Settings";
    ActionVariant settingsVariant = ActionVariant::Ghost;
    bool primaryEnabled = true;
    bool settingsEnabled = true;
    const char *primaryDisabledReason = nullptr;
    const char *settingsDisabledReason = nullptr;
    ComponentOptions components{};
};
// Draws host-owned status and actions. Returns a request; it never changes host state.
// The row wraps its actions when the available width is too narrow for one line.
CompactActionRowRequest CompactActionRow(const char *id, const CompactActionRowOptions &options);
struct Notification {
    const char *id;
    const char *text;
    StatusKind kind = StatusKind::Neutral;
    double expiresAt = 0;
};
// Returns dismiss request. Host owns removal and lifetime. expiresAt==0 persists.
bool NotificationCard(const Notification &notification, double now, ComponentOptions options = {});
IMKIT_DEPRECATED("pass ComponentOptions instead of a bare Theme pointer")
bool NotificationCard(const Notification &notification, double now, const Theme *theme);
bool BeginToolbar(const char *id, const ImVec2 &size = ImVec2(0, 0));
void EndToolbar(); // Always, like EndChild().
// Draw-only decoration: no extra item submission; safe after InputText etc.
void ValidationMessage(const char *message, bool invalid, ComponentOptions options = {});
IMKIT_DEPRECATED("pass ComponentOptions instead of a bare Theme pointer")
void ValidationMessage(const char *message, bool invalid, const Theme *theme);
// Optional inset elevation for the CURRENT popup window. Clip stays inside it.
void OverlayDecoration(const Theme &theme, AnimationState *animation = nullptr);
// Draws the theme's shadow around a shape: elevation.hard gives a solid copy moved by elevation.offset, otherwise the
// blurred shadow of `level`. `alpha` scales the strength. Draws outside the current clip rectangle, so call it before
// the shape itself.
void DrawShadow(const Theme &theme, ImDrawList *draw, ImVec2 minimum, ImVec2 maximum, float rounding,
                ElevationLevel level = ElevationLevel::Overlay, float alpha = 1.f);
// A tool window that floats over the main view: background zones.header, a 1 px border in the theme's border colour,
// and the raised shadow of Theme::elevation (a hard shadow is drawn only outside the window). Call PushFloatingPanel
// before Begin and PopFloatingPanel right after it, whatever Begin returned. A docked window gets no shadow. `theme`
// must stay alive until PopFloatingPanel.
void PushFloatingPanel(const Theme &theme);
void PopFloatingPanel();

// Label on top (with optional right-aligned unit/hint), slider and numeric input on the row below.
// Prevents right-hand label overflow in narrow inspector panels.
bool LabeledSliderFloat(const char *label, float *value, float minimum, float maximum,
                        const char *unit = nullptr, const char *format = "%.2f", float step = 0.f,
                        SliderInputInteraction *interaction = nullptr, ComponentOptions options = {});

struct ChoiceCardOptions {
    const char *subtitle = nullptr;      // Optional secondary line under the title.
    const char *trailingText = nullptr;  // Optional right-aligned value or badge text (e.g. "1.80").
    ImVec4 dotColor{};                   // Optional leading role/status dot when alpha > 0.
    bool selected = false;
    bool disabled = false;
    float width = 0.f;                   // <= 0 fills available width.
    ComponentOptions components{};
};
// Selectable flat/tactile card for 1..N column grids with optional leading dot, subtitle and trailing value.
bool ChoiceCard(const char *id, const char *title, const ChoiceCardOptions &options = {});

// Card container with optional title and description; avoids nested double borders in Modern/Flat look.
// Call EndSectionCard() only when BeginSectionCard() returns true.
bool BeginSectionCard(const char *id, const char *title = nullptr, const char *description = nullptr,
                      ComponentOptions options = {});
void EndSectionCard();

struct ActionRowItem {
    const char *label = "";
    ActionVariant variant = ActionVariant::Secondary;
    bool enabled = true;
    const char *disabledReason = nullptr;
};
// Renders buttons split evenly across the available width. Returns the pressed button index, or -1.
int ActionButtonRow(const char *id, std::span<const ActionRowItem> items, ComponentOptions options = {});

// Interactive status pill (with hover feedback and click return for tooltips/popovers).
bool StatusPill(const char *id, const char *text, StatusKind kind = StatusKind::Neutral,
                ComponentOptions options = {});

// Declarative typography helpers that follow Theme::typography / Theme::fonts (falling back to the last applied theme).
void TextTitle(const char *text, ComponentOptions options = {});
void TextHeading(const char *text, ComponentOptions options = {});
void TextLabel(const char *text, ComponentOptions options = {});
void TextCaption(const char *text, ComponentOptions options = {});
void KeyValueRow(const char *label, const char *value, bool numericValue = false,
                 ComponentOptions options = {});

// Web-style setting row: label (+ optional caption hint) on the left, Toggle switch aligned on the right.
bool SettingToggleRow(const char *label, bool *value, const char *hint = nullptr,
                      ComponentOptions options = {});
// Label above a full-width combo box with optional muted hint below.
bool LabeledCombo(const char *label, int *current, std::span<const char *const> items,
                  const char *hint = nullptr, ComponentOptions options = {});
// Heading + optional subtitle row for sections.
void SectionHeader(const char *title, const char *subtitle = nullptr, ComponentOptions options = {});
} // namespace imkit
