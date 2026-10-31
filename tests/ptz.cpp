#include <imkit/imkit.h>
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
}

int main() {
    using namespace imkit;
    // Pad vector: zero in the dead zone, linear ramp, clamped at the rim, finite for hostile input.
    Check(PtzPadVector({50, 50}, {50, 50}, 40, .3f).x == 0, "centre is zero");
    Check(PtzPadVector({50 + 10, 50}, {50, 50}, 40, .3f).x == 0, "inside dead zone is zero");
    const auto rim = PtzPadVector({50 + 40, 50}, {50, 50}, 40, .3f);
    Check(std::abs(rim.x - 1.f) < 1e-5f && rim.y == 0, "rim reaches full magnitude");
    const auto outside = PtzPadVector({50, 50 - 400}, {50, 50}, 40, .3f);
    Check(std::abs(outside.y + 1.f) < 1e-5f, "beyond the rim is clamped and points up in screen axes");
    const auto half = PtzPadVector({50 + 40 * .65f, 50}, {50, 50}, 40, .3f);
    Check(std::abs(half.x - .5f) < 1e-4f, "linear ramp between dead zone and rim");
    Check(PtzPadVector({std::numeric_limits<float>::quiet_NaN(), 0}, {0, 0}, 40, .3f).x == 0, "non-finite point is ignored");
    Check(PtzPadVector({10, 10}, {0, 0}, 0, .3f).x == 0, "zero radius is ignored");

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {900, 700};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 160> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    std::array<PtzPreset, 4> presets{{{"Wide", true}, {"", true}, {"", false}, {"", false}}};
    PtzView view;
    view.label = "Camera 1";
    view.position = {-12.5f, 4.f, 2.f, true};
    view.zoomFraction = .3f;
    view.presets = presets;
    PtzState state;
    PtzRequest request;
    float hostWidth = io.DisplaySize.x;
    float low = 20, high = 80;
    int ilow = 3, ihigh = 8;
    bool changedLow = false;
    auto frame = [&](bool connected = true, bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        view.connected = connected;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({hostWidth, io.DisplaySize.y});
        ImGui::Begin("Host");
        request = PtzControl("ptz", view, state, {}, options);
        PtzOptions compact;
        compact.compact = true;
        static PtzState compactState;
        PtzView compactView;
        compactView.connected = connected;
        compactView.capabilities = view.capabilities;
        PtzControl("compact", compactView, compactState, compact, options);
        ImGui::SetNextItemWidth(400);
        changedLow = RangeSlider("range", &low, &high, 0, 100, "%.0f", {}, options);
        RangeSliderOptions integer;
        integer.step = 2.f;
        integer.ticks = 5;
        RangeSliderInt("range-int", &ilow, &ihigh, 0, 10, "%d", integer, options);
        float hostileLow = 1, hostileHigh = 2;
        RangeSlider("hostile", &hostileLow, &hostileHigh, std::numeric_limits<float>::quiet_NaN(), 100, "%s%s", {}, options);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name, std::string_view description = {}) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name && (description.empty() || n.description == description))
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) {
        return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f};
    };

    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid");
    Check(find("Up") && find("Down left") && find("Home") && find("Zoom in") && find("Zoom out"), "direction, home and zoom buttons exist");
    Check(find("Wide") && find("Preset 2") && find("Preset 4"), "presets are named or numbered");
    Check(!request.moving && !request.stop, "idle without input");

    // Holding an arrow reports full speed scaled by the speed setting; release reports one stop.
    state.speed = .5f;
    if (const auto* up = find("Up")) {
        io.AddMousePosEvent(centre(*up).x, centre(*up).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        frame();
        Check(request.moving && request.velocity.y > .4f && request.velocity.y <= .5f + 1e-4f && request.velocity.x == 0,
              "held up moves tilt up at speed");
        io.AddMouseButtonEvent(0, false);
        frame();
        frame();
        Check(!request.moving, "release stops movement");
    } else
        Check(false, "up button");
    // Stop is reported on exactly one frame after the release.
    if (const auto* zoomIn = find("Zoom in")) {
        io.AddMousePosEvent(centre(*zoomIn).x, centre(*zoomIn).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        frame();
        Check(request.zoom > 0 && request.moving, "held zoom in reports positive zoom");
        io.AddMouseButtonEvent(0, false);
        int stops = 0;
        for (int i = 0; i < 4; ++i) {
            frame();
            stops += request.stop ? 1 : 0;
        }
        Check(stops == 1, "stop is reported once after release");
    } else
        Check(false, "zoom in button");
    // Presets: a stored slot recalls, an empty slot stores.
    if (const auto* wide = find("Wide")) {
        io.AddMousePosEvent(centre(*wide).x, centre(*wide).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(request.recallPreset == 0 && request.storePreset == -1, "click recalls a stored preset");
    }
    if (const auto* empty = find("Preset 3")) {
        io.AddMousePosEvent(centre(*empty).x, centre(*empty).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(request.storePreset == 2 && request.recallPreset == -1, "click stores an empty slot");
    }
    // Disconnected: every control is inert and an in-progress move is stopped.
    if (const auto* up = find("Up")) {
        io.AddMousePosEvent(centre(*up).x, centre(*up).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(request.moving, "move starts while connected");
        frame(false);
        Check(request.stop && !request.moving, "disconnect ends the move with a stop");
        io.AddMouseButtonEvent(0, false);
        frame(false);
    }
    // Capabilities: a pan-only head without Home, zoom or storing draws only what it can do.
    auto press = [&](const accessibility::SemanticNode& n, int frames = 2) {
        io.AddMousePosEvent(centre(n).x, centre(n).y);
        frame();
        io.AddMouseButtonEvent(0, true);
        for (int i = 0; i < frames; ++i)
            frame();
    };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        frame();
        frame();
    };
    view.capabilities = {};
    view.capabilities.tilt = false;
    view.capabilities.zoom = false;
    view.capabilities.home = false;
    view.capabilities.presetStore = false;
    view.readoutText.pan = "0.25";
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid for a pan-only head");
    Check(find("Left") && find("Right"), "pan-only head keeps left and right");
    Check(!find("Up") && !find("Down") && !find("Up left") && !find("Down right"), "pan-only head drops tilt and diagonals");
    Check(!find("Zoom in") && !find("Zoom out") && !find("Home"), "pan-only head drops zoom and Home");
    if (const auto* right = find("Right")) {
        state.speed = .5f;
        press(*right);
        Check(request.moving && request.velocity.x > .4f && request.velocity.y == 0, "right moves pan only");
        release();
    } else
        Check(false, "right button");
    // A click is reported on the frame the button is released, so read the request before it is cleared.
    if (const auto* empty = find("Preset 3")) {
        press(*empty, 1);
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(request.storePreset == -1 && request.recallPreset == -1, "empty slot is inert without store");
        frame();
    }
    if (const auto* wide = find("Wide")) {
        press(*wide, 1);
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(request.recallPreset == 0 && request.storePreset == -1, "stored slot still recalls without store");
        frame();
    }
    // Without variable speed the slider is gone and velocity is full scale.
    view.capabilities = {};
    view.capabilities.speed = false;
    state.speed = .25f;
    for (int i = 0; i < 2; ++i)
        frame();
    if (const auto* up = find("Up")) {
        press(*up);
        Check(request.moving && std::abs(request.velocity.y - 1.f) < 1e-4f, "fixed-speed camera ignores the speed setting");
        release();
    } else
        Check(false, "up button at fixed speed");
    // A zoom-only lens has no pad.
    view.capabilities = {};
    view.capabilities.pan = false;
    view.capabilities.tilt = false;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(!find("Up") && !find("Left") && !find("Home"), "zoom-only lens has no pad");
    if (const auto* zoomOut = find("Zoom out")) {
        press(*zoomOut);
        Check(request.moving && request.zoom < 0 && request.velocity.x == 0 && request.velocity.y == 0, "zoom-only lens zooms out");
        release();
    } else
        Check(false, "zoom out button on a zoom-only lens");
    // Nothing to offer: only a notice is drawn and nothing is ever requested.
    view.capabilities = {};
    view.capabilities.pan = view.capabilities.tilt = view.capabilities.zoom = false;
    view.capabilities.presets = false;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid with no controls");
    Check(!find("Up") && !find("Zoom in") && !find("Wide") && !request.moving && request.recallPreset == -1, "no controls draw no buttons");
    view.capabilities = {};
    view.readoutText = {};
    for (int i = 0; i < 2; ++i)
        frame();
    // A narrow host: the automatic pad shrinks so the pad and the zoom rocker stay inside the window.
    view.capabilities = {};
    hostWidth = 7.f * ImGui::GetFrameHeight(); // Narrower than the natural pad and rocker, wider than the minimum.
    for (int i = 0; i < 3; ++i)
        frame();
    if (const auto* zoomIn = find("Zoom in"); zoomIn && find("Right"))
        Check(zoomIn->maximum.x <= hostWidth && find("Right")->maximum.x <= hostWidth, "pad and zoom rocker fit a narrow host");
    else
        Check(false, "zoom rocker in a narrow host");
    hostWidth = io.DisplaySize.x;
    for (int i = 0; i < 2; ++i)
        frame();
    frame(true, true); // Reduced motion draws without animation state.
    Check(semantics.Tree().Validate(), "semantic tree stays valid after reduced motion");

    // RangeSlider: ordering, clamping, hostile input and accessibility steps.
    low = 90;
    high = 10;
    ilow = -5;
    ihigh = 99;
    frame();
    Check(low <= high && low >= 0 && high <= 100, "swapped input is normalised");
    Check(ilow == 0 && ihigh == 10, "integer input is clamped");
    low = 20;
    high = 80;
    frame();
    if (const auto* handle = find("range", "Minimum")) {
        accessibility::ActionRequest increment;
        increment.id = handle->id;
        increment.action = accessibility::SemanticAction::Increment;
        actions.Push(increment);
        frame();
        Check(changedLow && low > 20 && low < 22 && high == 80, "increment action nudges the low handle");
        const auto* after = find("range", "Minimum");
        Check(after && after->minimumValue == 0 && after->maximumValue == 100, "handle announces its range");
    } else
        Check(false, "low handle node");
    ImGui::DestroyContext();
    std::printf("PTZ failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
