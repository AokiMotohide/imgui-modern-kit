#pragma once
#include <imkit/version.h>
#include <imkit/easing.h>
#include <imkit/part_style.h>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace imkit {
struct Skin;
enum class ColorScheme { Light, Dark };
enum class ThemePreset : std::uint8_t {
    PrecisionLight, PrecisionDark, Graphite, Midnight, Ocean, Forest,
    WarmSand, Rose, Violet, Solar, HighContrastLight, HighContrastDark, Slate,
    Classic // The Precision Dark colours with ThemeLook::Classic: the Dear ImGui shapes of ImKit 4.1.
};
// Modern is the default for every preset: thin scrollbars, faint dividers, rounded floating windows and popups,
// calm tables and the redrawn ImKit parts. Classic keeps the Dear ImGui shapes for hosts that want the previous look.
enum class ThemeLook : std::uint8_t { Modern, Classic };
// Surface finish for Modern look. Flat is the default: clean flat fills and subtle hairlines.
// Tactile adds subtle vertical gradients, top specular highlights and layered shadows.
enum class SurfaceStyle : std::uint8_t { Flat, Tactile };
struct ThemePresetInfo {
    ThemePreset preset;
    std::string_view id;
    std::string_view displayName;
    ColorScheme scheme;
};
enum class ContrastMode { Standard, HighContrast };
enum class Density { Compact, Comfortable, Touch };
// Radius presets for SetCornerStyle. Soft matches the default radii: a smooth corner at a quarter of the control height.
// There is no capsule preset: a fully round button reads as a tag and leaves less room for its label.
enum class CornerStyle { Sharp, Soft, Round };
// Numeric is the body size in fonts.numeric (regular when null), for figures that line up in fields and readouts.
enum class TextRole { Caption, Body, Heading, Title, Mono, Numeric };
enum class ElevationLevel { Raised, Overlay };
// Where an overlay sits in a viewport's work area: toasts, the external control pill and similar floating parts.
enum class OverlayAnchor { TopLeft, TopCenter, TopRight, BottomLeft, BottomCenter, BottomRight };
// A frame border of this many design pixels or more is an outline: a deliberate stroke drawn at full strength on
// windows, frames and every button variant. Thinner borders stay soft hairlines, as before.
inline constexpr float OutlineMinimumWidth = 2.f;
inline constexpr float ThemeScaleDefault = 1.25f;
inline constexpr float ThemeScaleMinimum = 0.50f;
inline constexpr float ThemeScaleMaximum = 2.50f;
// Four steps a reader can tell apart at a glance: captions, body text, headings one step above the body, and titles.
struct Typography {
    float caption=12, body=14, label=14, heading=16, title=22, monospace=13;
};
struct SpacingTokens { std::array<float,8> steps{2,4,6,8,12,16,24,32}; };
// subtle: tags and small marks, control: frames and buttons, overlay: cards, popups and dialogs.
// controlShare > 0 derives all three from the control height when the theme resolves: control = height * share,
// subtle = half of it and overlay = height * share * 1.6, rounded to whole pixels. The default .25 keeps the same
// proportion at every density. Set it to 0 to use the pixel values as given.
struct Radius { float subtle=4, control=8, overlay=12; float controlShare=.25f; };
// border is a hairline up to 1 and an outline from OutlineMinimumWidth; dividers and table lines never exceed 1.
// `button` is the outline of buttons when it differs from other frames (0 uses `border`); `dashedDisabled` draws the
// outline of a disabled ImKit button as a dashed line, so it reads as "not now" without relying on colour alone.
// `primary` gives the primary button an outline of its own width (0 keeps `button`), drawn in `primaryColor`
// (alpha zero uses onAccent), so the one action to press stands out while other buttons keep a hairline.
struct Stroke { float border=1, focus=2; float button=0; bool dashedDisabled=false; float primary=0; ImVec4 primaryColor{}; };
// Shadow extent in pixels. OverlayDecoration draws overlay; raised is available to hosts through DrawShadow.
// hard replaces the blurred shadow with a solid copy of the shape moved by offset (design pixels, scaled with the UI).
struct Elevation { float surface=0, raised=4, overlay=14; ImVec2 offset{0,0}; bool hard=false; };
struct Opacity { float disabled=.55f, scrim=.5f; };
struct StateColors { ImVec4 rest{}, hover{}, pressed{}, focused{}, selected{}, disabled{}; };
struct SemanticColors {
    ImVec4 canvas{}, surface{}, surfaceRaised{}, overlay{}, text{}, textSecondary{}, textDisabled{};
    ImVec4 border{}, accent{}, onAccent{}, success{}, warning{}, error{};
    StateColors control{};
    ImVec4 shadow{}; // Alpha <= 0 uses black. A soft shadow takes only its colour; a hard shadow uses it as is.
    ImVec4 crown{};  // Optional colour of the top of the window (menu bar, step band). Alpha zero derives it.
};
// Derived from `Theme::semantic` by ResolveTheme; edit semantic tokens instead.
struct Palette {
    ImVec4 canvas, surface, input, raised, text, muted, border;
    ImVec4 accent, onAccent, selection, focus, destructive, onDestructive;
    ImVec4 success, warning;
};
// Sizes that components read while drawing. Ratios are multiples of the font size, so they follow the UI scale; the two
// pixel values are screen pixels, as the literals they replace were. Defaults draw exactly as before.
struct PartMetrics {
    float checkHeightScale = 1.15f;       // Checkbox and RadioButton height; close to the text so the mark never outweighs it.
    float toggleHeightScale = 1.75f;      // Toggle and Switch height.
    float dialogPaddingScale = 1.3f;      // Dialog window padding.
    float progressBarHeightScale = .2f;   // Progress bar height as a share of the frame height (kept within 4..8).
    float badgeExtraHeight = 8.f;         // StatusBadge height beyond the font size, in pixels.
    float segmentedInset = 3.f;           // Segmented control inset, in pixels.
};
// Derived from the density, tokens and `Theme::layout` by ResolveTheme; edit those instead.
struct Metrics {
    float controlHeight = 30, spacing = 6, radius = 8, border = 1;
    float bodySize = 14, headingSize = 15, focusWidth = 1.5f, elevation = 14;
    float horizontalPadding = 12;
    float spacingY = 6;                   // Vertical item spacing; `spacing` is the horizontal one.
    float windowPaddingX = 12, windowPaddingY = 12;
    float cellPaddingY = 4, indent = 18, scrollbar = 10, grabMin = 14, separatorTextPaddingY = 6;
    PartMetrics parts{};
};
// Overrides for the sizes that decide how tall controls are. An unset value keeps the density preset (or the
// component's own ratio), so a theme that sets nothing draws as before. ResolveTheme applies them after the density.
// Lengths are design pixels scaled with the UI; `*Scale` values are multiples of the font size.
struct LayoutTokens {
    std::optional<float> controlHeight, spacingX, spacingY, horizontalPadding;
    std::optional<float> windowPaddingX, windowPaddingY, cellPaddingY, indent, scrollbar, grabMin, separatorTextPaddingY;
    std::optional<float> checkHeightScale, toggleHeightScale, dialogPaddingScale, progressBarHeightScale;
    std::optional<float> badgeExtraHeight, segmentedInset; // Screen pixels.
};
struct Motion {
    float controlSeconds = .14f, overlaySeconds = .20f;
    bool enabled = true;
    bool reducedMotion = false;
    Easing easing = Easing::EaseOut;
};
struct FontSet {
    ImFont *regular = nullptr;
    ImFont *emphasis = nullptr;
    ImFont *display = nullptr; // Headings and titles; PushTextRole falls back to emphasis, then regular.
    ImFont *numeric = nullptr; // Figures in number fields (TextRole::Numeric); null uses regular.
}; // Non-owning.
struct EditorPalette {
    ImVec4 canvas{}, grid{}, ruler{}, trackHeader{}, videoClip{}, audioClip{}, captionClip{};
    ImVec4 key{}, selectedKey{}, marker{}, snapGuide{}, axisX{}, axisY{}, axisZ{}, gizmo{};
    ImVec4 scope{}, missing{}, proxy{}, error{}, locked{};
    ImVec4 effectClip{}, adjustmentClip{}, groupClip{};
};
enum class NodeHeaderFill : std::uint8_t { Strip, Tinted, Flat, Gradient, None };
enum class NodeLinkColor : std::uint8_t { OutputType, InputType, Gradient, Accent };
enum class NodeLinkArrow : std::uint8_t { Middle, None, End };
// Node editor look, read by node_editor::MakeNodeStyle. Colours with alpha zero and unset numbers follow the
// semantic tokens, so a theme that never mentions nodes still styles them.
struct NodeTokens {
    NodeHeaderFill headerFill = NodeHeaderFill::Strip;
    NodeLinkColor linkColor = NodeLinkColor::OutputType;
    NodeLinkArrow linkArrow = NodeLinkArrow::Middle;
    ImVec4 canvas{}, grid{}, header{};
    std::optional<float> headerTint, rounding, headerHeight, pinRadius, linkWidth, gridSize, pinSeat, flowLinkWidth;
    bool hollowPins = false, highlightLinks = false;
};
// The large regions of an application window. Neighbouring regions differ in lightness, so the toolbar band, the side
// panels, the work area and a strip of video previews read as separate places in every theme. Panel (lists) and
// Inspector (settings) share a lightness and differ in hue, so two side panels read as two roles without either
// standing out.
enum class SurfaceZone : std::uint8_t { Chrome, Panel, Workspace, Monitor, Inspector, Header, ChromeCanvas };
// The part a docked window plays. Each role takes the zone of that part as its background, so neighbouring windows read
// as different places by colour as well as by position; the focused window's tab line takes the accent.
enum class WindowRole : std::uint8_t { Browse, Edit, View, Monitor };
// Colours of one window role, derived by ResolveTheme from the zones. The emphasis stays at least 1.25:1 from the body.
struct WindowRoleColors {
    ImVec4 body{};     // Window and child background: panel, inspector, workspace or chromeCanvas.
    ImVec4 emphasis{}; // Section headers and the selected row: one step in front of the body, in its hue.
    ImVec4 label{};    // Tab line while the window is not focused, in the body's hue (the chrome's for Monitor).
    ImVec4 head{};     // The tab band of a dock node (DockTabBand): at least 1.25:1 from the body, so the selected
                       // tab (the body colour) reads as part of the window below it.
};
// Derived from `Theme::semantic` by ResolveTheme in OKLCH, so the steps are even to the eye in every theme. To override
// a value, assign it after ResolveTheme.
struct SurfaceZoneColors {
    ImVec4 chrome{};       // Menu bar and step band: semantic.crown when given, else the raised surface leaning to the accent.
    ImVec4 panel{};        // Lists and step lists: the surface.
    ImVec4 workspace{};    // Gaps between panels and around the main view: a step darker than the panel.
    ImVec4 monitor{};      // Inside a picture: an almost neutral, deep tone, so pictures are judged on a quiet ground.
    ImVec4 onMonitor{};    // Text drawn on the monitor zone.
    ImVec4 divider{};      // The line between two zones.
    ImVec4 inspector{};    // Settings panels: the panel's lightness, turned toward the accent's hue and calmer.
    ImVec4 header{};       // The heading row of a panel and floating tool windows: a step from the panel.
    ImVec4 chromeCanvas{}; // A canvas inside the chrome (a strip of previews): a deep tone of the chrome.
    std::array<WindowRoleColors, 4> roles{}; // Indexed by WindowRole.
};
struct Theme {
    ColorScheme scheme = ColorScheme::Light;
    ContrastMode contrast = ContrastMode::Standard;
    Density density = Density::Comfortable;
    LayoutTokens layout{}; // Optional size overrides on top of the density.
    SemanticColors semantic{};
    Typography typography{};
    SpacingTokens spacing{};
    Radius radius{};
    Stroke stroke{};
    Elevation elevation{};
    Opacity opacity{};
    Palette colors{};
    Metrics metrics{};
    Motion motion{};
    FontSet fonts{};
    EditorPalette editor{};
    NodeTokens node{};
    ThemeLook look = ThemeLook::Modern;
    SurfaceStyle surfaceStyle = SurfaceStyle::Flat;
    // Per part and state values (L1, part_style.h): `button.height`, `checkbox.on.fill`, `slider.thumb.size`.
    // Unset entries follow the tokens above. Lengths are design pixels.
    PartOverrides partOverrides{};
    const Skin *skin = nullptr; // Non-owning, like fonts; see skin.h. Not part of the theme text.
    SurfaceZoneColors zones{};  // Derived; see SurfaceZoneColors.
};
Theme MakePrecisionTheme(ColorScheme scheme = ColorScheme::Light);
std::span<const ThemePresetInfo> ThemePresets() noexcept;
std::optional<ThemePreset> ThemePresetFromId(std::string_view id) noexcept;
Theme MakeTheme(ThemePreset preset);
Theme MakeTheme(ColorScheme scheme=ColorScheme::Light, ContrastMode contrast=ContrastMode::Standard,
                Density density=Density::Comfortable);
