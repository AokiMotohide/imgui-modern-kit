#include <imkit/show/rules.h>
#include "internal.h"
#include <imkit/components.h>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstring>
#include <string>

namespace imkit::show {
using namespace impl;

namespace {
bool Contains(const char* hay, const char* needle) {
    if (!needle || !*needle)
        return true;
    const std::size_t n = std::strlen(needle);
    for (const char* h = hay; *h; ++h) {
        std::size_t i = 0;
        while (i < n && h[i] && std::tolower(static_cast<unsigned char>(h[i])) == std::tolower(static_cast<unsigned char>(needle[i])))
            ++i;
        if (i == n)
            return true;
    }
    return false;
}
const char* TypeWord(VariableType t) {
    switch (t) {
    case VariableType::Bool: return "on/off";
    case VariableType::Text: return "text";
    case VariableType::Choice: return "choice";
    case VariableType::Time: return "time";
    default: return "number";
    }
}
void ClockText(char* out, std::size_t n, double seconds) {
    const int total = static_cast<int>(std::fmod(std::max(0.0, seconds), 86400.0));
    std::snprintf(out, n, "%02d:%02d", total / 3600, total / 60 % 60);
}
std::string Format(VariableType type, const RuleValue& v, const char* unit, std::span<const char* const> choices) {
    char text[64];
    switch (type) {
    case VariableType::Bool: return v.flag ? "on" : "off";
    case VariableType::Time: ClockText(text, sizeof text, v.number); return text;
    case VariableType::Choice:
        if (v.choice >= 0 && v.choice < static_cast<int>(choices.size()))
            return choices[static_cast<std::size_t>(v.choice)];
        return "?";
    case VariableType::Text: return std::string("\"") + Safe(v.text) + "\"";
    default:
        std::snprintf(text, sizeof text, "%g", v.number);
        return std::string(text) + (unit && *unit ? std::string(" ") + unit : std::string());
    }
}

struct Ui {
    const ShowColors& c;
    const ComponentOptions& o;
    const RuleView& rule;
    const RuleContext& ctx;
    RuleBuilderState& st;
    RuleEditBuffer& edits;
    std::span<const RuleIssue> issues;
    float u;
    ImDrawList* draw;

