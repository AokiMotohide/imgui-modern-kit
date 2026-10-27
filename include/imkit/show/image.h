#pragma once
#include <imkit/show/controls.h>

// Picture buttons: any texture the host provides becomes a button, and end users can assign, replace, clear and
// reposition the picture. The library never loads files or owns textures; choosing a file is a request the host answers.
namespace imkit::show {
enum class ImageFit { Cover, Contain, Stretch };
enum class ImageShape { RoundedRect, Circle };
enum class ImageLabel { None, Overlay, Below };

// The part of the picture a button shows: a focal point (0..1 of the picture) and a zoom (1 = the whole fit area).
struct ImageCrop {
    float centerX = .5f, centerY = .5f, zoom = 1.f;
};

// Destination rectangle (relative to the button's top-left corner) and the part of the texture drawn into it.
struct ImageViewport {
    ImVec2 min{0, 0}, max{0, 0};
    ImVec2 uv0{0, 0}, uv1{1, 1};
};
// Places a picture of `imagePixels` in a `box`. Cover fills the box and crops the overflow, Contain letterboxes (at
// zoom 1; zoomed it behaves like Cover), Stretch ignores the aspect ratio. The focal point is kept inside the picture.
// A non-positive `imagePixels` is treated as square; non-finite crop values fall back to the defaults.
ImageViewport FitImage(ImVec2 box, ImVec2 imagePixels, ImageFit fit, ImageCrop crop = {});

struct ImageButtonView {
    ImTextureRef image{};       // Invalid or null shows the empty state.
    ImVec2 imagePixels{0, 0};   // Picture size in pixels, used for the aspect ratio.
    ImageCrop crop{};
    ImageFit fit = ImageFit::Cover;
    ImageShape shape = ImageShape::RoundedRect;
    const char* label = "";
    const char* caption = "";   // Small second line beside the label, for example a key or a duration.
    ImVec4 tint{1, 1, 1, 1};    // Multiplies the picture colour.
    ImVec4 accent{};            // Ring colour while on or playing; alpha <= 0 uses the theme accent.
    float progress = -1;        // 0..1 line along the bottom; negative hides it.
    int badge = 0;              // A count badge in the top-right corner when above zero.
    bool on = false;            // State of a Toggle button.
    bool error = false;         // Draws the error treatment over the picture.
};
struct ImageButtonOptions {
    ImVec2 size{0, 0};          // Zero draws a square of about 4 frame heights.
    ConsoleMode mode = ConsoleMode::Momentary;
    ImageLabel labelStyle = ImageLabel::Overlay;
    bool editable = false;      // Adds the empty-state call to action, the drop target and the right-click menu.
    const char* dropPayload = nullptr; // In-app drag payload type accepted when editable.
    // Optional hit test for pictures with transparent areas: returns the picture alpha at (u, v) in 0..1. Presses on
    // pixels below 0.1 are ignored. Without it the whole shape is the button.
    float (*alpha)(float u, float v, void* user) = nullptr;
    void* alphaUser = nullptr;
    const BindingInfo* binding = nullptr;
};
struct ImageButtonRequest {
    bool pressed = false;       // Primary press this frame.
    bool released = false;      // Primary release this frame.
    bool changed = false;       // A Toggle button flipped this frame.
    bool secondary = false;     // Right-click this frame.
    bool hovered = false;       // The pointer is over the button; hosts use it to pick the target of an OS file drop.
    bool chooseImage = false;   // The user asked to pick a picture (empty state click, "Choose image..." in the menu).
    bool clearImage = false;    // The user asked to remove the picture.
    bool adjustImage = false;   // The user asked to reposition it; the host shows an ImageCropEditor.
    const ImGuiPayload* payload = nullptr; // Set on the frame an in-app drag payload was dropped here.
};
ImageButtonRequest ImageButton(const char* id, const ImageButtonView& view, ImageButtonOptions layout = {}, ComponentOptions options = {});

// Repositions a picture inside the button shape: drag to pan, wheel or the slider to zoom. `crop` is edited in place;
// the returned edit reports the zoom (`value`) with started, changed and finished phases for the whole gesture.
ValueEdit ImageCropEditor(const char* id, const ImageButtonView& view, ImageCrop* crop, ImVec2 size = {0, 0}, ComponentOptions options = {});
} // namespace imkit::show
