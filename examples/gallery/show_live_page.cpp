#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

namespace {
struct CueSeed {
    const char* number;
    const char* name;
    const char* kind;
    CueContinue next;
    float preWait, duration;
    int depth;
    bool group;
};
constexpr CueSeed CueSeeds[] = {
    {"10", "House open", "Light", CueContinue::None, 0, 0, 0, false},
    {"20", "Intro loop", "Video", CueContinue::AutoContinue, 0, 12, 0, false},
    {"30", "Speaker walk-in", "Video", CueContinue::AutoFollow, 0, 8, 0, false},
    {"40", "Lower third", "Video", CueContinue::None, 2, 6, 0, false},
    {"50", "Keynote deck", "Slides", CueContinue::None, 0, 0, 0, true},
    {"50.1", "Title slide", "Slides", CueContinue::None, 0, 5, 1, false},
    {"50.2", "Demo clip", "Video", CueContinue::AutoFollow, 0, 9, 1, false},
    {"60", "Applause cue", "Audio", CueContinue::None, 0, 4, 0, false},
    {"70", "Outro film", "Video", CueContinue::None, 0, 14, 0, false},
};

void StartCue(ShowPages::CueSim& sim, std::size_t index) {
    if (index >= sim.views.size() || sim.views[index].state == CueState::Running)
        return;
    CueView& cue = sim.views[index];
    cue.state = cue.duration > 0 ? CueState::Running : CueState::Done;
    cue.elapsed = cue.duration > 0 ? 0.f : -1.f;
    if (cue.next == CueContinue::AutoContinue && index + 1 < sim.views.size())
        StartCue(sim, index + 1);
}
StableId NextStandby(const ShowPages::CueSim& sim) {
    for (const CueView& cue : sim.views)
        if (cue.state == CueState::Idle || cue.state == CueState::Standby)
            return cue.id;
    return 0;
}
} // namespace

