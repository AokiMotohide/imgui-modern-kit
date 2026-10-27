#include <imkit/show/rules.h>
#include "internal.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace imkit::show {
using namespace impl;

// ---- Operators and values --------------------------------------------------------------------------------------
namespace {
constexpr std::array<Compare, 7> NumberOps = {Compare::Equals, Compare::NotEquals, Compare::Greater, Compare::GreaterOrEqual,
                                              Compare::Less, Compare::LessOrEqual, Compare::Between};
constexpr std::array<Compare, 4> TimeOps = {Compare::Greater, Compare::Less, Compare::Between, Compare::Equals};
constexpr std::array<Compare, 4> TextOps = {Compare::Equals, Compare::NotEquals, Compare::Contains, Compare::StartsWith};
constexpr std::array<Compare, 2> BoolOps = {Compare::IsTrue, Compare::IsFalse};
constexpr std::array<Compare, 2> ChoiceOps = {Compare::Equals, Compare::NotEquals};

bool Allows(VariableType type, Compare op) {
    const auto ops = OperatorsFor(type);
    return std::find(ops.begin(), ops.end(), op) != ops.end();
}
RuleValue DefaultValue(VariableType type, double minimum) {
    RuleValue v;
    v.number = type == VariableType::Number ? minimum : 0;
    v.flag = true;
    return v;
}
void Clock(char* out, std::size_t n, double seconds) {
    const int total = static_cast<int>(std::fmod(std::max(0.0, seconds), 86400.0));
    std::snprintf(out, n, "%02d:%02d", total / 3600, total / 60 % 60);
}
std::string ValueText(VariableType type, const RuleValue& v, const char* unit, std::span<const char* const> choices) {
    char text[64];
    switch (type) {
    case VariableType::Bool: return v.flag ? "on" : "off";
    case VariableType::Time: Clock(text, sizeof text, v.number); return text;
    case VariableType::Choice:
        if (v.choice >= 0 && v.choice < static_cast<int>(choices.size()))
            return choices[static_cast<std::size_t>(v.choice)];
        std::snprintf(text, sizeof text, "#%d", v.choice);
        return text;
    case VariableType::Text: return std::string("\"") + Safe(v.text) + "\"";
    default:
        std::snprintf(text, sizeof text, "%g", v.number);
        return std::string(text) + (unit && *unit ? std::string(" ") + unit : std::string());
    }
}
// Text containment and prefix tests ignore case (ASCII), so "eve" matches "Evening"; Equals stays exact.
bool SameLetter(char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); }
bool StartsWithText(const char* hay, const char* needle) {
    for (; *needle; ++hay, ++needle)
        if (!*hay || !SameLetter(*hay, *needle))
            return false;
    return true;
}
bool ContainsText(const char* hay, const char* needle) {
    for (; *hay; ++hay)
        if (StartsWithText(hay, needle))
            return true;
    return !*needle;
}
bool Near(double a, double b) { return std::abs(a - b) <= 1e-9 * std::max(1.0, std::max(std::abs(a), std::abs(b))); }
const ActionTemplate* FindTemplate(const RuleContext& c, StableId id) {
    for (const auto& t : c.actionTemplates)
        if (t.id == id)
            return &t;
    return nullptr;
}
} // namespace

