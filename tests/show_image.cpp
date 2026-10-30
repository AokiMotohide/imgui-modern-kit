#include <imkit/imkit.h>
#include <imkit/show.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string_view>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
bool Near(double a, double b, double eps = 1e-3) { return std::abs(a - b) <= eps; }
float Transparent(float, float, void*) { return 0.f; }
} // namespace

int main() {
    using namespace imkit;
    using namespace imkit::show;

    // ---- Fitting -----------------------------------------------------------------------------------------------
    {
        const ImageViewport cover = FitImage({100, 100}, {200, 100}, ImageFit::Cover);
        Check(Near(cover.uv0.x, .25) && Near(cover.uv1.x, .75) && Near(cover.uv0.y, 0) && Near(cover.uv1.y, 1) && cover.max.x == 100 && cover.max.y == 100,
              "cover crops the overflow of a wide picture");
        const ImageViewport tall = FitImage({100, 100}, {100, 200}, ImageFit::Cover);
        Check(Near(tall.uv0.y, .25) && Near(tall.uv1.y, .75) && Near(tall.uv0.x, 0), "cover crops a tall picture vertically");
        const ImageViewport contain = FitImage({100, 100}, {200, 100}, ImageFit::Contain);
        Check(Near(contain.min.y, 25) && Near(contain.max.y, 75) && contain.min.x == 0 && Near(contain.uv1.x, 1), "contain letterboxes");
        const ImageViewport stretch = FitImage({100, 50}, {10, 10}, ImageFit::Stretch);
        Check(stretch.min.x == 0 && stretch.max.x == 100 && stretch.max.y == 50 && Near(stretch.uv1.y, 1), "stretch ignores the aspect ratio");
        const ImageViewport zoomed = FitImage({100, 100}, {200, 100}, ImageFit::Cover, {.5f, .5f, 2.f});
        Check(Near(zoomed.uv0.x, .375) && Near(zoomed.uv1.x, .625) && Near(zoomed.uv0.y, .25) && Near(zoomed.uv1.y, .75), "zoom halves the visible window");
        const ImageViewport edge = FitImage({100, 100}, {200, 100}, ImageFit::Cover, {0.f, 0.f, 1.f});
        Check(Near(edge.uv0.x, 0) && Near(edge.uv1.x, .5), "the focal point cannot leave the picture");
        const ImageViewport bad = FitImage({100, 100}, {0, 0}, ImageFit::Cover, {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()});
        Check(Near(bad.uv0.x, 0) && Near(bad.uv1.x, 1), "non-finite crop and unknown size fall back to defaults");
        const ImageViewport empty = FitImage({0, 0}, {10, 10}, ImageFit::Cover);
        Check(empty.max.x == 0 && empty.max.y == 0, "an empty box gives an empty viewport");
        const ImageViewport containZoom = FitImage({100, 100}, {200, 100}, ImageFit::Contain, {.5f, .5f, 2.f});
        Check(containZoom.min.x == 0 && containZoom.max.x == 100, "zoomed contain behaves like cover");
    }

    // ---- Components ----------------------------------------------------------------------------------------------
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1400, 1600};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int fw, fh;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fw, &fh);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 256> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    const ImTextureRef texture(static_cast<ImTextureID>(7));
    bool toggleOn = false;
    ImageCrop crop;
    ImageButtonRequest pad, toggle, empty, masked;
    ValueEdit cropEdit;
    auto frame = [&](bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        ImageButtonView view;
        view.image = texture;
        view.imagePixels = {96, 96};
        view.label = "Pad";
        view.caption = "0:42";
        view.badge = 3;
        view.progress = .4f;
        ImageButtonOptions opts;
        opts.size = {120, 90};
        pad = ImageButton("pad", view, opts, options);
        ImGui::SameLine();
        ImageButtonView tv = view;
        tv.label = "Toggle";
        tv.on = toggleOn;
        tv.shape = ImageShape::Circle;
        ImageButtonOptions to = opts;
        to.mode = ConsoleMode::Toggle;
        toggle = ImageButton("toggle", tv, to, options);
        if (toggle.changed)
            toggleOn = !toggleOn;
        ImGui::SameLine();
        ImageButtonView ev;
        ev.label = "Empty";
        ImageButtonOptions eo = opts;
        eo.editable = true;
        eo.dropPayload = "IMKIT_IMAGE";
        empty = ImageButton("empty", ev, eo, options);
        ImGui::SameLine();
        ImageButtonView mv = view;
        mv.label = "Masked";
        ImageButtonOptions mo = opts;
        mo.alpha = &Transparent;
        masked = ImageButton("masked", mv, mo, options);
        ImageButtonView cv = view;
        cv.label = "Crop";
        cropEdit = ImageCropEditor("crop", cv, &crop, {200, 200}, options);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) { return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f}; };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };
    auto click = [&](const accessibility::SemanticNode& n) {
        const ImVec2 c = centre(n);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
    };
    crop.zoom = 1.f;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid");
    Check(find("Pad") && find("Pad")->role == accessibility::SemanticRole::Button, "picture button is a button");
    Check(find("Toggle") && find("Toggle")->role == accessibility::SemanticRole::Toggle, "toggle picture button is a toggle");

    if (const auto* node = find("Pad")) {
        click(*node);
        Check(pad.pressed && pad.hovered && !pad.chooseImage, "a press is reported");
        release();
        Check(!pad.pressed, "no press reported while idle");
    } else
        Check(false, "pad node");
    if (const auto* node = find("Toggle")) {
        click(*node);
        Check(toggle.changed, "a toggle button reports the flip");
        release();
        Check(toggleOn, "the host applied the flip");
    } else
        Check(false, "toggle node");
    if (const auto* node = find("Empty")) {
        click(*node);
        Check(empty.chooseImage && empty.pressed, "clicking an empty editable button asks for a picture");
        release();
        const ImVec2 c = centre(*node);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(1, true);
        frame();
        Check(empty.secondary, "right-click is reported");
        io.AddMouseButtonEvent(1, false);
        frame();
        // A click on empty space closes the edit menu the right-click opened (and is consumed by it).
        io.AddMousePosEvent(1300.f, 1500.f);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        release();
    } else
        Check(false, "empty node");
    if (const auto* node = find("Masked")) {
        click(*node);
        Check(!masked.pressed, "a transparent pixel ignores the press");
        release();
        Check(!masked.released, "and the release");
    } else
        Check(false, "masked node");

    // Crop editor: dragging left pans the window to the right.
    if (const auto* node = find("Adjust position...")) {
        const ImVec2 c = centre(*node);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(cropEdit.started, "the crop gesture starts");
        for (int i = 1; i <= 4; ++i) {
            io.AddMousePosEvent(c.x - 10.f * static_cast<float>(i), c.y);
            frame();
        }
        Check(cropEdit.changed && crop.centerX > .5f, "dragging left moves the focal point right");
        release();
        Check(cropEdit.finished || crop.centerX > .5f, "the crop gesture ends");
    } else
        Check(false, "crop editor node");
    ImGui::DestroyContext();
    std::puts(failures ? "show image tests failed" : "show image tests passed");
    return failures ? 1 : 0;
}
