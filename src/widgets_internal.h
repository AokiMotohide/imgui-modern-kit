#pragma once
#include <imkit/part_style.h>
#include <imkit/widgets.h>
#include "part_style_internal.h"
#include <algorithm>
#include <cstdarg>

// Shared helpers for widgets.cpp, widgets_controls.cpp and widgets_containers.cpp.
namespace imkit::detail {
// Pushes the ImGui colours and style variables a widget style overrides, and pops them on destruction.
// Used while individual widgets are migrated to custom drawing (Wave 2), so L1/L2 values already take effect.
class WidgetStyleScope {
  public:
    template <class Style>
    WidgetStyleScope(Part part, const Style &style) {
        const PartValues call = ToPartValues(style);
        const PartValues *callPtr = call.Empty() ? nullptr : &call;
        if (!PartCustomized(part, callPtr))
            return;
        const ResolvedPartStyle r = ResolvePartStyle(part, callPtr);
        const auto pushCol = [&](PartState s, PartColor c) {
            const ImGuiCol col = ImGuiColorOf(part, s, c);
            if (col >= 0 && r.colorLayers[static_cast<std::size_t>(s)][static_cast<std::size_t>(c)] != PartLayer::Default) {
                ImGui::PushStyleColor(col, r.Color(s, c));
                ++colors_;
            }
        };
        pushCol(PartState::Rest, PartColor::Fill);
        pushCol(PartState::Hover, PartColor::Fill);
        pushCol(PartState::Pressed, PartColor::Fill);
        pushCol(PartState::Selected, PartColor::Fill);
        pushCol(PartState::Rest, PartColor::Border);
        pushCol(PartState::Rest, PartColor::Text);
        pushCol(PartState::Disabled, PartColor::Text);
        pushCol(PartState::Rest, PartColor::Mark);
        pushCol(PartState::Pressed, PartColor::Mark);

        const auto &map = ImGuiMapOf(part);
        const auto setVar = [&](PartVar v) {
            return r.varLayers[static_cast<std::size_t>(v)] != PartLayer::Default;
        };
        if (map.roundingVar >= 0 && setVar(PartVar::Radius)) {
            ImGui::PushStyleVar(map.roundingVar, r.Var(PartVar::Radius));
            ++vars_;
        }
        if (map.borderVar >= 0 && setVar(PartVar::BorderWidth)) {
            ImGui::PushStyleVar(map.borderVar, r.Var(PartVar::BorderWidth));
            ++vars_;
        }
        if (map.paddingVar == ImGuiStyleVar_FramePadding &&
            (setVar(PartVar::PaddingX) || setVar(PartVar::PaddingY) || setVar(PartVar::Height))) {
            ImVec2 pad = ImGui::GetStyle().FramePadding;
            if (setVar(PartVar::PaddingX))
                pad.x = r.Var(PartVar::PaddingX);
            if (setVar(PartVar::PaddingY))
                pad.y = r.Var(PartVar::PaddingY);
            else if (setVar(PartVar::Height))
                pad.y = std::max(0.f, (r.Var(PartVar::Height) - ImGui::GetFontSize()) * .5f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, pad);
            ++vars_;
        }
        if ((part == Part::Slider || part == Part::Scrollbar) && setVar(PartVar::ThumbSize)) {
            ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, r.Var(PartVar::ThumbSize));
            ++vars_;
        }
    }
    ~WidgetStyleScope() {
        if (vars_ > 0)
            ImGui::PopStyleVar(vars_);
        if (colors_ > 0)
            ImGui::PopStyleColor(colors_);
    }
    WidgetStyleScope(const WidgetStyleScope &) = delete;
    WidgetStyleScope &operator=(const WidgetStyleScope &) = delete;

  private:
    int colors_ = 0;
    int vars_ = 0;
};

// Selected rows get an accent bar on the leading edge; tabs get an inset pill underneath.
void DrawSelectionMark(bool selected, bool underline = false);
} // namespace imkit::detail
