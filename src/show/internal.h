#pragma once
#include <imkit/show/controls.h>
#include "component_internal.h"
#include "draw_internal.h"

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
float Clamp01(float v);
inline ImU32 U32(ImVec4 c, float alpha = 1.f) {
    c.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}
inline ImVec4 Mix(ImVec4 a, ImVec4 b, float t) { return ::imkit::detail::MixColor(a, b, t); }
// Text colour readable on `background`.
ImVec4 InkFor(const ShowColors& c, ImVec4 background);
inline float Unit() { return ImGui::GetFrameHeight(); }
// Corner radius of keys, pads and cells.
float ControlRound(const ComponentOptions& o);
// Fill of a recessed well (displays, tracks, previews): darker than the surface in dark themes, slightly darker in light ones.
// `depth` is 0..1.
ImVec4 Recessed(const ShowColors& c, float depth = .5f);
// True when an item `width` wide fits after the previous item on the same line, up to the screen x `rightEdge`.
inline bool FitsOnLine(float rightEdge, float width, float spacing) { return ImGui::GetItemRectMax().x + spacing + width <= rightEdge; }
inline bool HasImage(const ImTextureRef& t) { return t._TexData != nullptr || t._TexID != ImTextureID_Invalid; }

// How a console key looks. `progress` fills the body from the bottom (hold-to-confirm).
struct KeyLook {
    ImVec4 tone{};
    bool lit = false, solid = false, enabled = true;
    ConsoleGlyph glyph = ConsoleGlyph::None;
    float labelScale = 1.f, progress = 0.f;
    const char* hint = "";
};
void DrawConsoleKey(ImDrawList* draw, const ShowColors& c, ImVec2 a, ImVec2 b, const char* label, const KeyLook& look, bool hovered,
                    bool held, bool focused, float round);
// Binding indicator in a control's top-right corner: a dot, ringed while the host waits for Learn.
void DrawBindingDot(ImDrawList* draw, const ShowColors& c, const BindingInfo* b, ImVec2 at, float radius, const ComponentOptions& o);
// Draws one pad face (shared by PadGrid and ClipLauncher). The caller owns the item and its hit area.
void DrawPadCell(ImDrawList* draw, const ShowColors& c, const ComponentOptions& options, ImVec2 a, ImVec2 b, const PadView& pad,
                 int index, bool hovered, bool held, bool focused, float round);
// Pill-shaped button at the cursor, drawn into the window draw list. `tone.w <= 0` uses the neutral fill; `dashed`
// swaps the border for a dashed outline (an "add new" affordance). A `size` component <= 0 takes the default: text
// width plus padding by one frame height. The text is centred. Returns true on click.
bool PillButton(const char* id, const char* text, const ShowColors& c, const ComponentOptions& o, ImVec4 ink, ImVec4 tone = {},
                ImVec2 size = {}, bool dashed = false);

// Formatted text that stays valid for the rest of the frame (accessibility nodes borrow strings until the host
// publishes the tree). A ring of 4096 slots; one frame should not need more than the ring holds.
const char* FrameText(const char* format, ...);
// Value formatted with a host format string restricted to one floating conversion.
const char* FormatValue(const char* format, const char* unit, double value);

// A read-only item that still owns an ID, so accessibility nodes can attach to it (Dummy has none). Not a tab stop.
void ReadoutItem(ImVec2 size);
// Annotates the previous item with a value string (clocks, status text). No actions.
void AnnotateText(const ComponentOptions& o, const char* name, const char* value, accessibility::SemanticRole role,
                  bool disabled = false);

// Per-item edit bookkeeping. Construct right after the item that carries the gesture, inside the control's ID scope.
class EditFrame {
  public:
    // `suppressStart` ignores a gesture that begins this frame (Ctrl+click opens a typed entry instead).
    explicit EditFrame(float* value, bool suppressStart = false);
    // Escape during a drag restores the original value. Returns true when the gesture was cancelled.
    bool CancelOnEscape();
    // Ignores the rest of the current gesture.
    void Suppress();
    // A one-shot change (wheel, key, reset, popup entry): started, changed and finished in one frame.
    void Instant(float newValue);
    ValueEdit Done();
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
float SnapValue(float v, float minimum, float maximum, float step);

// Small popup for typing a value (Ctrl+click). Returns true on the frame a value was committed.
bool NumericEntry(const char* popupId, bool open, float* value, float minimum, float maximum, const char* format);

// Small flat glyphs drawn with the draw list so the module needs no icon atlas.
void GlyphPlay(ImDrawList* d, ImVec2 c, float s, ImU32 col);
void GlyphStop(ImDrawList* d, ImVec2 c, float s, ImU32 col);
void GlyphPause(ImDrawList* d, ImVec2 c, float s, ImU32 col);
void GlyphLock(ImDrawList* d, ImVec2 c, float s, ImU32 col, bool locked);
void GlyphWarning(ImDrawList* d, ImVec2 c, float s, ImU32 col);
void DrawTextCentered(ImDrawList* d, ImVec2 a, ImVec2 b, ImU32 col, const char* text);
// Draws `text` clipped to `width`; a trailing ".." marks the cut.
void DrawTextFit(ImDrawList* d, ImVec2 pos, float width, ImU32 col, const char* text);
} // namespace imkit::show::impl