std::span<const Compare> OperatorsFor(VariableType type) {
    switch (type) {
    case VariableType::Bool: return BoolOps;
    case VariableType::Text: return TextOps;
    case VariableType::Choice: return ChoiceOps;
    case VariableType::Time: return TimeOps;
    default: return NumberOps;
    }
}
const char* CompareWords(Compare op) {
    switch (op) {
    case Compare::Equals: return "is";
    case Compare::NotEquals: return "is not";
    case Compare::Greater: return "is greater than";
    case Compare::GreaterOrEqual: return "is at least";
    case Compare::Less: return "is less than";
    case Compare::LessOrEqual: return "is at most";
    case Compare::Between: return "is between";
    case Compare::Contains: return "contains";
    case Compare::StartsWith: return "starts with";
    case Compare::IsTrue: return "is on";
    default: return "is off";
    }
}
const VariableDef* FindVariable(const RuleContext& context, StableId id) {
    for (const auto& v : context.variables)
        if (v.id == id)
            return &v;
    return nullptr;
}
const char* RuleIssueText(RuleIssueKind kind) {
    switch (kind) {
    case RuleIssueKind::UnknownVariable: return "This variable no longer exists.";
    case RuleIssueKind::BadOperator: return "This comparison does not fit the variable.";
    case RuleIssueKind::EmptyRange: return "The range is empty.";
    case RuleIssueKind::EmptyText: return "Enter the text to compare with.";
    case RuleIssueKind::NoActions: return "Add at least one action.";
    case RuleIssueKind::TooDeep: return "Groups are nested too deeply.";
    case RuleIssueKind::UnknownAction: return "This action is not available.";
    default: return "Add a trigger, or the rule only runs manually.";
    }
}

// ---- Evaluation ------------------------------------------------------------------------------------------------
namespace {
struct Eval {
    const RuleView& rule;
    const RuleContext& context;
    VariableLookup lookup;
    std::span<ConditionTrace> trace;
    std::size_t traced = 0;
    bool missing = false;
};
void Record(Eval& e, StableId node, Truth t) {
    if (e.traced < e.trace.size())
        e.trace[e.traced] = {node, t};
    ++e.traced;
}
Truth Compare1(const VariableDef& def, const ConditionNode& n, const RuleValue& v) {
    const auto yes = [](bool b) { return b ? Truth::True : Truth::False; };
    if (!Allows(def.type, n.op))
        return Truth::False;
    switch (def.type) {
    case VariableType::Bool:
        return yes(n.op == Compare::IsTrue ? v.flag : !v.flag);
    case VariableType::Choice:
        return yes((v.choice == n.a.choice) == (n.op == Compare::Equals));
    case VariableType::Text: {
        const char* hay = Safe(v.text);
        const char* needle = Safe(n.a.text);
        switch (n.op) {
        case Compare::Equals: return yes(std::strcmp(hay, needle) == 0);
        case Compare::NotEquals: return yes(std::strcmp(hay, needle) != 0);
        case Compare::Contains: return yes(ContainsText(hay, needle));
        default: return yes(StartsWithText(hay, needle));
        }
    }
    default: {
        const double x = v.number, a = n.a.number, b = n.b.number;
        switch (n.op) {
        case Compare::Equals: return yes(Near(x, a));
        case Compare::NotEquals: return yes(!Near(x, a));
        case Compare::Greater: return yes(x > a);
        case Compare::GreaterOrEqual: return yes(x >= a);
        case Compare::Less: return yes(x < a);
        case Compare::LessOrEqual: return yes(x <= a);
        default:
            // A time range whose start is after its end wraps past midnight.
            if (def.type == VariableType::Time && a > b)
                return yes(x >= a || x <= b);
            return yes(x >= a && x <= b);
        }
    }
    }
}
bool EvalNode(Eval& e, const ConditionNode& n, int depth) {
    if (n.kind == NodeKind::Compare) {
        const VariableDef* def = FindVariable(e.context, n.variable);
        RuleValue v;
        Truth t = Truth::Unknown;
        if (def && e.lookup.read && e.lookup.read(e.lookup.user, n.variable, v))
            t = Compare1(*def, n, v);
        if (t == Truth::Unknown)
            e.missing = true;
        Record(e, n.id, t);
        return t == Truth::True;
    }
    int considered = 0, trues = 0;
    if (depth < 16)
        for (const ConditionNode& child : e.rule.conditions) {
            if (child.parent != n.id)
                continue;
            if (!child.enabled) {
                Record(e, child.id, Truth::Skipped);
                continue;
            }
            ++considered;
            trues += EvalNode(e, child, depth + 1) ? 1 : 0;
        }
    bool result = true;
    switch (n.mode) {
    case GroupMode::All: result = trues == considered; break;
    case GroupMode::Any: result = considered > 0 && trues > 0; break;
    default: result = trues == 0; break;
    }
    Record(e, n.id, result ? Truth::True : Truth::False);
    return result;
}
const ConditionNode* Root(const RuleView& rule) {
    for (const ConditionNode& n : rule.conditions)
        if (n.parent == 0 && n.kind == NodeKind::Group)
            return &n;
    return nullptr;
}
} // namespace

