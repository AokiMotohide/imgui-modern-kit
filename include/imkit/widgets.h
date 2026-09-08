#pragma once

#include <cstddef>

#include <imgui.h>

namespace imkit {

bool Button(
    const char* label,
    const ImVec2& size = ImVec2(0, 0));

bool Checkbox(
    const char* label,
    bool* value);

bool SliderFloat(
    const char* label,
    float* value,
    float minimum,
    float maximum,
    const char* format = "%.3f",
    ImGuiSliderFlags flags = 0);

bool InputText(
    const char* label,
    char* buffer,
    std::size_t bufferSize,
    ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr,
    void* userData = nullptr);

bool Selectable(
    const char* label,
    bool selected = false,
    ImGuiSelectableFlags flags = 0,
    const ImVec2& size = ImVec2(0, 0));

void ProgressBar(
    float fraction,
    const ImVec2& size = ImVec2(-1, 0),
    const char* overlay = nullptr);

}  // namespace imkit