void ShowPages::Live(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    const float dt = ImGui::GetIO().DeltaTime;
    const double now = ImGui::GetTime();
    const float u = ImGui::GetFrameHeight();

    if (cue.views.empty()) {
        for (std::size_t i = 0; i < std::size(CueSeeds); ++i) {
            const CueSeed& seed = CueSeeds[i];
            CueView v;
            v.id = i + 1;
            v.number = seed.number;
            v.name = seed.name;
            v.kind = seed.kind;
            v.next = seed.next;
            v.preWait = seed.preWait;
            v.duration = seed.duration;
            v.depth = seed.depth;
            v.group = seed.group;
            cue.views.push_back(v);
        }
        cue.views[7].state = CueState::Broken;
        cue.views[7].problem = "Missing file: applause.wav";
        cue.state.standby = 1;
        const char* layerNames[4] = {"Background", "Content", "Lower thirds", "Overlay"};
        const char* cellNames[6] = {"Loop A", "Loop B", "Title", "Logo", "Film", "Credits"};
        for (std::size_t l = 0; l < launcher.layers.size(); ++l) {
            launcher.layers[l].name = layerNames[l];
            launcher.layers[l].bus = l % 2 == 0 ? 1 : 2;
            launcher.layers[l].opacity = &launcher.opacity[l];
            launcher.layers[l].blend = l == 3 ? "Add" : "Normal";
            for (std::size_t c = 0; c < 6; ++c) {
                PadView& pad = launcher.cells[l * 6 + c];
                pad.label = cellNames[(c + l) % 6];
                pad.state = (l + c) % 5 == 4 ? PadState::Empty : (l + c) % 4 == 0 ? PadState::Loaded : PadState::Idle;
            }
        }
        launcher.cells[1].state = PadState::Playing;
        launcher.cells[6 * 2 + 3].state = PadState::Error;
        launcher.cells[6 * 2 + 3].caption = "Missing";
        effects.slots = {{"Blur", false, 1.f, {.35f, .5f, 0}}, {"Colour grade", false, .8f, {.5f, .5f, .6f}}, {"Glitch", true, .5f, {.2f, .7f, .1f}}};
        mapping.rows = {{1, BindingKind::Midi, "CC 21 ch1", "Background / Opacity", "0-100%", true, false},
                        {2, BindingKind::Osc, "/layer2/opacity", "Content / Opacity", "0-1", true, false},
                        {3, BindingKind::Key, "F5", "Cue GO", "", true, false},
                        {4, BindingKind::Midi, "CC 21 ch1", "Overlay / Opacity", "0-100%", true, true}};
        mapping.state.selected = 1;
        health.fps[0].fill(60.f);
        health.fps[1].fill(59.f);
        health.fps[2].fill(30.f);
    }

    // ---- Simulation -----------------------------------------------------------------------------------------
    if (!cue.paused)
        for (std::size_t i = 0; i < cue.views.size(); ++i) {
            CueView& v = cue.views[i];
            if (v.state != CueState::Running)
                continue;
            v.elapsed += dt;
            if (v.elapsed >= v.duration) {
                v.state = CueState::Done;
                v.elapsed = -1;
                if (v.next == CueContinue::AutoFollow)
                    StartCue(cue, i + 1);
            }
        }
    for (CueView& v : cue.views)
        if (v.state == CueState::Standby)
            v.state = CueState::Idle;
    cue.state.standby = NextStandby(cue);
    for (CueView& v : cue.views)
        if (v.id == cue.state.standby && v.state == CueState::Idle)
            v.state = CueState::Standby;
    for (std::size_t i = 0; i < launcher.cells.size(); ++i)
        if (launcher.cells[i].state == PadState::Playing)
            launcher.cells[i].progress = std::fmod(launcher.cells[i].progress < 0 ? 0.f : launcher.cells[i].progress + dt * .12f, 1.f);
    for (std::size_t i = 0; i < mixer.gain.size(); ++i) {
        const float t = static_cast<float>(now) * (1.5f + static_cast<float>(i) * .4f);
        const float base = mixer.mute[i] ? -120.f : mixer.gain[i] - 10.f;
        const float in[2] = {base + 9.f * std::sin(t), base + 8.f * std::sin(t * 1.3f + 1.f)};
        mixer.hold[i].Update(in, dt);
        mixer.level[i] = {mixer.hold[i].level[0], mixer.hold[i].level[1]};
        mixer.peak[i] = {mixer.hold[i].peak[0], mixer.hold[i].peak[1]};
    }
    {
        const float in[2] = {mixer.state.masterDb - 8.f + 6.f * std::sin(static_cast<float>(now) * 1.1f),
                             mixer.state.masterDb - 9.f + 6.f * std::sin(static_cast<float>(now) * 1.7f)};
        mixer.masterHold.Update(in, dt);
        mixer.masterLevel = {mixer.masterHold.level[0], mixer.masterHold.level[1]};
        mixer.masterPeak = {mixer.masterHold.peak[0], mixer.masterHold.peak[1]};
    }
    if (mixer.running) {
        mixer.transition = std::min(1.f, mixer.transition + dt / std::max(.1f, mixer.duration));
        if (mixer.transition >= 1.f)
            mixer.running = false;
    }
    if (now >= health.nextSample) {
        health.nextSample = now + .25;
        for (std::size_t d = 0; d < health.fps.size(); ++d) {
            std::rotate(health.fps[d].begin(), health.fps[d].begin() + 1, health.fps[d].end());
            const float base = d == 2 ? 30.f : 60.f;
            health.fps[d].back() = base - (d == 1 ? 2.5f + 2.f * std::sin(static_cast<float>(now) * 3.f) : .6f * std::sin(static_cast<float>(now) * 2.f));
        }
    }
    if (now >= mapping.nextMessage) {
        mapping.nextMessage = now + .45;
        std::array<char, 48> line{};
        const int n = mapping.counter++;
        BindingKind kind = n % 3 == 0 ? BindingKind::Osc : n % 3 == 1 ? BindingKind::Midi : BindingKind::Key;
        if (kind == BindingKind::Osc)
            std::snprintf(line.data(), line.size(), "/layer%d/opacity %.2f", n % 4 + 1, .5f + .4f * std::sin(static_cast<float>(n)));
        else if (kind == BindingKind::Midi)
            std::snprintf(line.data(), line.size(), "CC %d ch1 = %d", 20 + n % 3, 40 + (n * 7) % 80);
        else
            std::snprintf(line.data(), line.size(), "F%d", 5 + n % 4);
        if (!mapping.monitor.paused) {
            if (mapping.text.size() >= 200) {
                mapping.text.erase(mapping.text.begin(), mapping.text.begin() + 50);
                mapping.messages.erase(mapping.messages.begin(), mapping.messages.begin() + 50);
            }
            mapping.text.push_back(line);
            mapping.messages.push_back({now, kind, ""});
        }
        if (mapping.state.learning != 0 && kind == BindingKind::Midi) {
            for (MappingRow& row : mapping.rows)
                if (row.id == mapping.state.learning) {
                    static char learned[48];
                    std::snprintf(learned, sizeof learned, "%s", line.data());
                    row.source = learned;
                    row.kind = kind;
                }
            mapping.state.learning = 0;
        }
    }
    for (std::size_t i = 0; i < mapping.messages.size() && i < mapping.text.size(); ++i)
        mapping.messages[i].text = mapping.text[i].data();

    // ---- Page -----------------------------------------------------------------------------------------------
    if (BeginGrid(host, "show-live-grid", 2)) {
        const bool s1 = BeginSection(host, "Cue list and GO", "GO fires the standby cue. Auto-continue and auto-follow chain by themselves.");
        if (s1) {
            GoBarView view;
            for (const CueView& c : cue.views) {
                if (c.id == cue.state.standby)
                    view.next = c.name;
                if (c.state == CueState::Running) {
                    view.running = true;
                    view.remainingSeconds = std::max<double>(view.remainingSeconds, c.duration - c.elapsed);
                }
            }
            view.paused = cue.paused;
            const GoBarRequest r = GoBar("go", view, &armed, o);
            Record(host, "show-gobar");
            if (r.go && cue.state.standby != 0) {
                ++cue.goCount;
                for (std::size_t i = 0; i < cue.views.size(); ++i)
                    if (cue.views[i].id == cue.state.standby)
                        StartCue(cue, i);
            }
            if (r.pause)
                cue.paused = !cue.paused;
            if (r.stopAll || r.panic) {
                ++(r.panic ? cue.panicCount : cue.stopCount);
                for (CueView& c : cue.views)
                    if (c.state == CueState::Running) {
                        c.state = CueState::Idle;
                        c.elapsed = -1;
                    }
                cue.paused = false;
            }
            ImGui::Spacing();
            CueListOptions list;
            list.size = {0, ImGui::GetFrameHeight() * 1.4f * 8.f + 6.f};
            const CueRequest cr = CueList("cues", cue.views, cue.state, list, o);
            Record(host, "show-cuelist");
            if (cr.select)
                cue.state.selected = cr.select;
            if (cr.toggleGroup)
                for (CueView& c : cue.views)
                    if (c.id == cr.toggleGroup)
                        c.expanded = !c.expanded;
            if (cr.move && cr.move != cr.moveBefore) {
                auto from = std::find_if(cue.views.begin(), cue.views.end(), [&](const CueView& c) { return c.id == cr.move; });
                if (from != cue.views.end()) {
                    const CueView moved = *from;
                    cue.views.erase(from);
                    auto to = cr.moveBefore ? std::find_if(cue.views.begin(), cue.views.end(), [&](const CueView& c) { return c.id == cr.moveBefore; })
                                            : cue.views.end();
                    cue.views.insert(to, moved);
                }
            }
            ImGui::TextDisabled("GO: %d   Stop all: %d   Panic: %d%s", cue.goCount, cue.stopCount, cue.panicCount, cue.paused ? "   paused" : "");
        }
        EndSection(s1);

        const bool s2 = BeginSection(host, "Clip launcher", "Press a cell to play it on its layer. Column titles launch a whole scene.");
        if (s2) {
            const char* columns[6] = {"Scene 1", "Scene 2", "Scene 3", "Scene 4", "Scene 5", "Scene 6"};
            ClipLauncherView view;
            view.layers = launcher.layers;
            view.columns = columns;
            view.cells = launcher.cells;
            ClipLauncherOptions lo;
            lo.cellWidth = ImGui::GetFrameHeight() * 4.6f;
            const ClipLauncherRequest r = ClipLauncher("launcher", view, lo, o);
            Record(host, "show-launcher");
            auto play = [&](int layer, int column) {
                PadView& cell = launcher.cells[static_cast<std::size_t>(layer) * 6 + static_cast<std::size_t>(column)];
                if (cell.state == PadState::Empty || cell.state == PadState::Error)
                    return;
                for (int c = 0; c < 6; ++c) {
                    PadView& other = launcher.cells[static_cast<std::size_t>(layer) * 6 + static_cast<std::size_t>(c)];
                    if (other.state == PadState::Playing) {
                        other.state = PadState::Loaded;
                        other.progress = -1;
                    }
                }
                cell.state = PadState::Playing;
                cell.progress = 0;
            };
            if (r.pressed)
                play(r.layer, r.column);
            if (r.secondary && r.layer >= 0)
                launcher.cells[static_cast<std::size_t>(r.layer) * 6 + static_cast<std::size_t>(r.column)].state = PadState::Idle;
            if (r.triggerColumn >= 0) {
                ++launcher.triggers;
                for (int l = 0; l < 4; ++l)
                    play(l, r.triggerColumn);
            }
            if (r.stopLayer >= 0)
                for (int c = 0; c < 6; ++c) {
                    PadView& cell = launcher.cells[static_cast<std::size_t>(r.stopLayer) * 6 + static_cast<std::size_t>(c)];
                    if (cell.state == PadState::Playing) {
                        cell.state = PadState::Loaded;
                        cell.progress = -1;
                    }
                }
            if (r.selectLayer >= 0)
                for (int l = 0; l < 4; ++l)
                    launcher.layers[static_cast<std::size_t>(l)].selected = l == r.selectLayer;
            if (r.toggledLayer >= 0) {
                LayerView& lv = launcher.layers[static_cast<std::size_t>(r.toggledLayer)];
                if (r.toggle == LayerToggle::Mute)
                    lv.mute = !lv.mute;
                if (r.toggle == LayerToggle::Solo)
                    lv.solo = !lv.solo;
                if (r.toggle == LayerToggle::Bypass)
                    lv.bypass = !lv.bypass;
            }
            ImGui::TextDisabled("Scene launches: %d", launcher.triggers);
        }
        EndSection(s2);

        const bool s3 = BeginSection(host, "Mixer and transition", "Console-style faders with peak-hold meters, A/B buses and a crossfader.");
        if (s3) {
            const char* names[4] = {"Layer 1", "Layer 2", "Layer 3", "Layer 4"};
            static std::array<bool, 4> muteStore{}, soloStore{};
            std::array<MixerStrip, 4> strips{};
            for (std::size_t i = 0; i < strips.size(); ++i) {
                muteStore[i] = mixer.mute[i];
                soloStore[i] = mixer.solo[i];
                strips[i].name = names[i];
                strips[i].gainDb = &mixer.gain[i];
                strips[i].mute = &muteStore[i];
                strips[i].solo = &soloStore[i];
                strips[i].bus = &mixer.bus[i];
                strips[i].levelsDb = mixer.level[i];
                strips[i].peaksDb = mixer.peak[i];
            }
            MixerOptions mo;
            mo.masterLevelsDb = mixer.masterLevel;
            mo.masterPeaksDb = mixer.masterPeak;
            Mixer("mixer", strips, mixer.state, mo, o);
            Record(host, "show-mixer");
            for (std::size_t i = 0; i < strips.size(); ++i) {
                mixer.mute[i] = muteStore[i];
                mixer.solo[i] = soloStore[i];
            }
            ImGui::Spacing();
            TransitionView tv;
            tv.position = &mixer.transition;
            tv.durationSeconds = &mixer.duration;
            tv.running = mixer.running;
            const TransitionRequest tr = TransitionBar("transition", tv, o);
            Record(host, "show-transition");
            if (tr.cut)
                mixer.transition = mixer.transition > .5f ? 0.f : 1.f;
            if (tr.autoTransition) {
                mixer.running = true;
                mixer.transition = 0;
            }
        }
        EndSection(s3);

        const bool s4 = BeginSection(host, "Effects and modulator", "Drag the grip to reorder. The modulator preview shows the shape that will drive a parameter.");
        if (s4) {
            std::vector<std::array<EffectParam, 3>> params(effects.slots.size());
            std::vector<EffectSlot> views(effects.slots.size());
            const char* paramNames[3] = {"Amount", "Size", "Softness"};
            for (std::size_t i = 0; i < effects.slots.size(); ++i) {
                for (std::size_t p = 0; p < 3; ++p) {
                    params[i][p].name = paramNames[p];
                    params[i][p].value = &effects.slots[i].values[p];
                    params[i][p].defaultValue = .5f;
                }
                views[i].name = effects.slots[i].name.c_str();
                views[i].bypass = &effects.slots[i].bypass;
                views[i].mix = &effects.slots[i].mix;
                views[i].params = params[i];
            }
            const EffectRackRequest r = EffectRack("rack", views, effects.state, o);
            Record(host, "show-rack");
            if (r.remove >= 0 && r.remove < static_cast<int>(effects.slots.size()))
                effects.slots.erase(effects.slots.begin() + r.remove);
            if (r.add) {
                EffectSim::Fx fx;
                fx.name = "Effect " + std::to_string(effects.slots.size() + 1);
                fx.values = {.5f, .5f, .5f};
                effects.slots.push_back(fx);
            }
            if (r.moveFrom >= 0 && r.moveTo >= 0 && r.moveFrom != r.moveTo && r.moveFrom < static_cast<int>(effects.slots.size()) &&
                r.moveTo < static_cast<int>(effects.slots.size())) {
                auto first = effects.slots.begin() + std::min(r.moveFrom, r.moveTo);
                auto middle = effects.slots.begin() + r.moveFrom;
                auto last = effects.slots.begin() + std::max(r.moveFrom, r.moveTo) + 1;
                if (r.moveFrom < r.moveTo)
                    std::rotate(first, middle + 1, last);
                else
                    std::rotate(first, middle, last);
            }
            ImGui::Spacing();
            ModulatorView mv;
            mv.shape = &effects.shape;
            mv.rate = &effects.rate;
            mv.phase = &effects.phase;
            mv.amount = &effects.amount;
            mv.offset = &effects.offset;
            mv.beatSync = &effects.sync;
            mv.playPhase = std::fmod(static_cast<float>(now) * effects.rate, 1.f);
            ModulatorEditor("modulator", mv, o);
            Record(host, "show-modulator");
        }
        EndSection(s4);

        const bool s5 = BeginSection(host, "Mappings and messages", "Learn waits for the next MIDI message. Double-click a message to assign it.");
        if (s5) {
            const MappingRequest r = MappingTable("mappings", mapping.rows, mapping.state, o);
            Record(host, "show-mappings");
            if (r.select)
                mapping.state.selected = r.select;
            if (r.learn)
                mapping.state.learning = mapping.state.learning == r.learn ? 0 : r.learn;
            if (r.toggleEnabled)
                for (MappingRow& row : mapping.rows)
                    if (row.id == r.toggleEnabled)
                        row.enabled = !row.enabled;
            if (r.remove)
                mapping.rows.erase(std::remove_if(mapping.rows.begin(), mapping.rows.end(), [&](const MappingRow& row) { return row.id == r.remove; }),
                                   mapping.rows.end());
            ImGui::Spacing();
            const MessageMonitorRequest m = MessageMonitor("monitor", mapping.messages, mapping.monitor, {0, ImGui::GetFrameHeight() * 6.f}, o);
            Record(host, "show-monitor");
            if (m.clear) {
                mapping.messages.clear();
                mapping.text.clear();
            }
        }
        EndSection(s5);

        const bool s6 = BeginSection(host, "Device health", "Frame rate history per output with a status light that never relies on colour alone.");
        if (s6) {
            std::array<DeviceView, 3> devices{};
            devices[0] = {"Output 1  (Main LED)", "Connected 3840x2160", LedState::On, health.fps[0].back(), 41.f, 63.f, 0, health.fps[0]};
            devices[1] = {"Output 2  (Side wall)", "Dropping frames", LedState::Warning, health.fps[1].back(), 87.f, 78.f, 12, health.fps[1]};
            devices[2] = {"Output 3  (Floor)", "No signal", LedState::Error, -1, -1, -1, -1, {}};
            DeviceHealth("devices", devices, o);
            Record(host, "show-devices");
        }
        EndSection(s6);
        EndGrid();
    }
    (void)u;
}
} // namespace imkit::gallery