    const RuleIssue* IssueFor(StableId node) const {
        for (const RuleIssue& i : issues)
            if (i.node == node && node != 0)
                return &i;
        return nullptr;
    }
    bool Chip(const char* id, const char* text, ImVec4 ink, ImVec4 tone = {}, bool dashed = false) const {
        return PillButton(id, text, c, o, ink, tone, {}, dashed);
    }
    bool IconButton(const char* id, int glyph, ImVec4 ink, const char* accessibleName) const {
        ImGui::PushID(id);
        const float s = u * 1.0f;
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##icon", {s, s});
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        const ImVec2 m{a.x + s * .5f, a.y + s * .5f};
        const ImU32 col = U32(hovered ? Mix(ink, c.text, .5f) : ink);
        if (glyph == 0)
            detail::DrawCross(draw, m, s * .36f, col, 1.5f);
        else {
            draw->AddLine({m.x - s * .24f, m.y}, {m.x + s * .24f, m.y}, col, 1.6f);
            draw->AddLine({m.x, m.y - s * .24f}, {m.x, m.y + s * .24f}, col, 1.6f);
        }
        SemanticItem(o, accessibleName, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        ImGui::PopID();
        return clicked;
    }
    bool Dot(const char* id, bool on, const char* accessibleName) const {
        ImGui::PushID(id);
        const float s = u * .8f;
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##dot", {s, s});
        bool clicked = ImGui::IsItemClicked();
        if (SemanticItem(o, accessibleName, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, on))
            clicked = true;
        const ImVec2 m{a.x + s * .5f, a.y + s * .5f};
        if (on)
            draw->AddCircleFilled(m, s * .26f, U32(c.accent), 14);
        else
            draw->AddCircle(m, s * .26f, U32(c.muted), 14, 1.5f);
        if (ImGui::IsItemHovered())
            draw->AddCircle(m, s * .42f, U32(c.muted, .5f), 16, 1.f);
        ImGui::PopID();
        return clicked;
    }
    void Gap() const { ImGui::SameLine(0, u * .3f); }

    // ---- value editing ----
    // Edits one value of `type`; returns true and fills `out` on change.
    bool EditValue(const char* id, VariableType type, double mn, double mx, const char* unit, std::span<const char* const> choices,
                   const RuleValue& in, RuleValue& out, StableId owner, int side) const {
        bool changed = false;
        out = in;
        ImGui::PushID(id);
        switch (type) {
        case VariableType::Bool: break;
        case VariableType::Choice: {
            const char* preview = in.choice >= 0 && in.choice < static_cast<int>(choices.size()) ? choices[static_cast<std::size_t>(in.choice)] : "?";
            if (Chip("choice", preview, c.warning, {}, false))
                ImGui::OpenPopup("##choices");
            if (ImGui::BeginPopup("##choices")) {
                for (int i = 0; i < static_cast<int>(choices.size()); ++i)
                    if (ImGui::Selectable(choices[static_cast<std::size_t>(i)], i == in.choice)) {
                        out.choice = i;
                        changed = true;
                    }
                ImGui::EndPopup();
            }
            break;
        }
        case VariableType::Time: {
            char text[16];
            ClockText(text, sizeof text, in.number);
            if (Chip("time", text, c.warning))
                ImGui::OpenPopup("##time");
            if (ImGui::BeginPopup("##time")) {
                int h = static_cast<int>(in.number) / 3600, m = static_cast<int>(in.number) / 60 % 60;
                ImGui::SetNextItemWidth(u * 4.f);
                bool edit = ImGui::InputInt("##h", &h, 1, 6);
                ImGui::SameLine();
                ImGui::TextUnformatted(":");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(u * 4.f);
                edit |= ImGui::InputInt("##m", &m, 5, 15);
                if (edit) {
                    out.number = std::clamp(h, 0, 23) * 3600 + std::clamp(m, 0, 59) * 60;
                    changed = true;
                }
                ImGui::EndPopup();
            }
            break;
        }
        case VariableType::Text: {
            if (st.editing == owner && st.editingSide == side) {
                ImGui::SetNextItemWidth(u * 8.f);
                if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive())
                    ImGui::SetKeyboardFocusHere();
                const bool entered = ImGui::InputText("##text", st.text, sizeof(st.text), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (entered || ImGui::IsItemDeactivated()) {
                    out.text = st.text;
                    changed = true;
                    st.editing = 0;
                }
            } else {
                const char* shown = in.text && in.text[0] ? in.text : "type text";
                if (Chip("text", shown, in.text && in.text[0] ? c.warning : c.muted, {}, !(in.text && in.text[0]))) {
                    st.editing = owner;
                    st.editingSide = side;
                    std::snprintf(st.text, sizeof(st.text), "%s", Safe(in.text));
                }
            }
            break;
        }
        default: {
            double v = in.number;
            ImGui::SetNextItemWidth(u * 4.4f);
            double lo = mn, hi = mx;
            const double speed = std::max(.01, (mx - mn) / 200.0);
            if (ImGui::DragScalar("##n", ImGuiDataType_Double, &v, static_cast<float>(speed), &lo, &hi, "%g", ImGuiSliderFlags_AlwaysClamp)) {
                out.number = v;
                changed = true;
            }
            if (unit && *unit) {
                ImGui::SameLine(0, 4.f);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%s", unit);
            }
            break;
        }
        }
        ImGui::PopID();
        return changed;
    }

    // ---- pickers ----
    void VariablePicker(const char* popup, StableId parent, StableId node, RuleEdit::Kind kind) const {
        if (ImGui::IsPopupOpen(popup)) {
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
            ImGui::SetNextWindowSizeConstraints({u * 11.f, 0}, {u * 16.f, u * 16.f});
        }
        if (!ImGui::BeginPopup(popup))
            return;
        if (ImGui::IsWindowAppearing()) {
            st.search[0] = 0;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##search", Str(o, "show.searchVariables", "Search variables"), st.search, sizeof(st.search));
        const char* group = nullptr;
        for (const VariableDef& v : ctx.variables) {
            if (!Contains(Safe(v.name), st.search) && !Contains(Safe(v.group), st.search))
                continue;
            if (!group || std::strcmp(group, Safe(v.group)) != 0) {
                group = Safe(v.group);
                if (*group)
                    ImGui::SeparatorText(group);
            }
            const float w = ImGui::GetContentRegionAvail().x;
            if (ImGui::Selectable(FrameText("%s##v%llu", Safe(v.name), static_cast<unsigned long long>(v.id)), false, 0, {w, 0})) {
                RuleEdit e;
                e.kind = kind;
                e.parent = parent;
                e.node = node;
                e.item = v.id;
                edits.Push(e);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine(w - ImGui::CalcTextSize(TypeWord(v.type)).x);
            ImGui::TextDisabled("%s", TypeWord(v.type));
        }
        ImGui::EndPopup();
    }
    void OperatorPicker(const char* popup, const ConditionNode& n, const VariableDef& def) const {
        if (ImGui::IsPopupOpen(popup))
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
        if (!ImGui::BeginPopup(popup))
            return;
        for (Compare op : OperatorsFor(def.type))
            if (ImGui::Selectable(CompareWords(op), op == n.op)) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::SetOperator;
                e.node = n.id;
                e.op = op;
                edits.Push(e);
            }
        ImGui::EndPopup();
    }
    void ActionPicker(const char* popup, bool elseBranch) const {
        if (ImGui::IsPopupOpen(popup)) {
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
            ImGui::SetNextWindowSizeConstraints({u * 11.f, 0}, {u * 16.f, u * 16.f});
        }
        if (!ImGui::BeginPopup(popup))
            return;
        if (ImGui::IsWindowAppearing()) {
            st.search[0] = 0;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##search", Str(o, "show.searchActions", "Search actions"), st.search, sizeof(st.search));
        const char* group = nullptr;
        for (const ActionTemplate& t : ctx.actionTemplates) {
            if (!Contains(Safe(t.name), st.search) && !Contains(Safe(t.group), st.search))
                continue;
            if (!group || std::strcmp(group, Safe(t.group)) != 0) {
                group = Safe(t.group);
                if (*group)
                    ImGui::SeparatorText(group);
            }
            if (ImGui::Selectable(FrameText("%s##t%llu", Safe(t.name), static_cast<unsigned long long>(t.id)))) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::AddAction;
                e.item = t.id;
                e.elseBranch = elseBranch;
                edits.Push(e);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    // ---- conditions ----
    void Condition(const ConditionNode& n) const {
        ImGui::PushID(static_cast<int>(n.id));
        const VariableDef* def = FindVariable(ctx, n.variable);
        if (Dot("on", n.enabled, FrameText("Enable condition %s", def ? def->name : "")) ) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::SetConditionEnabled;
            e.node = n.id;
            e.flag = !n.enabled;
            edits.Push(e);
        }
        Gap();
        ImGui::BeginDisabled(!n.enabled);
        if (Chip("var", def ? def->name : "(missing)", def ? c.accent : c.warning, def ? ImVec4{} : c.warning))
            ImGui::OpenPopup("##variables");
        VariablePicker("##variables", 0, n.id, RuleEdit::Kind::SetVariable);
        if (def) {
            Gap();
            if (Chip("op", CompareWords(n.op), c.text))
                ImGui::OpenPopup("##operators");
            OperatorPicker("##operators", n, *def);
            if (n.op != Compare::IsTrue && n.op != Compare::IsFalse) {
                Gap();
                RuleValue out;
                if (EditValue("a", def->type, def->minimum, def->maximum, n.op == Compare::Between ? "" : def->unit, def->choices, n.a, out, n.id, 0)) {
                    RuleEdit e;
                    e.kind = RuleEdit::Kind::SetOperandA;
                    e.node = n.id;
                    e.value = out;
                    edits.Push(e);
                }
                if (n.op == Compare::Between) {
                    Gap();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%s", Str(o, "show.and", "and"));
                    Gap();
                    RuleValue outB;
                    if (EditValue("b", def->type, def->minimum, def->maximum, def->unit, def->choices, n.b, outB, n.id, 1)) {
                        RuleEdit e;
                        e.kind = RuleEdit::Kind::SetOperandB;
                        e.node = n.id;
                        e.value = outB;
                        edits.Push(e);
                    }
                }
            }
        }
        ImGui::EndDisabled();
        if (const RuleIssue* issue = IssueFor(n.id)) {
            Gap();
            ImGui::Dummy({u * .8f, u * .8f});
            GlyphWarning(draw, {ImGui::GetItemRectMin().x + u * .4f, ImGui::GetItemRectMin().y + u * .45f}, u * .7f, U32(c.warning));
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Str(o, "show.issue", RuleIssueText(issue->kind)));
        }
        Gap();
        if (IconButton("remove", 0, c.muted, "Remove condition")) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::RemoveCondition;
            e.node = n.id;
            edits.Push(e);
        }
        ImGui::PopID();
    }
    void Group(const ConditionNode& n, int depth) const {
        ImGui::PushID(static_cast<int>(n.id) + 5000);
        const float x0 = ImGui::GetCursorScreenPos().x;
        const float y0 = ImGui::GetCursorScreenPos().y;
        const char* modes[3] = {Str(o, "show.modeAll", "All"), Str(o, "show.modeAny", "Any"), Str(o, "show.modeNone", "None")};
        const ImVec4 tones[3] = {c.accent, c.warning, c.error};
        if (Chip("mode", modes[static_cast<int>(n.mode)], tones[static_cast<int>(n.mode)], tones[static_cast<int>(n.mode)]))
            ImGui::OpenPopup("##mode");
        if (ImGui::IsPopupOpen("##mode"))
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
        if (ImGui::BeginPopup("##mode")) {
            const char* words[3] = {"All of these are true", "Any of these is true", "None of these are true"};
            for (int i = 0; i < 3; ++i)
                if (ImGui::Selectable(words[i], static_cast<int>(n.mode) == i)) {
                    RuleEdit e;
                    e.kind = RuleEdit::Kind::SetGroupMode;
                    e.node = n.id;
                    e.mode = static_cast<GroupMode>(i);
                    edits.Push(e);
                }
            ImGui::EndPopup();
        }
        Gap();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", n.mode == GroupMode::All   ? Str(o, "show.allTrue", "of these are true")
                                  : n.mode == GroupMode::Any ? Str(o, "show.anyTrue", "of these is true")
                                                             : Str(o, "show.noneTrue", "of these are true"));
        Gap();
        if (Chip("addCond", Str(o, "show.addCondition", "+ condition"), c.muted, {}, true))
            ImGui::OpenPopup("##addCond");
        VariablePicker("##addCond", n.id, 0, RuleEdit::Kind::AddCondition);
        Gap();
        if (Chip("addGroup", Str(o, "show.addGroup", "+ group"), c.muted, {}, true)) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::AddGroup;
            e.parent = n.id;
            edits.Push(e);
        }
        if (depth > 0) {
            Gap();
            if (IconButton("removeGroup", 0, c.muted, "Remove group")) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::RemoveCondition;
                e.node = n.id;
                edits.Push(e);
            }
        }
        // Children, indented with a rail on the left.
        ImGui::Indent(u * 1.1f);
        bool any = false;
        for (const ConditionNode& child : rule.conditions) {
            if (child.parent != n.id)
                continue;
            any = true;
            if (child.kind == NodeKind::Group)
                Group(child, depth + 1);
            else
                Condition(child);
        }
        if (!any) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", Str(o, "show.noConditions", "No conditions yet. The rule always applies."));
        }
        ImGui::Unindent(u * 1.1f);
        const float y1 = ImGui::GetCursorScreenPos().y;
        draw->AddLine({x0 + u * .5f, y0 + u * 1.1f}, {x0 + u * .5f, y1 - u * .2f}, U32(Mix(c.border, tones[static_cast<int>(n.mode)], .5f), .9f), 2.f);
        ImGui::PopID();
    }

