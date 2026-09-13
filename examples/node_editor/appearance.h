#pragma once
#include <imkit/node_editor.h>
#include <algorithm>

namespace node_gallery {
struct BodyTextScope {
    explicit BodyTextScope(ImVec4 color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
    ~BodyTextScope() { ImGui::PopStyleColor(); }
};
// The example owns the draft and applies it after all borrowed graph views expire.
struct AppearanceEditor {
    imkit::node_editor::NodeId node{};
    ImVec4 draft{};
    bool Draw(const imkit::node_editor::EditorFrame& frame,
              const imkit::node_editor::NodeView* view, ImVec4 color, ImVec4 accent) {
        if (!view) return false;
        ImGui::PushID(static_cast<int>(view->id.value));
        bool locked = frame.options.readOnly || view->locked;
        auto parent = view->parent;
        for (std::size_t i = 0; parent && i < frame.graph.nodes.size(); ++i) {
            auto found = std::find_if(frame.graph.nodes.begin(), frame.graph.nodes.end(),
                                      [&](const auto& n) { return n.id == parent; });
            if (found == frame.graph.nodes.end()) break;
            locked |= found->locked;
            parent = found->parent;
        }
        ImGui::BeginDisabled(locked);
        if (ImGui::Button("Node color...")) {
            node = view->id;
            draft = color.w > 0 ? color : accent;
            ImGui::OpenPopup("Node color");
        }
        bool apply = false;
        if (ImGui::BeginPopup("Node color")) {
            ImGui::ColorPicker3("Color", &draft.x);
            if (ImGui::Button("Apply")) {
                draft.w = 1;
                apply = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Use theme")) {
                draft = {};
                apply = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::EndDisabled();
        ImGui::PopID();
        return apply;
    }
};
template<class Model>
bool ApplyColor(Model& model, imkit::node_editor::NodeId id, ImVec4 color) {
    auto* node = model.Find(id);
    if (!node || node->view.locked) return false;
    model.undo.push_back(model.data);
    if (model.undo.size() > 64) model.undo.erase(model.undo.begin());
    model.redo.clear();
    node->color = color;
    ++model.revision;
    return true;
}
template<class Model>
void StyleNodes(std::span<imkit::node_editor::NodeView> views, Model& model,
                const imkit::node_editor::NodeStyle& base) {
    for (auto& view : views)
        if (auto* node = model.Find(view.id); node && node->color.w > 0) {
            node->customStyle = base;
            node->customStyle.header = node->color;
            const auto c = node->color;
            node->customStyle.text = .2126f * c.x + .7152f * c.y + .0722f * c.z > .55f
                                        ? ImVec4{.04f, .04f, .04f, 1} : ImVec4{1, 1, 1, 1};
            view.style = &node->customStyle;
        }
}
}
