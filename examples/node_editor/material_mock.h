#pragma once
#include <imkit/node_editor.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Entirely host-owned GUI mock. No physical BRDF or shader compilation.
namespace material_mock {
namespace ne = imkit::node_editor;
inline const ne::SocketTypeView types[] = {
    {1, "Color", "Value", {.94f, .68f, .22f, 1}, ne::PinShape::Circle},
    {2, "Float", "Value", {.65f, .68f, .74f, 1}, ne::PinShape::Circle},
    {3, "Vector", "Value", {.35f, .56f, .95f, 1}, ne::PinShape::Diamond},
    {4, "Normal", "Direction", {.55f, .42f, .96f, 1}, ne::PinShape::Diamond},
    {5, "Image", "Resource", {.88f, .4f, .6f, 1}, ne::PinShape::Square},
    {6, "Closure", "Surface", {.3f, .8f, .57f, 1}, ne::PinShape::Triangle}};
inline constexpr const char *names[] = {"Image Texture", "RGB / Color", "Float Value", "Normal Map",
                                        "Diffuse BRDF",  "Glossy BRDF", "Glass BRDF",  "Principled Material",
                                        "Mix Closure",   "Add Closure", "Emission",    "Material Output"};
inline ne::ConnectionVerdict Compatibility(void *, std::uint64_t a, std::uint64_t b) {
    if (a == b)
        return {};
    if ((a == 1 && b == 2) || (a == 2 && b == 1))
        return {ne::ConnectionMatch::Convertible, "Color / scalar conversion"};
    if ((a == 3 && b == 4) || (a == 4 && b == 3))
        return {ne::ConnectionMatch::Convertible, "Normalize direction"};
    return {false, "No conversion between these types"};
}
struct Pin {
    ne::PinView view;
    std::string name;
    ne::PinValue value;
};
struct Node {
    ne::NodeView view;
    int type = 0;
    std::string name;
    ImVec4 preview{.4f, .6f, .8f, 1};
};
struct Data {
    std::vector<Node> nodes;
    std::vector<Pin> pins;
    std::vector<ne::LinkView> links;
};
struct Model {
    Data data;
    std::vector<Data> undo, redo;
    std::vector<ne::NodeView> nodes;
    std::vector<ne::PinView> pins;
    std::uint64_t next = 1, revision = 1;
    std::string message;
    Pin *Find(ne::PinId id) {
        for (auto &p : data.pins)
            if (p.view.id == id)
                return &p;
        return nullptr;
    }
    Node *Find(ne::NodeId id) {
        for (auto &n : data.nodes)
            if (n.view.id == id)
                return &n;
        return nullptr;
    }
    ne::GraphView View() {
        nodes.clear();
        pins.clear();
        for (auto &n : data.nodes) {
            auto v = n.view;
            v.title = n.name;
            nodes.push_back(v);
        }
        for (auto &p : data.pins) {
            auto v = p.view;
            v.label = p.name;
            pins.push_back(v);
        }
        ne::GraphView g{{77}, revision, nodes, pins, data.links};
        g.socketTypes = types;
        g.typeCompatibility = Compatibility;
        return g;
    }
    ne::PinId AddPin(ne::NodeId node, ne::PinKind kind, std::uint64_t type, std::string name,
                     std::uint64_t group = 0) {
        Pin p;
        p.view.id = {next++};
        p.view.node = node;
        p.view.kind = kind;
        p.view.side = kind == ne::PinKind::Input ? ne::Side::Left : ne::Side::Right;
        p.view.manualPosition = false;
        p.view.type = type;
        p.view.multiple = kind == ne::PinKind::Output;
        p.view.capabilities = {true, true, true, true, true, true, false, group, group ? 2 : 0, 16, {}};
        p.name = std::move(name);
        p.value.kind = type == 1                  ? ne::ValueKind::Color
                       : type == 2                ? ne::ValueKind::Float
                       : (type == 3 || type == 4) ? ne::ValueKind::Vector
                                                  : ne::ValueKind::None;
        p.value.number = {.45f, .65f, .85f, 1};
        auto id = p.view.id;
        data.pins.push_back(p);
        return id;
    }
    ne::NodeId Add(int type, ne::Point position) {
        Node n;
        n.view.id = {next++};
        n.type = type;
        n.name = names[type];
        n.view.position = position;
        n.view.size = {285, 360};
        n.view.inputs = {true, true, true, true, true, true, false, 0, 0, 16, {}};
        n.view.outputs = n.view.inputs;
        if (type == 8) {
            n.view.inputs.group = 1;
            n.view.inputs.minimum = 2;
        }
        auto id = n.view.id;
        data.nodes.push_back(n);
        auto in = [&](std::uint64_t t, const char *label, std::uint64_t group = 0) {
            return AddPin(id, ne::PinKind::Input, t, label, group);
        };
        auto out = [&](std::uint64_t t, const char *label) {
            return AddPin(id, ne::PinKind::Output, t, label);
        };
        switch (type) {
        case 0:
            in(5, "Image");
            in(3, "Coordinates");
            out(1, "Color");
            break;
        case 1:
            in(1, "Color value");
            out(1, "Color");
            break;
        case 2:
            in(2, "Value");
            out(2, "Value");
            break;
        case 3:
            in(1, "Color");
            in(2, "Strength");
            out(4, "Normal");
            break;
        case 4:
        case 5:
        case 6:
            in(1, "Base color");
            in(2, "Roughness");
            in(4, "Normal");
            out(6, "Closure");
            break;
        case 7:
            in(1, "Base color");
            in(2, "Metallic");
            in(2, "Roughness");
            in(4, "Normal");
            in(1, "Emission");
            out(6, "Closure");
            break;
        case 8:
            in(6, "Closure A", 1);
            in(6, "Closure B", 1);
            in(2, "Factor");
            out(6, "Closure");
            break;
        case 9:
            in(6, "Closure A");
            in(6, "Closure B");
            out(6, "Closure");
            break;
        case 10:
            in(1, "Color");
            in(2, "Strength");
            out(6, "Closure");
            break;
        case 11:
            in(6, "Surface");
            in(3, "Displacement");
            break;
        }
        Resize();
        return id;
    }
    void Resize() {
        for (auto &n : data.nodes) {
            auto count = std::count_if(data.pins.begin(), data.pins.end(),
                                       [&](auto &p) { return p.view.node == n.view.id && !p.view.hidden; });
            n.view.size.y = std::max(n.view.size.y, 300. + count * 44.);
        }
    }
    ne::PinId Socket(ne::NodeId id, ne::PinKind kind, std::uint64_t type = 0) {
        for (auto &p : data.pins)
            if (p.view.node == id && p.view.kind == kind && (!type || p.view.type == type))
                return p.view.id;
        return {};
    }
    void Init() {
        auto color = Add(1, {20, 30}), surface = Add(7, {375, 30}), output = Add(11, {735, 30});
        data.links.push_back(
            {{next++}, Socket(color, ne::PinKind::Output), Socket(surface, ne::PinKind::Input, 1)});
        data.links.push_back(
            {{next++}, Socket(surface, ne::PinKind::Output), Socket(output, ne::PinKind::Input, 6)});
        for (int i : {0, 2, 3, 4, 5, 6, 8, 9, 10})
            Add(i, {20. + (i % 3) * 355., 600. + (i / 3) * 470.});
        Evaluate();
    }
    bool Connect(ne::PinId a, ne::PinId b, ne::LinkId replacing = {}) {
        auto g = View();
        if (!ne::CanConnect(g, a, b, replacing).allowed)
            return false;
        if (Find(a)->view.kind == ne::PinKind::Input)
            std::swap(a, b);
        if (replacing)
            std::erase_if(data.links, [&](auto &l) { return l.id == replacing; });
        data.links.push_back({{next++}, a, b});
        return true;
    }
    bool ApplyOne(const ne::EditRequest &r) {
        auto *n = Find(r.node);
        auto *p = Find(r.from);
        switch (r.kind) {
        case ne::EditKind::CreateNode: {
            if (r.type >= std::size(names))
                return false;
            auto id = Add(int(r.type), r.after);
            if (r.from) {
                auto *source = Find(r.from);
                if (!source)
                    return false;
                auto kind =
                    source->view.kind == ne::PinKind::Input ? ne::PinKind::Output : ne::PinKind::Input;
                std::vector<ne::PinId> candidates;
                for (auto &candidate : data.pins)
                    if (candidate.view.node == id && candidate.view.kind == kind)
                        candidates.push_back(candidate.view.id);
                for (auto candidate : candidates)
                    if (Connect(r.from, candidate, r.link))
                        return true;
                return false;
            }
            return true;
        }
        case ne::EditKind::CreateLink:
        case ne::EditKind::Reconnect:
            return Connect(r.from, r.to, r.link);
        case ne::EditKind::InsertNode: {
            auto i =
                std::find_if(data.links.begin(), data.links.end(), [&](auto &l) { return l.id == r.link; });
            if (i == data.links.end())
                return false;
            auto old = *i;
            std::erase_if(data.links, [&](auto &l) { return l.id == old.id; });
            return Connect(old.from, r.from) && Connect(r.to, old.to);
        }
        case ne::EditKind::DeleteLink:
            std::erase_if(data.links, [&](auto &l) { return l.id == r.link; });
            return true;
        case ne::EditKind::Move:
            if (!n)
                return false;
            n->view.position = r.after;
            return true;
        case ne::EditKind::Resize:
            if (!n)
                return false;
            n->view.size = r.after;
            return true;
        case ne::EditKind::Rename:
            if (!n)
                return false;
            n->name = r.text.data();
            return true;
        case ne::EditKind::Collapse:
            if (!n)
                return false;
            n->view.collapsed = !n->view.collapsed;
            return true;
        case ne::EditKind::Lock:
            if (!n)
                return false;
            n->view.locked = !n->view.locked;
            return true;
        case ne::EditKind::DeleteNode: {
            if (!n)
                return false;
            std::vector<ne::PinId> ids;
            for (auto &pin : data.pins)
                if (pin.view.node == r.node)
                    ids.push_back(pin.view.id);
            std::erase_if(data.links, [&](auto &l) {
                return std::find(ids.begin(), ids.end(), l.from) != ids.end() ||
                       std::find(ids.begin(), ids.end(), l.to) != ids.end();
            });
            std::erase_if(data.pins, [&](auto &pin) { return pin.view.node == r.node; });
            std::erase_if(data.nodes, [&](auto &node) { return node.view.id == r.node; });
            return true;
        }
        default:
            break;
        }
        if (!ne::ValidatePinEdit(View(), r).allowed)
            return false;
        // Demonstrate host refusal: destructive edits never silently break existing wiring.
        if ((r.kind == ne::EditKind::DeletePin || r.kind == ne::EditKind::ChangePinType) &&
            ne::IsPinConnected(View(), r.from)) {
            message = "Disconnect this socket before deleting it or changing its type.";
            return false;
        }
        switch (r.kind) {
        case ne::EditKind::CreatePin: {
            auto id = AddPin(r.node, r.pinKind, r.type, r.text.data(), r.group);
            auto added = data.pins.back();
            data.pins.pop_back();
            int index = 0;
            auto it = data.pins.begin();
            for (; it != data.pins.end(); ++it)
                if (it->view.node == r.node && it->view.kind == r.pinKind &&
                    it->view.capabilities.group == r.group && index++ == r.index)
                    break;
            data.pins.insert(it, added);
            return bool(id);
        }
        case ne::EditKind::DeletePin:
            std::erase_if(data.pins, [&](auto &pin) { return pin.view.id == r.from; });
            return true;
        case ne::EditKind::RenamePin:
            p->name = r.text.data();
            return true;
        case ne::EditKind::ChangePinType:
            p->view.type = r.type;
            p->value.kind = r.type == 1                    ? ne::ValueKind::Color
                            : r.type == 2                  ? ne::ValueKind::Float
                            : (r.type == 3 || r.type == 4) ? ne::ValueKind::Vector
                                                           : ne::ValueKind::None;
            return true;
        case ne::EditKind::HidePin:
            p->view.hidden = r.index != 0;
            return true;
        case ne::EditKind::SetPinValue:
            p->value = r.value;
            return true;
        case ne::EditKind::SetPinMultiplicity:
            if (!r.index && std::count_if(data.links.begin(), data.links.end(),
                                          [&](auto &l) { return l.from == r.from || l.to == r.from; }) > 1)
                return false;
            p->view.multiple = r.index != 0;
            return true;
        case ne::EditKind::ExposePin:
            p->view.internal = p->view.internal ? ne::PinId{} : p->view.id;
            return true;
        case ne::EditKind::SetPinLimits:
            for (auto &pin : data.pins)
                if (pin.view.node == r.node && pin.view.kind == p->view.kind &&
                    pin.view.capabilities.group == p->view.capabilities.group) {
                    pin.view.capabilities.minimum = r.minimum;
                    pin.view.capabilities.maximum = r.maximum;
                }
            if (p->view.kind == ne::PinKind::Input) {
                n->view.inputs.minimum = r.minimum;
                n->view.inputs.maximum = r.maximum;
            } else {
                n->view.outputs.minimum = r.minimum;
                n->view.outputs.maximum = r.maximum;
            }
            return true;
        case ne::EditKind::ReorderPin: {
            std::vector<std::size_t> indices;
            for (std::size_t i = 0; i < data.pins.size(); ++i)
                if (data.pins[i].view.node == r.node && data.pins[i].view.kind == p->view.kind &&
                    data.pins[i].view.capabilities.group == p->view.capabilities.group)
                    indices.push_back(i);
            auto from = std::find_if(indices.begin(), indices.end(),
                                     [&](auto i) { return data.pins[i].view.id == r.from; });
            if (from == indices.end() || r.index < 0 || r.index >= int(indices.size()))
                return false;
            auto i = int(from - indices.begin());
            while (i != r.index) {
                int j = i + (i < r.index ? 1 : -1);
                std::swap(data.pins[indices[i]], data.pins[indices[j]]);
                i = j;
            }
            return true;
        }
        default:
            return false;
        }
    }
    bool Apply(std::span<const ne::EditRequest> requests) {
        const auto base = revision;
        bool result = true;
        for (std::size_t i = 0; i < requests.size();) {
            auto &r = requests[i];
            auto size = r.operationSize;
            if (!size || size > requests.size() - i)
                return false;
            if (r.phase != ne::Phase::Commit) {
                i += size;
                continue;
            }
            if (r.revision != base || r.graph != ne::GraphId{77}) {
                message = "Stale operation rejected";
                result = false;
                i += size;
                continue;
            }
            if (r.kind == ne::EditKind::Undo || r.kind == ne::EditKind::Redo) {
                auto &from = r.kind == ne::EditKind::Undo ? undo : redo;
                auto &to = r.kind == ne::EditKind::Undo ? redo : undo;
                if (!from.empty()) {
                    to.push_back(data);
                    data = from.back();
                    from.pop_back();
                    ++revision;
                }
                i += size;
                continue;
            }
            Data before = data;
            bool ok = true;
            for (std::size_t j = 0; j < size && ok; ++j) {
                auto &e = requests[i + j];
                ok = e.operation == r.operation && e.revision == r.revision && e.graph == r.graph &&
                     e.phase == r.phase && e.operationSize == size && ApplyOne(e);
            }
            if (ok) {
                undo.push_back(std::move(before));
                redo.clear();
                ++revision;
                message.clear();
                Resize();
            } else {
                data = std::move(before);
                if (message.empty())
                    message = "Operation rejected; graph unchanged";
                result = false;
            }
            i += size;
        }
        Evaluate();
        return result;
    }
    void Evaluate() {
        // Bounded CPU color propagation solely to visualize changing host results.
        for (auto &n : data.nodes)
            n.preview = {.18f, .24f, .33f, 1};
        for (int pass = 0; pass < 4; ++pass)
            for (auto &n : data.nodes) {
                ImVec4 color{.1f, .13f, .18f, 1};
                int count = 1;
                for (auto &p : data.pins)
                    if (p.view.node == n.view.id && p.view.kind == ne::PinKind::Input) {
                        ImVec4 c{p.value.number[0], p.value.number[1], p.value.number[2], 1};
                        if (p.value.kind == ne::ValueKind::Float)
                            c = {p.value.number[0], p.value.number[0], p.value.number[0], 1};
                        for (auto &l : data.links)
                            if (l.to == p.view.id)
                                if (auto *source = Find(l.from))
                                    if (auto *node = Find(source->view.node))
                                        c = node->preview;
                        color.x += c.x;
                        color.y += c.y;
                        color.z += c.z;
                        ++count;
                    }
                n.preview = {color.x / count, color.y / count, color.z / count, 1};
            }
    }
};
inline void DrawPreview(void *user, ImVec2 size, bool) {
    auto c = *static_cast<ImVec4 *>(user);
    auto a = ImGui::GetCursorScreenPos();
    auto *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, {a.x + size.x, a.y + size.y}, IM_COL32(18, 22, 28, 255), 5);
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 48; ++x) {
            float u = (x - 23.5f) / 23.5f, v = (y - 11.5f) / 11.5f;
            float d = u * u + v * v;
            if (d > 1)
                continue;
            float light = .2f + .8f * std::sqrt(1 - d);
            ImVec4 shade{c.x * light, c.y * light, c.z * light, 1};
            dl->AddRectFilled({a.x + x * size.x / 48, a.y + y * size.y / 24},
                              {a.x + (x + 1) * size.x / 48, a.y + (y + 1) * size.y / 24},
                              ImGui::ColorConvertFloat4ToU32(shade));
        }
    ImGui::Dummy(size);
}
struct Page {
    Model model;
    ne::EditorState state;
    std::array<ne::EditRequest, 256> storage{};
    struct Candidate {
        Page *page;
        int type;
    };
    std::array<Candidate, 12> candidates{};
    Page() {
        model.Init();
        state.Reserve(128);
        state.graph = {77};
        state.zoom = .85;
        state.active = model.data.nodes[1].view.id;
        state.selection = {state.active};
    }
    static bool Compatible(void *user, ne::PinId pin) {
        auto &c = *static_cast<Candidate *>(user);
        auto *p = c.page->model.Find(pin);
        if (!p)
            return false;
        Model prototype;
        auto id = prototype.Add(c.type, {});
        for (auto &candidate : prototype.data.pins)
            if (candidate.view.node == id && candidate.view.kind != p->view.kind) {
                auto a = p->view.kind == ne::PinKind::Output ? p->view.type : candidate.view.type;
                auto b = p->view.kind == ne::PinKind::Input ? p->view.type : candidate.view.type;
                if (Compatibility(nullptr, a, b).allowed)
                    return true;
            }
        return false;
    }
    void Draw(const imkit::Theme &theme) {
        auto g = model.View();
        ne::RequestBuffer out{storage};
        auto style = ne::MakeNodeStyle(theme);
        ne::EditorFrame controls;
        controls.graph = g;
        controls.state = &state;
        controls.requests = &out;
        controls.style = style;
        if (ImGui::SmallButton("+ Add node")) {
            state.palette = true;
            state.press = {state.origin.x + 100, state.origin.y + 80};
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Undo"))
            ne::QueueCommand(controls, ne::EditKind::Undo);
        ImGui::SameLine();
        if (ImGui::SmallButton("Redo"))
            ne::QueueCommand(controls, ne::EditKind::Redo);
        ImGui::SameLine();
        if (ImGui::SmallButton("Frame all"))
            ne::FrameNodes(state, g, {},
                           {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y});
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ne::LayoutToolbar(controls);
        ImGui::TextDisabled("GUI mock / CPU preview | Drag sockets to connect | Right-click a row to edit | "
                            "Middle drag to pan");
        if (!model.message.empty())
            ImGui::TextWrapped("%s", model.message.c_str());
        auto size = ImGui::GetContentRegionAvail();
        size.x = std::max(200.f, size.x - 300);
        ne::EditorOptions options;
        options.size = size;
        auto f = ne::BeginEditor("material", g, state, out, style, options);
        ne::DrawLinks(f);
        for (auto &n : model.data.nodes)
            if (ne::BeginNode(f, n.view.id)) {
                for (auto &p : model.data.pins)
                    if (p.view.node == n.view.id) {
                        ne::PinRowOptions opt;
                        opt.value = p.value;
                        ne::PinRow(f, p.view.id, opt);
                    }
                if (state.zoom >= style.detailZoom && !n.view.collapsed) {
                    ne::PinAddRow(f, n.view.id, ne::PinKind::Input, n.view.inputs.group);
                    ne::PreviewOutput preview;
                    preview.title = "Host CPU mock";
                    preview.status = ne::PreviewStatus::Ready;
                    preview.draw = DrawPreview;
                    preview.user = &n.preview;
                    ne::Preview(f, {&preview, 1});
                }
                ne::EndNode(f);
            }
        std::array<ne::PaletteEntry, 12> entries{};
        for (int i = 0; i < 12; ++i) {
            candidates[i] = {this, i};
            entries[i] = {std::uint64_t(i),     names[i],       "Material GUI mock",
                          "Host-owned example", &candidates[i], Compatible};
        }
        ne::NodePalette(f, entries);
        ne::EndEditor(f);
        ImGui::SameLine();
        ImGui::BeginChild("Material inspector", {0, size.y}, ImGuiChildFlags_Borders);
        ne::NodeInspector(f, state.active);
        ImGui::EndChild();
        for (auto &n : model.data.nodes) {
            ne::PreviewOutput preview;
            preview.title = "Host CPU mock";
            preview.status = ne::PreviewStatus::Ready;
            preview.draw = DrawPreview;
            preview.user = &n.preview;
            ne::DrawDetachedPreviews(f, n.view.id, {&preview, 1});
        }
        model.Apply(out.Requests());
    }
};
} // namespace material_mock
