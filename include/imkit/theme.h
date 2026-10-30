#pragma once
#include <imkit/version.h>
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
    WarmSand, Rose, Violet, Solar, HighContrastLight, HighContrastDark, Slate
};
struct ThemePresetInfo {
    ThemePreset preset;
    std::string_view id;
    std::string_view displayName;
    ColorScheme scheme;
};
enum class ContrastMode { Standard, HighContrast };
enum class Density { Compact, Comfortable, Touch };
enum class Easing { Linear, EaseOut, EaseInOut };
// Radius presets for SetCornerStyle. Soft matches the default radii.
enum class CornerStyle { Sharp, Soft, Round, Pill };
enum class TextRole { Caption, Body, Heading, Title, Mono };
enum class ElevationLevel { Raised, Overlay };
// A frame border of this many design pixels or more is an outline: a deliberate stroke drawn at full strength on
// windows, frames and every button variant. Thinner borders stay soft hairlines, as before.
inline constexpr float OutlineMinimumWidth = 2.f;
inline constexpr float ThemeScaleDefault = 1.25f;
inline constexpr float ThemeScaleMinimum = 0.50f;
inline constexpr float ThemeScaleMaximum = 2.50f;
struct Typography {
    float caption=12, body=14, label=14, heading=18, title=26, monospace=13;
};
struct SpacingTokens { std::array<float,8> steps{2,4,6,8,12,16,24,32}; };
// subtle: tags and small marks, control: frames and buttons, overlay: cards, popups and dialogs.
struct Radius { float subtle=4, control=8, overlay=12; };
// border is a hairline up to 1 and an outline from OutlineMinimumWidth; dividers and table lines never exceed 1.
struct Stroke { float border=1, focus=2; };
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
};
// Derived from `Theme::semantic` by ResolveTheme; edit semantic tokens instead.
struct Palette {
    ImVec4 canvas, surface, input, raised, text, muted, border;
    ImVec4 accent, onAccent, selection, focus, destructive, onDestructive;
    ImVec4 success, warning;
};
// Derived from the density and tokens by ResolveTheme; edit those instead.
struct Metrics {
    float controlHeight = 30, spacing = 6, radius = 8, border = 1;
    float bodySize = 14, headingSize = 18, focusWidth = 1.5f, elevation = 14;
    float horizontalPadding = 12;
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
}; // Non-owning.
struct EditorPalette {
    ImVec4 canvas{}, grid{}, ruler{}, trackHeader{}, videoClip{}, audioClip{}, captionClip{};
    ImVec4 key{}, selectedKey{}, marker{}, snapGuide{}, axisX{}, axisY{}, axisZ{}, gizmo{};
    ImVec4 scope{}, missing{}, proxy{}, error{}, locked{};
    ImVec4 effectClip{}, adjustmentClip{}, groupClip{};
};
struct Theme {
    ColorScheme scheme = ColorScheme::Light;
    ContrastMode contrast = ContrastMode::Standard;
    Density density = Density::Comfortable;
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
    const Skin *skin = nullptr; // Non-owning, like fonts; see skin.h. Not part of the theme text.
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