RuleOutcome EvaluateRule(const RuleView& rule, const RuleContext& context, VariableLookup lookup, std::span<ConditionTrace> trace) {
    Eval e{rule, context, lookup, trace};
    RuleOutcome out;
    const ConditionNode* root = Root(rule);
    out.fires = root ? EvalNode(e, *root, 0) : true;
    out.root = out.fires ? Truth::True : Truth::False;
    out.missingVariable = e.missing;
    out.traced = std::min(e.traced, trace.size());
    return out;
}

// ---- Sentence --------------------------------------------------------------------------------------------------
namespace {
void AppendCompare(std::string& s, const ConditionNode& n, const RuleContext& context) {
    const VariableDef* def = FindVariable(context, n.variable);
    s += def ? def->name : "(missing variable)";
    s += ' ';
    s += CompareWords(n.op);
    if (!def || n.op == Compare::IsTrue || n.op == Compare::IsFalse)
        return;
    s += ' ';
    s += ValueText(def->type, n.a, n.op == Compare::Between ? "" : def->unit, def->choices);
    if (n.op == Compare::Between) {
        s += " and ";
        s += ValueText(def->type, n.b, def->unit, def->choices);
    }
}
void AppendCondition(std::string& s, const RuleView& rule, const RuleContext& context, const ConditionNode& n, bool top, int depth) {
    if (n.kind == NodeKind::Compare) {
        AppendCompare(s, n, context);
        return;
    }
    std::vector<const ConditionNode*> kids;
    for (const ConditionNode& c : rule.conditions)
        if (c.parent == n.id && c.enabled)
            kids.push_back(&c);
    if (kids.empty()) {
        s += n.mode == GroupMode::Any ? "(nothing)" : "(always)";
        return;
    }
    const bool plain = kids.size() == 1 || (top && n.mode == GroupMode::All);
    if (!plain) {
        s += n.mode == GroupMode::All ? "all of: " : n.mode == GroupMode::Any ? "any of: " : "none of: ";
    }
    const char* join = n.mode == GroupMode::Any || n.mode == GroupMode::None ? " or " : " and ";
    for (std::size_t i = 0; i < kids.size(); ++i) {
        if (i)
            s += join;
        const bool nested = kids[i]->kind == NodeKind::Group && depth < 8;
        if (nested)
            s += '(';
        AppendCondition(s, rule, context, *kids[i], false, depth + 1);
        if (nested)
            s += ')';
    }
}
void AppendActions(std::string& s, std::span<const ActionNode> actions, const RuleContext& context) {
    bool first = true;
    for (const ActionNode& a : actions) {
        if (!a.enabled)
            continue;
        const ActionTemplate* t = FindTemplate(context, a.type);
        if (!first)
            s += a.parallel ? " together with " : ", then ";
        first = false;
        s += t ? t->name : "(unavailable action)";
        if (t)
            for (std::size_t i = 0; i < t->params.size() && i < a.values.size(); ++i) {
                s += ' ';
                s += ValueText(t->params[i].type, a.values[i], t->params[i].unit, t->params[i].choices);
            }
        if (a.delaySeconds > 0) {
            char text[32];
            std::snprintf(text, sizeof text, " after %g s", a.delaySeconds);
            s += text;
        }
    }
    if (first)
        s += "do nothing";
}
} // namespace

