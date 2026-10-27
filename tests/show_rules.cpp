#include <imkit/imkit.h>
#include <imkit/show.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
} // namespace

using namespace imkit;
using namespace imkit::show;

namespace {
enum : StableId { Occupancy = 1, Lux, Screen, Mode, Clock, Emergency, Playlist };
enum : StableId { PlayPlaylist = 100, SetOpacity, Blackout };

const char* screenChoices[] = {"offline", "online"};
const char* modeChoices[] = {"day", "evening", "night"};
const char* playlistChoices[] = {"Evening", "Standby", "Morning"};

const VariableDef variables[] = {
    {Occupancy, "Lobby.occupancy", VariableType::Number, "Sensors", "people", 0, 500, {}},
    {Lux, "Sensor.lux", VariableType::Number, "Sensors", "lx", 0, 2000, {}},
    {Screen, "Screen 2", VariableType::Choice, "Outputs", "", 0, 1, screenChoices},
    {Mode, "Mode", VariableType::Choice, "Show", "", 0, 2, modeChoices},
    {Clock, "Time of day", VariableType::Time, "Clock", "", 0, 86400, {}},
    {Emergency, "Emergency stop", VariableType::Bool, "Show", "", 0, 1, {}},
    {Playlist, "Playlist", VariableType::Text, "Show", "", 0, 0, {}},
};
const ParamDef playParams[] = {{"playlist", VariableType::Choice, 0, 2, 0, "", playlistChoices}};
const ParamDef opacityParams[] = {{"opacity", VariableType::Number, 0, 100, 100, "%", {}}};
const ActionTemplate templates[] = {
    {PlayPlaylist, "Play playlist", "Playback", playParams},
    {SetOpacity, "Set opacity", "Layers", opacityParams},
    {Blackout, "Blackout", "Playback", {}},
};
const TriggerOption triggerOptions[] = {{TriggerKind::Time, "At a time", "Every day at 18:00"}, {TriggerKind::Manual, "Manually", "Manually"}};
const RuleContext context{variables, triggerOptions, templates};

struct Values {
    std::map<StableId, RuleValue> map;
    static bool Read(void* user, StableId id, RuleValue& out) {
        auto& m = static_cast<Values*>(user)->map;
        const auto it = m.find(id);
        if (it == m.end())
            return false;
        out = it->second;
        return true;
    }
    VariableLookup Lookup() { return {this, &Read}; }
};
RuleValue MakeNumber(double n) {
    RuleValue v;
    v.number = n;
    return v;
}
RuleValue MakeChoice(int c) {
    RuleValue v;
    v.choice = c;
    return v;
}
RuleValue MakeText(const char* t) {
    RuleValue v;
    v.text = t;
    return v;
}
} // namespace

