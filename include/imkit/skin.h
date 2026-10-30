#pragma once
#include <imkit/components.h>

// Skin: replaces the frame and background of ImKit components with the host's own drawing. Everything else stays with
// the library: label, icon, focus ring, accessibility, tooltips, input and return values. Attach it to the theme:
//
//     imkit::Skin skin{&MyDrawFrame, &myState};
//     theme.skin = &skin;                       // non-owning, like theme.fonts
//
// A skin only reaches components that receive the theme (ComponentOptions::theme); components called without one keep
// the default look. Components the skin does not know are drawn by default.
namespace imkit {
// New parts are only ever appended; a skin must return false for a part it does not recognise.
enum class SkinPart { Button, Badge, Card };

struct SkinFrame {
    ImDrawList* draw = nullptr;
    ImVec2 min{0, 0}, max{0, 0};          // The frame to paint. A pressed button with a hard shadow is already moved.
    float rounding = 0;
    SkinPart part = SkinPart::Button;
    ActionVariant variant = ActionVariant::Secondary; // Button.
    StatusKind status = StatusKind::Neutral;          // Badge and Card.
    bool hovered = false, active = false, focused = false, disabled = false;
    float hover = 0;                      // 0..1, eased when the component has an AnimationState.
    const Theme* theme = nullptr;
};

struct Skin {
    // Paints the frame. Return true when it was painted; false keeps the default frame. Must not submit items.
    // Colours given to ImGui::GetColorU32(ImVec4) already follow a surrounding BeginDisabled(); raw ImU32 colours do not.
    bool (*drawFrame)(const SkinFrame& frame, void* user) = nullptr;
    void* user = nullptr;
};
} // namespace imkit