std::size_t FormatRuleSentence(const RuleView& rule, const RuleContext& context, std::span<char> out) {
    if (out.empty())
        return 0;
    std::string s = "When ";
    if (rule.triggers.empty())
        s += "it is run manually";
    for (std::size_t i = 0; i < rule.triggers.size(); ++i) {
        if (i)
            s += " or ";
        s += Safe(rule.triggers[i].text);
    }
    if (const ConditionNode* root = Root(rule)) {
        std::string cond;
        AppendCondition(cond, rule, context, *root, true, 0);
        if (cond != "(always)") {
            s += ", if ";
            s += cond;
        }
    }
    s += ", then ";
    AppendActions(s, rule.thenActions, context);
    if (rule.hasElse && !rule.elseActions.empty()) {
        s += ", otherwise ";
        AppendActions(s, rule.elseActions, context);
    }
    s += '.';
    const std::size_t n = std::min(s.size(), out.size() - 1);
    std::memcpy(out.data(), s.data(), n);
    if (n < s.size() && n >= 2) {
        out[n - 2] = '.';
        out[n - 1] = '.';
    }
    out[n] = '\0';
    return n;
}

std::size_t ValidateRule(const RuleView& rule, const RuleContext& context, std::span<RuleIssue> issues) {
    std::size_t count = 0;
    const auto add = [&](StableId node, RuleIssueKind kind) {
        if (count < issues.size())
            issues[count] = {node, kind};
        ++count;
    };
    for (const ConditionNode& n : rule.conditions) {
        int depth = 0;
        StableId parent = n.parent;
        while (parent && depth < 64) {
            ++depth;
            StableId next = 0;
            for (const ConditionNode& p : rule.conditions)
                if (p.id == parent) {
                    next = p.parent;
                    break;
                }
            parent = next;
        }
        if (depth > 8)
            add(n.id, RuleIssueKind::TooDeep);
        if (n.kind != NodeKind::Compare || !n.enabled)
            continue;
        const VariableDef* def = FindVariable(context, n.variable);
        if (!def) {
            add(n.id, RuleIssueKind::UnknownVariable);
            continue;
        }
        if (!Allows(def->type, n.op))
            add(n.id, RuleIssueKind::BadOperator);
        else if (n.op == Compare::Between && def->type != VariableType::Time && n.a.number > n.b.number)
            add(n.id, RuleIssueKind::EmptyRange);
        else if (def->type == VariableType::Text && !(n.a.text && n.a.text[0]))
            add(n.id, RuleIssueKind::EmptyText);
    }
    bool anyThen = false;
    for (const ActionNode& a : rule.thenActions) {
        anyThen = anyThen || a.enabled;
        if (!FindTemplate(context, a.type))
            add(a.id, RuleIssueKind::UnknownAction);
    }
    for (const ActionNode& a : rule.elseActions)
        if (!FindTemplate(context, a.type))
            add(a.id, RuleIssueKind::UnknownAction);
    if (!anyThen)
        add(0, RuleIssueKind::NoActions);
    if (rule.triggers.empty())
        add(0, RuleIssueKind::NoTrigger);
    return count;
}

