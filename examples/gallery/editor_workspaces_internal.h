#pragma once
#include <imkit/widgets.h>
#include "editor_workspaces.h"
#include <algorithm>
#include <cstdio>

namespace imkit::gallery::detail {
template<std::size_t N>
int FilterProperties(editor::PropertyView (&rows)[N], const char *search) {
    ImGuiTextFilter filter(search);
    int count = 0;
    for (auto row : rows) {
        char text[512];
        std::snprintf(text, sizeof(text), "%s %s", row.category, row.label);
        if (filter.PassFilter(text))
            rows[count++] = row;
    }
    return count;
}

struct PropertyRows {
    std::span<const editor::PropertyView> rows;
    static std::span<const editor::PropertyView> Query(void *user, int first, int count, std::string_view) {
        auto r = static_cast<PropertyRows *>(user)->rows;
        auto begin = (std::min)(r.size(), static_cast<std::size_t>((std::max)(0, first)));
        return r.subspan(begin, (std::min)(r.size() - begin, static_cast<std::size_t>((std::max)(0, count))));
    }
};

struct CompactEditorScope {
    bool compact = ImGui::GetIO().DisplaySize.y < 900;
    CompactEditorScope() {
        if (compact) {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6, 2});
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6, 4});
        }
    }
    ~CompactEditorScope() {
        if (compact)
            ImGui::PopStyleVar(2);
    }
};

void PropertyLanguage(editor::PropertyState &state, bool japanese);
void TransportLanguage(editor::TimeState &time, bool japanese);
void ApplyGizmoPreview(preview::Mesh &mesh, const cg::ViewportState &viewport);
double &TransformComponent(cg::Transform &transform, int component);
editor::StableId ObjectPropertyId(const EditorWorkspaces &state, editor::StableId object, int component);
editor::AssetProvider Assets(EditorWorkspaces &s);
void CurveLanguage(editor::CurveState &state, bool japanese);
editor::CurveProvider Curves(EditorWorkspaces &s);
editor::CurveProvider ColorCurves(EditorWorkspaces &s);
void PaneDivider(const char *id, float &width, float height, float total);
void Divider(const char *id, float &ratio, float total);
void Options(EditorWorkspaces &s);
} // namespace imkit::gallery::detail
