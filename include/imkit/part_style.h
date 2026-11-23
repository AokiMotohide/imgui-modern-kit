#pragma once
#include <imkit/version.h>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

// Four layers of customisation for every ImKit part (see docs: "ImKit モダン化計画" §2).
//
//   L1 values   Theme text keys per part and state: `checkbox.on.fill = accent`, `slider.thumb.size = 14`.
//               Stored in Theme::partOverrides; ApplyTheme makes them current.
//   L2 styles   Typed styles (ButtonStyle, SliderStyle, ...) passed to one call, or registered under a name and
//               applied to a scope: RegisterStyle("toolbar", ButtonStyle{...}); StyleScope s("toolbar");
//   L3 skins    Skin (skin.h) replaces the drawing of a part; layout and hit testing stay with ImKit.
//   L4 custom   Interact() and the drawing helpers below build new parts that feel like ImKit ones.
//
// Which value wins (strongest first):
//   1. the style passed to one call                 imkit::Button("Shoot", {}, style)
//   2. values pushed on a scope                      PushPartVar / PushPartColor, and ImGui PushStyleColor / PushStyleVar
//   3. a named style applied to a scope              StyleScope s("toolbar")
//   4. the theme text                                button.hover.fill = #...
//   5. the ImKit default (modern)
//
// Lengths in layers 1 to 4 are design pixels and follow the UI scale given to ApplyTheme; resolved values are screen
// pixels. A state that no layer names is derived from the rest state of the strongest layer that names it.
namespace imkit {
struct Theme;

// New parts are only ever appended.
enum class Part : std::uint8_t {
    Button, SmallButton, Checkbox, Toggle, Radio, Combo, Selectable, ColorEdit,
    Slider, Drag, Input, Progress, Header, Tree, Tab, Separator,
    Table, Scrollbar, Popup, Tooltip, Window, Modal
};
inline constexpr std::size_t PartCount = 22;

enum class PartState : std::uint8_t { Rest, Hover, Pressed, Focused, Selected, Disabled, Active, On, Off };
inline constexpr std::size_t PartStateCount = 9;

// Fill is the body, Border its outline, Text the label, Mark the inner mark (check, dot, slider fill and thumb,
// tab underline, scrollbar grab, selection bar).
enum class PartColor : std::uint8_t { Fill, Border, Text, Mark };
inline constexpr std::size_t PartColorCount = 4;

enum class PartVar : std::uint8_t {
    Height, MinWidth, BorderWidth, Radius, PaddingX, PaddingY, FontSize, PressDepth,
    BoxSize, MarkWidth, TrackHeight, ThumbSize, IndicatorHeight, Gap, FocusRing
};
inline constexpr std::size_t PartVarCount = 15;

// Where a resolved value came from; ShowStyleEditor shows it.
enum class PartLayer : std::uint8_t { Default, Theme, NamedStyle, Pushed, Call };

// Names used by the theme text: `button`, `small-button`, ..., `hover`, `fill`, `thumb.size`.
std::string_view PartName(Part part) noexcept;
std::string_view PartStateName(PartState state) noexcept;
std::string_view PartColorName(PartColor color) noexcept;
std::string_view PartVarName(PartVar var) noexcept;
std::optional<Part> PartFromName(std::string_view name) noexcept;
std::optional<PartState> PartStateFromName(std::string_view name) noexcept;
std::optional<PartColor> PartColorFromName(std::string_view name) noexcept;
// Accepts both the dotted and the dashed spelling: `thumb.size` and `thumb-size`.
std::optional<PartVar> PartVarFromName(std::string_view name) noexcept;

// "#RRGGBB" or "#RRGGBBAA" (the leading # may be omitted). Invalid text gives opaque magenta, so a typo is visible.
ImVec4 Hex(std::string_view text) noexcept;

// L1: the per part and state values of a theme. A value type; unset entries fall back to the next layer.
class PartOverrides {
  public:
    std::optional<float> Var(Part part, PartVar var) const noexcept {
        const std::size_t i = VarIndex(part, var);
        return varSet_[i] ? std::optional<float>(vars_[i]) : std::nullopt;
    }
    void SetVar(Part part, PartVar var, float value) noexcept {
        const std::size_t i = VarIndex(part, var);
        vars_[i] = value;
        varSet_.set(i);
    }
    void ClearVar(Part part, PartVar var) noexcept { varSet_.reset(VarIndex(part, var)); }
    std::optional<ImVec4> Color(Part part, PartState state, PartColor color) const noexcept {
        const std::size_t i = ColorIndex(part, state, color);
        return colorSet_[i] ? std::optional<ImVec4>(colors_[i]) : std::nullopt;
    }
    void SetColor(Part part, PartState state, PartColor color, ImVec4 value) noexcept {
        const std::size_t i = ColorIndex(part, state, color);
        colors_[i] = value;
        colorSet_.set(i);
    }
    void ClearColor(Part part, PartState state, PartColor color) noexcept { colorSet_.reset(ColorIndex(part, state, color)); }
    bool Empty() const noexcept { return varSet_.none() && colorSet_.none(); }
    bool HasPart(Part part) const noexcept;
    void Clear() noexcept { *this = PartOverrides{}; }