// ---- RuleDocument -----------------------------------------------------------------------------------------------
RuleDocument::RuleDocument() {
    Cond root;
    root.node.id = NextId();
    root.node.kind = NodeKind::Group;
    root.node.mode = GroupMode::All;
    conditions_.push_back(root);
    Refresh();
}
void RuleDocument::SetName(std::string name) {
    name_ = std::move(name);
    Refresh();
}
StableId RuleDocument::AddTrigger(TriggerKind kind, std::string text) {
    Trig t;
    t.node.id = NextId();
    t.node.kind = kind;
    t.text = std::move(text);
    triggers_.push_back(std::move(t));
    Refresh();
    return triggers_.back().node.id;
}
StableId RuleDocument::AddGroup(StableId parent, GroupMode mode) {
    Cond c;
    c.node.id = NextId();
    c.node.parent = parent ? parent : conditions_.front().node.id;
    c.node.kind = NodeKind::Group;
    c.node.mode = mode;
    conditions_.push_back(c);
    Refresh();
    return conditions_.back().node.id;
}
StableId RuleDocument::AddCompare(StableId parent, StableId variable, Compare op, RuleValue a, RuleValue b) {
    Cond c;
    c.node.id = NextId();
    c.node.parent = parent ? parent : conditions_.front().node.id;
    c.node.kind = NodeKind::Compare;
    c.node.variable = variable;
    c.node.op = op;
    c.node.a = a;
    c.node.b = b;
    c.textA = Safe(a.text);
    c.textB = Safe(b.text);
    conditions_.push_back(std::move(c));
    Refresh();
    return conditions_.back().node.id;
}
StableId RuleDocument::AddAction(StableId type, std::vector<RuleValue> values, bool elseBranch) {
    Act a;
    a.node.id = NextId();
    a.node.type = type;
    a.values = std::move(values);
    for (const RuleValue& v : a.values)
        a.texts.emplace_back(Safe(v.text));
    a.elseBranch = elseBranch;
    if (elseBranch)
        hasElse_ = true;
    actions_.push_back(std::move(a));
    Refresh();
    return actions_.back().node.id;
}
void RuleDocument::RemoveCondition(StableId id) {
    if (id == conditions_.front().node.id)
        return; // The root group stays.
    std::vector<StableId> doomed{id};
    for (std::size_t i = 0; i < doomed.size(); ++i)
        for (const Cond& c : conditions_)
            if (c.node.parent == doomed[i])
                doomed.push_back(c.node.id);
    conditions_.erase(std::remove_if(conditions_.begin(), conditions_.end(),
                                     [&](const Cond& c) { return std::find(doomed.begin(), doomed.end(), c.node.id) != doomed.end(); }),
                      conditions_.end());
}
void RuleDocument::ApplyAll(std::span<const RuleEdit> edits, const RuleContext& context) {
    for (const RuleEdit& e : edits)
        Apply(e, context);
}
void RuleDocument::Apply(const RuleEdit& e, const RuleContext& context) {
    using K = RuleEdit::Kind;
    const auto cond = [&](StableId id) -> Cond* {
        for (Cond& c : conditions_)
            if (c.node.id == id)
                return &c;
        return nullptr;
    };
    const auto act = [&](StableId id) -> Act* {
        for (Act& a : actions_)
            if (a.node.id == id)
                return &a;
        return nullptr;
    };
    switch (e.kind) {
    case K::SetEnabled: enabled_ = e.flag; break;
    case K::SetElse:
        hasElse_ = e.flag;
        if (!e.flag)
            actions_.erase(std::remove_if(actions_.begin(), actions_.end(), [](const Act& a) { return a.elseBranch; }), actions_.end());
        break;
    case K::AddTrigger: {
        std::string text = "Trigger";
        for (const auto& o : context.triggerOptions)
            if (static_cast<int>(o.kind) == e.index)
                text = o.defaultText;
        AddTrigger(static_cast<TriggerKind>(e.index), std::move(text));
        return;
    }
    case K::RemoveTrigger:
        triggers_.erase(std::remove_if(triggers_.begin(), triggers_.end(), [&](const Trig& t) { return t.node.id == e.node; }), triggers_.end());
        break;
    case K::AddCondition: {
        const VariableDef* def = FindVariable(context, e.item);
        if (!def)
            return;
        RuleValue a = DefaultValue(def->type, def->minimum);
        AddCompare(e.parent, def->id, OperatorsFor(def->type).front(), a, a);
        return;
    }
    case K::AddGroup: AddGroup(e.parent, GroupMode::Any); return;
    case K::RemoveCondition: RemoveCondition(e.node); break;
    case K::SetGroupMode:
        if (Cond* c = cond(e.node))
            c->node.mode = e.mode;
        break;
    case K::SetVariable:
        if (Cond* c = cond(e.node))
            if (const VariableDef* def = FindVariable(context, e.item)) {
                c->node.variable = def->id;
                if (!Allows(def->type, c->node.op))
                    c->node.op = OperatorsFor(def->type).front();
                c->node.a = c->node.b = DefaultValue(def->type, def->minimum);
                c->textA.clear();
                c->textB.clear();
            }
        break;
    case K::SetOperator:
        if (Cond* c = cond(e.node))
            c->node.op = e.op;
        break;
    case K::SetOperandA:
    case K::SetOperandB:
        if (Cond* c = cond(e.node)) {
            RuleValue& v = e.kind == K::SetOperandA ? c->node.a : c->node.b;
            std::string& t = e.kind == K::SetOperandA ? c->textA : c->textB;
            v = e.value;
            t = Safe(e.value.text);
        }
        break;
    case K::SetConditionEnabled:
        if (Cond* c = cond(e.node))
            c->node.enabled = e.flag;
        break;
    case K::AddAction: {
        const ActionTemplate* t = FindTemplate(context, e.item);
        if (!t)
            return;
        std::vector<RuleValue> values;
        for (const ParamDef& p : t->params) {
            RuleValue v;
            v.number = p.defaultNumber;
            v.flag = true;
            values.push_back(v);
        }
        AddAction(t->id, std::move(values), e.elseBranch);
        return;
    }
    case K::RemoveAction:
        actions_.erase(std::remove_if(actions_.begin(), actions_.end(), [&](const Act& a) { return a.node.id == e.node; }), actions_.end());
        break;
    case K::MoveAction:
        if (Act* a = act(e.node)) {
            // `index` is the destination position within the same branch.
            std::vector<std::size_t> branch;
            for (std::size_t i = 0; i < actions_.size(); ++i)
                if (actions_[i].elseBranch == a->elseBranch)
                    branch.push_back(i);
            const auto from = std::find(branch.begin(), branch.end(), static_cast<std::size_t>(a - actions_.data()));
            if (from == branch.end() || branch.empty())
                break;
            const std::size_t fromIndex = static_cast<std::size_t>(from - branch.begin());
            const std::size_t toIndex = std::min(static_cast<std::size_t>(std::max(0, e.index)), branch.size() - 1);
            Act moved = std::move(*a);
            std::vector<Act> list;
            for (std::size_t i : branch)
                if (i != branch[fromIndex])
                    list.push_back(std::move(actions_[i]));
            list.insert(list.begin() + static_cast<std::ptrdiff_t>(toIndex), std::move(moved));
            for (std::size_t k = 0; k < branch.size(); ++k)
                actions_[branch[k]] = std::move(list[k]);
        }
        break;
    case K::SetActionValue:
        if (Act* a = act(e.node); a && e.index >= 0 && e.index < static_cast<int>(a->values.size())) {
            a->values[static_cast<std::size_t>(e.index)] = e.value;
            a->texts[static_cast<std::size_t>(e.index)] = Safe(e.value.text);
        }
        break;
    case K::SetActionDelay:
        if (Act* a = act(e.node))
            a->node.delaySeconds = std::max(0.f, e.number);
        break;
    case K::SetActionParallel:
        if (Act* a = act(e.node))
            a->node.parallel = e.flag;
        break;
    case K::SetActionEnabled:
        if (Act* a = act(e.node))
            a->node.enabled = e.flag;
        break;
    default: break;
    }
    Refresh();
}
void RuleDocument::Refresh() {
    triggerViews_.clear();
    for (Trig& t : triggers_) {
        t.node.text = t.text.c_str();
        triggerViews_.push_back(t.node);
    }
    conditionViews_.clear();
    for (Cond& c : conditions_) {
        c.node.a.text = c.textA.c_str();
        c.node.b.text = c.textB.c_str();
        conditionViews_.push_back(c.node);
    }
    thenViews_.clear();
    elseViews_.clear();
    for (Act& a : actions_) {
        for (std::size_t i = 0; i < a.values.size(); ++i)
            a.values[i].text = a.texts[i].c_str();
        a.node.values = a.values;
        (a.elseBranch ? elseViews_ : thenViews_).push_back(a.node);
    }
    view_ = {name_.c_str(), enabled_, hasElse_, triggerViews_, conditionViews_, thenViews_, elseViews_};
}
} // namespace imkit::show