    // ---- actions ----
    void Actions(std::span<const ActionNode> list, bool elseBranch) const {
        for (int index = 0; index < static_cast<int>(list.size()); ++index) {
            const ActionNode& a = list[static_cast<std::size_t>(index)];
            const ActionTemplate* t = nullptr;
            for (const ActionTemplate& tt : ctx.actionTemplates)
                if (tt.id == a.type)
                    t = &tt;
            ImGui::PushID(static_cast<int>(a.id) + 9000);
            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            // Grip (drag source for reordering).
            ImGui::InvisibleButton("##grip", {u * .8f, u});
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
                struct Payload { StableId id; int elseBranch; } payload{a.id, elseBranch ? 1 : 0};
                ImGui::SetDragDropPayload("IMKIT_ACTION", &payload, sizeof(payload));
                ImGui::TextUnformatted(t ? t->name : "Action");
                ImGui::EndDragDropSource();
            }
            for (int d = 0; d < 6; ++d)
                draw->AddCircleFilled({rowMin.x + u * .22f + static_cast<float>(d % 2) * u * .2f, rowMin.y + u * .5f + (static_cast<float>(d / 2) - 1.f) * u * .2f},
                                      1.3f, U32(c.muted, ImGui::IsItemHovered() ? 1.f : .6f), 6);
            SemanticItem(o, FrameText("Move action %d", index + 1), accessibility::SemanticRole::Button, accessibility::SemanticAction::None);
            Gap();
            if (Dot("on", a.enabled, FrameText("Enable action %d", index + 1))) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::SetActionEnabled;
                e.node = a.id;
                e.flag = !a.enabled;
                edits.Push(e);
            }
            Gap();
            ImGui::BeginDisabled(!a.enabled);
            if (index > 0 && a.parallel) {
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%s", Str(o, "show.together", "together"));
                Gap();
            }
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(t ? c.text : c.warning, "%s", t ? t->name : "(unavailable action)");
            if (t)
                for (int p = 0; p < static_cast<int>(t->params.size()) && p < static_cast<int>(a.values.size()); ++p) {
                    const ParamDef& pd = t->params[static_cast<std::size_t>(p)];
                    Gap();
                    RuleValue out;
                    if (EditValue(FrameText("p%d", p), pd.type, pd.minimum, pd.maximum, pd.unit, pd.choices, a.values[static_cast<std::size_t>(p)], out, a.id, p + 10)) {
                        RuleEdit e;
                        e.kind = RuleEdit::Kind::SetActionValue;
                        e.node = a.id;
                        e.index = p;
                        e.value = out;
                        edits.Push(e);
                    }
                    if (pd.type == VariableType::Bool) {
                        bool flag = a.values[static_cast<std::size_t>(p)].flag;
                        if (Chip("flag", flag ? Str(o, "show.on", "on") : Str(o, "show.off", "off"), c.warning)) {
                            RuleEdit e;
                            e.kind = RuleEdit::Kind::SetActionValue;
                            e.node = a.id;
                            e.index = p;
                            e.value = a.values[static_cast<std::size_t>(p)];
                            e.value.flag = !flag;
                            edits.Push(e);
                        }
                    }
                }
            Gap();
            float delay = a.delaySeconds;
            ImGui::SetNextItemWidth(u * 3.6f);
            if (ImGui::DragFloat("##delay", &delay, .1f, 0.f, 3600.f, "after %.1f s", ImGuiSliderFlags_AlwaysClamp)) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::SetActionDelay;
                e.node = a.id;
                e.number = delay;
                edits.Push(e);
            }
            ImGui::EndDisabled();
            if (index > 0) {
                Gap();
                if (Chip("parallel", Str(o, "show.together", "together"), a.parallel ? c.accent : c.muted, a.parallel ? c.accent : ImVec4{})) {
                    RuleEdit e;
                    e.kind = RuleEdit::Kind::SetActionParallel;
                    e.node = a.id;
                    e.flag = !a.parallel;
                    edits.Push(e);
                }
            }
            Gap();
            if (IconButton("remove", 0, c.muted, FrameText("Remove action %d", index + 1))) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::RemoveAction;
                e.node = a.id;
                edits.Push(e);
            }
            ImGui::PopID();
            DropZone(list, elseBranch, index + 1);
        }
    }
    // Thin drop target between action rows; `slot` is the insertion position within the branch.
    void DropZone(std::span<const ActionNode> list, bool elseBranch, int slot) const {
        ImGui::PushID(slot + 20000);
        ImGui::Dummy({std::max(u, ImGui::GetContentRegionAvail().x), u * .3f});
        if (ImGui::BeginDragDropTarget()) {
            struct Payload {
                StableId id;
                int elseBranch;
            };
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(
                    "IMKIT_ACTION", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                if (p->DataSize == sizeof(Payload)) {
                    const Payload pl = *static_cast<const Payload*>(p->Data);
                    if ((pl.elseBranch != 0) == elseBranch) {
                        const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
                        draw->AddLine({a.x, (a.y + b.y) * .5f}, {b.x, (a.y + b.y) * .5f}, U32(c.accent), 2.f);
                        if (p->IsDelivery()) {
                            int from = -1;
                            for (int i = 0; i < static_cast<int>(list.size()); ++i)
                                if (list[static_cast<std::size_t>(i)].id == pl.id)
                                    from = i;
                            if (from >= 0) {
                                RuleEdit e;
                                e.kind = RuleEdit::Kind::MoveAction;
                                e.node = pl.id;
                                e.index = from < slot ? slot - 1 : slot;
                                if (e.index != from)
                                    edits.Push(e);
                            }
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
    }
    void SectionLabel(const char* text) const {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", text);
        ImGui::SameLine(u * 3.4f);
    }
};
} // namespace

void RuleBuilder(const char* id, const RuleView& rule, const RuleContext& context, RuleBuilderState& state, RuleEditBuffer& edits,
                 RuleBuilderOptions layout, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    Ui ui{c, options, rule, context, state, edits, layout.issues, u, ImGui::GetWindowDrawList()};
    ImGui::PushID(id);
    ImGui::BeginGroup();

    // Title row: name, enabled switch and the issue count.
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Safe(rule.name));
    ImGui::SameLine(0, u);
    bool enabled = rule.enabled;
    if (Toggle(Str(options, "show.ruleEnabled", "Enabled"), &enabled, options)) {
        RuleEdit e;
        e.kind = RuleEdit::Kind::SetEnabled;
        e.flag = enabled;
        edits.Push(e);
    }
    if (!layout.issues.empty()) {
        ImGui::SameLine(0, u);
        char text[40];
        std::snprintf(text, sizeof(text), "%d %s", static_cast<int>(layout.issues.size()), layout.issues.size() == 1 ? "issue" : "issues");
        ui.Chip("issues", text, c.warning, c.warning);
    }
    ImGui::Spacing();

    // When.
    ui.SectionLabel(Str(options, "show.when", "When"));
    ImGui::BeginGroup();
    for (const TriggerNode& t : rule.triggers) {
        ImGui::PushID(static_cast<int>(t.id));
        ui.Chip("trigger", Safe(t.text), c.text, c.accent);
        ImGui::SameLine(0, 2.f);
        if (ui.IconButton("remove", 0, c.muted, "Remove trigger")) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::RemoveTrigger;
            e.node = t.id;
            edits.Push(e);
        }
        ImGui::SameLine(0, u * .4f);
        ImGui::PopID();
    }
    if (ui.Chip("addTrigger", Str(options, "show.addTrigger", "+ trigger"), c.muted, {}, true))
        ImGui::OpenPopup("##triggers");
    if (ImGui::IsPopupOpen("##triggers"))
        ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
    if (ImGui::BeginPopup("##triggers")) {
        for (const TriggerOption& t : context.triggerOptions)
            if (ImGui::Selectable(Safe(t.name))) {
                RuleEdit e;
                e.kind = RuleEdit::Kind::AddTrigger;
                e.index = static_cast<int>(t.kind);
                edits.Push(e);
            }
        ImGui::EndPopup();
    }
    ImGui::EndGroup();
    ImGui::Spacing();

    // If.
    ui.SectionLabel(Str(options, "show.if", "If"));
    ImGui::BeginGroup();
    const ConditionNode* root = nullptr;
    for (const ConditionNode& n : rule.conditions)
        if (n.parent == 0 && n.kind == NodeKind::Group)
            root = &n;
    if (root)
        ui.Group(*root, 0);
    ImGui::EndGroup();
    ImGui::Spacing();

    // Then.
    ui.SectionLabel(Str(options, "show.then", "Then"));
    ImGui::BeginGroup();
    ui.DropZone(rule.thenActions, false, 0);
    ui.Actions(rule.thenActions, false);
    if (ui.Chip("addThen", Str(options, "show.addAction", "+ action"), c.muted, {}, true))
        ImGui::OpenPopup("##actionsThen");
    ui.ActionPicker("##actionsThen", false);
    ImGui::EndGroup();
    ImGui::Spacing();

    // Else.
    ui.SectionLabel(Str(options, "show.else", "Else"));
    ImGui::BeginGroup();
    if (!rule.hasElse) {
        if (ui.Chip("addElse", Str(options, "show.addElse", "+ otherwise"), c.muted, {}, true)) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::SetElse;
            e.flag = true;
            edits.Push(e);
        }
    } else {
        ui.DropZone(rule.elseActions, true, 0);
        ui.Actions(rule.elseActions, true);
        if (ui.Chip("addElseAction", Str(options, "show.addAction", "+ action"), c.muted, {}, true))
            ImGui::OpenPopup("##actionsElse");
        ui.ActionPicker("##actionsElse", true);
        ImGui::SameLine(0, u * .5f);
        if (ui.Chip("removeElse", Str(options, "show.removeElse", "remove otherwise"), c.muted)) {
            RuleEdit e;
            e.kind = RuleEdit::Kind::SetElse;
            e.flag = false;
            edits.Push(e);
        }
    }
    ImGui::EndGroup();

    if (layout.showSentence) {
        ImGui::Spacing();
        char sentence[640];
        FormatRuleSentence(rule, context, sentence);
        const float width = ImGui::GetContentRegionAvail().x;
        const ImVec2 ts = ImGui::CalcTextSize(sentence, nullptr, false, width - u * 1.2f);
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + ts.y + u * .8f};
        ImGui::Dummy({width, ts.y + u * .8f});
        ui.draw->AddRectFilled(a, b, U32(Mix(c.surface, c.accent, .06f)), u * .4f);
        ui.draw->AddRect(a, b, U32(Mix(c.border, c.accent, .3f), .8f), u * .4f, 0, 1.f);
        ui.draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {a.x + u * .6f, a.y + u * .4f}, U32(c.muted), sentence, nullptr, width - u * 1.2f);
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

