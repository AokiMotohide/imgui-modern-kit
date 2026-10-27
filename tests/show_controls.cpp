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
} // namespace

int main() {
    using namespace imkit;
    using namespace imkit::show;

    // Pure mappings.
    Check(Near(FaderPosition(0, 0, 1, FaderScale::Linear), 0) && Near(FaderPosition(1, 0, 1, FaderScale::Linear), 1), "linear endpoints");
    Check(Near(FaderPosition(0, -60, 6, FaderScale::Decibel), .7516, .01), "0 dB sits about three quarters up");
    Check(Near(FaderValue(FaderPosition(-18, -60, 6, FaderScale::Decibel), -60, 6, FaderScale::Decibel), -18, 1e-3), "decibel taper round trips");
    Check(FaderPosition(std::numeric_limits<float>::quiet_NaN(), 0, 1, FaderScale::Linear) == 0, "non-finite value is ignored");
    Check(FaderValue(5, 0, 10, FaderScale::Linear) == 10 && FaderValue(-1, 0, 10, FaderScale::Linear) == 0, "position is clamped");
    Check(Near(AmplitudeToDecibels(1.f), 0) && AmplitudeToDecibels(0.f) == -120.f, "amplitude to decibels");
    Check(CountdownUrgency(120) == ClockUrgency::Normal && CountdownUrgency(30) == ClockUrgency::Warning &&
              CountdownUrgency(5) == ClockUrgency::Critical,
          "countdown urgency thresholds");
    {
        PeakHold hold;
        const float loud[] = {-6.f}, quiet[] = {-40.f};
        hold.Update(loud, .016f);
        hold.Update(quiet, .5f);
        Check(Near(hold.peak[0], -6) && hold.level[0] == -40.f, "peak is held");
        hold.Update(quiet, 1.f);
        hold.Update(quiet, 1.f);
        Check(hold.peak[0] < -6.f, "peak falls after the hold time");
    }

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1400, 2600};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 256> nodes{};
    std::array<accessibility::ActionRequest, 8> storage{};
    accessibility::ActionQueue actions{storage};
    accessibility::AccessibilityFrame semantics{nodes, &actions};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    float fader = 0, knob = .25f, x = .5f, y = .5f;
    bool armed = false;
    int holdFires = 0, armChanges = 0;
    float holdSeconds = .2f;
    ValueEdit faderEdit, knobEdit, xyEdit, faderAll, knobAll;
    PadEvent padEvent;
    BindingRequest bindingRequest = BindingRequest::None;
    std::array<PadView, 4> pads{};
    pads[0].label = "Alpha";
    pads[1].label = "Beta";
    pads[2].state = PadState::Empty;
    pads[3].label = "Gamma";
    pads[3].state = PadState::Error;
    auto merge = [](ValueEdit& all, const ValueEdit& e) {
        all.started |= e.started;
        all.changed |= e.changed;
        all.finished |= e.finished;
        all.cancelled |= e.cancelled;
        if (e.started)
            all.original = e.original;
        all.value = e.value;
    };
    auto frame = [&](bool reduced = false) {
        theme.motion.reducedMotion = reduced;
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        FaderOptions fo;
        fo.defaultValue = .8f;
        fo.step = 0;
        faderEdit = Fader("Level", &fader, fo, options);
        merge(faderAll, faderEdit);
        ImGui::SameLine();
        KnobOptions ko;
        knobEdit = Knob("Gain", &knob, ko, options);
        merge(knobAll, knobEdit);
        ImGui::SameLine();
        xyEdit = XYPad("Pos", &x, &y, {}, options);
        PadGridOptions go;
        go.columns = 2;
        padEvent = PadGrid("pads", pads, go, options);
        const float levels[2] = {-10.f, -3.f}, peaks[2] = {-6.f, -2.f};
        MeterBar("Out", levels, peaks, {}, options);
        TimecodeView tc;
        tc.label = "Program";
        tc.time = editor::TicksPerSecond * 3661;
        tc.rate = {30, 1};
        TimecodeDisplay("tc", tc, options);
        BindingInfo info{BindingKind::Midi, "CC 7", "0-100%", false};
        bindingRequest = BindingBadge("b", info, options);
        HoldButtonOptions hb;
        hb.seconds = holdSeconds;
        if (HoldButton("Stop all", hb, options))
            ++holdFires;
        if (ArmSwitch("Arm", &armed, options))
            ++armChanges;
        StatusLed("live", LedState::Live, "ON AIR", options);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) {
        return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f};
    };
    // Releases the button and idles long enough (0.5 s) that the next press is not a double-click.
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };

    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "semantic tree is valid");
    const auto* fnode = find("Level");
    const auto* knode = find("Gain");
    Check(fnode && fnode->role == accessibility::SemanticRole::Slider && fnode->maximumValue == 1, "fader is announced as a slider with its range");
    Check(knode && knode->role == accessibility::SemanticRole::Slider, "knob is announced as a slider");
    const auto* meter = find("Out");
    Check(meter && meter->role == accessibility::SemanticRole::Meter && Near(meter->numericValue, -3), "meter announces the loudest channel");
    Check(find("Program") && find("Program")->value == "01:01:01:00", "timecode announces its value");
    Check(find("Alpha") && find("Empty pad 3") && find("Beta"), "pads are named");

    // Fader: clicking the rail jumps, dragging moves relative to the anchor, release finishes once.
    if (fnode) {
        const ImVec2 c = centre(*fnode);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        faderAll = {};
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(faderAll.started && faderAll.changed && fader > .25f && fader < .75f, "rail click starts the gesture and jumps");
        const float jumped = fader;
        for (int i = 1; i <= 6; ++i) {
            io.AddMousePosEvent(c.x, c.y - 6.f * static_cast<float>(i));
            frame();
        }
        Check(fader > jumped + .05f, "dragging up raises the value");
        release();
        Check(faderAll.finished && !faderAll.cancelled, "release finishes the gesture");
        Check(Near(faderAll.value, fader), "edit carries the final value");

        // Escape restores the value from before the gesture.
        const float before = fader;
        faderAll = {};
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMousePosEvent(c.x, c.y - 40.f);
        frame();
        Check(Near(faderAll.original, before), "gesture remembers the value it started from");
        io.AddKeyEvent(ImGuiKey_Escape, true);
        frame();
        io.AddKeyEvent(ImGuiKey_Escape, false);
        Check(faderAll.cancelled && Near(fader, before), "escape cancels and restores the original value");
        release();
        Check(!faderAll.finished, "a cancelled gesture does not finish");

        // Double-click resets to the default.
        fader = .1f;
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(Near(fader, .8f), "double-click restores the default value");
        release();

        // Accessibility increments move the value by the step.
        const float before2 = fader;
        accessibility::ActionRequest increment;
        increment.id = fnode->id;
        increment.action = accessibility::SemanticAction::Increment;
        actions.Push(increment);
        frame();
        Check(fader > before2 && faderEdit.started && faderEdit.finished, "accessibility increment is a complete edit");
    } else
        Check(false, "fader node");

    // Knob: vertical drag.
    if (knode) {
        const ImVec2 c = centre(*knode);
        const float start = knob;
        io.AddMousePosEvent(c.x, c.y - 10);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        for (int i = 1; i <= 8; ++i) {
            io.AddMousePosEvent(c.x, c.y - 10 - 5.f * static_cast<float>(i));
            frame();
        }
        Check(knob > start + .1f, "dragging a knob upwards raises it");
        release();
        Check(knobAll.started && knobAll.finished, "knob gesture is reported");
    }

    // Pads report press and release.
    if (const auto* pad = find("Beta")) {
        const ImVec2 c = centre(*pad);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(padEvent.index == 1 && padEvent.pressed, "pad press is reported with its index");
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(padEvent.index == 1 && padEvent.released, "pad release is reported");
    } else
        Check(false, "pad node");

    // Hold button: nothing before the hold time, one event when it completes, immediate on the accessibility Press.
    if (const auto* hold = find("Stop all")) {
        const ImVec2 c = centre(*hold);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        for (int i = 0; i < 6; ++i)
            frame();
        Check(holdFires == 0, "a short press does not confirm");
        for (int i = 0; i < 12; ++i)
            frame();
        Check(holdFires == 1, "holding confirms exactly once");
        release();
        accessibility::ActionRequest press;
        press.id = hold->id;
        press.action = accessibility::SemanticAction::Press;
        actions.Push(press);
        frame();
        Check(holdFires == 2, "accessibility press confirms immediately");
    } else
        Check(false, "hold button node");

    // Arm switch.
    if (const auto* arm = find("Arm")) {
        const ImVec2 c = centre(*arm);
        io.AddMousePosEvent(c.x - 6, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        frame();
        Check(armed && armChanges == 1, "clicking the arm switch arms it");
    } else
        Check(false, "arm node");

    // Binding badge: clicking the body asks for Learn.
    if (const auto* badge = find("MIDI CC 7  0-100%")) {
        const ImVec2 c = centre(*badge);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        Check(bindingRequest == BindingRequest::ToggleLearn, "binding badge toggles learn");
        release();
    } else
        Check(false, "binding badge node");

    frame(true);
    Check(semantics.Tree().Validate(), "semantic tree stays valid with reduced motion");
    ImGui::DestroyContext();
    std::printf("Show controls failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
