#pragma once
#include <imkit/version.h>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace imkit {
enum class ColorScheme { Light, Dark };
enum class ThemePreset : std::uint8_t {
    PrecisionLight, PrecisionDark, Graphite, Midnight, Ocean, Forest,
    WarmSand, Rose, Violet, Solar, HighContrastLight, HighContrastDark
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
inline constexpr float ThemeScaleDefault = 1.25f;
inline constexpr float ThemeScaleMinimum = 0.50f;
inline constexpr float ThemeScaleMaximum = 2.50f;
struct Typography {
    float caption=12, body=14, label=14, heading=18, title=26, monospace=13;
};
struct SpacingTokens { std::array<float,8> steps{2,4,6,8,12,16,24,32}; };
struct Radius { float subtle=2, control=4, overlay=6; };
struct Stroke { float border=1, focus=2; };
struct Elevation { float surface=0, raised=2, overlay=5; };
struct Opacity { float disabled=1, scrim=.5f; };
struct StateColors { ImVec4 rest{}, hover{}, pressed{}, focused{}, selected{}, disabled{}; };
struct SemanticColors {
    ImVec4 canvas{}, surface{}, surfaceRaised{}, overlay{}, text{}, textSecondary{}, textDisabled{};
    ImVec4 border{}, accent{}, onAccent{}, success{}, warning{}, error{};
    StateColors control{};
};
struct Palette {
    ImVec4 canvas, surface, input, raised, text, muted, border;
    ImVec4 accent, onAccent, selection, focus, destructive, onDestructive;
    ImVec4 success, warning;
};
struct Metrics {
    float controlHeight = 28, spacing = 6, radius = 4, border = 1;
    float bodySize = 14, headingSize = 18, focusWidth = 1.5f, elevation = 5;
    float horizontalPadding = 10;
};
struct Motion {
    float controlSeconds = .06f, overlaySeconds = .10f;
    bool enabled = true;
    bool reducedMotion = false;
    Easing easing = Easing::EaseOut;
};
struct FontSet {
    ImFont *regular = nullptr;
    ImFont *emphasis = nullptr;
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
