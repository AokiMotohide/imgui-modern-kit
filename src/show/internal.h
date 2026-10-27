#pragma once
#include <imkit/show/controls.h>
#include "component_internal.h"
#include "draw_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// Helpers private to the Show module. Nothing here owns ImGui state beyond the per-item ImGuiStorage entries.
namespace imkit::show::impl {
using ::imkit::detail::Col;
using ::imkit::detail::Ease;
using ::imkit::detail::MotionAllowed;
using ::imkit::detail::SafeFormat;
using ::imkit::detail::SemanticItem;
using ::imkit::detail::WithAlpha;

inline const char* Safe(const char* s) { return s ? s : ""; }
inline const char* Str(const ComponentOptions& o, const char* key, const char* fallback) {
    return o.locale ? o.locale->Text(key, fallback) : fallback;
}
inline float Clamp01(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : 0.f; }
inline ImU32 U32(ImVec4 c, float alpha = 1.f) {
    c.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}
inline ImVec4 Mix(ImVec4 a, ImVec4 b, float t) { return ::imkit::detail::MixColor(a, b, t); }
// Text colour readable on `background`.
inline ImVec4 InkFor(const ShowColors& c, ImVec4 background) {
    const ImVec4 dark{.06f, .07f, .09f, 1.f}, light{.97f, .98f, 1.f, 1.f};
    return ContrastRatio(dark, background) >= ContrastRatio(light, background) ? dark : light;
}
inline float Unit() { return ImGui::GetFrameHeight(); }
// Draws one pad face (shared by PadGrid and ClipLauncher). The caller owns the item and its hit area.
void DrawPadCell(ImDrawList* draw, const ShowColors& c, const ComponentOptions& options, ImVec2 a, ImVec2 b, const PadView& pad,
                 int index, bool hovered, bool held, bool focused, float round);

// Formatted text that stays valid for the rest of the frame (accessibility nodes borrow strings until the host
// publishes the tree). A ring of slots; one frame should not need more than the ring holds.
inline const char* FrameText(const char* format, ...) {
    static char ring[768][64];
    static unsigned next = 0;
    char* slot = ring[next++ % 768];
    va_list args;
    va_start(args, format);
    std::vsnprintf(slot, 64, format, args);
    va_end(args);
    return slot;
}
// Value formatted with a host format string restricted to one floating conversion.
inline const char* FormatValue(const char* format, const char* unit, double value) {
    char text[40];
    std::snprintf(text, sizeof(text), SafeFormat(format, "fge", "%.1f"), value);
    return FrameText(unit && *unit ? "%s %s" : "%s", text, unit);
}

// A read-only item that still owns an ID, so accessibility nodes can attach to it (Dummy has none). Not a tab stop.
inline void ReadoutItem(ImVec2 size) {
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    ImGui::InvisibleButton("##readout", size);
    ImGui::PopItemFlag();
}
// Annotates the previous item with a value string (clocks, status text). No actions.
inline void AnnotateText(const ComponentOptions& o, const char* name, const char* value, accessibility::SemanticRole role,
                         bool disabled = false) {
    if (!o.accessibility)
        return;
    accessibility::SemanticNode node;
    node.id = ImGui::GetItemID();
    node.parent = o.parent;
    node.role = role;
    node.name = name;
    if (const auto end = node.name.find("##"); end != std::string_view::npos)
        node.name = node.name.substr(0, end);
    node.value = value;
    node.state.disabled = disabled;
    node.state.readOnly = true;
    accessibility::AnnotateLastItem(*o.accessibility, node);
}

// Per-item edit bookkeeping. Construct right after the item that carries the gesture, inside the control's ID scope.
// Dear ImGui keeps the item active until the mouse is released and the public API cannot end that early, so a
// cancelled gesture (Escape, Ctrl+click, double-click reset) is marked "suppressed": the control ignores the rest of
// the drag and no `finished` is reported on release.
class EditFrame {
  public:
    // `suppressStart` ignores a gesture that begins this frame (Ctrl+click opens a typed entry instead).
    explicit EditFrame(float* value, bool suppressStart = false)
        : value_(value), before_(*value), storage_(ImGui::GetStateStorage()), key_(ImGui::GetID("##edit.original")),
          suppressKey_(ImGui::GetID("##edit.suppressed")) {
        active_ = ImGui::IsItemActive();
        deactivated_ = ImGui::IsItemDeactivated();
        if (ImGui::IsItemActivated()) {
            storage_->SetBool(suppressKey_, suppressStart);
            if (!suppressStart) {
                storage_->SetFloat(key_, *value);
                edit_.started = true;
            }
        }
        edit_.original = storage_->GetFloat(key_, *value);
        if (active_ && storage_->GetBool(suppressKey_))
            active_ = false;
    }
    // Escape during a drag restores the original value. Returns true when the gesture was cancelled.
    bool CancelOnEscape() {
        if (!active_ || !ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            return false;
        *value_ = static_cast<float>(edit_.original);
        edit_.cancelled = true;
        Suppress();
        return true;
    }
    // Ignores the rest of the current gesture.
    void Suppress() {
        storage_->SetBool(suppressKey_, true);
        active_ = false;
    }
    // A one-shot change (wheel, key, reset, popup entry): started, changed and finished in one frame.
    void Instant(float newValue) {
        if (newValue == *value_)
            return;
        edit_.original = *value_;
        *value_ = newValue;
        edit_.started = edit_.changed = edit_.finished = true;
        instant_ = true;
    }
    ValueEdit Done() {
        if (!instant_ && *value_ != before_)
            edit_.changed = true;
        if (!instant_ && deactivated_) {
            if (storage_->GetBool(suppressKey_))
                storage_->SetBool(suppressKey_, false);
            else
                edit_.finished = true;
        }
        edit_.value = *value_;
        return edit_;
    }
    bool Active() const { return active_; }
    ImGuiStorage* Storage() const { return storage_; }

  private:
    float* value_;
    float before_;
    ImGuiStorage* storage_;
    ImGuiID key_, suppressKey_;
    ValueEdit edit_{};
    bool active_ = false, deactivated_ = false, instant_ = false;
};

// Snaps to `step` from `minimum` and clamps. A non-positive step only clamps.
inline float SnapValue(float v, float minimum, float maximum, float step) {
    if (step > 0)
        v = minimum + std::round((v - minimum) / step) * step;
    return std::clamp(v, minimum, maximum);
}

// Small popup for typing a value (Ctrl+click). Returns true on the frame a value was committed.
inline bool NumericEntry(const char* popupId, bool open, float* value, float minimum, float maximum, const char* format) {
    if (open)
        ImGui::OpenPopup(popupId);
    bool committed = false;
    if (ImGui::BeginPopup(popupId)) {
        static float draft = 0;
        if (open || ImGui::IsWindowAppearing()) {
            draft = *value;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 4.5f);
        if (ImGui::InputFloat("##value", &draft, 0, 0, SafeFormat(format, "fge", "%.2f"),
                              ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            *value = std::clamp(std::isfinite(draft) ? draft : *value, minimum, maximum);
            committed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return committed;
}

// Small flat glyphs drawn with the draw list so the module needs no icon atlas.
inline void GlyphPlay(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddTriangleFilled({c.x - s * .32f, c.y - s * .45f}, {c.x - s * .32f, c.y + s * .45f}, {c.x + s * .48f, c.y}, col);
}
inline void GlyphStop(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddRectFilled({c.x - s * .38f, c.y - s * .38f}, {c.x + s * .38f, c.y + s * .38f}, col, s * .08f);
}
inline void GlyphPause(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddRectFilled({c.x - s * .36f, c.y - s * .42f}, {c.x - s * .12f, c.y + s * .42f}, col, s * .05f);
    d->AddRectFilled({c.x + s * .12f, c.y - s * .42f}, {c.x + s * .36f, c.y + s * .42f}, col, s * .05f);
}
inline void GlyphLock(ImDrawList* d, ImVec2 c, float s, ImU32 col, bool locked) {
    const float w = s * .56f, h = s * .40f;
    d->AddRectFilled({c.x - w * .5f, c.y - h * .05f}, {c.x + w * .5f, c.y + h * 1.05f}, col, s * .08f);
    const float r = w * .36f;
    const ImVec2 ctr{c.x + (locked ? 0.f : r * .8f), c.y - h * .05f};
    d->PathArcTo(ctr, r, 3.14159265f, 6.2831853f, 10);
    d->PathStroke(col, 0, std::max(1.4f, s * .09f));
    if (locked) {
        d->AddLine({c.x - r, ctr.y}, {c.x - r, c.y - h * .05f}, col, std::max(1.4f, s * .09f));
        d->AddLine({c.x + r, ctr.y}, {c.x + r, c.y - h * .05f}, col, std::max(1.4f, s * .09f));
    }
}
inline void GlyphWarning(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    d->AddTriangleFilled({c.x, c.y - s * .46f}, {c.x - s * .5f, c.y + s * .4f}, {c.x + s * .5f, c.y + s * .4f}, col);
}
inline void DrawTextCentered(ImDrawList* d, ImVec2 a, ImVec2 b, ImU32 col, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    d->AddText({a.x + (b.x - a.x - size.x) * .5f, a.y + (b.y - a.y - size.y) * .5f}, col, text);
}
// Draws `text` clipped to `width`; a trailing ".." marks the cut.
inline void DrawTextFit(ImDrawList* d, ImVec2 pos, float width, ImU32 col, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    if (size.x <= width) {
        d->AddText(pos, col, text);
        return;
    }
    const float dots = ImGui::CalcTextSize("..").x;
    d->PushClipRect(pos, {pos.x + std::max(0.f, width - dots), pos.y + size.y + 2}, true);
    d->AddText(pos, col, text);
    d->PopClipRect();
    d->AddText({pos.x + std::max(0.f, width - dots), pos.y}, col, "..");
}
} // namespace imkit::show::impl
