#pragma once
#include <imkit/show/common.h>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

// No-code rules: "When ... if ... then ... otherwise ...". The builder draws a rule and reports edits; the host (or the
// optional RuleDocument below) applies them. EvaluateRule, FormatRuleSentence and ValidateRule are pure functions, so a
// rule can be tested, previewed and explained without running anything.
namespace imkit::show {
using StableId = accessibility::StableId;

enum class VariableType { Bool, Number, Text, Choice, Time };
// One value. Which field matters follows the variable type: Bool uses `flag`, Number and Time (seconds since
// midnight) use `number`, Choice uses `choice`, Text uses `text`. `text` is borrowed.
struct RuleValue {
    double number = 0;
    bool flag = false;
    int choice = 0;
    const char* text = "";
};
struct VariableDef {
    StableId id = 0;
    const char* name = "";      // "Lobby.occupancy".
    VariableType type = VariableType::Number;
    const char* group = "";     // Picker heading, for example "Sensors".
    const char* unit = "";
    double minimum = 0, maximum = 100;
    std::span<const char* const> choices{};
};
// Text: Equals and NotEquals are exact; Contains and StartsWith ignore ASCII case.
enum class Compare { Equals, NotEquals, Greater, GreaterOrEqual, Less, LessOrEqual, Between, Contains, StartsWith, IsTrue, IsFalse };
// Operators that make sense for a variable type, in menu order.
std::span<const Compare> OperatorsFor(VariableType type);
const char* CompareWords(Compare op); // "is greater than".

enum class NodeKind { Group, Compare };
enum class GroupMode { All, Any, None };
struct ConditionNode {
    StableId id = 0;
    StableId parent = 0;        // Zero for the root group.
    NodeKind kind = NodeKind::Compare;
    GroupMode mode = GroupMode::All;
    StableId variable = 0;
    Compare op = Compare::Equals;
    RuleValue a{}, b{};             // `b` is the upper bound of Between.
    bool enabled = true;
};
enum class TriggerKind { Time, Schedule, Osc, Midi, Variable, Timecode, Startup, Manual };
struct TriggerNode {
    StableId id = 0;
    TriggerKind kind = TriggerKind::Time;
    const char* text = "";      // Host-formatted description: "Every weekday at 18:00".
};
struct TriggerOption {
    TriggerKind kind = TriggerKind::Time;
    const char* name = "";
    const char* defaultText = "";
};
struct ParamDef {
    const char* name = "";
    VariableType type = VariableType::Number;
    double minimum = 0, maximum = 100, defaultNumber = 0;
    const char* unit = "";
    std::span<const char* const> choices{};
};
struct ActionTemplate {
    StableId id = 0;
    const char* name = "";      // "Play playlist".
    const char* group = "";
    std::span<const ParamDef> params{};
};
struct ActionNode {
    StableId id = 0;
    StableId type = 0;          // ActionTemplate id.
    float delaySeconds = 0;
    bool parallel = false;      // Runs together with the previous action.
    bool enabled = true;
    std::span<const RuleValue> values{}; // One per ActionTemplate param.
};
struct RuleView {
    const char* name = "";
    bool enabled = true;
    bool hasElse = false;
    std::span<const TriggerNode> triggers{};
    std::span<const ConditionNode> conditions{};
    std::span<const ActionNode> thenActions{};
    std::span<const ActionNode> elseActions{};
};
struct RuleContext {
    std::span<const VariableDef> variables{};
    std::span<const TriggerOption> triggerOptions{};
    std::span<const ActionTemplate> actionTemplates{};
};
const VariableDef* FindVariable(const RuleContext& context, StableId id);

// ---- Edits --------------------------------------------------------------------------------------------------
struct RuleEdit {
    enum class Kind {
        None, SetEnabled, SetElse, AddTrigger, RemoveTrigger, AddCondition, AddGroup, RemoveCondition, SetGroupMode,
        SetVariable, SetOperator, SetOperandA, SetOperandB, SetConditionEnabled, AddAction, RemoveAction, MoveAction,
        SetActionValue, SetActionDelay, SetActionParallel, SetActionEnabled
    } kind = Kind::None;
    StableId node = 0;          // Condition, trigger or action the edit targets.
    StableId parent = 0;        // For additions: the group a condition goes into.
    StableId item = 0;          // Variable id, action template id.
    bool elseBranch = false;    // Action edits: the "otherwise" list.
    int index = 0;              // Param index, trigger kind or move destination.
    Compare op = Compare::Equals;
    GroupMode mode = GroupMode::All;
    bool flag = false;
    float number = 0;           // Delay seconds.
    RuleValue value{};              // `value.text` is valid only until the end of the frame.
};
// Fixed-capacity edit list for one frame. Overflow is reported, never silently dropped.
class RuleEditBuffer {
  public:
    explicit RuleEditBuffer(std::span<RuleEdit> storage) : storage_(storage) {}
    bool Push(const RuleEdit& edit) {
        if (count_ == storage_.size()) {
            overflow_ = true;
            return false;
        }
        storage_[count_++] = edit;
        return true;
    }
    std::span<const RuleEdit> Edits() const { return storage_.first(count_); }
    bool Overflow() const { return overflow_; }
    void Clear() {
        count_ = 0;
        overflow_ = false;
    }

