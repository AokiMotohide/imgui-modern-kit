#include "gallery.h"

#include <imgui.h>
#include <imkit/imkit.h>

namespace imkit::gallery {
namespace {

void BeginRow() {
    ImGui::TableNextRow();
}

void BeginCell(const char* id) {
    ImGui::TableNextColumn();
    ImGui::PushID(id);
}

void EndCell() {
    ImGui::PopID();
}

void ShowButtonRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-button");
    if (ImGui::Button("Button")) {
        ++state.standard.buttonClicks;
    }
    ImGui::Text("Clicks: %d", state.standard.buttonClicks);
    EndCell();

    BeginCell("wrapper-button");
    if (imkit::Button("Button")) {
        ++state.wrapper.buttonClicks;
    }
    ImGui::Text("Clicks: %d", state.wrapper.buttonClicks);
    EndCell();
}

void ShowCheckboxRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-checkbox");
    ImGui::Checkbox("Checkbox", &state.standard.checked);
    EndCell();

    BeginCell("wrapper-checkbox");
    imkit::Checkbox("Checkbox", &state.wrapper.checked);
    EndCell();
}

void ShowSliderRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-slider");
    ImGui::SliderFloat("SliderFloat", &state.standard.value, 0.0F, 1.0F);
    EndCell();

    BeginCell("wrapper-slider");
    imkit::SliderFloat("SliderFloat", &state.wrapper.value, 0.0F, 1.0F);
    EndCell();
}

void ShowInputTextRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-input");
    ImGui::InputText("InputText", state.standard.text, sizeof(state.standard.text));
    EndCell();

    BeginCell("wrapper-input");
    imkit::InputText("InputText", state.wrapper.text, sizeof(state.wrapper.text));
    EndCell();
}

void ShowSelectableRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-selectable");
    if (ImGui::Selectable("Selectable", state.standard.selected)) {
        state.standard.selected = !state.standard.selected;
    }
    EndCell();

    BeginCell("wrapper-selectable");
    if (imkit::Selectable("Selectable", state.wrapper.selected)) {
        state.wrapper.selected = !state.wrapper.selected;
    }
    EndCell();
}

void ShowProgressBarRow(GalleryState& state) {
    BeginRow();
    BeginCell("standard-progress");
    ImGui::ProgressBar(state.standard.value);
    EndCell();

    BeginCell("wrapper-progress");
    imkit::ProgressBar(state.wrapper.value);
    EndCell();
}

}  // namespace

void Show(GalleryState& state) {
    ImGui::Begin("ImKit Development Gallery");

    ImGui::TextUnformatted("Status: Foundation only / Design not implemented");
    ImGui::Text("Dear ImGui version: %s", ImGui::GetVersion());

    if (ImGui::Button("Reset")) {
        state.standard = ColumnState{};
        state.wrapper = ColumnState{};
    }
    ImGui::SameLine();
    ImGui::Checkbox("Show Dear ImGui Demo", &state.showDearImGuiDemo);

    if (ImGui::BeginTable(
            "comparison",
            2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Standard ImGui");
        ImGui::TableSetupColumn("ImKit wrapper");
        ImGui::TableHeadersRow();

        ShowButtonRow(state);
        ShowCheckboxRow(state);
        ShowSliderRow(state);
        ShowInputTextRow(state);
        ShowSelectableRow(state);
        ShowProgressBarRow(state);

        ImGui::EndTable();
    }

    ImGui::End();

    if (state.showDearImGuiDemo) {
        ImGui::ShowDemoWindow(&state.showDearImGuiDemo);
    }
}

}  // namespace imkit::gallery