// Rebuilds the legacy rendering projections from semantic tokens, without changing fonts.
void ResolveTheme(Theme& theme);
void SetDensity(Theme& theme, Density density);
float ContrastRatio(ImVec4 foreground, ImVec4 background);
bool ValidateContrast(const Theme& theme);
// Explicitly updates accent, onAccent, focus and selection only.
void SetAccent(Theme &theme, ImVec4 accent);
// Colours that define a brand. canvas, surface, text and accent are required (their alpha is ignored); every other
// colour is derived when its alpha is zero.
struct BrandPalette {
    ImVec4 canvas{}, surface{}, text{}, accent{};
    ImVec4 raised{}, border{}, textSecondary{}, success{}, warning{}, error{};
    ImVec4 crown{}; // The top of the window (semantic.crown).
};
// Builds a complete, resolved theme from a few colours: control states, selection, secondary text and a border are
// derived so that text and border keep their contrast against every surface. The light or dark scheme follows the
// canvas. Check ValidateContrast(theme) afterwards; a low result is a warning, the theme is still usable.
// Typography, radii, stroke, elevation and motion keep their defaults; adjust them on the returned value.
Theme MakeBrandTheme(const BrandPalette& brand);
// Sets radius.subtle, control and overlay together. Call ApplyTheme afterwards.
void SetCornerStyle(Theme& theme, CornerStyle style);
// Pushes the font and size of a text role: sizes come from Theme::typography, headings use fonts.display (then
// emphasis, then regular). Pair with PopTextRole; this does not load fonts.
void PushTextRole(const Theme& theme, TextRole role);
void PopTextRole();
// Writes every theme token that ParseTheme understands as `key = value` lines. Fonts and the skin are not written.
std::string SerializeTheme(const Theme& theme);
struct ThemeParseResult {
    int applied = 0;     // Lines that changed the theme.
    int ignored = 0;     // Lines skipped: unknown key or invalid value.
    std::string warnings; // One "line N: ..." entry per ignored line, capped.
};
// Applies `key = value` lines to `theme`, which acts as the base: only the keys that appear change. A `#` starts a
// comment line; colours are #RRGGBB or #RRGGBBAA. Order of application: `preset` (replaces the base, keeps fonts),
// then the `brand.*` colours (MakeBrandTheme), then every other key from top to bottom. Invalid lines are skipped with
// a warning and never stop the rest. The theme is resolved at the end. See docs/architecture/make-your-own-look.md for the keys.
ThemeParseResult ParseTheme(std::string_view text, Theme& theme);
// Call outside Begin/End windows, before NewFrame. Does not load or own fonts.
void ApplyTheme(const Theme &theme, float scale = ThemeScaleDefault);
ImVec4 SurfaceZoneColor(const Theme& theme, SurfaceZone zone);
// Pushes the zone colour as the window background for the next Begin; call PopSurfaceZone right after that Begin.
void PushSurfaceZone(const Theme& theme, SurfaceZone zone);
void PopSurfaceZone();
// Pushes the colours of a window role for the next Begin: background, child background, headers and the docked tab
// (the focused tab line takes the accent, an unfocused one the role label). Call PopWindowRole after End, so the widgets
// inside use the role colours too. Input fields keep the theme's own colour. An unresolved theme pushes the current
// style colours. The tab line shows only when ImGuiStyle::TabBarOverlineSize is above zero.
void PushWindowRole(const Theme& theme, WindowRole role);
void PopWindowRole();
// Paints the tab band of the current window's dock node with the role's head colour. Dear ImGui draws one band per dock
// node, so the leftmost tab's window decides it: call this right after Begin in every docked window that has a role,
// whatever Begin returned. It recolours the band Dear ImGui drew this frame (and the dimmed tabs that share its colour);
// it does nothing for a window that is not docked, a node without a tab bar, or an unresolved theme.
void DockTabBand(const Theme& theme, WindowRole role);
// Use within a frame, before Begin or around a group of widgets. Restore on the
// same live context; scopes must be destroyed before the host destroys it.
class ThemeScope {
  public:
    explicit ThemeScope(const Theme &theme, float scale = ThemeScaleDefault);
    ~ThemeScope();
    ThemeScope(const ThemeScope &) = delete;
    ThemeScope &operator=(const ThemeScope &) = delete;

  private:
    ImGuiContext *context_;
    ImGuiStyle previous_;
    PartMetrics previousParts_;
};
// Host-owned, fixed capacity. Separate instance per context. No retained context
// pointer: bind with a host-issued generation token; Reset before context reuse.
class AnimationState {
  public:
    void Reset(std::uint64_t contextGeneration = 0);
    float Update(ImGuiID id, float target, float dt, float seconds, int frame, bool enabled = true,
                 Easing easing = Easing::Linear);
    void Prune(int frame, int maxAge = 120);
    std::uint64_t Generation() const {
        return generation_;
    }

  private:
    struct Entry {
        ImGuiID id = 0;
        float value = 0;
        int frame = -1;
        bool used = false;
        float from = 0, target = 0, elapsed = 0;
    };
    std::array<Entry, 256> entries_{};
    std::uint64_t generation_ = 0;
};
} // namespace imkit