  private:
    std::span<RuleEdit> storage_;
    std::size_t count_ = 0;
    bool overflow_ = false;
};

// ---- Pure functions -----------------------------------------------------------------------------------------
enum class Truth { False, True, Unknown, Skipped };
struct ConditionTrace {
    StableId node = 0;
    Truth result = Truth::Unknown;
};
struct RuleOutcome {
    bool fires = false;         // The conditions hold, so the Then actions apply (otherwise the Else actions, if any).
    Truth root = Truth::True;
    bool missingVariable = false; // At least one comparison could not be evaluated and counted as false.
    std::size_t traced = 0;
};
// The host supplies current values. Return false when the variable has no value (it then counts as unknown).
struct VariableLookup {
    void* user = nullptr;
    bool (*read)(void* user, StableId variable, RuleValue& value) = nullptr;
};
// Empty All and None groups are true; an empty Any group is false. Disabled nodes are skipped. `trace`, when given,
// receives one entry per evaluated node (up to its capacity).
RuleOutcome EvaluateRule(const RuleView& rule, const RuleContext& context, VariableLookup lookup,
                         std::span<ConditionTrace> trace = {});
// One sentence such as "When every weekday at 18:00, if all of: Lobby.occupancy is greater than 0 and Screen 2 is
// online, then Play playlist Evening, otherwise Play playlist Standby." Returns the length written (always
// terminated; truncated with ".." when the buffer is short).
std::size_t FormatRuleSentence(const RuleView& rule, const RuleContext& context, std::span<char> out);
enum class RuleIssueKind { UnknownVariable, BadOperator, EmptyRange, EmptyText, NoActions, TooDeep, UnknownAction, NoTrigger };
struct RuleIssue {
    StableId node = 0;
    RuleIssueKind kind = RuleIssueKind::NoActions;
};
std::size_t ValidateRule(const RuleView& rule, const RuleContext& context, std::span<RuleIssue> issues);
const char* RuleIssueText(RuleIssueKind kind);

// ---- Builder ------------------------------------------------------------------------------------------------
struct RuleBuilderOptions {
    ImVec2 size{0, 0};          // Zero fills the width and grows with the content.
    bool showSentence = true;   // Plain-language summary under the builder.
    std::span<const RuleIssue> issues{}; // Rows with an issue get a warning mark.
};
// Draws the rule and records edits. Popups (variable, operator, action and trigger pickers) are part of the control.
// `scratch` holds the text being typed in a text operand and must outlive the frame.
struct RuleBuilderState {
    StableId editing = 0;       // Condition or action whose text operand is being typed.
    int editingSide = 0;
    char text[128]{};
    char search[64]{};          // Filter of the open variable or action picker.
};
void RuleBuilder(const char* id, const RuleView& rule, const RuleContext& context, RuleBuilderState& state, RuleEditBuffer& edits,
                 RuleBuilderOptions layout = {}, ComponentOptions options = {});

// Live variable table. Each row can be dragged onto a rule: the drag payload type is "IMKIT_VARIABLE" and carries
// the variable's StableId. `current` is read through `lookup`.
void VariableTable(const char* id, const RuleContext& context, VariableLookup lookup, ImVec2 size = {0, 0}, ComponentOptions options = {});

// Dry-run view: each evaluated condition with a shape-coded true/false mark and the overall verdict.
void RuleTrace(const char* id, const RuleView& rule, const RuleContext& context, std::span<const ConditionTrace> trace,
               const RuleOutcome& outcome, ComponentOptions options = {});

// ---- Optional document ---------------------------------------------------------------------------------------
// A ready-made owner for one rule so a host without its own model can still use the builder. It stores text in
// std::string, applies RuleEdit values and exposes a RuleView that borrows its storage until the next Apply or Refresh.
class RuleDocument {
  public:
    RuleDocument();
    // The view borrows this object's storage, so copies and moves rebuild their own view.
    RuleDocument(const RuleDocument& other);
    RuleDocument& operator=(const RuleDocument& other);
    RuleDocument(RuleDocument&& other) noexcept;
    RuleDocument& operator=(RuleDocument&& other) noexcept;
    void Apply(const RuleEdit& edit, const RuleContext& context);
    void ApplyAll(std::span<const RuleEdit> edits, const RuleContext& context);
    const RuleView& View() const { return view_; }
    // Convenience builders for setting up an initial rule.
    StableId AddTrigger(TriggerKind kind, std::string text);
    StableId AddGroup(StableId parent, GroupMode mode);
    StableId AddCompare(StableId parent, StableId variable, Compare op, RuleValue a, RuleValue b = {});
    StableId AddAction(StableId type, std::vector<RuleValue> values, bool elseBranch = false);
    void SetName(std::string name);
    void Refresh();

  private:
    struct Cond {
        ConditionNode node;
        std::string textA, textB;
    };
    struct Act {
        ActionNode node;
        std::vector<RuleValue> values;
        std::vector<std::string> texts;
        bool elseBranch = false;
    };
    struct Trig {
        TriggerNode node;
        std::string text;
    };
    StableId NextId() { return ++nextId_; }
    void RemoveCondition(StableId id);
    std::string name_;
    bool enabled_ = true, hasElse_ = false;
    StableId nextId_ = 0;
    std::vector<Trig> triggers_;
    std::vector<Cond> conditions_;
    std::vector<Act> actions_;
    std::vector<TriggerNode> triggerViews_;
    std::vector<ConditionNode> conditionViews_;
    std::vector<ActionNode> thenViews_, elseViews_;
    RuleView view_{};
};
} // namespace imkit::show