int main() {
    // Rule: when ..., if (occupancy > 0 and screen online and (lux < 40 or mode is night)) then play Evening, else play Standby.
    RuleDocument doc;
    doc.SetName("Evening mode");
    doc.AddTrigger(TriggerKind::Time, "every weekday at 18:00");
    const StableId rootId = doc.View().conditions.front().id;
    const StableId occ = doc.AddCompare(rootId, Occupancy, Compare::Greater, MakeNumber(0));
    doc.AddCompare(rootId, Screen, Compare::Equals, MakeChoice(1));
    const StableId any = doc.AddGroup(rootId, GroupMode::Any);
    const StableId lux = doc.AddCompare(any, Lux, Compare::Less, MakeNumber(40));
    doc.AddCompare(any, Mode, Compare::Equals, MakeChoice(2));
    doc.AddAction(PlayPlaylist, {MakeChoice(0)});
    doc.AddAction(PlayPlaylist, {MakeChoice(1)}, true);

    Values values;
    values.map[Occupancy] = MakeNumber(3);
    values.map[Screen] = MakeChoice(1);
    values.map[Lux] = MakeNumber(25);
    values.map[Mode] = MakeChoice(0);
    std::array<ConditionTrace, 16> trace{};
    RuleOutcome out = EvaluateRule(doc.View(), context, values.Lookup(), trace);
    Check(out.fires && !out.missingVariable, "all conditions hold");
    Check(out.traced == 6, "every node is traced");
    values.map[Lux] = MakeNumber(80);
    Check(!EvaluateRule(doc.View(), context, values.Lookup()).fires, "the Any group needs one true child");
    values.map[Mode] = MakeChoice(2);
    Check(EvaluateRule(doc.View(), context, values.Lookup()).fires, "the second Any child can carry it");
    values.map[Occupancy] = MakeNumber(0);
    Check(!EvaluateRule(doc.View(), context, values.Lookup()).fires, "occupancy 0 fails the All group");

    // Disabled nodes are skipped and missing values count as false.
    doc.Apply({RuleEdit::Kind::SetConditionEnabled, occ, 0, 0, false, 0, Compare::Equals, GroupMode::All, false}, context);
    out = EvaluateRule(doc.View(), context, values.Lookup(), trace);
    Check(out.fires, "a disabled condition is ignored");
    Check(trace[0].result == Truth::Skipped || trace[1].result == Truth::Skipped || trace[2].result == Truth::Skipped, "skipped nodes are traced");
    values.map.erase(Screen);
    out = EvaluateRule(doc.View(), context, values.Lookup());
    Check(!out.fires && out.missingVariable, "a missing value is false and reported");
    values.map[Screen] = MakeChoice(1);

    // Operators by type.
    Check(OperatorsFor(VariableType::Bool).size() == 2 && OperatorsFor(VariableType::Text).size() == 4, "operators follow the type");
    {
        RuleDocument d;
        d.AddCompare(0, Clock, Compare::Between, MakeNumber(22 * 3600), MakeNumber(6 * 3600));
        d.AddCompare(0, Playlist, Compare::Contains, MakeText("eve"));
        d.AddCompare(0, Emergency, Compare::IsFalse, {});
        Values v;
        v.map[Clock] = MakeNumber(23.5 * 3600);
        v.map[Playlist] = MakeText("Evening loop");
        RuleValue off;
        v.map[Emergency] = off;
        Check(EvaluateRule(d.View(), context, v.Lookup()).fires, "time range wraps past midnight, text contains, bool is off");
        v.map[Clock] = MakeNumber(12 * 3600);
        Check(!EvaluateRule(d.View(), context, v.Lookup()).fires, "noon is outside the overnight range");
        RuleDocument none;
        const StableId g = none.AddGroup(0, GroupMode::None);
        none.AddCompare(g, Emergency, Compare::IsTrue, {});
        RuleValue on;
        on.flag = true;
        Values nv;
        nv.map[Emergency] = on;
        Check(EvaluateRule(none.View(), context, nv.Lookup()).fires == false, "None group fails when a child holds");
    }

    // Sentence.
    std::array<char, 512> sentence{};
    FormatRuleSentence(doc.View(), context, sentence);
    const std::string text = sentence.data();
    Check(text.find("When every weekday at 18:00") == 0, "sentence starts with the trigger");
    Check(text.find("Sensor.lux is less than 40 lx") != std::string::npos, "sentence names the comparison with its unit");
    Check(text.find("any of: ") != std::string::npos && text.find("Mode is night") != std::string::npos, "nested groups are spelled out");
    Check(text.find("then Play playlist Evening") != std::string::npos && text.find("otherwise Play playlist Standby") != std::string::npos,
          "then and else branches appear");
    std::array<char, 24> tiny{};
    const std::size_t n = FormatRuleSentence(doc.View(), context, tiny);
    Check(n == 23 && tiny[23] == '\0' && std::strstr(tiny.data(), "..") != nullptr, "short buffers are terminated and marked");

    // Validation.
    std::array<RuleIssue, 16> issues{};
    Check(ValidateRule(doc.View(), context, issues) == 0, "the sample rule is valid");
    {
        RuleDocument bad;
        bad.AddCompare(0, 999, Compare::Equals, {});
        bad.AddCompare(0, Occupancy, Compare::Between, MakeNumber(10), MakeNumber(5));
        bad.AddCompare(0, Playlist, Compare::Equals, MakeText(""));
        const std::size_t count = ValidateRule(bad.View(), context, issues);
        bool unknown = false, range = false, empty = false, noActions = false, noTrigger = false;
        for (std::size_t i = 0; i < count && i < issues.size(); ++i) {
            unknown |= issues[i].kind == RuleIssueKind::UnknownVariable;
            range |= issues[i].kind == RuleIssueKind::EmptyRange;
            empty |= issues[i].kind == RuleIssueKind::EmptyText;
            noActions |= issues[i].kind == RuleIssueKind::NoActions;
            noTrigger |= issues[i].kind == RuleIssueKind::NoTrigger;
        }
        Check(unknown && range && empty && noActions && noTrigger, "validation finds each kind of problem");
    }

    // Applying edits.
    {
        RuleDocument d;
        d.AddTrigger(TriggerKind::Manual, "Manually");
        RuleEdit add;
        add.kind = RuleEdit::Kind::AddCondition;
        add.parent = 0;
        add.item = Lux;
        d.Apply(add, context);
        Check(d.View().conditions.size() == 2 && d.View().conditions[1].op == Compare::Equals, "AddCondition puts a default comparison under the root");
        RuleEdit variable;
        variable.kind = RuleEdit::Kind::SetVariable;
        variable.node = d.View().conditions[1].id;
        variable.item = Emergency;
        d.Apply(variable, context);
        Check(d.View().conditions[1].op == Compare::IsTrue, "changing the variable resets an operator that no longer fits");
        RuleEdit operand;
        operand.kind = RuleEdit::Kind::SetOperandA;
        operand.node = d.View().conditions[1].id;
        operand.value = MakeText("typed");
        variable.item = Playlist;
        d.Apply(variable, context);
        d.Apply(operand, context);
        Check(std::string(d.View().conditions[1].a.text) == "typed", "text operands are copied into the document");
        RuleEdit group;
        group.kind = RuleEdit::Kind::AddGroup;
        d.Apply(group, context);
        RuleEdit remove;
        remove.kind = RuleEdit::Kind::RemoveCondition;
        remove.node = d.View().conditions.front().id;
        d.Apply(remove, context);
        Check(d.View().conditions.size() == 3, "the root group cannot be removed");
        remove.node = d.View().conditions[2].id;
        d.Apply(remove, context);
        Check(d.View().conditions.size() == 2, "removing a group removes it");

        RuleEdit action;
        action.kind = RuleEdit::Kind::AddAction;
        action.item = SetOpacity;
        d.Apply(action, context);
        action.item = Blackout;
        d.Apply(action, context);
        action.item = PlayPlaylist;
        action.elseBranch = true;
        d.Apply(action, context);
        Check(d.View().thenActions.size() == 2 && d.View().elseActions.size() == 1 && d.View().hasElse, "actions land in their branch");
        Check(d.View().thenActions[0].values.size() == 1 && d.View().thenActions[0].values[0].number == 100, "action values start from the template defaults");
        RuleEdit move;
        move.kind = RuleEdit::Kind::MoveAction;
        move.node = d.View().thenActions[1].id;
        move.index = 0;
        d.Apply(move, context);
        Check(d.View().thenActions[0].type == Blackout && d.View().thenActions[1].type == SetOpacity, "moving an action reorders its branch");
        RuleEdit noElse;
        noElse.kind = RuleEdit::Kind::SetElse;
        noElse.flag = false;
        d.Apply(noElse, context);
        Check(!d.View().hasElse && d.View().elseActions.empty() && d.View().thenActions.size() == 2, "turning off otherwise drops only its actions");
    }
    {
        std::array<RuleEdit, 2> storage{};
        RuleEditBuffer buffer(storage);
        RuleEdit e;
        e.kind = RuleEdit::Kind::SetEnabled;
        buffer.Push(e);
        buffer.Push(e);
        Check(!buffer.Overflow() && buffer.Edits().size() == 2, "the edit buffer holds its capacity");
        Check(!buffer.Push(e) && buffer.Overflow(), "overflow is reported");
    }

    // UI.
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1600, 2400};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 512> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};
    RuleBuilderState builderState;
    std::array<RuleEdit, 32> editStorage{};
    RuleEditBuffer edits(editStorage);
    std::vector<RuleEdit> seen;
    auto frame = [&] {
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        edits.Clear();
        std::array<RuleIssue, 8> iss{};
        RuleBuilderOptions bo;
        const std::size_t count = ValidateRule(doc.View(), context, iss);
        bo.issues = std::span<const RuleIssue>(iss.data(), std::min<std::size_t>(count, iss.size()));
        RuleBuilder("rule", doc.View(), context, builderState, edits, bo, options);
        VariableTable("vars", context, values.Lookup(), {600, 200}, options);
        RuleTrace("trace", doc.View(), context, std::span<const ConditionTrace>(trace.data(), 6), out, options);
        for (const RuleEdit& e : edits.Edits())
            seen.push_back(e);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto click = [&](const accessibility::SemanticNode& n) {
        const ImVec2 c{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f};
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "the rule builder publishes a valid semantic tree");
    Check(find("Enable condition Lobby.occupancy") && find("Remove condition") && find("+ condition") && find("+ action"), "builder controls are announced");
    if (const auto* dot = find("Enable condition Lobby.occupancy")) {
        seen.clear();
        click(*dot);
        bool found = false;
        for (const RuleEdit& e : seen)
            found |= e.kind == RuleEdit::Kind::SetConditionEnabled && e.node == occ;
        Check(found, "the enable dot edits the condition");
    }
    if (const auto* remove = find("Remove condition")) {
        seen.clear();
        click(*remove);
        bool found = false;
        for (const RuleEdit& e : seen)
            found |= e.kind == RuleEdit::Kind::RemoveCondition;
        Check(found, "the remove button edits the rule");
    }
    (void)lux;
    ImGui::DestroyContext();
    std::printf("Show rules failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
