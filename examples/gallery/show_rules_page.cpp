#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

namespace {
enum : StableId { Occupancy = 1, Lux, Screen, Mode, Clock, Emergency };
enum : StableId { PlayPlaylist = 100, SetOpacity, SendOsc, Blackout };
const char* ScreenChoices[] = {"offline", "online"};
const char* ModeChoices[] = {"day", "evening", "night"};
const char* PlaylistChoices[] = {"Evening", "Standby", "Morning", "Event"};
const char* LayerChoices[] = {"Layer 1", "Layer 2", "Layer 3", "Layer 4"};

const VariableDef Variables[] = {
    {Occupancy, "Lobby.occupancy", VariableType::Number, "Sensors", "people", 0, 500, {}},
    {Lux, "Sensor.lux", VariableType::Number, "Sensors", "lx", 0, 2000, {}},
    {Screen, "Screen 2", VariableType::Choice, "Outputs", "", 0, 1, ScreenChoices},
    {Mode, "Mode", VariableType::Choice, "Show", "", 0, 2, ModeChoices},
    {Clock, "Time of day", VariableType::Time, "Clock", "", 0, 86400, {}},
    {Emergency, "Emergency stop", VariableType::Bool, "Show", "", 0, 1, {}},
};
const ParamDef PlayParams[] = {{"playlist", VariableType::Choice, 0, 3, 0, "", PlaylistChoices}};
const ParamDef OpacityParams[] = {{"layer", VariableType::Choice, 0, 3, 0, "", LayerChoices}, {"opacity", VariableType::Number, 0, 100, 100, "%", {}}};
const ParamDef OscParams[] = {{"address", VariableType::Text, 0, 0, 0, "", {}}};
const ActionTemplate Templates[] = {
    {PlayPlaylist, "Play playlist", "Playback", PlayParams},
    {Blackout, "Blackout", "Playback", {}},
    {SetOpacity, "Set layer opacity", "Layers", OpacityParams},
    {SendOsc, "Send OSC message", "Control", OscParams},
};
const TriggerOption Triggers[] = {
    {TriggerKind::Time, "At a time", "Every day at 18:00"},
    {TriggerKind::Schedule, "From a schedule", "Schedule: Evening"},
    {TriggerKind::Osc, "OSC message", "OSC /show/start"},
    {TriggerKind::Midi, "MIDI note", "MIDI note 60"},
    {TriggerKind::Variable, "When a value changes", "Mode changes"},
    {TriggerKind::Timecode, "At timecode", "Timecode 01:00:00:00"},
    {TriggerKind::Startup, "On startup", "On startup"},
    {TriggerKind::Manual, "Manually", "Manually"},
};
const RuleContext Context{Variables, Triggers, Templates};

RuleValue Number(double n) {
    RuleValue v;
    v.number = n;
    return v;
}
RuleValue Choice(int c) {
    RuleValue v;
    v.choice = c;
    return v;
}
} // namespace

void ShowPages::Rules(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    if (!rules.initialized) {
        rules.initialized = true;
        rules.doc.SetName("Evening mode");
        rules.doc.AddTrigger(TriggerKind::Time, "every weekday at 18:00");
        const StableId root = rules.doc.View().conditions.front().id;
        rules.doc.AddCompare(root, Occupancy, Compare::Greater, Number(0));
        rules.doc.AddCompare(root, Screen, Compare::Equals, Choice(1));
        const StableId any = rules.doc.AddGroup(root, GroupMode::Any);
        rules.doc.AddCompare(any, Lux, Compare::Less, Number(40));
        rules.doc.AddCompare(any, Mode, Compare::Equals, Choice(2));
        rules.doc.AddAction(PlayPlaylist, {Choice(0)});
        rules.doc.AddAction(SetOpacity, {Choice(1), Number(80)});
        rules.doc.AddAction(PlayPlaylist, {Choice(1)}, true);
    }

    struct Live {
        RulesSim& sim;
        static bool Read(void* user, StableId id, RuleValue& out) {
            const RulesSim& s = *static_cast<RulesSim*>(user);
            switch (id) {
            case Occupancy: out = Number(s.occupancy); return true;
            case Lux: out = Number(s.lux); return true;
            case Screen: out = Choice(s.screenOnline ? 1 : 0); return true;
            case Mode: out = Choice(s.mode); return true;
            case Clock: out = Number(s.hour * 3600.0); return true;
            case Emergency: out = RuleValue{}; out.flag = s.emergency; return true;
            default: return false;
            }
        }
    };
    const VariableLookup lookup{&rules, &Live::Read};

    if (BeginGrid(host, "show-rules-grid", 2)) {
        const bool s1 = BeginSection(host, "Rule builder", "Pick a variable, a comparison and a value. Drag the grip to reorder actions.");
        if (s1) {
            std::array<RuleIssue, 8> issues{};
            const std::size_t count = ValidateRule(rules.doc.View(), Context, issues);
            RuleBuilderOptions layout;
            layout.issues = std::span<const RuleIssue>(issues.data(), std::min<std::size_t>(count, issues.size()));
            std::array<RuleEdit, 32> storage{};
            RuleEditBuffer edits(storage);
            RuleBuilder("rule", rules.doc.View(), Context, rules.state, edits, layout, o);
            Record(host, "show-rule");
            rules.doc.ApplyAll(edits.Edits(), Context);
            rules.edits += static_cast<int>(edits.Edits().size());
        }
        EndSection(s1);

        const bool s2 = BeginSection(host, "Test with live values", "Move the values and watch each condition. Nothing runs; this is a dry run.");
        if (s2) {
            SliderFloat("Lobby.occupancy", &rules.occupancy, 0, 100, "%.0f");
            SliderFloat("Sensor.lux", &rules.lux, 0, 400, "%.0f");
            Checkbox("Screen 2 is online", &rules.screenOnline);
            const char* modes[] = {"day", "evening", "night"};
            Combo("Mode", &rules.mode, modes, 3);
            SliderFloat("Hour of day", &rules.hour, 0, 24, "%.1f");
            std::array<ConditionTrace, 16> trace{};
            const RuleOutcome outcome = EvaluateRule(rules.doc.View(), Context, lookup, trace);
            ImGui::Spacing();
            RuleTrace("trace", rules.doc.View(), Context, std::span<const ConditionTrace>(trace.data(), outcome.traced), outcome, o);
            Record(host, "show-trace");
        }
        EndSection(s2);

        const bool s3 = BeginSection(host, "Variables", "Live values the rule can use. Names come from the host.");
        if (s3) {
            VariableTable("variables", Context, lookup, {0, ImGui::GetFrameHeight() * 1.5f * 7.f}, o);
            Record(host, "show-variables");
            ImGui::TextDisabled("Edits applied: %d", rules.edits);
        }
        EndSection(s3);
        EndGrid();
    }
}
} // namespace imkit::gallery