  private:
    static constexpr std::size_t VarIndex(Part p, PartVar v) noexcept {
        return static_cast<std::size_t>(p) * PartVarCount + static_cast<std::size_t>(v);
    }
    static constexpr std::size_t ColorIndex(Part p, PartState s, PartColor c) noexcept {
        return (static_cast<std::size_t>(p) * PartStateCount + static_cast<std::size_t>(s)) * PartColorCount +
               static_cast<std::size_t>(c);
    }
    std::array<float, PartCount * PartVarCount> vars_{};
    std::array<ImVec4, PartCount * PartStateCount * PartColorCount> colors_{};
    std::bitset<PartCount * PartVarCount> varSet_{};
    std::bitset<PartCount * PartStateCount * PartColorCount> colorSet_{};
};

// L2: typed styles. Every member is optional; an unset one falls back to the next layer.
struct PartColors {
    std::optional<ImVec4> fill, border, text, mark;
};
struct PartStateColors {
    PartColors rest, hover, pressed, focused, selected, disabled, active, on, off;
    const PartColors &operator[](PartState state) const noexcept;
    PartColors &operator[](PartState state) noexcept;
};
struct PartBorder {
    std::optional<float> width;
};
// Members shared by every part. Lengths are design pixels.
struct PartStyle {
    std::optional<float> height, minWidth, radius, paddingX, paddingY, fontSize, gap, focusRing;
    PartBorder border;
    PartStateColors colors; // style.colors.hover.fill = imkit::Hex("#3d7eff");
};
struct ButtonStyle : PartStyle { std::optional<float> pressDepth; };
struct CheckboxStyle : PartStyle { std::optional<float> boxSize, markWidth; };
struct RadioStyle : PartStyle { std::optional<float> boxSize, markWidth; };
struct SliderStyle : PartStyle { std::optional<float> trackHeight, thumbSize; };
struct DragStyle : PartStyle {};
struct InputStyle : PartStyle {};
struct ComboStyle : PartStyle { std::optional<float> markWidth; };       // markWidth: the chevron stroke.
struct SelectableStyle : PartStyle { std::optional<float> markWidth; };  // markWidth: the selection bar.
struct HeaderStyle : PartStyle { std::optional<float> markWidth; };      // markWidth: the chevron stroke.
struct TreeStyle : PartStyle { std::optional<float> markWidth; };        // markWidth: the chevron stroke.
struct TabStyle : PartStyle { std::optional<float> indicatorHeight; };   // The underline of the selected tab.
struct ProgressStyle : PartStyle { std::optional<float> trackHeight; };
struct ColorEditStyle : PartStyle { std::optional<float> boxSize; };     // The colour swatch.

// Every value of one part in one flat record; the typed styles convert to it.
struct PartValues {
    std::array<std::optional<float>, PartVarCount> vars{};
    std::array<std::array<std::optional<ImVec4>, PartColorCount>, PartStateCount> colors{};
    bool Empty() const noexcept;
};
PartValues ToPartValues(const PartStyle &style);
PartValues ToPartValues(const ButtonStyle &style);
PartValues ToPartValues(const CheckboxStyle &style);
PartValues ToPartValues(const RadioStyle &style);
PartValues ToPartValues(const SliderStyle &style);
PartValues ToPartValues(const ComboStyle &style);
PartValues ToPartValues(const SelectableStyle &style);
PartValues ToPartValues(const HeaderStyle &style);
PartValues ToPartValues(const TreeStyle &style);
PartValues ToPartValues(const TabStyle &style);
PartValues ToPartValues(const ProgressStyle &style);
PartValues ToPartValues(const ColorEditStyle &style);

// Resolution. `call` is the style passed to one call (layer 1) or null. Values are screen pixels.
float ResolvePartVar(Part part, PartVar var, const PartValues *call = nullptr, PartLayer *layer = nullptr);
ImVec4 ResolvePartColor(Part part, PartState state, PartColor color, const PartValues *call = nullptr,
                        PartLayer *layer = nullptr);
// True when some layer above the default sets anything for the part; parts that change nothing skip extra work.
bool PartCustomized(Part part, const PartValues *call = nullptr);

struct ResolvedPartStyle {
    Part part = Part::Button;
    std::array<float, PartVarCount> vars{};
    std::array<std::array<ImVec4, PartColorCount>, PartStateCount> colors{};
    std::array<PartLayer, PartVarCount> varLayers{};
    std::array<std::array<PartLayer, PartColorCount>, PartStateCount> colorLayers{};
    float Var(PartVar var) const noexcept { return vars[static_cast<std::size_t>(var)]; }
    ImVec4 Color(PartState state, PartColor color) const noexcept {
        return colors[static_cast<std::size_t>(state)][static_cast<std::size_t>(color)];
    }
};
ResolvedPartStyle ResolvePartStyle(Part part, const PartValues *call = nullptr);
template <class Style> ResolvedPartStyle ResolvePartStyle(Part part, const Style &style) {
    const PartValues values = ToPartValues(style);
    return ResolvePartStyle(part, values.Empty() ? nullptr : &values);
}

// Layer 2 scopes, like ImGui::PushStyleVar / PushStyleColor. Values are design pixels.
void PushPartVar(Part part, PartVar var, float value);
void PopPartVar(int count = 1);
void PushPartColor(Part part, PartState state, PartColor color, ImVec4 value);
void PopPartColor(int count = 1);

// Named styles, like CSS classes. One name may hold a style for several parts; registering again replaces that part's
// entry. Names live in the current ImGui context.
void RegisterStyle(std::string_view name, Part part, const PartStyle &style);
void RegisterStyle(std::string_view name, const ButtonStyle &style);
void RegisterStyle(std::string_view name, const CheckboxStyle &style);
void RegisterStyle(std::string_view name, const RadioStyle &style);
void RegisterStyle(std::string_view name, const SliderStyle &style);
void RegisterStyle(std::string_view name, const DragStyle &style);
void RegisterStyle(std::string_view name, const InputStyle &style);
void RegisterStyle(std::string_view name, const ComboStyle &style);
void RegisterStyle(std::string_view name, const SelectableStyle &style);
void RegisterStyle(std::string_view name, const HeaderStyle &style);
void RegisterStyle(std::string_view name, const TreeStyle &style);
void RegisterStyle(std::string_view name, const TabStyle &style);
void RegisterStyle(std::string_view name, const ProgressStyle &style);
void RegisterStyle(std::string_view name, const ColorEditStyle &style);
void UnregisterStyle(std::string_view name);
bool IsStyleRegistered(std::string_view name);

// Applies a registered style to everything drawn until the scope ends. Scopes nest; the inner one wins.
// An unknown name is allowed and changes nothing.
class StyleScope {
  public:
    explicit StyleScope(std::string_view name);
    ~StyleScope();
    StyleScope(const StyleScope &) = delete;
    StyleScope &operator=(const StyleScope &) = delete;

