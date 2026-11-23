#include "widgets_internal.h"
#include "draw_internal.h"
#include "imgui_internal_compat.h"
#include <imkit/theme.h>
#include <algorithm>
#include <string>

namespace imkit {
namespace detail {
bool IsCurrentItemDisabled() {
    return (GImGui->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
}
void DrawSelectionMark(bool selected, bool underline) {
    if (!selected)
        return;
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const float height = b.y - a.y;
    auto *draw = ImGui::GetWindowDrawList();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_CheckMark);
    if (underline) {
        const float inset = std::min(10.f, (b.x - a.x) * .2f);
        draw->AddRectFilled({a.x + inset, b.y - 3}, {b.x - inset, b.y - 1}, color, 1.f);
    } else {
        // Drawn in the padding to the left of the row so it never overlaps the label.
        draw->AddRectFilled({a.x - 6, a.y + height * .22f}, {a.x - 3, b.y - height * .22f}, color, 1.5f);
    }
}
} // namespace detail

void SetItemTooltipV(const char *format, va_list args) {
    if (detail::ClassicLook()) {
        ImGui::SetItemTooltipV(format, args);
        return;
    }
    const ResolvedPartStyle r = ResolvePartStyle(Part::Tooltip);
    const ImVec4 fill = r.Color(PartState::Rest, PartColor::Fill);
    const ImVec4 border = r.Color(PartState::Rest, PartColor::Border);
    const ImVec4 text = r.Color(PartState::Rest, PartColor::Text);
    const float radius = r.Var(PartVar::Radius);
    const float borderW = r.Var(PartVar::BorderWidth);
    const float padX = r.Var(PartVar::PaddingX);
    const float padY = r.Var(PartVar::PaddingY);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, fill);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleColor(ImGuiCol_Text, text);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, radius);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, borderW);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padX, padY});
    ImGui::SetItemTooltipV(format, args);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}
void SetItemTooltip(const char *format, ...) {
    va_list args;
    va_start(args, format);
    imkit::SetItemTooltipV(format, args);
    va_end(args);
}
void SetItemTooltip(const PartStyle &style, const char *format, ...) {
    detail::WidgetStyleScope scope(Part::Tooltip, style);
    va_list args;
    va_start(args, format);
    imkit::SetItemTooltipV(format, args);
    va_end(args);
}

void ShowStyleEditor(Theme *theme, bool *open) {
    if (open && !*open)
        return;
    if (open) {
        if (!ImGui::Begin("ImKit Style Editor", open)) {
            ImGui::End();
            return;
        }
    }
    static int partIndex = 0;
    const char *currentName = PartName(static_cast<Part>(partIndex)).data();
    if (imkit::BeginCombo("Part", currentName)) {
        for (int i = 0; i < static_cast<int>(PartCount); ++i) {
            const bool selected = i == partIndex;
            if (imkit::Selectable(PartName(static_cast<Part>(i)).data(), selected))
                partIndex = i;
        }
        imkit::EndCombo();
    }
    const Part part = static_cast<Part>(partIndex);
    const ResolvedPartStyle resolved = ResolvePartStyle(part);
    const auto layerName = [](PartLayer l) {
        switch (l) {
        case PartLayer::Theme: return "Theme";
        case PartLayer::NamedStyle: return "Scope";
        case PartLayer::Pushed: return "Push";
        case PartLayer::Call: return "Call";
        default: return "Default";
        }
    };
    if (ImGui::BeginTable("##part-vars", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Property");
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("Layer");
        ImGui::TableHeadersRow();
        for (std::size_t v = 0; v < PartVarCount; ++v) {
            const auto var = static_cast<PartVar>(v);
            float val = resolved.vars[v];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(PartVarName(var).data());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(static_cast<int>(v));
            if (theme && imkit::DragFloat("##val", &val, .5f, 0.f, 400.f, "%.1f")) {
                theme->partOverrides.SetVar(part, var, val);
                detail::MutablePartTheme().overrides.SetVar(part, var, val);
            } else if (!theme) {
                ImGui::Text("%.1f", val);
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(layerName(resolved.varLayers[v]));
        }
        ImGui::EndTable();
    }
    if (theme && imkit::Button("Copy theme text")) {
        const std::string text = SerializeTheme(*theme);
        ImGui::SetClipboardText(text.c_str());
    }
    if (open)
        ImGui::End();
}
} // namespace imkit
