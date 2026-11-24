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
namespace {
// The node's input and output that can take the two halves of `link`.
bool FindInsertion(const EditorFrame &f, NodeId node, LinkId id, PinId &in, PinId &out) {
    auto it =
        std::find_if(f.graph.links.begin(), f.graph.links.end(), [&](auto &link) { return link.id == id; });
    if (it == f.graph.links.end())
        return false;
    auto *source = FindPin(f, it->from);
    auto *target = FindPin(f, it->to);
    if (!source || !target || source->node == node || target->node == node)
        return false;
    for (auto &input : f.graph.pins)
        if (input.node == node && input.kind == PinKind::Input &&
            CanConnect(f.graph, source->id, input.id, id).allowed)
            for (auto &output : f.graph.pins)
                if (output.node == node && output.kind == PinKind::Output &&
                    CanConnect(f.graph, output.id, target->id, id).allowed) {
                    in = input.id;
                    out = output.id;
                    return true;
                }
    return false;
}
} // namespace
bool InsertNode(EditorFrame &f, NodeId node, LinkId id) {
    auto *n = Find(f, node);
    if (!n || Locked(f, *n) || f.options.readOnly)
        return false;
    PinId in{}, out{};
    if (!FindInsertion(f, node, id, in, out))
        return false;
    auto r = Request(f, EditKind::InsertNode, node);
    r.link = id;
    r.from = in;
    r.to = out;
    return f.requests->Push(r);
}
bool ConnectSelected(EditorFrame &f) {
    auto &sel = f.state->selection;
    if (f.options.readOnly || sel.size() != 2)
        return false;
    auto *a = Find(f, sel[0]), *b = Find(f, sel[1]);
    if (!a || !b)
        return false;
    if (Position(f, *b).x < Position(f, *a).x)
        std::swap(a, b);
    for (auto &output : f.graph.pins)
        if (output.node == a->id && output.kind == PinKind::Output && !output.hidden)
            for (auto &input : f.graph.pins)
                if (input.node == b->id && input.kind == PinKind::Input && !input.hidden &&
                    !IsPinConnected(f.graph, input.id) && CanConnect(f.graph, output.id, input.id).allowed) {
                    auto r = Request(f, EditKind::CreateLink);
                    r.from = output.id;
                    r.to = input.id;
                    return f.requests->Push(r);
                }
    return false;
}
void LayoutToolbar(EditorFrame &f) {
    ImGui::BeginDisabled(f.options.readOnly || f.state->selection.empty());
    if (ImGui::Button("Arrange"))
        ImGui::OpenPopup("Layout actions");
    if (ImGui::BeginPopup("Layout actions")) {
        const char *labels[] = {"Align left",
                                "Align center X",
                                "Align right",
                                "Align top",
                                "Align center Y",
                                "Align bottom",
                                "Distribute horizontally",
                                "Distribute vertically",
                                "Arrange by connections",
                                "Snap to grid"};
        for (int i = 0; i < 10; ++i)
            if (ImGui::Selectable(labels[i]))
                ApplyLayout(f, i);
        ImGui::EndPopup();
    }
    ImGui::EndDisabled();
}
void BackgroundImage(EditorFrame &f, ImTextureRef texture, Rect bounds, ImVec4 tint) {
    if (!f.visible || texture.GetTexID() == ImTextureID{})
        return;
    ImGui::GetWindowDrawList()->AddImage(texture, Screen(f, bounds.min), Screen(f, bounds.max), {0, 0},
                                         {1, 1}, Color(tint));
}
void Annotation(EditorFrame &f, std::span<const Point> points, ImVec4 color, float width) {
    if (!f.visible || width <= 0 || !std::isfinite(width))
        return;
    for (std::size_t i = 1; i < points.size(); ++i)
        ImGui::GetWindowDrawList()->AddLine(Screen(f, points[i - 1]), Screen(f, points[i]), Color(color),
                                            width);
}
void NodeSearch(EditorFrame &f) {
    auto &s = *f.state;
    float button = ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(
        std::max(1.f, ImGui::GetContentRegionAvail().x - button - ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##node-search", "Find in graph", s.search.data(), s.search.size());
    ImGui::SameLine();
    if (ImGui::Button("...##navigation", {button, button}))
        ImGui::OpenPopup("Navigation");
    if (ImGui::BeginPopup("Navigation")) {
        if (ImGui::MenuItem("Back", "", false, s.historyIndex > 0))
            NavigateHistory(s, -1);
        if (ImGui::MenuItem("Forward", "", false, s.historyIndex + 1 < s.history.size()))
            NavigateHistory(s, 1);
        ImGui::Separator();
        for (bool upstream : {true, false})
            if (ImGui::MenuItem(upstream ? "Select upstream" : "Select downstream", nullptr, false,
                                bool(s.active))) {
                std::vector<NodeId> nodes(f.graph.nodes.size());
                auto result = TraceNodes(f.graph, s.active, upstream, nodes);
                if (result)
                    s.selection.assign(nodes.begin(), nodes.begin() + result.count);
            }
        if (ImGui::MenuItem("Clear search", nullptr, false, s.search[0] != 0))
            s.search.fill(0);
        ImGui::EndPopup();
    }
    if (!s.search[0])
        return;
    ImGui::BeginChild("##search-results", {0, ImGui::GetTextLineHeightWithSpacing() * 6},
                      ImGuiChildFlags_Borders);
    bool found = false;
    auto query = std::string_view(s.search.data());
    for (auto &n : f.graph.nodes) {
        auto match = [&](std::string_view text) {
            return std::search(text.begin(), text.end(), query.begin(), query.end(),
                               [](unsigned char a, unsigned char b) {
                                   return std::tolower(a) == std::tolower(b);
                               }) != text.end();
        };
        if (!match(n.title) && !match(n.description))
            continue;
        found = true;
        ImGui::PushID(std::to_string(n.id.value).c_str());
        if (ImGui::Selectable(std::string(n.title).c_str(), IsSelected(s, n.id))) {
            RememberView(s);
            Select(s, n.id);
            FrameNodes(s, f.graph, {&n.id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            RememberView(s);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", int(n.title.size()), n.title.data());
        ImGui::PopID();
    }
    if (!found)
        ImGui::TextDisabled("No matching nodes");
    ImGui::EndChild();
}
void NodeInspector(EditorFrame &f, NodeId id) {
    auto *n = Find(f, id);
    if (!n) {
        ImGui::TextWrapped(
            "Select a node to edit its inputs and outputs. Use Tab on the canvas to add a node.");
        return;
    }
    if (f.state->pinEditReason[0])
        ImGui::TextWrapped("%s", f.state->pinEditReason.data());
    ImGui::PushID(std::to_string(id.value).c_str());
    char title[128]{};
    std::copy_n(n->title.data(), std::min(n->title.size(), sizeof(title) - 1), title);
    ImGui::BeginDisabled(f.options.readOnly || Locked(f, *n));
    const bool rename = ImGui::InputText("Name", title, sizeof(title), ImGuiInputTextFlags_EnterReturnsTrue);
    if ((rename || ImGui::IsItemDeactivatedAfterEdit()) && std::string_view(title) != n->title) {
        auto r = Request(f, EditKind::Rename, id);
        std::copy(std::begin(title), std::end(title), r.text.begin());
        f.requests->Push(r);
    }
    if (ImGui::SmallButton("Collapse / expand"))
        QueueCommand(f, EditKind::Collapse, id);
    ImGui::SameLine();
    if (ImGui::SmallButton("Bypass"))
        QueueCommand(f, EditKind::Bypass, id);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(f.options.readOnly);
    if (ImGui::SmallButton(n->locked ? "Unlock" : "Lock"))
        QueueCommand(f, EditKind::Lock, id);
    if (n->kind == NodeKind::Group || n->kind == NodeKind::Frame) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit contents")) {
            Rect bounds;
            if (FitGroupToContents(f.graph, id, bounds, f.style.padding)) {
                auto a = Request(f, EditKind::Move, id);
                auto b = Request(f, EditKind::Resize, id);
                a.before = n->position;
                a.after = bounds.min;
                b.before = n->size;
                b.after = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y};
                b.operation = a.operation;
                a.operationSize = b.operationSize = 2;
                EditRequest requests[] = {a, b};
                f.requests->PushBatch(requests);
            }
        }
    }
    ImGui::BeginDisabled(Locked(f, *n));
    for (auto kind : {PinKind::Input, PinKind::Output}) {
        ImGui::SeparatorText(kind == PinKind::Input ? "Inputs" : "Outputs");
        PinAddRow(f, id, kind, (kind == PinKind::Input ? n->inputs : n->outputs).group);
        int index = 0;
        for (auto &pin : f.graph.pins)
            if (pin.node == id && pin.kind == kind) {
                ImGui::PushID(std::to_string(pin.id.value).c_str());
                bool open = ImGui::TreeNodeEx("socket", ImGuiTreeNodeFlags_None, "%.*s",
                                              int(pin.label.size()), pin.label.data());
                int groupIndex = 0;
                for (auto &previous : f.graph.pins) {
                    if (previous.id == pin.id)
                        break;
                    if (previous.node == id && previous.kind == kind &&
                        previous.capabilities.group == pin.capabilities.group)
                        ++groupIndex;
                }
                ReorderTarget(f, pin, groupIndex);
                ++index;
                if (open) {
                    PinActions(f, pin);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        if (!index)
            ImGui::TextDisabled("No sockets. Add one above if allowed by the host.");
    }
    ImGui::EndDisabled();
    if (f.options.canDebug) {
        if (ImGui::SmallButton(n->breakpoint ? "Remove breakpoint" : "Add breakpoint"))
            QueueCommand(f, EditKind::Breakpoint, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Watch value"))
            QueueCommand(f, EditKind::Watch, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Step"))
            QueueCommand(f, EditKind::Step, id);
    }
    if (f.options.canRun) {
        if (ImGui::SmallButton("Run node"))
            QueueCommand(f, EditKind::Run, id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Stop"))
            QueueCommand(f, EditKind::Stop);
    }
    ImGui::EndDisabled();
    ImGui::PopID();
}
void ExposedProperties(EditorFrame &f, NodeId node, std::span<const PropertyView> properties) {
    ImGui::BeginDisabled(f.options.readOnly);
    for (std::size_t i = 0; i < properties.size(); ++i) {
        auto &property = properties[i];
        ImGui::PushID(int(i));
        bool exposed = property.exposed;
        if (imkit::Checkbox(std::string(property.label).c_str(), &exposed)) {
            auto r = Request(f, EditKind::ExposeProperty, node);
            r.type = property.id;
            r.index = exposed ? 1 : 0;
            f.requests->Push(r);
        }
        auto reorder = [&](int index) {
            auto r = Request(f, EditKind::ReorderProperty, node);
            r.type = property.id;
            r.index = index;
            f.requests->Push(r);
        };
        ImGui::SameLine();
        ImGui::BeginDisabled(i == 0);
        if (ImGui::SmallButton("Up"))
            reorder(int(i) - 1);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(i + 1 == properties.size());
        if (ImGui::SmallButton("Down"))
            reorder(int(i) + 1);
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::EndDisabled();
}
void NodePalette(EditorFrame &f, std::span<const PaletteEntry> entries) {
    if (f.state->palette) {
        ImGui::OpenPopup("Add node");
        f.state->palette = false;
    }
    if (ImGui::BeginPopup("Add node")) {
        f.state->paletteWasOpen = true;
        f.blocked = true;
        ImGui::SetNextItemWidth(280);
        ImGui::InputTextWithHint("##search", "Search nodes", f.state->search.data(), f.state->search.size());
        std::vector<const PaletteEntry *> ordered;
        for (auto &e : entries)
            ordered.push_back(&e);
        auto rank = [&](std::uint64_t type) {
            if (std::find(f.state->favorites.begin(), f.state->favorites.end(), type) !=
                f.state->favorites.end())
                return std::size_t(0);
            auto i = std::find(f.state->recent.begin(), f.state->recent.end(), type);
            return i == f.state->recent.end() ? std::size_t(100)
                                              : std::size_t(i - f.state->recent.begin() + 1);
        };
        std::stable_sort(ordered.begin(), ordered.end(),
                         [&](auto *a, auto *b) { return rank(a->type) < rank(b->type); });
        for (auto *pointer : ordered) {
            const auto &entry = *pointer;
            auto matches = [&](std::string_view value) {
                auto query = std::string_view(f.state->search.data());
                return std::search(value.begin(), value.end(), query.begin(), query.end(),
                                   [](unsigned char a, unsigned char b) {
                                       return std::tolower(a) == std::tolower(b);
                                   }) != value.end() ||
                       query.empty();
            };
            if (!matches(entry.title) && !matches(entry.category))
                continue;
            if (f.state->connecting &&
                (!entry.compatible || !entry.compatible(entry.user, f.state->connecting)))
                continue;
            char key[40];
            std::snprintf(key, sizeof(key), "type-%llu", static_cast<unsigned long long>(entry.type));
            ImGui::PushID(key);
            bool favorite = std::find(f.state->favorites.begin(), f.state->favorites.end(), entry.type) !=
                            f.state->favorites.end();
            if (ImGui::SmallButton(favorite ? "*" : "+")) {
                if (favorite)
                    std::erase(f.state->favorites, entry.type);
                else
                    f.state->favorites.push_back(entry.type);
            }
            ImGui::SameLine();
            auto title = std::string(entry.category) + " / " + std::string(entry.title);
            ImGui::BeginDisabled(f.options.readOnly);
            if (ImGui::Selectable(title.c_str())) {
                auto r = Request(f, EditKind::CreateNode);
                r.type = entry.type;
                r.after = f.state->press;
                r.from = f.state->connecting;
                r.link = f.state->reconnecting;
                if (f.requests->Push(r)) {
                    std::erase(f.state->recent, entry.type);
                    f.state->recent.insert(f.state->recent.begin(), entry.type);
                    if (f.state->recent.size() > 16)
                        f.state->recent.resize(16);
                    f.state->connecting = {};
                }
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered() && !entry.description.empty())
                ImGui::SetTooltip("%.*s", int(entry.description.size()), entry.description.data());
            ImGui::PopID();
        }
        ImGui::EndPopup();
    } else if (f.state->paletteWasOpen) {
        f.state->paletteWasOpen = false;
        f.state->connecting = {};
        f.state->reconnecting = {};
    }
}
void MiniMap(EditorFrame &f, ImVec2 size) {
    if (f.graph.nodes.empty() || size.x <= 0 || size.y <= 0)
        return;
    const float margin = f.style.padding;
    size.x = std::min(size.x, f.max.x - f.min.x - 2 * margin);
    size.y = std::min(size.y, f.max.y - f.min.y - 2 * margin);
    if (size.x < 24 || size.y < 24)
        return;
    auto a = ImVec2{f.max.x - size.x - margin, f.max.y - size.y - margin}, b = Add(a, size);
    auto cursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::BeginChild("##minimap", size, ImGuiChildFlags_None,
                                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                         ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();
    if (!visible) {
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy({0, 0});
        return;
    }
    ImGui::InvisibleButton("##navigate", size);
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    auto *dl = ImGui::GetWindowDrawList();
    Rect bounds{f.graph.nodes.front().position, f.graph.nodes.front().position};
    for (auto &n : f.graph.nodes) {
        bounds.min = {std::min(bounds.min.x, n.position.x), std::min(bounds.min.y, n.position.y)};
        bounds.max = {std::max(bounds.max.x, n.position.x + n.size.x),
                      std::max(bounds.max.y, n.position.y + n.size.y)};
    }
    double scale = std::min((size.x - 12) / std::max(1., bounds.max.x - bounds.min.x),
                            (size.y - 12) / std::max(1., bounds.max.y - bounds.min.y));
    auto map = [&](Point p) {
        return ImVec2{a.x + 6 + float((p.x - bounds.min.x) * scale),
                      a.y + 6 + float((p.y - bounds.min.y) * scale)};
    };
    dl->AddRectFilled(a, b, Color(f.style.header), f.style.rounding);
    dl->AddRect(a, b, Color(f.style.border), f.style.rounding);
    dl->PushClipRect(a, b, true);
    for (auto &n : f.graph.nodes)
        dl->AddRectFilled(map(n.position), map({n.position.x + n.size.x, n.position.y + n.size.y}),
                          Color(IsSelected(*f.state, n.id) ? f.style.accent : f.style.muted), 1);
    const auto viewMin = map(f.state->origin),
               viewMax = map({f.state->origin.x + (f.max.x - f.min.x) / f.state->zoom,
                              f.state->origin.y + (f.max.y - f.min.y) / f.state->zoom});
    auto viewFill = f.style.accent;
    viewFill.w *= .12f;
    dl->AddRectFilled(viewMin, viewMax, Color(viewFill), 2);
    dl->AddRect(viewMin, viewMax, Color(f.style.accent), 2);
    dl->PopClipRect();
    if (hovered || active) {
        f.blocked = true;
        if (active && ImGui::IsMouseDown(0)) {
            auto p = ImGui::GetIO().MousePos;
            f.state->origin = {
                bounds.min.x + (p.x - a.x - 6) / scale - (f.max.x - f.min.x) / f.state->zoom / 2,
                bounds.min.y + (p.y - a.y - 6) / scale - (f.max.y - f.min.y) / f.state->zoom / 2};
        }
    }
    ImGui::EndChild();
    ImGui::SetCursorScreenPos(cursor);
    ImGui::Dummy({0, 0});
}
void Diagnostics(EditorFrame &f) {
    for (auto &n : f.graph.nodes)
        if (n.status == Status::Error || n.status == Status::Waiting || n.breakpoint) {
            auto label = std::string(n.title) + "##diagnostic" + std::to_string(n.id.value);
            if (ImGui::Selectable(label.c_str(), IsSelected(*f.state, n.id))) {
                Select(*f.state, n.id);
                FrameNodes(*f.state, f.graph, {&n.id, 1}, {f.max.x - f.min.x, f.max.y - f.min.y});
            }
            if (!n.description.empty())
                Text(n.description);
        }
}
void Breadcrumbs(EditorFrame &f, std::span<const PathEntry> path) {
    const float available = ImGui::GetContentRegionAvail().x;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i) {
            if (ImGui::GetContentRegionAvail().x < ImGui::GetFrameHeight() * 3)
                ImGui::NewLine();
            else {
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
            }
        }
        ImGui::PushID(int(i));
        float width =
            std::min(std::max(1.f, available), ImGui::CalcTextSize(std::string(path[i].title).c_str()).x +
                                                   2 * ImGui::GetStyle().FramePadding.x);
        if (ImGui::GetContentRegionAvail().x < width)
            ImGui::NewLine();
        if (ImGui::Button(std::string(path[i].title).c_str(),
                          {std::min(width, std::max(1.f, ImGui::GetContentRegionAvail().x)), 0})) {
            auto r = Request(f, EditKind::EnterGraph, path[i].node);
            r.type = path[i].graph.value;
            f.requests->Push(r);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", int(path[i].title.size()), path[i].title.data());
        ImGui::PopID();
    }
}
void Preview(EditorFrame &f, std::span<const PreviewOutput> outputs,
             void (*demand)(void *, const PreviewDemand &), void *user) {
    IM_ASSERT(f.nodeOpen);
    auto &p = GetPreview(*f.state, f.node->id);
    float z = float(f.state->zoom);
    ImGui::Separator();
    if (ImGui::SmallButton(p.collapsed ? "> Preview" : "v Preview"))
        p.collapsed = !p.collapsed;
    ImGui::SameLine();
    if (ImGui::SmallButton("...##preview-actions"))
        ImGui::OpenPopup("Preview actions");
    if (ImGui::BeginPopup("Preview actions")) {
        if (ImGui::MenuItem("Open"))
            p.expanded = true;
        ImGui::MenuItem("Pin", nullptr, &p.pinned);
        ImGui::MenuItem("Interact", nullptr, &p.interactive);
        ImGui::EndPopup();
    }
    if (p.collapsed || outputs.empty())
        return;
    p.output = std::clamp(p.output, 0, int(outputs.size()) - 1);
    if (outputs.size() > 1) {
        if (imkit::BeginCombo("##preview-output", std::string(outputs[p.output].title).c_str())) {
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.output))
                    p.output = i;
            imkit::EndCombo();
        }
    }
    ImVec2 size{std::max(1.f, ImGui::GetContentRegionAvail().x), std::max(24.f, p.height * z)};
    if (ImGui::BeginChild("##preview", size, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        bool visible = ImGui::IsRectVisible(size);
        if (demand)
            demand(user, {f.node->id, p.output, size, visible, p.interactive});
        if (visible)
            DrawOutput(outputs[p.output], size, p.interactive);
        if (ImGui::IsWindowHovered() && p.interactive)
            f.blocked = true;
    }
    ImGui::EndChild();
    ImGui::InvisibleButton("##preview-size", {size.x, 6});
    if (ImGui::IsItemActive())
        p.height = std::clamp(p.height + ImGui::GetIO().MouseDelta.y / z, 24.f, 800.f);
}
void DrawDetachedPreviews(EditorFrame &f, NodeId node, std::span<const PreviewOutput> outputs,
                          void (*demand)(void *, const PreviewDemand &), void *user) {
    auto &p = GetPreview(*f.state, node);
    if ((!p.expanded && !p.pinned) || outputs.empty())
        return;
    p.output = std::clamp(p.output, 0, int(outputs.size()) - 1);
    auto *n = Find(f, node);
    auto label = std::string(n ? n->title : std::string_view("Preview")) + "###preview-" +
                 std::to_string(f.editorId) + "-" + std::to_string(f.graph.id.value) + "-" +
                 std::to_string(node.value);
    bool open = true;
    ImGui::SetNextWindowSize({500, 380}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(label.c_str(), &open)) {
        imkit::Checkbox("Interact", &p.interactive);
        ImGui::SameLine();
        imkit::Checkbox("Pinned", &p.pinned);
        if (outputs.size() > 1 &&
            imkit::BeginCombo(
                "Compare",
                p.compare < 0
                    ? "None"
                    : std::string(outputs[std::min(p.compare, int(outputs.size()) - 1)].title).c_str())) {
            if (ImGui::Selectable("None", p.compare < 0))
                p.compare = -1;
            for (int i = 0; i < int(outputs.size()); ++i)
                if (ImGui::Selectable(std::string(outputs[i].title).c_str(), i == p.compare))
                    p.compare = i;
            imkit::EndCombo();
        }
        auto size = ImGui::GetContentRegionAvail();
        if (p.compare >= int(outputs.size()))
            p.compare = -1;
        if (p.compare >= 0)
            size.x = (size.x - ImGui::GetStyle().ItemSpacing.x) / 2;
        for (int i = 0; i < (p.compare >= 0 ? 2 : 1); ++i) {
            if (i)
                ImGui::SameLine();
            ImGui::PushID(i);
            int index = i ? p.compare : p.output;
            if (ImGui::BeginChild("output", size)) {
                if (demand)
                    demand(user, {node, index, size, true, p.interactive});
                DrawOutput(outputs[index], size, p.interactive);
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
    }
    ImGui::End();
    if (!open)
        p.expanded = p.pinned = false;
}
void EndEditor(EditorFrame &f) {
    IM_ASSERT(!f.nodeOpen && f.context == ImGui::GetCurrentContext());
    auto &s = *f.state;
    auto &io = ImGui::GetIO();
    if (f.visible) {
        if (f.linksQueued)
            for (auto &link : f.graph.links)
                Link(f, link);
        auto *dl = ImGui::GetWindowDrawList();
        const PinView *hoveredPin = nullptr;
        const PinView *snap = nullptr; // Nearest compatible socket while wiring.
        float snapBest = std::max(0.f, f.style.snapDistance);
        for (auto &pin : f.graph.pins) {
            auto *n = Find(f, pin.node);
            if (!n || Concealed(f, pin) || !Visible(f, *n))
                continue;
            auto p = PinPosition(f, pin);
            const auto &st = Style(f, *n);
            if (n->kind == NodeKind::Reroute) { // The knot itself is the socket.
                // While wiring, the knot accepts the direction that completes the wire; otherwise its ring
                // starts a new wire from the output.
                const float knot = KnotRadius(s.zoom);
                const float d = std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y);
                const auto *from = s.connecting ? FindPin(f, s.connecting) : nullptr;
                const bool wanted = from ? (s.movingLinks ? from->kind == pin.kind && from->id != pin.id
                                                          : from->kind != pin.kind) &&
                                               d <= knot + kKnotRing
                                         : pin.kind == PinKind::Output && d > knot * .85f && d <= knot + kKnotRing;
                if (wanted)
                    hoveredPin = &pin;
                continue;
            }
            float r = st.pinRadius * std::clamp(float(s.zoom), .8f, 2.f);
            ImVec4 tone = PinColor(f, pin);
            auto shape = pin.shape;
            for (auto &type : f.graph.socketTypes)
                if (type.id == pin.type && pin.inheritTypeStyle)
                    shape = type.shape;
            const bool connected = IsPinConnected(f.graph, pin.id);
            if (s.connecting && s.connecting != pin.id) {
                auto verdict = DragVerdict(f, pin.id);
                if (!verdict.allowed) // Sockets that cannot take the wire step back instead of shouting.
                    tone = WithAlpha(tone, std::clamp(st.incompatibleAlpha, 0.f, 1.f));
                else {
                    const bool convertible = verdict.match == ConnectionMatch::Convertible;
                    dl->AddCircle(p, r + 4, Color(convertible ? f.style.warning : f.style.success), 0, 2.f);
                    const float distance = std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y);
                    if (distance <= snapBest) {
                        snapBest = distance;
                        snap = &pin;
                    }
                }
            }
            for (auto &link : f.graph.links) // An input fed by a link the host marked invalid shows it.
                if (link.validity == LinkValidity::Invalid && pin.kind == PinKind::Input &&
                    (link.to == pin.id || link.from == pin.id)) {
                    dl->AddCircle(p, r + 3, Color(f.style.error), 0, 2.f);
                    break;
                }
            const ImU32 c = Color(tone);
            if (pin.multiple && !IsMultiInput(pin))
                dl->AddCircle(p, r + 2, c);
            if (pin.capabilities.group)
                dl->AddLine(Add(p, {-r, -r - 4}), Add(p, {r, -r - 4}), c, 2);
            PinChromeView glyph;
            glyph.pin = &pin;
            glyph.center = p;
            glyph.radius = r;
            glyph.seat = st.pinSeat * std::clamp(float(s.zoom), .8f, 2.f);
            glyph.color = tone;
            glyph.canvas = st.canvas;
            glyph.shape = shape;
            glyph.connected = connected;
            glyph.hollow = st.hollowUnconnectedPins;
            glyph.hovered = Inside(io.MousePos, Sub(p, {r + 5, r + 5}), Add(p, {r + 5, r + 5}));
            if (pin.keyType)
                for (auto &type : f.graph.socketTypes)
                    if (type.id == pin.keyType)
                        glyph.keyColor = type.color;
            if (IsMultiInput(pin))
                for (auto &link : f.graph.links)
                    glyph.links += link.to == pin.id || link.from == pin.id;
            if (!(st.hooks && st.hooks->pin && st.hooks->pin(dl, glyph, st.hooks->user)))
                DrawPinGlyph(dl, glyph);
            // A required input that has nothing connected carries a small error mark.
            if (pin.kind == PinKind::Input && pin.capabilities.required && !connected) {
                const ImVec2 m = Add(p, {r * .85f, -r * .85f});
                dl->AddCircleFilled(m, std::max(2.5f, r * .5f) + 1.2f, Color(st.canvas));
                dl->AddCircleFilled(m, std::max(2.5f, r * .5f), Color(st.error));
            }
            if (Inside(io.MousePos, Sub(p, {r + 5, r + 5}), Add(p, {r + 5, r + 5})))
                hoveredPin = &pin;
        }
        if (s.connecting && !hoveredPin && snap)
            hoveredPin = snap;
        // Dragging one node over a link it could split: the link lights up, and releasing inserts the node.
        s.dropLink = {};
        if (s.gesture.size() == 1 && s.gesture.front().kind == EditKind::Move && !f.options.readOnly) {
            const NodeId moving = s.gesture.front().node;
            for (auto &link : f.graph.links) {
                auto *x = FindPin(f, link.from), *y = FindPin(f, link.to);
                PinId in{}, out{};
                if (!x || !y || x->node == moving || y->node == moving ||
                    !FindInsertion(f, moving, link.id, in, out))
                    continue;
                const auto points = LinkCurve(f, link, *x, *y);
                bool near = false;
                for (std::size_t i = 1; i < points.size() && !near; ++i)
                    near = SegmentDistance(io.MousePos, points[i - 1], points[i]) < 10;
                if (near) {
                    s.dropLink = link.id;
                    dl->AddPolyline(points.data(), int(points.size()), Color(WithAlpha(f.style.accent, .45f)),
                                    ImDrawFlags_None, f.style.linkWidth + 6);
                    break;
                }
            }
        }
        if (f.options.minimap)
            MiniMap(f);
        // Child widgets and popups win over canvas shortcuts and gestures.
        bool widgetActive = ImGui::IsAnyItemActive();
        bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        bool available = f.hovered && !popup && !f.blocked && !widgetActive;
        if (hoveredPin && f.hovered && !popup && !widgetActive) {
            if (!s.connecting) { // Name, type and the host's current value or the reason it is unavailable.
                std::string tip(hoveredPin->label);
                for (auto &type : f.graph.socketTypes)
                    if (type.id == hoveredPin->type && !type.name.empty()) {
                        tip += tip.empty() ? "" : "  \xC2\xB7  ";
                        tip += type.name;
                    }
                if (!hoveredPin->inspect.empty())
                    tip.append("\n").append(hoveredPin->inspect);
                if ((hoveredPin->flags & PinFlags_Unavailable) && !hoveredPin->reason.empty())
                    tip.append("\n").append(hoveredPin->reason);
                if (!tip.empty())
                    ImGui::SetTooltip("%s", tip.c_str());
            }
            const bool linked = IsPinConnected(f.graph, hoveredPin->id);
            if (ImGui::IsMouseClicked(0) && !f.options.readOnly && io.KeyAlt) {
                // Alt-click breaks every link of the socket in one operation.
                std::vector<EditRequest> batch;
                const auto op = s.nextOperation++;
                for (auto &link : f.graph.links)
                    if (link.from == hoveredPin->id || link.to == hoveredPin->id) {
                        auto r = Request(f, EditKind::DeleteLink);
                        r.link = link.id;
                        r.operation = op;
                        batch.push_back(r);
                    }
                for (auto &r : batch)
                    r.operationSize = batch.size();
                if (!batch.empty())
                    f.requests->PushBatch(batch);
            } else if (ImGui::IsMouseClicked(0) && !f.options.readOnly && io.KeyCtrl && linked) {
                s.connecting = hoveredPin->id; // Ctrl-drag carries all links to another socket.
                s.reconnecting = {};
                s.movingLinks = true;
            } else if (ImGui::IsMouseClicked(0) && !f.options.readOnly) {
                s.connecting = hoveredPin->id;
                s.reconnecting = {};
                s.movingLinks = false;
                if (hoveredPin->kind == PinKind::Input)
                    for (auto &link : f.graph.links)
                        if (link.to == hoveredPin->id) {
                            s.connecting = link.from;
                            s.reconnecting = link.id;
                            break;
                        }
            }
        }
        if (s.connecting) {
            auto *from = FindPin(f, s.connecting);
            const bool target = hoveredPin && hoveredPin->id != s.connecting;
            const auto verdict =
                target ? DragVerdict(f, hoveredPin->id) : ConnectionVerdict{};
            const bool convertible = verdict.allowed && verdict.match == ConnectionMatch::Convertible;
            if (from) {
                // The draft wire has the final shape: it leaves the socket like a link and lands on the target.
                const auto a = PinPosition(f, *from);
                const auto da = Direction(PinSide(f, *from));
                const auto q = target ? PinPosition(f, *hoveredPin) : io.MousePos;
                const auto dq = target ? Direction(PinSide(f, *hoveredPin)) : Mul(da, -1);
                const auto points = CurvePoints(f.style.linkStyle, a, da, q, dq);
                const ImVec4 color = !verdict.allowed ? f.style.error : convertible ? f.style.warning : PinColor(f, *from);
                if (!target)
                    DashedPolyline(dl, points, Color(color), f.style.linkWidth, 2, 1);
                else if (!verdict.allowed)
                    DashedPolyline(dl, points, Color(color), std::max(1.f, f.style.linkWidth * .75f), 1, 1);
                else if (convertible)
                    DashedPolyline(dl, points, Color(color), f.style.linkWidth, 3, 1);
                else
                    dl->AddPolyline(points.data(), int(points.size()), Color(color), ImDrawFlags_None,
                                    f.style.linkWidth);
                if (target && verdict.allowed) { // The socket that will receive the wire.
                    const float r = f.style.pinRadius * std::clamp(float(s.zoom), .8f, 2.f);
                    dl->AddCircle(q, r + 8, Color(WithAlpha(color, .35f)), 0, 3.f);
                }
            }
            if (target) {
                // The verdict sits next to the cursor, above the nodes, in the host's own words when given.
                const char *label = !verdict.allowed ? Tr(f, "node_editor.connect.rejected", "Rejected")
                                    : convertible   ? Tr(f, "node_editor.connect.convertible", "Convertible")
                                                    : Tr(f, "node_editor.connect.exact", "Exact");
                std::string text = verdict.reason.empty() ? std::string(label) : std::string(verdict.reason);
                auto *fg = ImGui::GetForegroundDrawList();
                fg->PushClipRect(f.min, f.max, true);
                const auto size = ImGui::CalcTextSize(text.c_str());
                const ImVec2 lo{io.MousePos.x + 14, io.MousePos.y + 18};
                const ImVec2 hi{lo.x + size.x + 16, lo.y + size.y + 6};
                const ImVec4 tone = !verdict.allowed ? f.style.error : convertible ? f.style.warning : f.style.success;
                fg->AddRectFilled(lo, hi, Color(f.style.surface), (hi.y - lo.y) / 2);
                fg->AddRect(lo, hi, Color(WithAlpha(tone, .8f)), (hi.y - lo.y) / 2, 0, 1.2f);
                fg->AddText({lo.x + 8, lo.y + 3}, Color(tone), text.c_str());
                fg->PopClipRect();
                if (ImGui::IsMouseReleased(0) && s.movingLinks) {
                    if (verdict.allowed) { // Every link of the source moves, as one operation.
                        std::vector<EditRequest> batch;
                        const auto op = s.nextOperation++;
                        for (auto &link : f.graph.links)
                            if (link.from == s.connecting || link.to == s.connecting) {
                                auto r = Request(f, EditKind::Reconnect);
                                r.link = link.id;
                                r.from = link.from == s.connecting ? hoveredPin->id : link.from;
                                r.to = link.to == s.connecting ? hoveredPin->id : link.to;
                                r.operation = op;
                                batch.push_back(r);
                            }
                        for (auto &r : batch)
                            r.operationSize = batch.size();
                        f.requests->PushBatch(batch);
                    }
                    s.connecting = {};
                    s.reconnecting = {};
                    s.movingLinks = false;
                } else if (ImGui::IsMouseReleased(0)) {
                    if (verdict.allowed) {
                        auto r = Request(f, s.reconnecting ? EditKind::Reconnect : EditKind::CreateLink);
                        r.from = s.connecting;
                        r.to = hoveredPin->id;
                        r.link = s.reconnecting;
                        if (from && from->kind == PinKind::Input)
                            std::swap(r.from, r.to);
                        f.requests->Push(r);
                    }
                    s.connecting = {};
                    s.reconnecting = {};
                }
            } else if (ImGui::IsMouseReleased(0) && !popup) {
                if (s.movingLinks) { // Moving links onto empty canvas does nothing.
                    s.connecting = {};
                    s.movingLinks = false;
                } else {
                    s.press = World(f, io.MousePos);
                    s.palette = true;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                s.connecting = {};
                s.reconnecting = {};
            }
        }
        bool overNode = false;
        for (auto &n : f.graph.nodes)
            if (Visible(f, n)) {
                auto a = Screen(f, Position(f, n));
                auto size = Size(f, n);
                if (Inside(io.MousePos, a, Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)})))
                    overNode = true;
            }
        if (available && s.gesture.empty() && !s.connecting) {
            if (ImGui::IsMouseDragging(f.options.keys.panButton)) {
                s.origin.x -= io.MouseDelta.x / s.zoom;
                s.origin.y -= io.MouseDelta.y / s.zoom;
            }
            if (io.MouseWheel) {
                auto target = s.zoom * std::pow(1.15, io.MouseWheel);
                double lo = std::max(.05, f.options.minimumZoom), hi = std::max(lo, f.options.maximumZoom);
                SetZoom(s, std::clamp(target, lo, hi), {io.MousePos.x - f.min.x, io.MousePos.y - f.min.y});
            }
            if (Chord(f.options.keys.frame)) {
                RememberView(s);
                FrameNodes(s, f.graph, s.selection, {f.max.x - f.min.x, f.max.y - f.min.y});
                RememberView(s);
            }
            if (Chord(f.options.keys.selectAll)) {
                s.selection.clear();
                for (auto &n : f.graph.nodes)
                    if (!Hidden(f, n))
                        s.selection.push_back(n.id);
            }
            if (Chord(f.options.keys.add) && !f.options.readOnly) {
                s.press = World(f, io.MousePos);
                s.palette = true;
            }
            const std::pair<ImGuiKeyChord, EditKind> keys[] = {
                {f.options.keys.remove, EditKind::DeleteNode},
                {f.options.keys.duplicate, EditKind::Duplicate},
                {f.options.keys.copy, EditKind::Copy},
                {f.options.keys.cut, EditKind::Cut},
                {f.options.keys.paste, EditKind::Paste},
                {f.options.keys.undo, EditKind::Undo},
                {f.options.keys.redo, EditKind::Redo},
                {f.options.keys.group, EditKind::Group},
                {f.options.keys.bypass, EditKind::Bypass},
                {f.options.keys.collapse, EditKind::Collapse}};
            for (auto [key, kind] : keys)
                if (Chord(key)) {
                    if (kind == EditKind::DeleteNode && s.selectedLink) {
                        auto r = Request(f, EditKind::DeleteLink);
                        r.link = s.selectedLink;
                        if (!f.options.readOnly)
                            f.requests->Push(r);
                    } else
                        QueueCommand(f, kind);
                }
            if (ImGui::IsMouseClicked(0) && !hoveredPin && !overNode) {
                bool hit = false;
                for (auto &link : f.graph.links) {
                    auto *a = FindPin(f, link.from);
                    auto *b = FindPin(f, link.to);
                    if (!a || !b)
                        continue;
                    auto points = LinkCurve(f, link, *a, *b);
                    for (std::size_t i = 1; i < points.size(); ++i)
                        if (SegmentDistance(io.MousePos, points[i - 1], points[i]) < 6) {
                            s.selectedLink = link.id;
                            hit = true;
                            if (ImGui::IsMouseDoubleClicked(0) && !f.options.readOnly) {
                                auto r = Request(f, EditKind::InsertReroute);
                                r.link = link.id;
                                r.after = World(f, io.MousePos);
                                f.requests->Push(r);
                            }
                            break;
                        }
                }
                if (!hit) {
                    if (!io.KeyShift && !io.KeyCtrl)
                        s.selection.clear();
                    s.selectedLink = {};
                    s.press = World(f, io.MousePos);
                    s.selecting = !io.KeyAlt;
                    s.cutting = io.KeyAlt && !f.options.readOnly;
                    s.lasso.clear();
                    s.lasso.push_back(s.press);
                }
            }
        }
        if (s.selecting || s.cutting) {
            auto p = World(f, io.MousePos);
            if (s.lasso.size() < 4096) {
                auto last = s.lasso.back();
                if (std::hypot(p.x - last.x, p.y - last.y) * s.zoom > 3)
                    s.lasso.push_back(p);
            }
            auto a = Screen(f, s.press), b = io.MousePos;
            if (f.options.lasso || s.cutting)
                for (std::size_t i = 1; i < s.lasso.size(); ++i)
                    dl->AddLine(Screen(f, s.lasso[i - 1]), Screen(f, s.lasso[i]), Color(f.style.accent));
            else {
                dl->AddRect({std::min(a.x, b.x), std::min(a.y, b.y)},
                            {std::max(a.x, b.x), std::max(a.y, b.y)}, Color(f.style.accent));
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                s.selecting = s.cutting = false;
                s.lasso.clear();
            } else if (ImGui::IsMouseReleased(0)) {
                if (s.selecting)
                    for (auto &n : f.graph.nodes)
                        if (!Hidden(f, n)) {
                            Point center{n.position.x + n.size.x / 2, n.position.y + n.size.y / 2};
                            bool selected = f.options.lasso
                                                ? editor::InPolygon(center, s.lasso)
                                                : n.position.x <= std::max(s.press.x, p.x) &&
                                                      n.position.x + n.size.x >= std::min(s.press.x, p.x) &&
                                                      n.position.y <= std::max(s.press.y, p.y) &&
                                                      n.position.y + n.size.y >= std::min(s.press.y, p.y);
                            if (selected)
                                Select(s, n.id, true, io.KeyCtrl);
                        }
                if (s.cutting) {
                    std::vector<EditRequest> batch;
                    auto op = s.nextOperation++;
                    for (auto &link : f.graph.links) {
                        auto *x = FindPin(f, link.from);
                        auto *y = FindPin(f, link.to);
                        if (!x || !y)
                            continue;
                        auto points = LinkCurve(f, link, *x, *y);
                        bool hit = false;
                        for (std::size_t q = 1; q < s.lasso.size(); ++q)
                            for (std::size_t i = 1; i < points.size(); ++i) {
                                auto a = Screen(f, s.lasso[q - 1]), b = Screen(f, s.lasso[q]);
                                auto cross = [](ImVec2 a, ImVec2 b, ImVec2 c) {
                                    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                                };
                                auto c = points[i - 1], d = points[i];
                                const bool crossing = cross(a, b, c) * cross(a, b, d) <= 0 &&
                                                      cross(c, d, a) * cross(c, d, b) <= 0 &&
                                                      std::max(std::min(a.x, b.x), std::min(c.x, d.x)) <=
                                                          std::min(std::max(a.x, b.x), std::max(c.x, d.x)) &&
                                                      std::max(std::min(a.y, b.y), std::min(c.y, d.y)) <=
                                                          std::min(std::max(a.y, b.y), std::max(c.y, d.y));
                                if (crossing || SegmentDistance(b, c, d) < 7)
                                    hit = true;
                            }
                        if (hit) {
                            auto r = Request(f, EditKind::DeleteLink);
                            r.link = link.id;
                            r.operation = op;
                            batch.push_back(r);
                        }
                    }
                    for (auto &r : batch)
                        r.operationSize = batch.size();
                    f.requests->PushBatch(batch);
                }
                s.selecting = s.cutting = false;
                s.lasso.clear();
            }
        }
        if (f.hovered && !f.blocked && !widgetActive && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            for (auto &n : f.graph.nodes)
                if (Visible(f, n)) {
                    auto a = Screen(f, Position(f, n));
                    auto size = Size(f, n);
                    if (Inside(io.MousePos, a, Add(a, {float(size.x * s.zoom), float(size.y * s.zoom)})) &&
                        !IsSelected(s, n.id))
                        Select(s, n.id);
                }
            s.press = World(f, io.MousePos);
            ImGui::OpenPopup("Node actions");
        }
        if (ImGui::BeginPopup("Node actions")) {
            if (ImGui::MenuItem("Add node", nullptr, false, !f.options.readOnly))
                s.palette = true;
            if (ImGui::MenuItem("Frame selection"))
                FrameNodes(s, f.graph, s.selection, {f.max.x - f.min.x, f.max.y - f.min.y});
            if (ImGui::MenuItem("Bookmark view"))
                AddBookmark(s, "View");
            for (std::size_t i = 0; i < s.bookmarks.size(); ++i) {
                auto label = std::string(s.bookmarks[i].label.data()) + "##bookmark" + std::to_string(i);
                if (ImGui::MenuItem(label.c_str()) && s.bookmarks[i].view.graph == f.graph.id)
                    RestoreView(s, s.bookmarks[i].view);
            }
            ImGui::Separator();
            ImGui::BeginDisabled(f.options.readOnly);
            if (ImGui::MenuItem("Insert active node into selected wire", nullptr, false,
                                bool(s.active) && bool(s.selectedLink)))
                InsertNode(f, s.active, s.selectedLink);
            if (ImGui::MenuItem("Connect selected", nullptr, false, s.selection.size() == 2))
                ConnectSelected(f);
            const std::pair<const char *, EditKind> commands[] = {{"Duplicate", EditKind::Duplicate},
                                                                  {"Delete", EditKind::DeleteNode},
                                                                  {"Copy", EditKind::Copy},
                                                                  {"Cut", EditKind::Cut},
                                                                  {"Paste", EditKind::Paste},
                                                                  {"Group", EditKind::Group},
                                                                  {"Ungroup", EditKind::Ungroup},
                                                                  {"Make subgraph", EditKind::MakeSubgraph},
                                                                  {"Collapse / expand", EditKind::Collapse},
                                                                  {"Lock / unlock", EditKind::Lock},
                                                                  {"Bypass", EditKind::Bypass},
                                                                  {"Save template", EditKind::SaveTemplate}};
            for (auto [label, kind] : commands)
                if (ImGui::MenuItem(label))
                    QueueCommand(f, kind);
            LayoutToolbar(f);
            if (f.options.canRun && ImGui::MenuItem("Run selection"))
                QueueCommand(f, EditKind::Run);
            if (f.options.canDebug && ImGui::MenuItem("Toggle breakpoint"))
                QueueCommand(f, EditKind::Breakpoint);
            if (f.options.canDebug && ImGui::MenuItem("Step"))
                QueueCommand(f, EditKind::Step);
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }
        if (f.requests->overflow) {
            dl->AddText(Add(f.min, {12, 12}), Color(f.style.warning),
                        Tr(f, "node_editor.request_buffer_full",
                           "Request buffer full: drain / enlarge it to finish the operation"));
        }
    }
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopID();
    f.visible = false;
}
} // namespace imkit::node_editor