  private:
    ImGuiContext *context_;
};

// L4: interaction for custom parts. Registers the rectangle as an item (ImGui::ItemAdd), so IsItemHovered, tooltips,
// BeginDisabled and keyboard navigation work, and returns the pointer state with eased 0..1 values kept in the window's
// ImGui state storage (the host keeps nothing).
enum InteractFlags_ : int {
    InteractFlags_None = 0,
    InteractFlags_Layout = 1 << 0,        // Also reserve the rectangle in the layout (ImGui::ItemSize).
    InteractFlags_NoNavFocus = 1 << 1,    // Not reachable with the keyboard.
    InteractFlags_PressOnClick = 1 << 2,  // Report `pressed` on mouse down instead of release.
    InteractFlags_AllowOverlap = 1 << 3,  // Let later items take the hover.
};
using InteractFlags = int;
struct InteractResult {
    ImGuiID id = 0;
    ImVec2 min{0, 0}, max{0, 0};
    bool visible = false;      // False when clipped; the other flags are then false.
    bool hovered = false, held = false, pressed = false;
    bool focused = false;      // Keyboard navigation is on this item.
    bool focusVisible = false; // Focused and the navigation cursor is shown: draw a focus ring.
    bool disabled = false;
    float hover01 = 0, active01 = 0, focus01 = 0; // Eased with the theme's control motion.
};
InteractResult Interact(ImGuiID id, ImVec2 min, ImVec2 max, InteractFlags flags = InteractFlags_None);
// The same at the cursor position, reserving `size` in the layout.
InteractResult Interact(const char *strId, ImVec2 size, InteractFlags flags = InteractFlags_None);
// Eases a value toward `target` with the theme's control motion, kept in the window's state storage under `id`.
float AnimatePart(ImGuiID id, float target);

// L4 drawing helpers: the same shapes the ImKit parts use.
// Smooth (continuous-curvature) rounded rectangle; `radius` is what a circular corner would have.
void DrawSmoothRect(ImDrawList *draw, ImVec2 min, ImVec2 max, ImU32 fill, float radius);
void DrawSmoothRectBorder(ImDrawList *draw, ImVec2 min, ImVec2 max, ImU32 color, float radius, float thickness);
void DrawDashedRect(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, ImU32 color, float thickness, float dash);
// Soft shadow around a rectangle, fading over `extent` pixels.
void DrawSoftShadow(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, float extent, float alpha,
                    ImVec4 color = ImVec4(0, 0, 0, 1));
// Focus ring outside the rectangle with a gap, in the theme's focus colour. `amount` (0..1) fades it, e.g. focus01.
// A negative width or gap uses the theme's (PartVar::FocusRing, and half of it).
void DrawFocusRing(ImDrawList *draw, ImVec2 min, ImVec2 max, float radius, float amount = 1.f, float width = -1.f,
                   float gap = -1.f);
void DrawChevron(ImDrawList *draw, ImVec2 center, float size, ImU32 color, ImGuiDir direction, float thickness = 1.6f);
// Chevron turned by `angle` radians from pointing right; for arrows that rotate as a header opens.
void DrawChevronRotated(ImDrawList *draw, ImVec2 center, float size, ImU32 color, float angle, float thickness = 1.6f);
// Check mark; `progress` (0..1) draws it partly, for a mark that draws itself in.
void DrawCheckMark(ImDrawList *draw, ImVec2 center, float size, ImU32 color, float thickness = 1.8f,
                   float progress = 1.f);
} // namespace imkit
