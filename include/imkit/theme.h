#pragma once
#include <imkit/version.h>
#include <array>
#include <cstdint>

namespace imkit {
enum class ColorScheme { Light, Dark };
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
};
struct FontSet {
    ImFont *regular = nullptr;
    ImFont *emphasis = nullptr;
}; // Non-owning.
struct Theme {
    ColorScheme scheme = ColorScheme::Light;
    Palette colors{};
    Metrics metrics{};
    Motion motion{};
    FontSet fonts{};
};
Theme MakePrecisionTheme(ColorScheme scheme = ColorScheme::Light);
// Explicitly updates accent, onAccent, focus and selection only.
void SetAccent(Theme &theme, ImVec4 accent);
// Call outside Begin/End windows, before NewFrame. Does not load or own fonts.
void ApplyTheme(const Theme &theme, float scale = 1.0f);
// Use within a frame, before Begin or around a group of widgets. Restore on the
// same live context; scopes must be destroyed before the host destroys it.
class ThemeScope {
  public:
    explicit ThemeScope(const Theme &theme, float scale = 1.0f);
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
    float Update(ImGuiID id, float target, float dt, float seconds, int frame, bool enabled = true);
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
    };
    std::array<Entry, 256> entries_{};
    std::uint64_t generation_ = 0;
};
} // namespace imkit
