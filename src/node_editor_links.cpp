#include "node_editor_internal.h"
#include <imkit/widgets.h>
#include "draw_internal.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

namespace imkit::node_editor {
using namespace detail;
void BeginPin(EditorFrame &f, PinId id) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    f.pinOpen = true;
    char key[40];
    std::snprintf(key, sizeof(key), "pin-%llu", static_cast<unsigned long long>(id.value));
    ImGui::PushID(key);
    ImGui::BeginGroup();
    auto *pin = FindPin(f, id);
    if (pin && pin->node == f.node->id && !pin->hidden)
        Text(pin->label);
}
void EndPin(EditorFrame &f) {
    IM_ASSERT(f.pinOpen);
    ImGui::EndGroup();
    ImGui::PopID();
    f.pinOpen = false;
}
bool GetPinPosition(const EditorFrame &f, PinId id, ImVec2 &out) {
    auto *pin = FindPin(f, id);
    if (!pin || !Find(f, pin->node))
        return false;
    out = PinPosition(f, *pin);
    return true;
}
namespace {
bool EmitPin(EditorFrame &f, EditRequest r) {
    if (f.options.readOnly)
        return false;
    auto verdict = ValidatePinEdit(f.graph, r);
    bool queued = verdict.allowed && QueuePinEdits(f.graph, {&r, 1}, *f.requests, f.state->nextOperation++);
    f.state->pinEditReason.fill(0);
    if (!queued) {
        auto reason =
            verdict.allowed ? std::string_view("Request buffer full; retry the edit") : verdict.reason;
        std::copy_n(reason.data(), std::min(reason.size(), f.state->pinEditReason.size() - 1),
                    f.state->pinEditReason.begin());
    }
    return queued;
}
} // namespace
namespace detail {
void ReorderTarget(EditorFrame &f, const PinView &pin, int index) {
    struct Payload {
        GraphId graph;
        NodeId node;
        PinId pin;
        PinKind kind;
        std::uint64_t group;
    };
    if (pin.capabilities.reorder && ImGui::BeginDragDropSource()) {
        Payload payload{f.graph.id, pin.node, pin.id, pin.kind, pin.capabilities.group};
        ImGui::SetDragDropPayload("IMKIT_SOCKET", &payload, sizeof(payload));
        Text(pin.label);
        ImGui::EndDragDropSource();
    }
    if (pin.capabilities.reorder && ImGui::BeginDragDropTarget()) {
        if (auto *payload = ImGui::AcceptDragDropPayload("IMKIT_SOCKET")) {
            if (payload->DataSize == sizeof(Payload)) {
                auto value = *static_cast<const Payload *>(payload->Data);
                if (value.graph == f.graph.id && value.node == pin.node && value.kind == pin.kind &&
                    value.group == pin.capabilities.group) {
                    EditRequest r;
                    r.kind = EditKind::ReorderPin;
                    r.node = pin.node;
                    r.from = value.pin;
                    r.index = index;
                    EmitPin(f, r);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}
void PinActions(EditorFrame &f, const PinView &pin) {
    auto emit = [&](EditRequest r) {
        r.node = pin.node;
        r.from = pin.id;
        return EmitPin(f, r);
    };
    char name[128]{};
    std::copy_n(pin.label.data(), std::min(pin.label.size(), sizeof(name) - 1), name);
    ImGui::BeginDisabled(!pin.capabilities.rename);
    if (ImGui::InputText("Name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue)) {
        EditRequest r;
        r.kind = EditKind::RenamePin;
        std::copy_n(name, sizeof(name), r.text.begin());
        emit(r);
    }
    ImGui::EndDisabled();
    const SocketTypeView *current = nullptr;
    for (auto &t : f.graph.socketTypes)
        if (t.id == pin.type)
            current = &t;
    ImGui::BeginDisabled(!pin.capabilities.changeType);
    if (imkit::BeginCombo("Type", current ? std::string(current->name).c_str() : "Custom")) {
        for (auto &t : f.graph.socketTypes) {
            ImGui::PushID(std::to_string(t.id).c_str());
            if (ImGui::Selectable(std::string(t.name).c_str(), t.id == pin.type)) {
                EditRequest r;
                r.kind = EditKind::ChangePinType;
                r.type = t.id;
                if (f.options.confirmPinImpact && IsPinConnected(f.graph, pin.id)) {
                    f.state->pendingPinEdit = r;
                    f.state->pendingPinEdit.node = pin.node;
                    f.state->pendingPinEdit.from = pin.id;
                } else
                    emit(r);
            }
            ImGui::PopID();
        }
        imkit::EndCombo();
    }
    ImGui::EndDisabled();
    bool shown = !pin.hidden;
    if (imkit::Checkbox("Visible", &shown)) {
        EditRequest r;
        r.kind = EditKind::HidePin;
        r.index = !shown;
        emit(r);
    }
    bool multiple = pin.multiple;
    ImGui::BeginDisabled(!pin.capabilities.changeMultiplicity);
    if (imkit::Checkbox("Multiple connections", &multiple)) {
        EditRequest r;
        r.kind = EditKind::SetPinMultiplicity;
        r.index = multiple;
        emit(r);
    }
    ImGui::EndDisabled();
    if (ImGui::SmallButton("Expose")) {
        EditRequest r;
        r.kind = EditKind::ExposePin;
        emit(r);
    }
    if (pin.capabilities.group) {
        int limits[]{pin.capabilities.minimum, pin.capabilities.maximum};
        if (ImGui::InputInt2("Min / Max", limits, ImGuiInputTextFlags_EnterReturnsTrue)) {
            EditRequest r;
            r.kind = EditKind::SetPinLimits;
            r.minimum = limits[0];
            r.maximum = limits[1];
            emit(r);
        }
    }
    EditRequest remove;
    remove.kind = EditKind::DeletePin;
    remove.node = pin.node;
    remove.from = pin.id;
    auto verdict = ValidatePinEdit(f.graph, remove);
    ImGui::BeginDisabled(!verdict.allowed);
    if (ImGui::SmallButton("Delete socket")) {
        if (f.options.confirmPinImpact && IsPinConnected(f.graph, pin.id))
            f.state->pendingPinEdit = remove;
        else
            emit(remove);
    }
    ImGui::EndDisabled();
    if (!verdict.allowed)
        ImGui::TextWrapped("%.*s", int(verdict.reason.size()), verdict.reason.data());
    if (f.state->pendingPinEdit.from == pin.id) {
        auto count = std::count_if(f.graph.links.begin(), f.graph.links.end(),
                                   [&](auto &l) { return l.from == pin.id || l.to == pin.id; });
        ImGui::TextWrapped("Affects %d link(s). The host may reject this change.", int(count));
        if (ImGui::SmallButton("Confirm")) {
            emit(f.state->pendingPinEdit);
            f.state->pendingPinEdit = {};
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cancel"))
            f.state->pendingPinEdit = {};
    }
    if (!pin.capabilities.reason.empty())
        ImGui::TextWrapped("%.*s", int(pin.capabilities.reason.size()), pin.capabilities.reason.data());
}
} // namespace detail
void PinAddRow(EditorFrame &f, NodeId node, PinKind kind, std::uint64_t group) {
    auto *n = Find(f, node);
    if (!n)
        return;
    auto caps = kind == PinKind::Input ? n->inputs : n->outputs;
    if (!caps.add)
        return;
    int count = 0;
    for (auto &p : f.graph.pins)
        if (p.node == node && p.kind == kind && p.capabilities.group == group)
            ++count;
    ImGui::PushID(std::to_string(node.value).c_str());
    ImGui::PushID(int(kind));
    ImGui::PushID(std::to_string(group).c_str());
    ImGui::BeginDisabled(f.options.readOnly || count >= caps.maximum || n->locked);
    if (ImGui::SmallButton(kind == PinKind::Input ? "+ Input" : "+ Output"))
        ImGui::OpenPopup("Socket type");
    if (ImGui::BeginPopup("Socket type")) {
        for (auto &type : f.graph.socketTypes) {
            ImGui::PushID(std::to_string(type.id).c_str());
            if (ImGui::Selectable(std::string(type.name).c_str())) {
                EditRequest r;
                r.kind = EditKind::CreatePin;
                r.node = node;
                r.pinKind = kind;
                r.index = count;
                r.type = type.id;
                r.group = group;
                std::copy_n(type.name.data(), std::min(type.name.size(), r.text.size() - 1), r.text.begin());
                EmitPin(f, r);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::EndDisabled();
    ImGui::PopID();
    ImGui::PopID();
    ImGui::PopID();
}
void PinRow(EditorFrame &f, PinId id, PinRowOptions options) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    auto *pin = FindPin(f, id);
    if (!pin || pin->node != f.node->id || Concealed(f, *pin))
        return;
    auto &s = *f.state;
    ImGui::PushID(std::to_string(id.value).c_str());
    ImGui::BeginGroup();
    auto start = ImGui::GetCursorScreenPos();
    float rowHeight = std::max(ImGui::GetFrameHeight(), f.style.headerHeight * float(s.zoom) * .7f);
    const bool detail = s.zoom >= Style(f, *f.node).detailZoom;
    bool connected = IsPinConnected(f.graph, id);
    bool showValue = detail && pin->kind == PinKind::Input && options.value.kind != ValueKind::None &&
                     (!connected || !options.hideConnectedValue);
    float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    float labelWidth = showValue ? width * .43f : width;
    ImGui::Selectable("##label", false, ImGuiSelectableFlags_None, {labelWidth, rowHeight});
    auto *dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(start, {start.x + labelWidth, start.y + rowHeight}, true);
    auto label = std::string(pin->label);
    if (pin->capabilities.required)
        label += " *";
    float x = pin->kind == PinKind::Output
                  ? std::max(start.x, start.x + labelWidth - ImGui::CalcTextSize(label.c_str()).x)
                  : start.x;
    dl->AddText({x, start.y + (rowHeight - ImGui::GetFontSize()) * .5f}, Color(f.style.text), label.c_str());
    dl->PopClipRect();
    if (ImGui::IsItemHovered()) {
        for (auto &type : f.graph.socketTypes)
            if (type.id == pin->type)
                ImGui::SetTooltip("%.*s (%.*s)", int(pin->label.size()), pin->label.data(),
                                  int(type.name.size()), type.name.data());
    }
    int index = 0;
    for (auto &p : f.graph.pins) {
        if (p.id == id)
            break;
        if (p.node == pin->node && p.kind == pin->kind && p.capabilities.group == pin->capabilities.group)
            ++index;
    }
    ReorderTarget(f, *pin, index);
    if (ImGui::BeginPopupContextItem("Socket")) {
        PinActions(f, *pin);
        ImGui::EndPopup();
    }
    if (showValue) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(1.f, ImGui::GetContentRegionAvail().x));
        bool editable = !connected && !f.options.readOnly && !Locked(f, *f.node) && !s.valueTerminalPending;
        ImGui::BeginDisabled(!editable);
        auto value = s.valueEditing && s.valueEdit.from == id ? s.valueEdit.value : options.value;
        bool changed = false;
        Phase customPhase = Phase::Update;
        switch (value.kind) {
        case ValueKind::Color:
            changed = ImGui::ColorEdit4("##value", value.number.data(), ImGuiColorEditFlags_NoInputs);
            break;
        case ValueKind::Float:
            changed = imkit::SliderFloat("##value", value.number.data(), options.minimum, options.maximum);
            break;
        case ValueKind::Vector: {
            auto padding = ImGui::GetStyle().FramePadding;
            padding.x = std::min(padding.x, 2.f * float(s.zoom));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
            changed = ImGui::DragFloat3("##value", value.number.data(), .01f, 0, 0, "%.2g");
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("X: %.6g  Y: %.6g  Z: %.6g", value.number[0], value.number[1],
                                  value.number[2]);
            break;
        }
        case ValueKind::Integer:
            changed = ImGui::InputInt("##value", &value.integer);
            break;
        case ValueKind::Boolean:
            changed = imkit::Checkbox("##value", &value.boolean);
            break;
        case ValueKind::Text:
            changed = ImGui::InputText("##value", value.text.data(), value.text.size());
            break;
        case ValueKind::Enum:
            if (imkit::BeginCombo("##value", value.integer >= 0 && value.integer < int(options.choices.size())
                                                 ? std::string(options.choices[value.integer]).c_str()
                                                 : "Choose")) {
                for (int i = 0; i < int(options.choices.size()); ++i)
                    if (ImGui::Selectable(std::string(options.choices[i]).c_str(), value.integer == i)) {
                        value.integer = i;
                        changed = true;
                        customPhase = Phase::Commit;
                    }
                imkit::EndCombo();
            }
            break;
        case ValueKind::Custom:
            if (options.draw)
                changed = options.draw(options.user, value, customPhase);
            break;
        default:
            break;
        }
        bool activated = ImGui::IsItemActivated(), active = ImGui::IsItemActive(),
             deactivated = ImGui::IsItemDeactivated();
        ImGui::EndDisabled();
        if (editable && !s.valueEditing &&
            (activated || changed || (value.kind == ValueKind::Custom && customPhase == Phase::Begin))) {
            EditRequest r;
            r.kind = EditKind::SetPinValue;
            r.node = pin->node;
            r.from = id;
            r.value = options.value;
            r.previousValue = options.value;
            r.graph = f.graph.id;
            r.revision = f.graph.revision;
            r.operation = s.nextOperation++;
            r.phase = Phase::Begin;
            if (f.requests->Push(r)) {
                s.valueEditing = true;
                s.valueEdit = r;
            }
        }
        if (s.valueEditing && s.valueEdit.from == id && !s.valueTerminalPending) {
            if (changed) {
                auto r = s.valueEdit;
                r.value = value;
                r.phase = Phase::Update;
                f.requests->Push(r);
                s.valueEdit = r;
            }
            bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || customPhase == Phase::Cancel;
            if (cancel || (value.kind == ValueKind::Custom
                               ? customPhase == Phase::Commit
                               : deactivated || (changed && (!active || customPhase == Phase::Commit)))) {
                s.valueEdit.phase = cancel ? Phase::Cancel : Phase::Commit;
                if (f.requests->Push(s.valueEdit))
                    s.valueEditing = false;
                else
                    s.valueTerminalPending = true;
            }
        }
    }
    ImGui::EndGroup();
    auto local = World(f, {start.x, start.y + rowHeight * .5f});
    auto origin = Position(f, *f.node);
    auto row =
        std::find_if(s.rowPositions.begin(), s.rowPositions.end(), [&](auto &r) { return r.pin == id; });
    if (row == s.rowPositions.end())
        s.rowPositions.push_back({id, {local.x - origin.x, local.y - origin.y}});
    else
        row->local = {local.x - origin.x, local.y - origin.y};
    ImGui::PopID();
}
bool BeginNodePanel(EditorFrame &f, std::uint64_t panel, std::string_view label, bool *open) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    auto &s = *f.state;
    const auto &st = Style(f, *f.node);
    PanelState *state = nullptr;
    for (auto &p : s.panels)
        if (p.node == f.node->id && p.panel == panel)
            state = &p;
    if (!state) {
        s.panels.push_back({f.node->id, panel, open ? *open : true, 0});
        state = &s.panels.back();
    }
    if (open)
        state->open = *open;
    char key[48];
    std::snprintf(key, sizeof(key), "panel-%llu", static_cast<unsigned long long>(panel));
    ImGui::PushID(key);
    const auto start = ImGui::GetCursorScreenPos();
    const float z = float(s.zoom);
    const float rowHeight = std::max(ImGui::GetFrameHeight(), st.headerHeight * z * .7f);
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    if (ImGui::InvisibleButton("##title", {width, rowHeight})) {
        state->open = !state->open;
        if (open)
            *open = state->open;
    }
    const bool hovered = ImGui::IsItemHovered();
    auto *dl = ImGui::GetWindowDrawList();
    const float cy = start.y + rowHeight * .5f;
    dl->AddLine({start.x, start.y}, {start.x + width, start.y}, Color(WithAlpha(st.border, .7f)));
    const float k = ImGui::GetFontSize() * .26f;
    const ImU32 ink = Color(hovered ? st.text : st.muted);
    if (state->open)
        dl->AddTriangleFilled({start.x, cy - k * .6f}, {start.x + 2 * k, cy - k * .6f}, {start.x + k, cy + k * .9f}, ink);
    else
        dl->AddTriangleFilled({start.x + k * .3f, cy - k}, {start.x + k * 1.8f, cy}, {start.x + k * .3f, cy + k}, ink);
    const std::string text(label);
    dl->AddText({start.x + 3 * k, cy - ImGui::GetFontSize() * .5f}, Color(st.text), text.c_str());
    if (!state->open) { // How many sockets wait on the closed row.
        int count = 0;
        for (auto &pin : f.graph.pins)
            count += pin.node == f.node->id && pin.panel == panel && !Concealed(f, pin);
        if (count) {
            char number[16];
            std::snprintf(number, sizeof(number), "%d", count);
            const auto size = ImGui::CalcTextSize(number);
            dl->AddText({start.x + width - size.x, cy - size.y * .5f}, Color(st.muted), number);
        }
    }
    state->row = World(f, {start.x, cy}).y - Position(f, *f.node).y;
    if (!state->open)
        ImGui::PopID();
    return state->open;
}
void EndNodePanel(EditorFrame &f) {
    IM_ASSERT(f.nodeOpen && !f.pinOpen);
    ImGui::PopID();
}
void Link(EditorFrame &f, const LinkView &link) {
    if (!f.visible)
        return;
    auto *a = FindPin(f, link.from);
    auto *b = FindPin(f, link.to);
    if (!a || !b || a->hidden || b->hidden)
        return;
    if (!Find(f, a->node) || !Find(f, b->node))
        return;
    auto points = LinkCurve(f, link, *a, *b);
    // The stored direction may be either way round; colour sources speak of the output and the input.
    const PinView &output = a->kind == PinKind::Output ? *a : *b;
    const PinView &input = a->kind == PinKind::Output ? *b : *a;
    auto &s = *f.state;
    ImVec4 from = f.style.accent, to = f.style.accent;
    if (link.color.w > 0)
        from = to = link.color;
    else
        switch (f.style.linkColor) {
        case LinkColorSource::OutputType:
            if (HasTypeColor(f, output))
                from = to = PinColor(f, output);
            break;
        case LinkColorSource::InputType:
            if (HasTypeColor(f, input))
                from = to = PinColor(f, input);
            break;
        case LinkColorSource::Gradient:
            if (HasTypeColor(f, output))
                from = PinColor(f, output);
            to = HasTypeColor(f, input) ? PinColor(f, input) : from;
            if (a->kind != PinKind::Output)
                std::swap(from, to);
            break;
        case LinkColorSource::Accent:
            break;
        }
    const bool flow = output.role == PinRole::Flow;
    if (flow && link.color.w <= 0) // Execution order reads apart from data: the text colour, thicker.
        from = to = f.style.text;
    const bool selected = s.selectedLink == link.id;
    const bool invalid = link.validity == LinkValidity::Invalid;
    if (selected)
        from = to = f.style.selected;
    else if (invalid)
        from = to = f.style.error;
    auto *dl = ImGui::GetWindowDrawList();
    const float linkWidth = link.width > 0 ? link.width : flow ? f.style.flowLinkWidth : f.style.linkWidth;
    bool emphasized = selected;
    if (f.style.highlightConnectedLinks) {
        const auto *na = Find(f, a->node), *nb = Find(f, b->node);
        emphasized = emphasized || IsSelected(s, na->id) || IsSelected(s, nb->id) || f.hoveredNode == na->id ||
                     f.hoveredNode == nb->id;
        if (!emphasized && f.hovered && !s.connecting && !f.hoveredNode) {
            const auto mouse = ImGui::GetIO().MousePos;
            for (std::size_t i = 1; i < points.size() && !emphasized; ++i)
                emphasized = SegmentDistance(mouse, points[i - 1], points[i]) < 6;
        }
        if (emphasized) // A soft halo under the line keeps the hue and reads in light and dark themes.
            dl->AddPolyline(points.data(), static_cast<int>(points.size()), Color(WithAlpha(Mix(from, to, .5f), .3f)),
                            ImDrawFlags_None, linkWidth + 5);
    }
    const float width = emphasized && f.style.highlightConnectedLinks ? linkWidth + 1 : linkWidth;
    LinkChromeView chrome;
    chrome.link = &link;
    chrome.points = points;
    chrome.from = from;
    chrome.to = to;
    chrome.width = width;
    chrome.hovered = emphasized && !selected;
    chrome.selected = selected;
    chrome.flow = flow;
    const bool hooked = f.style.hooks && f.style.hooks->link && f.style.hooks->link(dl, chrome, f.style.hooks->user);
    const int start = dl->VtxBuffer.Size;
    if (hooked) { // The host drew the line; markers and labels below still apply.
    } else if (invalid) // Long dashes: the broken state reads from the line itself, not only from its colour.
        DashedPolyline(dl, points, Color(from), width, 2, 1);
    else if (!link.reference) {
        // One polyline keeps joins smooth; per-segment lines show notches at the joints.
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), Color(from), ImDrawFlags_None, width);
    } else
        DashedPolyline(dl, points, Color(from), width, 1, 1);
    const bool gradient = !hooked && (from.x != to.x || from.y != to.y || from.z != to.z || from.w != to.w);
    if (gradient) { // Shade along the dominant axis between the two ends.
        const auto p = points.front(), q = points.back();
        const float length = std::max(1.f, std::abs(q.x - p.x));
        for (int i = start; i < dl->VtxBuffer.Size; ++i) {
            auto &v = dl->VtxBuffer[i];
            const float t = std::clamp((v.pos.x - p.x) / (q.x - p.x == 0 ? length : q.x - p.x), 0.f, 1.f);
            auto c = Mix(from, to, t);
            c.w *= ImGui::ColorConvertU32ToFloat4(v.col).w / std::max(.001f, from.w);
            v.col = Color(c);
        }
    }
    float curveLength = 0;
    for (std::size_t i = 1; i < points.size(); ++i)
        curveLength += std::sqrt(Dot(Sub(points[i], points[i - 1]), Sub(points[i], points[i - 1])));
    // Chevrons cut into the line show the direction of execution; a short link keeps only the middle one.
    const std::size_t marks[] = {12, 7, 17};
    const std::size_t markCount = curveLength > 160 ? 3 : 1;
    if (flow && !invalid && !link.reference)
        for (std::size_t m = 0; m < markCount; ++m) {
            const std::size_t i = marks[m];
            const bool forward = a->kind == PinKind::Output;
            auto d = forward ? Sub(points[i + 1], points[i - 1]) : Sub(points[i - 1], points[i + 1]);
            const float len = std::sqrt(Dot(d, d));
            if (len <= 0)
                continue;
            d = Mul(d, 1 / len);
            const ImVec2 n{-d.y, d.x};
            const float k = std::max(2.f, width * .8f);
            const auto c = points[i];
            dl->AddPolyline(std::array<ImVec2, 3>{Add(Sub(c, Mul(d, k * .6f)), Mul(n, k)), Add(c, Mul(d, k * .6f)),
                                                  Sub(Sub(c, Mul(d, k * .6f)), Mul(n, k))}
                                .data(),
                            3, Color(f.style.canvas), ImDrawFlags_None, std::max(1.2f, width * .45f));
        }
    if (f.style.linkArrow != LinkArrow::None && !flow) {
        const bool atEnd = f.style.linkArrow == LinkArrow::End;
        // Arrows point from output to input whatever order the host stored the ends in.
        const bool forward = a->kind == PinKind::Output;
        std::size_t i = atEnd ? (forward ? 23 : 1) : 18;
        auto tip = points[i];
        auto direction = forward ? Sub(points[i], points[i - 1]) : Sub(points[i - 1], points[i]);
        const auto length = std::sqrt(Dot(direction, direction));
        if (length > 0) {
            direction = Mul(direction, 1 / length);
            if (atEnd) // Stop short of the socket so the arrow does not cover it.
                tip = Sub(forward ? points.back() : points.front(),
                          Mul(direction, f.style.pinRadius * std::clamp(float(s.zoom), .8f, 2.f) + 3));
            auto normal = ImVec2{-direction.y, direction.x};
            const auto color = Color(atEnd ? (forward ? to : from) : Mix(from, to, .75f));
            dl->AddTriangleFilled(tip, Add(Sub(tip, Mul(direction, 7)), Mul(normal, 3)),
                                  Sub(Sub(tip, Mul(direction, 7)), Mul(normal, 3)), color);
        }
    }
    const bool bead = link.validity != LinkValidity::Valid;
    if (bead) {
        DrawBead(dl, points[12], link.validity, f.style, s.zoom);
        const auto mouse = ImGui::GetIO().MousePos;
        const float r = BeadRadius(s.zoom) + 2;
        const bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        if (f.hovered && !popup && !s.connecting && Inside(mouse, Sub(points[12], {r, r}), Add(points[12], {r, r}))) {
            const char *fallback = link.validity == LinkValidity::Invalid  ? "Invalid link"
                                   : link.validity == LinkValidity::Cycle ? "Creates a cycle"
                                                                          : "Converted";
            const char *key = link.validity == LinkValidity::Invalid  ? "node_editor.link.invalid"
                              : link.validity == LinkValidity::Cycle ? "node_editor.link.cycle"
                                                                     : "node_editor.link.converted";
            if (link.reason.empty())
                ImGui::SetTooltip("%s", Tr(f, key, fallback));
            else
                ImGui::SetTooltip("%s: %.*s", Tr(f, key, fallback), int(link.reason.size()), link.reason.data());
        }
    }
    if (!link.label.empty()) { // A pill keeps the label legible over grid and other links.
        const auto size = ImGui::CalcTextSize(link.label.data(), link.label.data() + link.label.size());
        const auto c = points[bead ? 7 : 12];
        const ImVec2 pad{6, 2};
        const ImVec2 lo{c.x - size.x / 2 - pad.x, c.y - size.y / 2 - pad.y};
        const ImVec2 hi{c.x + size.x / 2 + pad.x, c.y + size.y / 2 + pad.y};
        dl->AddRectFilled(lo, hi, Color(f.style.surface), (hi.y - lo.y) / 2);
        dl->AddRect(lo, hi, Color(WithAlpha(Mix(from, to, .5f), .6f)), (hi.y - lo.y) / 2);
        dl->AddText(Add(lo, pad), Color(f.style.muted), link.label.data(), link.label.data() + link.label.size());
    }
    if (link.active) { // Marks travel from output to input; they hold still when motion is off.
        std::array<float, 25> along{};
        for (std::size_t i = 1; i < points.size(); ++i) {
            const auto d = Sub(points[i], points[i - 1]);
            along[i] = along[i - 1] + std::sqrt(Dot(d, d));
        }
        const float spacing = std::max(4.f, f.style.flowSpacing);
        const float travelled = f.style.animate ? float(ImGui::GetTime()) * 60 * f.style.flowSpeed : 0;
        const bool forward = a->kind == PinKind::Output;
        const ImU32 mark = Color(flow ? f.style.accent : f.style.success);
        const float radius = std::max(1.5f, width * .65f);
        std::size_t segment = 1;
        for (float d = std::fmod(travelled, spacing); d < along.back(); d += spacing) {
            const float at = forward ? d : along.back() - d;
            while (segment + 1 < points.size() && along[segment] < at)
                ++segment;
            while (segment > 1 && along[segment - 1] > at)
                --segment;
            const float span = std::max(.001f, along[segment] - along[segment - 1]);
            const float t = std::clamp((at - along[segment - 1]) / span, 0.f, 1.f);
            dl->AddCircleFilled(Add(points[segment - 1], Mul(Sub(points[segment], points[segment - 1]), t)), radius,
                                mark);
        }
    }
}
void DrawLinks(EditorFrame &f) {
    f.linksQueued = true; // EndEditor draws after standard row positions are known.
}
} // namespace imkit::node_editor