void VariableTable(const char* id, const RuleContext& context, VariableLookup lookup, ImVec2 size, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    ImGui::PushID(id);
    if (ImGui::BeginTable("##variables", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, size)) {
        ImGui::TableSetupColumn(Str(options, "show.variable", "Variable"), ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn(Str(options, "show.type", "Type"), ImGuiTableColumnFlags_WidthFixed, u * 4.5f);
        ImGui::TableSetupColumn(Str(options, "show.value", "Value"), ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(context.variables.size()), u * 1.1f + ImGui::GetStyle().CellPadding.y * 2.f);
        while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                const VariableDef& def = context.variables[static_cast<std::size_t>(i)];
                ImGui::PushID(i);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, u * 1.1f);
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Selectable(Safe(def.name), false, ImGuiSelectableFlags_SpanAllColumns, {0, u * 1.1f});
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
                    ImGui::SetDragDropPayload("IMKIT_VARIABLE", &def.id, sizeof(StableId));
                    ImGui::TextUnformatted(Safe(def.name));
                    ImGui::EndDragDropSource();
                }
                SemanticItem(options, Safe(def.name), accessibility::SemanticRole::Row, accessibility::SemanticAction::Select);
                ImGui::TableSetColumnIndex(1);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%s", TypeWord(def.type));
                ImGui::TableSetColumnIndex(2);
                ImGui::AlignTextToFramePadding();
                RuleValue v;
                if (lookup.read && lookup.read(lookup.user, def.id, v))
                    ImGui::TextColored(c.text, "%s", Format(def.type, v, def.unit, def.choices).c_str());
                else
                    ImGui::TextDisabled("-");
                ImGui::PopID();
            }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void RuleTrace(const char* id, const RuleView& rule, const RuleContext& context, std::span<const ConditionTrace> trace,
               const RuleOutcome& outcome, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    {
        const bool then = outcome.fires;
        const char* word = then                ? Str(options, "show.runsThen", "Runs the Then actions")
                           : rule.hasElse      ? Str(options, "show.runsElse", "Runs the Else actions")
                                               : Str(options, "show.doesNotRun", "Does not run");
        const ImVec2 ts = ImGui::CalcTextSize(word);
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + ts.x + u * 2.f, a.y + u * 1.1f};
        ReadoutItem({b.x - a.x, b.y - a.y});
        const ImVec4 tone = then ? c.success : c.muted;
        draw->AddRectFilled(a, b, U32(Mix(c.input, tone, .2f)), u * .5f);
        draw->AddRect(a, b, U32(tone, .7f), u * .5f, 0, 1.f);
        if (then)
            detail::DrawCheck(draw, {a.x + u * .7f, (a.y + b.y) * .5f}, u * .45f, U32(tone), 1.8f);
        else
            detail::DrawCross(draw, {a.x + u * .7f, (a.y + b.y) * .5f}, u * .4f, U32(tone), 1.6f);
        draw->AddText({a.x + u * 1.2f, a.y + (u * 1.1f - ts.y) * .5f}, U32(c.text), word);
        AnnotateText(options, Str(options, "show.ruleResult", "Rule result"), word, accessibility::SemanticRole::Status);
        if (outcome.missingVariable) {
            ImGui::SameLine(0, u * .6f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(c.warning, "%s", Str(options, "show.missingValue", "Some values were not available and counted as false."));
        }
    }
    ImGui::Spacing();
    for (const ConditionTrace& t : trace) {
        const ConditionNode* node = nullptr;
        for (const ConditionNode& n : rule.conditions)
            if (n.id == t.node)
                node = &n;
        if (!node || node->kind != NodeKind::Compare)
            continue;
        const VariableDef* def = FindVariable(context, node->variable);
        std::string text = def ? def->name : "(missing)";
        text += ' ';
        text += CompareWords(node->op);
        if (def && node->op != Compare::IsTrue && node->op != Compare::IsFalse) {
            text += ' ';
            text += Format(def->type, node->a, def->unit, def->choices);
            if (node->op == Compare::Between) {
                text += " and ";
                text += Format(def->type, node->b, def->unit, def->choices);
            }
        }
        ImGui::PushID(static_cast<int>(t.node));
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ReadoutItem({ImGui::GetContentRegionAvail().x, u * 1.1f});
        const ImVec2 m{a.x + u * .5f, a.y + u * .55f};
        switch (t.result) {
        case Truth::True: detail::DrawCheck(draw, m, u * .45f, U32(c.success), 1.8f); break;
        case Truth::False: detail::DrawCross(draw, m, u * .4f, U32(c.error), 1.6f); break;
        case Truth::Skipped: draw->AddLine({m.x - u * .2f, m.y}, {m.x + u * .2f, m.y}, U32(c.muted), 1.6f); break;
        default: GlyphWarning(draw, m, u * .6f, U32(c.warning)); break;
        }
        draw->AddText({a.x + u * 1.1f, a.y + (u * 1.1f - ImGui::GetTextLineHeight()) * .5f}, U32(t.result == Truth::Skipped ? c.muted : c.text), text.c_str());
        const char* word = t.result == Truth::True ? "true" : t.result == Truth::False ? "false" : t.result == Truth::Skipped ? "skipped" : "no value";
        AnnotateText(options, FrameText("%s", text.c_str()), word, accessibility::SemanticRole::Status);
        ImGui::PopID();
    }
    ImGui::PopID();
}
} // namespace imkit::show
