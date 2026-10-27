#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

void ShowPages::Init() {
    if (initialized)
        return;
    const char* names[8] = {"Intro", "Loop A", "Loop B", "Title", "Logo", "Outro", "", "Broken"};
    const PadState states[8] = {PadState::Playing, PadState::Loaded, PadState::Queued, PadState::Idle,
                                PadState::Idle, PadState::Loaded, PadState::Empty, PadState::Error};
    for (std::size_t i = 0; i < pads.size(); ++i) {
        pads[i].label = names[i];
        pads[i].state = states[i];
    }
    pads[0].caption = "0:42";
    pads[1].caption = "Key 1";
    pads[7].caption = "Missing";
    initialized = true;
}

void ShowPages::Show(int page, GalleryState& host) {
    Init();
    if (page == 25)
        Controls(host);
    else if (page == 26)
        Live(host);
    else if (page == 27)
        Rules(host);
}

void ShowPages::Controls(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    const float dt = ImGui::GetIO().DeltaTime;
    const float time = static_cast<float>(ImGui::GetTime());
    auto remember = [&](const ValueEdit& e) {
        if (e)
            lastEdit = e;
    };

    if (BeginGrid(host, "show-controls-grid", 2)) {
        const bool section1 = BeginSection(host, "Faders and meters", "Drag the cap or click the rail. Shift = fine, double-click = reset, Ctrl+click = type."); if (section1) {
            const float ticks[] = {6, 0, -6, -12, -24, -48};
            FaderOptions db;
            db.scale = FaderScale::Decibel;
            db.minimum = -60;
            db.maximum = 6;
            db.defaultValue = 0;
            db.ticks = ticks;
            db.tickFormat = "%.0f";
            db.unit = "dB";
            db.binding = bound ? &binding : nullptr;
            remember(Fader("Layer 1", &faders[0], db, o));
            Record(host, "show-fader-0");
            ImGui::SameLine();
            FaderOptions linear;
            linear.format = "%.2f";
            linear.defaultValue = 1;
            const float lt[] = {0, .5f, 1};
            linear.ticks = lt;
            remember(Fader("Layer 2", &faders[1], linear, o));
            Record(host, "show-fader-1");
            ImGui::SameLine();
            remember(Fader("Master", &faders[2], db, o));
            Record(host, "show-fader-2");
            ImGui::SameLine(0, ImGui::GetFrameHeight());
            const float input[2] = {-14.f + 12.f * std::sin(time * 3.1f), -16.f + 14.f * std::sin(time * 2.3f + 1.f)};
            hold.Update(input, dt);
            const float scale[] = {0, -6, -12, -24, -48};
            MeterOptions meter;
            meter.scaleDb = scale;
            MeterBar("Output", std::span<const float>(hold.level.data(), 2), std::span<const float>(hold.peak.data(), 2), meter, o);
            Record(host, "show-meter");
            FaderOptions horizontal;
            horizontal.vertical = false;
            horizontal.bipolar = true;
            horizontal.defaultValue = .5f;
            horizontal.format = "A/B %.2f";
            horizontal.size = {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.6f};
            remember(Fader("Crossfade", &crossfade, horizontal, o));
            Record(host, "show-crossfader");
        }
        EndSection(section1);

        const bool section2 = BeginSection(host, "Knobs and assignment", "Vertical drag. The dot on Mix shows a MIDI assignment."); if (section2) {
            KnobOptions gain;
            gain.minimum = -24;
            gain.maximum = 24;
            gain.bipolar = true;
            gain.format = "%.1f";
            gain.unit = "dB";
            remember(Knob("Gain", &knobs[0], gain, o));
            Record(host, "show-knob-0");
            ImGui::SameLine();
            KnobOptions pan;
            pan.minimum = -1;
            pan.maximum = 1;
            pan.bipolar = true;
            remember(Knob("Pan", &knobs[1], pan, o));
            ImGui::SameLine();
            KnobOptions speed;
            speed.minimum = .1f;
            speed.maximum = 4;
            speed.defaultValue = 1;
            speed.format = "%.2f";
            speed.unit = "x";
            remember(Knob("Speed", &knobs[2], speed, o));
            ImGui::SameLine();
            KnobOptions hue;
            hue.maximum = 360;
            hue.step = 1;
            hue.format = "%.0f";
            hue.unit = "deg";
            remember(Knob("Hue", &knobs[3], hue, o));
            ImGui::SameLine();
            KnobOptions mix;
            mix.format = "%.2f";
            mix.binding = bound ? &binding : nullptr;
            remember(Knob("Mix", &knobs[4], mix, o));
            ImGui::Spacing();
            BindingInfo shown = binding;
            if (!bound)
                shown = {};
            switch (BindingBadge("binding", shown, o)) {
            case BindingRequest::ToggleLearn:
                if (!bound) {
                    bound = true;
                    binding.learning = true;
                } else
                    binding.learning = !binding.learning;
                break;
            case BindingRequest::Clear:
                bound = false;
                binding.learning = false;
                break;
            default: break;
            }
            Record(host, "show-binding");
        }
        EndSection(section2);

        const bool section3 = BeginSection(host, "XY pad and pads", "Click a pad to trigger it. Right-click to clear."); if (section3) {
            XYPadOptions xyo;
            xyo.size = {ImGui::GetFrameHeight() * 7.f, ImGui::GetFrameHeight() * 7.f};
            remember(XYPad("Position", &xy[0], &xy[1], xyo, o));
            Record(host, "show-xy");
            ImGui::SameLine();
            ImGui::BeginGroup();
            PadGridOptions grid;
            grid.columns = 4;
            const PadEvent e = PadGrid("pads", pads, grid, o);
            Record(host, "show-pads");
            if (e.pressed && e.index >= 0 && pads[static_cast<std::size_t>(e.index)].state != PadState::Empty &&
                pads[static_cast<std::size_t>(e.index)].state != PadState::Error) {
                for (auto& pad : pads)
                    if (pad.state == PadState::Playing)
                        pad.state = PadState::Loaded;
                pads[static_cast<std::size_t>(e.index)].state = PadState::Playing;
                padProgress[static_cast<std::size_t>(e.index)] = 0;
            }
            if (e.secondary && e.index >= 0)
                pads[static_cast<std::size_t>(e.index)].state = PadState::Idle;
            for (std::size_t i = 0; i < pads.size(); ++i) {
                if (pads[i].state == PadState::Playing) {
                    padProgress[i] = std::fmod(padProgress[i] + dt * .15f, 1.f);
                    pads[i].progress = padProgress[i];
                } else
                    pads[i].progress = -1;
            }
            ImGui::EndGroup();
        }
        EndSection(section3);

        const bool section4 = BeginSection(host, "Clocks and safety", "Fixed-width digits, shape-coded status and hold-to-confirm for dangerous actions."); if (section4) {
            TimecodeView clock;
            clock.label = "Program";
            clock.time = static_cast<editor::Tick>(static_cast<double>(time) * static_cast<double>(editor::TicksPerSecond));
            clock.rate = {30, 1};
            clock.running = true;
            TimecodeDisplay("program", clock, o);
            ImGui::SameLine(0, ImGui::GetFrameHeight() * 2);
            const double remaining = 90.0 - std::fmod(static_cast<double>(time), 95.0);
            TimecodeView countdown;
            countdown.label = "Next cue";
            countdown.time = static_cast<editor::Tick>(std::max(0.0, remaining) * static_cast<double>(editor::TicksPerSecond));
            countdown.countdown = true;
            countdown.running = true;
            countdown.urgency = CountdownUrgency(remaining);
            TimecodeDisplay("countdown", countdown, o);
            ImGui::Spacing();
            StatusLed("off", LedState::Off, "Off", o);
            ImGui::SameLine();
            StatusLed("on", LedState::On, "Ready", o);
            ImGui::SameLine();
            StatusLed("warn", LedState::Warning, "Hot", o);
            ImGui::SameLine();
            StatusLed("err", LedState::Error, "Lost", o);
            ImGui::SameLine();
            StatusLed("live", LedState::Live, "ON AIR", o);
            ImGui::Spacing();
            ArmSwitch("Arm GO", &armed, o);
            Record(host, "show-arm");
            ImGui::SameLine(0, ImGui::GetFrameHeight());
            ImGui::BeginDisabled(!armed);
            if (ActionButton("GO", ActionVariant::Primary, {ImGui::GetFrameHeight() * 3.f, 0}, o))
                ++goCount;
            Record(host, "show-go");
            ImGui::EndDisabled();
            ImGui::SameLine(0, ImGui::GetFrameHeight());
            if (HoldButton("Stop all", {}, o))
                ++panicCount;
            Record(host, "show-hold");
            ImGui::TextDisabled("GO: %d   Stop all: %d   last edit: %s%s%s%s", goCount, panicCount, lastEdit.started ? "started " : "",
                                lastEdit.changed ? "changed " : "", lastEdit.finished ? "finished " : "", lastEdit.cancelled ? "cancelled" : "");
        }
        EndSection(section4);
        EndGrid();
    }
}
} // namespace imkit::gallery
